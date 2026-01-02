// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include "constants.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDateTime>
#include <QSqlError>

Utils::Utils(QObject *parent)
    : QObject{parent}
{
}

Utils *Utils::self()
{
    Utils *instance = new Utils();
    return instance;
}

bool Utils::execWithLog(QSqlQuery &query, const std::string &description)
{
    if (!query.exec()) {
        qWarning().nospace() << "Query error: " << QString::fromStdString(description) << ": " << query.lastError() << " " << query.lastQuery();
        return false;
    }
    return true;
}
