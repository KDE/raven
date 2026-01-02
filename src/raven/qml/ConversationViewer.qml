// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.raven
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

import './mailpartview'

Kirigami.ScrollablePage {
    id: root

    property string subject
    property var thread

    Kirigami.ColumnView.interactiveResizeEnabled: true

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    actions: [
        Kirigami.Action {
            text: thread && thread.starred ? i18n("Unflag") : i18n("Flag")
            icon.name: "flag-symbolic"
            enabled: Raven.daemonStatus.available
            onTriggered: {
                if (thread) {
                    Raven.mailListModel.setThreadFlagged(thread, !thread.starred)
                }
            }
        },
        Kirigami.Action {
            text: thread && thread.unread ? i18n("Mark as read") : i18n("Mark as unread")
            icon.name: thread && thread.unread ? "mail-mark-read" : "mail-mark-unread"
            enabled: Raven.daemonStatus.available
            onTriggered: {
                if (thread) {
                    if (thread.unread) {
                        Raven.mailListModel.markThreadAsRead(thread)
                    } else {
                        Raven.mailListModel.markThreadAsUnread(thread)
                    }
                }
            }
        },
        Kirigami.Action {
            text: i18n("Move to trash")
            icon.name: "albumfolder-user-trash"
            enabled: Raven.daemonStatus.available
            onTriggered: {
                if (thread) {
                    Raven.mailListModel.moveThreadToTrash(thread)
                    // Go back to the folder view after moving to trash
                    applicationWindow().pageStack.pop()
                }
            }
        }
    ]

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            id: mailSubject
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit

            text: root.subject
            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Text.ElideRight

            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
        }


        Repeater {
            id: messageList
            model: Raven.threadViewModel

            delegate: MailViewer {
                Layout.fillWidth: true
                Layout.bottomMargin: Kirigami.Units.gridUnit * 2
                message: model.message  // Pass message for attachment loading
                subject: model.subject
                from: model.from
                to: model.to
                sender: "" // TODO props.sender
                dateTime: model.date
                content: model.content
                isPlaintext: model.isPlaintext
            }
        }

        Item {
            Layout.preferredHeight: Kirigami.Units.gridUnit * 2
        }
    }
}
