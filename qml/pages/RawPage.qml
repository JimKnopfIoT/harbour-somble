import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Re-parse capture")
                enabled: omron.rawHex.length > 0
                onClicked: omron.reparse()
            }
            MenuItem {
                text: omron.busy ? qsTr("Cancel") : qsTr("Download from device")
                onClicked: omron.busy ? omron.cancel() : omron.download()
            }
        }

        Column {
            id: col
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Raw data / pairing key") }

            SectionHeader { text: qsTr("Pairing key") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The monitor stores a 16-byte key and only talks to whoever "
                         + "presents it. Pairing writes this key into the monitor. Change "
                         + "it only to re-use a key another tool already programmed.")
            }

            TextField {
                id: keyField
                width: parent.width
                label: qsTr("16 bytes, 32 hex characters")
                text: omron.pairingKey
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: omron.pairingKey = text
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Apply key")
                onClicked: omron.pairingKey = keyField.text
            }

            SectionHeader { text: qsTr("Raw EEPROM capture") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                font.pixelSize: Theme.fontSizeExtraSmall
                font.family: "monospace"
                color: Theme.primaryColor
                wrapMode: Text.WrapAnywhere
                text: omron.rawHex.length ? omron.rawHex : qsTr("(nothing captured yet)")
            }
        }
        VerticalScrollDecorator {}
    }
}
