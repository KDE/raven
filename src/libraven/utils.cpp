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
QString Utils::idForFolder(const QString &accountId, const QString &folderPath)
{
    QString src_str = accountId + QStringLiteral(":") + folderPath;
    auto hash = QCryptographicHash::hash(src_str.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

// message ids are sha256 hashes of information from the message, account id and folder path
QString Utils::idForMessage(const QString &accountId, const QString &folderPath, IMAPMessage *msg)
{
    Array * addresses = new Array();
    addresses->addObjectsFromArray(msg->header()->to());
    addresses->addObjectsFromArray(msg->header()->cc());
    addresses->addObjectsFromArray(msg->header()->bcc());

    Array * emails = new Array();
    for (unsigned int i = 0; i < addresses->count(); ++i) {
        Address * addr = (Address*)addresses->objectAtIndex(i);
        emails->addObject(addr->mailbox());
    }

    emails->sortArray(compareEmails, nullptr);

    String * participants = emails->componentsJoinedByString(MCSTR(""));

    addresses->release();
    emails->release();

    String * messageID = msg->header()->isMessageIDAutoGenerated() ? MCSTR("") : msg->header()->messageID();
    String * subject = msg->header()->subject();

    QString src_str = accountId;
    src_str = src_str.append(QStringLiteral("-"));

    time_t date = msg->header()->date();
    if (date == -1) {
        date = msg->header()->receivedDate();
    }
    if (date > 0) {
        // Use the unix timestamp, not a formatted (localized) date
        src_str += QString::number(date);
    } else {
        // This message has no date information and subject + recipients alone are not enough
        // to build a stable ID across the mailbox.

        // As a fallback, we use the Folder + UID. The UID /will/ change when UIDInvalidity
        // occurs and if the message is moved to another folder, but seeing it as a delete +
        // create (and losing metadata) is better than sync thrashing caused by it thinking
        // many UIDs are all the same message.
        src_str += folderPath + QStringLiteral(":") + QString::number(msg->uid());
    }

    if (subject) {
        src_str += QString::fromUtf8(subject->UTF8Characters());
    }
    src_str += QStringLiteral("-");
    src_str += QString::fromUtf8(participants->UTF8Characters());
    src_str += QStringLiteral("-");
    src_str += QString::fromUtf8(messageID->UTF8Characters());

    auto hash = QCryptographicHash::hash(src_str.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

int Utils::compareEmails(void * a, void * b, void * context) {
    return ((String*)a)->compare((String*)b);
}

QString Utils::namespacePrefixOrBlank(IMAPSession *session)
{
    if (!session->defaultNamespace()->mainPrefix()) {
        return {};
    }
    return QString::fromUtf8(session->defaultNamespace()->mainPrefix()->UTF8Characters());
}

IMAPMessagesRequestKind Utils::messagesRequestKindFor(IndexSet * capabilities, bool heavyOrNeedToComputeIDs)
{
    bool gmail = capabilities->containsIndex(IMAPCapabilityGmail);

    if (heavyOrNeedToComputeIDs) {
        if (gmail) {
            return IMAPMessagesRequestKind(IMAPMessagesRequestKindHeaders | IMAPMessagesRequestKindInternalDate | IMAPMessagesRequestKindFlags | IMAPMessagesRequestKindGmailLabels | IMAPMessagesRequestKindGmailThreadID | IMAPMessagesRequestKindGmailMessageID);
        }
        return IMAPMessagesRequestKind(IMAPMessagesRequestKindHeaders | IMAPMessagesRequestKindInternalDate | IMAPMessagesRequestKindFlags);
    }

    if (gmail) {
        return IMAPMessagesRequestKind(IMAPMessagesRequestKindFlags | IMAPMessagesRequestKindGmailLabels);
    }
    return IMAPMessagesRequestKind(IMAPMessagesRequestKindFlags);
}

QString Utils::qmarks(size_t count)
{
    if (count == 0) {
        return QString();
    }
    QString qmarks = QStringLiteral("?");
    for (unsigned int i = 1; i < count; i ++) {
        qmarks += QStringLiteral(",?");
    }
    return qmarks;
}

MessageAttributes Utils::messageAttributesForMessage(IMAPMessage * msg) {
    auto m = MessageAttributes{};
    m.uid = msg->uid();
    m.unread = bool(!(msg->flags() & MessageFlagSeen));
    m.starred = bool(msg->flags() & MessageFlagFlagged);
    m.labels = {};

    Array * labels = msg->gmailLabels();
    bool draftLabelPresent = false;
    bool trashSpamLabelPresent = false;
    if (labels != nullptr) {
        for (unsigned int ii = 0; ii < labels->count(); ii ++) {
            QString str = QString::fromUtf8(((String *)labels->objectAtIndex(ii))->UTF8Characters());
            // Gmail exposes Trash and Spam as folders and labels. We want them
            // to be folders so we ignore their presence as labels.
            if ((str == QStringLiteral("\\Trash")) || (str == QStringLiteral("\\Spam"))) {
                trashSpamLabelPresent = true;
                continue;
            }
            if ((str == QStringLiteral("\\Draft"))) {
                draftLabelPresent = true;
            }
            m.labels.push_back(str);
        }
        std::sort(m.labels.begin(), m.labels.end());
    }

    m.draft = (bool(msg->flags() & MessageFlagDraft) || draftLabelPresent) && !trashSpamLabelPresent;

    return m;
}

bool Utils::messageAttributesMatch(MessageAttributes a, MessageAttributes b) {
    return a.unread == b.unread && a.starred == b.starred && a.uid == b.uid && a.labels == b.labels;
}

bool Utils::execWithLog(QSqlQuery &query, const std::string &description)
{
    if (!query.exec()) {
        qWarning().nospace() << "Query error: " << QString::fromStdString(description) << ": " << query.lastError();
        return false;
    }
    return true;
}
