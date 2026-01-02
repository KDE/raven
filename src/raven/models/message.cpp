// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "message.h"
#include "thread.h"
#include "../utils.h"
#include "../constants.h"

#include <QUuid>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

Message::Message(QObject *parent)
    : QObject{parent}
{
}

Message::Message(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toString()}
    , m_folderId{query.value(QStringLiteral("folderId")).toString()}
    , m_accountId{query.value(QStringLiteral("accountId")).toString()}
    , m_threadId{query.value(QStringLiteral("threadId")).toString()}
    , m_to{}
    , m_cc{}
    , m_bcc{}
    , m_replyTo{}
    , m_from{}
    , m_headerMessageId{query.value(QStringLiteral("headerMessageId")).toString()}
    , m_gmailMessageId{query.value(QStringLiteral("gmailMessageId")).toString()}
    , m_gmailThreadId{query.value(QStringLiteral("gmailThreadId")).toString()}
    , m_subject{query.value(QStringLiteral("subject")).toString()}
    , m_draft{query.value(QStringLiteral("draft")).toBool()}
    , m_unread{query.value(QStringLiteral("unread")).toBool()}
    , m_starred{query.value(QStringLiteral("starred")).toBool()}
    , m_date{query.value(QStringLiteral("date")).toDateTime()}
    , m_syncedAt{}
    , m_remoteUid{query.value(QStringLiteral("remoteUID")).toString()}
    , m_labels{}
    , m_snippet{}
    , m_plaintext{}
{
    QJsonObject json = QJsonDocument::fromJson(query.value(QStringLiteral("data")).toString().toUtf8()).object();

    m_syncedAt = QDateTime::fromSecsSinceEpoch(json[QStringLiteral("syncedAt")].toDouble());
    m_from = new MessageContact{(QObject *) this, json[QStringLiteral("from")].toObject()};
    m_labels = json[QStringLiteral("labels")].toVariant().toStringList();

    for (auto contact : json[QStringLiteral("to")].toArray()) {
        m_to.push_back(new MessageContact{(QObject *) this, contact.toObject()});
    }
    for (auto contact : json[QStringLiteral("cc")].toArray()) {
        m_cc.push_back(new MessageContact{(QObject *) this, contact.toObject()});
    }
    for (auto contact : json[QStringLiteral("bcc")].toArray()) {
        m_bcc.push_back(new MessageContact{(QObject *) this, contact.toObject()});
    }
    for (auto contact : json[QStringLiteral("replyTo")].toArray()) {
        m_replyTo.push_back(new MessageContact{(QObject *) this, contact.toObject()});
    }

    m_snippet = json[QStringLiteral("snippet")].toString();
    m_plaintext = json[QStringLiteral("plaintext")].toBool();

    createSnapshot();
}

QList<Message*> Message::fetchByThread(QSqlDatabase &db, const QString &threadId, const QString &accountId, QObject *parent)
{
    QList<Message*> messages;
    QSqlQuery query{db};

    query.prepare(QStringLiteral("SELECT * FROM ") + MESSAGE_TABLE +
        QStringLiteral(" WHERE threadId = :threadId AND accountId = :accountId"));
    query.bindValue(QStringLiteral(":threadId"), threadId);
    query.bindValue(QStringLiteral(":accountId"), accountId);

    if (!Utils::execWithLog(query, "fetching messages by thread")) {
        return messages;
    }

    while (query.next()) {
        messages.push_back(new Message(parent, query));
    }
    return messages;
}

