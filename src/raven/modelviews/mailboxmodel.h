// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QSqlDatabase>

#include "../models/folder.h"

#include <qqmlintegration.h>

struct MailBoxEntry
{
    Folder *folder;
    QString name;
    QString accountId;
    int level;
    bool isCollapsible;
    bool isCollapsed;
    bool visible;
};

struct MailBoxNode
{
    MailBoxEntry entry;
    QList<MailBoxNode> children;
};

class MailBoxModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MailBoxModel not creatable in QML")

public:
    MailBoxModel(QObject *parent = nullptr);

    enum {
        NameRole,
        FolderRole,
        LevelRole,
        IsCollapsibleRole,
        IsCollapsedRole,
        VisibleRole
    };

    static MailBoxModel *self();

    // Set the database connection to use for operations
    void setDatabase(const QSqlDatabase &db);

    void load();

    Q_INVOKABLE void toggleCollapse(int rowIndex);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    std::pair<MailBoxEntry, QStringList> folderToMailbox(Folder *folder);

    QList<MailBoxEntry> initMailBoxes(const QList<Folder *> &folders);
    void organizeMailBoxFolders(MailBoxNode &node);
    void flattenMailBoxTree(const MailBoxNode &node, QList<MailBoxEntry> &list);
    void insertMailBoxIntoTree(MailBoxNode &node, MailBoxEntry &entry, QStringList &ancestors, int level);

    QSqlDatabase m_db;
    QList<MailBoxEntry> m_mailBoxes;
};
