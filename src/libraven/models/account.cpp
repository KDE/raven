// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "account.h"
#include "constants.h"

#include <QDir>
#include <QSqlQuery>

#include <KWallet>

const QString ACCOUNT_CONFIG_GROUP = QStringLiteral("Account");
const QString METADATA_CONFIG_GROUP = QStringLiteral("Metadata");

Account::Account(QObject *parent)
    : QObject{parent}
    , m_wallet{Wallet::openWallet(Wallet::NetworkWallet(), 0)}
    , m_id{QUuid::createUuid().toString(QUuid::Id128)}
{
    setupWallet();
}

Account::Account(QObject *parent, KConfig *config)
    : QObject{parent}
    , m_config{config}
{
    setupWallet();

    auto metadata = KConfigGroup{config, METADATA_CONFIG_GROUP};
    m_valid = metadata.readEntry("valid", false);
    m_id = metadata.readEntry("id");

    auto group = KConfigGroup{config, ACCOUNT_CONFIG_GROUP};
    m_email = group.readEntry("email");
    m_name = group.readEntry("name");

    m_imapHost = group.readEntry("imapHost");
    m_imapPort = group.readEntry("imapPort", 0);
    m_imapUsername = group.readEntry("imapUsername");
    m_wallet->readPassword(m_id + QStringLiteral("-imapPassword"), m_imapPassword);
    m_imapAuthenticationType = (AuthenticationType) group.readEntry("imapAuthenticationType", 0);
    m_imapConnectionType = (ConnectionType) group.readEntry("imapConnectionType", 0);

    m_smtpHost = group.readEntry("smtpHost");
    m_smtpPort = group.readEntry("smtpPort", 0);
    m_smtpUsername = group.readEntry("smtpUsername");
    m_wallet->readPassword(m_id + QStringLiteral("-smtpPassword"), m_smtpPassword);
    m_smtpAuthenticationType = (AuthenticationType) group.readEntry("smtpAuthenticationType", 0);
    m_smtpConnectionType = (ConnectionType) group.readEntry("smtpConnectionType", 0);
}

void Account::setupWallet()
{
    m_wallet = Wallet::openWallet(Wallet::NetworkWallet(), 0);

    const QString folder = QStringLiteral("raven");

    if (!m_wallet->hasFolder(folder)) {
        m_wallet->createFolder(folder);
    }
    m_wallet->setFolder(folder);
}

void Account::initSettings()
{
    if (m_config) {
        return;
    }

    QString accountFolder = RAVEN_CONFIG_LOCATION + QStringLiteral("/accounts/") + m_id;
    if (!QDir().mkpath(accountFolder)) {
        qWarning() << "Could not create account config folder" << accountFolder;
        return;
    }

    m_config = new KConfig{accountFolder + QStringLiteral("/account.ini")};
    saveSettings();
    m_config->sync();
}

void Account::saveSettings()
{
    if (!m_config) {
        return;
    }

    auto metadata = KConfigGroup{m_config, METADATA_CONFIG_GROUP};
    metadata.writeEntry("version", 1);
    metadata.writeEntry("valid", m_valid);
    metadata.writeEntry("id", m_id);

    auto group = KConfigGroup{m_config, ACCOUNT_CONFIG_GROUP};
    group.writeEntry("email", m_email);
    group.writeEntry("name", m_name);

    group.writeEntry("imapHost", m_imapHost);
    group.writeEntry("imapPort", m_imapPort);
    group.writeEntry("imapUsername", m_imapUsername);
    m_wallet->writePassword(m_id + QStringLiteral("-imapPassword"), m_imapPassword);
    group.writeEntry("imapAuthenticationType", (int) m_imapAuthenticationType);
    group.writeEntry("imapConnectionType", (int) m_imapConnectionType);

    group.writeEntry("smtpHost", m_smtpHost);
    group.writeEntry("smtpPort", m_smtpPort);
    group.writeEntry("smtpUsername", m_smtpUsername);
    m_wallet->writePassword(m_id + QStringLiteral("-smtpPassword"), m_smtpPassword);
    group.writeEntry("smtpAuthenticationType", (int) m_smtpAuthenticationType);
    group.writeEntry("smtpConnectionType", (int) m_smtpConnectionType);

    m_config->sync();
}

void Account::remove(QSqlDatabase &db)
{
    QString accountsFolder = RAVEN_CONFIG_LOCATION + QStringLiteral("/accounts");
    QDir dir{accountsFolder + QStringLiteral("/") + m_id};
    dir.removeRecursively();
    
    m_wallet->removeEntry(m_id + QStringLiteral("-imapPassword"));
    m_wallet->removeEntry(m_id + QStringLiteral("-smtpPassword"));
    
    db.transaction();
    
    QSqlQuery query{db};
    query.exec(QStringLiteral("DELETE FROM jobs WHERE accountId = ")  + m_id);
    query.exec(QStringLiteral("DELETE FROM message_body JOIN message ON message.id = message_body.id WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM message WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM thread WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM thread_reference WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM thread_folder WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM folder WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM label WHERE accountId = ") + m_id);
    query.exec(QStringLiteral("DELETE FROM file WHERE accountId = ") + m_id);
    
    db.commit();
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
