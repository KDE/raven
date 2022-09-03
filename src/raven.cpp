// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"

#include "mailmodel.h"
#include "accounts/ispdb/ispdb.h"

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

Raven *Raven::self()
{
    static Raven *instance = new Raven();
    return instance;
}
