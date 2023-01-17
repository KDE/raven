// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QList>

#include "../libraven/models/message.h"
#include "../libraven/models/thread.h"

class ThreadViewModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ThreadViewModel(QObject *parent = nullptr);

    enum {
        SubjectRole,
        FromRole,
        ToRole,
        BccRole,
        CcRole,
        IsPlaintextRole,
        ContentRole,
        SnippetRole,
        UnreadRole,
        DateRole
    };

    static ThreadViewModel *self();

    Q_INVOKABLE void loadThread(Thread *thread);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QString getContactsStr(QList<MessageContact *> contacts);

    QList<Message *> m_messages;
    QStringList m_messageContents;
    QStringList m_messageTo;
    QStringList m_messageCc;
    QStringList m_messageBcc;
};

