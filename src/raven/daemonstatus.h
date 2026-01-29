// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QDBusServiceWatcher>
#include <qqmlintegration.h>

// Track the availability of the Raven daemon on D-Bus
class DaemonStatus : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit DaemonStatus(QObject *parent = nullptr);
    ~DaemonStatus() override;

    static DaemonStatus *self();

    bool isAvailable() const;

    // Triggers D-Bus activation if the daemon is not running
    Q_INVOKABLE void activateDaemon();

Q_SIGNALS:
    void availableChanged();

private Q_SLOTS:
    void onServiceRegistered(const QString &serviceName);
    void onServiceUnregistered(const QString &serviceName);

private:
    void checkCurrentStatus();

    QDBusServiceWatcher *m_watcher = nullptr;
    bool m_available = false;
};
