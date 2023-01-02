// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>
#include <QDateTime>

#include <MailCore/MailCore.h>

#include "messagecontact.h"

class Message : public QObject
{
    Q_OBJECT

public:
    Message(QObject *parent = nullptr, mailcore::IMAPMessage *msg = nullptr);

    QList<MessageContact *> &cc();
    QList<MessageContact *> &bcc();
    QList<MessageContact *> &replyTo();

    MessageContact *from() const;

    QDateTime time() const;
    QString subject() const;

private:
    QList<MessageContact *> m_cc;
    QList<MessageContact *> m_bcc;
    QList<MessageContact *> m_replyTo;
    MessageContact *m_from;

    QDateTime m_date;
    QString m_subject;

};
