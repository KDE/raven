// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-License-Identifier: GPL-2.0-or-later


import QtQuick
import QtQuick.Controls as QQC2
import QtWebEngine
import QtQuick.Window

import org.kde.raven
import org.kde.kirigami as Kirigami

Item {
    id: root
    objectName: "htmlPart"

    property string content

    // TODO: search text
    property string searchString

    property bool autoLoadImages: true

    // We have to give it a minimum size so the html content starts to expand
    property int minimumSize: 10
    implicitHeight: Math.max(minimumSize, htmlView.contentsSize.height)
    implicitWidth: Math.max(minimumSize, htmlView.contentsSize.width)

    onSearchStringChanged: {
        htmlView.findText(searchString)
    }
    onContentChanged: {
        htmlView.loadHtml(content, "file:///");
    }

    WebEngineView {
        id: htmlView
        objectName: "htmlView"
        anchors.fill: parent

        Component.onCompleted: loadHtml(content, "file:///")

        onLoadingChanged: (loadingInfo) => {
            if (loadingInfo.status == WebEngineView.LoadFailedStatus) {
                console.warn("Failed to load html content.")
                console.warn("Error is ", loadingInfo.errorString)
            }
        }
        onLinkHovered: {
            console.debug("Link hovered ", hoveredUrl)
        }
        onNavigationRequested: {
            if (request.navigationType == WebEngineNavigationRequest.LinkClickedNavigation) {
                console.debug("Nav request ", request.navigationType, request.url)
                Qt.openUrlExternally(request.url)
                request.action = WebEngineNavigationRequest.IgnoreRequest
            }
        }
        onNewWindowRequested: (request) => {
            console.debug("New view request ", request, request.requestedUrl)
            // We ignore requests for new views and open a browser instead
            Qt.openUrlExternally(request.requestedUrl)
        }
        settings {
            webGLEnabled: false
            touchIconsEnabled: false
            spatialNavigationEnabled: false
            screenCaptureEnabled: false
            pluginsEnabled: false
            localStorageEnabled: false
            localContentCanAccessRemoteUrls: false
            // Allow loading local file:// URLs for inline images (cid: replacements)
            localContentCanAccessFileUrls: true
            linksIncludedInFocusChain: false

            javascriptEnabled: false
            javascriptCanOpenWindows: false
            javascriptCanAccessClipboard: false

            hyperlinkAuditingEnabled: false
            fullScreenSupportEnabled: false
            errorPageEnabled: false
            autoLoadImages: root.autoLoadImages
            autoLoadIconsForPage: root.autoLoadImages
            accelerated2dCanvasEnabled: false
            focusOnNavigationEnabled: false
            touchEventsApiEnabled: false
            forceDarkMode: Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark
        }
        profile {
            offTheRecord: true
            httpCacheType: WebEngineProfile.NoCache
            persistentCookiesPolicy: WebEngineProfile.NoPersistentCookies
        }
        onContextMenuRequested: function(request) {
            request.accepted = true;
        }
    }
}
