// SPDX-FileCopyrightText: 2010 Omat Holding B.V. <info@omat.nl>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QDebug>
#include <QIcon>
#include <QQuickStyle>
#include <QtWebEngineQuick>
#include <QCommandLineParser>

#include <KCrash>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KWindowSystem>

#include "models/account.h"
#include "models/folder.h"
#include "models/message.h"
#include "models/messagecontact.h"
#include "accountmodel.h"
#include "dbmanager.h"
#include "dbwatcher.h"
#include "utils.h"

#include "modelviews/attachmentmodel.h"
#include "modelviews/threadviewmodel.h"
#include "modelviews/mailboxmodel.h"
#include "modelviews/maillistmodel.h"

#include "version.h"
#include "abouttype.h"
#include "raven.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    // Initialize QtWebEngine before QApplication
    QtWebEngineQuick::initialize();

    // set default style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
    // if using org.kde.desktop, ensure we use kde style if possible
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")) {
        qputenv("QT_QPA_PLATFORMTHEME", "kde");
    }

    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("raven");

    KAboutData aboutData(QStringLiteral("raven"),
                         i18n("Raven"),
                         QStringLiteral(RAVEN_VERSION_STRING),
                         i18n("A mail client"),
                         KAboutLicense::GPL,
                         i18n("© 2025 KDE Community"));
    aboutData.addAuthor(i18n("Devin Lin"), QString(), QStringLiteral("devin@kde.org"), QStringLiteral("https://espi.dev"));
    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();

    // Parse command line arguments
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    QCommandLineOption openMessageOption(
        QStringLiteral("open-message"),
        i18n("Open a specific message by ID"),
        QStringLiteral("message-id")
    );
    parser.addOption(openMessageOption);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    QString messageIdToOpen;
    if (parser.isSet(openMessageOption)) {
        messageIdToOpen = parser.value(openMessageOption);
    }

    KDBusService service(KDBusService::Unique);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule(QStringLiteral("org.kde.raven"), QStringLiteral("Main"));

    QQuickWindow *mainWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    Q_ASSERT(mainWindow);

    Raven* ravenInstance = engine.singletonInstance<Raven *>("org.kde.raven", "Raven");

    QObject::connect(&service,
                         &KDBusService::activateRequested,
                         mainWindow,
                         [mainWindow, ravenInstance](const QStringList &arguments, const QString &workingDirectory) {
                            Q_UNUSED(workingDirectory);

                            // Parse arguments for second instance activation
                            QCommandLineParser parser;
                            QCommandLineOption openMessageOption(
                                QStringLiteral("open-message"),
                                QStringLiteral("Open a specific message by ID"),
                                QStringLiteral("message-id")
                            );
                            parser.addOption(openMessageOption);
                            parser.parse(arguments);

                            if (parser.isSet(openMessageOption)) {
                                QString messageId = parser.value(openMessageOption);
                                qDebug() << "Secondary instance requested to open message:" << messageId;
                                ravenInstance->openMessage(messageId);
                            }

                            // Raise and activate window
                            mainWindow->requestActivate();
                            KWindowSystem::activateWindow(mainWindow);
                         });

    // If we have a message to open, set it
    if (!messageIdToOpen.isEmpty()) {
        ravenInstance->openMessage(messageIdToOpen);
    }

    // required for X11
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.raven")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
