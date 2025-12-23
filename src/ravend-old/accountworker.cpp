// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

// Much of the logic is based on https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/SyncWorker.cpp

#include "../libraven/constants.h"
#include "../libraven/utils.h"
#include "accountworker.h"
#include "dbmanager.h"
#include "progresscollectors.h"
#include "mailprocessor.h"
#include "mailcorelogger.h"
#include "taskprocessor.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QHash>
#include <QThread>
#include <QDebug>

#define CACHE_CLEANUP_INTERVAL      60 * 60
#define SHALLOW_SCAN_INTERVAL       60 * 2
#define DEEP_SCAN_INTERVAL          60 * 10

#define MAX_FULL_HEADERS_REQUEST_SIZE  1024
#define MODSEQ_TRUNCATION_THRESHOLD 4000
#define MODSEQ_TRUNCATION_UID_COUNT 12000

// These keys are saved to the folder object's "localState".
// Starred keys are used in the client to show sync progress.
#define LS_BUSY                     QStringLiteral("busy")           // *
#define LS_UIDNEXT                  QStringLiteral("uidnext")        // *
#define LS_SYNCED_MIN_UID           QStringLiteral("syncedMinUID")   // *
#define LS_BODIES_PRESENT           QStringLiteral("bodiesPresent")  // *
#define LS_BODIES_WANTED            QStringLiteral("bodiesWanted")   // *
#define LS_LAST_CLEANUP             QStringLiteral("lastCleanup")
/// IMPORTANT: deep/shallow are only used for some IMAP servers
#define LS_LAST_SHALLOW             QStringLiteral("lastShallow")
#define LS_LAST_DEEP                QStringLiteral("lastDeep")
#define LS_HIGHESTMODSEQ            QStringLiteral("highestmodseq")
#define LS_UIDVALIDITY              QStringLiteral("uidvalidity")
#define LS_UIDVALIDITY_RESET_COUNT  QStringLiteral("uidvalidityResetCount")


AccountWorker::AccountWorker(QObject *parent, Account *account)
    : QObject{parent}
    , m_account{account}
    , m_imapSession{new IMAPSession}
    , m_mailProcessor{new MailProcessor{(QObject *) this, this}}
    , m_unlinkPhase{1}
{
}

AccountWorker::~AccountWorker()
{
    delete m_mailProcessor;
    delete m_imapSession;
    getDB().close();
}

Account *AccountWorker::account() const
{
    return m_account;
}

QSqlDatabase AccountWorker::getDB() const
{
    return QSqlDatabase::database(m_account->id());
}

// assume that this is running in the new thread
void AccountWorker::run()
{
    qDebug() << "Started worker for account" << m_account->email();

    setupSession();

    qDebug() << "Finished setting up session for account" << m_account->email();

    syncNow();
}

void AccountWorker::setupSession()
{
    // initialize database connection for thread

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_account->id());
    db.setDatabaseName(RAVEN_DATA_LOCATION + QStringLiteral("/raven.sqlite"));

    if (!db.open()) {
        qWarning() << "Could not open database" << db.lastError();
    }

    // initialize IMAP session

    // TODO update connection details if changed (enqueue job?)
    m_imapSession->setHostname(AS_MCSTR(m_account->imapHost().toStdString()));
    m_imapSession->setPort(m_account->imapPort());
    m_imapSession->setUsername(AS_MCSTR(m_account->imapUsername().toStdString()));
    m_imapSession->setPassword(AS_MCSTR(m_account->imapPassword().toStdString()));
    m_imapSession->setConnectionLogger(new ConnectionLogger());

    switch (m_account->imapConnectionType()) {
        case Account::ConnectionType::None:
            m_imapSession->setConnectionType(ConnectionType::ConnectionTypeClear);
            break;
        case Account::ConnectionType::SSL:
            m_imapSession->setConnectionType(ConnectionType::ConnectionTypeTLS);
            break;
        case Account::ConnectionType::StartTLS:
            m_imapSession->setConnectionType(ConnectionType::ConnectionTypeStartTLS);
            break;
    }

    switch (m_account->imapAuthenticationType()) {
        case Account::AuthenticationType::NoAuth:
            m_imapSession->setAuthType(AuthType::AuthTypeSASLNone);
            break;
        case Account::AuthenticationType::OAuth2:
            m_imapSession->setAuthType(AuthType::AuthTypeXOAuth2);
            break;
        case Account::AuthenticationType::Plain:
            // it will figure it out itself
            break;
    }
    
    // TODO add config option
    m_imapSession->setCheckCertificateEnabled(false);
    m_imapSession->setAutomaticConfigurationEnabled(true);
    
    m_imapSession->setConnectionLogger(new MailCoreLogger);
    
    ErrorCode err = ErrorCode::ErrorNone;
    m_imapSession->connectIfNeeded(&err);
    
    if (err != ErrorCode::ErrorNone) {
        qDebug() << "Could not connect to IMAP server for" << m_account->email() << "due to" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        m_enabled = false;
    } else {
        m_enabled = true;
    }
}

