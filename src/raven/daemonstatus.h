// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QDBusServiceWatcher>
#include <qqmlintegration.h>

/**
 * @brief Tracks the availability of the Raven daemon on D-Bus
 *
 * This class monitors the org.kde.raven.daemon service on the session bus
 * and provides a property that can be used in QML to show/hide UI elements
 * based on daemon availability.
 *
 * D-Bus activation is configured so the daemon will auto-start when methods
 * are called, but this class provides immediate feedback about current status.
 */
class DaemonStatus : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(bool connecting READ isConnecting NOTIFY connectingChanged)

public:
    explicit DaemonStatus(QObject *parent = nullptr);
    ~DaemonStatus() override;

    static DaemonStatus *self();

    /**
     * @brief Check if the daemon is currently available on D-Bus
     */
    bool isAvailable() const;

    /**
     * @brief Check if we're currently trying to connect to the daemon
     */
    bool isConnecting() const;

    /**
     * @brief Attempt to activate the daemon via D-Bus
     *
     * This triggers D-Bus activation if the daemon is not running.
     * The daemon should start automatically due to the .service file.
     */
    Q_INVOKABLE void activateDaemon();

Q_SIGNALS:
    /**
     * @brief Emitted when daemon availability changes
     */
    void availableChanged();

    /**
     * @brief Emitted when connecting state changes
     */
    void connectingChanged();

    /**
     * @brief Emitted when the daemon comes online
     */
    void daemonOnline();

    /**
     * @brief Emitted when the daemon goes offline
     */
    void daemonOffline();

private Q_SLOTS:
    void onServiceRegistered(const QString &serviceName);
    void onServiceUnregistered(const QString &serviceName);

private:
    void checkCurrentStatus();

    QDBusServiceWatcher *m_watcher = nullptr;
    bool m_available = false;
    bool m_connecting = false;
};
