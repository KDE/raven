// SPDX-FileCopyrightText: 2022-2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormButtonDelegate {
    id: root

    property string textValue

    signal textSaved(string savedText)

    description: textValue === "" ? i18n("Enter a value…") : "●●●●●"

    onClicked: {
        textField.text = root.textValue;
        dialog.open();
        textField.forceActiveFocus();
    }

    Kirigami.Dialog {
        id: dialog
        standardButtons: Kirigami.Dialog.Cancel | Kirigami.Dialog.Apply
        title: root.text

        padding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        preferredWidth: Kirigami.Units.gridUnit * 20

        onApplied: {
            root.textSaved(textField.text);
            dialog.close();
        }

        Kirigami.PasswordField {
            id: textField
            text: root.textValue
            onAccepted: dialog.applied()
        }
    }
}

