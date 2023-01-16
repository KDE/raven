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
    createMetadata.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS metadata (migrationId INTEGER NOT NULL)"));
    exec(createMetadata);

    // Find out current revision
    QSqlQuery currentRevision(QSqlDatabase::database());
    currentRevision.prepare(QStringLiteral("SELECT migrationId FROM metadata ORDER BY migrationId DESC LIMIT 1"));
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
    update.prepare(QStringLiteral("INSERT INTO metadata (migrationId) VALUES (:migrationId)"));
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
        "CREATE TABLE IF NOT EXISTS " + MESSAGE_BODY_TABLE.toStdString() + " (id TEXT PRIMARY KEY, `value` TEXT, fetchedAt DATETIME)";

    auto threadsCreate = // TODO add foreign key??? (maybe not since annoying), add folderId
        "CREATE TABLE IF NOT EXISTS " + THREAD_TABLE.toStdString() + " ("
            "id TEXT PRIMARY KEY,"
            "accountId TEXT,"
            "data TEXT,"
            "gmailThreadId TEXT,"
            "subject TEXT,"
            "snippet TEXT,"
            "unread INTEGER,"
            "starred INTEGER,"
            "firstMessageTimestamp DATETIME,"
            "lastMessageTimestamp DATETIME"
        ")";

    auto threadRefsCreate =
        "CREATE TABLE IF NOT EXISTS " + THREAD_REFERENCE_TABLE.toStdString() + " ("
            "threadId TEXT,"
            "accountId TEXT,"
            "headerMessageId TEXT,"
            "PRIMARY KEY (threadId, accountId, headerMessageId)"
        ")";
        
    auto threadFolderCreate =
        "CREATE TABLE IF NOT EXISTS " + THREAD_FOLDER_TABLE.toStdString() + " ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "accountId TEXT,"
            "threadId TEXT,"
            "folderId TEXT,"
            "FOREIGN KEY(threadId) REFERENCES " + THREAD_TABLE.toStdString() + "(id),"
            "FOREIGN KEY(folderId) REFERENCES " + FOLDER_TABLE.toStdString() + "(id)"
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
            "fileName TEXT"
        ")";

    createTables.exec(QString::fromStdString(jobsCreate));
    createTables.exec(QString::fromStdString(messagesCreate));
    createTables.exec(QString::fromStdString(messageBodyCreate));
    createTables.exec(QString::fromStdString(threadsCreate));
    createTables.exec(QString::fromStdString(threadRefsCreate));
    createTables.exec(QString::fromStdString(threadFolderCreate));
    createTables.exec(QString::fromStdString(foldersCreate));
    createTables.exec(QString::fromStdString(labelsCreate));
    createTables.exec(QString::fromStdString(filesCreate));
    QSqlDatabase::database().commit();
}

QHash<uint32_t, MessageAttributes> DBManager::fetchMessagesAttributesInRange(Range range, Folder &folder, QSqlDatabase &db)
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM ") + MESSAGE_TABLE + QStringLiteral(" WHERE accountId = ? AND folderId = ? AND remoteUID >= ? AND remoteUID <= ?"));
    query.addBindValue(folder.accountId());
    query.addBindValue(folder.id());
    query.addBindValue((long long) (range.location));
    
    // Range is uint64_t, and "*" is represented by UINT64_MAX.
    // SQLite doesn't support UINT64 and the conversion /can/ fail.
    if (range.length == UINT64_MAX) {
        query.addBindValue(LLONG_MAX);
    } else {
        query.addBindValue((long long) (range.location + range.length));
    }
    
    query.exec();
    
    QHash<uint32_t, MessageAttributes> results;
    
    while (query.next()) {
        auto msg = std::make_shared<Message>(nullptr, query);
        
        MessageAttributes attrs;
        attrs.uid = (uint32_t) msg->remoteUid().toInt();
        attrs.starred = msg->starred();
        attrs.unread = msg->unread();
        attrs.labels = msg->labels();
        
        results[attrs.uid] = attrs;
    }
    
    return results;
}

uint32_t DBManager::fetchMessageUIDAtDepth(QSqlDatabase &db, Folder &folder, uint32_t depth, uint32_t before) {
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT remoteUID FROM ") + MESSAGE_TABLE + QStringLiteral(" WHERE accountId = ? AND folderId = ? AND remoteUID ? ORDER BY remoteUID DESC LIMIT 1 OFFSET ?"));
    query.addBindValue(folder.accountId());
    query.addBindValue(folder.id());
    query.addBindValue(before);
    query.addBindValue(depth);
    query.exec();
    
    if (query.next()) {
        return query.value(QStringLiteral("remoteUID")).toUInt();
    }
    
    return 1;
}
