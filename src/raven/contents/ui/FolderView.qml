// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.raven 1.0 as Raven
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels

Kirigami.ScrollablePage {
    id: folderView
    title: Raven.Raven.selectedFolderName
    
    Component {
        id: contextMenu
        QQC2.Menu {
            property int row
            property var status

            QQC2.Menu {
                title: i18nc("@action:menu", "Mark Message")
                QQC2.MenuItem {
                    text: i18n("Mark Message as Read")
                }
                QQC2.MenuItem {
                    text: i18n("Mark Message as Unread")
                }

                QQC2.MenuSeparator {}

                QQC2.MenuItem {
                    text: status.isImportant ? i18n("Don't Mark as Important") : i18n("Mark as Important")
                }
            }

            QQC2.MenuItem {
                icon.name: 'delete'
                text: i18n("Move to trash")
            }

            QQC2.MenuItem {
                icon.name: 'edit-move'
                text: i18n("Move Message to...")
            }

            QQC2.MenuItem {
                icon.name: 'edit-copy'
                text: i18n("Copy Message to...")
            }

            QQC2.MenuItem {
                icon.name: 'edit-copy'
                text: i18n("Add Followup Reminder")
            }
        }
    }

    ListView {
        id: mails
        model: Raven.MailListModel
        currentIndex: -1
        reuseItems: true
        
        Kirigami.PlaceholderMessage {
            id: mailboxSelected
            anchors.centerIn: parent
            visible: Raven.Raven.selectedFolderName === ""
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
            showSeparator: model.index !== folderView.count - 1
            
            datetime: model.date
            author: model.from
            title: model.subject
            contentPreview: model.snippet
            
            isRead: !model.unread
            
            onOpenMailRequested: {
                Raven.ThreadViewModel.loadThread(model.thread);
                
                applicationWindow().pageStack.push(Qt.resolvedUrl('ConversationViewer.qml'), {subject: model.subject});
            }
            
//             onStarMailRequested: {
//                 const status = MailManager.folderModel.copyMessageStatus(model.status);
//                 status.isImportant = !status.isImportant;
//                 MailManager.folderModel.updateMessageStatus(index, status)
//             }
            
            onContextMenuRequested: {
                const menu = contextMenu.createObject(folderView, {
                    row: index,
                    status: MailManager.folderModel.copyMessageStatus(model.status),
                });
                menu.popup();
            }
        }
    }
}
