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

#include "mailmodel.h"

#include <QTimer>

// Akonadi
#include <control.h>
#include <CollectionFilterProxyModel>
#include <ItemFetchScope>
#include <MessageModel>
#include <Monitor>
#include <KDescendantsProxyModel>
#include <EntityMimeTypeFilterModel>
#include <EntityTreeModel>
#include <SelectionProxyModel>
#include <QApplication>
#include <QtCore/QItemSelectionModel>

#include <KItemModels/KDescendantsProxyModel>

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
    monitor->itemFetchScope().fetchFullPayload(true);

//    Akonadi::mBrowserModel = new AkonadiBrowserModel(mBrowserMonitor, this);
//    mBrowserModel->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);
//    mBrowserModel->setShowSystemEntities(true);
//    mBrowserModel->setListFilter(CollectionFetchScope::Display);

    auto treeModel = new Akonadi::EntityTreeModel(monitor);
    treeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    m_entityTreeModel = new Akonadi::CollectionFilterProxyModel();
    m_entityTreeModel->setSourceModel(treeModel);
    m_entityTreeModel->addMimeTypeFilter(QLatin1String("message/rfc822"));

    m_descendantsProxyModel = new KDescendantsProxyModel(this);
    m_descendantsProxyModel->setSourceModel(m_entityTreeModel);


    using namespace Akonadi;
    //                         itemView
    //                             ^
    //                             |
    //                         itemModel
    //                             |
    //                         flatModel
    //                             |
    //   collectionView --> selectionModel
    //           ^                 ^
    //           |                 |
    //  collectionFilter           |
    //            \______________model

    // setup collection model
//    EntityMimeTypeFilterModel *collectionFilter = new EntityMimeTypeFilterModel( this );
//    collectionFilter->setSourceModel( treeModel );
//    collectionFilter->addMimeTypeInclusionFilter( Collection::mimeType() );
//    collectionFilter->setHeaderGroup( EntityTreeModel::CollectionTreeHeaders );
//    // setup collection view
//    EntityTreeView *collectionView = new EntityTreeView( this );
//    collectionView->setModel( collectionFilter );

    // fake selectionModel
    m_collectionSelectionModel = new QItemSelectionModel(m_entityTreeModel);
    // setup selection model
    auto selectionModel = new SelectionProxyModel(m_collectionSelectionModel, this);
    selectionModel->setSourceModel(treeModel);
    selectionModel->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);
    qDebug() << selectionModel->filterBehavior();

    // setup item model
    KDescendantsProxyModel *descendedList = new KDescendantsProxyModel(this);
    descendedList->setSourceModel(selectionModel);

    auto folderFilterModel = new EntityMimeTypeFilterModel(this);
    folderFilterModel->setSourceModel(descendedList);
    folderFilterModel->setHeaderGroup(EntityTreeModel::ItemListHeaders);
    folderFilterModel->addMimeTypeExclusionFilter(Collection::mimeType());

    m_folderModel = new MailModel( this );
    m_folderModel->setSourceModel(folderFilterModel);

    //connect(treeModel, &Akonadi::EntityTreeModel::columnsChanged, selectionModel, &Akonadi::SelectionProxyModel::invalidate);

    connect(m_folderModel, &MailModel::rowsInserted, [](){qDebug() << "rowAdded";});

    m_loading = false;
    Q_EMIT entityTreeModelChanged();
    Q_EMIT descendantsProxyModelChanged();
    Q_EMIT folderModelChanged();
    Q_EMIT loadingChanged();
}

MailModel *QuickMail::folderModel() const
{
    return m_folderModel;
}

void QuickMail::loadMailCollection(const int &index)
{
    QModelIndex flatIndex = m_descendantsProxyModel->index(index, 0);
    QModelIndex modelIndex = m_descendantsProxyModel->mapToSource(flatIndex);

    if (!modelIndex.isValid()) {
        return;
    }

    m_collectionSelectionModel->select(modelIndex, QItemSelectionModel::ClearAndSelect);

    qDebug() << modelIndex << m_collectionSelectionModel->selectedIndexes() << m_folderModel->rowCount();

    const Akonadi::Collection collection = modelIndex.model()->data(modelIndex, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    qDebug() << collection.displayName() << collection.enabled();

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

KDescendantsProxyModel *QuickMail::descendantsProxyModel() const
{
    return m_descendantsProxyModel;
}


#include "quickmail.moc"


