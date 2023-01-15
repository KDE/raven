// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../libraven/constants.h"
#include "accountworker.h"

#include <QObject>
#include <QThread>
#include <QList>

#include <MailCore/MailCore.h>

using namespace mailcore;

class WorkerManager : public QObject
{
    Q_OBJECT

public:
    WorkerManager(QObject *parent = nullptr);
    virtual ~WorkerManager();
    
    static WorkerManager *self();

    void addAccountWorker(Account *account);
    void removeAccountWorker(Account *account);

private:
    QList<std::pair<AccountWorker *, QThread *>> m_workers;
};

