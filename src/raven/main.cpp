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

#include <KCrash>
#include <KLocalizedContext>
#include <KLocalizedString>

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

    QQmlApplicationEngine engine;

    // Initialize models and set database connection
    Raven::self();

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule(QStringLiteral("org.kde.raven"), QStringLiteral("Main"));

    // required for X11
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.raven")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
