// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class TaskProcessor : public QObject
{
    Q_OBJECT
    
public:
    TaskProcessor(QObject *parent = nullptr, AccountWorker *accountWorker);
    
private:
    AccountWorker *m_accountWorker;
};
