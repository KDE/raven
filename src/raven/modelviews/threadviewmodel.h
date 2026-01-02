// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSqlDatabase>

#include "../models/message.h"
#include "../models/thread.h"

#include <qqmlintegration.h>

class ThreadViewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ThreadViewModel not creatable in QML")

public:
    ThreadViewModel(QObject *parent = nullptr);

    enum {
        MessageRole,
        SubjectRole,
        FromRole,
        ToRole,
        BccRole,
        CcRole,
        IsPlaintextRole,
        ContentRole,
        SnippetRole,
        UnreadRole,
        StarredRole,
        DateRole
    };

    static ThreadViewModel *self();

    // Set the database connection to use for operations
    void setDatabase(const QSqlDatabase &db);

    Q_INVOKABLE void loadThread(Thread *thread);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    // Refresh the current thread (reloads data from database)
    void refresh();

    // Update specific messages (targeted update instead of full refresh)
    // Only updates messages that are in the current thread view
    void updateMessages(const QStringList &messageIds);

private:
    QString getContactsStr(QList<MessageContact *> contacts);

    QSqlDatabase m_db;
    Thread *m_currentThread = nullptr;
    QList<Message *> m_messages;
    QStringList m_messageContents;
    QStringList m_messageTo;
    QStringList m_messageCc;
    QStringList m_messageBcc;
};

