// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "label.h"
#include "../constants.h"

#include <QJsonDocument>

Label::Label(QObject *parent, QString id, QString accountId)
    : Folder{parent, id, accountId}
{}

Label::Label(QObject *parent, const QSqlQuery &query)
    : Folder{parent, query}
{}

void Label::saveToDb(QSqlDatabase &db) const
{
    QJsonObject object;
    object[QStringLiteral("localStatus")] = m_localStatus;

    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO ") + LABEL_TABLE +
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

void Label::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + LABEL_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
}
