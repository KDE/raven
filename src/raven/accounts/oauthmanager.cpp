// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "oauthmanager.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QRandomGenerator>

OAuthManager::OAuthManager(QObject *parent)
    : QObject{parent}
    , m_manager{new QNetworkAccessManager{this}}
    , m_server{new QTcpServer{this}}
{
    connect(m_server, &QTcpServer::newConnection, this, &OAuthManager::onNewConnection);
}

OAuthManager::~OAuthManager()
{
    cancel();
}

bool OAuthManager::hasProviderForEmail(const QString &email)
{
    return OAuthProviderRegistry::instance().providerByEmail(email).has_value();
}

QString OAuthManager::providerIdForEmail(const QString &email)
{
    auto provider = OAuthProviderRegistry::instance().providerByEmail(email);
    return provider.has_value() ? provider->id : QString();
}

QString OAuthManager::providerNameForEmail(const QString &email)
{
    auto provider = OAuthProviderRegistry::instance().providerByEmail(email);
    return provider.has_value() ? provider->name : QString();
}

void OAuthManager::startAuthFlowForEmail(const QString &email)
{
    auto provider = OAuthProviderRegistry::instance().providerByEmail(email);
    if (!provider.has_value()) {
        setErrorMessage(QStringLiteral("No OAuth provider found for this email"));
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    startAuthFlow(provider->id, email);
}

void OAuthManager::startAuthFlow(const QString &providerId, const QString &email)
{
    if (m_inProgress) {
        qWarning() << "OAuth flow already in progress";
        return;
    }

    auto provider = OAuthProviderRegistry::instance().providerById(providerId);
    if (!provider.has_value()) {
        setErrorMessage(QStringLiteral("Unknown OAuth provider: %1").arg(providerId));
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    if (!provider->isValid()) {
        setErrorMessage(QStringLiteral("OAuth provider %1 is not configured (missing client ID)").arg(provider->name));
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    m_currentProvider = *provider;
    m_currentProviderId = providerId;
    Q_EMIT currentProviderIdChanged();

    setInProgress(true);
    setErrorMessage(QString());
    m_email = email;

    // Generate PKCE code verifier and challenge
    m_codeVerifier = createCodeVerifier();
    QString codeChallenge = createCodeChallenge(m_codeVerifier);

    // Start local server to receive the callback
    // Try ports in range 49152-65535 (dynamic/private ports)
    bool serverStarted = false;
    for (int port = 49152; port < 49200; ++port) {
        if (m_server->listen(QHostAddress::LocalHost, port)) {
            m_redirectPort = port;
            serverStarted = true;
            qDebug() << "OAuth callback server listening on port" << port;
            break;
        }
    }

    if (!serverStarted) {
        setErrorMessage(QStringLiteral("Failed to start local OAuth callback server"));
        setInProgress(false);
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    // Build authorization URL
    QUrl authUrl(m_currentProvider.authEndpoint);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), m_currentProvider.clientId);
    query.addQueryItem(QStringLiteral("redirect_uri"),
                       QStringLiteral("http://localhost:%1").arg(m_redirectPort));
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    query.addQueryItem(QStringLiteral("scope"), m_currentProvider.scope);
    query.addQueryItem(QStringLiteral("code_challenge"), codeChallenge);
    query.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    query.addQueryItem(QStringLiteral("access_type"), QStringLiteral("offline")); // Get refresh token

    // Provider-specific parameters
    if (m_currentProviderId == QStringLiteral("gmail")) {
        query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent")); // Force consent to get refresh token
    } else if (m_currentProviderId == QStringLiteral("outlook")) {
        query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent"));
    }

    if (!email.isEmpty()) {
        query.addQueryItem(QStringLiteral("login_hint"), email);
    }

    authUrl.setQuery(query);

    qDebug() << "Opening OAuth URL for provider" << m_currentProvider.name << ":" << authUrl.toString();

    // Open the URL in the default browser
    if (!QDesktopServices::openUrl(authUrl)) {
        setErrorMessage(QStringLiteral("Failed to open browser for authentication"));
        cancel();
        Q_EMIT authFailed(m_errorMessage);
    }
}

void OAuthManager::cancel()
{
    if (m_server->isListening()) {
        m_server->close();
    }
    m_codeVerifier.clear();
    m_redirectPort = 0;
    m_currentProviderId.clear();
    setInProgress(false);
}

void OAuthManager::onNewConnection()
{
    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QByteArray data = socket->readAll();
        QString request = QString::fromUtf8(data);

        // Parse the HTTP request to extract the authorization code
        // Expected: GET /?code=AUTH_CODE&... HTTP/1.1
        QString code;
        QString error;

        QRegularExpression codeRegex(QStringLiteral("code=([^&\\s]+)"));
        QRegularExpression errorRegex(QStringLiteral("error=([^&\\s]+)"));

        QRegularExpressionMatch codeMatch = codeRegex.match(request);
        QRegularExpressionMatch errorMatch = errorRegex.match(request);

        if (errorMatch.hasMatch()) {
            error = QUrl::fromPercentEncoding(errorMatch.captured(1).toUtf8());
        } else if (codeMatch.hasMatch()) {
            code = QUrl::fromPercentEncoding(codeMatch.captured(1).toUtf8());
        }

        // Send response to browser
        QString providerName = m_currentProvider.name;
        QString responseHtml;
        if (!error.isEmpty()) {
            responseHtml = QStringLiteral(
                "<html><body style='font-family: sans-serif; text-align: center; padding: 50px;'>"
                "<h1 style='color: #c0392b;'>Authentication Failed</h1>"
                "<p>Error: %1</p>"
                "<p>You can close this window.</p>"
                "</body></html>").arg(error.toHtmlEscaped());
        } else if (!code.isEmpty()) {
            responseHtml = QStringLiteral(
                "<html><body style='font-family: sans-serif; text-align: center; padding: 50px;'>"
                "<h1 style='color: #27ae60;'>%1 Authentication Successful!</h1>"
                "<p>You can close this window and return to Raven.</p>"
                "</body></html>").arg(providerName.toHtmlEscaped());
        } else {
            responseHtml = QStringLiteral(
                "<html><body style='font-family: sans-serif; text-align: center; padding: 50px;'>"
                "<h1 style='color: #c0392b;'>Authentication Failed</h1>"
                "<p>No authorization code received.</p>"
                "<p>You can close this window.</p>"
                "</body></html>");
        }

        QString response = QStringLiteral(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "\r\n%1").arg(responseHtml);

        socket->write(response.toUtf8());
        socket->flush();
        socket->disconnectFromHost();

        // Stop listening
        m_server->close();

        // Process the result
        if (!error.isEmpty()) {
            setErrorMessage(error);
            setInProgress(false);
            Q_EMIT authFailed(error);
        } else if (!code.isEmpty()) {
            // Exchange the code for tokens
            exchangeCodeForTokens(code);
        } else {
            setErrorMessage(QStringLiteral("No authorization code received"));
            setInProgress(false);
            Q_EMIT authFailed(m_errorMessage);
        }
    });

    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void OAuthManager::exchangeCodeForTokens(const QString &code)
{
    qDebug() << "Exchanging authorization code for tokens with provider" << m_currentProvider.name;

    QUrl tokenUrl(m_currentProvider.tokenEndpoint);
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("client_id"), m_currentProvider.clientId);
    // Note: client_secret is omitted when using PKCE (code_verifier provides the security)
    params.addQueryItem(QStringLiteral("code"), code);
    params.addQueryItem(QStringLiteral("code_verifier"), m_codeVerifier);
    params.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    params.addQueryItem(QStringLiteral("redirect_uri"),
                       QStringLiteral("http://localhost:%1").arg(m_redirectPort));

    QNetworkReply *reply = m_manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &OAuthManager::onTokenResponse);
}

