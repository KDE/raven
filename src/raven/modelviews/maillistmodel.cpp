// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maillistmodel.h"
#include "constants.h"
#include "accountmodel.h"
#include "utils.h"
#include "../models/message.h"
#include "ravendaemoninterface.h"

#include <QDBusConnection>
#include <QSqlQuery>
#include <QDBusPendingReply>
#include <algorithm>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlError>

MailListModel::MailListModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

void MailListModel::refresh()
{
    if (m_currentFolder) {
        qDebug() << "MailListModel: Refreshing current folder";
        loadFolder(m_currentFolder);
    }
}

void MailListModel::updateMessages(const QStringList &messageIds)
{
    if (messageIds.isEmpty() || !m_db.isOpen() || m_threads.isEmpty()) {
        return;
    }

    qDebug() << "MailListModel: Targeted update for" << messageIds.size() << "messages";

    // Build a query to find which threads contain these messages
    // We need to get distinct thread IDs for the given message IDs
    QString placeholders;
    for (int i = 0; i < messageIds.size(); ++i) {
        if (i > 0) placeholders += QStringLiteral(", ");
        placeholders += QStringLiteral("?");
    }

    QSqlQuery query{m_db};
    query.prepare(QStringLiteral("SELECT DISTINCT threadId FROM message WHERE id IN (") + placeholders + QStringLiteral(")"));
    for (const auto &id : messageIds) {
        query.addBindValue(id);
    }

    if (!Utils::execWithLog(query, "finding threads for message IDs")) {
        return;
    }

    // Collect the thread IDs that need updating
    QSet<QString> affectedThreadIds;
    while (query.next()) {
        affectedThreadIds.insert(query.value(0).toString());
    }

    if (affectedThreadIds.isEmpty()) {
        return;
    }

    qDebug() << "MailListModel: Found" << affectedThreadIds.size() << "affected threads";

    // Find which rows in our model correspond to these threads and update them
    for (int row = 0; row < m_threads.size(); ++row) {
        Thread *thread = m_threads[row];
        if (affectedThreadIds.contains(thread->id())) {
            // Reload this thread from the database
            Thread *updatedThread = Thread::fetchById(m_db, thread->id(), this);
            if (updatedThread) {
                // Update the thread in our list
                m_threads[row]->deleteLater();
                m_threads[row] = updatedThread;

                // Update the cached "from" string
                m_threadFrom[row] = getThreadFrom(updatedThread);

                // Emit dataChanged for this specific row
                QModelIndex idx = index(row, 0);
                Q_EMIT dataChanged(idx, idx, {UnreadRole, StarredRole});

                qDebug() << "MailListModel: Updated thread at row" << row << "(" << updatedThread->subject() << ")";
            }
        }
    }
}

void MailListModel::smartRefresh()
{
    if (!m_currentFolder || !m_db.isOpen()) {
        return;
    }

    qDebug() << "MailListModel: Smart refresh for folder" << m_currentFolder->path();

    // Fetch current threads from database
    auto dbThreads = Thread::fetchByFolder(m_db, m_currentFolder->id(), m_currentFolder->accountId(), this, 100);

    // Build sets for comparison
    QSet<QString> modelThreadIds;
    for (const auto *thread : m_threads) {
        modelThreadIds.insert(thread->id());
    }

    QSet<QString> dbThreadIds;
    QMap<QString, Thread*> dbThreadMap;
    for (auto *thread : dbThreads) {
        dbThreadIds.insert(thread->id());
        dbThreadMap.insert(thread->id(), thread);
    }

    // Find threads to remove (in model but not in DB)
    QSet<QString> toRemove = modelThreadIds - dbThreadIds;

    // Find threads to add (in DB but not in model)
    QSet<QString> toAdd = dbThreadIds - modelThreadIds;

    // Remove threads that are no longer in the folder
    // Process in reverse order to maintain valid indices
    for (int row = m_threads.size() - 1; row >= 0; --row) {
        if (toRemove.contains(m_threads[row]->id())) {
            beginRemoveRows(QModelIndex(), row, row);
            m_threads[row]->deleteLater();
            m_threads.removeAt(row);
            m_threadFrom.removeAt(row);
            m_threadDate.removeAt(row);
            endRemoveRows();
            qDebug() << "MailListModel: Removed thread at row" << row;
        }
    }

    // Add new threads at the beginning (newest first)
    // Collect new threads sorted by timestamp
    QList<Thread*> newThreads;
    for (const QString &threadId : toAdd) {
        newThreads.append(dbThreadMap.take(threadId));
    }

    // Sort by lastMessageTimestamp descending
    std::sort(newThreads.begin(), newThreads.end(), [](Thread *a, Thread *b) {
        return a->lastMessageTimestamp() > b->lastMessageTimestamp();
    });

    // Insert new threads at appropriate positions based on timestamp
    auto currentDate = QDateTime::currentDateTime();
    for (Thread *newThread : newThreads) {
        // Find insertion position to maintain sort order
        int insertPos = 0;
        for (; insertPos < m_threads.size(); ++insertPos) {
            if (newThread->lastMessageTimestamp() > m_threads[insertPos]->lastMessageTimestamp()) {
                break;
            }
        }

        beginInsertRows(QModelIndex(), insertPos, insertPos);
        m_threads.insert(insertPos, newThread);
        m_threadFrom.insert(insertPos, getThreadFrom(newThread));

        // Calculate date display
        auto date = newThread->lastMessageTimestamp().toLocalTime();
        QString dateStr;
        if (date.daysTo(currentDate) == 0) {
            dateStr = date.toString(QStringLiteral("h:mm ap"));
        } else if (date.daysTo(currentDate) < 7) {
            dateStr = date.toString(QStringLiteral("ddd h:mm ap"));
        } else if (date.date().year() == currentDate.date().year()) {
            dateStr = date.toString(QStringLiteral("ddd MMM dd"));
        } else {
            dateStr = date.toString(QStringLiteral("MMM dd, yyyy"));
        }
        m_threadDate.insert(insertPos, dateStr);
        endInsertRows();

        qDebug() << "MailListModel: Inserted new thread at row" << insertPos << "(" << newThread->subject() << ")";
    }

    // Clean up threads we didn't use
    for (auto *thread : dbThreadMap) {
        thread->deleteLater();
    }

    qDebug() << "MailListModel: Smart refresh complete - removed" << toRemove.size() << ", added" << toAdd.size();
}

