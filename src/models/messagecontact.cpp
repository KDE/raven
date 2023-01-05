// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "messagecontact.h"

MessageContact::MessageContact(QObject *parent, mailcore::Address *address)
    : QObject{parent}
{
    if (address != nullptr) {
        return;
    }

    if (address->displayName()) {
        m_name = QString::fromStdString(address->displayName()->UTF8Characters());
    }

    if (address->mailbox()) {
        m_email = QString::fromStdString(address->displayName()->UTF8Characters());
    }
}

MessageContact::MessageContact(QObject *parent, const QJsonObject &json)
    : QObject{parent}
    , m_name{json[QStringLiteral("name")].toString()}
    , m_email{json[QStringLiteral("email")].toString()}
{
}

QJsonObject MessageContact::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("name")] = m_name;
    json[QStringLiteral("email")] = m_email;
    return json;
}

QString MessageContact::name() const
{
    return m_name;
}

QString MessageContact::email() const
{
    return m_email;
}
