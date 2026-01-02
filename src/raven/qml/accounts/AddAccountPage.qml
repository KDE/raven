// SPDX-FileCopyrightText: 2022-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.raven

Kirigami.ScrollablePage {
    id: root
    title: i18n("Add Account")

    property bool settingsDetected: false

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
                standardButtons: Controls.DialogButtonBox.Cancel | Controls.DialogButtonBox.Save

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

        NewAccountManager {
            id: newAccount
        }

        FormCard.FormCard {

            FormTextInputDelegate {
                id: emailDelegate
                text: i18n("Email")
                textValue: newAccount.email
                onTextSaved: newAccount.email = savedText
            }

            FormCard.FormDelegateSeparator { above: emailDelegate; below: nameDelegate }

            FormTextInputDelegate {
                id: nameDelegate
                text: i18n("Name")
                textValue: newAccount.name
                onTextSaved: newAccount.name = savedText
            }

            FormCard.FormDelegateSeparator { above: nameDelegate; below: autoFillDelegate }

            FormCard.FormTextDelegate {
                id: autoFillDelegate
                text: i18n("Auto-fill settings")
                trailing: Controls.Button {
                    text: i18n("Fill")
                    icon.name: "search"
                    onClicked: {
                        newAccount.searchIspdbForConfig();
                        root.settingsDetected = true;
                    }
                }
            }

            FormCard.FormDelegateSeparator { above: autoFillDelegate; below: passwordDelegate; visible: passwordDelegate.visible }

            FormPasswordInputDelegate {
                id: passwordDelegate
                visible: !newAccount.usesOAuth && root.settingsDetected
                text: i18n("Password")
                textValue: newAccount.password
                onTextSaved: newAccount.password = savedText
            }

            FormCard.FormDelegateSeparator { above: autoFillDelegate; below: oauthDelegate; visible: oauthDelegate.visible }

            FormCard.AbstractFormDelegate {
                id: oauthDelegate
                visible: newAccount.usesOAuth && newAccount.oauthProviderName.length > 0
                background: null
                Layout.fillWidth: true
                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Controls.Label {
                            Layout.fillWidth: true
                            text: newAccount.hasOAuthTokens
                                ? i18n("%1 account connected", newAccount.oauthProviderName)
                                : i18n("Sign in with your %1 account", newAccount.oauthProviderName)
                            color: newAccount.hasOAuthTokens ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                        }

                        Controls.Button {
                            text: newAccount.hasOAuthTokens
                                ? i18n("Re-authenticate")
                                : i18n("Sign in with %1", newAccount.oauthProviderName)
                            enabled: !newAccount.oauthInProgress
                            onClicked: newAccount.startOAuth()
                        }

                        Controls.Button {
                            text: i18n("Cancel")
                            visible: newAccount.oauthInProgress
                            onClicked: newAccount.cancelOAuth()
                        }
                    }

                    Controls.Label {
                        visible: newAccount.oauthInProgress
                        text: i18n("Waiting for authentication in browser...")
                        font.italic: true
                        opacity: 0.7
                    }

                    Controls.Label {
                        visible: newAccount.oauthErrorMessage.length > 0
                        text: newAccount.oauthErrorMessage
                        color: Kirigami.Theme.negativeTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        FormCard.FormHeader {
            title: i18n("Receiving (IMAP)")
            visible: root.settingsDetected
        }

        FormCard.FormCard {
            visible: root.settingsDetected

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

            FormCard.FormDelegateSeparator { above: receivingUsernameDelegate; below: receivingPasswordDelegate; visible: !newAccount.hasOAuthTokens }

            FormPasswordInputDelegate {
                id: receivingPasswordDelegate
                visible: !newAccount.hasOAuthTokens
                text: i18n("Password")
                textValue: newAccount.imapPassword
                onTextSaved: newAccount.imapPassword = savedText
            }

            FormCard.FormDelegateSeparator { above: receivingUsernameDelegate; below: receivingOAuthIndicator; visible: newAccount.hasOAuthTokens }

            FormCard.FormTextDelegate {
                id: receivingOAuthIndicator
                visible: newAccount.hasOAuthTokens
                text: i18n("Authentication")
                trailing: Controls.Label {
                    text: i18n("OAuth2 (%1)", newAccount.oauthProviderName)
                    color: Kirigami.Theme.positiveTextColor
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        FormCard.FormHeader {
            title: i18n("Sending (SMTP)")
            visible: root.settingsDetected
        }

        FormCard.FormCard {
            visible: root.settingsDetected

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

            FormCard.FormDelegateSeparator { above: smtpUsernameDelegate; below: smtpPasswordDelegate; visible: !newAccount.hasOAuthTokens }

            FormPasswordInputDelegate {
                id: smtpPasswordDelegate
                visible: !newAccount.hasOAuthTokens
                text: i18n("Password")
                textValue: newAccount.smtpPassword
                onTextSaved: newAccount.smtpPassword = savedText
            }

            FormCard.FormDelegateSeparator { above: smtpUsernameDelegate; below: smtpOAuthIndicator; visible: newAccount.hasOAuthTokens }

            FormCard.FormTextDelegate {
                id: smtpOAuthIndicator
                visible: newAccount.hasOAuthTokens
                text: i18n("Authentication")
                trailing: Controls.Label {
                    Layout.fillWidth: true
                    text: i18n("OAuth2 (%1)", newAccount.oauthProviderName)
                    color: Kirigami.Theme.positiveTextColor
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }
}
