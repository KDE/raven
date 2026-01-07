// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

#include "message.h"

struct ThreadSnapshot
{
    QStringList folderIds;
};

class Thread : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Thread not creatable in QML")

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString accountId READ accountId CONSTANT)
    Q_PROPERTY(QString subject READ subject CONSTANT)
    Q_PROPERTY(QString snippet READ snippet CONSTANT)
    Q_PROPERTY(int unread READ unread CONSTANT)
    Q_PROPERTY(int starred READ starred CONSTANT)
    Q_PROPERTY(QDateTime firstMessageDate READ firstMessageTimestamp CONSTANT)
    Q_PROPERTY(QDateTime lastMessageDate READ lastMessageTimestamp CONSTANT)
    Q_PROPERTY(QList<MessageContact *> participants READ participants CONSTANT)

public:
    Thread(QObject *parent = nullptr, QString accountId = {}, QString subject = {});
    Thread(QObject *parent, const QSqlQuery &query);

    // Static fetch methods
    static QList<Thread*> fetchByFolder(QSqlDatabase &db, const QString &folderId, const QString &accountId, QObject *parent = nullptr, int limit = 100);
    static Thread* fetchById(QSqlDatabase &db, const QString &id, QObject *parent = nullptr);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;

    void createSnapshot();

    void updateAfterMessageChanges(const MessageSnapshot &oldMsg, Message *newMsg);

    QString id() const;

    QString accountId() const;
    void setAccountId(const QString &accountId);

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

    QString m_subject;
    QString m_snippet;
    int m_unread;
    int m_starred;

    QDateTime m_firstMessageTimestamp;
    QDateTime m_lastMessageTimestamp;

    QList<MessageContact *> m_participants;
    QStringList m_folderIds;

    ThreadSnapshot m_snapshot;
};
