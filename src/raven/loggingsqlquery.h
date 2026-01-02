// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class LoggingSqlQuery : public QSqlQuery
{
public:
    explicit LoggingSqlQuery(QSqlDatabase &db)
        : QSqlQuery(db)
    {
    }

    bool exec(const QString &sql)
    {
        if (!QSqlQuery::exec(sql)) {
            qWarning() << "SQL error:" << lastError().text() << "Query:" << sql;
            return false;
        }
        return true;
    }

    bool exec()
    {
        if (!QSqlQuery::exec()) {
            qWarning() << "SQL error:" << lastError().text() << "Query:" << lastQuery();
            return false;
        }
        return true;
    }
};
