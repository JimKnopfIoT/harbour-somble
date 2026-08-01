import QtQuick 2.6
import Sailfish.Silica 1.0

// Everything the monitor tells us about itself: the standard Device
// Information / Battery characteristics, plus its own clock, which is what
// stamps every stored reading.
Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Set the monitor's clock")
                enabled: !omron.busy
                onClicked: clockRemorse.execute(
                               qsTr("Writing the phone's time to the monitor"),
                               function() { omron.setDeviceClock() })
            }
            MenuItem {
                text: omron.busy ? qsTr("Cancel") : qsTr("Read from device")
                onClicked: omron.busy ? omron.cancel() : omron.download()
            }
        }

        RemorsePopup { id: clockRemorse }

        Column {
            id: col
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Device") }

            SectionHeader { text: qsTr("Clock") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: omron.deviceClock.length
                      ? omron.deviceClock
                      : qsTr("not set — every reading is stored with the same "
                           + "meaningless timestamp until the clock is set")
                color: omron.deviceClock.length ? Theme.primaryColor : "#FB8C00"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Setting the clock is the only thing this app ever writes "
                         + "to the monitor. It only affects readings taken afterwards — "
                         + "already-stored ones keep their timestamp.")
            }

            SectionHeader {
                text: qsTr("Identification")
                visible: infoRepeater.count > 0
            }

            Column {
                width: parent.width
                Repeater {
                    id: infoRepeater
                    model: omron.deviceInfo
                    delegate: Item {
                        width: col.width
                        height: Theme.itemSizeExtraSmall
                        Label {
                            x: Theme.horizontalPageMargin
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.label
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                        }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: Theme.horizontalPageMargin
                            text: modelData.value
                            font.pixelSize: Theme.fontSizeSmall
                            truncationMode: TruncationMode.Fade
                            width: parent.width * 0.5
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            ViewPlaceholder {
                enabled: infoRepeater.count === 0
                text: qsTr("Nothing read yet")
                hintText: qsTr("Pull down and read from the device.")
            }
        }
        VerticalScrollDecorator {}
    }
}
