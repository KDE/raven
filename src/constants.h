// SPDX-FileCopyrightText: 2017 Ben Gotow <ben@foundry376.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define AS_MCSTR(X)         mailcore::String::uniquedStringWithUTF8Characters(X.c_str())
#define AS_WIDE_MCSTR(X)    mailcore::String::stringWithCharacters(X.c_str())

#define RAVEN_DATA_LOCATION QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/raven")
#define RAVEN_CONFIG_LOCATION QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/raven")

#include <QHash>
#include <QMap>

#include <MailCore/MailCore.h>

#include <libetpan/mailsmtp_types.h>

using namespace mailcore;

static const QString JOBS_TABLE = QStringLiteral("Jobs");
static const QString MESSAGES_TABLE = QStringLiteral("Messages");
static const QString THREADS_TABLE = QStringLiteral("Threads");
static const QString FOLDERS_TABLE = QStringLiteral("Folders");
static const QString LABELS_TABLE = QStringLiteral("Labels");

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

    {"Mailspring/Snoozed", "snoozed"},
    {"Mailspring.Snoozed", "snoozed"},
};

static QHash<int, std::string> LibEtPanCodeToTypeMap = {
    {MAILSMTP_NO_ERROR, "MAILSMTP_NO_ERROR"},
    {MAILSMTP_ERROR_UNEXPECTED_CODE, "MAILSMTP_ERROR_UNEXPECTED_CODE"},
    {MAILSMTP_ERROR_SERVICE_NOT_AVAILABLE, "MAILSMTP_ERROR_SERVICE_NOT_AVAILABLE"},
    {MAILSMTP_ERROR_STREAM, "MAILSMTP_ERROR_STREAM"},
    {MAILSMTP_ERROR_HOSTNAME, "MAILSMTP_ERROR_HOSTNAME"},
    {MAILSMTP_ERROR_NOT_IMPLEMENTED, "MAILSMTP_ERROR_NOT_IMPLEMENTED"},
    {MAILSMTP_ERROR_ACTION_NOT_TAKEN, "MAILSMTP_ERROR_ACTION_NOT_TAKEN"},
    {MAILSMTP_ERROR_EXCEED_STORAGE_ALLOCATION, "MAILSMTP_ERROR_EXCEED_STORAGE_ALLOCATION"},
    {MAILSMTP_ERROR_IN_PROCESSING, "MAILSMTP_ERROR_IN_PROCESSING"},
    {MAILSMTP_ERROR_INSUFFICIENT_SYSTEM_STORAGE, "MAILSMTP_ERROR_INSUFFICIENT_SYSTEM_STORAGE"},
    {MAILSMTP_ERROR_MAILBOX_UNAVAILABLE, "MAILSMTP_ERROR_MAILBOX_UNAVAILABLE"},
    {MAILSMTP_ERROR_MAILBOX_NAME_NOT_ALLOWED, "MAILSMTP_ERROR_MAILBOX_NAME_NOT_ALLOWED"},
    {MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND, "MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND"},
    {MAILSMTP_ERROR_USER_NOT_LOCAL, "MAILSMTP_ERROR_USER_NOT_LOCAL"},
    {MAILSMTP_ERROR_TRANSACTION_FAILED, "MAILSMTP_ERROR_TRANSACTION_FAILED"},
    {MAILSMTP_ERROR_MEMORY, "MAILSMTP_ERROR_MEMORY"},
    {MAILSMTP_ERROR_AUTH_NOT_SUPPORTED, "MAILSMTP_ERROR_AUTH_NOT_SUPPORTED"},
    {MAILSMTP_ERROR_AUTH_LOGIN, "MAILSMTP_ERROR_AUTH_LOGIN"},
    {MAILSMTP_ERROR_AUTH_REQUIRED, "MAILSMTP_ERROR_AUTH_REQUIRED"},
    {MAILSMTP_ERROR_AUTH_TOO_WEAK, "MAILSMTP_ERROR_AUTH_TOO_WEAK"},
    {MAILSMTP_ERROR_AUTH_TRANSITION_NEEDED, "MAILSMTP_ERROR_AUTH_TRANSITION_NEEDED"},
    {MAILSMTP_ERROR_AUTH_TEMPORARY_FAILTURE, "MAILSMTP_ERROR_AUTH_TEMPORARY_FAILTURE"},
    {MAILSMTP_ERROR_AUTH_ENCRYPTION_REQUIRED, "MAILSMTP_ERROR_AUTH_ENCRYPTION_REQUIRED"},
    {MAILSMTP_ERROR_STARTTLS_TEMPORARY_FAILURE, "MAILSMTP_ERROR_STARTTLS_TEMPORARY_FAILURE"},
    {MAILSMTP_ERROR_STARTTLS_NOT_SUPPORTED, "MAILSMTP_ERROR_STARTTLS_NOT_SUPPORTED"},
    {MAILSMTP_ERROR_CONNECTION_REFUSED, "MAILSMTP_ERROR_CONNECTION_REFUSED"},
    {MAILSMTP_ERROR_AUTH_AUTHENTICATION_FAILED, "MAILSMTP_ERROR_AUTH_AUTHENTICATION_FAILED"},
    {MAILSMTP_ERROR_SSL, "MAILSMTP_ERROR_SSL"},
};

