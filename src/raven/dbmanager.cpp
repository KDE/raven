// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbmanager.h"
#include "constants.h"

#include <QStandardPaths>
#include <QSqlError>
#include <QSqlQuery>
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
        return db;
    }

    // Configure SQLite for concurrent access with the daemon
    // The daemon uses WAL mode, so we must also use WAL mode for compatibility
    QSqlQuery query{db};

    // Set WAL mode for better concurrency (matches daemon configuration)
    if (!query.exec(QStringLiteral("PRAGMA journal_mode = WAL"))) {
        qWarning() << "Failed to set WAL mode:" << query.lastError();
    }

    // Set busy timeout to wait up to 5 seconds if database is locked
    // This prevents immediate failures when the daemon is writing
    if (!query.exec(QStringLiteral("PRAGMA busy_timeout = 5000"))) {
        qWarning() << "Failed to set busy timeout:" << query.lastError();
    }

    // Enable foreign keys for data integrity
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError();
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
