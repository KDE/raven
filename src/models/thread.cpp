// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thread.h"
#include "constants.h"

#include <QVariant>
#include <QJsonObject>

Thread::Thread(QObject *parent, QString accountId, QString subject, QString gmailThreadId)
    : QObject{parent}
    , m_id{QUuid::createUuid().toString(QUuid::WithoutBraces)}
    , m_accountId{accountId}
    , m_gmailThreadId{gmailThreadId}
    , m_subject{subject}
    , m_snippet{}
    , m_unread{0}
{
}

Thread::Thread(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toString()}
    , m_accountId{query.value(QStringLiteral("accountId")).toString()}
    , m_gmailThreadId{query.value(QStringLiteral("gmailThreadId")).toString()}
    , m_subject{query.value(QStringLiteral("subject")).toString()}
    , m_snippet{}
    , m_unread{0}
{
    QJsonObject object = query.value(QStringLiteral("data")).toJsonObject();
    m_unread = object[QStringLiteral("unread")].toInt();
    m_snippet = object[QStringLiteral("snippet")].toString();
}

void Thread::saveToDb(QSqlDatabase &db) const
{
    QJsonObject object;
    object[QStringLiteral("unread")] = m_unread;
    object[QStringLiteral("snippet")] = m_snippet;

    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT INTO ") + THREAD_TABLE +
        QStringLiteral(" (id, accountId, data, gmailThreadId, subject, snippet)") +
        QStringLiteral(" VALUES (:id, :accountId, :data, :gmailThreadId, :subject, :snippet)"));

    query.bindValue(QStringLiteral(":id"), m_id);
    query.bindValue(QStringLiteral(":accountId"), m_accountId);
    query.bindValue(QStringLiteral(":data"), object);
    query.bindValue(QStringLiteral(":gmailThreadId"), m_gmailThreadId);
    query.bindValue(QStringLiteral(":subject"), m_subject);
    query.exec();
}

QString Thread::id() const
{
    return m_id;
}

void Thread::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + THREAD_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
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

int Thread::unread()
{
    return m_unread;
}

void Thread::setUnread(int unread)
{
    m_unread = unread;
}
