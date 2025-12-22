// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "googleoauth.h"

#include <QCryptographicHash>
#include <QUrlQuery>

const QString GOOGLE_OAUTH_ENDPOINT = QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth");
const QString GOOGLE_CLIENT_ID = QStringLiteral("1095027536469-irmaqcgpjkf3dre21nvd0sg378etobi7.apps.googleusercontent.com");
const QString GOOGLE_LOOPBACK_IP = QStringLiteral("http://127.0.0.1:80");

GoogleOAuth::GoogleOAuth(QObject *parent)
    : QObject{parent}
    , m_manager{new QNetworkAccessManager{this}}
{
}

void GoogleOAuth::makeConnection()
{
    QNetworkRequest request;
    request.setUrl(QUrl(GOOGLE_OAUTH_ENDPOINT));
    
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("scope"), QStringLiteral("email profile"));
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    query.addQueryItem(QStringLiteral("state"), QStringLiteral("test"));
    query.addQueryItem(QStringLiteral("redirect_uri"), GOOGLE_LOOPBACK_IP);
    query.addQueryItem(QStringLiteral("client_id"), GOOGLE_CLIENT_ID);
    
    // TODO
}

QString GoogleOAuth::createCodeVerifier()
{
    return createRandomString(43);
}

QString GoogleOAuth::createCodeChallenge(const QString &verifier)
{
    return QString::fromUtf8(QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256).toBase64());
}

QString GoogleOAuth::createRandomString(int len) {
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(len, 0);
    std::generate_n( str.begin(), len, randchar );
    return QString::fromStdString(str);
}
