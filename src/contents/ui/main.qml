// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Controls 2.15 as Controls
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.quickmail.private 1.0

Kirigami.ApplicationWindow {
    id: root

    title: i18n("KMailQuick")

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: QuickMail.loading ? loadingPage : folderPageComponent

    Component {
        id: loadingPage
        Kirigami.Page {
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                text: i18n("Loading, please wait...")
            }
        }
    }

    /*menuBar: Loader {
        id: menuLoader
        active: Kirigami.Settings.hasPlatformMenuBar != undefined ?
                !Kirigami.Settings.hasPlatformMenuBar && !Kirigami.Settings.isMobile :
                !Kirigami.Settings.isMobile

        sourceComponent: WindowMenu {
            parentWindow: root
            todoMode: pageStack.currentItem.objectName == "todoView"
            Kirigami.Theme.colorSet: Kirigami.Theme.Header
        }
    }*/


    globalDrawer: Sidebar {
        //bottomPadding: menuLoader.active ? menuLoader.height : 0
        mailListPage: QuickMail.loading ? null : pageStack.get(0)
    }

    Component {
        id: folderPageComponent

        Kirigami.ScrollablePage {
            id: folderView
            property var mailViewer: null;
            ListView {
                id: mails
                model: QuickMail.folderModel
                delegate: Kirigami.BasicListItem {
                    label: model.title
                    subtitle: sender
                    onClicked: {
                        if (!folderView.mailViewer) {
                            folderView.mailViewer = root.pageStack.push(mailComponent, {
                                viewerHelper: QuickMail.folderModel.viewerHelper
                            });
                        } else {
                            applicationWindow().pageStack.currentIndex = applicationWindow().pageStack.depth - 1;
                        }

                        QuickMail.folderModel.loadItem(index);
                    }
                }
            }
        }
    }

    Component {
        id: mailComponent
        MessageViewer {}
    }
}
