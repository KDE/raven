// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtGraphicalEffects 1.15

import org.kde.raven 1.0
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels

import './mailpartview'
import './private'

Item {
    id: root
    property var item
    property string subject
    property string from
    property string sender
    property string cc
    property string bcc
    property string to
    property string content
    property date dateTime
    property bool isPlaintext

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    property real padding: Kirigami.Units.largeSpacing * 2
    
    implicitHeight: column.implicitHeight
    
    Rectangle {
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }

    ColumnLayout {
        id: column
        spacing: 0
        
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        
        QQC2.ToolBar {
            id: mailHeader
            Layout.fillWidth: true
            padding: root.padding

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View

            background: Item {
                Rectangle {
                    anchors.fill: parent
                    color: Kirigami.Theme.alternateBackgroundColor
                }
                
                Kirigami.Separator {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                }
                
                Kirigami.Separator {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                }
            }

            ColumnLayout {
                width: mailHeader.width - mailHeader.leftPadding - mailHeader.rightPadding
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true
                    
                    QQC2.Label {
                        text: root.from
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        text: root.dateTime.toLocaleString(Qt.locale(), Locale.ShortFormat)
                        horizontalAlignment: Text.AlignRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    
                    QQC2.Label {
                        text: i18n("Sender:")
                        font.bold: true
                        visible: root.sender.length > 0 && root.sender !== root.from
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }

                    QQC2.Label {
                        visible: root.sender.length > 0 && root.sender !== root.from
                        text: root.sender
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    
                    QQC2.Label {
                        text: i18n("To:")
                        font.bold: true
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }

                    QQC2.Label {
                        text: root.to
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                
                RowLayout {
                    visible: root.cc !== ""
                    Layout.fillWidth: true
                    
                    QQC2.Label {
                        text: i18n("CC:")
                        font.bold: true
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }

                    QQC2.Label {
                        text: root.cc
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                
                RowLayout {
                    visible: root.bcc !== ""
                    Layout.fillWidth: true
                    
                    QQC2.Label {
                        text: i18n("BCC:")
                        font.bold: true
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }

                    QQC2.Label {
                        text: root.bcc
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            Layout.leftMargin: root.padding
            Layout.rightMargin: root.padding

            // plain text view
            Loader {
                active: root.isPlaintext
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.largeSpacing
                sourceComponent: TextPart {
                    content: root.content
                    textFormat: TextEdit.PlainText
                }
            }
            
            // html view
            Loader {
                active: !root.isPlaintext
                Layout.fillWidth: true
                sourceComponent: HtmlPart {
                    content: root.content
                }
            }
        }

        // QQC2.ToolBar {
        //     Layout.fillWidth: true
        //     padding: root.padding
        // 
        //     Kirigami.Theme.inherit: false
        //     Kirigami.Theme.colorSet: Kirigami.Theme.View
        // 
        //     background: Item {
        //         Kirigami.Separator {
        //             anchors {
        //                 left: parent.left
        //                 right: parent.right
        //                 top: undefined
        //                 bottom:  parent.bottom
        //             }
        //         }
        //     }

            // Flow {
            //     anchors.fill: parent
            //     spacing: Kirigami.Units.smallSpacing
            //     Repeater {
            //         model: mailPartView.attachmentModel
            // 
            //         delegate: AttachmentDelegate {
            //             name: model.name
            //             type: model.type
            //             icon.name: model.iconName
            // 
            //             clip: true
            // 
            //             actionIcon: 'download'
            //             actionTooltip: i18n("Save attachment")
            //             onExecute: mailPartView.attachmentModel.saveAttachmentToDisk(mailPartView.attachmentModel.index(index, 0))
            //             onClicked: mailPartView.attachmentModel.openAttachment(mailPartView.attachmentModel.index(index, 0))
            //             onPublicKeyImport: mailPartView.attachmentModel.importPublicKey(mailPartView.attachmentModel.index(index, 0))
            //         }
            //     }
            // }
        // }
        Kirigami.Separator { Layout.fillWidth: true }
    }
}
