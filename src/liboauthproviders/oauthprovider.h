// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <optional>

/**
 * OAuth2 provider configuration.
 * Contains all the information needed to perform OAuth2 authentication
 * for a specific email provider.
 */
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

/**
 * OAuth2 provider registry.
 * Provides access to configured OAuth2 providers.
 */
class OAuthProviderRegistry {
public:
    /**
     * Get the singleton instance of the registry.
     */
    static OAuthProviderRegistry& instance();

    /**
     * Get all registered OAuth providers.
     */
    QList<OAuthProvider> providers() const;

    /**
     * Find a provider by its ID.
     * @param id Provider ID (e.g., "gmail", "outlook", "yahoo")
     * @return The provider if found, std::nullopt otherwise
     */
    std::optional<OAuthProvider> providerById(const QString& id) const;

    /**
     * Find a provider by email domain.
     * @param domain Email domain (e.g., "gmail.com", "outlook.com")
     * @return The provider if found, std::nullopt otherwise
     */
    std::optional<OAuthProvider> providerByDomain(const QString& domain) const;

    /**
     * Find a provider by email address.
     * Extracts the domain from the email and looks up the provider.
     * @param email Full email address
     * @return The provider if found, std::nullopt otherwise
     */
    std::optional<OAuthProvider> providerByEmail(const QString& email) const;

    /**
     * Check if a domain has an OAuth provider.
     * @param domain Email domain
     * @return true if an OAuth provider exists for this domain
     */
    bool hasProviderForDomain(const QString& domain) const;

private:
    OAuthProviderRegistry();
    void registerProviders();

    QList<OAuthProvider> m_providers;
};
