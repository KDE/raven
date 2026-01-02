// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.raven
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: folderView
    title: Raven.selectedFolderName

    Kirigami.ColumnView.interactiveResizeEnabled: true
    Kirigami.ColumnView.maximumWidth: Kirigami.Units.gridUnit * 50
    Kirigami.ColumnView.minimumWidth: Kirigami.Units.gridUnit * 10
    Kirigami.ColumnView.preferredWidth: Kirigami.Units.gridUnit * 20

    actions: [
        Kirigami.Action {
            text: i18n("Sync")
            icon.name: "view-refresh"
            visible: Raven.mailListModel.currentFolder !== null
            enabled: Raven.daemonStatus.available
            onTriggered: {
                if (Raven.mailListModel.currentFolder) {
                    Raven.triggerSyncForAccount(Raven.mailListModel.currentFolder.accountId)
                }
            }
        }
    ]

    ListView {
        id: mails
        model: Raven.mailListModel
        currentIndex: -1
        reuseItems: true

        Kirigami.PlaceholderMessage {
            id: mailboxSelected
            anchors.centerIn: parent
            visible: Raven.selectedFolderName === ""
            text: i18n("No mailbox selected")
            explanation: i18n("Select a mailbox from the sidebar.")
            icon.name: "mail-unread"
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: mails.count === 0 && !mailboxSelected.visible
            text: i18n("Mailbox is empty")
            icon.name: "mail-folder-inbox"
        }

        delegate: MailDelegate {
            width: mails.width
            showSeparator: model.index !== folderView.count - 1

            datetime: model.date
            author: model.from
            title: model.subject.length > 0 ? model.subject : i18nc("What is displayed when the email subject is empty", "(no subject)")
            contentPreview: model.snippet

            isStarred: model.starred
            isRead: !model.unread

            onOpenMailRequested: {
                Raven.threadViewModel.loadThread(model.thread);

                applicationWindow().pageStack.push(Qt.resolvedUrl('ConversationViewer.qml'), {
                    subject: model.subject,
                    thread: model.thread
                });
            }

            QQC2.ContextMenu.menu: QQC2.Menu {
                property int row: model.index

                modal: true

                QQC2.MenuItem {
                    icon.name: "flag-symbolic"
                    text: model.starred ? i18n("Unflag") : i18n("Flag")
                    enabled: Raven.daemonStatus.available
                    onTriggered: {
                        Raven.mailListModel.setThreadFlagged(model.thread, !model.starred)
                    }
                }
                QQC2.MenuItem {
                    icon.name: model.unread ? "mail-mark-read" : "mail-mark-unread"
                    text: model.unread ? i18n("Mark as read") : i18n("Mark as unread")
                    enabled: Raven.daemonStatus.available
                    onTriggered: {
                        if (model.unread) {
                            Raven.mailListModel.markThreadAsRead(model.thread)
                        } else {
                            Raven.mailListModel.markThreadAsUnread(model.thread)
                        }
                    }
                }
                QQC2.MenuItem {
                    icon.name: "delete"
                    text: i18n("Move to trash")
                    enabled: Raven.daemonStatus.available
                    onTriggered: {
                        Raven.mailListModel.moveThreadToTrash(model.thread)
                    }
                }
            }
        }
    }
}

