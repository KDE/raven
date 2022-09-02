// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls

import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.raven 1.0

import "mailboxselector"

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Raven")
    
    width: 1200
    height: 600

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.globalToolBar.canContainHandles: true
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton;
    
    pageStack.initialPage: FolderView {}

    // pop pages when not in use in mobile mode
    Connections {
        target: applicationWindow().pageStack
        function onCurrentIndexChanged() {
            // wait for animation to finish before popping pages
            popTimer.restart();
        }
    }
    
    Timer {
        id: popTimer
        interval: 300
        onTriggered: {
            if (!root.isWidescreen) {
                let currentIndex = applicationWindow().pageStack.currentIndex;
                while (applicationWindow().pageStack.depth > (currentIndex + 1) && currentIndex >= 0) {
                    applicationWindow().pageStack.pop();
                }
            }
        }
    }
    
    property bool isWidescreen: root.width > 500
    onIsWidescreenChanged: changeSidebar(isWidescreen);
    
    Kirigami.PagePool {
        id: pagePool
    }
    
    function getPage(name) {
        switch (name) {
            case "FolderView": return pagePool.loadPage("qrc:/FolderView.qml")
            case "MailBoxListPage": return pagePool.loadPage("qrc:/mailboxselector/MailBoxListPage.qml")
        }
    }
    
    Component.onCompleted: {
        // initial page and nav type
        changeSidebar(isWidescreen);
        
        if (isWidescreen) {
            root.pageStack.push(getPage("FolderView"));
        }
    }
    
    // switch between page and sidebar
    function changeSidebar(toWidescreen) {
        if (toWidescreen) {
            // unload first page (mailboxes page)
            let array = [];
            while (root.pageStack.depth > 0) {
                array.push(root.pageStack.pop());
            }
            for (let i = array.length - 2; i >= 0; i--) {
                root.pageStack.push(array[i]);
            }
            
            // load sidebar
            sidebarLoader.active = true;
            root.globalDrawer = sidebarLoader.item;
            
            // restore mail list if not there
            if (root.pageStack.depth === 0) {
                root.pageStack.push(getPage("FolderView"));
            }
        } else {
            // unload sidebar
            sidebarLoader.active = false;
            root.globalDrawer = null;
            
            // load mailboxes page as first page, and then restore all pages
            let array = [];
            while (root.pageStack.depth > 0) {
                array.push(root.pageStack.pop());
            }
            root.pageStack.push(getPage("MailBoxListPage"));
            for (let i = array.length - 1; i >= 0; i--) {
                root.pageStack.push(array[i]);
            }
        }
    }
    
    Loader {
        id: sidebarLoader
        source: "qrc:/mailboxselector/MailBoxListSidebar.qml"
        active: false
    }
}
