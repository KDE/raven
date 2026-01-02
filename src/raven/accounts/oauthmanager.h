// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QTcpServer>

#include "oauthprovider.h"

/**
 * Provider-agnostic OAuth2 manager.
 * Handles OAuth2 authentication flow for any configured provider.
 */
class OAuthManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY inProgressChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString currentProviderId READ currentProviderId NOTIFY currentProviderIdChanged)

public:
    OAuthManager(QObject *parent = nullptr);
    ~OAuthManager();

    /// Start the OAuth2 flow for the given provider
    Q_INVOKABLE void startAuthFlow(const QString &providerId, const QString &email);

    /// Start the OAuth2 flow by detecting provider from email
    Q_INVOKABLE void startAuthFlowForEmail(const QString &email);

    /// Cancel any in-progress OAuth flow
    Q_INVOKABLE void cancel();

    /// Check if a provider exists for the given email domain
    Q_INVOKABLE static bool hasProviderForEmail(const QString &email);

    /// Get the provider ID for an email address (empty if none)
    Q_INVOKABLE static QString providerIdForEmail(const QString &email);

    /// Get the provider name for display
    Q_INVOKABLE static QString providerNameForEmail(const QString &email);

    bool inProgress() const { return m_inProgress; }
    QString errorMessage() const { return m_errorMessage; }
    QString currentProviderId() const { return m_currentProviderId; }

Q_SIGNALS:
    /// Emitted when OAuth2 flow completes successfully
    void authSuccess(const QString &providerId, const QString &accessToken, const QString &refreshToken, qint64 expiresAt);

    /// Emitted when OAuth2 flow fails
    void authFailed(const QString &error);

    void inProgressChanged();
    void errorMessageChanged();
    void currentProviderIdChanged();

private Q_SLOTS:
    void onNewConnection();
    void onTokenResponse();

private:
    QString createCodeVerifier();
    QString createCodeChallenge(const QString &verifier);
    QString createRandomString(int len);
    void exchangeCodeForTokens(const QString &code);
    void setInProgress(bool inProgress);
    void setErrorMessage(const QString &error);

    QNetworkAccessManager *m_manager;
    QTcpServer *m_server;
    QString m_codeVerifier;
    QString m_email;
    bool m_inProgress = false;
    QString m_errorMessage;
    int m_redirectPort = 0;

    // Current provider being used
    QString m_currentProviderId;
    OAuthProvider m_currentProvider;
};
