// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tray_bridge.h"

#include <QApplication>
#include <QMenu>
#include <QProcess>
#include <KStatusNotifierItem>
#include <KService>
#include <KIO/ApplicationLauncherJob>

#include <memory>
#include <atomic>

// Global state
static std::unique_ptr<QApplication> g_app;
static std::unique_ptr<KStatusNotifierItem> g_tray;
static std::unique_ptr<QMenu> g_menu;
static std::atomic<bool> g_quit_requested{false};

// Helper to open the Raven client app
static void openRavenClient() {
    // Try to find and launch the Raven desktop app
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.raven"));

    if (service) {
        auto *job = new KIO::ApplicationLauncherJob(service);
        job->start();
    } else {
        // Fallback: try to run the raven executable directly
        QProcess::startDetached(QStringLiteral("raven"), QStringList());
    }
}

bool tray_init() {
    // Create QApplication if it doesn't exist
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char appName[] = "ravend";
        static char* argv[] = { appName, nullptr };

        g_app = std::make_unique<QApplication>(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("ravend"));
        QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    }

    // Create the status notifier item
    g_tray = std::make_unique<KStatusNotifierItem>();
    g_tray->setIconByName(QStringLiteral("org.kde.raven"));
    g_tray->setToolTipIconByName(QStringLiteral("org.kde.raven"));
    g_tray->setToolTipTitle(QStringLiteral("Raven Mail"));
    g_tray->setToolTipSubTitle(QStringLiteral("Email sync service"));
    g_tray->setCategory(KStatusNotifierItem::Communications);
    g_tray->setStatus(KStatusNotifierItem::Active);
    g_tray->setStandardActionsEnabled(false);

    // Create context menu
    g_menu = std::make_unique<QMenu>();

    QAction* quitAction = g_menu->addAction(QStringLiteral("Quit"));
    QObject::connect(quitAction, &QAction::triggered, []() {
        g_quit_requested.store(true);
    });

    g_tray->setContextMenu(g_menu.get());

    // Connect activate signal (left click)
    QObject::connect(g_tray.get(), &KStatusNotifierItem::activateRequested,
                     [](bool active, const QPoint& pos) {
        Q_UNUSED(active);
        Q_UNUSED(pos);
        openRavenClient();
    });

    return true;
}

void tray_cleanup() {
    g_menu.reset();
    g_tray.reset();
    // Don't destroy QApplication - it may cause issues during shutdown
}

bool tray_is_quit_requested() {
    return g_quit_requested.load();
}

void tray_process_events() {
    if (QCoreApplication::instance()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
}