void AccountWorker::idleCycleIteration()
{
    QSqlDatabase db = getDB();
    
    // run body fetch requests from the client
    while (m_idleFetchBodyIDs.size() > 0) {
        QString id = m_idleFetchBodyIDs.back();
        m_idleFetchBodyIDs.pop_back();
        
        QSqlQuery query{db};
        query.prepare(QStringLiteral("SELECT * FROM message WHERE id = ?"));
        query.addBindValue(id);
        
        Utils::execWithLog(query, "idleCycleIteration - fetch messages to sync body");
        
        if (query.next()) {
            auto msg = std::make_shared<Message>(nullptr, query);
            qDebug() << "Fetching body for message ID" << msg->id();
            syncMessageBody(msg.get());
        }
    }

    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }
    
    // Connect and login to the IMAP server
    
    ErrorCode err = ErrorCode::ErrorNone;
    m_imapSession->connectIfNeeded(&err);
    if (err != ErrorCode::ErrorNone) {
        qDebug() << "ISSUE: idleCycleIteration - connectIfNeeded" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return;
    }
    
    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }
    
    m_imapSession->loginIfNeeded(&err);
    if (err != ErrorCode::ErrorNone) {
        qDebug() << "ISSUE: idleCycleIteration - loginIfNeeded" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return;
    }
    
    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }

    // Run tasks ready for performRemote. This is set up in an odd way
    // because we want tasks created when a task runs to also be run
    // immediately. (eg: A SendDraftTask queueing a SyncbackMetadataTask)
    //
    vector<shared_ptr<Task>> tasks;
    TaskProcessor processor { m_account, store, &session };

    // Ensure our pile of completed tasks doesn't grow unbounded
    processor.cleanupOldTasksAtRuntime();

    // Find tasks ready for "remote" that we haven't processed yet in this pass
    int rowid = -1;
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM Task WHERE accountId = ? AND status = \"remote\" AND rowid > ?"));
    Utils::execWithLog(query, "idleCycleIteration - find task");

    do {
        tasks = {};
        statement.bind(1, m_account->id());
        statement.bind(2, rowid);
        while (statement.executeStep()) {
            tasks.push_back(make_shared<Task>(statement));
            rowid = max(rowid, statement.getColumn("rowid").getInt());
        }
        statement.reset();
        for (auto & task : tasks) {
            processor.performRemote(task.get());
        }
    } while (tasks.size() > 0);

    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }

    // identify the preferred idle folder (inbox / all)
    
    query.prepare(QStringLiteral("SELECT * FROM folder WHERE accountId = ? AND role = inbox"));
    query.addBindValue(m_account->id());
    Utils::execWithLog(query, "idleCycleIteration - select folder with inbox role");
    
    std::shared_ptr<Folder> inbox;
    if (query.next()) {
        inbox = std::make_shared<Folder>(nullptr, query);
    } else {
        query.prepare(QStringLiteral("SELECT * FROM folder WHERE accountId = ? AND role = all"));
        query.addBindValue(m_account->id());
        
        inbox = std::make_shared<Folder>(nullptr, query);
        if (query.next()) {
            qDebug() << "ISSUE: no-inbox - There is no inbox or all folder to IDLE on.";
            return;
        }
    }
    QJsonObject inboxInitialStatus { inbox->localStatus() };
    
    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }
    
    // Check for mail in the preferred idle folder (inbox / all)
    bool hasStartedSyncingFolder = !inbox->localStatus()[LS_SYNCED_MIN_UID].isUndefined();

    if (hasStartedSyncingFolder) {
        String path = AS_MCSTR(inbox->path().toStdString());
        IMAPFolderStatus remoteStatus = m_imapSession->folderStatus(&path, &err);
        
        // Note: If we have CONDSTORE but don't have QRESYNC, this if/else may result
        // in us not seeing "vanished" messages until the next shallow sync iteration.
        // Right now I think that's fine.
        if (m_imapSession->storedCapabilities()->containsIndex(IMAPCapabilityCondstore)) {
            syncFolderChangesViaCondstore(*inbox, remoteStatus, false);
        } else {
            uint32_t uidnext = remoteStatus.uidNext();
            uint32_t syncedMinUID = inbox->localStatus()[LS_SYNCED_MIN_UID].toInt();
            uint32_t bottomUID = store->fetchMessageUIDAtDepth(*inbox, 100, uidnext);
            if (bottomUID < syncedMinUID) { bottomUID = syncedMinUID; }
            syncFolderUIDRange(*inbox, RangeMake(bottomUID, uidnext - bottomUID), false);
            inbox->localStatus()[LS_LAST_SHALLOW] = (long long) time(0);
            inbox->localStatus()[LS_UIDNEXT] = (long long) uidnext;
        }

        syncMessageBodies(*inbox, remoteStatus);
        
        if (inboxInitialStatus != inbox->localStatus()) {
            db.transaction();
            
            query.prepare();
            for (auto field : inbox->localStatus()) {
                
            }
            
            db.commit();
        }
        
        store->saveFolderStatus(inbox.get(), inboxInitialStatus);
    }

    // Idle on the folder
    
    if (m_interruptIdle) {
        m_interruptIdle = false;
        return;
    }
    if (m_imapSession->setupIdle()) {
        qDebug() << "Idling on folder" << inbox->path();
        String path = AS_MCSTR(inbox->path().toStdString());
        m_imapSession->idle(&path, 0, &err);
        m_imapSession->unsetupIdle();
        qDebug() << "Idle exited with code" << QString::fromStdString(ErrorCodeToTypeMap[err]);
    } else {
        qDebug() << "Connection does not support idling. Locking until more to do...";
        std::unique_lock<std::mutex> lck(idleMtx);
        idleCv.wait(lck);
    }
}

