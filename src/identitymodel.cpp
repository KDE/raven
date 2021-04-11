// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "identitymodel.h"
#include <KIdentityManagement/Identity>
#include <QDebug>

IdentityModel::IdentityModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_identityManager(new IdentityManager(false, this))
{
    connect(m_identityManager, &KIdentityManagement::IdentityManager::needToReloadIdentitySettings, this, &IdentityModel::reloadUoidList);
    reloadUoidList();

}

void IdentityModel::reloadUoidList()
{
    beginResetModel();
    m_identitiesUoid.clear();
    qDebug() << m_identityManager;
    qDebug() << m_identityManager->identities();
    IdentityManager::ConstIterator it;
    IdentityManager::ConstIterator end(m_identityManager->end());
    for (it = m_identityManager->begin(); it != m_identityManager->end(); ++it) {
        m_identitiesUoid << it->uoid();
    }
    endResetModel();
}


IdentityModel::~IdentityModel()
{
}

QVariant IdentityModel::data (const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const auto &identity = m_identityManager->modifyIdentityForUoid(m_identitiesUoid[index.row()]);
    switch (role) {
        case Qt::DisplayRole:
            return identity.fullEmailAddr();
        case NameRole:
            return identity.identityName();
        case EmailRole:
            return identity.primaryEmailAddress();
        
    }
    
    return {};
}

int IdentityModel::rowCount(const QModelIndex& parent) const
{
    return m_identitiesUoid.count();
}


QHash<int, QByteArray> IdentityModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {UuidRole, QByteArrayLiteral("uoid")},
        {EmailRole, QByteArrayLiteral("email")},
        {NameRole, QByteArrayLiteral("name")}
    };
}
