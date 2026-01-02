// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file.h"
#include "../constants.h"
#include "../utils.h"

#include <KLocalizedString>
#include <QDir>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QStandardPaths>

File::File(QObject *parent, const QJsonObject &json)
    : QObject{parent}
    , m_id{json[QStringLiteral("id")].toString()}
    , m_accountId{json[QStringLiteral("accountId")].toString()}
    , m_messageId{json[QStringLiteral("messageId")].toString()}
    , m_filename{json[QStringLiteral("filename")].toString()}
    , m_partId{json[QStringLiteral("partId")].toString()}
    , m_contentId{json[QStringLiteral("contentId")].toString()}
    , m_contentType{json[QStringLiteral("contentType")].toString()}
    , m_size{json[QStringLiteral("size")].toInt()}
    , m_isInline{json[QStringLiteral("isInline")].toBool()}
    , m_downloaded{json[QStringLiteral("downloaded")].toBool()}
{
}

File::File(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toString()}
    , m_accountId{query.value(QStringLiteral("accountId")).toString()}
    , m_messageId{query.value(QStringLiteral("messageId")).toString()}
    , m_filename{query.value(QStringLiteral("fileName")).toString()}
    , m_partId{query.value(QStringLiteral("partId")).toString()}
    , m_contentId{query.value(QStringLiteral("contentId")).toString()}
    , m_contentType{query.value(QStringLiteral("contentType")).toString()}
    , m_size{query.value(QStringLiteral("size")).toInt()}
    , m_isInline{query.value(QStringLiteral("isInline")).toBool()}
    , m_downloaded{query.value(QStringLiteral("downloaded")).toBool()}
{
}

QList<File*> File::fetchByMessage(QSqlDatabase &db, const QString &messageId, QObject *parent)
{
    QList<File*> files;
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM ") + FILE_TABLE +
        QStringLiteral(" WHERE messageId = :messageId"));
    query.bindValue(QStringLiteral(":messageId"), messageId);

    if (!Utils::execWithLog(query, "fetching files by message")) {
        return files;
    }

    while (query.next()) {
        files.push_back(new File(parent, query));
    }
    return files;
}

File* File::fetchById(QSqlDatabase &db, const QString &id, QObject *parent)
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM ") + FILE_TABLE + QStringLiteral(" WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!Utils::execWithLog(query, "fetching file by id")) {
        return nullptr;
    }

    if (query.next()) {
        return new File(parent, query);
    }
    return nullptr;
}

QJsonObject File::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("id")] = m_id;
    object[QStringLiteral("accountId")] = m_accountId;
    object[QStringLiteral("messageId")] = m_messageId;
    object[QStringLiteral("filename")] = m_filename;
    object[QStringLiteral("partId")] = m_partId;
    object[QStringLiteral("contentId")] = m_contentId;
    object[QStringLiteral("contentType")] = m_contentType;
    object[QStringLiteral("size")] = m_size;
    object[QStringLiteral("isInline")] = m_isInline;
    object[QStringLiteral("downloaded")] = m_downloaded;
    return object;
}

QString File::id() const
{
    return m_id;
}

QString File::accountId() const
{
    return m_accountId;
}

QString File::messageId() const
{
    return m_messageId;
}

QString File::fileName() const
{
    return m_filename;
}

QString File::partId() const
{
    return m_partId;
}

QString File::contentId() const
{
    return m_contentId;
}

void File::setContentId(const QString &contentId)
{
    m_contentId = contentId;
}

QString File::contentType() const
{
    return m_contentType;
}

int File::size() const
{
    return m_size;
}

bool File::isInline() const
{
    return m_isInline;
}

bool File::downloaded() const
{
    return m_downloaded;
}

void File::setDownloaded(bool downloaded)
{
    if (m_downloaded != downloaded) {
        m_downloaded = downloaded;
        Q_EMIT downloadedChanged();
    }
}

QString File::iconName() const
{
    // Use QMimeDatabase to get the appropriate icon
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(m_contentType);
    if (mime.isValid()) {
        return mime.iconName();
    }
    return QStringLiteral("application-octet-stream");
}

QString File::formattedSize() const
{
    if (m_size < 1024) {
        return QStringLiteral("%1 B").arg(m_size);
    } else if (m_size < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(m_size / 1024.0, 0, 'f', 1);
    } else {
        return QStringLiteral("%1 MB").arg(m_size / (1024.0 * 1024.0), 0, 'f', 1);
    }
}

QString File::filePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString messageIdSafe = QString(m_messageId).replace(QLatin1Char(':'), QLatin1Char('_')).replace(QLatin1Char('/'), QLatin1Char('_'));
    QString diskFilename = QStringLiteral("%1_%2").arg(messageIdSafe, m_filename);
    return QDir(dataDir).filePath(QStringLiteral("raven/files/%1/%2").arg(m_accountId, diskFilename));
}
