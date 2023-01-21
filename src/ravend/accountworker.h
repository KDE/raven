// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <MailCore/MailCore.h>

#include "../libraven/models/account.h"
#include "../libraven/models/folder.h"
#include "../libraven/models/message.h"
#include "../libraven/models/label.h"
#include "mailprocessor.h"

using namespace mailcore;

class MailProcessor;

// Worker that handles a single account, processing all backend mail tasks.
//
// Each account has a job queue which is intended to be processed by its own AccountWorker.
// The UI should not communicate with this thread (this is intended to eventually be part of a daemon process).
// All data is synced to the sqlite db, which the UI should fetch its data from.
class AccountWorker : public QObject
{
    Q_OBJECT

public:
    AccountWorker(QObject *parent = nullptr, Account *account = nullptr);
    virtual ~AccountWorker();

    Account *account() const;
    QSqlDatabase getDB() const;

    void run();
    
    void idleCycleIteration();

    void cleanMessageCache(Folder &folder);
    time_t maxAgeForBodySync(Folder &folder);
    bool shouldCacheBodiesInFolder(Folder &folder);
    long long countBodiesDownloaded(Folder &folder);
    long long countBodiesNeeded(Folder &folder);
    bool syncMessageBodies(Folder &folder, IMAPFolderStatus &remoteStatus);
    void syncMessageBody(Message *message);

private:
    void setupSession();
    QList<std::shared_ptr<Folder>> syncFoldersAndLabels();

    bool syncNow();
    void syncFolderUIDRange(Folder &folder, Range range, bool heavyInitialRequest, QList<std::shared_ptr<Message>> * syncedMessages = nullptr);
    void syncFolderChangesViaCondstore(Folder &folder, IMAPFolderStatus &remoteStatus, bool mustSyncAll);

    Account *m_account;
    IMAPSession *m_imapSession;
    MailProcessor *m_mailProcessor;

    int m_syncIterationsSinceLaunch;
    int m_unlinkPhase;
};