static QHash<ErrorCode, std::string> ErrorCodeToTypeMap = {
    {ErrorNone, "ErrorNone"}, // 0
    {ErrorRename, "ErrorRename"},
    {ErrorDelete, "ErrorDelete"},
    {ErrorCreate, "ErrorCreate"},
    {ErrorSubscribe, "ErrorSubscribe"},
    {ErrorAppend, "ErrorAppend"},
    {ErrorCopy, "ErrorCopy"},
    {ErrorExpunge, "ErrorExpunge"},
    {ErrorFetch, "ErrorFetch"},
    {ErrorIdle, "ErrorIdle"}, // 20
    {ErrorIdentity, "ErrorIdentity"},
    {ErrorNamespace, "ErrorNamespace"},
    {ErrorStore, "ErrorStore"},
    {ErrorCapability, "ErrorCapability"},
    {ErrorSendMessageIllegalAttachment, "ErrorSendMessageIllegalAttachment"},
    {ErrorStorageLimit, "ErrorStorageLimit"},
    {ErrorSendMessageNotAllowed, "ErrorSendMessageNotAllowed"},
    {ErrorSendMessage, "ErrorSendMessage"}, // 30
    {ErrorFetchMessageList, "ErrorFetchMessageList"},
    {ErrorDeleteMessage, "ErrorDeleteMessage"},
    {ErrorFile, "ErrorFile"},
    {ErrorCompression, "ErrorCompression"},
    {ErrorNoSender, "ErrorNoSender"},
    {ErrorNoRecipient, "ErrorNoRecipient"},
    {ErrorNoop, "ErrorNoop"},
    {ErrorServerDate, "ErrorServerDate"},
    {ErrorCustomCommand, "ErrorCustomCommand"},
    {ErrorYahooSendMessageSpamSuspected, "ErrorYahooSendMessageSpamSuspected"},
    {ErrorYahooSendMessageDailyLimitExceeded, "ErrorYahooSendMessageDailyLimitExceeded"},
    {ErrorOutlookLoginViaWebBrowser, "ErrorOutlookLoginViaWebBrowser"},
    {ErrorTiscaliSimplePassword, "ErrorTiscaliSimplePassword"},
    {ErrorConnection, "ErrorConnection"},
    {ErrorInvalidAccount, "ErrorInvalidAccount"},
    {ErrorTLSNotAvailable, "ErrorTLSNotAvailable"},
    {ErrorParse, "ErrorParse"},
    {ErrorCertificate, "ErrorCertificate"},
    {ErrorAuthentication, "ErrorAuthentication"},
    {ErrorGmailIMAPNotEnabled, "ErrorGmailIMAPNotEnabled"},
    {ErrorGmailExceededBandwidthLimit, "ErrorGmailExceededBandwidthLimit"},
    {ErrorGmailTooManySimultaneousConnections, "ErrorGmailTooManySimultaneousConnections"},
    {ErrorMobileMeMoved, "ErrorMobileMeMoved"},
    {ErrorYahooUnavailable, "ErrorYahooUnavailable"},
    {ErrorNonExistantFolder, "ErrorNonExistantFolder"},
    {ErrorStartTLSNotAvailable, "ErrorStartTLSNotAvailable"},
    {ErrorGmailApplicationSpecificPasswordRequired, "ErrorGmailApplicationSpecificPasswordRequired"},
    {ErrorOutlookLoginViaWebBrowser, "ErrorOutlookLoginViaWebBrowser"},
    {ErrorNeedsConnectToWebmail, "ErrorNeedsConnectToWebmail"},
    {ErrorNoValidServerFound, "ErrorNoValidServerFound"},
    {ErrorAuthenticationRequired, "ErrorAuthenticationRequired"},
};