// Much of the logic is based on https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/SyncWorker.cpp
QList<std::shared_ptr<Folder>> AccountWorker::syncFoldersAndLabels()
{
    QSqlDatabase db = getDB();

    ErrorCode err = ErrorCode::ErrorNone;
    Array *remoteFolders = m_imapSession->fetchAllFolders(&err);

    if (err != ErrorNone) {
        // TODO better error handling (enqueue job to retry, log errors)
        qWarning() << "ISSUE: syncFoldersAndLabels" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return {};
    }

    QList<std::shared_ptr<Folder>> foldersToSync;
    QHash<QString, std::shared_ptr<Folder>> allFoundCategories;

    QString mainPrefix = Utils::namespacePrefixOrBlank(m_imapSession);

    bool isGmail = m_imapSession->storedCapabilities()->containsIndex(IMAPCapabilityGmail);


    // find all stored folders and labels

    QHash<QString, std::shared_ptr<Folder>> unusedLocalFolders;
    QHash<QString, std::shared_ptr<Label>> unusedLocalLabels;

    QSqlQuery folderQuery{QStringLiteral("SELECT * FROM ") + FOLDER_TABLE + QStringLiteral(" WHERE accountId = ") + m_account->id(), db};
    while (folderQuery.next()) {
        auto folder = std::make_shared<Folder>(nullptr, folderQuery);
        unusedLocalFolders[folder->id()] = folder;
    }

    QSqlQuery labelQuery{QStringLiteral("SELECT * FROM ") + LABEL_TABLE + QStringLiteral(" WHERE accountId = ") + m_account->id(), db};
    while (labelQuery.next()) {
        auto label = std::make_shared<Label>(nullptr, labelQuery);
        unusedLocalLabels[label->id()] = label;
    }

    // prepare transaction for saving
    db.transaction();

    // eliminate unselectable folders
    for (int i = remoteFolders->count() - 1; i >= 0; --i) {
        IMAPFolder *remoteFolder = (IMAPFolder *) remoteFolders->objectAtIndex(i);

        if (remoteFolder->flags() & IMAPFolderFlagNoSelect) {
            remoteFolders->removeObjectAtIndex(i);
            continue;
        }
    }


    // find or create local folders and labels to match the remote ones

    for (unsigned int i = 0; i < remoteFolders->count(); ++i) {
        IMAPFolder *remoteFolder = (IMAPFolder *) remoteFolders->objectAtIndex(i);

        QString remotePath = QString::fromUtf8(remoteFolder->path()->UTF8Characters());
        QString remoteId = Utils::idForFolder(m_account->id(), remotePath);

        bool isLabel = false;
        if (isGmail) {
            IMAPFolderFlag folderFlags = remoteFolder->flags();
            isLabel = !(folderFlags & IMAPFolderFlagAll) && !(folderFlags & IMAPFolderFlagSpam) && !(folderFlags & IMAPFolderFlagTrash);
        }

        std::shared_ptr<Folder> localFolder;

        // create folder/label locally if remote folder is not found here
        if (isLabel) {
            // treat as a label
            if (unusedLocalLabels.count(remoteId) > 0) {
                localFolder = unusedLocalLabels[remoteId];
                unusedLocalLabels.remove(remoteId);
            } else {
                localFolder = std::make_shared<Label>(nullptr, remoteId, m_account->id());
                localFolder->setPath(remotePath);
                localFolder->saveToDb(db);
            }

        } else {
            // treat as a folder
            if (unusedLocalFolders.count(remoteId) > 0) {
                localFolder = unusedLocalFolders[remoteId];
                unusedLocalFolders.remove(remoteId);
            } else {
                localFolder = std::make_shared<Folder>(nullptr, remoteId, m_account->id());
                localFolder->setPath(remotePath);
                localFolder->saveToDb(db);
            }

            foldersToSync.push_back(localFolder);
        }

        allFoundCategories[remoteId] = localFolder;
    }


    // match folders to roles

    for (auto role : Utils::roles()) {
        bool found = false;

        for (auto folder : allFoundCategories.values()) {
            if (folder->role() == role) {
                found = true;
                break;
            }
        }

        // skip if role already matched
        if (found) {
            continue;
        }

        // find a folder that matches the flags
        for (unsigned int i = 0; i < remoteFolders->count(); ++i) {
            IMAPFolder * remote = (IMAPFolder *) remoteFolders->objectAtIndex(i);
            QString cr = Utils::roleForFolderViaFlags(remote);
            if (cr != role) {
                continue;
            }

            QString remoteId = Utils::idForFolder(m_account->id(), QString::fromUtf8(remote->path()->UTF8Characters()));
            if (!allFoundCategories.count(remoteId)) {
                qWarning() << "-X found folder for role, couldn't find local object for" << role;
                continue;
            }
            allFoundCategories[remoteId]->setRole(role);
            allFoundCategories[remoteId]->saveToDb(db);
            found = true;
            break;
        }

        if (found) {
            continue;
        }

        // find a folder that matches the name
        for (unsigned int i = 0; i < remoteFolders->count(); ++i) {
            IMAPFolder * remote = (IMAPFolder *) remoteFolders->objectAtIndex(i);
            QString cr = Utils::roleForFolderViaPath("", mainPrefix.toStdString(), remote);
            if (cr != role) {
                continue;
            }

            QString remoteId = Utils::idForFolder(m_account->id(), QString::fromUtf8(remote->path()->UTF8Characters()));
            if (!allFoundCategories.count(remoteId)) {
                qWarning() << "-X found folder for role, couldn't find local object for" << role;
                continue;
            }

            allFoundCategories[remoteId]->setRole(role);
            allFoundCategories[remoteId]->saveToDb(db);
            found = true;
            break;
        }
    }

    // delete any folders / labels no longer present on the remote

    for (auto const &item : unusedLocalFolders) {
        item->deleteFromDb(db);
    }

    for (auto const &item : unusedLocalLabels) {
        item->deleteFromDb(db);
    }

    // commit
    db.commit();

    return foldersToSync;
}

