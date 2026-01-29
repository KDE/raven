// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QObject>
#include <QSqlDatabase>

#include "models/thread.h"
#include "models/message.h"
#include "models/file.h"

#include <qqmlintegration.h>

// Bridge class for Qt WebChannel communication with the thread view WebEngineView.
// Provides thread/message data as JSON and handles attachment operations.
class ThreadViewBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString themeCSS READ themeCSS WRITE setThemeCSS NOTIFY themeChanged)
    Q_PROPERTY(QString currentThreadId READ currentThreadId NOTIFY threadLoaded)

public:
    explicit ThreadViewBridge(QObject *parent = nullptr);

    QString themeCSS() const;
    void setThemeCSS(const QString &css);

    QString currentThreadId() const;

    Q_INVOKABLE QString getMessagesJson() const;
    Q_INVOKABLE QString getAttachmentsJson(const QString &messageId) const;

    Q_INVOKABLE void openAttachment(const QString &fileId);
    Q_INVOKABLE void saveAttachment(const QString &fileId);

    // Download attachment and return the file path (for inline images)
    Q_INVOKABLE QString downloadAttachment(const QString &fileId);

    Q_INVOKABLE void openExternalUrl(const QString &url);

public Q_SLOTS:
    void loadThread(const QString &threadId, const QString &accountId, const QString &folderRole = QString());

private Q_SLOTS:
    void handlePortalResponse(uint response, const QVariantMap &results);

Q_SIGNALS:
    void threadLoaded(const QString &messagesJson);
    void themeChanged();
    void attachmentDownloaded(const QString &fileId, const QString &path);

private:
    QJsonObject messageToJson(Message *msg, const QString &content) const;
    QJsonObject fileToJson(File *file) const;

    QString processInlineImages(const QString &html, const QString &messageId) const;
    QString fetchAttachmentViaDbus(const QString &fileId) const;
    QString formatContacts(const QList<MessageContact*> &contacts) const;

    bool isSpamFolder() const;

    mutable QSqlDatabase m_db;
    QString m_themeCSS;
    QString m_currentThreadId;
    QString m_currentAccountId;
    QString m_currentFolderRole;
    QList<Message*> m_messages;
    QStringList m_messageContents;

    // Map of portal request path -> source file path for pending saves
    QMap<QString, QString> m_pendingPortalSaves;
};
