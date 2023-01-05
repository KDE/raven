// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>
#include <QDateTime>

#include <MailCore/MailCore.h>

#include "models/folder.h"
#include "models/file.h"
#include "messagecontact.h"

class Message : public QObject
{
    Q_OBJECT

public:
    Message(QObject *parent = nullptr);
    Message(QObject *parent, mailcore::IMAPMessage *msg, const Folder &folder, time_t syncTimestamp);
    Message(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;

    QString id() const;

    QString folderId() const;
    void setFolderId(const QString &folderId);

    QString accountId() const;
    void setAccountId(const QString &accountId);

    QString threadId() const;
    void setThreadId(const QString &threadId);

    QList<MessageContact *> &to();
    QList<MessageContact *> &cc();
    QList<MessageContact *> &bcc();
    QList<MessageContact *> &replyTo();
    MessageContact *from() const;

    QString headerMessageId() const;

    QString subject() const;
    QDateTime date() const;

    QString remoteUid() const;
    void setRemoteUid(const QString &remoteUid);

    QString snippet() const;
    void setSnippet(const QString &snippet);

    bool plaintext() const;
    void setPlaintext(bool plaintext);

    QList<std::shared_ptr<File>> &files();
    void setFiles(QList<std::shared_ptr<File>> files);

private:
    QString m_id;
    QString m_folderId; // TODO not implemented
    QString m_accountId;
    QString m_threadId;

    QList<MessageContact *> m_to;
    QList<MessageContact *> m_cc;
    QList<MessageContact *> m_bcc;
    QList<MessageContact *> m_replyTo;
    MessageContact *m_from;

    QString m_headerMessageId;
    QString m_gmailMessageId;
    QString m_gmailThreadId;
    QString m_subject;
    bool m_draft;
    bool m_unread;
    bool m_starred;

    QDateTime m_date;
    QDateTime m_syncedAt;

    QString m_remoteUid;
    QStringList m_labels; // only for GMail

    QString m_snippet;
    bool m_plaintext;

    QList<std::shared_ptr<File>> m_files;
};