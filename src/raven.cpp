// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"

#include "mailmodel.h"

#include <QTimer>

// Akonadi
#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/ItemFetchScope>
// #include <MessageModel>
#include <Akonadi/Monitor>
#include <Akonadi/Session>
#include <Akonadi/ChangeRecorder>
#include <MailCommon/FolderCollectionMonitor>
#include <KMime/Message>
#include <KDescendantsProxyModel>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/SelectionProxyModel>
#include <Akonadi/ServerManager>
#include <QApplication>
#include <QtCore/QItemSelectionModel>

#include <KItemModels/KDescendantsProxyModel>

Raven::Raven(QObject *parent)
    : QObject(parent)
    , m_loading(true)
{

    using namespace Akonadi;
    //                              mailModel
    //                                  ^
    //                                  |
    //                              itemModel
    //                                  |
    //                              flatModel
    //                                  |
    //  descendantsProxyModel ------> selectionModel
    //           ^                      ^
    //           |                      |
    //  collectionFilter                |
    //            \__________________treemodel

    m_session = new Session(QByteArrayLiteral("KQuickMail Kernel ETM"), this);
    auto folderCollectionMonitor = new MailCommon::FolderCollectionMonitor(m_session, this);

    // setup collection model
    auto treeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor->monitor(), this);
    treeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    m_foldersModel = new Akonadi::CollectionFilterProxyModel(this);
    m_foldersModel->setSourceModel(treeModel);
    m_foldersModel->addMimeTypeFilter(KMime::Message::mimeType());

    // Setup selection model
    m_collectionSelectionModel = new QItemSelectionModel(m_foldersModel);
    auto selectionModel = new SelectionProxyModel(m_collectionSelectionModel, this);
    selectionModel->setSourceModel(treeModel);
    selectionModel->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);

    // Setup mail model
    auto folderFilterModel = new EntityMimeTypeFilterModel(this);
    folderFilterModel->setSourceModel(selectionModel);
    folderFilterModel->setHeaderGroup(EntityTreeModel::ItemListHeaders);
    folderFilterModel->addMimeTypeInclusionFilter(KMime::Message::mimeType());
    folderFilterModel->addMimeTypeExclusionFilter(Collection::mimeType());

    // Proxy for QML roles
    m_folderModel = new MailModel(this);
    m_folderModel->setSourceModel(folderFilterModel);

    if (Akonadi::ServerManager::isRunning()) {
        m_loading = false;
    } else {
        connect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged,
                this, [this](Akonadi::ServerManager::State state) {
                    if (state == Akonadi::ServerManager::State::Broken) {
                        qApp->exit(-1);
                        return;
                    }
                    bool loading = state != Akonadi::ServerManager::State::Running;
                    if (loading == m_loading) {
                        return;
                    }
                    m_loading = loading;
                    Q_EMIT loadingChanged();
                    disconnect(Akonadi::ServerManager::self(), &Akonadi::ServerManager::stateChanged, this, nullptr);
                });
    }
    Q_EMIT folderModelChanged();
    Q_EMIT loadingChanged();
}

MailModel *Raven::folderModel() const
{
    return m_folderModel;
}

void Raven::loadMailCollection(const QModelIndex &modelIndex)
{
    if (!modelIndex.isValid()) {
        qDebug() << "hello";
        return;
    }

    m_collectionSelectionModel->select(modelIndex, QItemSelectionModel::ClearAndSelect);
}

bool Raven::loading() const
{
    return m_loading;
}

Akonadi::CollectionFilterProxyModel *Raven::foldersModel() const
{
    return m_foldersModel;
}

Akonadi::Session *Raven::session() const
{
    return m_session;
}