// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mailboxmodel.h"
#include "constants.h"

#include <QDebug>
#include <QSqlError>

MailBoxModel::MailBoxModel(QObject *parent)
    : QAbstractListModel{parent}
{
    load();
}

MailBoxModel *MailBoxModel::self()
{
    static MailBoxModel *instance = new MailBoxModel;
    return instance;
}

void MailBoxModel::load()
{
    beginResetModel();

    for (auto mailbox : m_mailBoxes) {
        mailbox.folder->deleteLater();
    }
    m_mailBoxes.clear();

    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM ") + FOLDERS_TABLE);

    if (!query.exec()) {
        // TODO better error handling
        qDebug() << "error fetching folders:" << query.lastError();
        endResetModel();
        return;
    }

    // loop over folders fetched from db
    while (query.next()) {

        Folder *folder = new Folder(this, query);

        MailBoxEntry entry;
        entry.folder = folder;
        entry.accountId = folder->accountId();
        entry.isCollapsible = false;
        entry.isCollapsed = false;

        // split name by / and determine folder ancestors
        QStringList ancestors = folder->path().split(QStringLiteral("/"));
        if (ancestors.size() > 0) {
            entry.name = ancestors.last();

            // remove last entry from ancestors
            ancestors.erase(ancestors.end() - 1);
        }

        entry.level = ancestors.size();

        // insert next to other folders of the same account
        int i = 0;
        int currentLevel = 0;
        for (; i < m_mailBoxes.size(); ++i) {
            auto mailbox = m_mailBoxes[i];

            // found mailbox
            if (mailbox.accountId == folder->accountId()) {

                // find correct level
                for (; i < m_mailBoxes.size() && m_mailBoxes[i].accountId == folder->accountId(); ++i) {

                    // insert if we are on the correct level
                    if (ancestors.isEmpty()) {
                        break;
                    }

                    // if the level went down (subtree doesn't exist)
                    if (m_mailBoxes[i].level < currentLevel) {
                        break;
                    }

                    // if the name is correct
                    if (m_mailBoxes[i].name == ancestors.first()) {
                        m_mailBoxes[i].isCollapsible = true;
                        ancestors.pop_front();
                        ++currentLevel;
                    }
                }

                break;
            }
        }

        // insert mailbox at i position

        // insert ancestors first
        while (!ancestors.isEmpty()) {
            MailBoxEntry ancestor;
            ancestor.folder = nullptr;
            ancestor.name = ancestors.first();
            ancestor.accountId = folder->accountId();
            ancestor.level = currentLevel;
            ancestor.isCollapsible = true;
            ancestor.isCollapsed = false;
            m_mailBoxes.insert(i, ancestor);

            ancestors.pop_front();
            ++currentLevel;
            ++i;
        }

        // now insert actual mailbox
        m_mailBoxes.insert(i, entry);
    }

    endResetModel();
}

void MailBoxModel::toggleCollapse(int rowIndex)
{
    if (rowIndex >= m_mailBoxes.count() || rowIndex < 0) {
        return;
    }

    m_mailBoxes[rowIndex].isCollapsed = !m_mailBoxes[rowIndex].isCollapsed;
    Q_EMIT dataChanged(index(rowIndex), index(rowIndex), {IsCollapsedRole});
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
    return {{NameRole, "name"}, {FolderRole, "folder"}, {LevelRole, "level"}, {IsCollapsibleRole, "isCollapsible"}, {IsCollapsedRole, "isCollapsed"}};
}
