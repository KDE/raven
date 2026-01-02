// SPDX-FileCopyrightText: 2022 Felipe Kinoshita <kinofhek@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <KAboutData>

#include <qqmlintegration.h>
#include <QQmlEngine>
#include <QJSEngine>

class AboutType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(KAboutData aboutData READ aboutData CONSTANT)

public:
    static AboutType *instance()
    {
        static AboutType *_instance = new AboutType();
        return _instance;
    }
    static AboutType *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        AboutType *model = instance();
        QQmlEngine::setObjectOwnership(model, QQmlEngine::CppOwnership);
        return model;
    }

    [[nodiscard]] KAboutData aboutData() const
    {
        return KAboutData::applicationData();
    }
};
