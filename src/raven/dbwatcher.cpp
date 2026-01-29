// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbwatcher.h"
#include "constants.h"

#include <QDBusConnection>
#include <QDebug>

DBWatcher::DBWatcher(QObject *parent)
    : QObject{parent}
{
}

void DBWatcher::initWatcher()
{
    m_interface = new OrgKdeRavenDaemonInterface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());

    connect(m_interface, &OrgKdeRavenDaemonInterface::TableChanged, this, &DBWatcher::onTableChanged);
    connect(m_interface, &OrgKdeRavenDaemonInterface::MessagesChanged, this, &DBWatcher::onMessagesChanged);
}

void DBWatcher::onTableChanged(const QString &tableName)
{
    qDebug() << "DBWatcher: D-Bus signal received for table:" << tableName;
    handleTableChange(tableName);
}

void DBWatcher::onMessagesChanged(const QStringList &messageIds)
{
    qDebug() << "DBWatcher: MessagesChanged D-Bus signal received for" << messageIds.size() << "messages";
    Q_EMIT specificMessagesChanged(messageIds);
}

void DBWatcher::handleTableChange(const QString &tableName)
{
    // Emit generic signal
    Q_EMIT tableChanged(tableName);

    // Emit specific signals
    if (tableName == QStringLiteral("folder")) {
        Q_EMIT foldersChanged();
    } else if (tableName == QStringLiteral("thread")) {
        Q_EMIT threadsChanged();
    } else if (tableName == QStringLiteral("message")) {
        Q_EMIT messagesChanged();
    } else if (tableName == QStringLiteral("label")) {
        Q_EMIT labelsChanged();
    } else if (tableName == QStringLiteral("file")) {
        Q_EMIT filesChanged();
    }
}
