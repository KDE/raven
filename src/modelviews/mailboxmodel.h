// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>

#include "models/folder.h"

struct MailBoxEntry {
    Folder *folder;
    QString name;
    QString accountId;
    int level;
    bool isCollapsible;
    bool isCollapsed;
};

class MailBoxModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MailBoxModel(QObject *parent = nullptr);

    enum {
        NameRole,
        FolderRole,
        LevelRole,
        IsCollapsibleRole,
        IsCollapsedRole
    };

    static MailBoxModel *self();

    void load();

    Q_INVOKABLE void toggleCollapse(int rowIndex);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<MailBoxEntry> m_mailBoxes;
};