// Much of the logic is based on https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/SyncWorker.cpp
bool AccountWorker::syncNow()
{
    if (!m_enabled) {
        return false;
    }
    
    bool syncAgainImmediately = false;

    QList<std::shared_ptr<Folder>> folders = syncFoldersAndLabels();
    bool hasCondstore = m_imapSession->isCondstoreEnabled();
    bool hasQResync = m_imapSession->isQResyncEnabled();

    // Identify folders to sync. On Gmail, labels are mapped to IMAP folders and we only want to sync all, spam, and trash.

    std::array<std::string, 7> roleOrder{"inbox", "sent", "drafts", "all", "archive", "trash", "spam"};
    std::sort(folders.begin(), folders.end(), [&roleOrder](const std::shared_ptr<Folder> lhs, const std::shared_ptr<Folder> rhs) {
        ptrdiff_t lhsRank = std::find(roleOrder.begin(), roleOrder.end(), lhs->role().toStdString()) - roleOrder.begin();
        ptrdiff_t rhsRank = std::find(roleOrder.begin(), roleOrder.end(), rhs->role().toStdString()) - roleOrder.begin();
        return lhsRank < rhsRank;
    });

    QSqlDatabase db = getDB();

    for (auto &folder : folders) {
        auto &localStatus = folder->localStatus();
        auto initialLocalStatus = localStatus; // copy

        String path = AS_MCSTR(folder->path().toStdString());
        ErrorCode err = ErrorCode::ErrorNone;
        IMAPFolderStatus remoteStatus = m_imapSession->folderStatus(&path, &err);
        bool firstChunk = false;

        if (err != ErrorNone) {
            qWarning() << "ISSUE: syncNow" << QString::fromStdString(ErrorCodeToTypeMap[err]);
            continue;
        }

        // Step 1: Check folder UIDValidity
        if (localStatus.empty() || localStatus[LS_UIDVALIDITY].isUndefined()) {
            // We're about to fetch the top N UIDs in the folder and start working backwards in time.
            // When we eventually finish and start using CONDSTORE, this will be the highestmodseq
            // from the /oldest/ synced block of UIDs, ensuring we see changes.
            localStatus[LS_HIGHESTMODSEQ] = (double) remoteStatus.highestModSeqValue();
            localStatus[LS_UIDVALIDITY] = (int) remoteStatus.uidValidity();
            localStatus[LS_UIDVALIDITY_RESET_COUNT] = 0;
            localStatus[LS_UIDNEXT] = (int) remoteStatus.uidNext();
            localStatus[LS_SYNCED_MIN_UID] = (int) remoteStatus.uidNext();
            localStatus[LS_LAST_SHALLOW] = 0;
            localStatus[LS_LAST_DEEP] = 0;
            firstChunk = true;
        }

        if (localStatus[LS_UIDVALIDITY].toInt() != (int) remoteStatus.uidValidity()) {
            // UID Invalidity means that the UIDs the server previously reported for messages
            // in this folder can no longer be used. To recover from this, we need to:
            //
            // 1) Set remoteUID to the "UNLINKED" value for every message in the folder
            // 2) Run a 'deep' scan which will refetch the metadata for the messages,
            //    compute the local message IDs and re-map local models to remote UIDs.
            //
            // Notes:
            // - It's very important that this not generate deltas - because we're only changing
            //   the folderRemoteUID it should not broadcast this update.
            //
            // - UIDNext must be reset to the updated remote value
            //
            // - syncedMinUID must be reset to something and we set it to zero. If we haven't
            //   finished the initial scan of the folder yet, this could result in the creation
            //   of a huge number of Message models all at once and flood the app. Hopefully
            //   this scenario is rare.
            qWarning() << "UIDInvalidity! Resetting remoteFolderUIDs, rebuilding index. This may take a moment...";
            QSqlQuery selectUnlinkQuery{db};
            selectUnlinkQuery.prepare(QStringLiteral("SELECT * FROM message WHERE folderId = %1"));
            selectUnlinkQuery.addBindValue(folder->id());
            m_mailProcessor->unlinkMessagesMatchingQuery(selectUnlinkQuery, m_unlinkPhase);
            syncFolderUIDRange(*folder, RangeMake(1, UINT64_MAX), false);

            if (localStatus[LS_UIDVALIDITY_RESET_COUNT].isUndefined()) {
                localStatus[LS_UIDVALIDITY_RESET_COUNT] = 1;
            }
            localStatus[LS_UIDVALIDITY_RESET_COUNT] = localStatus[LS_UIDVALIDITY_RESET_COUNT].toInt() + 1;
            localStatus[LS_HIGHESTMODSEQ] = (double) remoteStatus.highestModSeqValue();
            localStatus[LS_UIDVALIDITY] = (int) remoteStatus.uidValidity();
            localStatus[LS_UIDNEXT] = (int) remoteStatus.uidNext();
            localStatus[LS_SYNCED_MIN_UID] = 1;
            localStatus[LS_LAST_SHALLOW] = 0;
            localStatus[LS_LAST_DEEP] = 0;

            folder->saveToDb(db);
            continue;
        }

         // Step 2: Initial sync. Until we reach UID 1, we grab chunks of messages
         uint32_t syncedMinUID = localStatus[LS_SYNCED_MIN_UID].toInt();
         uint32_t chunkSize = firstChunk ? 750 : 5000;

         if (syncedMinUID > 1) {
            // The UID value space is sparse, meaning there can be huge gaps where there are no
            // messages. If the folder indicates UIDNext is 100000 but there are only 100 messages,
            // go ahead and fetch them all in one chunk. Otherwise, scan the UID space in chunks,
            // ensuring we never bite off more than we can chew.
            int chunkMinUID = syncedMinUID > chunkSize ? syncedMinUID - chunkSize : 1;
            if (remoteStatus.messageCount() < chunkSize) {
                chunkMinUID = 1;
            }
            syncFolderUIDRange(*folder, RangeMake(chunkMinUID, syncedMinUID - chunkMinUID), true);
            localStatus[LS_SYNCED_MIN_UID] = chunkMinUID;
            syncedMinUID = chunkMinUID;
        }

        // Step 3: A) Retrieve new messages  B) update existing messages  C) delete missing messages
        // CONDSTORE, when available, does A + B.
        // XYZRESYNC, when available, does C
        if (hasCondstore && hasQResync) {
            // Hooray! We never need to fetch the entire range to sync. Just look at
            // highestmodseq / uidnext and sync if we need to.
            syncFolderChangesViaCondstore(*folder, remoteStatus, true);
        } else {
            uint32_t remoteUidnext = remoteStatus.uidNext();
            uint32_t localUidnext = localStatus[LS_UIDNEXT].toInt();
            
            bool newMessages = remoteUidnext > localUidnext;
            bool timeForDeepScan = (m_syncIterationsSinceLaunch > 0) && (0 - localStatus[LS_LAST_DEEP].toInt() > DEEP_SCAN_INTERVAL);
            bool timeForShallowScan = !timeForDeepScan && (0 - localStatus[LS_LAST_SHALLOW].toInt() > SHALLOW_SCAN_INTERVAL);

            // If there are new messages in the folder (UIDnext has increased), do a heavy fetch of
            // those /AND/ get the bodies. This ensures people see both very quickly, which is important.
            //
            // This could potentially grab zillions of messages, in which case syncFolderUIDRange will
            // bail out and the next "deep" scan will pick up the ones we skipped.
            //
            if (newMessages) {
                QList<std::shared_ptr<Message>> synced;
                syncFolderUIDRange(*folder, RangeMake(localUidnext, remoteUidnext - localUidnext), true, &synced);

                if ((folder->role() == QStringLiteral("inbox")) || (folder->role() == QStringLiteral("all"))) {
                    // if UIDs are ascending, flip them so we download the newest (highest) UID bodies first
                    if (synced.size() > 1 && synced[0]->remoteUid() < synced[1]->remoteUid()) {
                        std::reverse(synced.begin(), synced.end());
                    }

                    int count = 0;
                    for (auto &msg : synced) {
                        // TODO scan all mail 
                        // if (!msg->isInInbox()) {
                        //     continue; // skip "all mail" that is not in inbox
                        // }
                        syncMessageBody(msg.get());
                        if (count++ > 30) { break; }
                    }
                }
            }

            if (timeForShallowScan) {
                // note: we use local uidnext here, because we just fetched everything between
                // localUIDNext and remoteUIDNext so fetching that section again would just slow us down.
                uint32_t bottomUID = DBManager::fetchMessageUIDAtDepth(db, *folder, 399, localUidnext);
                if (bottomUID < syncedMinUID) {
                    bottomUID = syncedMinUID;
                }
                syncFolderUIDRange(*folder, RangeMake(bottomUID, remoteUidnext - bottomUID), false);
                localStatus[LS_LAST_SHALLOW] = 0;
                localStatus[LS_UIDNEXT] = (int) remoteUidnext;
            }

            if (timeForDeepScan) {
                syncFolderUIDRange(*folder, RangeMake(syncedMinUID, UINT64_MAX), false);
                localStatus[LS_LAST_SHALLOW] = 0;
                localStatus[LS_LAST_DEEP] = 0;
                localStatus[LS_UIDNEXT] = (int) remoteUidnext;
            }
        }

         bool moreToDo = false;

        // Retrieve some message bodies. We do this concurrently with the full header
        // scan so the user sees snippets on some messages quickly.
        if (syncMessageBodies(*folder, remoteStatus)) {
            moreToDo = true;
        }
        if (syncedMinUID > 1) {
            moreToDo = true;
        }

        // Update cache metrics and cleanup bodies we don't want anymore.
        // these queries are expensive so we do this infrequently and increment
        // blindly as we download bodies.
        time_t lastCleanup = localStatus[LS_LAST_CLEANUP].isUndefined() ? 0 : localStatus[LS_LAST_CLEANUP].toInt();
        if (syncedMinUID == 1 && (-lastCleanup > CACHE_CLEANUP_INTERVAL)) {
            cleanMessageCache(*folder);
            localStatus[LS_LAST_CLEANUP] = 0;
        }

        // Save a general flag that indicates whether we're still doing stuff
        // like syncing message bodies. Set to true below.
        localStatus[LS_BUSY] = moreToDo;
        syncAgainImmediately = syncAgainImmediately || moreToDo;

        // Save the folder - note that helper methods above mutated localStatus.
        // Avoid the save if we can, because this creates a lot of noise in the client.
        if (folder->localStatus() != initialLocalStatus) {
            folder->saveToDb(db);
        }
    }

    // We've just unlinked a bunch of messages with PHASE A, now we'll delete the ones
    // with PHASE B. This ensures anything we /just/ discovered was missing gets one
    // cycle to appear in another folder before we decide it's really, really gone.
    m_unlinkPhase = m_unlinkPhase == 1 ? 2 : 1;
    qDebug() << "Sync loop deleting unlinked messages with phase" << m_unlinkPhase;
    m_mailProcessor->deleteMessagesStillUnlinkedFromPhase(m_unlinkPhase);

    qDebug() << "Sync loop complete.";
    ++m_syncIterationsSinceLaunch;

    return syncAgainImmediately;
}

