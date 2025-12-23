// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import Qt.labs.qmlmodels 1.0

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kitemmodels 1.0
import org.kde.raven 1.0 as Raven

ListView {
    id: mailList

    model: Raven.MailBoxModel

    onModelChanged: currentIndex = -1

    signal folderChosen()

    delegate: DelegateChooser {
        role: 'isCollapsible'

        DelegateChoice {
            roleValue: true

            ColumnLayout {
                visible: model.visible
                spacing: 0
                height: visible ? implicitHeight : 0
                width: ListView.view.width

                Item {
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    visible: (model.level === 0) && (model.index !== 0)
                }

                QQC2.ItemDelegate {
                    id: categoryHeader
                    Layout.fillWidth: true
                    topPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.largeSpacing
                    leftPadding: Kirigami.Units.largeSpacing * (model.level + 1)

                    Kirigami.Theme.colorSet: applicationWindow().isWidescreen ? Kirigami.Theme.Window : Kirigami.Theme.View
                    Kirigami.Theme.inherit: false

                    property string displayText: model.name

                    onClicked: mailList.model.toggleCollapse(index)

                    contentItem: RowLayout {
                        Kirigami.Icon {
                            Layout.alignment: Qt.AlignVCenter
                            visible: model.level > 0
                            source: "folder-symbolic"
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            Layout.preferredWidth: Layout.preferredHeight
                        }

                        QQC2.Label {
                            Layout.fillWidth: true

                            color: Kirigami.Theme.disabledTextColor
                            text: categoryHeader.displayText
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Kirigami.Icon {
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            source: !model.isCollapsed ? 'arrow-up' : 'arrow-down'
                        }
                    }
                }
            }
        }

        DelegateChoice {
            roleValue: false

            QQC2.ItemDelegate {
                id: controlRoot
                visible: model.visible
                text: model.name

                height: visible ? implicitHeight : 0
                width: ListView.view.width

                topPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.largeSpacing
                rightPadding: Kirigami.Units.largeSpacing
                leftPadding: Kirigami.Units.largeSpacing * (model.level + 1)

                property bool chosen: false

                Connections {
                    target: mailList

                    function onFolderChosen() {
                        if (controlRoot.chosen) {
                            controlRoot.chosen = false;
                            controlRoot.highlighted = true;
                        } else {
                            controlRoot.highlighted = false;
                        }
                    }
                }

                property bool showSelected: (controlRoot.pressed || (controlRoot.highlighted && applicationWindow().isWidescreen))

                // background: Rectangle {
                //     color: Qt.rgba(
                //         Kirigami.Theme.highlightColor.r,
                //         Kirigami.Theme.highlightColor.g,
                //         Kirigami.Theme.highlightColor.b,
                //         controlRoot.showSelected ? 0.5 : (controlRoot.hovered && !Kirigami.Settings.isMobile) ? 0.2 : 0)
                // }

                // indicator rectangle
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: 1
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1

                    width: 4
                    visible: controlRoot.highlighted
                    color: Kirigami.Theme.highlightColor
                }

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        Layout.alignment: Qt.AlignVCenter
                        source: "mail-folder-inbox" // model.decoration // TODO
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        Layout.preferredWidth: Layout.preferredHeight
                    }

                    QQC2.Label {
                        leftPadding: controlRoot.mirrored ? (controlRoot.indicator ? controlRoot.indicator.width : 0) + controlRoot.spacing : 0
                        rightPadding: !controlRoot.mirrored ? (controlRoot.indicator ? controlRoot.indicator.width : 0) + controlRoot.spacing : 0

                        text: controlRoot.text
                        font: controlRoot.font
                        color: Kirigami.Theme.textColor
                        elide: Text.ElideRight
                        visible: controlRoot.text
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignLeft
                        Layout.fillWidth: true
                    }
                }

                onClicked: {
                    Raven.Raven.selectedFolderName = model.name;
                    Raven.MailListModel.loadFolder(model.folder);

                    controlRoot.chosen = true;
                    mailList.folderChosen();

                    // push list page if in narrow mode
                    if (!applicationWindow().isWidescreen) {
                        applicationWindow().pageStack.push(applicationWindow().getPage("FolderView"));
                    }
                }
            }
        }
    }
}
