// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class Raven : public QObject
{
    Q_OBJECT

public:
    Raven(QObject *parent = nullptr);
    ~Raven() override = default;
    
    static Raven *self();
};

Q_GLOBAL_STATIC(Raven, raven)
