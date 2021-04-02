// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace Akonadi {
    class CollectionFilterProxyModel;
    class EntityMimeTypeFilterModel;
    class Session;
}

class KDescendantsProxyModel;
class QItemSelectionModel;

class MailModel;

/**
 * Main class of the project and used as Singleton to communicate with
 * QML.
 */
class QuickMail : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    
    Q_PROPERTY(Akonadi::CollectionFilterProxyModel *entityTreeModel READ entityTreeModel NOTIFY entityTreeModelChanged)
    Q_PROPERTY(MailModel *folderModel READ folderModel NOTIFY folderModelChanged)

    Q_PROPERTY(KDescendantsProxyModel *descendantsProxyModel READ descendantsProxyModel NOTIFY descendantsProxyModelChanged)

public:
    static QuickMail &instance();

    bool loading() const;
    Akonadi::CollectionFilterProxyModel *entityTreeModel() const;
    KDescendantsProxyModel *descendantsProxyModel() const;
    MailModel *folderModel() const;
    Akonadi::Session *session() const;

    Q_INVOKABLE void loadMailCollection(const int &index);

private Q_SLOTS:
    void delayedInit();

Q_SIGNALS:
    void loadingChanged();
    void entityTreeModelChanged();
    void descendantsProxyModelChanged();
    void folderModelChanged();
    
private:
    QuickMail(QObject *parent = nullptr);
    ~QuickMail() override = default;
    bool m_loading;
    Akonadi::CollectionFilterProxyModel *m_entityTreeModel;
    Akonadi::Session *m_session;
    KDescendantsProxyModel *m_descendantsProxyModel;

    //folders
    QItemSelectionModel *m_collectionSelectionModel;
    MailModel *m_folderModel;
};
