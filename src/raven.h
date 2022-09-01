// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class Raven : public QObject
{
    Q_OBJECT

public:
    Raven(QObject *parent = nullptr);
    ~Raven() override = default;
};

Q_GLOBAL_STATIC(Raven, raven)
