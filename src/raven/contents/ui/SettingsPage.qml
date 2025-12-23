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
                model: Raven.AccountModel

                delegate: FormCard.AbstractFormDelegate {
                    id: delegate

                    property Raven.Account account: model.account

                    Layout.fillWidth: true

                    Loader {
                        id: dialogLoader
                        sourceComponent: Kirigami.PromptDialog {
                            id: dialog
                            title: i18n("Configure %1", account.email)
                            subtitle: i18n("Modify or delete this account.")
                            standardButtons: Kirigami.Dialog.NoButton

                            customFooterActions: [
                                Kirigami.Action {
                                    text: i18n("Modify")
                                    icon.name: "edit-entry"
                                    onTriggered: {
                                        // TODO
                                        dialog.close();
                                    }
                                },
                                Kirigami.Action {
                                    text: i18n("Delete")
                                    icon.name: "delete"
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

                        FormCard.FormArrow {
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            direction: FormCard.FormArrow.Right
                        }
                    }
                }
            }

            FormCard.FormDelegateSeparator { visible: repeater.count > 0; below: addAccountDelegate }

            FormCard.FormButtonDelegate {
                id: addAccountDelegate
                text: i18n("Add Account")
                icon.name: "list-add"
                onClicked: applicationWindow().pageStack.layers.push("qrc:/accounts/AddAccountPage.qml")
            }
        }
    }
}
