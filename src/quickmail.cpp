// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    m_session = new Session(QByteArrayLiteral("KQuickMail Kernel ETM"), this);
    auto folderCollectionMonitor = new MailCommon::FolderCollectionMonitor(m_session, this);

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

    // Proxy model for displaying the tree in a QML view.
    m_descendantsProxyModel = new KDescendantsProxyModel(this);
    m_descendantsProxyModel->setSourceModel(m_entityTreeModel);

    // Setup selection model
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
}

bool QuickMail::loading() const
{
    return m_loading;
}

Akonadi::CollectionFilterProxyModel *QuickMail::entityTreeModel() const
{
    return m_entityTreeModel;
}

Akonadi::Session *QuickMail::session() const
{
    return m_session;
}

KDescendantsProxyModel *QuickMail::descendantsProxyModel() const
{
    return m_descendantsProxyModel;
}

QuickMail &QuickMail::instance()
{
    static QuickMail _instance;
    return _instance;
}
