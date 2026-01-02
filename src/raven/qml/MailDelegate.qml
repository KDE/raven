// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

QQC2.ItemDelegate {
    id: root

    property bool showSeparator: false

    property string datetime
    property string author
    property string title
    property string contentPreview

    property bool isStarred
    property bool isRead

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    hoverEnabled: true

    signal openMailRequested()

    property bool showSelected: (root.down || (root.highlighted && applicationWindow().isWidescreen))

    background: Rectangle {
        color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, root.showSelected ? 0.5 : (!Kirigami.Settings.isMobile && root.hovered) ? 0.2 : 0)

        // indicator rectangle
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1

            width: 4
            visible: !root.isRead
            color: Kirigami.Theme.highlightColor
        }

        Kirigami.Separator {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: root.leftPadding
            anchors.rightMargin: root.rightPadding
            visible: root.showSeparator && !root.showSelected
            opacity: 0.5
        }
    }

    onClicked: root.openMailRequested()

    contentItem: QQC2.Control {
        id: item
        leftPadding: Kirigami.Units.gridUnit
        rightPadding: Kirigami.Units.gridUnit
        topPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing

        Kirigami.Icon {
            id: starredIcon
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.largeSpacing

            source: "flag-red"
            implicitWidth: Kirigami.Units.iconSizes.small
            implicitHeight: Kirigami.Units.iconSizes.small
            visible: root.isStarred
        }

        contentItem: RowLayout {
            id: rowLayout

            Components.Avatar {
                // Euristic to extract name from "Name <email>" pattern
                name: author.replace(/<.*>/, '').replace(/\(.*\)/, '')
                // Extract and use email address as unique identifier for image provider
                source: 'image://contact/' + new RegExp("<(.*)>").exec(author)[1] ?? ''
                Layout.rightMargin: Kirigami.Units.largeSpacing
                sourceSize.width: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
                sourceSize.height: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
                Layout.preferredWidth: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
                Layout.preferredHeight: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: root.author
                        elide: Text.ElideRight
                        font.weight: root.isRead ? Font.Normal : Font.Bold
                    }

                    QQC2.Label {
                        font.pointSize: applicationWindow().isWidescreen ? Kirigami.Theme.defaultFont.pointSize : Kirigami.Theme.smallFont.pointSize
                        color: Kirigami.Theme.disabledTextColor
                        text: root.datetime
                    }
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: root.title
                    elide: Text.ElideRight
                    font.weight: root.isRead ? Font.Normal : Font.Bold
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: root.contentPreview
                    elide: Text.ElideRight
                    color: Kirigami.Theme.disabledTextColor
                }
            }
        }
    }
}
