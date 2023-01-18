// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QList>

#include "../libraven/models/folder.h"
#include "../libraven/models/thread.h"

class MailListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MailListModel(QObject *parent = nullptr);

    enum {
        ThreadRole,
        FromRole,
        SubjectRole,
        SnippetRole,
        UnreadRole,
        StarredRole,
        DateRole
    };

    static MailListModel *self();

    Q_INVOKABLE void loadFolder(Folder *folder);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QString getThreadFrom(Thread *thread);
    
    QList<Thread *> m_threads;
    QStringList m_threadDate;
    QStringList m_threadFrom;
};
