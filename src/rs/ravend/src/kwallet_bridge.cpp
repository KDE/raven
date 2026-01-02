// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwallet_bridge.h"
#include <KWallet>
#include <QString>
#include <memory>
#include <mutex>

using KWallet::Wallet;

// Global wallet instance with mutex for thread safety
static std::unique_ptr<Wallet> g_wallet;
static std::mutex g_wallet_mutex;

bool kwallet_open() {
    std::lock_guard<std::mutex> lock(g_wallet_mutex);

    if (g_wallet && g_wallet->isOpen()) {
        return true;
    }

    g_wallet.reset(Wallet::openWallet(Wallet::NetworkWallet(), 0));
    if (!g_wallet || !g_wallet->isOpen()) {
        return false;
    }

    const QString folder = QStringLiteral("raven");
    if (!g_wallet->hasFolder(folder)) {
        g_wallet->createFolder(folder);
    }
    g_wallet->setFolder(folder);

    return true;
}

void kwallet_close() {
    std::lock_guard<std::mutex> lock(g_wallet_mutex);
    g_wallet.reset();
}

rust::String kwallet_read_password(rust::Str key) {
    std::lock_guard<std::mutex> lock(g_wallet_mutex);

    if (!g_wallet || !g_wallet->isOpen()) {
        return rust::String("");
    }

    QString password;
    QString qkey = QString::fromUtf8(key.data(), key.size());

    int result = g_wallet->readPassword(qkey, password);
    if (result != 0) {
        return rust::String("");
    }

    QByteArray utf8 = password.toUtf8();
    return rust::String(utf8.constData(), utf8.size());
}

bool kwallet_write_password(rust::Str key, rust::Str password) {
    std::lock_guard<std::mutex> lock(g_wallet_mutex);

    if (!g_wallet || !g_wallet->isOpen()) {
        return false;
    }

    QString qkey = QString::fromUtf8(key.data(), key.size());
    QString qpassword = QString::fromUtf8(password.data(), password.size());

    int result = g_wallet->writePassword(qkey, qpassword);
    return result == 0;
}

bool kwallet_remove_entry(rust::Str key) {
    std::lock_guard<std::mutex> lock(g_wallet_mutex);

    if (!g_wallet || !g_wallet->isOpen()) {
        return false;
    }

    QString qkey = QString::fromUtf8(key.data(), key.size());

    int result = g_wallet->removeEntry(qkey);
    return result == 0;
}
