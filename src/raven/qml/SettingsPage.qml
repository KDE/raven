// SPDX-FileCopyrightText: 2022-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.raven

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

        FormCard.FormHeader {
            title: i18n("General")
        }

        FormCard.FormCard {
            Layout.fillWidth: true

            FormCard.FormButtonDelegate {
                id: aboutDelegate
                text: i18n("About")
                onClicked: applicationWindow().pageStack.layers.push(applicationWindow().getPage("AboutPage"))
            }
        }

        FormCard.FormHeader {
            title: i18n("Accounts")
        }

        FormCard.FormCard {
            Layout.fillWidth: true

            Repeater {
                id: repeater
                model: Raven.accountModel

                delegate: FormCard.FormTextDelegate {
                    id: delegate

                    property Account account: model.account

                    icon.name: "folder-mail"
                    text: account.email
                    description: account.name

                    trailing: QQC2.ToolButton {
                        text: i18n("Delete")
                        display: QQC2.ToolButton.IconOnly
                        icon.name: "delete"
                        onClicked: {
                            dialogLoader.active = true;
                            dialogLoader.item.open();
                        }
                    }

                    Loader {
                        id: dialogLoader
                        sourceComponent: Kirigami.PromptDialog {
                            id: dialog
                            title: i18n("Confirm Account Deletion")
                            subtitle: i18n("Are you sure want to delete the account %1?", account.email)
                            standardButtons: Kirigami.Dialog.Cancel | Kirigami.Dialog.Ok

                            onAccepted: {
                                Raven.accountModel.removeAccount(model.index);
                            }
                        }
                    }
                }
            }

            FormCard.FormDelegateSeparator { visible: repeater.count > 0; below: addAccountDelegate }

            FormCard.FormButtonDelegate {
                id: addAccountDelegate
                text: i18n("Add Account")
                icon.name: "list-add"
                onClicked: applicationWindow().pageStack.layers.push(Qt.resolvedUrl("accounts/AddAccountPage.qml"))
            }
        }
    }
}
