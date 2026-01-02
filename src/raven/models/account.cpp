// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "account.h"
#include "constants.h"
#include "loggingsqlquery.h"
#include "ravendaemoninterface.h"

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDir>

const QString ACCOUNT_CONFIG_GROUP = QStringLiteral("Account");
const QString METADATA_CONFIG_GROUP = QStringLiteral("Metadata");

Account::Account(QObject *parent)
    : QObject{parent}
    , m_id{QUuid::createUuid().toString(QUuid::Id128)}
{
}

Account::Account(QObject *parent, KConfig *config)
    : QObject{parent}
    , m_config{config}
{
    auto metadata = KConfigGroup{config, METADATA_CONFIG_GROUP};
    m_valid = metadata.readEntry("valid", false);
    m_id = metadata.readEntry("id");

    auto group = KConfigGroup{config, ACCOUNT_CONFIG_GROUP};
    m_email = group.readEntry("email");
    m_name = group.readEntry("name");

    m_imapHost = group.readEntry("imapHost");
    m_imapPort = group.readEntry("imapPort", 0);
    m_imapUsername = group.readEntry("imapUsername");
    m_imapPassword = readPasswordFromDaemon(m_id + QStringLiteral("-imapPassword"));
    m_imapAuthenticationType = (AuthenticationType) group.readEntry("imapAuthenticationType", 0);
    m_imapConnectionType = (ConnectionType) group.readEntry("imapConnectionType", 0);

    m_smtpHost = group.readEntry("smtpHost");
    m_smtpPort = group.readEntry("smtpPort", 0);
    m_smtpUsername = group.readEntry("smtpUsername");
    m_smtpPassword = readPasswordFromDaemon(m_id + QStringLiteral("-smtpPassword"));
    m_smtpAuthenticationType = (AuthenticationType) group.readEntry("smtpAuthenticationType", 0);
    m_smtpConnectionType = (ConnectionType) group.readEntry("smtpConnectionType", 0);
}

QString Account::readPasswordFromDaemon(const QString &key)
{
    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "Failed to connect to daemon for password read";
        return QString();
    }

    QDBusPendingReply<QString> reply = iface.ReadPassword(key);
    reply.waitForFinished();
    if (!reply.isValid()) {
        qWarning() << "Failed to read password from daemon:" << reply.error().message();
        return QString();
    }
    return reply.value();
}

bool Account::writePasswordToDaemon(const QString &key, const QString &password)
{
    OrgKdeRavenDaemonInterface iface(DBUS_SERVICE, DBUS_PATH, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "Failed to connect to daemon for password write";
        return false;
    }

    QDBusPendingReply<bool> reply = iface.WritePassword(key, password);
    reply.waitForFinished();
    if (!reply.isValid()) {
        qWarning() << "Failed to write password to daemon:" << reply.error().message();
        return false;
    }
    return reply.value();
}

QString Account::id() const
{
    return m_id;
}

bool Account::valid() const
{
    return m_valid;
}

void Account::setValid(bool valid)
{
    if (m_valid != valid) {
        m_valid = valid;
        Q_EMIT validChanged();
    }
}

QString Account::email() const
{
    return m_email;
}

void Account::setEmail(const QString &email)
{
    if (m_email != email) {
        m_email = email;
        Q_EMIT emailChanged();
    }
}

QString Account::name() const
{
    return m_name;
}

void Account::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}

QString Account::imapHost() const
{
    return m_imapHost;
}

void Account::setImapHost(const QString &host)
{
    if (m_imapHost != host) {
        m_imapHost = host;
        Q_EMIT imapHostChanged();
    }
}

int Account::imapPort() const
{
    return m_imapPort;
}

void Account::setImapPort(int port)
{
    if (m_imapPort != port) {
        m_imapPort = port;
        Q_EMIT imapPortChanged();
    }
}

QString Account::imapUsername() const
{
    return m_imapUsername;
}

void Account::setImapUsername(const QString &username)
{
    if (m_imapUsername != username) {
        m_imapUsername = username;
        Q_EMIT imapUsernameChanged();
    }
}

QString Account::imapPassword() const
{
    return m_imapPassword;
}

void Account::setImapPassword(const QString &password)
{
    if (m_imapPassword != password) {
        m_imapPassword = password;
        Q_EMIT imapPasswordChanged();
    }
}

Account::ConnectionType Account::imapConnectionType() const
{
    return m_imapConnectionType;
}

void Account::setImapConnectionType(ConnectionType connectionType)
{
    if (m_imapConnectionType != connectionType) {
        m_imapConnectionType = connectionType;
        Q_EMIT imapConnectionTypeChanged();
    }
}

Account::AuthenticationType Account::imapAuthenticationType() const
{
    return m_imapAuthenticationType;
}

void Account::setImapAuthenticationType(AuthenticationType authenticationType)
{
    if (m_imapAuthenticationType != authenticationType) {
        m_imapAuthenticationType = authenticationType;
        Q_EMIT imapAuthenticationTypeChanged();
    }
}

QString Account::smtpHost() const
{
    return m_smtpHost;
}

void Account::setSmtpHost(const QString &host)
{
    if (m_smtpHost != host) {
        m_smtpHost = host;
        Q_EMIT smtpHostChanged();
    }
}

int Account::smtpPort() const
{
    return m_smtpPort;
}

void Account::setSmtpPort(int port)
{
    if (m_smtpPort != port) {
        m_smtpPort = port;
        Q_EMIT smtpPortChanged();
    }
}

QString Account::smtpUsername() const
{
    return m_smtpUsername;
}