void AccountWorker::syncFolderUIDRange(Folder &folder, Range range, bool heavyInitialRequest, QList<std::shared_ptr<Message>> *syncedMessages)
{
    QString remotePath = folder.path();

    // Safety check: "0" is not a valid start and causes the server to return only the last item
    if (range.location == 0) {
        range.location = 1;
    }
    // Safety check: force an attributes-only sync of the range if the requested UID range is so
    // large the query might never complete if we ask for it all. We might still need to fetch all the
    // bodies, but we'll cap the number we fetch.
    if (range.length > MAX_FULL_HEADERS_REQUEST_SIZE) {
        heavyInitialRequest = false;
    }

    qDebug() << "syncFolderUIDRange for" << remotePath << ", UIDs:" << range.location << "-" << range.location + range.length << ", Heavy:" << heavyInitialRequest;

    // allocated mailcore objects freed when `pool` is removed from the stack
    AutoreleasePool pool;
    
    IndexSet *set = IndexSet::indexSetWithRange(range);
    IndexSet *heavyNeeded = new IndexSet();
    IMAPProgress cb;
    ErrorCode err{ErrorCode::ErrorNone};
    String path{AS_MCSTR(remotePath.toStdString())};
    int heavyNeededIdeal = 0;

    QSqlDatabase db = getDB();
    
    // Step 1: fetch local attributes (unread, starred, etc.)
    QHash<uint32_t, MessageAttributes> local(DBManager::fetchMessagesAttributesInRange(range, folder, db));

    // Step 2: Fetch the remote attributes (unread, starred, etc.) for the same UID range
    time_t syncDataTimestamp = time(0);
    auto kind = Utils::messagesRequestKindFor(m_imapSession->storedCapabilities(), heavyInitialRequest);
    Array *remote = m_imapSession->fetchMessagesByUID(&path, kind, set, &cb, &err);

    if (err != ErrorNone) {
        qWarning() << "ISSUE: syncFolderUIDRange - fetchMessagesByUID" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return;
    }

    clock_t lastSleepClock = clock();

    qDebug().nospace() << "syncFolderUIDRange - " << remotePath << ": remote=" << remote->count() << ", local=" << local.size() << ", remoteUID=" << folder.id();

    for (int ii = ((int)remote->count()) - 1; ii >= 0; ii--) {
        // Never sit in a hard loop inserting things into the database for more than 250ms.
        // This ensures we don't starve another thread waiting for a database connection
        if (((clock() - lastSleepClock) * 4) / CLOCKS_PER_SEC > 1) {
            QThread::msleep(50);
            lastSleepClock = clock();
        }

        IMAPMessage * remoteMsg = (IMAPMessage *)(remote->objectAtIndex(ii));
        uint32_t remoteUID = remoteMsg->uid();

        // Step 3: Collect messages that are different or not in our local UID set.
        bool inFolder = (local.count(remoteUID) > 0);
        bool same = inFolder && Utils::messageAttributesMatch(local[remoteUID], Utils::messageAttributesForMessage(remoteMsg));

        if (!inFolder || !same) {
            // Step 4: Attempt to insert the new message. If we get unique exceptions,
            // look for the existing message and do an update instead. This happens whenever
            // a message has moved between folders or it's attributes have changed.

            // Note: We could prefetch all changedOrMissingIDs and then decide to update/insert,
            // but we can only query for 500 at a time, it /feels/ nasty, and we /could/ always
            // hit the exception anyway since another thread could be IDLEing and retrieving
            // the messages alongside us.
            if (heavyInitialRequest) {
                auto local = m_mailProcessor->insertFallbackToUpdateMessage(remoteMsg, folder, syncDataTimestamp);
                if (syncedMessages != nullptr) {
                    syncedMessages->push_back(local);
                }
            } else {
                if (heavyNeededIdeal < MAX_FULL_HEADERS_REQUEST_SIZE) {
                    heavyNeeded->addIndex(remoteUID);
                }
                heavyNeededIdeal += 1;
            }
        }

        local.remove(remoteUID);
    }

    if (!heavyInitialRequest && heavyNeeded->count() > 0) {
        qDebug() << "Fetching full headers for" << heavyNeeded->count() << "(of" << heavyNeededIdeal << "needed)";

        // Note: heavyNeeded could be enormous if the user added a zillion items to a folder, if it's been
        // years since the app was launched, or if a sync bug caused us to delete messages we shouldn't have.
        // (eg the issue with uidnext becoming zero suddenly)
        //
        // We don't re-fetch them all in one request because it could be an impossibly large amount of data.
        // Instead we sync MAX_FULL_HEADERS_REQUEST_SIZE and on the next "deep scan" in 10 minutes, we'll
        // sync X more.
        //
        syncDataTimestamp = time(0);
        auto kind = Utils::messagesRequestKindFor(m_imapSession->storedCapabilities(), true);
        remote = m_imapSession->fetchMessagesByUID(&path, kind, heavyNeeded, &cb, &err);

        if (err != ErrorNone) {
            qWarning() << "ISSUE: syncFolderUIDRange - fetchMessagesByUID (heavy) " << QString::fromStdString(ErrorCodeToTypeMap[err]);
        }

        for (int ii = ((int)remote->count()) - 1; ii >= 0; ii--) {
            IMAPMessage * remoteMsg = (IMAPMessage *)(remote->objectAtIndex(ii));
            auto local = m_mailProcessor->insertFallbackToUpdateMessage(remoteMsg, folder, syncDataTimestamp);
            if (syncedMessages != nullptr) {
                syncedMessages->push_back(local);
            }
            remote->removeLastObject();
        }
    }

    // Step 5: Unlink. The messages left in local map are the ones we had in the range,
    // which the server reported were no longer there. Remove their remoteUID.
    // We'll delete them later if they don't appear in another folder during sync.
    if (local.size() > 0) {
        QList<uint32_t> deletedUIDs = local.keys();
        
        for (QList<uint32_t> chunk : Utils::chunksOfVector(deletedUIDs, 200)) {
            QString chunkList;
            for (int i = 0; i < chunk.size(); ++i) {
                if (i != 0) {
                    chunkList += QStringLiteral(", ");
                }
                chunkList += QString::number(chunk[i]);
            }
            
            QSqlQuery query{db};
            query.prepare(QStringLiteral("SELECT * FROM message WHERE folderId = ? AND remoteUID IN(") + chunkList + QStringLiteral(")"));
            query.addBindValue(folder.id());
            
            m_mailProcessor->unlinkMessagesMatchingQuery(query, m_unlinkPhase);
        }
    }
}

