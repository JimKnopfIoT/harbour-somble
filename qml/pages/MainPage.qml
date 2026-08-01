import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    // The chart hangs off this page as an attached page, so a left swipe opens
    // it. It only makes sense once there is something to plot, hence the
    // re-check whenever the archive changes rather than a one-shot attach.
    function updateAttachedChart() {
        if (status !== PageStatus.Active)
            return
        if (omron.count > 0 && !pageStack.nextPage(page))
            pageStack.pushAttached(Qt.resolvedUrl("ChartPage.qml"))
    }

    onStatusChanged: updateAttachedChart()
    Component.onCompleted: updateAttachedChart()

    Connections {
        target: omron
        onMeasurementsChanged: page.updateAttachedChart()
    }

    // Deleting everything is permanent — the readings are also remembered as
    // deleted, so a later download will not restore them. Worth a question
    // rather than a countdown the user might miss.
    Component {
        id: deleteAllDialog
        Dialog {
            allowedOrientations: Orientation.All
            onAccepted: omron.deleteAll()
            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                DialogHeader {
                    acceptText: qsTr("Delete all")
                    cancelText: qsTr("Keep")
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    text: qsTr("Really delete all readings?")
                    font.pixelSize: Theme.fontSizeLarge
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: qsTr("All %1 reading(s) are removed from the archive and "
                             + "will not reappear on the next download. The monitor's "
                             + "own memory is not touched. Export a CSV first if you "
                             + "want to keep them.").arg(omron.count)
                }
            }
        }
    }

    Component {
        id: filePicker
        FilePickerPage {
            title: qsTr("Select a saved archive")
            nameFilters: [ "*.json" ]
            onSelectedContentPropertiesChanged:
                omron.loadFromFile(selectedContentProperties.filePath, true)
        }
    }

    // Colour code by hypertension grade (systolic).
    function sysColor(sys) {
        if (sys >= 160) return "#E53935"       // grade 2+
        if (sys >= 140) return "#FB8C00"       // grade 1
        if (sys >= 130) return "#FDD835"       // high-normal
        return "#43A047"                        // normal
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: omron.measurements

        header: Column {
            width: list.width
            PageHeader { title: qsTr("Omron") }

            Item {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: statusRow.height
                Row {
                    id: statusRow
                    spacing: Theme.paddingMedium
                    BusyIndicator { running: omron.busy; size: BusyIndicatorSize.Small
                        anchors.verticalCenter: parent.verticalCenter }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: omron.statusText
                        color: Theme.secondaryHighlightColor
                    }
                }
            }
            Item { width: 1; height: Theme.paddingMedium }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Device")
                onClicked: pageStack.push(Qt.resolvedUrl("DevicePage.qml"))
            }
            MenuItem {
                text: qsTr("Raw data / pairing key")
                onClicked: pageStack.push(Qt.resolvedUrl("RawPage.qml"))
            }
            MenuItem {
                text: qsTr("Export CSV")
                visible: omron.count > 0
                onClicked: omron.exportCsv()
            }
            MenuItem {
                text: qsTr("Load archive file…")
                onClicked: pageStack.push(filePicker)
            }
            MenuItem {
                text: qsTr("Save archive to file")
                visible: omron.count > 0
                onClicked: omron.saveToFile()
            }
            MenuItem {
                text: qsTr("Restore deleted (%1)").arg(omron.deletedCount)
                visible: omron.deletedCount > 0
                onClicked: omron.forgetDeletions()
            }
            MenuItem {
                text: qsTr("Clear archive")
                visible: omron.count > 0
                onClicked: clearRemorse.execute(qsTr("Clearing the local archive"),
                                                function() { omron.clearArchive() })
            }
            MenuItem {
                text: qsTr("Pair with monitor")
                enabled: !omron.busy
                onClicked: pairDialog.open()
            }
            MenuItem {
                text: omron.busy ? qsTr("Cancel") : qsTr("Download from device")
                onClicked: omron.busy ? omron.cancel() : omron.download()
            }
        }

        // Pairing reprograms the key stored in the monitor, which invalidates
        // any other app that was paired with it — worth one confirmation.
        RemorsePopup { id: pairRemorse }
        RemorsePopup { id: clearRemorse }
        QtObject {
            id: pairDialog
            function open() {
                pairRemorse.execute(qsTr("Pairing — put the monitor in pairing mode"),
                                    function() { omron.pair() })
            }
        }

        ViewPlaceholder {
            enabled: omron.count === 0 && !omron.busy
            text: qsTr("No measurements")
            hintText: qsTr("Pair the monitor once (pull down → Pair with monitor, "
                         + "with the monitor in pairing mode), then download.")
        }

        delegate: ListItem {
            id: item
            width: list.width
            contentHeight: Theme.itemSizeMedium
            property var m: modelData

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Delete this reading")
                    onClicked: {
                        // Capture both now: deleting rebuilds the list, which
                        // tears down this delegate's context before the remorse
                        // timer fires — and then `omron` resolves to undefined.
                        var api = omron
                        var idx = index
                        item.remorseAction(qsTr("Deleting"),
                                           function() { api.deleteRecord(idx) })
                    }
                }
                MenuItem {
                    text: qsTr("Delete all readings")
                    onClicked: pageStack.push(deleteAllDialog)
                }
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingMedium

                Rectangle {
                    width: Theme.paddingSmall
                    height: Theme.itemSizeSmall
                    radius: 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: page.sysColor(item.m.systolic)
                }

                Column {
                    width: parent.width * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: item.m.systolic + "/" + item.m.diastolic + " mmHg"
                        font.pixelSize: Theme.fontSizeMedium
                    }
                    Label {
                        // Readings taken while the monitor's clock was unset
                        // carry a factory timestamp; showing it would be a lie.
                        property bool dated: item.m.timeValid === undefined
                                             || item.m.timeValid
                        text: dated ? item.m.timestamp : qsTr("no date — clock was unset")
                        font.pixelSize: Theme.fontSizeExtraSmall
                        font.italic: !dated
                        color: Theme.secondaryColor
                    }
                }

                Column {
                    width: parent.width * 0.22
                    anchors.verticalCenter: parent.verticalCenter
                    Label {
                        text: "♥ " + item.m.pulse
                        font.pixelSize: Theme.fontSizeMedium
                        color: Theme.highlightColor
                    }
                    Label {
                        visible: item.m.arrhythmia
                        text: qsTr("arrhythmia")
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: "#E53935"
                    }
                }

                // P1/P2 toggle. The monitor keeps a single memory and records
                // no user, so who a reading belongs to is the owner's own note;
                // it lives in the archive and survives further downloads.
                Rectangle {
                    id: personToggle
                    anchors.verticalCenter: parent.verticalCenter
                    property int person: item.m.person === 2 ? 2 : 1
                    width: Theme.itemSizeExtraSmall * 1.15
                    height: Theme.itemSizeExtraSmall * 0.66
                    radius: height / 2
                    color: person === 2 ? Theme.rgba(Theme.highlightColor, 0.35)
                                        : Theme.rgba(Theme.secondaryColor, 0.18)
                    border.color: person === 2 ? Theme.highlightColor
                                               : Theme.rgba(Theme.secondaryColor, 0.5)
                    border.width: 1

                    Rectangle {
                        width: parent.height - 6
                        height: width
                        radius: width / 2
                        y: 3
                        x: personToggle.person === 2 ? parent.width - width - 3 : 3
                        color: personToggle.person === 2 ? Theme.highlightColor
                                                         : Theme.secondaryColor
                        Behavior on x { NumberAnimation { duration: 120 } }
                    }
                    Label {
                        // Sits on whichever side the knob is not, so the label
                        // stays readable instead of being covered by it.
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: personToggle.person === 2 ? parent.left : undefined
                        anchors.right: personToggle.person === 2 ? undefined : parent.right
                        anchors.leftMargin: Theme.paddingSmall
                        anchors.rightMargin: Theme.paddingSmall
                        text: personToggle.person === 2 ? "P2" : "P1"
                        font.pixelSize: Theme.fontSizeTiny
                        color: Theme.primaryColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -Theme.paddingMedium   // easier to hit
                        onClicked: omron.assignPerson(index, personToggle.person === 2 ? 1 : 2)
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
