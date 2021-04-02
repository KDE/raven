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
#include <AkonadiCore/Session>
#include <ChangeRecorder>
#include <MailCommon/FolderCollectionMonitor>
#include <KMime/Message>
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

    //Monitor *monitor = new Monitor( this );
    //monitor->fetchCollection(true);
    //monitor->setCollectionMonitored(Collection::root());
    //monitor->setMimeTypeMonitored(KMime::Message::mimeType());
    auto session = new Session(QByteArrayLiteral("KQuickMail Kernel ETM"), this);
    auto folderCollectionMonitor = new MailCommon::FolderCollectionMonitor(session, this);

    //Akonadi::Monitor *monitor = new Akonadi::Monitor();
    //monitor->setObjectName(QStringLiteral("CollectionWidgetMonitor"));
    //monitor->fetchCollection(true);
    //monitor->setAllMonitored(true);
    //monitor->itemFetchScope().fetchFullPayload(true);

    // setup collection model
    auto treeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor->monitor(), this);
    treeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    m_entityTreeModel = new Akonadi::CollectionFilterProxyModel();
    m_entityTreeModel->setSourceModel(treeModel);
    m_entityTreeModel->addMimeTypeFilter(KMime::Message::mimeType());

    m_descendantsProxyModel = new KDescendantsProxyModel(this);
    m_descendantsProxyModel->setSourceModel(m_entityTreeModel);

    // setup selection model
    m_collectionSelectionModel = new QItemSelectionModel(m_entityTreeModel);
    auto selectionModel = new SelectionProxyModel(m_collectionSelectionModel, this);
    selectionModel->setSourceModel(treeModel);
    selectionModel->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);
    qDebug() << selectionModel->filterBehavior();

    // Setup mail model
    auto folderFilterModel = new EntityMimeTypeFilterModel(this);
    folderFilterModel->setSourceModel(selectionModel);
    folderFilterModel->setHeaderGroup(EntityTreeModel::ItemListHeaders);
    folderFilterModel->addMimeTypeInclusionFilter(QStringLiteral("message/rfc822"));
    folderFilterModel->addMimeTypeExclusionFilter(Collection::mimeType());

    // Proxy for QML roles
    m_folderModel = new MailModel(this);
    m_folderModel->setSourceModel(folderFilterModel);

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
    const Akonadi::Collection collection = modelIndex.model()->data(modelIndex, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    qDebug() << "loaded";
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
