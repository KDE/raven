// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QStringList>

#include <MailCore/MailCore.h>

using namespace mailcore;

class Utils : public QObject
{
    Q_OBJECT

public:
    Utils(QObject *parent = nullptr);

    static Utils *self();

    static QStringList roles();
    static QString roleForFolder(const QString &containerFolderPath, const QString &mainPrefix, IMAPFolder *folder);
    static QString roleForFolderViaFlags(IMAPFolder *folder);
    static QString roleForFolderViaPath(std::string containerFolderPath, std::string mainPrefix, IMAPFolder *folder);

    static QString idForFolder(const QString &accountId, const QString &folderPath);

    static QString namespacePrefixOrBlank(IMAPSession *session);

    static std::string toBase58(const std::string &inputStr, size_t len);
};
