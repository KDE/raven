import QtQuick 2.1
import org.kde.kirigami 2.4 as Kirigami
import QtQuick.Controls 2.0 as Controls
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.quickmail.private 1.0

Kirigami.ApplicationWindow {
    id: root

    title: i18n("KMailQuick")

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("KMailQuick")
        titleIcon: "applications-graphics"
        actions: [
            Kirigami.Action {
                text: i18n("View")
                iconName: "view-list-icons"
                Kirigami.Action {
                    text: i18n("View Action 1")
                    onTriggered: showPassiveNotification(i18n("View Action 1 clicked"))
                }
                Kirigami.Action {
                    text: i18n("View Action 2")
                    onTriggered: showPassiveNotification(i18n("View Action 2 clicked"))
                }
            },
            Kirigami.Action {
                text: i18n("Action 1")
                onTriggered: showPassiveNotification(i18n("Action 1 clicked"))
            },
            Kirigami.Action {
                text: i18n("Action 2")
                onTriggered: showPassiveNotification(i18n("Action 2 clicked"))
            }
        ]
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: QuickMail.loading ? loadingPage : mainPageComponent
    
    Component {
        id: loadingPage
        Kirigami.Page {
            Text { text: "loadin" }
        }
    }

    Component {
        id: mainPageComponent

        Kirigami.ScrollablePage {
            title: i18n("KMailQuick")

            ListView {
                id: folders
                model: QuickMail.descendantsProxyModel
                delegate: Kirigami.BasicListItem {
                    text: display
                    leftPadding: Kirigami.Units.gridUnit * model.kDescendantLevel
                    onClicked: {
                        QuickMail.loadMailCollection(model.index)
                        root.pageStack.push(folderPageComponent)
                    }
                }
            }
        }
    }
    Component {
        id: folderPageComponent

        Kirigami.ScrollablePage {
            title: "Folder"
            ListView {
                id: mails
                model: QuickMail.folderModel
                delegate: Kirigami.BasicListItem {
                    backgroundColor: "green"
                    text: sender+": "+model.title+"\n   "+plainText

                }
            }
        }
    }
}
