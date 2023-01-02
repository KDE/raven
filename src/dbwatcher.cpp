// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbwatcher.h"
#include "constants.h"
#include "modelviews/mailboxmodel.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QDebug>

DBWatcher::DBWatcher(QObject *parent)
    : QObject{parent}
{
}

DBWatcher *DBWatcher::self()
{
    static DBWatcher *instance = new DBWatcher;
    return instance;
}

void DBWatcher::initWatcher()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlDriver *driver = db.driver();

    connect(driver, QOverload<const QString &, QSqlDriver::NotificationSource, const QVariant &>::of(&QSqlDriver::notification), this, &DBWatcher::notificationSlot);

    driver->subscribeToNotification(FOLDERS_TABLE);
    driver->subscribeToNotification(LABELS_TABLE);
    driver->subscribeToNotification(MESSAGES_TABLE);
}

void DBWatcher::notificationSlot(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    qDebug() << "dbwatcher: event received:" << name << source << payload;

    if (name == FOLDERS_TABLE) {
        // reload folders
        MailBoxModel::self()->load();
    } else if (name == LABELS_TABLE) {

    } else if (name == MESSAGES_TABLE) {
        // TODO
    }
}