QList<MessageWithBody> Message::fetchByThreadWithBody(QSqlDatabase &db, const QString &threadId, const QString &accountId, QObject *parent)
{
    QList<MessageWithBody> results;
    QSqlQuery query{db};

    // Use GROUP BY to deduplicate messages that exist in multiple folders (e.g., Gmail labels)
    // COALESCE handles cases where headerMessageId might be null
    query.prepare(QStringLiteral("SELECT * FROM ") + MESSAGE_TABLE +
        QStringLiteral(" LEFT JOIN ") + MESSAGE_BODY_TABLE +
        QStringLiteral(" ON ") + MESSAGE_TABLE + QStringLiteral(".id = ") + MESSAGE_BODY_TABLE + QStringLiteral(".id") +
        QStringLiteral(" WHERE ") + MESSAGE_TABLE + QStringLiteral(".threadId = :threadId") +
        QStringLiteral(" AND ") + MESSAGE_TABLE + QStringLiteral(".accountId = :accountId") +
        QStringLiteral(" GROUP BY COALESCE(") + MESSAGE_TABLE + QStringLiteral(".headerMessageId, ") + MESSAGE_TABLE + QStringLiteral(".id)") +
        QStringLiteral(" ORDER BY ") + MESSAGE_TABLE + QStringLiteral(".date ASC"));
    query.bindValue(QStringLiteral(":threadId"), threadId);
    query.bindValue(QStringLiteral(":accountId"), accountId);

    if (!Utils::execWithLog(query, "fetching messages by thread with body")) {
        return results;
    }

    while (query.next()) {
        MessageWithBody mwb;
        mwb.message = new Message(parent, query);
        mwb.bodyContent = query.value(QStringLiteral("value")).toString();
        results.push_back(mwb);
    }
    return results;
}

Message* Message::fetchById(QSqlDatabase &db, const QString &id, QObject *parent)
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("SELECT * FROM ") + MESSAGE_TABLE + QStringLiteral(" WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!Utils::execWithLog(query, "fetching message by id")) {
        return nullptr;
    }

    if (query.next()) {
        return new Message(parent, query);
    }
    return nullptr;
}

void Message::saveToDb(QSqlDatabase &db)
{
    QJsonArray to;
    for (auto contact : m_to) {
        to.push_back(contact->toJson());
    }

    QJsonArray cc;
    for (auto contact : m_cc) {
        cc.push_back(contact->toJson());
    }

    QJsonArray bcc;
    for (auto contact : m_bcc) {
        bcc.push_back(contact->toJson());
    }

    QJsonArray replyTo;
    for (auto contact : m_replyTo) {
        replyTo.push_back(contact->toJson());
    }

    // prepare json object
    QJsonObject json;
    json[QStringLiteral("syncedAt")] = m_syncedAt.toSecsSinceEpoch();
    json[QStringLiteral("from")] = m_from->toJson();
    json[QStringLiteral("labels")] = QJsonArray::fromStringList(m_labels);
    json[QStringLiteral("to")] = to;
    json[QStringLiteral("cc")] = cc;
    json[QStringLiteral("bcc")] = bcc;
    json[QStringLiteral("replyTo")] = replyTo;
    json[QStringLiteral("snippet")] = m_snippet;
    json[QStringLiteral("plaintext")] = m_plaintext;

    QSqlQuery query{db};
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO ") + MESSAGE_TABLE +
        QStringLiteral(" (id, accountId, data, folderId, threadId, headerMessageId, gmailMessageId, gmailThreadId, subject, draft, unread, starred, date, remoteUID)") +
        QStringLiteral(" VALUES (:id, :accountId, :data, :folderId, :threadId, :headerMessageId, :gmailMessageId, :gmailThreadId, :subject, :draft, :unread, :starred, :date, :remoteUID)"));
    query.bindValue(QStringLiteral(":id"), m_id);
    query.bindValue(QStringLiteral(":accountId"), m_accountId);
    query.bindValue(QStringLiteral(":data"), QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":folderId"), m_folderId);
    query.bindValue(QStringLiteral(":threadId"), m_threadId);
    query.bindValue(QStringLiteral(":headerMessageId"), m_headerMessageId);
    query.bindValue(QStringLiteral(":gmailMessageId"), m_gmailMessageId);
    query.bindValue(QStringLiteral(":gmailThreadId"), m_gmailThreadId);
    query.bindValue(QStringLiteral(":subject"), m_subject);
    query.bindValue(QStringLiteral(":draft"), m_draft);
    query.bindValue(QStringLiteral(":unread"), m_unread);
    query.bindValue(QStringLiteral(":starred"), m_starred);
    query.bindValue(QStringLiteral(":date"), m_date);
    query.bindValue(QStringLiteral(":remoteUID"), m_remoteUid);
    query.exec();

    // update thread details
    if (!threadId().isEmpty()) {
        QSqlQuery threadQuery{db};
        query.prepare(QStringLiteral("SELECT * FROM ") + THREAD_TABLE + QStringLiteral(" WHERE id = ?"));
        query.addBindValue(threadId());
        query.exec();

        if (query.next()) {
            auto thread = std::make_shared<Thread>(nullptr, query);
            thread->updateAfterMessageChanges(m_snapshot, this);
            thread->saveToDb(db);
        }
    }

    createSnapshot();
}

