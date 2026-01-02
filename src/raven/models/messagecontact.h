// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QtQml/qqmlregistration.h>

class MessageContact : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString email READ email CONSTANT)

public:
    MessageContact(QObject *parent, const QJsonObject &json);

    QJsonObject toJson() const;

    QString name() const;
    QString email() const;

private:
    QString m_name;
    QString m_email;
};

