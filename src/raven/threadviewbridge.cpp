// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "threadviewbridge.h"
#include "constants.h"
#include "dbmanager.h"
#include "ravendaemoninterface.h"

#include <QDateTime>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QWindow>

ThreadViewBridge::ThreadViewBridge(QObject *parent)
    : QObject(parent)
    , m_db(DBManager::openDatabase(QStringLiteral("threadviewbridge")))
{
}

QString ThreadViewBridge::themeCSS() const
{
    return m_themeCSS;
}

void ThreadViewBridge::setThemeCSS(const QString &css)
{
    if (m_themeCSS != css) {
        m_themeCSS = css;
        Q_EMIT themeChanged();
    }
}

QString ThreadViewBridge::currentThreadId() const
{
    return m_currentThreadId;
}

void ThreadViewBridge::loadThread(const QString &threadId, const QString &accountId, const QString &folderRole)
{
    // Clear previous data
    qDeleteAll(m_messages);
    m_messages.clear();
    m_messageContents.clear();
    m_currentThreadId = threadId;
    m_currentAccountId = accountId;
    m_currentFolderRole = folderRole;

    if (threadId.isEmpty()) {
        Q_EMIT threadLoaded(QStringLiteral("[]"));
        return;
    }

    if (!m_db.isOpen()) {
        qWarning() << "ThreadViewBridge: Database not open";
        Q_EMIT threadLoaded(QStringLiteral("[]"));
        return;
    }

    // Fetch messages with body content
    auto messagesWithBody = Message::fetchByThreadWithBody(m_db, threadId, accountId, this);

    for (const auto &mwb : messagesWithBody) {
        m_messages.append(mwb.message);
        m_messageContents.append(mwb.bodyContent);
    }

    Q_EMIT threadLoaded(getMessagesJson());
}

QString ThreadViewBridge::getMessagesJson() const
{
    QJsonArray messagesArray;

    for (int i = 0; i < m_messages.size(); ++i) {
        messagesArray.append(messageToJson(m_messages.at(i), m_messageContents.at(i)));
    }

    return QString::fromUtf8(QJsonDocument(messagesArray).toJson(QJsonDocument::Compact));
}

QString ThreadViewBridge::getAttachmentsJson(const QString &messageId) const
{
    QList<File*> files = File::fetchByMessage(m_db, messageId, nullptr);
    QJsonArray attachmentsArray;

    for (File *file : files) {
        attachmentsArray.append(fileToJson(file));
        delete file;
    }

    return QString::fromUtf8(QJsonDocument(attachmentsArray).toJson(QJsonDocument::Compact));
}

void ThreadViewBridge::openAttachment(const QString &fileId)
{
    QString path = downloadAttachment(fileId);
    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void ThreadViewBridge::saveAttachment(const QString &fileId)
{
    QString sourcePath = downloadAttachment(fileId);
    if (sourcePath.isEmpty()) {
        return;
    }

    File *file = File::fetchById(m_db, fileId, nullptr);
    if (!file) {
        return;
    }

    QString fileName = file->fileName();
    delete file;

    // Get window handle for the portal
    QString windowHandle;
    QWindow *activeWindow = QGuiApplication::focusWindow();
    if (activeWindow) {
        // Wayland: use wayland handle, X11: use X11 window id
        windowHandle = QStringLiteral("wayland:") + activeWindow->property("_q_waylandHandle").toString();
        if (windowHandle == QStringLiteral("wayland:")) {
            // Fallback for X11
            windowHandle = QStringLiteral("x11:") + QString::number(activeWindow->winId());
        }
    }

    // Use XDG Desktop Portal SaveFile via D-Bus
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.FileChooser"),
        QStringLiteral("SaveFile")
    );

    // Build options map
    QVariantMap options;
    options[QStringLiteral("handle_token")] = QStringLiteral("raven_save_%1").arg(QDateTime::currentMSecsSinceEpoch());
    options[QStringLiteral("current_name")] = fileName;
    options[QStringLiteral("current_folder")] = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation).toUtf8();

    msg << windowHandle << tr("Save Attachment") << options;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(pendingCall, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, sourcePath](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QDBusObjectPath> reply = *w;
        w->deleteLater();

        if (reply.isError()) {
            qWarning() << "Portal SaveFile call failed:" << reply.error().message();
            return;
        }

        QString requestPath = reply.value().path();

        // Connect to the Response signal on the request object
        QDBusConnection::sessionBus().connect(
            QStringLiteral("org.freedesktop.portal.Desktop"),
            requestPath,
            QStringLiteral("org.freedesktop.portal.Request"),
            QStringLiteral("Response"),
            this,
            SLOT(handlePortalResponse(uint, QVariantMap))
        );

        // Store the source path for when the response comes back
        m_pendingPortalSaves[requestPath] = sourcePath;
    });
}

QString ThreadViewBridge::downloadAttachment(const QString &fileId)
{
    File *file = File::fetchById(m_db, fileId, nullptr);
    if (!file) {
        return {};
    }

    QString filePath = file->filePath();

    if (file->downloaded() && QFile::exists(filePath)) {
        delete file;
        Q_EMIT attachmentDownloaded(fileId, filePath);
        return filePath;
    }

    delete file;

    // Fetch via D-Bus
    QString downloadedPath = fetchAttachmentViaDbus(fileId);
    if (!downloadedPath.isEmpty()) {
        Q_EMIT attachmentDownloaded(fileId, downloadedPath);
    }

    return downloadedPath;
}

void ThreadViewBridge::openExternalUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

