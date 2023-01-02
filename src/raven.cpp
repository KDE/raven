// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"
#include "modelviews/accountmodel.h"

#include <QDebug>

// notes:
// imap config - https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/MailUtils.cpp#L714

Raven::Raven(QObject *parent)
    : QObject(parent)
{
    connect(AccountModel::self(), &AccountModel::accountAdded, this, &Raven::addAccountWorker);
    connect(AccountModel::self(), &AccountModel::accountRemoved, this, &Raven::removeAccountWorker);

    // fetch accounts
    for (auto account : AccountModel::self()->accounts()) {
        addAccountWorker(account);
    }
}

Raven::~Raven()
{
    for (auto pair : m_workers) {
        auto worker = pair.first;
        auto thread = pair.second;

        thread->quit();
        thread->wait();

        delete worker;
        delete thread;
    }
}

Raven *Raven::self()
{
    static Raven *instance = new Raven();
    return instance;
}

QString Raven::selectedFolderName() const
{
    return {}; // TODO
}

void Raven::addAccountWorker(Account *account)
{
    QThread *thread = new QThread{this};
    AccountWorker *worker = new AccountWorker{nullptr, account};

    connect(thread, &QThread::started, worker, &AccountWorker::run);

    worker->moveToThread(thread);
    thread->start();

    m_workers.push_back({worker, thread});
}

void Raven::removeAccountWorker(Account *account)
{
    for (int i = 0; i < m_workers.count(); ++i) {
        auto worker = m_workers[i].first;
        auto thread = m_workers[i].second;

        if (worker->account() == account) {
            connect(thread, &QThread::finished, worker, &AccountWorker::deleteLater);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            thread->quit();
            m_workers.erase(m_workers.begin() + i);

            break;
        }
    }
}
