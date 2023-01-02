// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "folder.h"
#include "constants.h"

#include <QUuid>

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
{
}

void Folder::saveToDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT INTO ") + FOLDERS_TABLE + QStringLiteral(" (id, accountId, data, path, role, createdAt) VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(m_id);
    query.addBindValue(m_accountId);
    query.addBindValue(QStringLiteral("{}")); // TODO data
    query.addBindValue(m_path);
    query.addBindValue(m_role);
    query.addBindValue(m_createdAt);
    query.exec();
}

void Folder::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + FOLDERS_TABLE + QStringLiteral(" WHERE id = ") + m_id);
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
