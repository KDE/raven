// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define RAVEN_DATA_LOCATION QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/raven")
#define RAVEN_CONFIG_LOCATION QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/raven")

#include <QHash>
#include <QMap>

static const QString JOB_TABLE = QStringLiteral("job");
static const QString MESSAGE_TABLE = QStringLiteral("message");
static const QString THREAD_TABLE = QStringLiteral("thread");
static const QString FOLDER_TABLE = QStringLiteral("folder");
static const QString LABEL_TABLE = QStringLiteral("label");
static const QString THREAD_REFERENCE_TABLE = QStringLiteral("thread_reference");
static const QString THREAD_FOLDER_TABLE = QStringLiteral("thread_folder");
static const QString MESSAGE_BODY_TABLE = QStringLiteral("message_body");
static const QString FILE_TABLE = QStringLiteral("file");

static const QString DBUS_SERVICE = QStringLiteral("org.kde.raven.daemon");
static const QString DBUS_PATH = QStringLiteral("/org/kde/raven/daemon");

static QMap<std::string, std::string> COMMON_FOLDER_NAMES = {
    {"gel\xc3\xb6scht", "trash"},
    {"papierkorb", "trash"},
    {"\xd0\x9a\xd0\xbe\xd1\x80\xd0\xb7\xd0\xb8\xd0\xbd\xd0\xb0", "trash"},
    {"[imap]/trash", "trash"},
    {"papelera", "trash"},
    {"borradores", "trash"},
    {"[imap]/\xd0\x9a\xd0\xbe\xd1\x80", "trash"},
    {"\xd0\xb7\xd0\xb8\xd0\xbd\xd0\xb0", "trash"},
    {"deleted items", "trash"},
    {"\xd0\xa1\xd0\xbc\xd1\x96\xd1\x82\xd1\x82\xd1\x8f", "trash"},
    {"papierkorb/trash", "trash"},
    {"gel\xc3\xb6schte elemente", "trash"},
    {"deleted messages", "trash"},
    {"[gmail]/trash", "trash"},
    {"trash", "trash"},
    {"удаленные", "trash"},
    {"kosz", "trash"},
    {"yдалённые", "trash"},

    {"roskaposti", "spam"},
    {"skr\xc3\xa4ppost", "spam"},
    {"spamverdacht", "spam"},
    {"spam", "spam"},
    {"[gmail]/spam", "spam"},
    {"[imap]/spam", "spam"},
    {"\xe5\x9e\x83\xe5\x9c\xbe\xe9\x82\xae\xe4\xbb\xb6", "spam"},
    {"junk", "spam"},
    {"junk mail", "spam"},
    {"junk e-mail", "spam"},
    {"junk email", "spam"},
    {"bulk mail", "spam"},
    {"спам", "spam"},

    {"inbox", "inbox"},

    {"dateneintrag", "archive"},
    {"archivio", "archive"},
    {"archive", "archive"},

    {"postausgang", "sent"},
    {"sent", "sent"},
    {"[gmail]/sent mail", "sent"},
    {"\xeb\xb3\xb4\xeb\x82\xbc\xed\x8e\xb8\xec\xa7\x80\xed\x95\xa8", "sent"},
    {"elementos enviados", "sent"},
    {"sent items", "sent"},
    {"sent messages", "sent"},
    {"odeslan\xc3\xa9", "sent"},
    {"sent-mail", "sent"},
    {"ko\xc5\xa1", "sent"},
    {"sentmail", "sent"},
    {"papierkorb", "sent"},
    {"gesendet", "sent"},
    {"ko\xc5\xa1/sent items", "sent"},
    {"gesendete elemente", "sent"},
    {"отправленные", "sent"},
    {"sentbox", "sent"},
    {"wys&AUI-ane", "sent"},

    {"drafts", "drafts"},
    {"draft", "drafts"},
    {"brouillons", "drafts"},
    {"черновики", "drafts"},
    {"draftbox", "drafts"},
    {"robocze", "drafts"},
};
