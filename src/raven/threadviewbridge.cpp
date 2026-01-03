// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "threadviewbridge.h"
#include "constants.h"
#include "dbmanager.h"
#include "ravendaemoninterface.h"

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

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

void ThreadViewBridge::loadThread(const QString &threadId, const QString &accountId)
{
    // Clear previous data
    qDeleteAll(m_messages);
    m_messages.clear();
    m_messageContents.clear();
    m_currentThreadId = threadId;
    m_currentAccountId = accountId;

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

    // Emit the loaded signal with JSON data
    Q_EMIT threadLoaded(getMessagesJson());
}

QString ThreadViewBridge::getMessagesJson() const
{
    QJsonArray messagesArray;

    for (int i = 0; i < m_messages.size(); ++i) {
        Message *msg = m_messages.at(i);
        QString content = m_messageContents.at(i);

        QJsonObject msgObj;
        msgObj[QStringLiteral("id")] = msg->id();
        msgObj[QStringLiteral("subject")] = msg->subject();
        msgObj[QStringLiteral("from")] = QStringLiteral("%1 <%2>").arg(msg->from()->name(), msg->from()->email());
        msgObj[QStringLiteral("fromName")] = msg->from()->name();
        msgObj[QStringLiteral("fromEmail")] = msg->from()->email();
        msgObj[QStringLiteral("to")] = formatContacts(msg->to());
        msgObj[QStringLiteral("cc")] = formatContacts(msg->cc());
        msgObj[QStringLiteral("bcc")] = formatContacts(msg->bcc());
        msgObj[QStringLiteral("date")] = msg->date().toString(Qt::ISODate);
        msgObj[QStringLiteral("dateFormatted")] = QLocale().toString(msg->date(), QLocale::ShortFormat);
        msgObj[QStringLiteral("isPlaintext")] = msg->plaintext();
        msgObj[QStringLiteral("unread")] = msg->unread();
        msgObj[QStringLiteral("starred")] = msg->starred();
        msgObj[QStringLiteral("snippet")] = msg->snippet();

        // Process content to replace cid: URLs with file: URLs
        QString processedContent = msg->plaintext() ? content : processInlineImages(content, msg->id());
        msgObj[QStringLiteral("content")] = processedContent;

        // Get attachments for this message
        QList<File*> files = File::fetchByMessage(m_db, msg->id(), nullptr);
        QJsonArray attachmentsArray;
        int nonInlineCount = 0;

        for (File *file : files) {
            QJsonObject fileObj;
            fileObj[QStringLiteral("id")] = file->id();
            fileObj[QStringLiteral("filename")] = file->fileName();
            fileObj[QStringLiteral("contentType")] = file->contentType();
            fileObj[QStringLiteral("size")] = file->size();
            fileObj[QStringLiteral("formattedSize")] = file->formattedSize();
            fileObj[QStringLiteral("iconName")] = file->iconName();
            fileObj[QStringLiteral("isInline")] = file->isInline();
            fileObj[QStringLiteral("downloaded")] = file->downloaded();
            fileObj[QStringLiteral("filePath")] = file->filePath();

            attachmentsArray.append(fileObj);

            if (!file->isInline()) {
                nonInlineCount++;
            }

            delete file;
        }

        msgObj[QStringLiteral("attachments")] = attachmentsArray;
        msgObj[QStringLiteral("attachmentCount")] = nonInlineCount;

        messagesArray.append(msgObj);
    }

    return QString::fromUtf8(QJsonDocument(messagesArray).toJson(QJsonDocument::Compact));
}

QString ThreadViewBridge::getAttachmentsJson(const QString &messageId) const
{
    QList<File*> files = File::fetchByMessage(m_db, messageId, nullptr);
    QJsonArray attachmentsArray;

    for (File *file : files) {
        QJsonObject fileObj;
        fileObj[QStringLiteral("id")] = file->id();
        fileObj[QStringLiteral("filename")] = file->fileName();
        fileObj[QStringLiteral("contentType")] = file->contentType();
        fileObj[QStringLiteral("size")] = file->size();
        fileObj[QStringLiteral("formattedSize")] = file->formattedSize();
        fileObj[QStringLiteral("iconName")] = file->iconName();
        fileObj[QStringLiteral("isInline")] = file->isInline();
        fileObj[QStringLiteral("downloaded")] = file->downloaded();
        fileObj[QStringLiteral("filePath")] = file->filePath();

        attachmentsArray.append(fileObj);
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

    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString fileName = file->fileName();
    delete file;

    // Use non-static QFileDialog for proper portal support in Flatpak
    // The portal grants write access to the selected location
    QFileDialog *dialog = new QFileDialog(nullptr, tr("Save Attachment"), defaultDir, QString());
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->selectFile(fileName);
    dialog->setFileMode(QFileDialog::AnyFile);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Connect to handle the result asynchronously (required for portal dialogs)
    connect(dialog, &QFileDialog::fileSelected, this, [this, sourcePath](const QString &destPath) {
        if (destPath.isEmpty()) {
            return;
        }

        if (QFile::exists(destPath)) {
            QFile::remove(destPath);
        }

        if (!QFile::copy(sourcePath, destPath)) {
            qWarning() << "Failed to save attachment to:" << destPath;
        } else {
            qDebug() << "Saved attachment to:" << destPath;
        }
    });

    dialog->open();
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

QString ThreadViewBridge::processInlineImages(const QString &html, const QString &messageId) const
{
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