void Message::deleteFromDb(QSqlDatabase &db)
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + MESSAGE_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();

    // update thread details
    if (!threadId().isEmpty()) {
        QSqlQuery threadQuery{db};
        query.prepare(QStringLiteral("SELECT * FROM ") + THREAD_TABLE + QStringLiteral(" WHERE id = ") + threadId());
        query.exec();

        if (query.next()) {
            auto thread = std::make_shared<Thread>(nullptr, query);
            thread->updateAfterMessageChanges(m_snapshot, nullptr);

            // delete if there are no folders left that reference it
            if (thread->folderIds().isEmpty()) {
                thread->deleteFromDb(db);
            } else {
                thread->saveToDb(db);
            }
        }
    }
}

void Message::createSnapshot()
{
    m_snapshot.unread = unread();
    m_snapshot.starred = starred();
}

QString Message::id() const
{
    return m_id;
}

QString Message::folderId() const
{
    return m_folderId;
}

void Message::setFolderId(const QString &folderId)
{
    m_folderId = folderId;
}

QString Message::accountId() const
{
    return m_accountId;
}

void Message::setAccountId(const QString &accountId)
{
    m_accountId = accountId;
}

QString Message::threadId() const
{
    return m_threadId;
}

void Message::setThreadId(const QString &threadId)
{
    m_threadId = threadId;
}

QList<MessageContact *> &Message::to()
{
    return m_to;
}

QList<MessageContact *> &Message::cc()
{
    return m_cc;
}

QList<MessageContact *> &Message::bcc()
{
    return m_bcc;
}

QList<MessageContact *> &Message::replyTo()
{
    return m_replyTo;
}

MessageContact *Message::from() const
{
    return m_from;
}

QString Message::headerMessageId() const
{
    return m_headerMessageId;
}

QString Message::subject() const
{
    return m_subject;
}

bool Message::draft() const
{
    return m_draft;
}

bool Message::unread() const
{
    return m_unread;
}

bool Message::starred() const
{
    return m_starred;
}

QDateTime Message::date() const
{
    return m_date;
}

QDateTime Message::syncedAt() const
{
    return m_syncedAt;
}

void Message::setSyncedAt(time_t syncedAt)
{
    m_syncedAt = QDateTime::fromSecsSinceEpoch(syncedAt);
}

QString Message::remoteUid() const
{
    return m_remoteUid;
}

void Message::setRemoteUid(const QString &remoteUid)
{
    m_remoteUid = remoteUid;
}

QStringList Message::labels() const
{
    return m_labels;
}

QString Message::snippet() const
{
    return m_snippet;
}

void Message::setSnippet(const QString &snippet)
{
    m_snippet = snippet;
}

bool Message::plaintext() const
{
    return m_plaintext;
}

void Message::setPlaintext(bool plaintext)
{
    m_plaintext = plaintext;
}

QList<std::shared_ptr<File>> &Message::files()
{
    return m_files;
}

void Message::setFiles(QList<std::shared_ptr<File>> files)
{
    m_files = files;
    // auto delete files when this message instance is deleted
    for (auto file : m_files) {
        file->setParent(this);
    }
}
