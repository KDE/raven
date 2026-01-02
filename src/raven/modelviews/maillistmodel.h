// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QHash>
#include <QList>
#include <QSqlDatabase>

#include "../models/folder.h"
#include "../models/thread.h"

#include <qqmlintegration.h>

class MailListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MailListModel not creatable in QML")

    Q_PROPERTY(Folder* currentFolder READ currentFolder NOTIFY currentFolderChanged)

public:
    MailListModel(QObject *parent = nullptr);

    enum {
        ThreadRole,
        FromRole,
        SubjectRole,
        SnippetRole,
        UnreadRole,
        StarredRole,
        DateRole
    };

    static MailListModel *self();

    // Set the database connection to use for operations
    void setDatabase(const QSqlDatabase &db);

    Q_INVOKABLE void loadFolder(Folder *folder);

    // Message action methods (non-blocking D-Bus calls)
    Q_INVOKABLE void markAsRead(const QStringList &messageIds);
    Q_INVOKABLE void markAsUnread(const QStringList &messageIds);
    Q_INVOKABLE void setFlagged(const QStringList &messageIds, bool flagged);
    Q_INVOKABLE void moveToTrash(const QStringList &messageIds);

    // Thread-level action methods (convenience wrappers that operate on all messages in a thread)
    Q_INVOKABLE void markThreadAsRead(Thread *thread);
    Q_INVOKABLE void markThreadAsUnread(Thread *thread);
    Q_INVOKABLE void setThreadFlagged(Thread *thread, bool flagged);
    Q_INVOKABLE void moveThreadToTrash(Thread *thread);

    Folder* currentFolder() const;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void currentFolderChanged();

public Q_SLOTS:
    // Refresh the current folder (reloads data from database)
    void refresh();

    // Smart refresh - compares database with current model and emits
    // proper beginInsertRows/beginRemoveRows for new/deleted items
    void smartRefresh();

    // Update specific messages (targeted update instead of full refresh)
    // Looks up which threads contain these messages and updates only those rows
    void updateMessages(const QStringList &messageIds);

private:
    // Remove a thread from the model by its ID (for optimistic updates)
    void removeThreadById(const QString &threadId);
    QString getThreadFrom(Thread *thread);
    QStringList getMessageIdsForThread(Thread *thread);

    QSqlDatabase m_db;
    Folder *m_currentFolder = nullptr;
    QList<Thread *> m_threads;
    QStringList m_threadDate;
    QStringList m_threadFrom;
};
