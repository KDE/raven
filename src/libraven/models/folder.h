// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QList>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime>
#include <QJsonObject>

#include "messagecontact.h"

class Folder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString role READ role NOTIFY roleChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    Folder(QObject *parent = nullptr, QString id = QString(), QString accountId = QString());
    Folder(QObject *parent, const QSqlQuery &query);

    virtual void saveToDb(QSqlDatabase &db) const;
    virtual void deleteFromDb(QSqlDatabase &db) const;

    QString id() const;
    QString accountId() const;

    QString path() const;
    void setPath(const QString &path);

    QString role() const;
    void setRole(const QString &role);

    QDateTime createdAt() const;

    QString status() const;
    void setStatus(const QString &status);

    QJsonObject &localStatus();

Q_SIGNALS:
    void pathChanged();
    void roleChanged();
    void statusChanged();

private:
    QString m_id;
    QString m_accountId;
    QString m_path;
    QString m_role;
    QDateTime m_createdAt;
    QJsonObject m_localStatus;

    // not SQL field
    QString m_status;

    friend class Label;
};