void AccountWorker::syncFolderChangesViaCondstore(Folder &folder, IMAPFolderStatus &remoteStatus, bool mustSyncAll)
{
    // allocated mailcore objects freed when `pool` is removed from the stack
    AutoreleasePool pool;

    uint32_t uidnext = folder.localStatus()[LS_UIDNEXT].toInt();
    uint64_t modseq = folder.localStatus()[LS_HIGHESTMODSEQ].toInt();
    uint64_t remoteModseq = remoteStatus.highestModSeqValue();
    uint32_t remoteUIDNext = remoteStatus.uidNext();
    time_t syncDataTimestamp = time(0);

    qDebug().nospace() << "syncFolderChangesViaCondstore - " << folder.path() << ": modseq " << modseq << " to " << remoteModseq << ", uidnext " << uidnext << " to " << remoteUIDNext;

    if (modseq == remoteModseq && uidnext == remoteUIDNext) {
        return;
    }

    // if the difference between our stored modseq and highestModseq is very large,
    // we can create a request that takes forever to complete and /blocks/ the foreground
    // worker from performing mailbox actions, which is really bad. To bound the request,
    // we ask for changes within the last 25,000 UIDs only. Our intermittent "deep" scan
    // will recover the rest of the changes so it's safe not to ingest them here.
    IndexSet * uids = IndexSet::indexSetWithRange(RangeMake(1, UINT64_MAX));
    if (!mustSyncAll && remoteModseq - modseq > MODSEQ_TRUNCATION_THRESHOLD) {
        uint32_t bottomUID = remoteUIDNext > MODSEQ_TRUNCATION_UID_COUNT ? remoteUIDNext - MODSEQ_TRUNCATION_UID_COUNT : 1;
        uids = IndexSet::indexSetWithRange(RangeMake(bottomUID, UINT64_MAX));
        qWarning() << "syncFolderChangesViaCondstore - request limited to " << bottomUID << "-*, remaining changes will be detected via deep scan";
    }

    IMAPProgress cb;
    ErrorCode err = ErrorCode::ErrorNone;
    String path(AS_MCSTR(folder.path().toStdString()));

    auto kind = Utils::messagesRequestKindFor(m_imapSession->storedCapabilities(), true);
    IMAPSyncResult *result = m_imapSession->syncMessagesByUID(&path, kind, uids, modseq, &cb, &err);
    
    if (err != ErrorCode::ErrorNone) {
        qWarning() << "ISSUE: syncFolderChangesViaCondstore - syncMessagesByUID" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return;
    }

    // for modified messages, fetch local copy and apply changes
    Array * modifiedOrAdded = result->modifiedOrAddedMessages();
    IndexSet * vanished = result->vanishedMessages();

    qDebug() << "syncFolderChangesViaCondstore - Changes since HMODSEQ " << modseq << ": " << modifiedOrAdded->count() << " changed, " << QString::number((vanished != nullptr) ? vanished->count() : 0) << " vanished";

    QSqlDatabase db = getDB();
    
    for (unsigned int i = 0; i < modifiedOrAdded->count(); ++i) {
        IMAPMessage * msg = (IMAPMessage *)modifiedOrAdded->objectAtIndex(i);
        QString id = Utils::idForMessage(folder.accountId(), folder.path(), msg);

        QSqlQuery query{db};
        query.prepare(QStringLiteral("SELECT * FROM message WHERE id = ?"));
        query.addBindValue(id);
        
        Utils::execWithLog(query, "syncFolderChangesViaCondstore - select message");
        
        if (query.next()) {
            auto local = std::make_shared<Message>(nullptr, query);

            if (local == nullptr) {
                // Found message with an ID we've never seen in any folder. Add it!
                m_mailProcessor->insertFallbackToUpdateMessage(msg, folder, syncDataTimestamp);
            } else {
                // Found message with an existing ID. Update it's attributes & folderId.
                // Note: Could potentially have moved from another folder!
                local->setFolderId(folder.id());
                local->setSyncedAt(syncDataTimestamp);
                
                // TODO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
                // SHOULD UPDATE OTHER FIELDS OF THE MESSAGE
                
                local->saveToDb(db);
            }
        }
    }

    // for deleted messages, collect UIDs and destroy. Note: vanishedMessages is only
    // populated when QRESYNC is available. IMPORTANT: vanished may include an infinite
    // range, like 12:* so we can't convert it to a fixed array.
    
    // TODO 
    // if (vanished != NULL) {
    //     vector<Query> queries = Utils::queriesForUIDRangesInIndexSet(folder.id(), vanished);
    //     for (Query & query : queries) {
    //         m_mailProcessor->unlinkMessagesMatchingQuery(query, m_unlinkPhase);
    //     }
    // }

    folder.localStatus()[LS_UIDNEXT] = (int) remoteUIDNext;
    folder.localStatus()[LS_HIGHESTMODSEQ] = (int) remoteModseq;
}

