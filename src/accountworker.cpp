// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

// Much of the logic is based on https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/SyncWorker.cpp

#include "accountworker.h"
#include "constants.h"
#include "utils.h"
#include "dbmanager.h"
#include "models/folder.h"
#include "models/label.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QHash>
#include <QDebug>

AccountWorker::AccountWorker(QObject *parent, Account *account)
    : QObject{parent}
    , m_account{account}
    , m_imapSession{new IMAPSession}
{
}

AccountWorker::~AccountWorker()
{
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

    auto folders = fetchFoldersAndLabels();

    // TODO
    for (const auto &folder : folders) {
        qDebug() << "FOLDER:" << folder->id() << folder->path();
    }
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
}

QList<std::shared_ptr<Folder>> AccountWorker::fetchFoldersAndLabels()
{
    QSqlDatabase db = getDB();

    ErrorCode err = ErrorCode::ErrorNone;
    Array *remoteFolders = m_imapSession->fetchAllFolders(&err);

    if (err) {
        // TODO better error handling (enqueue job to retry, log errors)
        qWarning() << "ISSUE: fetchFoldersAndLabels" << err;
        return {};
    }

    QList<std::shared_ptr<Folder>> foldersToSync;
    QHash<QString, std::shared_ptr<Folder>> allFoundCategories;

    QString mainPrefix = Utils::namespacePrefixOrBlank(m_imapSession);

    bool isGmail = m_imapSession->storedCapabilities()->containsIndex(IMAPCapabilityGmail);


    // find all stored folders and labels

    QHash<QString, std::shared_ptr<Folder>> unusedLocalFolders;
    QHash<QString, std::shared_ptr<Label>> unusedLocalLabels;

    QSqlQuery folderQuery{QStringLiteral("SELECT * FROM ") + FOLDERS_TABLE + QStringLiteral(" WHERE accountId = ") + m_account->id(), db};
    while (folderQuery.next()) {
        auto folder = std::make_shared<Folder>(nullptr, folderQuery);
        unusedLocalFolders[folder->id()] = folder;
    }

    QSqlQuery labelQuery{QStringLiteral("SELECT * FROM ") + LABELS_TABLE + QStringLiteral(" WHERE accountId = ") + m_account->id(), db};
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
