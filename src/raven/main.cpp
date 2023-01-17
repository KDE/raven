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

#include "../libraven/models/account.h"
#include "../libraven/models/folder.h"
#include "../libraven/models/message.h"
#include "../libraven/models/messagecontact.h"
#include "../libraven/accountmodel.h"
#include "../libraven/utils.h"

#include "modelviews/threadviewmodel.h"
#include "modelviews/mailboxmodel.h"
#include "modelviews/maillistmodel.h"

#include "accounts/newaccountmanager.h"

#include "version.h"
#include "abouttype.h"
#include "raven.h"
#include "dbmanager.h"
#include "dbwatcher.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("raven");

    KAboutData aboutData(QStringLiteral("raven"),
                         i18n("Raven"),
                         QStringLiteral(RAVEN_VERSION_STRING),
                         i18n("A mail client"),
                         KAboutLicense::GPL,
                         i18n("© 2023 Plasma Development Team"));
    aboutData.addAuthor(i18n("Devin Lin"), QString(), QStringLiteral("devin@kde.org"), QStringLiteral("https://espi.dev"));
    KAboutData::setApplicationData(aboutData);

    QQmlApplicationEngine engine;

    // TODO DBUS ACTIVATION FOR DAEMON
    // we also can't assume database is active, so we should pause until the background daemon is finished setting up
    
    // initialize connection
    DBManager::self();

    // watch for db events for the UI
    DBWatcher::self()->initWatcher();

    // initialize immediately
    Raven::self();
    AccountModel::self();
    MailBoxModel::self();

    // configure types
    qmlRegisterType<NewAccountManager>("org.kde.raven", 1, 0, "NewAccountManager");
    qmlRegisterType<Account>("org.kde.raven", 1, 0, "Account");
    qmlRegisterType<Folder>("org.kde.raven", 1, 0, "Folder");
    qmlRegisterType<Message>("org.kde.raven", 1, 0, "Message");
    qmlRegisterType<MessageContact>("org.kde.raven", 1, 0, "MessageContact");

    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "ThreadViewModel", ThreadViewModel::self());
    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "MailListModel", MailListModel::self());
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
