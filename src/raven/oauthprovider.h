// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <optional>

struct OAuthProvider {
    QString id;              // Unique identifier (e.g., "gmail", "outlook", "yahoo")
    QString name;            // Display name (e.g., "Google", "Microsoft", "Yahoo")
    QString clientId;        // OAuth2 client ID
    QString authEndpoint;    // Authorization endpoint URL
    QString tokenEndpoint;   // Token exchange endpoint URL
    QString scope;           // OAuth2 scope for mail access
    QStringList domains;     // Email domains this provider handles (e.g., "gmail.com", "googlemail.com")

    bool isValid() const {
        return !id.isEmpty() && !clientId.isEmpty() && !authEndpoint.isEmpty() && !tokenEndpoint.isEmpty();
    }
};

class OAuthProviderRegistry {
public:
    static OAuthProviderRegistry& instance();

    QList<OAuthProvider> providers() const;

    std::optional<OAuthProvider> providerById(const QString& id) const;
    std::optional<OAuthProvider> providerByDomain(const QString& domain) const;
    std::optional<OAuthProvider> providerByEmail(const QString& email) const;

    bool hasProviderForDomain(const QString& domain) const;

private:
    OAuthProviderRegistry();
    void registerProviders();

    QList<OAuthProvider> m_providers;
};
