// SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
// SPDX-FileCopyrightText: 2010 Tom Albers <toma@kde.org>
// SPDX-FileCopyrightText: 2012-2022 Laurent Montel <montel@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "newaccountmanager.h"
#include "modelviews/accountmodel.h"

NewAccountManager::NewAccountManager(QObject* parent)
    : QObject(parent)
    , m_ispdb{nullptr}
    , m_imapPort{993}
    , m_imapAuthenticationType{Account::AuthenticationType::Plain}
    , m_smtpPort{587}
    , m_smtpAuthenticationType{Account::AuthenticationType::Plain}
{
}

NewAccountManager::~NewAccountManager() noexcept
{
}

QString &NewAccountManager::email()
{
    return m_email;
}

void NewAccountManager::setEmail(const QString& email)
{
    if (m_email != email) {
        m_email = email;
        Q_EMIT emailChanged();
    }
}

QString &NewAccountManager::name()
{
    return m_name;
}

void NewAccountManager::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}

QString &NewAccountManager::password()
{
    return m_password;
}

void NewAccountManager::setPassword(const QString &password)
{
    if (m_password != password) {
        m_password = password;
        Q_EMIT passwordChanged();
    }
}

bool NewAccountManager::ispdbIsSearching()
{
    return m_ispdbIsSearching;
}

QString &NewAccountManager::imapHost()
{
    return m_imapHost;
}

void NewAccountManager::setImapHost(QString host)
{
    if (m_imapHost != host) {
        m_imapHost = host;
        Q_EMIT imapHostChanged();
    }
}

int NewAccountManager::imapPort()
{
    return m_imapPort;
}

void NewAccountManager::setImapPort(int port)
{
    if (m_imapPort != port) {
        m_imapPort = port;
        Q_EMIT imapPortChanged();
    }
}

QString &NewAccountManager::imapUsername()
{
    return m_imapUsername;
}

void NewAccountManager::setImapUsername(QString username)
{
    if (m_imapUsername != username) {
        m_imapUsername = username;
        Q_EMIT imapUsernameChanged();
    }
}

QString &NewAccountManager::imapPassword()
{
    return m_imapPassword;
}

void NewAccountManager::setImapPassword(QString password)
{
    if (m_imapPassword != password) {
        m_imapPassword = password;
        Q_EMIT imapPasswordChanged();
    }
}

Account::AuthenticationType NewAccountManager::imapAuthenticationType()
{
    return m_imapAuthenticationType;
}

void NewAccountManager::setImapAuthenticationType(Account::AuthenticationType authenticationType)
{
   if (m_imapAuthenticationType != authenticationType) {
       m_imapAuthenticationType = authenticationType;
       Q_EMIT imapAuthenticationTypeChanged();
} 
}

Account::ConnectionType NewAccountManager::imapConnectionType()
{
    return m_imapConnectionType;
}

void NewAccountManager::setImapConnectionType(Account::ConnectionType connectionType)
{
    if (m_imapConnectionType != connectionType) {
        m_imapConnectionType = connectionType;
        Q_EMIT imapConnectionTypeChanged();
    }
}

QString &NewAccountManager::smtpHost()
{
    return m_smtpHost;
}

void NewAccountManager::setSmtpHost(QString host)
{
    if (m_smtpHost != host) {
        m_smtpHost = host;
        Q_EMIT smtpHostChanged();
    }
}

int NewAccountManager::smtpPort()
{
    return m_smtpPort;
}

void NewAccountManager::setSmtpPort(int port)
{
    if (m_smtpPort != port) {
        m_smtpPort = port;
        Q_EMIT smtpPortChanged();
    }
}

QString &NewAccountManager::smtpUsername()
{
    return m_smtpUsername;
}

void NewAccountManager::setSmtpUsername(QString username)
{
    if (m_smtpUsername != username) {
        m_smtpUsername = username;
        Q_EMIT smtpUsernameChanged();
    }
}

QString &NewAccountManager::smtpPassword()
{
    return m_smtpPassword;
}

void NewAccountManager::setSmtpPassword(QString password)
{
    if (m_smtpPassword != password) {
        m_smtpPassword = password;
        Q_EMIT smtpPasswordChanged();
    }
}

Account::AuthenticationType NewAccountManager::smtpAuthenticationType()
{
    return m_smtpAuthenticationType;
}

