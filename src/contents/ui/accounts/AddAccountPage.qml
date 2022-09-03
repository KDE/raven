// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm
import org.kde.raven 1.0

Kirigami.ScrollablePage {
    id: root
    title: i18n("Add Account")
    
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    
    leftPadding: 0
    rightPadding: 0
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit
    
    footer: Controls.Control {
        id: footerToolBar

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                implicitContentHeight + topPadding + bottomPadding)

        leftPadding: Kirigami.Units.smallSpacing
        rightPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing
        topPadding: Kirigami.Units.smallSpacing + footerSeparator.implicitHeight

        contentItem: RowLayout {
            spacing: parent.spacing

            // footer buttons
            Controls.DialogButtonBox {
                // we don't explicitly set padding, to let the style choose the padding
                id: dialogButtonBox
                standardButtons: Controls.DialogButtonBox.Close | Controls.DialogButtonBox.Save

                Layout.fillWidth: true
                Layout.alignment: dialogButtonBox.alignment

                position: Controls.DialogButtonBox.Footer

                onAccepted: newAccount.addAccount()
                onRejected: applicationWindow().pageStack.layers.pop()
            }
        }

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
            // separator above footer
            Kirigami.Separator {
                id: footerSeparator
                visible: root.contentItem.height < root.contentItem.flickableItem.contentHeight
                width: parent.width
                anchors.top: parent.top
            }
        }
    }

    ColumnLayout {
        spacing: 0
        width: root.width
        
        NewAccount {
            id: newAccount
        }
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            
            contentItem: ColumnLayout {
                spacing: 0
                
                FormTextInputDelegate {
                    id: nameDelegate
                    text: i18n("Name")
                    textValue: newAccount.name
                    onTextSaved: newAccount.name = savedText
                }
                
                MobileForm.FormDelegateSeparator { above: nameDelegate; below: emailDelegate }
                
                FormTextInputDelegate {
                    id: emailDelegate
                    text: i18n("Email")
                    textValue: newAccount.email
                    onTextSaved: newAccount.email = savedText
                }
                
                MobileForm.FormDelegateSeparator { above: emailDelegate; below: passwordDelegate }
                
                FormTextInputDelegate {
                    id: passwordDelegate
                    text: i18n("Password")
                    textValue: newAccount.password
                    onTextSaved: newAccount.password = savedText
                }
                
                MobileForm.FormDelegateSeparator { above: passwordDelegate; below: autoFillDelegate }
                
                MobileForm.AbstractFormDelegate {
                    id: autoFillDelegate
                    Layout.fillWidth: true
                    contentItem: RowLayout {
                        Controls.Label {
                            Layout.fillWidth: true
                            text: i18n("Auto-fill settings")
                        }
                        
                        Controls.Button {
                            text: i18n("Fill")
                            icon.name: "search"
                            onClicked: newAccount.searchIspdbForConfig()
                        }
                    }
                }
            }
        }
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            
            contentItem: ColumnLayout {
                spacing: 0
                
                MobileForm.FormCardHeader {
                    title: i18n("Receiving")
                }
                
                MobileForm.FormComboBoxDelegate {
                    id: receiveEmailProtocolDelegate
                    text: i18n("Protocol")
                    currentValue: newAccount.receivingMailProtocol === NewAccount.Imap ? "IMAP" : "POP3"
                    model: ["IMAP", "POP3"]
                    dialogDelegate: Controls.RadioDelegate {
                        implicitWidth: Kirigami.Units.gridUnit * 16
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2
                        
                        text: modelData
                        checked: receiveEmailProtocolDelegate.currentValue == modelData
                        onCheckedChanged: {
                            if (checked) {
                                receiveEmailProtocolDelegate.currentValue = modelData;
                                newAccount.receivingMailProtocol = modelData === "IMAP" ? NewAccount.Imap : NewAccount.Pop3;
                            }
                        }
                    }

                }
                
                MobileForm.FormDelegateSeparator { above: receiveEmailProtocolDelegate; below: receivingHostDelegate }
                
                FormTextInputDelegate {
                    id: receivingHostDelegate
                    text: i18n("Host")
                    textValue: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapHost : newAccount.pop3Host
                    onTextSaved: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapHost = savedText : newAccount.pop3Host = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: receivingHostDelegate; below: receivingPortDelegate }
                
                FormTextInputDelegate {
                    id: receivingPortDelegate
                    text: i18n("Port")
                    textValue: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapPort : newAccount.pop3Port
                    onTextSaved: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapPort = savedText : newAccount.pop3Port = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: receivingPortDelegate; below: receivingUsernameDelegate }
                
                FormTextInputDelegate {
                    id: receivingUsernameDelegate
                    text: i18n("Username")
                    textValue: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapUsername : newAccount.pop3Username
                    onTextSaved: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapUsername = savedText : newAccount.pop3Username = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: receivingUsernameDelegate; below: receivingPasswordDelegate }
                
                FormTextInputDelegate {
                    id: receivingPasswordDelegate
                    text: i18n("Password")
                    textValue: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapPassword : newAccount.pop3Password
                    onTextSaved: newAccount.receivingMailProtocol === NewAccount.Imap ? newAccount.imapPassword = savedText : newAccount.pop3Password = savedText 
                }
            }
        }
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            
            contentItem: ColumnLayout {
                spacing: 0
                
                MobileForm.FormCardHeader {
                    title: i18n("Sending")
                }
                
                FormTextInputDelegate {
                    id: smtpHostDelegate
                    text: i18n("Host")
                    textValue: newAccount.smtpHost
                    onTextSaved: newAccount.smtpHost = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: smtpHostDelegate; below: smtpPortDelegate }
                
                FormTextInputDelegate {
                    id: smtpPortDelegate
                    text: i18n("Port")
                    textValue: newAccount.smtpPort
                    onTextSaved: newAccount.smtpPort = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: smtpPortDelegate; below: smtpUsernameDelegate }
                
                FormTextInputDelegate {
                    id: smtpUsernameDelegate
                    text: i18n("Username")
                    textValue: newAccount.smtpUsername
                    onTextSaved: newAccount.smtpUsername = savedText 
                }
                
                MobileForm.FormDelegateSeparator { above: smtpUsernameDelegate; below: smtpPasswordDelegate }
                
                FormTextInputDelegate {
                    id: smtpPasswordDelegate
                    text: i18n("Password")
                    textValue: newAccount.smtpPassword
                    onTextSaved: newAccount.smtpPassword = savedText 
                }
            }
        }
    }
}
