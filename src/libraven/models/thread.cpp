// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thread.h"
#include "../constants.h"
#include "../utils.h"

#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlError>

Thread::Thread(QObject *parent, QString accountId, QString subject, QString gmailThreadId)
    : QObject{parent}
    , m_id{QUuid::createUuid().toString(QUuid::WithoutBraces)}
    , m_accountId{accountId}
    , m_gmailThreadId{gmailThreadId}
    , m_subject{subject}
    , m_snippet{}
    , m_unread{0}
    , m_starred{0}
    , m_firstMessageTimestamp{QDateTime::fromSecsSinceEpoch(0)}
    , m_lastMessageTimestamp{QDateTime::fromSecsSinceEpoch(0)}
    , m_participants{}
    , m_folderIds{}
{
}

Thread::Thread(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toString()}
    , m_accountId{query.value(QStringLiteral("accountId")).toString()}
    , m_gmailThreadId{query.value(QStringLiteral("gmailThreadId")).toString()}
    , m_subject{query.value(QStringLiteral("subject")).toString()}
    , m_snippet{query.value(QStringLiteral("snippet")).toString()}
    , m_unread{query.value(QStringLiteral("unread")).toInt()}
    , m_starred{query.value(QStringLiteral("starred")).toInt()}
    , m_firstMessageTimestamp{query.value(QStringLiteral("firstMessageTimestamp")).toDateTime()}
    , m_lastMessageTimestamp{query.value(QStringLiteral("lastMessageTimestamp")).toDateTime()}
    , m_participants{}
    , m_folderIds{}
{
    QJsonObject object = QJsonDocument::fromJson(query.value(QStringLiteral("data")).toString().toUtf8()).object();
    
    for (auto participant : object[QStringLiteral("participants")].toArray()) {
        m_participants.push_back(new MessageContact{(QObject *) this, participant.toObject()});
    }
    for (auto folderId : object[QStringLiteral("folderIds")].toArray()) {
        m_folderIds.push_back(folderId.toString());
    }
}

void Thread::saveToDb(QSqlDatabase &db) const
{
    db.transaction();
    
    QJsonArray participants;
    for (auto contact : m_participants) {
        participants.push_back(contact->toJson());
    }
    QJsonArray folderIds;
    for (auto &folderId : m_folderIds) {
        folderIds.push_back(folderId);
    }
    
    QJsonObject object;
    object[QStringLiteral("participants")] = participants;
    object[QStringLiteral("folderIds")] = folderIds;

    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO ") + THREAD_TABLE +
        QStringLiteral(" (id, accountId, data, gmailThreadId, subject, snippet, unread, starred, firstMessageTimestamp, lastMessageTimestamp)") +
        QStringLiteral(" VALUES (:id, :accountId, :data, :gmailThreadId, :subject, :snippet, :unread, :starred, :firstMessageTimestamp, :lastMessageTimestamp)"));
    
    query.bindValue(QStringLiteral(":id"), m_id);
    query.bindValue(QStringLiteral(":accountId"), m_accountId);
    query.bindValue(QStringLiteral(":data"), QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":gmailThreadId"), m_gmailThreadId);
    query.bindValue(QStringLiteral(":subject"), m_subject);
    query.bindValue(QStringLiteral(":snippet"), m_snippet);
    query.bindValue(QStringLiteral(":unread"), m_unread);
    query.bindValue(QStringLiteral(":starred"), m_starred);
    query.bindValue(QStringLiteral(":firstMessageTimestamp"), m_firstMessageTimestamp);
    query.bindValue(QStringLiteral(":lastMessageTimestamp"), m_lastMessageTimestamp);
    Utils::execWithLog(query, "saving thread");
    
    // update thread <-> folder join table with any changes
    QStringList folderIdsToUpdate = m_folderIds;
    for (auto &folderId : m_snapshot.folderIds) {
        if (folderIdsToUpdate.indexOf(folderId) == -1) {
            
            query.prepare(QStringLiteral("DELETE FROM ") + THREAD_FOLDER_TABLE + QStringLiteral(" WHERE threadId = ? AND folderId = ?"));
            query.addBindValue(m_id);
            query.addBindValue(folderId);
            Utils::execWithLog(query, "remove thread <-> folder relationship");
            
        } else {
            folderIdsToUpdate.removeAll(folderId);
        }
    }
    
    for (auto &folderId : folderIdsToUpdate) {
        query.prepare(QStringLiteral("INSERT OR REPLACE INTO ") + THREAD_FOLDER_TABLE + 
            QStringLiteral(" (accountId, threadId, folderId) VALUES (:accountId, :threadId, :folderId)"));
        
        query.bindValue(QStringLiteral(":accountId"), m_accountId);
        query.bindValue(QStringLiteral(":threadId"), m_id);
        query.bindValue(QStringLiteral(":folderId"), folderId);
        
        Utils::execWithLog(query, "add thread <-> folder relationship");
    }
    
    db.commit();
}

