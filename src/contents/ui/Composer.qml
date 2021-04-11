// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Controls 2.15 as Controls
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.quickmail.private 1.0

Kirigami.Page {
    ComposerHelper {
        id : composerHelper
    }
    
    actions.main: Kirigami.Action {
        text: i18n("Send")
        icon.name: "document-send"
        onTriggered: {
            let toHeaders = [];
            let ccHeaders = [];
            let bccHeaders = [];
            for (let i = 0; i < headerModel.count; i++) {
                console.log(headerModel.get(i), headerModel.count, i)
                const element = headerModel.get(i);
                if (element.name === "to") {
                    toHeaders.push(element.value);
                }
                if (element.name === "cc") {
                    ccHeaders.push(element.value);
                }
                if (element.name === "bcc") {
                    bccHeaders.push(element.value);
                }
            }
            composerHelper.infoPart.to = toHeaders;
            composerHelper.infoPart.cc = ccHeaders;
            composerHelper.infoPart.bcc = bccHeaders;
            composerHelper.infoPart.subject = subject.text;
            //composerHelper.infoPart.from = from.text;
            
            composerHelper.textPart.cleanPlainText = textArea.text
            
            composerHelper.send();
        }
    }
    
    
    GridLayout {
        columns: 2
        anchors.fill: parent
        
        Controls.Label {
            text: i18n("Identity:")
            opacity: 0.8
        }
        
        Controls.ComboBox {
            id: identity
            model: IdentityModel {}
            textRole: "display"
            valueRole: "uoid"
        }
        
        Controls.Label {
            text: i18n("From:")
            opacity: 0.8
        }
        
        Controls.TextField {
            id: from
            Layout.fillWidth: true
            enabled: false
        }
        ListModel {
            id: headerModel
            ListElement {
                name: "to"
                value: ""
            }
        }
       
        Repeater {
            model: headerModel
            Controls.ComboBox {
                id: control
                Layout.row: index + 1
                Layout.column: 0
                property int modelWidth
                textRole: "text"
                valueRole: "value"
                implicitWidth: modelWidth + leftPadding + contentItem.leftPadding + rightPadding + contentItem.rightPadding + Kirigami.Units.gridUnit
                Component.onCompleted: {
                    currentIndex = indexOfValue(name)
                }
                TextMetrics {
                    id: textMetrics
                }
                model: [
                    { value: "to", text: i18n("To:") },
                    { value: "cc", text: i18n("CC:") },
                    { value: "bcc", text: i18n("BCC:") },
                    { value: "reply-to", text: i18n("Reply-To:") },
                ]
                onModelChanged: {
                    textMetrics.font = control.font
                    for (let i = 0; i < model.length; i++){
                        textMetrics.text = model[i].text
                        modelWidth = Math.max(textMetrics.width, modelWidth)
                    }
                }
            }
        }
            
        Repeater {
            model: headerModel
            Controls.TextField {
                Layout.fillWidth: true
                Layout.row: index + 1
                Layout.column: 1
                onTextChanged: {
                    if (index != 0 && text.trim().length === 0) {
                        headerModel.remove(index, 1);
                    }
                    if (index + 1 === headerModel.count) {
                        headerModel.append({name: "cc", value: ""})
                    }
                    headerModel.setProperty(index, "value", text);
                }
            }
        }
        
        Controls.Label {
            id: subject
            text: i18n("Subject:")
            opacity: 0.8
        }
        Controls.TextField {
            id: textArea
            Layout.fillWidth: true
        }
        
        Controls.TextArea {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
