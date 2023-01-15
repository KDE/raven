// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QHash>

#include <models/folder.h>
#include <utils.h>

class DBManager : public QObject
{
    Q_OBJECT

public:
    DBManager(QObject *parent = nullptr);

    static DBManager *self();

    static void exec(QSqlQuery &query);

    void migrate();
    
    static QHash<uint32_t, MessageAttributes> fetchMessagesAttributesInRange(Range range, Folder &folder, QSqlDatabase &db);
    static uint32_t fetchMessageUIDAtDepth(QSqlDatabase &db, Folder &folder, uint32_t depth, uint32_t before);
    
private:
    void migrationV1(uint current);
};