void Thread::deleteFromDb(QSqlDatabase &db) const
{
    db.transaction();
    
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + THREAD_FOLDER_TABLE + QStringLiteral(" WHERE threadId = ") + m_id);
    query.exec();
    
    query.prepare(QStringLiteral("DELETE FROM ") + THREAD_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
    
    db.commit();
}

void Thread::createSnapshot()
{
    m_snapshot.folderIds = m_folderIds;
}

void Thread::updateAfterMessageChanges(const MessageSnapshot &oldMsg, Message *newMsg)
{
    // newMsg is nullptr if we are just removing oldMsg
    
    // remove oldData
    setUnread(unread() - oldMsg.unread);
    setStarred(starred() - oldMsg.starred);
    
    int folderIdIndex = m_folderIds.indexOf(oldMsg.folderId);
    if (folderIdIndex != -1) {
        m_folderIds.removeAt(folderIdIndex);
    }
    
    // add new data
    if (newMsg) {
        setUnread(unread() + newMsg->unread());
        setStarred(starred() + newMsg->starred());
        
        if (newMsg->date() > lastMessageTimestamp() || lastMessageTimestamp() == QDateTime::fromSecsSinceEpoch(0)) {
            setLastMessageTimestamp(newMsg->date());
        }
        if (newMsg->date() < firstMessageTimestamp() || firstMessageTimestamp() == QDateTime::fromSecsSinceEpoch(0)) {
            setFirstMessageTimestamp(newMsg->date());
        }
        
        QSet<QString> emails;
        for (const auto &participant : m_participants) {
            emails.insert(participant->email());
        }
        
        addMissingParticipants(emails, {newMsg->from()});
        addMissingParticipants(emails, newMsg->to());
        addMissingParticipants(emails, newMsg->cc());
        addMissingParticipants(emails, newMsg->bcc());
        
        m_folderIds.push_back(newMsg->folderId());
    }
}

QString Thread::id() const
{
    return m_id;
}

QString Thread::accountId() const
{
    return m_accountId;
}

void Thread::setAccountId(const QString &accountId)
{
    m_accountId = accountId;
}

QString Thread::gmailThreadId() const
{
    return m_gmailThreadId;
}

void Thread::setGmailThreadId(const QString &gmailThreadId)
{
    m_gmailThreadId = gmailThreadId;
}

QString Thread::subject() const
{
    return m_subject;
}

void Thread::setSubject(const QString &subject)
{
    m_subject = subject;
}

QString Thread::snippet() const
{
    return m_snippet;
}

void Thread::setSnippet(const QString &snippet)
{
    m_snippet = snippet;
}

int Thread::unread() const
{
    return m_unread;
}

void Thread::setUnread(int unread)
{
    m_unread = unread;
}

int Thread::starred() const
{
    return m_starred;
}

void Thread::setStarred(int starred)
{
    m_starred = starred;
}

QDateTime Thread::firstMessageTimestamp() const
{
    return m_firstMessageTimestamp;
}

void Thread::setFirstMessageTimestamp(const QDateTime &firstMessageTimestamp)
{
    m_firstMessageTimestamp = firstMessageTimestamp;
}

QDateTime Thread::lastMessageTimestamp() const
{
    return m_lastMessageTimestamp;
}

void Thread::setLastMessageTimestamp(const QDateTime &lastMessageTimestamp)
{
    m_lastMessageTimestamp = lastMessageTimestamp;
}

QList<MessageContact *> &Thread::participants()
{
    return m_participants;
}

QStringList &Thread::folderIds()
{
    return m_folderIds;
}

void Thread::addMissingParticipants(QSet<QString> &emails, const QList<MessageContact *> &contacts)
{
    for (const auto &participant : contacts) {
        if (emails.find(participant->email()) == emails.end()) {
            m_participants.push_back(participant);
            emails.insert(participant->email());
        }
    }
}
