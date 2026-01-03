// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtWebEngine
import QtWebChannel

import org.kde.raven
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root

    property string subject
    property var thread
    property string folderRole: ""

    Kirigami.ColumnView.interactiveResizeEnabled: true

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    // Check if current theme is dark based on background color luminance
    function isDarkTheme() {
        return Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark;
    }

    // Build CSS variables from Kirigami theme colors
    function buildThemeCSS() {
        let colorScheme = isDarkTheme() ? "dark" : "light"
        return ":root {" +
            " --background-color: " + Kirigami.Theme.backgroundColor + ";" +
            " --alt-background-color: " + windowColorProxy.Kirigami.Theme.backgroundColor + ";" +
            " --text-color: " + Kirigami.Theme.textColor + ";" +
            " --secondary-text-color: " + Kirigami.Theme.disabledTextColor + ";" +
            " --border-color: " + Qt.darker(Kirigami.Theme.backgroundColor, 1.2) + ";" +
            " --accent-color: " + Kirigami.Theme.highlightColor + ";" +
            " --link-color: " + Kirigami.Theme.linkColor + ";" +
            " --hover-background: " + (colorScheme === "dark" ? Qt.lighter(windowColorProxy.Kirigami.Theme.backgroundColor, 1.2) : Qt.darker(windowColorProxy.Kirigami.Theme.backgroundColor, 1.1)) + ";" +
            " --color-scheme: " + colorScheme + ";" +
            " color-scheme: " + colorScheme + ";" +
            "}";
    }

    Item {
        id: windowColorProxy
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false
    }

    // Update theme CSS on bridge
    function updateTheme() {
        threadBridge.themeCSS = buildThemeCSS();
    }

    // Update theme when Kirigami colors change
    Connections {
        target: Kirigami.Theme
        function onBackgroundColorChanged() { updateTheme(); }
        function onTextColorChanged() { updateTheme(); }
        function onHighlightColorChanged() { updateTheme(); }
        function onLinkColorChanged() { updateTheme(); }
    }

    Component.onCompleted: {
        updateTheme();
    }

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
            displayHint: Kirigami.DisplayHint.AlwaysHide
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
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: {
                if (thread) {
                    Raven.mailListModel.moveThreadToTrash(thread)
                    // Go back to the folder view after moving to trash
                    applicationWindow().pageStack.pop()
                }
            }
        }
    ]

    // Bridge for WebChannel communication
    ThreadViewBridge {
        id: threadBridge
        WebChannel.id: "threadBridge"
    }

    // WebChannel to expose bridge to JavaScript
    WebChannel {
        id: webChannel
        registeredObjects: [threadBridge]
    }

    // WebEngineView fills the page
    WebEngineView {
        id: webView
        anchors.fill: parent

        url: "qrc:///threadview/index.html"
        webChannel: webChannel

        settings.javascriptEnabled: true
        settings.localContentCanAccessFileUrls: true
        settings.localContentCanAccessRemoteUrls: false
        settings.autoLoadImages: true
        settings.pluginsEnabled: false
        settings.forceDarkMode: isDarkTheme()

        backgroundColor: Kirigami.Theme.backgroundColor

        onLoadingChanged: function(loadingInfo) {
            if (loadingInfo.status === WebEngineView.LoadSucceededStatus) {
                if (root.thread) {
                    threadBridge.loadThread(root.thread.id, root.thread.accountId, root.folderRole)
                }
            }
        }

        onNewWindowRequested: function(request) {
            // Open new window requests in external browser
            Qt.openUrlExternally(request.requestedUrl)
        }

        onNavigationRequested: function(request) {
            // Allow navigation within our app
            if (request.url.toString().startsWith("qrc:")) {
                request.action = WebEngineNavigationRequest.AcceptRequest
            } else if (request.url.toString().startsWith("file:")) {
                // Allow local file access (for inline images)
                request.action = WebEngineNavigationRequest.AcceptRequest
            } else {
                // Open external URLs in browser
                request.action = WebEngineNavigationRequest.IgnoreRequest
                Qt.openUrlExternally(request.url)
            }
        }
    }

    // Reload thread when thread property changes
    onThreadChanged: {
        if (thread && webView.loadProgress === 100) {
            threadBridge.loadThread(thread.id, thread.accountId, root.folderRole)
        }
    }
}
