// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oauthprovider_bridge.h"
#include "oauthprovider.h"

namespace {
    QString toQString(rust::Str s) {
        return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
    }

    rust::String toRustString(const QString& s) {
        QByteArray utf8 = s.toUtf8();
        return rust::String(utf8.constData(), utf8.size());
    }
}

int oauth_provider_count() {
    return OAuthProviderRegistry::instance().providers().size();
}

rust::String oauth_provider_id(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].id);
}

rust::String oauth_provider_name(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].name);
}

rust::String oauth_provider_client_id(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].clientId);
}

rust::String oauth_provider_auth_endpoint(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].authEndpoint);
}

rust::String oauth_provider_token_endpoint(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].tokenEndpoint);
}

rust::String oauth_provider_scope(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return rust::String();
    }
    return toRustString(providers[index].scope);
}

int oauth_provider_find_by_id(rust::Str id) {
    QString qid = toQString(id);
    auto providers = OAuthProviderRegistry::instance().providers();
    for (int i = 0; i < providers.size(); ++i) {
        if (providers[i].id == qid) {
            return i;
        }
    }
    return -1;
}

int oauth_provider_find_by_domain(rust::Str domain) {
    QString qdomain = toQString(domain);
    auto provider = OAuthProviderRegistry::instance().providerByDomain(qdomain);
    if (!provider.has_value()) {
        return -1;
    }
    // Find the index
    auto providers = OAuthProviderRegistry::instance().providers();
    for (int i = 0; i < providers.size(); ++i) {
        if (providers[i].id == provider->id) {
            return i;
        }
    }
    return -1;
}

int oauth_provider_find_by_email(rust::Str email) {
    QString qemail = toQString(email);
    auto provider = OAuthProviderRegistry::instance().providerByEmail(qemail);
    if (!provider.has_value()) {
        return -1;
    }
    // Find the index
    auto providers = OAuthProviderRegistry::instance().providers();
    for (int i = 0; i < providers.size(); ++i) {
        if (providers[i].id == provider->id) {
            return i;
        }
    }
    return -1;
}

bool oauth_provider_is_valid(int index) {
    auto providers = OAuthProviderRegistry::instance().providers();
    if (index < 0 || index >= providers.size()) {
        return false;
    }
    return providers[index].isValid();
}
