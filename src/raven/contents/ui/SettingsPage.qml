// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm
import org.kde.raven 1.0 as Raven

Kirigami.ScrollablePage {
    id: root
    title: i18n("Settings")
    
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    
    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    ColumnLayout {
        spacing: 0
        width: root.width
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            
            contentItem: ColumnLayout {
                spacing: 0
                
                MobileForm.FormCardHeader {
                    title: i18n("General")
                }
                
                MobileForm.FormButtonDelegate {
                    id: aboutDelegate
                    text: i18n("About")
                    onClicked: applicationWindow().pageStack.layers.push(applicationWindow().getPage("AboutPage"))
                }
            }
        }
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            
            contentItem: ColumnLayout {
                spacing: 0
                
                MobileForm.FormCardHeader {
                    title: i18n("Accounts")
                }
                
                Repeater {
                    id: repeater
                    model: Raven.AccountModel

                    delegate: MobileForm.AbstractFormDelegate {
                        id: delegate

                        property Raven.Account account: model.account

                        Layout.fillWidth: true

                        Loader {
                            id: dialogLoader
                            sourceComponent: Kirigami.PromptDialog {
                                id: dialog
                                title: i18n("Configure %1", model.email)
                                subtitle: i18n("Modify or delete this account.")
                                standardButtons: Kirigami.Dialog.NoButton
                                
                                customFooterActions: [
                                    Kirigami.Action {
                                        text: i18n("Modify")
                                        iconName: "edit-entry"
                                        onTriggered: {
                                            // TODO
                                            dialog.close();
                                        }
                                    },
                                    Kirigami.Action {
                                        text: i18n("Delete")
                                        iconName: "delete"
                                        onTriggered: {
                                            dialog.close();
                                            Raven.AccountModel.removeAccount(model.index);
                                        }
                                    }
                                ]
                            }
                        }
                        
                        onClicked: {
                            dialogLoader.active = true;
                            dialogLoader.item.open();
                        }
                        
                        contentItem: RowLayout {
                            Kirigami.Icon {
                                source: "folder-mail"
                                Layout.rightMargin: Kirigami.Units.largeSpacing
                                implicitWidth: Kirigami.Units.iconSizes.medium
                                implicitHeight: Kirigami.Units.iconSizes.medium
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing
                                
                                Controls.Label {
                                    Layout.fillWidth: true
                                    text: account.email
                                    elide: Text.ElideRight
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    color: Kirigami.Theme.textColor
                                }
                                
                                Controls.Label {
                                    Layout.fillWidth: true
                                    text: account.name
                                    color: Kirigami.Theme.disabledTextColor
                                    font: Kirigami.Theme.smallFont
                                    elide: Text.ElideRight
                                }
                            }
                            
                            MobileForm.FormArrow {
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                direction: MobileForm.FormArrow.Right
                            }
                        }
                    }
                }
                
                MobileForm.FormDelegateSeparator { visible: repeater.count > 0; below: addAccountDelegate }
                
                MobileForm.FormButtonDelegate {
                    id: addAccountDelegate
                    text: i18n("Add Account")
                    icon.name: "list-add"
                    onClicked: applicationWindow().pageStack.layers.push("qrc:/accounts/AddAccountPage.qml")
                }
            }
        }
    }
}
