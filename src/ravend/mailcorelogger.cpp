#include "mailcorelogger.h"

#include <QDebug>

void MailCoreLogger::log(void *sender, mailcore::ConnectionLogType logType, mailcore::Data *buffer)
{
    QString logTypeString = QStringLiteral("unknown");
    switch (logType) {
        case mailcore::ConnectionLogTypeReceived:
            logTypeString = QStringLiteral("recv");
            break;

        case mailcore::ConnectionLogTypeSent:
            logTypeString = QStringLiteral("sent");
            break;

        case mailcore::ConnectionLogTypeSentPrivate:
            logTypeString = QStringLiteral("sent-private");
            break;

        case mailcore::ConnectionLogTypeErrorParse:
            logTypeString = QStringLiteral("error-parse");
            break;

        case mailcore::ConnectionLogTypeErrorReceived:
            logTypeString = QStringLiteral("error-received");
            break;

        case mailcore::ConnectionLogTypeErrorSent:
            logTypeString = QStringLiteral("error-sent");
            break;

        default:
            break;
    }
    QString msg = buffer != nullptr ? QString::fromUtf8(buffer->stringWithCharset("UTF-8")->UTF8Characters()) : QString{};
    qDebug() << msg << logTypeString;
}
