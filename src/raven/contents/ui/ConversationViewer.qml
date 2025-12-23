// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.raven 1.0
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.raven 1.0 as Raven

import './mailpartview'

Kirigami.ScrollablePage {
    id: root

    property string subject

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    actions: [
        Kirigami.Action {
            text: i18n("Move to trash")
            icon.name: "albumfolder-user-trash"
            // TODO implement move to trash
        }
    ]

    ListView {
        id: listView
        spacing: Kirigami.Units.gridUnit * 2
        model: Raven.ThreadViewModel

        header: RowLayout {
            id: rowLayout
            width: listView.width

            QQC2.Label {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing * 2
                Layout.rightMargin: Kirigami.Units.largeSpacing * 2
                Layout.topMargin: Kirigami.Units.gridUnit
                Layout.bottomMargin: Kirigami.Units.gridUnit

                text: root.subject
                maximumLineCount: 2
                wrapMode: Text.Wrap
                elide: Text.ElideRight

                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
            }
        }

        delegate: MailViewer {
            width: listView.width
            item: root.item
            subject: model.subject
            from: model.from
            to: model.to
            sender: "" // TODO props.sender
            dateTime: model.date
            content: model.content
            isPlaintext: model.isPlaintext
        }

        // footer spacing
        footer: Item {
            width: listView.width
            height: Kirigami.Units.gridUnit * 2
        }
    }
}
