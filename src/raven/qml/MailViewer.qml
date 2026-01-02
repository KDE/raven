// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.raven
import org.kde.kirigami as Kirigami

import './mailpartview'
import './private'

Item {
    id: root
    property Message message  // Message object for loading attachments
    property string subject
    property string from
    property string sender
    property string cc
    property string bcc
    property string to
    property string content
    property date dateTime
    property bool isPlaintext

    // Load attachments when message changes
    Component.onCompleted: {
        if (message) {
            Raven.attachmentModel.loadMessage(message);
        }
    }

    onMessageChanged: {
        if (message) {
            Raven.attachmentModel.loadMessage(message);
        }
    }

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

        // Attachment toolbar - only visible when there are non-inline attachments
        QQC2.ToolBar {
            Layout.fillWidth: true
            padding: root.padding
            visible: Raven.attachmentModel.nonInlineCount > 0

            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.View

            background: Item {
                Rectangle {
                    anchors.fill: parent
                    color: Kirigami.Theme.alternateBackgroundColor
                }
                Kirigami.Separator {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                    }
                }
            }

            ColumnLayout {
                width: parent.width - parent.leftPadding - parent.rightPadding
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18n("Attachments (%1)", Raven.attachmentModel.nonInlineCount)
                    font.bold: true
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: Raven.attachmentModel

                        delegate: AttachmentDelegate {
                            // Only show non-inline attachments
                            visible: !model.isInline
                            width: visible ? implicitWidth : 0
                            height: visible ? implicitHeight : 0

                            name: model.fileName + " (" + model.formattedSize + ")"
                            type: model.contentType
                            icon.name: model.iconName

                            clip: true

                            actionIcon: model.downloaded ? 'document-open' : 'download'
                            actionTooltip: model.downloaded ? i18n("Open attachment") : i18n("Download attachment")
                            onExecute: {
                                Raven.attachmentModel.openAttachment(model.index);
                            }
                            onSave: {
                                Raven.attachmentModel.saveAttachment(model.index);
                            }
                            onPublicKeyImport: {
                                // TODO: Implement PGP key import
                                console.log("Import public key: " + model.fileId);
                            }
                        }
                    }
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }
    }
}