void NewAccountManager::setSmtpAuthenticationType(Account::AuthenticationType authenticationType)
{
   if (m_smtpAuthenticationType != authenticationType) {
       m_smtpAuthenticationType = authenticationType;
       Q_EMIT smtpAuthenticationTypeChanged();
} 
}

Account::ConnectionType NewAccountManager::smtpConnectionType()
{
    return m_smtpConnectionType;
}

void NewAccountManager::setSmtpConnectionType(Account::ConnectionType connectionType)
{
    if (m_smtpConnectionType != connectionType) {
        m_smtpConnectionType = connectionType;
        Q_EMIT smtpConnectionTypeChanged();
    }
}

void NewAccountManager::searchIspdbForConfig()
{
    delete m_ispdb;
    m_ispdb = new Ispdb(this);
    
    m_ispdb->setEmail(m_email);
    m_ispdb->start();
    
    connect(m_ispdb, &Ispdb::finished, this, &NewAccountManager::ispdbFinishedSearchingSlot);
    
    m_ispdbIsSearching = true;
    Q_EMIT ispdbIsSearchingChanged();
}

void NewAccountManager::addAccount()
{
    Account *account = new Account{AccountModel::self()};

    account->setEmail(m_email);
    account->setName(m_name);

    account->setImapHost(m_imapHost);
    account->setImapPort(m_imapPort);
    account->setImapUsername(m_imapUsername);
    account->setImapPassword(m_imapPassword);
    account->setImapAuthenticationType(m_imapAuthenticationType);
    account->setImapConnectionType(m_imapConnectionType);

    account->setSmtpHost(m_smtpHost);
    account->setSmtpPort(m_smtpPort);
    account->setSmtpUsername(m_smtpUsername);
    account->setSmtpPassword(m_smtpPassword);
    account->setSmtpAuthenticationType(m_smtpAuthenticationType);
    account->setSmtpConnectionType(m_smtpConnectionType);

    account->initSettings();
    AccountModel::self()->addAccount(account);
}

void NewAccountManager::ispdbFinishedSearchingSlot()
{
    if (!m_ispdb) {
        return;
    }
    
    m_ispdbIsSearching = false;
    Q_EMIT ispdbIsSearchingChanged();
    
    // add smtp settings
    if (!m_ispdb->smtpServers().isEmpty()) {
        const Server s = m_ispdb->smtpServers().at(0);
        setSmtpHost(s.hostname);
        setSmtpPort(s.port);
        setSmtpUsername(s.username);
        setSmtpPassword(m_password);
        setSmtpAuthenticationType(ispdbTypeToAuth(s.authentication));
        setSmtpConnectionType(ispdbTypeToSocket(s.socketType));
    }
    
    // add imap settings
    if (!m_ispdb->imapServers().isEmpty()) {
        const Server s = m_ispdb->imapServers().at(0);
        setImapHost(s.hostname);
        setImapPort(s.port);
        setImapUsername(s.username);
        setImapPassword(m_password);
        setImapAuthenticationType(ispdbTypeToAuth(s.authentication));
        setImapConnectionType(ispdbTypeToSocket(s.socketType));
    }
}

Account::AuthenticationType NewAccountManager::ispdbTypeToAuth(Ispdb::authType authType)
{
    switch (authType) {
        case Ispdb::Plain:
            return Account::AuthenticationType::Plain;
        case Ispdb::CramMD5:
            return Account::AuthenticationType::NoAuth;
        case Ispdb::NTLM:
            return Account::AuthenticationType::NoAuth; // TODO
        case Ispdb::GSSAPI:
            return Account::AuthenticationType::NoAuth; // TODO
        case Ispdb::OAuth2:
            return Account::AuthenticationType::OAuth2;
        case Ispdb::ClientIP:
        case Ispdb::NoAuth:
        default:
            return Account::AuthenticationType::NoAuth;
    }
}

Account::ConnectionType NewAccountManager::ispdbTypeToSocket(Ispdb::socketType socketType)
{
    switch (socketType) {
        case Ispdb::SSL:
            return Account::ConnectionType::SSL;
        case Ispdb::StartTLS:
            return Account::ConnectionType::StartTLS;
        case Ispdb::None:
        default:
            return Account::ConnectionType::None;
    }
}

