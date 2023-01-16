// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../libraven/constants.h"
#include "../libraven/models/folder.h"

#include <QObject>
#include <QThread>
#include <QList>

#include <MailCore/MailCore.h>

using namespace mailcore;

class Raven : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedFolderName READ selectedFolderName WRITE setSelectedFolderName NOTIFY selectedFolderNameChanged)

public:
    Raven(QObject *parent = nullptr);
    
    static Raven *self();

    QString selectedFolderName() const;
    void setSelectedFolderName(QString folderName);

Q_SIGNALS:
    void selectedFolderNameChanged();

private:
    QString m_selectedFolderName;
};
