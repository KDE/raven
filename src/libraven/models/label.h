// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime>

#include "messagecontact.h"
#include "folder.h"

class Label : public Folder
{
    Q_OBJECT

public:
    Label(QObject *parent = nullptr, QString id = QString(), QString accountId = QString());
    Label(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const override;
    void deleteFromDb(QSqlDatabase &db) const override;
};