void Account::setSmtpUsername(const QString &username)
{
    if (m_smtpUsername != username) {
        m_smtpUsername = username;
        Q_EMIT smtpUsernameChanged();
    }
}

QString Account::smtpPassword() const
{
    return m_smtpPassword;
}

void Account::setSmtpPassword(const QString &password)
{
    if (m_smtpPassword != password) {
        m_smtpPassword = password;
        Q_EMIT smtpPasswordChanged();
    }
}

Account::ConnectionType Account::smtpConnectionType() const
{
    return m_smtpConnectionType;
}

void Account::setSmtpConnectionType(ConnectionType connectionType)
{
    if (m_smtpConnectionType != connectionType) {
        m_smtpConnectionType = connectionType;
        Q_EMIT smtpConnectionTypeChanged();
    }
}

Account::AuthenticationType Account::smtpAuthenticationType() const
{
    return m_smtpAuthenticationType;
}

void Account::setSmtpAuthenticationType(AuthenticationType authenticationType)
{
    if (m_smtpAuthenticationType != authenticationType) {
        m_smtpAuthenticationType = authenticationType;
        Q_EMIT smtpAuthenticationTypeChanged();
    }
}

QString Account::oauthProviderId() const
{
    return m_oauthProviderId;
}

void Account::setOAuthProviderId(const QString &providerId)
{
    m_oauthProviderId = providerId;
}

qint64 Account::oauthTokenExpiry() const
{
    return m_oauthTokenExpiry;
}

void Account::setOAuthTokenExpiry(qint64 expiry)
{
    m_oauthTokenExpiry = expiry;
}

void Account::setOAuthTokens(const QString &accessToken, const QString &refreshToken)
{
    writePasswordToDaemon(m_id + QStringLiteral("-oauthAccessToken"), accessToken);
    writePasswordToDaemon(m_id + QStringLiteral("-oauthRefreshToken"), refreshToken);
}

bool Account::save(QString *errorOut)
{
    // Create account directory
    QString accountDir = RAVEN_CONFIG_LOCATION + QStringLiteral("/accounts/") + m_id;
    if (!QDir().mkpath(accountDir)) {
        QString error = QStringLiteral("Failed to create account directory: %1").arg(accountDir);
        qWarning() << error;
        if (errorOut) {
            *errorOut = error;
        }
        return false;
    }

    // Create config file path
    QString configPath = accountDir + QStringLiteral("/account.ini");
    auto config = new KConfig{configPath, KConfig::SimpleConfig};

    // Write Metadata section
    auto metadata = KConfigGroup{config, METADATA_CONFIG_GROUP};
    metadata.writeEntry("version", 1);
    metadata.writeEntry("id", m_id);
    metadata.writeEntry("valid", true);

    // Write Account section
    auto group = KConfigGroup{config, ACCOUNT_CONFIG_GROUP};
    group.writeEntry("email", m_email);
    group.writeEntry("name", m_name);

    // IMAP settings
    group.writeEntry("imapHost", m_imapHost);
    group.writeEntry("imapPort", m_imapPort);
    group.writeEntry("imapUsername", m_imapUsername);
    group.writeEntry("imapConnectionType", static_cast<int>(m_imapConnectionType));
    group.writeEntry("imapAuthenticationType", static_cast<int>(m_imapAuthenticationType));

    // SMTP settings
    group.writeEntry("smtpHost", m_smtpHost);
    group.writeEntry("smtpPort", m_smtpPort);
    group.writeEntry("smtpUsername", m_smtpUsername);
    group.writeEntry("smtpConnectionType", static_cast<int>(m_smtpConnectionType));
    group.writeEntry("smtpAuthenticationType", static_cast<int>(m_smtpAuthenticationType));

    // OAuth2 settings (non-sensitive data only)
    if (!m_oauthProviderId.isEmpty()) {
        auto oauth2 = KConfigGroup{config, QStringLiteral("OAuth2")};
        oauth2.writeEntry("providerId", m_oauthProviderId);
        if (m_oauthTokenExpiry > 0) {
            oauth2.writeEntry("tokenExpiry", m_oauthTokenExpiry);
        }
    }

    // Sync to disk
    config->sync();
    delete config;

    // IMAP password
    if (!m_imapPassword.isEmpty()) {
        if (!writePasswordToDaemon(m_id + QStringLiteral("-imapPassword"), m_imapPassword)) {
            qWarning() << "Failed to save IMAP password for account" << m_id;
        }
    }

    // SMTP password
    if (!m_smtpPassword.isEmpty()) {
        if (!writePasswordToDaemon(m_id + QStringLiteral("-smtpPassword"), m_smtpPassword)) {
            qWarning() << "Failed to save SMTP password for account" << m_id;
        }
    }

    // OAuth2 tokens (they're already saved by setOAuthTokens, so we don't need to write them again)
    // Just verify they exist if OAuth2 is enabled
    if (m_imapAuthenticationType == AuthenticationType::OAuth2 ||
        m_smtpAuthenticationType == AuthenticationType::OAuth2) {
        QString oauthAccessToken = readPasswordFromDaemon(m_id + QStringLiteral("-oauthAccessToken"));
        QString oauthRefreshToken = readPasswordFromDaemon(m_id + QStringLiteral("-oauthRefreshToken"));

        if (oauthAccessToken.isEmpty() || oauthRefreshToken.isEmpty()) {
            qWarning() << "OAuth2 authentication enabled but tokens not found for account" << m_id;
        }
    }

    qDebug() << "Account configuration saved to" << configPath;
    m_valid = true;
    return true;
}

