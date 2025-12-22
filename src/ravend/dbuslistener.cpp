// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbuslistener.h"

#include <QDBusConnection>

DBusListener::DBusListener(QObject *parent)
    : QObject{parent}
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), this);
}