void AccountWorker::cleanMessageCache(Folder &folder) {
    qDebug() << "Cleaning local cache and updating stats";

    // delete bodies we no longer want. Note: you can't do INNER JOINs within a DELETE
    // note: we only delete messages fetchedd more than 14 days ago to avoid deleting
    // old messages you're actively viewing / could still want
    QSqlDatabase db = getDB();
    QSqlQuery purgeQuery{db};
    purgeQuery.prepare(QStringLiteral(
        "DELETE FROM message_body "
            "WHERE message_body.fetchedAt < datetime('now', '-14 days') "
                "AND message_body.id IN (SELECT message.id FROM message WHERE message.folderId = ? AND message.draft = 0 AND message.date < ?)"));
    purgeQuery.bindValue(0, folder.id());
    purgeQuery.bindValue(1, (double) (time(0) - maxAgeForBodySync(folder)));
    
    Utils::execWithLog(purgeQuery, "cleanMessageCache");
    
    qDebug() << "-- message bodies deleted from local cache.";
    // TODO BG: Remove them from the search index and remove attachments

    // update messages body stats
    folder.localStatus()[LS_BODIES_PRESENT] = countBodiesDownloaded(folder);
    folder.localStatus()[LS_BODIES_WANTED] = countBodiesNeeded(folder);
}


time_t AccountWorker::maxAgeForBodySync(Folder &folder) {
    return 24 * 60 * 60 * 30 * 3; // three months TODO pref!
}

bool AccountWorker::shouldCacheBodiesInFolder(Folder &folder) {
    if ((folder.role() == QStringLiteral("spam")) || (folder.role() == QStringLiteral("trash"))) {
        return false;
    }
    return true;
}

long long AccountWorker::countBodiesDownloaded(Folder &folder) {
    QSqlDatabase db = getDB();
    QSqlQuery query{db};
    query.prepare(QStringLiteral(
        "SELECT COUNT(message.id) FROM message "
            "INNER JOIN message_body ON message_body.id = message.id "
            "WHERE message_body.value IS NOT NULL AND message.folderId = ?"));
    query.bindValue(0, folder.id());
    
    Utils::execWithLog(query, "countBodiesNeeded");
    
    query.next();
    return query.value(0).toDouble();
}

