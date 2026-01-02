// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbmanager.h"
#include "constants.h"

#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QUuid>

DBManager::DBManager(QObject *parent)
    : QObject{parent}
{
}

DBManager::~DBManager()
{
}

QString DBManager::defaultDatabasePath()
{
    return RAVEN_DATA_LOCATION + QStringLiteral("/raven.sqlite");
}

QSqlDatabase DBManager::openDatabase(const QString &connectionName)
{
    // Generate a unique connection name if not provided
    QString connName = connectionName.isEmpty()
        ? QUuid::createUuid().toString(QUuid::Id128)
        : connectionName;

    // Ensure data directory exists
    QDir dir;
    if (!dir.mkpath(RAVEN_DATA_LOCATION)) {
        qWarning() << "Could not create data directory:" << RAVEN_DATA_LOCATION;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(defaultDatabasePath());

    if (!db.open()) {
        qWarning() << "Could not open database:" << db.lastError();
    }

    return db;
}

void DBManager::closeDatabase(const QString &connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(connectionName);
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }
}
