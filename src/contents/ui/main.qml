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

    pageStack.initialPage: QuickMail.loading ? loadingPage : mainPageComponent

    Component {
        id: loadingPage
        Kirigami.Page {
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                text: i18n("Loading, please wait...")
            }
        }
    }

    Component {
        id: mainPageComponent

        Kirigami.ScrollablePage {
            id: folderListView
            title: i18n("KMailQuick")
            property var mailListPage: null

            ListView {
                model: QuickMail.descendantsProxyModel
                delegate: Kirigami.BasicListItem {
                    text: model.display
                    leftPadding: Kirigami.Units.gridUnit * model.kDescendantLevel
                    onClicked: {
                        QuickMail.loadMailCollection(model.index);
                        if (folderListView.mailListPage) {
                            folderListView.mailListPage.title = model.display
                            folderListView.mailListPage.forceActiveFocus();
                            //applicationWindow().pageStack.currentIndex = folderListView.mailListPage;
                        } else {
                            folderListView.mailListPage = root.pageStack.push(folderPageComponent, {
                                title: model.display
                            });
                        }
                    }
                }
            }
        }
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
                                'mail': model.mail
                            });
                        } else {
                            folderView.mailViewer.mail = model.mail;
                            //applicationWindow().pageStack.currentItem = folderView.mailViewer;
                        }
                    }
                }
            }
        }
    }

    Component {
        id: mailComponent

        Kirigami.ScrollablePage {
            required property var mail
            title: mail.subject

            Kirigami.OverlaySheet {
                id: linkOpenDialog
                property string link
                header: Kirigami.Heading {
                    text: i18n("Open Link")
                }
                contentItem: Controls.Label {
                    text: i18n("Do you really want to open '%1'?", linkOpenDialog.link)
                    wrapMode: Text.Wrap
                }
                footer: Controls.DialogButtonBox {
                    standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
                    onAccepted: {
                        console.log("rejo");
                    }
                }
            }

            ColumnLayout {
                Kirigami.FormLayout {
                    Layout.fillWidth: true
                    Controls.Label {
                        Kirigami.FormData.label: i18n("From:")
                        text: mail.from
                    }
                    Controls.Label {
                        Kirigami.FormData.label: i18n("To:")
                        text: mail.to.join(', ')
                    }
                    Controls.Label {
                        visible: mail.sender !== mail.from && mail.sender.length > 0
                        Kirigami.FormData.label: i18n("Sender:")
                        text: mail.sender
                    }
                    Controls.Label {
                        Kirigami.FormData.label: i18n("Date:")
                        text: mail.date.toLocaleDateString()
                    }
                }
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                Controls.TextArea {
                    background: Item {}
                    textFormat: TextEdit.RichText
                    Layout.fillWidth: true
                    readOnly: true
                    selectByMouse: true
                    text: mail.plainContent
                    wrapMode: Text.Wrap
                    Controls.ToolTip.text: hoveredLink.length
                    Controls.ToolTip.visible: hoveredLink.trim().length > 0
                    Controls.ToolTip.delay: Kirigami.Units.shortDuration
                    onLinkActivated: {
                        linkOpenDialog.link = link
                        linkOpenDialog.open();
                    }
                }
            }
        }
    }
}
