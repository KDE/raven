// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDriver>

// The static instance watches over database events to trigger updates in the UI.
class DBWatcher : public QObject
{
    Q_OBJECT
public:
    DBWatcher(QObject *parent = nullptr);

    static DBWatcher *self();

    void initWatcher();

public Q_SLOTS:
    void notificationSlot(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload);
};
