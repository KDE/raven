// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.raven 1.0 as Raven

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
        topPadding: Kirigami.Units.smallSpacing

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

                onAccepted: {
                    newAccount.addAccount();
                    applicationWindow().pageStack.layers.pop();
                }
                onRejected: applicationWindow().pageStack.layers.pop()
            }
        }

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor

            // separator above footer
            Kirigami.Separator {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
    }

    ColumnLayout {
        spacing: 0
        width: root.width

        Raven.NewAccountManager {
            id: newAccount
        }

        FormCard.FormCard {

            FormTextInputDelegate {
                id: nameDelegate
                text: i18n("Name")
                textValue: newAccount.name
                onTextSaved: newAccount.name = savedText
            }

            FormCard.FormDelegateSeparator { above: nameDelegate; below: emailDelegate }

            FormTextInputDelegate {
                id: emailDelegate
                text: i18n("Email")
                textValue: newAccount.email
                onTextSaved: newAccount.email = savedText
            }

            FormCard.FormDelegateSeparator { above: emailDelegate; below: passwordDelegate }

            FormPasswordInputDelegate {
                id: passwordDelegate
                text: i18n("Password")
                textValue: newAccount.password
                onTextSaved: newAccount.password = savedText
            }

            FormCard.FormDelegateSeparator { above: passwordDelegate; below: autoFillDelegate }

            FormCard.AbstractFormDelegate {
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

        FormCard.FormHeader {
            title: i18n("Receiving")
        }

        FormCard.FormCard {

            FormTextInputDelegate {
                id: receivingHostDelegate
                text: i18n("Host")
                textValue: newAccount.imapHost
                onTextSaved: newAccount.imapHost = savedText
            }

            FormCard.FormDelegateSeparator { above: receivingHostDelegate; below: receivingPortDelegate }

            FormTextInputDelegate {
                id: receivingPortDelegate
                text: i18n("Port")
                textValue: newAccount.imapPort
                onTextSaved: newAccount.imapPort = savedText
            }

            FormCard.FormDelegateSeparator { above: receivingPortDelegate; below: receivingUsernameDelegate }

            FormTextInputDelegate {
                id: receivingUsernameDelegate
                text: i18n("Username")
                textValue: newAccount.imapUsername
                onTextSaved: newAccount.imapUsername = savedText
            }

            FormCard.FormDelegateSeparator { above: receivingUsernameDelegate; below: receivingPasswordDelegate }

            FormPasswordInputDelegate {
                id: receivingPasswordDelegate
                text: i18n("Password")
                textValue: newAccount.imapPassword
                onTextSaved: newAccount.imapPassword = savedText
            }

            // TODO choose connection type and authentication
        }

        FormCard.FormHeader {
            title: i18n("Sending")
        }

        FormCard.FormCard {
            FormTextInputDelegate {
                id: smtpHostDelegate
                text: i18n("Host")
                textValue: newAccount.smtpHost
                onTextSaved: newAccount.smtpHost = savedText
            }

            FormCard.FormDelegateSeparator { above: smtpHostDelegate; below: smtpPortDelegate }

            FormTextInputDelegate {
                id: smtpPortDelegate
                text: i18n("Port")
                textValue: newAccount.smtpPort
                onTextSaved: newAccount.smtpPort = savedText
            }

            FormCard.FormDelegateSeparator { above: smtpPortDelegate; below: smtpUsernameDelegate }

            FormTextInputDelegate {
                id: smtpUsernameDelegate
                text: i18n("Username")
                textValue: newAccount.smtpUsername
                onTextSaved: newAccount.smtpUsername = savedText
            }

            FormCard.FormDelegateSeparator { above: smtpUsernameDelegate; below: smtpPasswordDelegate }

            FormPasswordInputDelegate {
                id: smtpPasswordDelegate
                text: i18n("Password")
                textValue: newAccount.smtpPassword
                onTextSaved: newAccount.smtpPassword = savedText
            }
        }
    }
}

