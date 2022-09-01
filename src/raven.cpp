// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"

#include "mailmodel.h"

#include <QtCore/QItemSelectionModel>
#include <QTimer>
#include <QApplication>

// Akonadi
#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>
#include <Akonadi/Session>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ServerManager>
#include <Akonadi/SelectionProxyModel>

#include <MailCommon/FolderCollectionMonitor>

#include <KMime/Message>

#include <KDescendantsProxyModel>

#include <KItemModels/KDescendantsProxyModel>

Raven::Raven(QObject *parent)
    : QObject(parent)
{
}
