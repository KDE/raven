// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "label.h"
#include "constants.h"

Label::Label(QObject *parent, QString id, QString accountId)
    : Folder{parent, id, accountId}
{}

Label::Label(QObject *parent, const QSqlQuery &query)
    : Folder{parent, query}
{}

void Label::saveToDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT INTO ") + LABELS_TABLE + QStringLiteral(" (id, accountId, data, path, role, createdAt) VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(m_id);
    query.addBindValue(m_accountId);
    query.addBindValue(QStringLiteral("{}")); // TODO data
    query.addBindValue(m_path);
    query.addBindValue(m_role);
    query.addBindValue(m_createdAt);
    query.exec();
}

void Label::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + LABELS_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
}
