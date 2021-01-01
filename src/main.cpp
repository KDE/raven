#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <KLocalizedContext>
#include <CollectionFilterProxyModel>
#include <EntityMimeTypeFilterModel>
#include <KDescendantsProxyModel>

#include "quickmail.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("KDE");
    QCoreApplication::setOrganizationDomain("kde.org");
    QCoreApplication::setApplicationName("KMailQuick");

    QQmlApplicationEngine engine;

    QuickMail quickmail;

    qRegisterMetaType<Akonadi::CollectionFilterProxyModel*>("Akonadi::CollectionFilterProxyModel*");
    qRegisterMetaType<Akonadi::EntityMimeTypeFilterModel*>("Akonadi::EntityMimeTypeFilterModel*");
    qRegisterMetaType<KDescendantsProxyModel*>("KDescendantsProxyModel*");
    qmlRegisterSingletonInstance("org.kde.quickmail.private", 1, 0, "QuickMail", &quickmail);

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    

    return app.exec();
}
