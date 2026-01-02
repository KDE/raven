// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.raven
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.AboutPage {
    id: aboutPage
    aboutData: AboutType.aboutData
}

