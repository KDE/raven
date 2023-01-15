// SPDX-FileCopyrightText: 2023 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCommandLineParser>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>

#include "dbmanager.h"
#include "workermanager.h"
#include "version.h"
#include "../libraven/constants.h"

QCommandLineParser *createParser()
{
    QCommandLineParser *parser = new QCommandLineParser;
    parser->addHelpOption();
    return parser;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    KLocalizedString::setApplicationDomain("ravend");
    KAboutData aboutData(QStringLiteral("ravend"),
                         QStringLiteral("Raven background daemon"),
                         QStringLiteral(RAVEN_VERSION_STRING),
                         QStringLiteral("Raven background daemon"),
                         KAboutLicense::GPL,
                         i18n("© 2023 KDE Community"));
    aboutData.addAuthor(i18n("Devin Lin"), QLatin1String(), QStringLiteral("devin@kde.org"));
    KAboutData::setApplicationData(aboutData);

    // parse command line arguments ~~~~
    {
        QScopedPointer<QCommandLineParser> parser(createParser());
        parser->process(app);
    }
    
    // initialize data folders
    if (!QDir().mkpath(RAVEN_DATA_LOCATION)) {
        qWarning() << "Could not create database directory at" << RAVEN_DATA_LOCATION;
    }

    if (!QDir().mkpath(RAVEN_CONFIG_LOCATION)) {
        qWarning() << "Could not create config folder at" << RAVEN_CONFIG_LOCATION;
    }
    
    if (!QDir().mkpath(RAVEN_DATA_LOCATION + QStringLiteral("/files"))) {
        qWarning() << "Could not create files folder at" << RAVEN_DATA_LOCATION + QStringLiteral("/files/");
    }
    
    KDBusService service(KDBusService::Unique);
    
    qDebug() << "Starting ravend" << RAVEN_VERSION_STRING;

    // run database migrations first
    // constructor enables sql connection for main thread
    DBManager::self()->migrate();
    
    // start worker manager
    WorkerManager::self();
    
    return app.exec();
}
