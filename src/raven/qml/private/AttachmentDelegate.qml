// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

QQC2.AbstractButton {
    id: root

    property string name
    property string type
    property alias actionIcon: actionButton.icon.name
    property alias actionTooltip: actionButtonTooltip.text

    signal execute()
    signal save()
    signal publicKeyImport()

    Kirigami.Theme.colorSet: Kirigami.Theme.Button
    Kirigami.Theme.inherit: false

    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    contentItem: RowLayout {
        id: content
        spacing: Kirigami.Units.smallSpacing

        Rectangle {
            color: Kirigami.Theme.backgroundColor
            Layout.preferredHeight: Kirigami.Units.gridUnit
            Layout.preferredWidth: Kirigami.Units.gridUnit
            Kirigami.Icon {
                anchors.verticalCenter: parent.verticalCenter
                height: Kirigami.Units.gridUnit
                width: Kirigami.Units.gridUnit
                source: root.icon.name
            }
        }
        QQC2.Label {
            text: root.name
        }
        QQC2.ToolButton {
            visible: root.type === "application/pgp-keys"
            icon.name: 'gpg'
            onClicked: root.publicKeyImport()
            QQC2.ToolTip {
                text: i18n("Import key")
            }
        }
        QQC2.ToolButton {
            id: actionButton
            onClicked: root.execute()
            QQC2.ToolTip {
                id: actionButtonTooltip
            }
        }
        QQC2.ToolButton {
            icon.name: 'document-save'
            onClicked: root.save()
            QQC2.ToolTip {
                text: i18n("Save attachment")
            }
        }
    }
}
