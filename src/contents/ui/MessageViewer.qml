// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.raven.private 1.0

Kirigami.ScrollablePage {

    required property var viewerHelper

    titleDelegate: Row {
        Controls.ToolButton {
            text: i18nc('Reply to Email', 'Reply')
            icon.name: 'mail-reply-sender'
        }
        Controls.ToolButton {
            text: i18nc('Forward Email', 'Forward')
            icon.name: 'mail-forward'
        }
    }

    actions.main: Kirigami.Action {
        text: i18nc('Move email to trash', 'Move to trash')
        icon.name: 'albumfolder-user-trash'
    }

    Kirigami.OverlaySheet {
        id: linkOpenDialog
        property string link
        header: Kirigami.Heading {
            text: i18n("Open Link")
        }
        contentItem: Controls.Label {
            text: i18n("Do you really want to open '%1'?", linkOpenDialog.link)
            wrapMode: Text.Wrap
        }
        footer: Controls.DialogButtonBox {
            standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
            onAccepted: Qt.openUrlExternally(linkOpenDialog.link)
        }
    }
    Kirigami.Theme.colorSet : Kirigami.Theme.View

    header: Controls.Pane {
        Kirigami.Theme.colorSet : Kirigami.Theme.Window
        ColumnLayout {
            Row {
                Kirigami.Heading {
                    level: 2
                    text: viewerHelper.subject
                }
            }

            RowLayout {
                Kirigami.Avatar {
                    width: Kirigami.Units.gridUnit * 3
                    height: width
                    name: viewerHelper.from
                    Layout.topMargin: Kirigami.Units.gridUnit 
                }

                Kirigami.FormLayout {
                    Layout.leftMargin: Kirigami.Units.gridUnit 
                    Controls.Label {
                        Kirigami.FormData.label: i18n("From:")
                        text: viewerHelper.from
                    }
                    Controls.Label {
                        Kirigami.FormData.label: i18n("To:")
                        text: viewerHelper.to.join(', ')
                    }
                    Controls.Label {
                        visible: viewerHelper.sender !== viewerHelper.from && viewerHelper.sender.length > 0
                        Kirigami.FormData.label: i18n("Sender:")
                        text: viewerHelper.sender
                    }
                    Controls.Label {
                        Kirigami.FormData.label: i18n("Date:")
                        text: viewerHelper.date.toLocaleDateString()
                    }
                }
            }
            //Kirigami.InlineMessage {
                //DKIMMailStatus {
                    //id: dkimMailStatus
                    //currentItemId: viewerHelper.itemId
                //}
                //visible: DKIMCheckSignatureJob.DKIMStatus.Valid !== dkimMailStatus.status
                //function escapeHtml(unsafe) {
                    //return unsafe
                         //.replace(/&/g, "&amp;")
                         //.replace(/</g, "&lt;")
                         //.replace(/>/g, "&gt;")
                         //.replace(/"/g, "&quot;")
                         //.replace(/'/g, "&#039;");
                //}
                //text: dkimMailStatus.text + '<br />' + escapeHtml(dkimMailStatus.tooltip)
                //type: {
                    //console.log(dkimMailStatus.status, DKIMCheckSignatureJob.DKIMStatus.Valid)
                    //switch (dkimMailStatus.status) {
                    //case DKIMCheckSignatureJob.DKIMStatus.Invalid:
                        //return Kirigami.MessageType.Error;
                    //case DKIMCheckSignatureJob.DKIMStatus.EmailNotSigned:
                    //case DKIMCheckSignatureJob.DKIMStatus.NeedToBeSigned:
                        //return Kirigami.MessageType.Error;
                    //}
                    //return Kirigami.MessageType.Information;
                //}
            //}
        }
    }

    Controls.TextArea {
        background: Item {}
        textFormat: TextEdit.AutoText
        Layout.fillWidth: true
        readOnly: true
        selectByMouse: true
        text: viewerHelper.content
        wrapMode: Text.Wrap
        Controls.ToolTip.text: hoveredLink
        // TODO FIXME sometimes the tooltip is visible even when the link is empty
        Controls.ToolTip.visible: hoveredLink && hoveredLink.trim().length > 0
        Controls.ToolTip.delay: Kirigami.Units.shortDuration
        onLinkActivated: {
            linkOpenDialog.link = link
            linkOpenDialog.open();
        }
    }
}
