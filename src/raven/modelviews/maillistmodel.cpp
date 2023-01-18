// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maillistmodel.h"
#include "../libraven/constants.h"
#include "../libraven/accountmodel.h"
#include "../libraven/utils.h"

#include <QDebug>
#include <QSqlError>

MailListModel::MailListModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

MailListModel *MailListModel::self()
{
    static MailListModel *instance = new MailListModel;
    return instance;
}


void MailListModel::loadFolder(Folder *folder)
{
    beginResetModel();

    for (auto thread : m_threads) {
        thread->deleteLater();
    }
    m_threads.clear();
    m_threadFrom.clear();

    QSqlDatabase db = QSqlDatabase::database();
    
    QSqlQuery query{db};
    
    query.prepare(QStringLiteral("SELECT * FROM thread INNER JOIN thread_folder ON thread_folder.threadId=thread.id AND thread_folder.accountId = ? AND thread_folder.folderId = ? LIMIT 100"));
    query.addBindValue(folder->accountId());
    query.addBindValue(folder->id());
    
    if (!Utils::execWithLog(query, "fetching mail list:")) {
        qDebug() << query.executedQuery();
        endResetModel();
        return;
    }

    auto currentDate = QDateTime::currentDateTime();
    
    // loop over folders fetched from db
    while (query.next()) {
        auto thread = new Thread(this, query);
        m_threads.push_back(thread);
        m_threadFrom.push_back(getThreadFrom(thread));
        
        // calculate date display
        auto date = thread->lastMessageTimestamp();
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

