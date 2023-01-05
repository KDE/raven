// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "constants.h"
#include "backendworker/accountworker.h"

#include <QObject>
#include <QThread>
#include <QList>

#include <MailCore/MailCore.h>

using namespace mailcore;

class Raven : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedFolderName READ selectedFolderName NOTIFY selectedFolderNameChanged)

public:
    Raven(QObject *parent = nullptr);
    virtual ~Raven();
    
    static Raven *self();

    QString selectedFolderName() const;

    void addAccountWorker(Account *account);
    void removeAccountWorker(Account *account);

Q_SIGNALS:
    void selectedFolderNameChanged();

private:
    QList<std::pair<AccountWorker *, QThread *>> m_workers;
};
