// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <QSqlQuery>

class Thread : public QObject
{
    Q_OBJECT

public:
    Thread(QObject *parent = nullptr, QString messageId = {}, QString accountId = {}, QString subject = {}, QString gmailThreadId = {});
    Thread(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;

    QString id() const;

    QString messageId() const;
    void setMessageId(const QString &messageId);

    QString accountId() const;
    void setAccountId(const QString &accountId);

    QString gmailThreadId() const;
    void setGmailThreadId(const QString &gmailThreadId);

    QString subject() const;
    void setSubject(const QString &subject);

    int unread();
    void setUnread(int unread);

private:
    QString m_id;
    QString m_messageId;
    QString m_accountId;
    QString m_gmailThreadId;

    QString m_subject;
    int m_unread;
};
