// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "messagewrapper.h"
#include <KLocalizedString>
#include <algorithm>

MessageWrapper::MessageWrapper(KMime::Message::Ptr mail, QObject *parent)
    : QObject(parent)
    , m_mail(mail)
{
}

MessageWrapper::~MessageWrapper()
{
}

QString MessageWrapper::from() const
{
    return m_mail->from()->asUnicodeString();
}

QStringList MessageWrapper::to() const
{
    const auto addresses = m_mail->to()->addresses();
    QList<QString> ret;
    std::transform(addresses.cbegin(), addresses.cend(),
                   std::back_inserter(ret), [](const QByteArray &addresss) {
                       return QString::fromUtf8(addresss);
                   });
    return ret;
}

QStringList MessageWrapper::cc() const
{
    const auto addresses = m_mail->cc()->addresses();
    QList<QString> ret;
    std::transform(addresses.cbegin(), addresses.cend(),
                   std::back_inserter(ret), [](const QByteArray &addresss) {
                       return QString::fromUtf8(addresss);
                   });
    return ret;
}

QString MessageWrapper::sender() const
{
    return m_mail->sender()->asUnicodeString();
}

QString MessageWrapper::subject() const
{
    if (m_mail->subject()) {
        return m_mail->subject()->asUnicodeString();
    } else {
        return i18n("(No subject)");
    }
}

QDateTime MessageWrapper::date() const
{
    if (m_mail->date()) {
        return m_mail->date()->dateTime();
    } else {
        return QDateTime();
    }
}

QString MessageWrapper::plainContent() const
{
    const auto plain = m_mail->mainBodyPart("text/plain");
    if (plain) {
        return plain->body();
    }
    return m_mail->textContent()->body();
}
