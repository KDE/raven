// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "attachmentmodel.h"
#include "ravendaemoninterface.h"
#include "constants.h"

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QUrl>

AttachmentModel::AttachmentModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

AttachmentModel *AttachmentModel::self()
{
    static AttachmentModel *instance = new AttachmentModel();
    return instance;
}

void AttachmentModel::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
}

void AttachmentModel::loadMessage(Message *message)
{
    if (!message) {
        clear();
        return;
    }

    beginResetModel();
    qDeleteAll(m_files);
    m_files.clear();
    m_files = File::fetchByMessage(m_db, message->id(), this);
    endResetModel();
    Q_EMIT countChanged();
}

void AttachmentModel::clear()
{
    beginResetModel();
    qDeleteAll(m_files);
    m_files.clear();
    endResetModel();
    Q_EMIT countChanged();
}

QString AttachmentModel::ensureDownloaded(int index)
{
    if (index < 0 || index >= m_files.count()) {
        return {};
    }

    File *file = m_files.at(index);
    QString filePath = file->filePath();

    if (file->downloaded() && QFile::exists(filePath)) {
        return filePath;
    }

    // Fetch via D-Bus
    QString downloadedPath = fetchAttachmentViaDbus(file->id());
    if (downloadedPath.isEmpty()) {
        return {};
    }

    // Update state
    file->setDownloaded(true);
    QModelIndex modelIndex = createIndex(index, 0);
    Q_EMIT dataChanged(modelIndex, modelIndex, {DownloadedRole});

    return downloadedPath;
}

void AttachmentModel::openAttachment(int index)
{
    QString path = ensureDownloaded(index);
    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void AttachmentModel::saveAttachment(int index)
{
    QString sourcePath = ensureDownloaded(index);
    if (sourcePath.isEmpty()) {
        return;
    }

    File *file = m_files.at(index);
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString suggestedPath = defaultDir + QStringLiteral("/") + file->fileName();

    QString destPath = QFileDialog::getSaveFileName(nullptr, QStringLiteral("Save Attachment"), suggestedPath);
    if (destPath.isEmpty()) {
        return;
    }

    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    if (!QFile::copy(sourcePath, destPath)) {
        qWarning() << "Failed to save attachment to:" << destPath;
    }
}

int AttachmentModel::count() const
{
    return m_files.count();
}

int AttachmentModel::nonInlineCount() const
{
    int count = 0;
    for (const auto &file : m_files) {
        if (!file->isInline()) {
            count++;
        }
    }
    return count;
}

int AttachmentModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_files.count();
}

QVariant AttachmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.count()) {
        return {};
    }

    File *file = m_files.at(index.row());

    switch (role) {
    case FileIdRole:
        return file->id();
    case FileNameRole:
        return file->fileName();
    case ContentTypeRole:
        return file->contentType();
    case SizeRole:
        return file->size();
    case FormattedSizeRole:
        return file->formattedSize();
    case IconNameRole:
        return file->iconName();
    case IsInlineRole:
        return file->isInline();
    case DownloadedRole:
        return file->downloaded();
    case FilePathRole:
        return file->filePath();
    }

    return {};
}

Qt::ItemFlags AttachmentModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> AttachmentModel::roleNames() const
{
    return {
        {FileIdRole, "fileId"},
        {FileNameRole, "fileName"},
        {ContentTypeRole, "contentType"},
        {SizeRole, "size"},
        {FormattedSizeRole, "formattedSize"},
        {IconNameRole, "iconName"},
        {IsInlineRole, "isInline"},
        {DownloadedRole, "downloaded"},
        {FilePathRole, "filePath"}
    };
}

QString AttachmentModel::fetchAttachmentViaDbus(const QString &fileId) const
{
    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "AttachmentModel: D-Bus interface not available";
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
