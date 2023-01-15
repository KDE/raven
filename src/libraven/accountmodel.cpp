// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/account.h"
#include "accountmodel.h"
#include "constants.h"

#include <QDir>
#include <QStandardPaths>

#include <uuid.h>

AccountModel::AccountModel(QObject *parent)
    : QAbstractListModel{parent}
{
    load();
}

AccountModel *AccountModel::self()
{
    static AccountModel *instance = new AccountModel;
    return instance;
}

const QList<Account *> &AccountModel::accounts() const
{
    return m_accounts;
}

void AccountModel::load()
{
    QString accountsFolder = RAVEN_CONFIG_LOCATION + QStringLiteral("/accounts");
    if (!QDir().mkpath(accountsFolder)) {
        qWarning() << "Could not create accounts config folder" << accountsFolder;
    }

    // loop over folders in raven/accounts folder
    for (auto entry : QDir{accountsFolder}.entryList(QStringList{}, QDir::Dirs)) {

        // skip "." and ".." folder
        if (entry == QStringLiteral(".") || entry == QStringLiteral("..")) {
            continue;
        }

        QString configPath = accountsFolder + QStringLiteral("/") + entry + QStringLiteral("/account.ini");

        // load config
        auto config = new KConfig{configPath};
        if (config) {
            auto account = new Account{this, config};
            m_accounts.push_back(account);
            Q_EMIT accountAdded(account);
        }
    }
}

void AccountModel::addAccount(Account *account)
{
    beginInsertRows(QModelIndex(), m_accounts.count(), m_accounts.count());
    m_accounts.push_back(account);
    endInsertRows();

    Q_EMIT accountAdded(account);
}

void AccountModel::removeAccount(int index)
{
    if (index < 0 || index >= m_accounts.count()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    Account *account = m_accounts[index];
    m_accounts.erase(m_accounts.begin() + index);
    endRemoveRows();

    Q_EMIT accountRemoved(account);
    account->deleteLater();

    // TODO enqueue jobs to delete entries from sql database
}

Account *AccountModel::accountById(const QString &id)
{
    for (auto account : m_accounts) {
        if (account->id() == id) {
            return account;
        }
    }
    return nullptr;
}

int AccountModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_accounts.count();
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    if (!index.isValid() || index.row() >= m_accounts.count()) {
        return QVariant{};
    }

    auto *account = m_accounts[index.row()];
    return account ? QVariant::fromValue(account) : QVariant{};
}

Qt::ItemFlags AccountModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> AccountModel::roleNames() const
{
    return {{AccountRole, "account"}};
}
