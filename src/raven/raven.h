// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../libraven/constants.h"

#include <QObject>
#include <QThread>
#include <QList>

#include <MailCore/MailCore.h>

using namespace mailcore;

class Raven : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedFolderName READ selectedFolderName NOTIFY selectedFolderNameChanged)

public:
    Raven(QObject *parent = nullptr);
    
    static Raven *self();

    QString selectedFolderName() const;

Q_SIGNALS:
    void selectedFolderNameChanged();
};
