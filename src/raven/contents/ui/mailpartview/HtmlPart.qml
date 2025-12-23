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

    property string searchString
    property bool autoLoadImages: true

    //We have to give it a minimum size so the html content starts to expand
    property int minimumSize: 10
    implicitHeight: minimumSize

    onSearchStringChanged: {
        htmlView.findText(searchString)
    }
    onContentChanged: {
        htmlView.loadHtml(content, "file:///");
    }

    Flickable {
        id: flickable
        boundsBehavior: Flickable.StopAtBounds
        anchors.fill: parent

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
                root.implicitWidth = Math.max(contentsSize.width, flickable.minimumSize)

                if (loadingInfo.status == WebEngineView.LoadSucceededStatus) {
                    runJavaScript("[document.body.scrollHeight, document.body.scrollWidth, document.documentElement.scrollHeight]", function(result) {
                        let height = Math.min(Math.max(result[0], result[2]), 4000);
                        let width = Math.min(Math.max(result[1], flickable.width), 2000);
                        root.implicitHeight = height;
                        root.implicitWidth = width;
                        // htmlView.height = height;
                        // htmlView.width = width;
                    });
                }
            }
            onLinkHovered: {
                console.debug("Link hovered ", hoveredUrl)
            }
            onNavigationRequested: {
                console.debug("Nav request ", request.navigationType, request.url)
                if (request.navigationType == WebEngineNavigationRequest.LinkClickedNavigation) {
                    Qt.openUrlExternally(request.url)
                    request.action = WebEngineNavigationRequest.IgnoreRequest
                }
            }
            onNewWindowRequested: (request) => {
                console.debug("New view request ", request, request.requestedUrl)
                //We ignore requests for new views and open a browser instead
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
                localContentCanAccessFileUrls: false
                linksIncludedInFocusChain: false
                javascriptEnabled: true
                javascriptCanOpenWindows: false
                javascriptCanAccessClipboard: false
                hyperlinkAuditingEnabled: false
                fullScreenSupportEnabled: false
                errorPageEnabled: false
                //defaultTextEncoding: ???
                autoLoadImages: root.autoLoadImages
                autoLoadIconsForPage: false
                accelerated2dCanvasEnabled: false
                //The webview should not steal focus
                focusOnNavigationEnabled: false
            }
            profile {
                offTheRecord: true
                httpCacheType: WebEngineProfile.NoCache
                persistentCookiesPolicy: WebEngineProfile.NoPersistentCookies
            }
            onContextMenuRequested: function(request) {
                request.accepted = true
            }
        }
    }
}
