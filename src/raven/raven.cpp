// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "raven.h"

#include <QDebug>

Raven::Raven(QObject *parent)
    : QObject(parent)
{
}


Raven *Raven::self()
{
    static Raven *instance = new Raven();
    return instance;
}

QString Raven::selectedFolderName() const
{
    return m_selectedFolderName;
}

void Raven::setSelectedFolderName(QString folderName)
{
    if (folderName != m_selectedFolderName) {
        m_selectedFolderName = folderName;
        Q_EMIT selectedFolderNameChanged();
    }
}
