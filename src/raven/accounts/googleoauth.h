// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

class GoogleOAuth : public QObject
{
    Q_OBJECT
    
public:
    GoogleOAuth(QObject *parent = nullptr);
    void makeConnection();
    
private:
    QString createCodeVerifier();
    QString createCodeChallenge(const QString &verifier);
    QString createRandomString(int len);
    
    QNetworkAccessManager *m_manager;
};
