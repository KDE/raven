// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbmanager.h"
#include "constants.h"

#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

constexpr auto DATABASE_REVISION = 1; // Keep MIGRATE_TO_LATEST_FROM in sync
#define MIGRATE_TO(n, current) \
    if (current < n) { \
        qDebug() << "running migration" << #n; \
        migrationV##n(current); \
        qDebug() << "finished running migration" << #n; \
    }
#define MIGRATE_TO_LATEST_FROM(current) MIGRATE_TO(1, current)

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

void DBManager::migrate()
{
    // Create migration table
    QSqlQuery createMetadata(QSqlDatabase::database());
    createMetadata.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS Metadata (migrationId INTEGER NOT NULL)"));
    exec(createMetadata);

    // Find out current revision
    QSqlQuery currentRevision(QSqlDatabase::database());
    currentRevision.prepare(QStringLiteral("SELECT migrationId FROM Metadata ORDER BY migrationId DESC LIMIT 1"));
    exec(currentRevision);
    currentRevision.first();

    uint revision = 0;
    if (currentRevision.isValid()) {
         revision = currentRevision.value(0).toUInt();
    }

    qDebug() << "current database revision" << revision;

    // Run migration if necessary
    if (revision >= DATABASE_REVISION) {
        return;
    }

    MIGRATE_TO_LATEST_FROM(revision);

    // Update migration info if necessary
    QSqlQuery update;
    update.prepare(QStringLiteral("INSERT INTO Metadata (migrationId) VALUES (:migrationId)"));
    update.bindValue(QStringLiteral(":migrationId"), DATABASE_REVISION);
    exec(update);
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

void DBManager::migrationV1(uint)
{
    QSqlDatabase::database().transaction();
    QSqlQuery createTables;

    auto jobsCreate =
        "CREATE TABLE IF NOT EXISTS " + JOB_TABLE.toStdString() + " ("
            "id INTEGER PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "createdAt DATETIME,"
            "status TEXT"
        ")";

    auto messagesCreate =
        "CREATE TABLE IF NOT EXISTS " + MESSAGE_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "folderId TEXT,"
            "threadId TEXT,"
            "headerMessageId TEXT,"
            "gmailMessageId TEXT,"
            "gmailThreadId TEXT,"
            "subject TEXT,"
            "draft TINYINT(1),"
            "unread TINYINT(1),"
            "starred TINYINT(1),"
            "date DATETIME,"
            "remoteUID INTEGER"
        ")";

    auto messageBodyCreate =
        "CREATE TABLE IF NOT EXISTS " + MESSAGE_BODY_TABLE.toStdString() + " (id TEXT PRIMARY KEY, `value` TEXT)";

    auto threadsCreate =
        "CREATE TABLE IF NOT EXISTS " + THREAD_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "gmailThreadId TEXT,"
            "subject TEXT,"
            "snippet TEXT,"
            "firstMessageTimestamp DATETIME,"
            "lastMessageTimestamp DATETIME,"
            "lastMessageReceivedTimestamp DATETIME,"
            "lastMessageSentTimestamp DATETIME"
        ")";

    auto threadRefsCreate =
        "CREATE TABLE IF NOT EXISTS " + THREAD_REFERENCE_TABLE.toStdString() + " ("
            "threadId TEXT,"
            "accountId TEXT,"
            "headerMessageId TEXT,"
            "PRIMARY KEY (threadId, accountId, headerMessageId)"
        ")";

    auto foldersCreate =
        "CREATE TABLE IF NOT EXISTS " + FOLDER_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "path TEXT,"
            "role TEXT,"
            "createdAt DATETIME"
        ")";

    auto labelsCreate =
        "CREATE TABLE IF NOT EXISTS " + LABEL_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "path TEXT,"
            "role TEXT,"
            "createdAt DATETIME"
        ")";

    auto filesCreate =
        "CREATE TABLE IF NOT EXISTS " + FILE_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "data TEXT,"
            "accountId TEXT,"
            "filename TEXT"
        ")";

    createTables.exec(QString::fromStdString(jobsCreate));
    createTables.exec(QString::fromStdString(messagesCreate));
    createTables.exec(QString::fromStdString(messageBodyCreate);
    createTables.exec(QString::fromStdString(threadsCreate));
    createTables.exec(QString::fromStdString(threadRefsCreate));
    createTables.exec(QString::fromStdString(foldersCreate));
    createTables.exec(QString::fromStdString(labelsCreate));
    createTables.exec(QString::fromStdString(filesCreate));
    QSqlDatabase::database().commit();
}
