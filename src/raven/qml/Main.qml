// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.raven

import "mailboxselector"

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Raven")

    width: 1200
    height: 600

    pageStack {
        globalToolBar {
            canContainHandles: true
            style: Kirigami.ApplicationHeaderStyle.ToolBar
            showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton
        }
        columnView {
            columnResizeMode: isWidescreen ? Kirigami.ColumnView.DynamicColumns : Kirigami.ColumnView.SingleColumn
        }
        initialPage: Qt.resolvedUrl("FolderView.qml")
    }

    property bool isWidescreen: root.width > 800

    Kirigami.PagePool {
        id: pagePool
    }

    function getPage(name) {
        switch (name) {
            case "FolderView": return pagePool.loadPage(Qt.resolvedUrl("FolderView.qml"))
            case "SettingsPage": return pagePool.loadPage(Qt.resolvedUrl("SettingsPage.qml"))
            case "AboutPage": return pagePool.loadPage(Qt.resolvedUrl("AboutPage.qml"))
        }
    }

    Connections {
        target: Raven

        // Handle opening a specific message from command-line arguments
        function onOpenThreadRequested(folder: Folder, thread: Thread) {
            const splitPath = folder.path.split("/")
            Raven.selectedFolderName = splitPath[splitPath.length - 1];
            Raven.mailListModel.loadFolder(folder);

            // Mark as read
            if (thread.unread) {
                Raven.mailListModel.markThreadAsRead(thread);
            }

            // Load thread in model
            Raven.threadViewModel.loadThread(thread);

            while (applicationWindow().pageStack.depth > 1) {
                applicationWindow().pageStack.pop();
            }

            applicationWindow().pageStack.push(Qt.resolvedUrl('ThreadViewer.qml'), {
                subject: thread.subject,
                thread: thread,
                folderRole: folder.role || ""
            });
        }
    }

    globalDrawer: MailBoxListSidebar {}

    // Daemon offline status banner
    header: ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        Kirigami.InlineMessage {
            id: daemonStatusBanner
            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            text: i18n("Email sync service is not running. Some features may be unavailable.")
            visible: !Raven.daemonStatus.available
            showCloseButton: false

            actions: [
                Kirigami.Action {
                    text: i18n("Retry")
                    icon.name: "view-refresh"
                    onTriggered: Raven.daemonStatus.activateDaemon()
                }
            ]
        }
    }
}
