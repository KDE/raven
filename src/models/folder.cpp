// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "folder.h"
#include "constants.h"

#include <QUuid>
#include <QJsonDocument>

Folder::Folder(QObject *parent, QString id, QString accountId)
    : QObject{parent}
    , m_id{id}
    , m_accountId{accountId}
    , m_createdAt{QDateTime::currentDateTime()}
{}

Folder::Folder(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toInt()}
    , m_accountId{query.value(QStringLiteral("accountId")).toString()}
    , m_path{query.value(QStringLiteral("path")).toString()}
    , m_role{query.value(QStringLiteral("role")).toInt()}
    , m_createdAt{query.value(QStringLiteral("createdAt")).toDateTime()}
    , m_localStatus{}
    , m_status{}
{
    QJsonObject data = query.value(QStringLiteral("data")).toJsonDocument().object();

    m_localStatus = data[QStringLiteral("localStatus")].toObject();
}

void Folder::saveToDb(QSqlDatabase &db) const
{
    QJsonObject object;
    object[QStringLiteral("localStatus")] = m_localStatus;

    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO ") + FOLDER_TABLE +
        QStringLiteral(" (id, accountId, data, path, role, createdAt)") +
        QStringLiteral(" VALUES (:id, :accountId, :data, :path, :role, :createdAt)"));

    query.bindValue(QStringLiteral(":id"), m_id);
    query.bindValue(QStringLiteral(":accountId"), m_accountId);
    query.bindValue(QStringLiteral(":data"), QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":path"), m_path);
    query.bindValue(QStringLiteral(":role"), m_role);
    query.bindValue(QStringLiteral(":createdAt"), m_createdAt);
    query.exec();
}

void Folder::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + FOLDER_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
}

QString Folder::id() const
{
    return m_id;
}

QString Folder::accountId() const
{
    return m_accountId;
}

QString Folder::path() const
{
    return m_path;
}

void Folder::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        Q_EMIT pathChanged();
    }
}

QString Folder::role() const
{
    return m_role;
}

void Folder::setRole(const QString &role)
{
    if (m_role != role) {
        m_role = role;
        Q_EMIT roleChanged();
    }
}

QDateTime Folder::createdAt() const
{
    return m_createdAt;
}

QString Folder::status() const
{
    return m_status;
}

void Folder::setStatus(const QString &status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

QJsonObject &Folder::localStatus()
{
    return m_localStatus;
}
