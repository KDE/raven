// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "mailaccounts.h"

#include <Akonadi/AgentInstanceModel>
#include <Akonadi/AgentFilterProxyModel>
#include <KMime/Message>

MailAccounts::MailAccounts(QObject *parent)
    : QObject{parent}
{
}

Akonadi::AgentFilterProxyModel *MailAccounts::runningMailAgents()
{
    if (m_runningMailAgents) {
        return m_runningMailAgents;
    }
    
    auto agentInstanceModel = new Akonadi::AgentInstanceModel(this);
    m_runningMailAgents = new Akonadi::AgentFilterProxyModel(this);
    
    m_runningMailAgents->addMimeTypeFilter(KMime::Message::mimeType());
    m_runningMailAgents->setSourceModel(agentInstanceModel);
    m_runningMailAgents->addCapabilityFilter(QStringLiteral("Resource"));
    m_runningMailAgents->excludeCapabilities(QStringLiteral("MailTransport"));
    m_runningMailAgents->excludeCapabilities(QStringLiteral("Notes"));
    return m_runningMailAgents;
}
