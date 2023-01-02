// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <MailCore/MailCore.h>

#include "models/account.h"
#include "models/folder.h"

using namespace mailcore;

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


private:
    void setupSession();
    QList<std::shared_ptr<Folder>> fetchFoldersAndLabels();

    Account *m_account;
    IMAPSession *m_imapSession;
};
