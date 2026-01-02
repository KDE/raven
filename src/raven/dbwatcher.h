// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDBusConnection>
#include <QObject>

#include "ravendaemoninterface.h"

// Watches for database change signals from the ravend daemon via D-Bus.
class DBWatcher : public QObject
{
    Q_OBJECT

public:
    explicit DBWatcher(QObject *parent = nullptr);

    void initWatcher();

Q_SIGNALS:
    // Emitted when any table changes
    void tableChanged(const QString &tableName);

    // Specific database tables
    void foldersChanged();
    void threadsChanged();
    void messagesChanged();
    void labelsChanged();
    void filesChanged();

    // Emitted when specific messages are updated
    void specificMessagesChanged(const QStringList &messageIds);

public Q_SLOTS:
    void onTableChanged(const QString &tableName);
    void onMessagesChanged(const QStringList &messageIds);

private:
    void handleTableChange(const QString &tableName);

    OrgKdeRavenDaemonInterface *m_interface = nullptr;
};
