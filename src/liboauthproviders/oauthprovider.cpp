// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oauthprovider.h"

OAuthProviderRegistry& OAuthProviderRegistry::instance() {
    static OAuthProviderRegistry instance;
    return instance;
}

OAuthProviderRegistry::OAuthProviderRegistry() {
    registerProviders();
}

void OAuthProviderRegistry::registerProviders() {
    // Google/Gmail
    // Docs: https://developers.google.com/identity/protocols/oauth2/native-app
    m_providers.append({
        QStringLiteral("gmail"),
        QStringLiteral("Google"),
        QStringLiteral("1095027536469-6li34du2en7ht2su6955pht5najnebnq.apps.googleusercontent.com"), // Raven Mail
        QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"),
        QStringLiteral("https://oauth2.googleapis.com/token"),
        QStringLiteral("https://mail.google.com/"),
        {QStringLiteral("gmail.com"), QStringLiteral("googlemail.com")}
    });

    // Microsoft Outlook/Hotmail
    // Docs: https://learn.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-auth-code-flow
    // Note: Microsoft requires app registration in Azure AD
    m_providers.append({
        QStringLiteral("outlook"),
        QStringLiteral("Microsoft"),
        QStringLiteral("d612d734-9396-4d58-887d-503598590f53"), // Raven Mail App ID - Microsoft Entra
        QStringLiteral("https://login.microsoftonline.com/common/oauth2/v2.0/authorize"),
        QStringLiteral("https://login.microsoftonline.com/common/oauth2/v2.0/token"),
        QStringLiteral("https://outlook.office.com/IMAP.AccessAsUser.All https://outlook.office.com/SMTP.Send offline_access"),
        {
            // Primary domains
            QStringLiteral("hotmail.com"), QStringLiteral("live.com"), QStringLiteral("msn.com"),
            QStringLiteral("outlook.com"), QStringLiteral("windowslive.com"),
            // Outlook regional domains
            QStringLiteral("outlook.at"), QStringLiteral("outlook.be"), QStringLiteral("outlook.cl"),
            QStringLiteral("outlook.cz"), QStringLiteral("outlook.de"), QStringLiteral("outlook.dk"),
            QStringLiteral("outlook.es"), QStringLiteral("outlook.fr"), QStringLiteral("outlook.hu"),
            QStringLiteral("outlook.ie"), QStringLiteral("outlook.in"), QStringLiteral("outlook.it"),
            QStringLiteral("outlook.jp"), QStringLiteral("outlook.kr"), QStringLiteral("outlook.lv"),
            QStringLiteral("outlook.my"), QStringLiteral("outlook.ph"), QStringLiteral("outlook.pt"),
            QStringLiteral("outlook.sa"), QStringLiteral("outlook.sg"), QStringLiteral("outlook.sk"),
            QStringLiteral("outlook.co.id"), QStringLiteral("outlook.co.il"), QStringLiteral("outlook.co.th"),
            QStringLiteral("outlook.com.ar"), QStringLiteral("outlook.com.au"), QStringLiteral("outlook.com.br"),
            QStringLiteral("outlook.com.gr"), QStringLiteral("outlook.com.tr"), QStringLiteral("outlook.com.vn"),
            // Hotmail regional domains
            QStringLiteral("hotmail.be"), QStringLiteral("hotmail.ca"), QStringLiteral("hotmail.cl"),
            QStringLiteral("hotmail.cz"), QStringLiteral("hotmail.de"), QStringLiteral("hotmail.dk"),
            QStringLiteral("hotmail.es"), QStringLiteral("hotmail.fi"), QStringLiteral("hotmail.fr"),
            QStringLiteral("hotmail.gr"), QStringLiteral("hotmail.hu"), QStringLiteral("hotmail.it"),
            QStringLiteral("hotmail.lt"), QStringLiteral("hotmail.lv"), QStringLiteral("hotmail.my"),
            QStringLiteral("hotmail.nl"), QStringLiteral("hotmail.no"), QStringLiteral("hotmail.ph"),
            QStringLiteral("hotmail.rs"), QStringLiteral("hotmail.se"), QStringLiteral("hotmail.sg"),
            QStringLiteral("hotmail.sk"), QStringLiteral("hotmail.co.id"), QStringLiteral("hotmail.co.il"),
            QStringLiteral("hotmail.co.in"), QStringLiteral("hotmail.co.jp"), QStringLiteral("hotmail.co.kr"),
            QStringLiteral("hotmail.co.th"), QStringLiteral("hotmail.co.uk"), QStringLiteral("hotmail.co.za"),
            QStringLiteral("hotmail.com.ar"), QStringLiteral("hotmail.com.au"), QStringLiteral("hotmail.com.br"),
            QStringLiteral("hotmail.com.hk"), QStringLiteral("hotmail.com.tr"), QStringLiteral("hotmail.com.tw"),
            QStringLiteral("hotmail.com.vn"),
            // Live regional domains
            QStringLiteral("live.at"), QStringLiteral("live.be"), QStringLiteral("live.ca"),
            QStringLiteral("live.cl"), QStringLiteral("live.cn"), QStringLiteral("live.de"),
            QStringLiteral("live.dk"), QStringLiteral("live.fi"), QStringLiteral("live.fr"),
            QStringLiteral("live.hk"), QStringLiteral("live.ie"), QStringLiteral("live.in"),
            QStringLiteral("live.it"), QStringLiteral("live.jp"), QStringLiteral("live.nl"),
            QStringLiteral("live.no"), QStringLiteral("live.ru"), QStringLiteral("live.se"),
            QStringLiteral("live.co.jp"), QStringLiteral("live.co.kr"), QStringLiteral("live.co.uk"),
            QStringLiteral("live.co.za"), QStringLiteral("live.com.ar"), QStringLiteral("live.com.au"),
            QStringLiteral("live.com.mx"), QStringLiteral("live.com.my"), QStringLiteral("live.com.ph"),
            QStringLiteral("live.com.pt"), QStringLiteral("live.com.sg"), QStringLiteral("livemail.tw"),
            // Other Microsoft domains
            QStringLiteral("olc.protection.outlook.com")
        }
    });

    // Yahoo Mail
    // Docs: https://developer.yahoo.com/oauth2/guide/
    // Note: Yahoo requires app registration in Yahoo Developer Network
    m_providers.append({
        QStringLiteral("yahoo"),
        QStringLiteral("Yahoo"),
        QStringLiteral(""), // TODO: Register app in Yahoo Developer Network and add client ID
        QStringLiteral("https://api.login.yahoo.com/oauth2/request_auth"),
        QStringLiteral("https://api.login.yahoo.com/oauth2/get_token"),
        QStringLiteral("mail-w"),
        {QStringLiteral("yahoo.com"), QStringLiteral("yahoo.co.uk"), QStringLiteral("yahoo.ca"),
         QStringLiteral("yahoo.com.au"), QStringLiteral("yahoo.de"), QStringLiteral("yahoo.fr"),
         QStringLiteral("yahoo.co.jp"), QStringLiteral("ymail.com"), QStringLiteral("rocketmail.com")}
    });
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
