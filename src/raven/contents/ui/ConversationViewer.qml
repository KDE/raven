// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

import org.kde.raven 1.0
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels
import org.kde.raven 1.0 as Raven

import './mailpartview'

Kirigami.ScrollablePage {
    id: root

    property string subject
    
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    contextualActions: [
        Kirigami.Action {
            text: i18n("Move to trash")
            iconName: "albumfolder-user-trash"
            // TODO implement move to trash
        }
    ]
    
    ColumnLayout {
        spacing: 0
        
        QQC2.Label {
            Layout.leftMargin: Kirigami.Units.largeSpacing * 2
            Layout.rightMargin: Kirigami.Units.largeSpacing * 2
            Layout.topMargin: Kirigami.Units.gridUnit 
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.fillWidth: true
            
            text: root.subject
            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
        }
        
        Repeater {
            model: Raven.ThreadViewModel
            
            delegate: MailViewer {
                Layout.bottomMargin: Kirigami.Units.gridUnit * 2
                Layout.fillWidth: true

                item: root.item
                subject: model.subject
                from: model.from
                to: model.to
                sender: "" // TODO props.sender
                dateTime: model.date
                content: model.content
            }
        }
    }
}