MailListModel *MailListModel::self()
{
    static MailListModel *instance = new MailListModel;
    return instance;
}

void MailListModel::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
}

void MailListModel::loadFolder(Folder *folder)
{
    beginResetModel();

    for (auto thread : m_threads) {
        thread->deleteLater();
    }
    m_threads.clear();
    m_threadFrom.clear();
    m_threadDate.clear();

    // Store the current folder
    m_currentFolder = folder;
    Q_EMIT currentFolderChanged();

    if (!m_db.isOpen()) {
        qWarning() << "MailListModel::loadFolder() - Database not open";
        endResetModel();
        return;
    }

    m_threads = Thread::fetchByFolder(m_db, folder->id(), folder->accountId(), this, 100);

    auto currentDate = QDateTime::currentDateTime();

    for (auto thread : m_threads) {
        m_threadFrom.push_back(getThreadFrom(thread));

        // calculate date display
        // Convert to local time for proper display (timestamps are stored as UTC)
        auto date = thread->lastMessageTimestamp().toLocalTime();
        if (date.daysTo(currentDate) == 0) {
            // today, just display time
            m_threadDate.push_back(date.toString(QStringLiteral("h:mm ap")));
        } else if (date.daysTo(currentDate) < 7) {
            // this week
            m_threadDate.push_back(date.toString(QStringLiteral("ddd h:mm ap")));
        } else if (date.date().year() == currentDate.date().year()) {
            // this year
            m_threadDate.push_back(date.toString(QStringLiteral("ddd MMM dd")));
        } else {
            // previous years
            m_threadDate.push_back(date.toString(QStringLiteral("MMM dd, yyyy")));
        }
    }

    endResetModel();
}

void MailListModel::markAsRead(const QStringList &messageIds)
{
    if (messageIds.isEmpty()) {
        return;
    }

    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "D-Bus interface not available for markAsRead";
        return;
    }

    QDBusPendingReply<QString> reply = iface.MarkAsRead(messageIds);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> reply = *w;
        if (reply.isError()) {
            qWarning() << "MarkAsRead failed:" << reply.error().message();
        } else {
            auto result = QJsonDocument::fromJson(reply.value().toUtf8()).object();
            auto failed = result[QStringLiteral("failed")].toArray();
            if (!failed.isEmpty()) {
                qWarning() << "MarkAsRead partial failure:" << failed;
            }
        }
        w->deleteLater();
    });
}

void MailListModel::markAsUnread(const QStringList &messageIds)
{
    if (messageIds.isEmpty()) {
        return;
    }

    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "D-Bus interface not available for markAsUnread";
        return;
    }

    QDBusPendingReply<QString> reply = iface.MarkAsUnread(messageIds);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> reply = *w;
        if (reply.isError()) {
            qWarning() << "MarkAsUnread failed:" << reply.error().message();
        } else {
            auto result = QJsonDocument::fromJson(reply.value().toUtf8()).object();
            auto failed = result[QStringLiteral("failed")].toArray();
            if (!failed.isEmpty()) {
                qWarning() << "MarkAsUnread partial failure:" << failed;
            }
        }
        w->deleteLater();
    });
}

