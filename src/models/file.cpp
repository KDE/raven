// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file.h"
#include "constants.h"

#include <KLocalizedString>

File::File(QObject *parent, Message *msg, mailcore::Attachment *a)
    : QObject{parent}
    , m_id{} // TODO AUTOGENERATED????
    , m_messageId{msg->id()}
    , m_partId{QString::fromUtf8(a->partID()->UTF8Characters())}
{
    if (a->isInlineAttachment() && a->contentID()) {
        m_contentId = QString::fromUtf8(a->contentID()->UTF8Characters());
    }
    if (a->mimeType()) {
        m_contentType = QString::fromUtf8(a->mimeType()->UTF8Characters());
    }

    QString name;

    if (a->filename()) {
        m_filename = QString::fromUtf8(a->filename()->UTF8Characters());
    }

    // default names for attachment if no file name is specified
    // TODO: i18nc properly
    if (m_filename.isEmpty()) {
        m_filename = QStringLiteral("Unnamed Attachment");

        if (m_contentType == QStringLiteral("text/calendar")) {
            m_filename = QStringLiteral("Event.ics");
        }
        if (m_contentType == QStringLiteral("image/png") || m_contentType == QStringLiteral("image/x-png")) {
            m_filename = QStringLiteral("Unnamed Image.png");
        }
        if (m_contentType == QStringLiteral("image/jpg")) {
            m_filename = QStringLiteral("Unnamed Image.jpg");
        }
        if (m_contentType == QStringLiteral("image/jpeg")) {
            m_filename = QStringLiteral("Unnamed Image.gif");
        }
        if (m_contentType == QStringLiteral("message/delivery-status")) {
            m_filename = QStringLiteral("Delivery Status.txt");
        }
        if (m_contentType == QStringLiteral("message/feedback-report")) {
            m_filename = QStringLiteral("Feedback Report.txt");
        }
    }

    m_size = a->data()->length();
}

File::File(QObject *parent, const QJsonObject &json)
    : QObject{parent}
    , m_id{json[QStringLiteral("id")].toString()}
    , m_messageId{json[QStringLiteral("messageId")].toString()}
    , m_filename{json[QStringLiteral("fileName")].toString()}
    , m_partId{json[QStringLiteral("partId")].toString()}
    , m_contentId{json[QStringLiteral("contentId")].toString()}
    , m_contentType{json[QStringLiteral("contentType")].toString()}
    , m_size{json[QStringLiteral("size")].toInt()}
{
}

File::File(QObject *parent, const QSqlQuery &query)
    : QObject{parent}
    , m_id{query.value(QStringLiteral("id")).toString()}
{

}

void File::saveToDb(QSqlDatabase &db) const
{

}

void File::deleteFromDb(QSqlDatabase &db) const
{
    QSqlQuery query{db};
    query.prepare(QStringLiteral("DELETE FROM ") + FILE_TABLE + QStringLiteral(" WHERE id = ") + m_id);
    query.exec();
}

QJsonObject File::toJson()
{
    QJsonObject object;
    object[QStringLiteral("id")] = m_id;
    object[QStringLiteral("messageId")] = m_messageId;
    object[QStringLiteral("fileName")] = m_filename;
    object[QStringLiteral("partId")] = m_partId;
    object[QStringLiteral("contentId")] = m_contentId;
    object[QStringLiteral("contentType")] = m_contentType;
    object[QStringLiteral("size")] = m_size;
    return object;
}

QString File::messageId() const
{
    return m_messageId;
}


QString File::fileName() const
{
    return m_filename;
}

QString File::partId() const
{
    return m_partId;
}

QString File::contentId() const
{
    return m_contentId;
}

void File::setContentId(const QString &contentId)
{
    m_contentId = contentId;
}

QString File::contentType() const
{
    return m_contentType;
}

int File::size() const
{
    return m_size;
}