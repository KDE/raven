// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QList>
#include <QSqlQuery>

struct MessageAttributes {
    uint32_t uid;
    bool unread;
    bool starred;
    bool draft;
    QList<QString> labels;
};

class Utils : public QObject
{
    Q_OBJECT

public:
    Utils(QObject *parent = nullptr);

    static Utils *self();

    // executes query and logs errors
    static bool execWithLog(QSqlQuery &query, const std::string &description);

    template<typename T>
    static QList<QList<T>> chunksOfVector(QList<T> & v, size_t chunkSize) {
        QList<QList<T>> results{};

        while (v.size() > 0) {
            auto from = v.begin();
            auto to = v.size() > chunkSize ? from + chunkSize : v.end();

            results.push_back(QList<T>{std::make_move_iterator(from), std::make_move_iterator(to)});
            v.erase(from, to);
        }
        return results;
    }
};
