// SPDX-FileCopyrightText: 2022 Felipe Kinoshita <kinofhek@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <KAboutData>

class AboutType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KAboutData aboutData READ aboutData CONSTANT)
public:
    static AboutType &instance()
    {
        static AboutType _instance;
        return _instance;
    }

    [[nodiscard]] KAboutData aboutData() const
    {
        return KAboutData::applicationData();
    }
};
