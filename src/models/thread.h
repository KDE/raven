// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <QSqlQuery>

class Thread : public QObject
{
    Q_OBJECT

public:
    Thread(QObject *parent = nullptr, QString accountId = {}, QString subject = {}, QString gmailThreadId = {});
    Thread(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;

    QString id() const;

    QString accountId() const;
    void setAccountId(const QString &accountId);

    QString gmailThreadId() const;
    void setGmailThreadId(const QString &gmailThreadId);

    QString subject() const;
    void setSubject(const QString &subject);

    QString snippet() const;
    void setSnippet(const QString &snippet);

    int unread();
    void setUnread(int unread);

private:
    QString m_id;
    QString m_accountId;
    QString m_gmailThreadId;

    QString m_subject;
    QString m_snippet;
    int m_unread;
};
