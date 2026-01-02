// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "accountmodel.h"
#include "constants.h"
#include "daemonstatus.h"
#include "dbwatcher.h"
#include "dbmanager.h"
#include "models/folder.h"

#include "modelviews/attachmentmodel.h"
#include "modelviews/mailboxmodel.h"
#include "modelviews/maillistmodel.h"
#include "modelviews/threadviewmodel.h"

#include <QObject>
#include <QThread>
#include <QList>

#include <qqmlintegration.h>
#include <QQmlEngine>
#include <QJSEngine>

class Raven : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(AccountModel *accountModel MEMBER m_accountModel CONSTANT)
    Q_PROPERTY(AttachmentModel *attachmentModel MEMBER m_attachmentModel CONSTANT)
    Q_PROPERTY(DaemonStatus *daemonStatus MEMBER m_daemonStatus CONSTANT)
    Q_PROPERTY(MailBoxModel *mailBoxModel MEMBER m_mailBoxModel CONSTANT)
    Q_PROPERTY(MailListModel *mailListModel MEMBER m_mailListModel CONSTANT)
    Q_PROPERTY(ThreadViewModel *threadViewModel MEMBER m_threadViewModel CONSTANT)
    Q_PROPERTY(QString selectedFolderName READ selectedFolderName WRITE setSelectedFolderName NOTIFY selectedFolderNameChanged)

public:
    Raven(QObject *parent = nullptr);

    static Raven *self();

    QString selectedFolderName() const;
    void setSelectedFolderName(QString folderName);

    // Sync action (triggers daemon to sync with server)
    Q_INVOKABLE void triggerSyncForAccount(const QString &accountId = QString());

Q_SIGNALS:
    void selectedFolderNameChanged();

private:
    AccountModel *m_accountModel = nullptr;
    AttachmentModel *m_attachmentModel = nullptr;
    DaemonStatus *m_daemonStatus = nullptr;
    MailBoxModel *m_mailBoxModel = nullptr;
    MailListModel *m_mailListModel = nullptr;
    ThreadViewModel *m_threadViewModel = nullptr;

    QString m_selectedFolderName;
};
