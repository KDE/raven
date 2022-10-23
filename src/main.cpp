// SPDX-FileCopyrightText: 2010 Omat Holding B.V. <info@omat.nl>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QDebug>
#include <QIcon>

#include <KLocalizedContext>
#include <KDescendantsProxyModel>
#include <MessageComposer/Composer>
#include <MessageComposer/AttachmentModel>
#include <MessageComposer/TextPart>
#include <MessageComposer/InfoPart>

#include "raven.h"
#include "mailmanager.h"
#include "abouttype.h"
#include "mailmodel.h"
#include "contactimageprovider.h"
#include "mime/htmlutils.h"
#include "mime/messageparser.h"
#include "accounts/mailaccounts.h"
#include "accounts/newaccount.h"

#include <Akonadi/Item>
#include <Akonadi/CollectionFilterProxyModel>

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Raven"));

    KAboutData aboutData(QStringLiteral("raven"),
                         i18n("Raven"),
                         QStringLiteral("0.1"),
                         i18n("A mail client"),
                         KAboutLicense::GPL,
                         i18n("© 2022 Plasma Development Team"));
    aboutData.setBugAddress("https://bugs.kde.org/describecomponents.cgi?product=kweather");
    aboutData.addAuthor(i18n("Devin Lin"), QString(), QStringLiteral("devin@kde.org"), QStringLiteral("https://espi.dev"));
    KAboutData::setApplicationData(aboutData);

    QQmlApplicationEngine engine;
    
    // configure types
    qmlRegisterSingletonType<MailManager>("org.kde.raven", 1, 0, "MailManager", [](QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new MailManager;
    });

    qmlRegisterSingletonType<HtmlUtils::HtmlUtils>("org.kde.raven", 1, 0, "HtmlUtils", [](QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new HtmlUtils::HtmlUtils;
    });
    
    qmlRegisterSingletonType<MailAccounts>("org.kde.raven", 1, 0, "MailAccounts", [](QQmlEngine *engine, QJSEngine *scriptEngine) { return new MailAccounts; });

    qmlRegisterType<NewAccount>("org.kde.raven", 1, 0, "NewAccount");
    qmlRegisterType<MessageParser>("org.kde.raven", 1, 0, "MessageParser");

    qRegisterMetaType<MailModel *>("MailModel*");
    qRegisterMetaType<Akonadi::CollectionFilterProxyModel *>("Akonadi::CollectionFilterProxyModel *");
    qRegisterMetaType<Akonadi::Item::Id>("Akonadi::Item::Id");
    qRegisterMetaType<KDescendantsProxyModel*>("KDescendantsProxyModel*");
    
    qmlRegisterSingletonInstance("org.kde.raven", 1, 0, "AboutType", &AboutType::instance());
    qmlRegisterSingletonInstance<Raven>("org.kde.raven", 1, 0, "Raven", raven);

    engine.addImageProvider(QLatin1String("contact"), new ContactImageProvider);
    
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    // required for X11
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.raven")));
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
