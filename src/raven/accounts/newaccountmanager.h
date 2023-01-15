// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>

#include "accounts/ispdb.h"
#include "../libraven/models/account.h"

/**
 * Object that is created in QML to facilitate the creation of a new email account.
 */

class NewAccountManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool ispdbIsSearching READ ispdbIsSearching NOTIFY ispdbIsSearchingChanged)
    
    // imap
    Q_PROPERTY(QString imapHost READ imapHost WRITE setImapHost NOTIFY imapHostChanged)
    Q_PROPERTY(int imapPort READ imapPort WRITE setImapPort NOTIFY imapPortChanged)
    Q_PROPERTY(QString imapUsername READ imapUsername WRITE setImapUsername NOTIFY imapUsernameChanged)
    Q_PROPERTY(QString imapPassword READ imapPassword WRITE setImapPassword NOTIFY imapPasswordChanged)
    Q_PROPERTY(Account::AuthenticationType imapAuthenticationType READ imapAuthenticationType WRITE setImapAuthenticationType NOTIFY imapAuthenticationTypeChanged)
    Q_PROPERTY(Account::ConnectionType imapConnectionType READ imapConnectionType WRITE setImapConnectionType NOTIFY imapConnectionTypeChanged)
    
    // smtp
    Q_PROPERTY(QString smtpHost READ smtpHost WRITE setSmtpHost NOTIFY smtpHostChanged)
    Q_PROPERTY(int smtpPort READ smtpPort WRITE setSmtpPort NOTIFY smtpPortChanged)
    Q_PROPERTY(QString smtpUsername READ smtpUsername WRITE setSmtpUsername NOTIFY smtpUsernameChanged)
    Q_PROPERTY(QString smtpPassword READ smtpPassword WRITE setSmtpPassword NOTIFY smtpPasswordChanged)
    Q_PROPERTY(Account::AuthenticationType smtpAuthenticationType READ smtpAuthenticationType WRITE setSmtpAuthenticationType NOTIFY smtpAuthenticationTypeChanged)
    Q_PROPERTY(Account::ConnectionType smtpConnectionType READ smtpConnectionType WRITE setSmtpConnectionType NOTIFY smtpConnectionTypeChanged)
    
public:
    NewAccountManager(QObject *parent = nullptr);
    virtual ~NewAccountManager() noexcept;
    
    QString &email();
    void setEmail(const QString &email);
    
    QString &name();
    void setName(const QString &name);
    
    QString &password();
    void setPassword(const QString &password);
    
    bool ispdbIsSearching();
    
    // imap form
    QString &imapHost();
    void setImapHost(QString host);
    
    int imapPort();
    void setImapPort(int port);
    
    QString &imapUsername();
    void setImapUsername(QString username);
    
    QString &imapPassword();
    void setImapPassword(QString password);
    
    Account::AuthenticationType imapAuthenticationType();
    void setImapAuthenticationType(Account::AuthenticationType authenticationType);
    
    Account::ConnectionType imapConnectionType();
    void setImapConnectionType(Account::ConnectionType connectionType);
    
    // smtp form
    QString &smtpHost();
    void setSmtpHost(QString host);
    
    int smtpPort();
    void setSmtpPort(int port);
    
    QString &smtpUsername();
    void setSmtpUsername(QString username);
    
    QString &smtpPassword();
    void setSmtpPassword(QString password);
    
    Account::AuthenticationType smtpAuthenticationType();
    void setSmtpAuthenticationType(Account::AuthenticationType authenticationType);
    
    Account::ConnectionType smtpConnectionType();
    void setSmtpConnectionType(Account::ConnectionType connectionType);
    
    // search online (mozilla db) for SMTP/IMAP/POP3 settings for the given email
    Q_INVOKABLE void searchIspdbForConfig();
    
    // add account with the current settings
    Q_INVOKABLE void addAccount();
    
public Q_SLOTS:
    void ispdbFinishedSearchingSlot();
    
Q_SIGNALS:
    void emailChanged();
    void nameChanged();
    void passwordChanged();
    void ispdbIsSearchingChanged();
    void receivingMailProtocolChanged();
    
    void imapHostChanged();
    void imapPortChanged();
    void imapUsernameChanged();
    void imapPasswordChanged();
    void imapAuthenticationTypeChanged();
    void imapConnectionTypeChanged();
    void smtpHostChanged();
    void smtpPortChanged();
    void smtpUsernameChanged();
    void smtpPasswordChanged();
    void smtpAuthenticationTypeChanged();
    void smtpConnectionTypeChanged();
    
    void setupSucceeded(const QString &msg);
    void setupFailed(const QString &msg);
    void setupInfo(const QString &msg);
    
private:
    Account::AuthenticationType ispdbTypeToAuth(Ispdb::authType authType);
    Account::ConnectionType ispdbTypeToSocket(Ispdb::socketType socketType);
    
    QString m_email;
    QString m_name;
    QString m_password;
    
    bool m_ispdbIsSearching;
    Ispdb *m_ispdb;
    
    QString m_imapHost;
    int m_imapPort;
    QString m_imapUsername;
    QString m_imapPassword;
    Account::AuthenticationType m_imapAuthenticationType;
    Account::ConnectionType m_imapConnectionType;
    
    QString m_smtpHost;
    int m_smtpPort;
    QString m_smtpUsername;
    QString m_smtpPassword;
    Account::AuthenticationType m_smtpAuthenticationType;
    Account::ConnectionType m_smtpConnectionType;
};
