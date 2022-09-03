// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.2

import org.kde.kirigami 2.19 as Kirigami
import org.kde.raven 1.0

Kirigami.AboutPage {
    id: aboutPage
    aboutData: AboutType.aboutData
}