void MailListModel::setFlagged(const QStringList &messageIds, bool flagged)
{
    if (messageIds.isEmpty()) {
        return;
    }

    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "D-Bus interface not available for setFlagged";
        return;
    }

    QDBusPendingReply<QString> reply = iface.SetFlagged(messageIds, flagged);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> reply = *w;
        if (reply.isError()) {
            qWarning() << "SetFlagged failed:" << reply.error().message();
        } else {
            auto result = QJsonDocument::fromJson(reply.value().toUtf8()).object();
            auto failed = result[QStringLiteral("failed")].toArray();
            if (!failed.isEmpty()) {
                qWarning() << "SetFlagged partial failure:" << failed;
            }
        }
        w->deleteLater();
    });
}

void MailListModel::moveToTrash(const QStringList &messageIds)
{
    if (messageIds.isEmpty()) {
        return;
    }

    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "D-Bus interface not available for moveToTrash";
        return;
    }

    QDBusPendingReply<QString> reply = iface.MoveToTrash(messageIds);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QString> reply = *w;
        if (reply.isError()) {
            qWarning() << "MoveToTrash failed:" << reply.error().message();
        } else {
            auto result = QJsonDocument::fromJson(reply.value().toUtf8()).object();
            auto failed = result[QStringLiteral("failed")].toArray();
            if (!failed.isEmpty()) {
                qWarning() << "MoveToTrash partial failure:" << failed;
            }
        }
        w->deleteLater();
    });
}

void MailListModel::markThreadAsRead(Thread *thread)
{
    if (!thread) {
        return;
    }
    auto messageIds = getMessageIdsForThread(thread);
    markAsRead(messageIds);
}

void MailListModel::markThreadAsUnread(Thread *thread)
{
    if (!thread) {
        return;
    }
    auto messageIds = getMessageIdsForThread(thread);
    markAsUnread(messageIds);
}

void MailListModel::setThreadFlagged(Thread *thread, bool flagged)
{
    if (!thread) {
        return;
    }
    auto messageIds = getMessageIdsForThread(thread);
    setFlagged(messageIds, flagged);
}

void MailListModel::moveThreadToTrash(Thread *thread)
{
    if (!thread) {
        return;
    }
    auto messageIds = getMessageIdsForThread(thread);

    // Send the D-Bus request - smartRefresh will handle removal when
    // the daemon confirms the move via TableChanged signal
    moveToTrash(messageIds);
}

void MailListModel::removeThreadById(const QString &threadId)
{
    for (int row = 0; row < m_threads.size(); ++row) {
        if (m_threads[row]->id() == threadId) {
            beginRemoveRows(QModelIndex(), row, row);
            m_threads[row]->deleteLater();
            m_threads.removeAt(row);
            m_threadFrom.removeAt(row);
            m_threadDate.removeAt(row);
            endRemoveRows();
            qDebug() << "MailListModel: Optimistically removed thread at row" << row;
            break;
        }
    }
}

Folder* MailListModel::currentFolder() const
{
    return m_currentFolder;
}

QStringList MailListModel::getMessageIdsForThread(Thread *thread)
{
    QStringList messageIds;
    if (!thread || !m_db.isOpen()) {
        return messageIds;
    }

    auto messages = Message::fetchByThread(m_db, thread->id(), thread->accountId());
    for (auto *msg : messages) {
        messageIds.append(msg->id());
        msg->deleteLater();
    }
    return messageIds;
}

int MailListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_threads.count();
}

QVariant MailListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_threads.count()) {
        return QVariant{};
    }

    auto thread = m_threads[index.row()];
    switch (role) {
        case ThreadRole:
            return QVariant::fromValue(thread);
        case FromRole:
            return m_threadFrom[index.row()];
        case SubjectRole:
            return thread->subject();
        case SnippetRole:
            return thread->snippet();
        case UnreadRole:
            return thread->unread() != 0;
        case StarredRole:
            return thread->starred() != 0;
        case DateRole:
            return m_threadDate[index.row()];
    }
    return {};
}

Qt::ItemFlags MailListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> MailListModel::roleNames() const
{
    return {{ThreadRole, "thread"}, {FromRole, "from"}, {SubjectRole, "subject"}, {SnippetRole, "snippet"}, {UnreadRole, "unread"}, {StarredRole, "starred"}, {DateRole, "date"}};
}

QString MailListModel::getThreadFrom(Thread *thread)
{
    QString from;
    auto account = AccountModel::self()->accountById(thread->accountId());

    // return every participant except ourself
    bool firstAdded = false;
    for (int i = 0; i < thread->participants().count(); ++i) {
        auto participant = thread->participants()[i];

        if (participant->email().toLower() != account->email().toLower()) {
            if (firstAdded) {
                from += QStringLiteral(", ");
            }
            from += QString{QStringLiteral("%1 <%2>")}.arg(participant->name(), participant->email());
            firstAdded = true;
        }
    }
    return from;
}
