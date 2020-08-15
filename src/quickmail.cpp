/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright 2020  Carl Schwan <carl@carlschwan.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quickmail.h"

#include <QTimer>

// Akonadi
#include <control.h>
#include <CollectionFilterProxyModel>
#include <MessageModel>
#include <Monitor>
#include <EntityTreeModel>
#include <QApplication>

QuickMail::QuickMail(QObject *parent)
    : QObject(parent)
    , m_loading(true)
    , m_entityTreeModel(nullptr)
{
    setObjectName("QuickMail");
    QTimer::singleShot(0, this, &QuickMail::delayedInit);
}

void QuickMail::delayedInit()
{
    if (!Akonadi::Control::start() ) {
        qApp->exit(-1);
        return;
    }
    
    Akonadi::Monitor *monitor = new Akonadi::Monitor();
    monitor->setObjectName(QStringLiteral("CollectionWidgetMonitor"));
    monitor->fetchCollection(true);
    monitor->setAllMonitored(true);

    Akonadi::EntityTreeModel *treeModel = new Akonadi::EntityTreeModel(monitor);

    m_entityTreeModel = new Akonadi::CollectionFilterProxyModel();
    m_entityTreeModel->setSourceModel(treeModel);
    m_entityTreeModel->addMimeTypeFilter(QLatin1String("message/rfc822"));
    
    m_loading = false;
    Q_EMIT entityTreeModelChanged();
    Q_EMIT loadingChanged();
}

void QuickMail::loadMailCollection(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    const Akonadi::Collection collection = index.model()->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    
    if (!collection.isValid()) {
        return;
    }
}

bool QuickMail::loading() const
{
    return m_loading;
}


Akonadi::CollectionFilterProxyModel *QuickMail::entityTreeModel() const
{
    return m_entityTreeModel;
}


#include "quickmail.moc"


