// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mailboxmodel.h"
#include "constants.h"
#include "accountmodel.h"
#include "utils.h"
#include "dbmanager.h"

#include <QDebug>
#include <QSqlError>

MailBoxModel::MailBoxModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_db{DBManager::openDatabase(QStringLiteral("mailBoxModel"))}
{
}

MailBoxModel *MailBoxModel::self()
{
    static MailBoxModel *instance = new MailBoxModel;
    return instance;
}

void MailBoxModel::setDatabase(const QSqlDatabase &db)
{
    m_db = db;
}

std::pair<MailBoxEntry, QStringList> MailBoxModel::folderToMailbox(Folder *folder)
{
    MailBoxEntry entry;
    entry.folder = folder;
    entry.accountId = folder->accountId();
    entry.isCollapsible = false;
    entry.isCollapsed = false;
    entry.visible = true;

    // split name by / and determine folder ancestors
    QStringList ancestors = folder->path().split(QStringLiteral("/"));
    if (ancestors.size() > 0) {
        entry.name = ancestors.last();

        // remove last entry from ancestors
        ancestors.erase(ancestors.end() - 1);
    }

    // add email to ancestor
    Account *account = AccountModel::self()->accountById(folder->accountId());
    if (account) {
        ancestors.push_front(account->email());
    }

    entry.level = ancestors.size();

    return {entry, ancestors};
}

void MailBoxModel::insertMailBoxIntoTree(MailBoxNode &node, MailBoxEntry &entry, QStringList &ancestors, int level)
{
    if (ancestors.empty()) {
        node.entry.isCollapsible = true;
        node.children.push_back(MailBoxNode{entry, {}});
        return;
    }

    for (auto &child : node.children) {
        if (child.entry.name == ancestors.front()) {
            ancestors.pop_front();
            insertMailBoxIntoTree(child, entry, ancestors, level + 1);
            return;
        }
    }

    MailBoxEntry ancestor;
    ancestor.folder = nullptr;
    ancestor.name = ancestors.first();
    ancestor.accountId = entry.folder->accountId();
    ancestor.level = level;
    ancestor.isCollapsible = true;
    ancestor.isCollapsed = false;
    ancestor.visible = true;

    node.children.push_back(MailBoxNode{ancestor, {}});
    ancestors.pop_front();

    insertMailBoxIntoTree(node.children[node.children.size() - 1], entry, ancestors, level + 1);
}

void MailBoxModel::flattenMailBoxTree(const MailBoxNode &node, QList<MailBoxEntry> &list)
{
    if (!node.entry.name.isEmpty()) {
        list.push_back(std::move(node.entry));
    }
    for (auto child : node.children) {
        flattenMailBoxTree(child, list);
    }
}

void MailBoxModel::organizeMailBoxFolders(MailBoxNode &node)
{
    int folderCount = 0;

    // determine folder count and recurse to children first
    for (auto &child : node.children) {
        if (child.children.size() > 0) { // is folder
            ++folderCount;
            organizeMailBoxFolders(child);
        }
    }

    // move folders to the back of the children
    for (int i = 0; i < folderCount; i++) {
        for (int j = node.children.size() - 1 - i; j >= 0; --j) {
            if (node.children[j].children.size() > 0) { // is folder
                int swapPos = node.children.size() - 1 - i;
                node.children.swapItemsAt(j, swapPos);
            }
        }
    }
}

QList<MailBoxEntry> MailBoxModel::initMailBoxes(const QList<Folder *> &folders)
{
    // first step: create tree
    MailBoxNode root;

    for (auto folder : folders) {
        auto pair = folderToMailbox(folder);
        MailBoxEntry entry = pair.first;
        QStringList ancestors = pair.second;

        insertMailBoxIntoTree(root, entry, ancestors, 0);
    }

    // second step: move folders to bottom of all groups
    organizeMailBoxFolders(root);

    // third step: flatten tree into list
    QList<MailBoxEntry> list;
    flattenMailBoxTree(root, list);

    return list;
}

void MailBoxModel::load()
{
    beginResetModel();
    qDebug() << "MailBoxModel::load() - reloading";

    for (auto mailbox : m_mailBoxes) {
        if (mailbox.folder) {
            mailbox.folder->deleteLater();
        }
    }
    m_mailBoxes.clear();

    if (!m_db.isOpen()) {
        qWarning() << "MailBoxModel::load() - Database not open";
        endResetModel();
        return;
    }

    QList<Folder *> folders = Folder::fetchAll(m_db, this);
    m_mailBoxes = initMailBoxes(folders);

    endResetModel();
}

void MailBoxModel::toggleCollapse(int rowIndex)
{
    if (rowIndex >= m_mailBoxes.count() || rowIndex < 0) {
        return;
    }

    m_mailBoxes[rowIndex].isCollapsed = !m_mailBoxes[rowIndex].isCollapsed;

    int i = rowIndex + 1;
    while (i < m_mailBoxes.size() && m_mailBoxes[rowIndex].level < m_mailBoxes[i].level) {
        m_mailBoxes[i].visible = !m_mailBoxes[i].visible;
        ++i;
    }

    Q_EMIT dataChanged(index(rowIndex), index(rowIndex), {IsCollapsedRole});
    Q_EMIT dataChanged(index(rowIndex), index(i-1), {VisibleRole});
}

int MailBoxModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_mailBoxes.count();
}

QVariant MailBoxModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_mailBoxes.count()) {
        return QVariant{};
    }

    switch (role) {
        case FolderRole:
            return QVariant::fromValue(m_mailBoxes[index.row()].folder);
        case NameRole:
            return m_mailBoxes[index.row()].name;
        case LevelRole:
            return m_mailBoxes[index.row()].level;
        case IsCollapsibleRole:
            return m_mailBoxes[index.row()].isCollapsible;
        case IsCollapsedRole:
            return m_mailBoxes[index.row()].isCollapsed;
        case VisibleRole:
            return m_mailBoxes[index.row()].visible;
    }
    return {};
}

Qt::ItemFlags MailBoxModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> MailBoxModel::roleNames() const
{
    return {{NameRole, "name"}, {FolderRole, "folder"}, {LevelRole, "level"}, {IsCollapsibleRole, "isCollapsible"}, {IsCollapsedRole, "isCollapsed"}, {VisibleRole, "visible"}};
}