QJsonObject ThreadViewBridge::messageToJson(Message *msg, const QString &content) const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = msg->id();
    obj[QStringLiteral("subject")] = msg->subject();
    obj[QStringLiteral("from")] = QStringLiteral("%1 <%2>").arg(msg->from()->name(), msg->from()->email());
    obj[QStringLiteral("fromName")] = msg->from()->name();
    obj[QStringLiteral("fromEmail")] = msg->from()->email();
    obj[QStringLiteral("to")] = formatContacts(msg->to());
    obj[QStringLiteral("cc")] = formatContacts(msg->cc());
    obj[QStringLiteral("bcc")] = formatContacts(msg->bcc());
    obj[QStringLiteral("date")] = msg->date().toString(Qt::ISODate);
    obj[QStringLiteral("dateFormatted")] = QLocale().toString(msg->date(), QLocale::ShortFormat);
    obj[QStringLiteral("isPlaintext")] = msg->plaintext();
    obj[QStringLiteral("unread")] = msg->unread();
    obj[QStringLiteral("starred")] = msg->starred();
    obj[QStringLiteral("snippet")] = msg->snippet();

    QString processedContent = msg->plaintext() ? content : processInlineImages(content, msg->id());
    obj[QStringLiteral("content")] = processedContent;

    QList<File*> files = File::fetchByMessage(m_db, msg->id(), nullptr);
    QJsonArray attachmentsArray;
    int nonInlineCount = 0;

    for (File *file : files) {
        attachmentsArray.append(fileToJson(file));
        if (!file->isInline()) {
            nonInlineCount++;
        }
        delete file;
    }

    obj[QStringLiteral("attachments")] = attachmentsArray;
    obj[QStringLiteral("attachmentCount")] = nonInlineCount;

    return obj;
}

QJsonObject ThreadViewBridge::fileToJson(File *file) const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = file->id();
    obj[QStringLiteral("filename")] = file->fileName();
    obj[QStringLiteral("contentType")] = file->contentType();
    obj[QStringLiteral("size")] = file->size();
    obj[QStringLiteral("formattedSize")] = file->formattedSize();
    obj[QStringLiteral("iconName")] = file->iconName();
    obj[QStringLiteral("isInline")] = file->isInline();
    obj[QStringLiteral("downloaded")] = file->downloaded();
    obj[QStringLiteral("filePath")] = file->filePath();
    return obj;
}

QString ThreadViewBridge::processInlineImages(const QString &html, const QString &messageId) const
{
    if (isSpamFolder()) {
        return html;
    }

    QString processed = html;

    // Get attachments for this message
    QList<File*> files = File::fetchByMessage(m_db, messageId, nullptr);

    for (File *file : files) {
        if (file->isInline() && !file->contentId().isEmpty()) {
            // Replace cid:contentId with file:// URL
            QString cidUrl = QStringLiteral("cid:%1").arg(file->contentId());
            QString fileUrl = QStringLiteral("file://%1").arg(file->filePath());
            processed.replace(cidUrl, fileUrl);
        }
        delete file;
    }

    return processed;
}

QString ThreadViewBridge::fetchAttachmentViaDbus(const QString &fileId) const
{
    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "ThreadViewBridge: D-Bus interface not available";
        return {};
    }

    QDBusPendingReply<QString> reply = iface.FetchAttachment(fileId);
    reply.waitForFinished();
    if (reply.isValid() && !reply.value().isEmpty()) {
        return reply.value();
    }

    qWarning() << "Failed to fetch attachment:" << reply.error().message();
    return {};
}

QString ThreadViewBridge::formatContacts(const QList<MessageContact*> &contacts) const
{
    QString result;
    for (int i = 0; i < contacts.size(); ++i) {
        if (i > 0) {
            result += QStringLiteral(", ");
        }
        MessageContact *contact = contacts.at(i);
        result += QStringLiteral("%1 <%2>").arg(contact->name(), contact->email());
    }
    return result;
}

bool ThreadViewBridge::isSpamFolder() const
{
    // Block auto-downloads for spam/junk folders as a security measure
    return m_currentFolderRole == QStringLiteral("spam");
}

void ThreadViewBridge::handlePortalResponse(uint response, const QVariantMap &results)
{
    // Find the source path for the first pending save (signals come in order)
    if (m_pendingPortalSaves.isEmpty()) {
        return;
    }

    // Get the first pending save
    auto it = m_pendingPortalSaves.begin();
    QString requestPath = it.key();
    QString sourcePath = it.value();
    m_pendingPortalSaves.erase(it);

    // Disconnect the signal for this request
    QDBusConnection::sessionBus().disconnect(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        requestPath,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Response"),
        this,
        SLOT(handlePortalResponse(uint, QVariantMap))
    );

    // Response 0 = success, 1 = user cancelled, 2 = other error
    if (response != 0) {
        if (response == 1) {
            qDebug() << "User cancelled save dialog";
        } else {
            qWarning() << "Portal save failed with response:" << response;
        }
        return;
    }

    // Get the selected URIs
    QStringList uris = results.value(QStringLiteral("uris")).toStringList();
    if (uris.isEmpty()) {
        qWarning() << "Portal returned no URIs";
        return;
    }

    QString destUri = uris.first();
    qDebug() << "Portal save destination:" << destUri;

    // The portal returns a file:// URI - copy the file
    QUrl destUrl(destUri);
    if (!destUrl.isLocalFile()) {
        qWarning() << "Portal returned non-local URI:" << destUri;
        return;
    }

    QString destPath = destUrl.toLocalFile();

    // Copy the file
    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    if (!QFile::copy(sourcePath, destPath)) {
        qWarning() << "Failed to copy attachment to:" << destPath;
    } else {
        qDebug() << "Saved attachment to:" << destPath;
    }
}
