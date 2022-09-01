// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.raven 1.0

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Raven")

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: FolderView {}

    globalDrawer: Kirigami.GlobalDrawer {
        title: "test"
        
        MailSidebar {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
