import QtQuick 2.6
import Sailfish.Silica 1.0

// Everything the monitor tells us about itself: the standard Device
// Information / Battery characteristics, plus its own clock, which is what
// stamps every stored reading.
Page {
    id: page
    allowedOrientations: Orientation.All

    // The picker lists "identify it" first, then the known models in the order
    // the C++ side returns them, so index 0 is the automatic entry.
    function modelIndexOf(id) {
        if (!id)
            return 0
        var list = omron.knownModels
        for (var i = 0; i < list.length; ++i)
            if (list[i].id === id)
                return i + 1
        return 0
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height

        PullDownMenu {
            MenuItem {
                // Hidden rather than greyed out where it cannot work: on a
                // monitor whose clock location is unconfirmed there is nothing
                // to enable, and the explanation is right below in the page.
                visible: omron.clockWritable
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

            SectionHeader { text: qsTr("Model") }

            ComboBox {
                id: modelCombo
                width: parent.width
                label: qsTr("Monitor model")
                description: qsTr("Where the readings and the clock sit in the "
                                + "monitor's memory differs per model. Leave this "
                                + "automatic unless the monitor is not recognised.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Identify automatically") }
                    Repeater {
                        model: omron.knownModels
                        MenuItem { text: modelData.label }
                    }
                }
                Component.onCompleted: currentIndex = page.modelIndexOf(omron.modelId)
                onCurrentIndexChanged: {
                    var id = currentIndex <= 0 ? "" : omron.knownModels[currentIndex - 1].id
                    if (id !== omron.modelId)
                        omron.modelId = id
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Decoding as: %1").arg(omron.modelLabel)
            }

            SectionHeader { text: qsTr("Clock") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                // Four cases, and they mean different things: a clock this app
                // read; nothing read yet, so nothing is known either way; a
                // clock it could read and found unset; and a model whose clock
                // it cannot find at all. Only the unset one is a warning.
                text: omron.deviceClock.length
                      ? omron.deviceClock
                      : !omron.modelIdentified
                        ? qsTr("not read yet")
                        : omron.clockWritable
                          ? qsTr("not set — every reading is stored with the same "
                               + "meaningless timestamp until the clock is set")
                          : qsTr("cannot be read on this model")
                color: omron.deviceClock.length ? Theme.primaryColor
                     : (omron.modelIdentified && omron.clockWritable) ? "#FB8C00"
                     : Theme.secondaryColor
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: !omron.modelIdentified
                      ? qsTr("Whether the clock can be set depends on the model, and the "
                           + "monitor has not been read yet. Read from the device once "
                           + "and this will say.")
                      : omron.clockWritable
                        ? qsTr("Setting the clock is the only thing this app ever writes "
                             + "to the monitor. It only affects readings taken afterwards — "
                             + "already-stored ones keep their timestamp.")
                        : qsTr("Setting the clock is the only thing this app would ever "
                             + "write to the monitor, and where the clock is stored is not "
                             + "confirmed for this model — a write to the wrong address "
                             + "could damage it. So somble does not write at all here: "
                             + "please set the date and time on the monitor itself, "
                             + "before taking readings.")
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

            // Not a ViewPlaceholder: that one binds its own y to a third of
            // the screen height and so lands on top of a page that already has
            // content. Same two labels, but in the column flow, so the hint
            // follows the text above it instead of floating over it.
            Column {
                width: parent.width
                visible: infoRepeater.count === 0
                topPadding: Theme.paddingLarge

                InfoLabel {
                    text: qsTr("Nothing read yet")
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                    color: Theme.highlightColor
                    opacity: Theme.opacityLow
                    text: qsTr("Pull down and read from the device.")
                }
            }
        }
        VerticalScrollDecorator {}
    }
}
