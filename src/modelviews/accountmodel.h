// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>

#include "models/account.h"

class AccountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    AccountModel(QObject *parent = nullptr);

    enum {
        AccountRole,
    };

    static AccountModel *self();
    const QList<Account *> &accounts() const;

    void load();
    void addAccount(Account *account);
    Q_INVOKABLE void removeAccount(int index);

    Account *accountById(const QString &id);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void accountAdded(Account *account);
    void accountRemoved(Account *account);

private:
    QList<Account *> m_accounts;
};
