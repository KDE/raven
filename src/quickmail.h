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

#pragma once

// Qt
#include <QObject>

namespace Akonadi {
    class CollectionFilterProxyModel;
}

class KDescendantsProxyModel;

/**
 * Main class of the project and used as Singleton to communicate with
 * QML.
 */
class QuickMail : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    
    Q_PROPERTY(Akonadi::CollectionFilterProxyModel *entityTreeModel READ entityTreeModel NOTIFY entityTreeModelChanged)

    Q_PROPERTY(KDescendantsProxyModel *descendantsProxyModel READ descendantsProxyModel NOTIFY descendantsProxyModelChanged)

public:
    /**
     * Default constructor
     */
    QuickMail(QObject *parent = nullptr);
    ~QuickMail() override = default;
    
    bool loading() const;
    Akonadi::CollectionFilterProxyModel *entityTreeModel() const;

    KDescendantsProxyModel *descendantsProxyModel() const;
    
    Q_INVOKABLE void loadMailCollection(const int &index);
    
private Q_SLOTS:
    void delayedInit();
    
Q_SIGNALS:
    void loadingChanged();
    void entityTreeModelChanged();
    void descendantsProxyModelChanged();
    
private:
    bool m_loading;
    Akonadi::CollectionFilterProxyModel *m_entityTreeModel;
    KDescendantsProxyModel *m_descendantsProxyModel;
};
