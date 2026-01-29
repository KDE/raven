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

void DaemonStatus::activateDaemon()
{
    if (m_available) {
        qDebug() << "Daemon already available, no activation needed";
        return;
    }

    qDebug() << "Attempting to activate daemon via D-Bus...";

    // Request D-Bus to start the service
    // This triggers D-Bus activation via the .service file
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    if (iface) {
        QDBusReply<void> reply = iface->startService(DBUS_SERVICE);
        if (!reply.isValid()) {
            qWarning() << "Failed to start daemon service:" << reply.error().message();
        }
        // If successful, onServiceRegistered will be called when daemon starts
    } else {
        qWarning() << "D-Bus session bus interface not available";
    }
}

void DaemonStatus::onServiceRegistered(const QString &serviceName)
{
    if (serviceName == DBUS_SERVICE) {
        qDebug() << "Daemon service registered on D-Bus";

        m_available = true;

        Q_EMIT availableChanged();
    }
}

void DaemonStatus::onServiceUnregistered(const QString &serviceName)
{
    if (serviceName == DBUS_SERVICE) {
        qDebug() << "Daemon service unregistered from D-Bus";

        m_available = false;

        Q_EMIT availableChanged();
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
        }

        qDebug() << "Daemon status check: available =" << m_available;
    }
}
