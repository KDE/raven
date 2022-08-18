// SPDX-FileCopyrightText: 2021 Felipe Kinoshita <kinofhek@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import Qt.labs.qmlmodels 1.0

import org.kde.kirigami 2.15 as Kirigami
import org.kde.raven.private 1.0
import org.kde.kitemmodels 1.0

Kirigami.OverlayDrawer {
    id: sidebar

    edge: Qt.application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge
    modal: !wideScreen
    onModalChanged: drawerOpen = !modal
    handleVisible: !wideScreen
    handleClosedIcon.source: null
    handleOpenIcon.source: null
    drawerOpen: !Kirigami.Settings.isMobile
    width: Kirigami.Units.gridUnit * 16

    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    property var mailListPage: null

    contentItem: ColumnLayout {
        id: container
        spacing: 0

        QQC2.ToolBar {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: pageStack.globalToolBar.preferredHeight

            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: Kirigami.Units.smallSpacing
            topPadding: 0
            bottomPadding: 0

            RowLayout {
                id: searchContainer
                anchors.fill: parent

                /*Kirigami.SearchField { // TODO: Make this open a new search results page
                 *    id: searchItem
                 *    Layout.fillWidth: true
                }*/

                Kirigami.Heading {
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                    text: i18n("Raven")
                }

                Kirigami.ActionToolBar {
                    id: menu
                    visible: false // only make it visible when menubar is hidden, progress in another MR: https://invent.kde.org/pim/kalendar/-/merge_requests/41
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: parent.height
                    overflowIconName: "application-menu"

                    Component.onCompleted: {
                        for (let i in actions) {
                            let action = actions[i]
                            action.displayHint = Kirigami.DisplayHint.AlwaysHide
                        }
                    }
                }
            }
        }

        QQC2.ScrollView {
            id: folderListView
            implicitWidth: Kirigami.Units.gridUnit * 16
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Kirigami.Units.largeSpacing * 2
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            contentWidth: availableWidth
            clip: true

            ListView {
                id: calendarList

                model: KDescendantsProxyModel {
                    id: foldersModel
                    model: Raven.foldersModel
                }
                onModelChanged: currentIndex = -1

                delegate: DelegateChooser {
                    role: 'kDescendantExpandable'
                    DelegateChoice {
                        roleValue: true

                        Kirigami.BasicListItem {
                            label: display
                            labelItem.color: Kirigami.Theme.disabledTextColor
                            labelItem.font.weight: Font.DemiBold
                            topPadding: 2 * Kirigami.Units.largeSpacing
                            leftPadding: (Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing * 2 : Kirigami.Units.largeSpacing) + Kirigami.Units.gridUnit * (model.kDescendantLevel - 1)
                            hoverEnabled: false
                            background: Item {}

                            separatorVisible: false

                            leading: Kirigami.Icon {
                                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                                implicitHeight: Kirigami.Units.iconSizes.smallMedium
                                color: Kirigami.Theme.disabledTextColor
                                isMask: true
                                source: model.decoration
                            }
                            leadingPadding: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing * 2 : Kirigami.Units.largeSpacing

                            trailing: Kirigami.Icon {
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                source: model.kDescendantExpanded ? 'arrow-up' : 'arrow-down'
                            }

                            onClicked: calendarList.model.toggleChildren(index)
                        }
                    }

                    DelegateChoice {
                        roleValue: false
                        Kirigami.BasicListItem {
                            label: display
                            labelItem.color: Kirigami.Theme.textColor
                            leftPadding: Kirigami.Units.largeSpacing * 2 * (model.kDescendantLevel - 2)
                            hoverEnabled: sidebar.todoMode
                            separatorVisible: false
                            reserveSpaceForIcon: true

                            onClicked: {
                                model.checkState = model.checkState === 0 ? 2 : 0
                                Raven.loadMailCollection(foldersModel.mapToSource(foldersModel.index(model.index, 0)));
                                if (sidebar.mailListPage) {
                                    sidebar.mailListPage.title = model.display
                                    sidebar.mailListPage.forceActiveFocus();
                                    applicationWindow().pageStack.currentIndex = 1;
                                } else {
                                    sidebar.mailListPage = root.pageStack.push(folderPageComponent, {
                                        title: model.display
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
