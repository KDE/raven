// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "threadviewmodel.h"
#include "constants.h"
#include "accountmodel.h"
#include "utils.h"
#include "dbmanager.h"

#include <QDebug>
#include <QSqlError>

ThreadViewModel::ThreadViewModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_db{DBManager::openDatabase(QStringLiteral("threadViewModel"))}
{
}

ThreadViewModel *ThreadViewModel::self()
{
    static ThreadViewModel *instance = new ThreadViewModel;
    return instance;
}

void ThreadViewModel::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
}

void ThreadViewModel::loadThread(Thread *thread)
{
    beginResetModel();

    for (auto message : m_messages) {
        message->deleteLater();
    }
    m_messages.clear();
    m_messageContents.clear();
    m_messageTo.clear();
    m_messageCc.clear();
    m_messageBcc.clear();

    // Store current thread for refresh
    m_currentThread = thread;

    if (!thread) {
        qWarning() << "ThreadViewModel::loadThread() - thread is null";
        endResetModel();
        return;
    }

    if (!m_db.isOpen()) {
        qWarning() << "ThreadViewModel::loadThread() - Database not open";
        endResetModel();
        return;
    }

    auto messagesWithBody = Message::fetchByThreadWithBody(m_db, thread->id(), thread->accountId(), this);

    for (const auto &mwb : messagesWithBody) {
        m_messages.push_back(mwb.message);
        m_messageContents.push_back(mwb.bodyContent);

        m_messageTo.push_back(getContactsStr(mwb.message->to()));
        m_messageCc.push_back(getContactsStr(mwb.message->cc()));
        m_messageBcc.push_back(getContactsStr(mwb.message->bcc()));
    }

    endResetModel();
}

void ThreadViewModel::refresh()
{
    if (m_currentThread) {
        qDebug() << "ThreadViewModel: Refreshing current thread";
        loadThread(m_currentThread);
    }
}

void ThreadViewModel::updateMessages(const QStringList &messageIds)
{
    if (messageIds.isEmpty() || !m_db.isOpen() || m_messages.isEmpty()) {
        return;
    }

    // Convert to set for faster lookup
    QSet<QString> messageIdSet(messageIds.begin(), messageIds.end());

    qDebug() << "ThreadViewModel: Checking" << messageIds.size() << "messages for updates";

    // Check each message in our current view
    for (int row = 0; row < m_messages.size(); ++row) {
        Message *currentMsg = m_messages[row];
        if (messageIdSet.contains(currentMsg->id())) {
            // This message was updated, reload it from the database
            Message *updatedMsg = Message::fetchById(m_db, currentMsg->id(), this);
            if (updatedMsg) {
                // Replace the message in our list
                m_messages[row]->deleteLater();
                m_messages[row] = updatedMsg;

                // Update cached contact strings
                m_messageTo[row] = getContactsStr(updatedMsg->to());
                m_messageCc[row] = getContactsStr(updatedMsg->cc());
                m_messageBcc[row] = getContactsStr(updatedMsg->bcc());

                // Note: We don't update m_messageContents as body content rarely changes
                // and fetching it would require a separate query

                // Emit dataChanged for this specific row
                QModelIndex idx = index(row, 0);
                Q_EMIT dataChanged(idx, idx, {UnreadRole, StarredRole});

                qDebug() << "ThreadViewModel: Updated message at row" << row << "(" << updatedMsg->subject() << ")";
            }
        }
    }
}

int ThreadViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_messages.count();
}

QVariant ThreadViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.count()) {
        return QVariant{};
    }

    auto message = m_messages[index.row()];
    switch (role) {
        case MessageRole:
            return QVariant::fromValue(message);
        case SubjectRole:
            return message->subject();
        case FromRole:
            return QStringLiteral("%1 <%2>").arg(message->from()->name(), message->from()->email());
        case ToRole:
            return m_messageTo[index.row()];
        case CcRole:
            return m_messageCc[index.row()];
        case BccRole:
            return m_messageBcc[index.row()];
        case IsPlaintextRole:
            return message->plaintext();
        case ContentRole:
            return m_messageContents[index.row()];
        case SnippetRole:
            return message->snippet();
        case UnreadRole:
            return message->unread();
        case DateRole:
            return message->date();
    }
    return {};
}

Qt::ItemFlags ThreadViewModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> ThreadViewModel::roleNames() const
{
    return {{MessageRole, "message"}, {SubjectRole, "subject"}, {FromRole, "from"}, {ToRole, "to"}, {CcRole, "cc"}, {BccRole, "bcc"}, {IsPlaintextRole, "isPlaintext"}, {ContentRole, "content"}, {SnippetRole, "snippet"}, {UnreadRole, "unread"}, {DateRole, "date"}};
}

QString ThreadViewModel::getContactsStr(QList<MessageContact *> contacts)
{
    QString ret;
    for (int i = 0; i < contacts.count(); ++i) {
        auto contact = contacts[i];

        if (i != 0) {
            ret += QStringLiteral(", ");
        }
        ret += QString{QStringLiteral("%1 <%2>")}.arg(contact->name(), contact->email());
    }
    return ret;
}
