// SPDX-FileCopyrightText: 2010 Omat Holding B.V. <info@omat.nl>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QDebug>
#include <QIcon>

#include <KLocalizedContext>
#include <KLocalizedString>

#include <MailCore/MailCore.h>

#include "models/account.h"
#include "models/folder.h"
#include "models/message.h"
#include "models/messagecontact.h"

#include "modelviews/accountmodel.h"
#include "modelviews/mailboxmodel.h"

#include "accounts/newaccountmanager.h"

#include "abouttype.h"
#include "raven.h"
#include "dbmanager.h"
#include "utils.h"
#include "dbwatcher.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("raven");

    KAboutData aboutData(QStringLiteral("raven"),
                         i18n("Raven"),
                         QStringLiteral("0.1"),
                         i18n("A mail client"),
                         KAboutLicense::GPL,
                         i18n("© 2022 Plasma Development Team"));
    aboutData.addAuthor(i18n("Devin Lin"), QString(), QStringLiteral("devin@kde.org"), QStringLiteral("https://espi.dev"));
    KAboutData::setApplicationData(aboutData);

    QQmlApplicationEngine engine;

    // initialize data folders
    if (!QDir().mkpath(RAVEN_DATA_LOCATION)) {
        qWarning() << "Could not create database directory at" << RAVEN_DATA_LOCATION;
    }

    if (!QDir().mkpath(RAVEN_CONFIG_LOCATION)) {
        qWarning() << "Could not create config folder at" << RAVEN_CONFIG_LOCATION;
    }

    // run database migrations first
    // constructor enables sql connection for main thread
    DBManager::self()->migrate();

    // watch for db events for the UI
    DBWatcher::self()->initWatcher();

    // initialize immediately
    Raven::self();
    Utils::self();
    AccountModel::self();
    MailBoxModel::self();

    // configure types
    qmlRegisterType<NewAccountManager>("org.kde.raven", 1, 0, "NewAccountManager");
    qmlRegisterType<Account>("org.kde.raven", 1, 0, "Account");
    qmlRegisterType<Folder>("org.kde.raven", 1, 0, "Folder");
    qmlRegisterType<Message>("org.kde.raven", 1, 0, "Message");
    qmlRegisterType<MessageContact>("org.kde.raven", 1, 0, "MessageContact");

    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "MailBoxModel", MailBoxModel::self());
    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "AccountModel", AccountModel::self());

    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "AboutType", &AboutType::instance());
    qmlRegisterSingletonInstance<Raven>("org.kde.raven", 1, 0, "Raven", Raven::self());
    
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    // required for X11
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.raven")));
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
