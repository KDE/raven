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

/**
 * Bridge class for Qt WebChannel communication with the thread view WebEngineView.
 * Provides thread/message data as JSON and handles attachment operations.
 */
class ThreadViewBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString themeCSS READ themeCSS WRITE setThemeCSS NOTIFY themeChanged)
    Q_PROPERTY(QString currentThreadId READ currentThreadId NOTIFY threadLoaded)

public:
    explicit ThreadViewBridge(QObject *parent = nullptr);

    // Get/set CSS variables for theming (set from QML using Kirigami.Theme)
    QString themeCSS() const;
    void setThemeCSS(const QString &css);

    // Current thread ID
    QString currentThreadId() const;

    // Get all messages for current thread as JSON array
    Q_INVOKABLE QString getMessagesJson() const;

    // Get attachments for a specific message as JSON array
    Q_INVOKABLE QString getAttachmentsJson(const QString &messageId) const;

    // Attachment actions
    Q_INVOKABLE void openAttachment(const QString &fileId);
    Q_INVOKABLE void saveAttachment(const QString &fileId);

    // Download attachment and return the file path (for inline images)
    Q_INVOKABLE QString downloadAttachment(const QString &fileId);

    // Open external URL in system browser
    Q_INVOKABLE void openExternalUrl(const QString &url);

public Q_SLOTS:
    // Load a thread by ID - triggers threadLoaded signal when done
    // folderRole is used to determine if we should block attachment downloads (e.g., for spam)
    void loadThread(const QString &threadId, const QString &accountId, const QString &folderRole = QString());

private Q_SLOTS:
    // Handle XDG portal file chooser response
    void handlePortalResponse(uint response, const QVariantMap &results);

Q_SIGNALS:
    // Emitted when thread data is loaded, contains JSON array of messages
    void threadLoaded(const QString &messagesJson);

    // Emitted when theme CSS changes
    void themeChanged();

    // Emitted when an attachment download completes
    void attachmentDownloaded(const QString &fileId, const QString &path);

private:
    // Process HTML content to replace cid: URLs with file: URLs
    QString processInlineImages(const QString &html, const QString &messageId) const;

    // Fetch attachment via D-Bus and return the file path
    QString fetchAttachmentViaDbus(const QString &fileId) const;

    // Format contacts list to string
    QString formatContacts(const QList<MessageContact*> &contacts) const;

    // Check if current folder blocks attachment auto-download (spam/junk)
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
