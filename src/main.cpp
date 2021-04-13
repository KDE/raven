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
#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Identity>
#include <QDebug>

#include "composerhelper.h"
#include "mailmodel.h"
#include "quickmail.h"
#include "identitymodel.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("KDE");
    QCoreApplication::setOrganizationDomain("kde.org");
    QCoreApplication::setApplicationName("KMailQuick");

    QQmlApplicationEngine engine;

    qRegisterMetaType<MailModel*>("MailModel*");
    qRegisterMetaType<MessageComposer::Composer *>("Composer *");
    qRegisterMetaType<MessageComposer::InfoPart *>("InfoPart *");
    qRegisterMetaType<MessageComposer::TextPart *>("TextPart *");
    qRegisterMetaType<MessageComposer::AttachmentModel *>("AttachmentModel *");
    qRegisterMetaType<KDescendantsProxyModel*>("KDescendantsProxyModel*");
    
    qmlRegisterType<ComposerHelper>("org.kde.quickmail.private", 1, 0, "ComposerHelper");
    qmlRegisterSingletonInstance<QuickMail>("org.kde.quickmail.private", 1, 0, "QuickMail", quickMail);
    
    qmlRegisterType<IdentityModel>("org.kde.quickmail.private", 1, 0, "IdentityModel");

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));
    
    
    KIdentityManagement::IdentityManager manager;
    for (const auto &identity : manager) {
        qDebug() << identity.uoid();
    }
    
    auto identityManager = KIdentityManagement::IdentityManager::self();
    qDebug() << identityManager->identities();
    for (const auto &identity : *identityManager) {
        qDebug() << identity.uoid();
    }

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
