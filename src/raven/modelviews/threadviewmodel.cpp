// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "threadviewmodel.h"
#include "../libraven/constants.h"
#include "../libraven/accountmodel.h"
#include "../libraven/utils.h"

#include <QDebug>
#include <QSqlError>

ThreadViewModel::ThreadViewModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

ThreadViewModel *ThreadViewModel::self()
{
    static ThreadViewModel *instance = new ThreadViewModel;
    return instance;
}

void ThreadViewModel::loadThread(Thread *thread)
{
    beginResetModel();

    for (auto message : m_messages) {
        message->deleteLater();
    }
    m_messages.clear();
    m_messageContents.clear();

    QSqlDatabase db = QSqlDatabase::database();
    
    QSqlQuery query{db};
    
    query.prepare(QStringLiteral("SELECT * FROM message LEFT JOIN message_body ON message.id = message_body.id WHERE message.threadId = ? AND message.accountId = ?"));
    query.addBindValue(thread->id());
    query.addBindValue(thread->accountId());
    
    if (!Utils::execWithLog(query, "fetching thread message list:")) {
        qDebug() << query.executedQuery();
        endResetModel();
        return;
    }

    // loop over folders fetched from db
    while (query.next()) {
        auto message = new Message(this, query);
        m_messages.push_back(message);
        QString content = query.value(QStringLiteral("message_body.value")).toString();
        m_messageContents.push_back(content);
        
        m_messageTo.push_back(getContactsStr(message->to()));
        m_messageCc.push_back(getContactsStr(message->cc()));
        m_messageBcc.push_back(getContactsStr(message->bcc()));
    }

    endResetModel();
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
    return {{SubjectRole, "subject"}, {FromRole, "from"}, {ToRole, "to"}, {CcRole, "cc"}, {BccRole, "bcc"}, {IsPlaintextRole, "isPlaintext"}, {ContentRole, "content"}, {SnippetRole, "snippet"}, {UnreadRole, "unread"}, {DateRole, "date"}};
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
