// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>

#include <MailCore/MailCore.h>

class MessageContact : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString email READ email CONSTANT)

public:
    MessageContact(QObject *parent = nullptr, mailcore::Address *address = nullptr);

    QString name() const;
    QString email() const;

private:
    QString m_name;
    QString m_email;
};

