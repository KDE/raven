// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "message.h"

#include "../constants.h"

Message::Message(QObject *parent, mailcore::IMAPMessage *msg)
    : QObject{parent}
{
    if (msg == nullptr) {
        return;
    }

    for (unsigned int i = 0; i < msg->header()->cc()->count(); ++i) {
        auto addr = static_cast<mailcore::Address *>(msg->header()->cc()->objectAtIndex(i));
        m_cc.push_back(new MessageContact(this, addr));
    }

    for (unsigned int i = 0; i < msg->header()->bcc()->count(); ++i) {
        auto addr = static_cast<mailcore::Address *>(msg->header()->cc()->objectAtIndex(i));
        m_bcc.push_back(new MessageContact(this, addr));
    }

    for (unsigned int i = 0; i < msg->header()->replyTo()->count(); ++i) {
        auto addr = static_cast<mailcore::Address *>(msg->header()->cc()->objectAtIndex(i));
        m_replyTo.push_back(new MessageContact(this, addr));
    }

    m_from = new MessageContact(this, msg->header()->from());
    m_subject = QString::fromStdString(msg->header()->subject() ? msg->header()->subject()->UTF8Characters() : "");
    m_date = QDateTime::fromSecsSinceEpoch(msg->header()->date() == -1 ? msg->header()->receivedDate() : msg->header()->date());
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

QDateTime Message::time() const
{
    return m_date;
}

QString Message::subject() const
{
    return m_subject;
}
