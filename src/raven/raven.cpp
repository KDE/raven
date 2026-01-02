// SPDX-FileCopyrightText: 2022-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"
#include "ravendaemoninterface.h"

#include <QDebug>

Raven::Raven(QObject *parent)
    : QObject(parent)
    , m_accountModel{AccountModel::self()}
    , m_attachmentModel{AttachmentModel::self()}
    , m_daemonStatus{DaemonStatus::self()}
    , m_mailBoxModel{MailBoxModel::self()}
    , m_mailListModel{MailListModel::self()}
    , m_threadViewModel{ThreadViewModel::self()}
{
    // Try to activate daemon if not already running
    // D-Bus activation will auto-start it via the .service file
    if (!m_daemonStatus->isAvailable()) {
        m_daemonStatus->activateDaemon();
    }

    // Open main database connection
    QSqlDatabase db = DBManager::openDatabase(QStringLiteral("main"));

    // Create DBWatcher and connect signals
    auto dbWatcher = new DBWatcher(this);
    dbWatcher->initWatcher();

    // Setup models with database
    AttachmentModel::self()->setDatabase(db);
    MailBoxModel::self()->setDatabase(db);
    MailListModel::self()->setDatabase(db);
    ThreadViewModel::self()->setDatabase(db);

    // Connect DBWatcher signals to reload models
    QObject::connect(dbWatcher, &DBWatcher::foldersChanged, MailBoxModel::self(), &MailBoxModel::load);

    // Targeted updates for specific message changes (e.g., read/unread, starred)
    QObject::connect(dbWatcher, &DBWatcher::specificMessagesChanged, MailListModel::self(), &MailListModel::updateMessages);
    QObject::connect(dbWatcher, &DBWatcher::specificMessagesChanged, ThreadViewModel::self(), &ThreadViewModel::updateMessages);

    // Smart refresh for general message/thread changes (new messages, moves, deletes)
    QObject::connect(dbWatcher, &DBWatcher::messagesChanged, MailListModel::self(), &MailListModel::smartRefresh);
    QObject::connect(dbWatcher, &DBWatcher::threadsChanged, MailListModel::self(), &MailListModel::smartRefresh);

    // Load initial data
    MailBoxModel::self()->load();
}


Raven *Raven::self()
{
    static Raven *instance = new Raven();
    return instance;
}

QString Raven::selectedFolderName() const
{
    return m_selectedFolderName;
}

void Raven::setSelectedFolderName(QString folderName)
{
    if (folderName != m_selectedFolderName) {
        m_selectedFolderName = folderName;
        Q_EMIT selectedFolderNameChanged();
    }
}

void Raven::triggerSyncForAccount(const QString &accountId)
{
    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "D-Bus interface not available for triggerSync";
        return;
    }

    QDBusPendingReply<bool> reply = iface.TriggerSync(accountId);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool> reply = *w;
        if (reply.isError()) {
            qWarning() << "TriggerSync failed:" << reply.error().message();
        } else if (!reply.value()) {
            qWarning() << "TriggerSync returned false";
        }
        w->deleteLater();
    });
}
