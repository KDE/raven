#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <KLocalizedContext>
#include <KDescendantsProxyModel>
#include <MessageComposer/Composer>
#include <MessageComposer/AttachmentModel>
#include <MessageComposer/TextPart>
#include <MessageComposer/InfoPart>
#include <QDebug>

#include "raven.h"
#include "mailmanager.h"
#include "mailmodel.h"
#include "mime/htmlutils.h"
#include "mime/messageparser.h"

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

    QQmlApplicationEngine engine;
    
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

    qmlRegisterType<MessageParser>("org.kde.raven", 1, 0, "MessageParser");

    qRegisterMetaType<MailModel *>("MailModel*");
    qRegisterMetaType<Akonadi::CollectionFilterProxyModel *>("Akonadi::CollectionFilterProxyModel *");
    qRegisterMetaType<Akonadi::Item::Id>("Akonadi::Item::Id");
    qRegisterMetaType<KDescendantsProxyModel*>("KDescendantsProxyModel*");
    
    qmlRegisterSingletonInstance<Raven>("org.kde.raven", 1, 0, "Raven", raven);

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
