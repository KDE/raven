// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QJsonObject>

#include <KConfigGroup>
#include <KWallet>
#include <KSharedConfig>

using namespace KWallet;

class Account : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)

    Q_PROPERTY(QString imapHost READ imapHost WRITE setImapHost NOTIFY imapHostChanged)
    Q_PROPERTY(int imapPort READ imapPort WRITE setImapPort NOTIFY imapPortChanged)
    Q_PROPERTY(QString imapUsername READ imapUsername WRITE setImapUsername NOTIFY imapUsernameChanged)
    Q_PROPERTY(QString imapPassword READ imapPassword WRITE setImapPassword NOTIFY imapPasswordChanged)
    Q_PROPERTY(ConnectionType imapConnectionType READ imapConnectionType WRITE setImapConnectionType NOTIFY imapConnectionTypeChanged)
    Q_PROPERTY(AuthenticationType imapAuthenticationType READ imapAuthenticationType WRITE setImapAuthenticationType NOTIFY imapAuthenticationTypeChanged)

    Q_PROPERTY(QString smtpHost READ smtpHost WRITE setSmtpHost NOTIFY smtpHostChanged)
    Q_PROPERTY(int smtpPort READ smtpPort WRITE setSmtpPort NOTIFY smtpPortChanged)
    Q_PROPERTY(QString smtpUsername READ smtpUsername WRITE setSmtpUsername NOTIFY smtpUsernameChanged)
    Q_PROPERTY(QString smtpPassword READ smtpPassword WRITE setSmtpPassword NOTIFY smtpPasswordChanged)
    Q_PROPERTY(ConnectionType smtpConnectionType READ smtpConnectionType WRITE setSmtpConnectionType NOTIFY smtpConnectionTypeChanged)
    Q_PROPERTY(AuthenticationType smtpAuthenticationType READ smtpAuthenticationType WRITE setSmtpAuthenticationType NOTIFY smtpAuthenticationTypeChanged)

public:
    Account(QObject *parent = nullptr);
    Account(QObject *parent, KConfig *config);

    enum ConnectionType {
        SSL,
        StartTLS,
        None
    };
    Q_ENUM(ConnectionType)

    enum AuthenticationType {
        Plain,
        OAuth2,
        NoAuth
    };
    Q_ENUM(AuthenticationType)

    void initSettings();
    Q_INVOKABLE void saveSettings();

    QString id() const;

    bool valid() const;
    void setValid(bool valid);

    QString email() const;
    void setEmail(const QString &email);

    QString name() const;
    void setName(const QString &name);

    QString imapHost() const;
    void setImapHost(const QString &host);

    int imapPort() const;
    void setImapPort(int port);

    QString imapUsername() const;
    void setImapUsername(const QString &username);

    QString imapPassword() const;
    void setImapPassword(const QString &password);

    ConnectionType imapConnectionType() const;
    void setImapConnectionType(ConnectionType connectionType);

    AuthenticationType imapAuthenticationType() const;
    void setImapAuthenticationType(AuthenticationType authenticationType);

    QString smtpHost() const;
    void setSmtpHost(const QString &host);

    int smtpPort() const;
    void setSmtpPort(int port);

    QString smtpUsername() const;
    void setSmtpUsername(const QString &username);

    QString smtpPassword() const;
    void setSmtpPassword(const QString &password);

    ConnectionType smtpConnectionType() const;
    void setSmtpConnectionType(ConnectionType connectionType);

    AuthenticationType smtpAuthenticationType() const;
    void setSmtpAuthenticationType(AuthenticationType authenticationType);

Q_SIGNALS:
    void validChanged();
    void emailChanged();
    void nameChanged();

    void imapHostChanged();
    void imapPortChanged();
    void imapUsernameChanged();
    void imapPasswordChanged();
    void imapConnectionTypeChanged();
    void imapAuthenticationTypeChanged();

    void smtpHostChanged();
    void smtpPortChanged();
    void smtpUsernameChanged();
    void smtpPasswordChanged();
    void smtpConnectionTypeChanged();
    void smtpAuthenticationTypeChanged();

private:
    void setupWallet();
    KConfig *m_config = nullptr;
    Wallet *m_wallet = nullptr;

    bool m_valid;

    QString m_id;
    QString m_email;
    QString m_name;

    QString m_imapHost;
    int m_imapPort;
    QString m_imapUsername;
    QString m_imapPassword;
    ConnectionType m_imapConnectionType;
    AuthenticationType m_imapAuthenticationType;

    QString m_smtpHost;
    int m_smtpPort;
    QString m_smtpUsername;
    QString m_smtpPassword;
    ConnectionType m_smtpConnectionType;
    AuthenticationType m_smtpAuthenticationType;
};
