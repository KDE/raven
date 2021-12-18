// SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "viewer.h"

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/Session>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/KMime/MessageParts>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <MailTransportAkonadi/ErrorAttribute>

#include <KLocalizedString>

ViewerHelper::ViewerHelper(QObject *parent)
    : QObject(parent)
    , m_session(new Akonadi::Session("QuickMessageViewer-" + QByteArray::number(reinterpret_cast<quintptr>(this)), this))
    , m_message(new KMime::Message())
{}

ViewerHelper::~ViewerHelper()
{}

KMime::Message::Ptr ViewerHelper::message() const
{
    return m_message;
}

void ViewerHelper::setMessage(const KMime::Message::Ptr &message, MimeTreeParser::UpdateMode updateMode)
{
    if (message == m_message) {
        return;
    }
    m_message = message;
    update(updateMode);
    Q_EMIT messageChanged();
}

void ViewerHelper::setMessageItem(const Akonadi::Item &messageItem, MimeTreeParser::UpdateMode updateMode)
{
    if (messageItem == m_messageItem) {
        return;
    }
    if (messageItem.isValid() && messageItem.loadedPayloadParts().contains(Akonadi::MessagePart::Body)) {
        const auto itemsMonitoredEx = m_monitor.itemsMonitoredEx();

        for (const Akonadi::Item::Id monitoredId : itemsMonitoredEx) {
            m_monitor.setItemMonitored(Akonadi::Item(monitoredId), false);
        }
        Q_ASSERT(m_monitor.itemsMonitoredEx().isEmpty());

        m_messageItem = messageItem;
        if (m_messageItem.isValid()) {
            m_monitor.setItemMonitored(m_messageItem, true);
        }
        if (!m_messageItem.hasPayload<KMime::Message::Ptr>()) {
            if (m_messageItem.isValid()) {
                qWarning() << "Payload is not a MessagePtr!";
            }
            return;
        }

        setMessage(m_messageItem.payload<KMime::Message::Ptr>(), updateMode);
    } else {
        Akonadi::ItemFetchJob *job = createFetchJob(messageItem);
        connect(job, &Akonadi::ItemFetchJob::result, [this](KJob *job) {
            m_loading = false;
            Q_EMIT loadingChanged();
            if (job->error()) {
                Q_EMIT displayLoadingError(i18n("Message loading failed: %1.", job->errorText()));
            } else {
                auto fetch = qobject_cast<Akonadi::ItemFetchJob *>(job);
                Q_ASSERT(fetch);
                if (fetch->items().isEmpty()) {
                    Q_EMIT displayLoadingError(i18n("Message not found."));
                } else {
                    setMessageItem(fetch->items().constFirst());
                }
            }
        });
        m_loading = true;
        Q_EMIT loadingChanged();
    }
}

bool ViewerHelper::loading() const
{
    return m_loading;
}

void ViewerHelper::setMessagePath(const QString &messagePath)
{
    if (m_messagePath == messagePath) {
        return;
    }
    m_messagePath = messagePath;
    Q_EMIT messagePathChanged();
}

QString ViewerHelper::messagePath() const
{
    return m_messagePath;
}

void ViewerHelper::update(MimeTreeParser::UpdateMode updateMode)
{

}

Akonadi::ItemFetchJob *ViewerHelper::createFetchJob(const Akonadi::Item &item)
{
    auto job = new Akonadi::ItemFetchJob(item, m_session);
    job->fetchScope().fetchAllAttributes();
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    job->fetchScope().fetchFullPayload(true);
    job->fetchScope().setFetchRelations(true); // needed to know if we have notes or not
    job->fetchScope().fetchAttribute<MailTransport::ErrorAttribute>();
    return job;
}


Akonadi::Item ViewerHelper::messageItem() const
{
    return m_messageItem;
}

QString ViewerHelper::from() const
{
    return m_message->from()->asUnicodeString();
}

QStringList ViewerHelper::to() const
{
    const auto addresses = m_message->to()->addresses();
    QList<QString> ret;
    std::transform(addresses.cbegin(), addresses.cend(),
                   std::back_inserter(ret), [](const QByteArray &addresss) {
                       return QString::fromUtf8(addresss);
                   });
    return ret;
}

QStringList ViewerHelper::cc() const
{
    const auto addresses = m_message->cc()->addresses();
    QList<QString> ret;
    std::transform(addresses.cbegin(), addresses.cend(),
                   std::back_inserter(ret), [](const QByteArray &addresss) {
                       return QString::fromUtf8(addresss);
                   });
    return ret;
}

QString ViewerHelper::sender() const
{
    return m_message->sender()->asUnicodeString();
}

QString ViewerHelper::subject() const
{
    if (m_message->subject()) {
        return m_message->subject()->asUnicodeString();
    } else {
        return i18n("(No subject)");
    }
}

QDateTime ViewerHelper::date() const
{
    if (m_message->date()) {
        return m_message->date()->dateTime();
    } else {
        return QDateTime();
    }
}

QString ViewerHelper::content() const
{
    const auto plain = m_message->mainBodyPart("text/plain");
    if (plain) {
        return plain->decodedText();
    }
    return m_message->textContent()->decodedText();
}

Akonadi::Item::Id ViewerHelper::itemId() const
{
    return m_messageItem.id();
}