long long AccountWorker::countBodiesNeeded(Folder &folder) {
    if (!shouldCacheBodiesInFolder(folder)) {
        return 0;
    }

    QSqlDatabase db = getDB();
    QSqlQuery query{db};
    query.prepare(QStringLiteral(
        "SELECT COUNT(message.id) FROM message "
            "WHERE message.folderId = ? AND (message.date > ? OR message.draft = 1) AND message.remoteUid > 0"));
    query.bindValue(0, folder.id());
    query.bindValue(1, (double) (time(0) - maxAgeForBodySync(folder)));
    
    Utils::execWithLog(query, "countBodiesNeeded");
    
    query.next();
    return query.value(0).toDouble();
}

// Syncs the top N missing message bodies. Returns true if it did work, false if it did nothing.
bool AccountWorker::syncMessageBodies(Folder &folder, IMAPFolderStatus &remoteStatus) {
    if (!shouldCacheBodiesInFolder(folder)) {
        return false;
    }

    QSqlDatabase db = getDB();

    // very slow query = 400ms+
    QSqlQuery missingQuery{db};
    missingQuery.prepare(QStringLiteral(
        "SELECT message.id, message.remoteUID FROM message "
            "LEFT JOIN message_body ON message_body.id = message.id "
            "WHERE message.accountId = ? AND message.folderId = ? AND (message.date > ? OR message.draft = 1) AND message.remoteUID > 0 AND message_body.id IS NULL "
            "ORDER BY message.date DESC LIMIT 30"));
    missingQuery.bindValue(0, folder.accountId());
    missingQuery.bindValue(1, folder.id());
    missingQuery.bindValue(2, (double) (0 - maxAgeForBodySync(folder)));
    Utils::execWithLog(missingQuery, "syncMessageBodies - find messages with missing bodies");

    QStringList ids;
    while (missingQuery.next()) {
        if (missingQuery.value(1).toUInt() >= UINT32_MAX - 2) {
            continue; // message is scheduled for cleanup
        }
        ids.push_back(missingQuery.value(0).toString());
    }

    QSqlQuery stillMissingQuery{db};
    stillMissingQuery.prepare(QString(QStringLiteral(
        "SELECT message.* FROM message "
            "LEFT JOIN message_body ON message_body.id = message.id "
            "WHERE message.id IN (%1) AND message_body.id IS NULL")).arg(Utils::qmarks(ids.size())));

    QSqlQuery insertPlaceholderQuery{db};
    insertPlaceholderQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO message_body (id, value) VALUES (?, ?)"));

    QList<std::shared_ptr<Message>> results;
    {
        db.transaction();

        // very fast query for the messages found during very slow query that still have no message body.
        // Inserting empty message body reserves them for processing here. We do this within a transaction
        // to ensure we don't process the same message twice.
        for (int i = 0; i < ids.size(); ++i) {
            stillMissingQuery.bindValue(i, ids[i]);
        }
        Utils::execWithLog(stillMissingQuery, "syncMessageBodies - find all fields for messages found missing bodies");

        
        while (stillMissingQuery.next()) {
            results.push_back(std::make_shared<Message>(nullptr, stillMissingQuery));
        }

        if (results.size() < ids.size()) {
            qDebug() << "Body for" << ids.size() - results.size() << "messages already being fetched.";
        }

        for (auto result : results) {
            // write a blank entry into the MessageBody table so we'll only try to fetch each
            // message once. Otherwise a persistent ErrorFetch or crash for a single message
            // can cause the account to stay "syncing" forever.
            insertPlaceholderQuery.bindValue(0, result->id());
            insertPlaceholderQuery.bindValue(1, QString());
            Utils::execWithLog(insertPlaceholderQuery, "syncMessageBodies - insert placeholder message body");
        }

        db.commit();
    }

    auto &ls = folder.localStatus();
    if (ls[LS_BODIES_PRESENT].isUndefined() || !ls[LS_BODIES_PRESENT].isDouble()) {
        ls[LS_BODIES_PRESENT] = 0;
    }

    for (auto result : results) {
        // increment local sync state - it's fine if this sometimes fails to save,
        // we recompute the value via COUNT(*) during cleanup
        ls[LS_BODIES_PRESENT] = ls[LS_BODIES_PRESENT].toDouble() + 1;

        // attempt to fetch the message body
        syncMessageBody(result.get());
    }

    return results.size() > 0;
}

void AccountWorker::syncMessageBody(Message *message) {
    // allocated mailcore objects freed when `pool` is removed from the stack
    AutoreleasePool pool;

    IMAPProgress cb;
    ErrorCode err = ErrorCode::ErrorNone;

    QSqlDatabase db = getDB();
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM folder WHERE id = ?"));
    query.bindValue(0, message->folderId());
    Utils::execWithLog(query, "fetch folder for syncMessageBody");
    
    if (!query.next()) {
        return;
    }
    
    auto folder = std::make_shared<Folder>(nullptr, query);
    QString folderPath = folder->path();
    String path(AS_MCSTR(folderPath.toStdString()));

    Data * data = m_imapSession->fetchMessageByUID(&path, message->remoteUid().toUInt(), &cb, &err);
    
    if (err != ErrorNone) {
        qWarning() << QString(QStringLiteral("Unable to fetch body for message \"%1\" (%2 UID %3). Error %4")).arg(message->subject(), folderPath, message->remoteUid(), QString::fromStdString(ErrorCodeToTypeMap[err]));

        if (err == ErrorFetch) {
            // syncing message bodies can fail often
            return;
        }

        qWarning() << "ISSUE: syncMessageBody - fetchMessageByUID:" << QString::fromStdString(ErrorCodeToTypeMap[err]);
        return;
    }
    
    MessageParser *messageParser = MessageParser::messageParserWithData(data);
    m_mailProcessor->retrievedMessageBody(message, messageParser);
}
