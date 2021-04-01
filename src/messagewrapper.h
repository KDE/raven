// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <KMime/Message>

/// Simple wrapper arround a KMime::Message for QML consumption.
class MessageWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString from READ from CONSTANT);
    Q_PROPERTY(QStringList to READ to CONSTANT);
    Q_PROPERTY(QStringList cc READ cc CONSTANT);
    Q_PROPERTY(QString sender READ sender CONSTANT);
    Q_PROPERTY(QString subject READ subject CONSTANT);
    Q_PROPERTY(QDateTime date READ date CONSTANT);
    Q_PROPERTY(QString plainContent READ plainContent CONSTANT);
public:
    explicit MessageWrapper(KMime::Message::Ptr mail, QObject *parent = nullptr);
    ~MessageWrapper();

    QString from() const;
    QStringList to() const;
    QStringList cc() const;
    QString sender() const;
    QString subject() const;
    QDateTime date() const;
    QString plainContent() const;

private:
    KMime::Message::Ptr m_mail;
};
