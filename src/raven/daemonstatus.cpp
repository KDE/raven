// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemonstatus.h"
#include "constants.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDebug>

DaemonStatus::DaemonStatus(QObject *parent)
    : QObject(parent)
{
    // Create service watcher for the daemon
    m_watcher = new QDBusServiceWatcher(
        DBUS_SERVICE,
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
    );

    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &DaemonStatus::onServiceRegistered);
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &DaemonStatus::onServiceUnregistered);

    // Check current status on startup
    checkCurrentStatus();
}

DaemonStatus::~DaemonStatus() = default;

DaemonStatus *DaemonStatus::self()
{
    static DaemonStatus *instance = new DaemonStatus();
    return instance;
}

bool DaemonStatus::isAvailable() const
{
    return m_available;
}

bool DaemonStatus::isConnecting() const
{
    return m_connecting;
}

void DaemonStatus::activateDaemon()
{
    if (m_available) {
        qDebug() << "Daemon already available, no activation needed";
        return;
    }

    if (m_connecting) {
        qDebug() << "Already attempting to connect to daemon";
        return;
    }

    qDebug() << "Attempting to activate daemon via D-Bus...";

    m_connecting = true;
    Q_EMIT connectingChanged();

    // Request D-Bus to start the service
    // This triggers D-Bus activation via the .service file
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    if (iface) {
        QDBusReply<void> reply = iface->startService(DBUS_SERVICE);
        if (!reply.isValid()) {
            qWarning() << "Failed to start daemon service:" << reply.error().message();
            m_connecting = false;
            Q_EMIT connectingChanged();
        }
        // If successful, onServiceRegistered will be called when daemon starts
    } else {
        qWarning() << "D-Bus session bus interface not available";
        m_connecting = false;
        Q_EMIT connectingChanged();
    }
}

void DaemonStatus::onServiceRegistered(const QString &serviceName)
{
    if (serviceName == DBUS_SERVICE) {
        qDebug() << "Daemon service registered on D-Bus";

        m_connecting = false;
        m_available = true;

        Q_EMIT connectingChanged();
        Q_EMIT availableChanged();
        Q_EMIT daemonOnline();
    }
}

void DaemonStatus::onServiceUnregistered(const QString &serviceName)
{
    if (serviceName == DBUS_SERVICE) {
        qDebug() << "Daemon service unregistered from D-Bus";

        m_available = false;

        Q_EMIT availableChanged();
        Q_EMIT daemonOffline();
    }
}

void DaemonStatus::checkCurrentStatus()
{
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    if (iface) {
        bool wasAvailable = m_available;
        m_available = iface->isServiceRegistered(DBUS_SERVICE);

        if (m_available != wasAvailable) {
            Q_EMIT availableChanged();
            if (m_available) {
                Q_EMIT daemonOnline();
            } else {
                Q_EMIT daemonOffline();
            }
        }

        qDebug() << "Daemon status check: available =" << m_available;
    }
}
