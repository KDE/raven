// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include "constants.h"

#include <QCryptographicHash>
#include <QDebug>

Utils::Utils(QObject *parent)
    : QObject{parent}
{
}

Utils *Utils::self()
{
    Utils *instance = new Utils();
    return instance;
}

QStringList Utils::roles()
{
    return {
        QStringLiteral("all"),
        QStringLiteral("sent"),
        QStringLiteral("drafts"),
        QStringLiteral("spam"),
        QStringLiteral("important"),
        QStringLiteral("starred"),
        QStringLiteral("archive"),
        QStringLiteral("inbox"),
        QStringLiteral("trash")
    };
}

QString Utils::roleForFolder(const QString &containerFolderPath, const QString &mainPrefix, IMAPFolder *folder)
{
    QString role = roleForFolderViaFlags(folder);
    if (role.isEmpty()) {
        role = roleForFolderViaPath(containerFolderPath.toStdString(), mainPrefix.toStdString(), folder);
    }
    return role;
}

QString Utils::roleForFolderViaFlags(IMAPFolder *folder)
{
    IMAPFolderFlag flags = folder->flags();

    if (flags & IMAPFolderFlagAll) {
        return QStringLiteral("all");
    }
    if (flags & IMAPFolderFlagSentMail) {
        return QStringLiteral("sent");
    }
    if (flags & IMAPFolderFlagDrafts) {
        return QStringLiteral("drafts");
    }
    if (flags & IMAPFolderFlagJunk) {
        return QStringLiteral("spam");
    }
    if (flags & IMAPFolderFlagSpam) {
        return QStringLiteral("spam");
    }
    if (flags & IMAPFolderFlagImportant) {
        return QStringLiteral("important");
    }
    if (flags & IMAPFolderFlagStarred) {
        return QStringLiteral("starred");
    }
    if (flags & IMAPFolderFlagInbox) {
        return QStringLiteral("inbox");
    }
    if (flags & IMAPFolderFlagTrash) {
        return QStringLiteral("trash");
    }
    return {};
}

QString Utils::roleForFolderViaPath(std::string containerFolderPath, std::string mainPrefix, IMAPFolder *folder)
{
    std::string delimiter = std::string{folder->delimiter()};
    std::string path = std::string(folder->path()->UTF8Characters());

    // Strip the namespace prefix if it's present
    if ((mainPrefix.size() > 0) && (path.size() > mainPrefix.size()) && (path.substr(0, mainPrefix.size()) == mainPrefix)) {
        path = path.substr(mainPrefix.size());
    }

    // Strip the delimiter if the delimiter is the first character after stripping prefix
    if (path.size() > 1 && path.substr(0, 1) == delimiter) {
        path = path.substr(1);
    }

    // Lowercase the path
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    std::transform(containerFolderPath.begin(), containerFolderPath.end(), containerFolderPath.begin(), ::tolower);

    // Match against a lookup table of common names
    // [Gmail]/Spam => [gmail]/spam => spam
    if (COMMON_FOLDER_NAMES.find(path) != COMMON_FOLDER_NAMES.end()) {
        return QString::fromStdString(COMMON_FOLDER_NAMES[path]);
    }

    return QString();
}

// folder ids are sha256 hashes of the account id and folder path
QString Utils::idForFolder(const QString &accountId, const QString &folderPath) {
    QString src_str = accountId + QStringLiteral(":") + folderPath;
    auto hash = QCryptographicHash::hash(src_str.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

QString Utils::namespacePrefixOrBlank(IMAPSession *session) {
    if (!session->defaultNamespace()->mainPrefix()) {
        return {};
    }
    return QString::fromUtf8(session->defaultNamespace()->mainPrefix()->UTF8Characters());
}