void OAuthManager::onTokenResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorBody = QString::fromUtf8(reply->readAll());
        qWarning() << "Token exchange failed:" << reply->errorString() << errorBody;
        setErrorMessage(QStringLiteral("Token exchange failed: %1 %2").arg(reply->errorString(), errorBody));
        setInProgress(false);
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject json = doc.object();

    QString accessToken = json[QStringLiteral("access_token")].toString();
    QString refreshToken = json[QStringLiteral("refresh_token")].toString();
    int expiresIn = json[QStringLiteral("expires_in")].toInt(3600);

    if (accessToken.isEmpty()) {
        setErrorMessage(QStringLiteral("No access token in response"));
        setInProgress(false);
        Q_EMIT authFailed(m_errorMessage);
        return;
    }

    // Calculate expiration timestamp
    qint64 expiresAt = QDateTime::currentSecsSinceEpoch() + expiresIn;

    qDebug() << "OAuth2 authentication successful for provider" << m_currentProvider.name;
    qDebug() << "Access token received, expires in" << expiresIn << "seconds";
    qDebug() << "Refresh token:" << (refreshToken.isEmpty() ? "not received" : "received");

    QString providerId = m_currentProviderId;
    setInProgress(false);
    Q_EMIT authSuccess(providerId, accessToken, refreshToken, expiresAt);
}

QString OAuthManager::createCodeVerifier()
{
    // PKCE code verifier: 43-128 characters, URL-safe
    return createRandomString(64);
}

QString OAuthManager::createCodeChallenge(const QString &verifier)
{
    // SHA256 hash of verifier, base64url encoded
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    // Convert to base64url (replace + with -, / with _, remove =)
    QString base64 = QString::fromUtf8(hash.toBase64());
    base64.replace(QLatin1Char('+'), QLatin1Char('-'));
    base64.replace(QLatin1Char('/'), QLatin1Char('_'));
    base64.remove(QLatin1Char('='));
    return base64;
}

QString OAuthManager::createRandomString(int len)
{
    // URL-safe characters for PKCE
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    const int charsetLen = sizeof(charset) - 1;

    QString result;
    result.reserve(len);

    for (int i = 0; i < len; ++i) {
        result.append(QLatin1Char(charset[QRandomGenerator::global()->bounded(charsetLen)]));
    }

    return result;
}

void OAuthManager::setInProgress(bool inProgress)
{
    if (m_inProgress != inProgress) {
        m_inProgress = inProgress;
        Q_EMIT inProgressChanged();
    }
}

void OAuthManager::setErrorMessage(const QString &error)
{
    if (m_errorMessage != error) {
        m_errorMessage = error;
        Q_EMIT errorMessageChanged();
    }
}
