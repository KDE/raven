// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

// Database manager for libraven.
// Provides static methods to open and close database connections.
// Does not use a singleton pattern - callers manage their own connections.
class DBManager : public QObject
{
    Q_OBJECT

public:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();

    // Open a database connection and return it.
    // If connectionName is empty, a unique name will be generated.
    // Caller is responsible for managing the connection lifecycle.
    static QSqlDatabase openDatabase(const QString &connectionName = QString());

    // Close a specific database connection.
    static void closeDatabase(const QString &connectionName);

    // Get the default database path.
    static QString defaultDatabasePath();
};
