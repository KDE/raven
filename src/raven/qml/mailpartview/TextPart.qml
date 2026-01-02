// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2023-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.raven
import org.kde.kirigami as Kirigami

Item {
    id: root

    property string content
    property alias textFormat: textEdit.textFormat

    implicitHeight: textEdit.height

    QQC2.TextArea {
        id: textEdit
        objectName: "textView"
        background: Item {}
        readOnly: true
        textFormat: TextEdit.PlainText
        padding: 0

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        text: content.substring(0, 100000).replace(/\u00A0/g, ' ') // TextEdit deals poorly with messages that are too large.
        onLinkActivated: Qt.openUrlExternally(link)

        wrapMode: TextEdit.WordWrap
    }
}
