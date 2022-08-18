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

#include "composerhelper.h"
#include "mailmodel.h"
#include "raven.h"
#include "identitymodel.h"
#include "messageviewer/viewer.h"
// #include <messageviewer/dkimmailstatus.h>
#include <messageviewer/dkimchecksignaturejob.h>
#include <Akonadi/Item>
#include <Akonadi/CollectionFilterProxyModel>

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("KMailQuick"));

    QQmlApplicationEngine engine;

    qRegisterMetaType<MailModel*>("MailModel*");
    qRegisterMetaType<ViewerHelper*>("ViewerHelper*");
    qRegisterMetaType<MessageComposer::Composer *>("Composer *");
    qRegisterMetaType<MessageComposer::InfoPart *>("InfoPart *");
    qRegisterMetaType<MessageComposer::TextPart *>("TextPart *");
    qRegisterMetaType<Akonadi::CollectionFilterProxyModel *>("Akonadi::CollectionFilterProxyModel *");
    qRegisterMetaType<MessageComposer::AttachmentModel *>("AttachmentModel *");
//     qRegisterMetaType<MessageViewer::DKIMCheckSignatureJob::DKIMStatus>("DKIMCheckSignatureJob::DKIMStatus");
    qRegisterMetaType<MessageViewer::DKIMCheckSignatureJob::DKIMError>("DKIMCheckSignatureJob::DKIMError");
    qRegisterMetaType<Akonadi::Item::Id>("Akonadi::Item::Id");
    qRegisterMetaType<KDescendantsProxyModel*>("KDescendantsProxyModel*");
    
    qmlRegisterType<ComposerHelper>("org.kde.raven.private", 1, 0, "ComposerHelper");
    qmlRegisterType<ComposerHelper>("org.kde.raven.private", 1, 0, "ViewerHelper");
//     qmlRegisterType<MessageViewer::DKIMMailStatus>("org.kde.raven.private", 1, 0, "DKIMMailStatus");
    qmlRegisterType<MessageViewer::DKIMCheckSignatureJob>("org.kde.raven.private", 1, 0, "DKIMCheckSignatureJob");
    qmlRegisterSingletonInstance<Raven>("org.kde.raven.private", 1, 0, "Raven", raven);
    
    qmlRegisterType<IdentityModel>("org.kde.raven.private", 1, 0, "IdentityModel");

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
