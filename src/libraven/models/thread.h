// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QDateTime>

#include "message.h"

class Thread : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString subject READ subject CONSTANT)
    Q_PROPERTY(QString snippet READ snippet CONSTANT)
    Q_PROPERTY(int unread READ unread CONSTANT)
    Q_PROPERTY(int starred READ starred CONSTANT)
    Q_PROPERTY(QList<MessageContact *> participants READ participants CONSTANT)

public:
    Thread(QObject *parent = nullptr, QString accountId = {}, QString subject = {}, QString gmailThreadId = {});
    Thread(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;
    
    void updateAfterMessageChanges(const MessageSnapshot &oldMsg, Message *newMsg); 

    QString id() const;

    QString accountId() const;
    void setAccountId(const QString &accountId);

    QString gmailThreadId() const;
    void setGmailThreadId(const QString &gmailThreadId);

    QString subject() const;
    void setSubject(const QString &subject);

    QString snippet() const;
    void setSnippet(const QString &snippet);

    int unread() const;
    void setUnread(int unread);
    
    int starred() const;
    void setStarred(int starred);
    
    QDateTime firstMessageTimestamp() const;
    void setFirstMessageTimestamp(const QDateTime &firstMessageTimestamp);
    
    QDateTime lastMessageTimestamp() const;
    void setLastMessageTimestamp(const QDateTime &lastMessageTimestamp);
    
    QList<MessageContact *> &participants();
    
    QStringList &folderIds();

private:
    void addMissingParticipants(QSet<QString> &emails, const QList<MessageContact *> &contacts);
                                
    QString m_id;
    QString m_accountId;
    QString m_gmailThreadId;

    QString m_subject;
    QString m_snippet;
    int m_unread;
    int m_starred;
    
    QDateTime m_firstMessageTimestamp;
    QDateTime m_lastMessageTimestamp;
    
    QList<MessageContact *> m_participants;
    QStringList m_folderIds;
};
