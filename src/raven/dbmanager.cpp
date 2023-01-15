// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbmanager.h"
#include "../libraven/constants.h"
#include "../libraven/models/message.h"

#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

DBManager *DBManager::self()
{
    static DBManager *manager = new DBManager;
    return manager;
}

DBManager::DBManager(QObject* parent)
    : QObject{parent}
{
    // initialize for main thread
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(RAVEN_DATA_LOCATION + QStringLiteral("/raven.sqlite"));

    if (!db.open()) {
        qWarning() << "Could not open database" << db.lastError();
    }
}

void DBManager::exec(QSqlQuery &query)
{
    if (query.lastQuery().isEmpty()) {
        // Sending empty queries doesn't make sense
        Q_UNREACHABLE();
    }
    if (!query.exec()) {
        qWarning() <<  "Query" << query.lastQuery() << "resulted in" << query.lastError();
    }
}
