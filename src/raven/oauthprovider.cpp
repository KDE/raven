// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oauthprovider.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

OAuthProviderRegistry& OAuthProviderRegistry::instance() {
    static OAuthProviderRegistry instance;
    return instance;
}

OAuthProviderRegistry::OAuthProviderRegistry() {
    registerProviders();
}

void OAuthProviderRegistry::registerProviders() {
    QFile file(QStringLiteral(":/oauth/oauth_providers.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to load OAuth provider configuration from" << file.fileName();
        qCritical() << "Error:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qCritical() << "Invalid OAuth provider configuration format";
        return;
    }

    QJsonArray providers = doc.object()[QStringLiteral("providers")].toArray();
    for (const QJsonValue &value : providers) {
        QJsonObject obj = value.toObject();

        QStringList domains;
        QJsonArray domainsArray = obj[QStringLiteral("domains")].toArray();
        for (const QJsonValue &domain : domainsArray) {
            domains.append(domain.toString());
        }

        m_providers.append({
            obj[QStringLiteral("id")].toString(),
            obj[QStringLiteral("name")].toString(),
            obj[QStringLiteral("client_id")].toString(),
            obj[QStringLiteral("auth_endpoint")].toString(),
            obj[QStringLiteral("token_endpoint")].toString(),
            obj[QStringLiteral("scope")].toString(),
            domains
        });
    }

    qDebug() << "Loaded" << m_providers.size() << "OAuth providers";
}

QList<OAuthProvider> OAuthProviderRegistry::providers() const {
    return m_providers;
}

std::optional<OAuthProvider> OAuthProviderRegistry::providerById(const QString& id) const {
    for (const auto& provider : m_providers) {
        if (provider.id == id) {
            return provider;
        }
    }
    return std::nullopt;
}

std::optional<OAuthProvider> OAuthProviderRegistry::providerByDomain(const QString& domain) const {
    QString lowerDomain = domain.toLower();
    for (const auto& provider : m_providers) {
        for (const auto& providerDomain : provider.domains) {
            if (lowerDomain == providerDomain || lowerDomain.endsWith(QLatin1Char('.') + providerDomain)) {
                return provider;
            }
        }
    }
    return std::nullopt;
}

std::optional<OAuthProvider> OAuthProviderRegistry::providerByEmail(const QString& email) const {
    int atIndex = email.indexOf(QLatin1Char('@'));
    if (atIndex < 0) {
        return std::nullopt;
    }
    QString domain = email.mid(atIndex + 1);
    return providerByDomain(domain);
}

bool OAuthProviderRegistry::hasProviderForDomain(const QString& domain) const {
    return providerByDomain(domain).has_value();
}
