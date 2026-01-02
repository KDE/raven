// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "message.h"

class Message;
class File : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString accountId READ accountId CONSTANT)
    Q_PROPERTY(QString messageId READ messageId CONSTANT)
    Q_PROPERTY(QString fileName READ fileName CONSTANT)
    Q_PROPERTY(QString contentType READ contentType CONSTANT)
    Q_PROPERTY(int size READ size CONSTANT)
    Q_PROPERTY(bool isInline READ isInline CONSTANT)
    Q_PROPERTY(bool downloaded READ downloaded NOTIFY downloadedChanged)

public:
    File(QObject *parent, const QJsonObject &json);
    File(QObject *parent, const QSqlQuery &query);

    // Static fetch methods (read-only - mutations go through D-Bus to daemon)
    static QList<File*> fetchByMessage(QSqlDatabase &db, const QString &messageId, QObject *parent = nullptr);
    static File* fetchById(QSqlDatabase &db, const QString &id, QObject *parent = nullptr);

    QJsonObject toJson() const;

    QString id() const;
    QString accountId() const;
    QString messageId() const;
    QString fileName() const;
    QString partId() const;
    QString contentId() const;
    void setContentId(const QString &contentId);
    QString contentType() const;
    int size() const;
    bool isInline() const;
    bool downloaded() const;
    void setDownloaded(bool downloaded);

    // Get the icon name for this attachment type
    QString iconName() const;

    // Get formatted size string (e.g., "1.5 MB")
    QString formattedSize() const;

    // Get the absolute file path on disk
    QString filePath() const;

Q_SIGNALS:
    void downloadedChanged();

private:
    QString m_id;
    QString m_accountId;
    QString m_messageId;
    QString m_filename;
    QString m_partId;
    QString m_contentId;
    QString m_contentType;
    int m_size;
    bool m_isInline;
    bool m_downloaded;
};
