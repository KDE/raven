// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <MailCore/MailCore.h>

#include "message.h"

class Message;
class File : public QObject
{
    Q_OBJECT

public:
    File(QObject *parent = nullptr, Message *msg = nullptr, mailcore::Attachment *a = nullptr);
    File(QObject *parent, const QJsonObject &json);
    File(QObject *parent, const QSqlQuery &query);

    void saveToDb(QSqlDatabase &db) const;
    void deleteFromDb(QSqlDatabase &db) const;

    QJsonObject toJson();

    QString id() const;
    QString accountId() const;
    QString messageId() const;
    QString fileName() const;
    QString partId() const;
    QString contentId() const;
    void setContentId(const QString &contentId);
    QString contentType() const;
    int size() const;

private:
    QString m_id;
    QString m_accountId;
    QString m_messageId;
    QString m_filename;
    QString m_partId;
    QString m_contentId;
    QString m_contentType;
    int m_size;
};
