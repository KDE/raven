// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

// Much of the logic is based on https://github.com/Foundry376/Mailspring-Sync/blob/master/MailSync/MailProcessor.cpp

#pragma once

#include <QObject>

#include <MailCore/MailCore.h>

#include "models/message.h"
#include "models/thread.h"
#include "models/file.h"
#include "backendworker/accountworker.h"

using namespace mailcore;

class MailProcessor : public QObject
{
    Q_OBJECT

public:
    MailProcessor(QObject *parent = nullptr, AccountWorker *worker = nullptr);

    std::shared_ptr<Message> insertFallbackToUpdateMessage(IMAPMessage *mMsg, Folder &folder, time_t syncDataTimestamp);
    std::shared_ptr<Message> insertMessage(IMAPMessage *mMsg, Folder &folder, time_t syncDataTimestamp);
    void retrievedMessageBody(Message *message, MessageParser *parser);
    bool retrievedFileData(File *file, Data *data);
    void unlinkMessagesMatchingQuery(const QString &sqlQuery, int phase);
    void deleteMessagesStillUnlinkedFromPhase(int phase);

private:
    void appendToThreadSearchContent(Thread *thread, Message *messageToAppendOrNull, mailcore::String *bodyToAppendOrNull);
    void upsertThreadReferences(QString threadId, QString accountId, QString headerMessageId, mailcore::Array *references);
    void upsertContacts(Message *message);
    std::shared_ptr<Label> labelForXGMLabelName(QString mlname);

    AccountWorker *m_worker;
};
