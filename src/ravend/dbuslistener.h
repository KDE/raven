// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class DBusListener : public QObject
{
    Q_OBJECT
    
public:
    DBusListener(QObject *parent = nullptr);
};
