// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QHash>

#include "../libraven/models/folder.h"
#include "../libraven/utils.h"

class DBManager : public QObject
{
    Q_OBJECT

public:
    DBManager(QObject *parent = nullptr);

    static DBManager *self();

    static void exec(QSqlQuery &query);
};

