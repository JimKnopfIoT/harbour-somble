import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height
        VerticalScrollDecorator {}

        Column {
            id: col
            width: parent.width
            spacing: Theme.paddingMedium
            bottomPadding: Theme.paddingLarge

            PageHeader {
                title: qsTr("About somble")
                description: qsTr("Version %1").arg(omron.version)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                onLinkActivated: Qt.openUrlExternally(link)
                text: qsTr(
                    "<p>Reads the measurements stored in an <b>Omron</b> blood-pressure " +
                    "monitor over Bluetooth LE, charts them and exports them. Developed " +
                    "against an <b>EVOLV (HEM-7600T)</b>.</p>" +

                    "<p>It speaks to <tt>BlueZ</tt> directly over D-Bus rather than through " +
                    "QtBluetooth, whose development headers the Sailfish target does not " +
                    "ship.</p>" +

                    "<p><b>Getting started.</b> The monitor has to be bonded once in " +
                    "Settings → Bluetooth — Sailfish does not let an ordinary app drive " +
                    "the pairing itself. Then <i>Pair with monitor</i> here to store this " +
                    "app's key in it, and <i>Device → Set the monitor's clock</i>.</p>" +

                    "<p><b>Set the clock first.</b> The monitor is delivered with its clock " +
                    "unset and reports 2015-01-01 00:00 until it is set — and it stamps " +
                    "every reading from that clock. The timestamp lives inside the record " +
                    "and cannot be corrected afterwards, so readings taken beforehand keep " +
                    "no usable date. They are shown as <i>no date</i> and left out of the " +
                    "chart rather than plotted on a made-up one.</p>" +

                    "<p><b>One memory, two people.</b> The monitor stores no user " +
                    "information and combines everyone's readings, so the P1/P2 switch on " +
                    "each reading is your own note. It is kept in the archive and survives " +
                    "further downloads.</p>")
            }

            SectionHeader { text: qsTr("Status & responsible use") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                text: qsTr(
                    "<p><b>Proof of concept / work in progress.</b> A hobby project, " +
                    "shared <b>as is</b> with <b>no warranty</b> of any kind (see the " +
                    "GPLv3). It may be incomplete, rough around the edges, or change " +
                    "without notice — use it at your own risk.</p>" +

                    "<p><b>This is not a medical device and displays no medically " +
                    "validated data.</b> It only reads back what the monitor already " +
                    "stored. Never use it to diagnose, to judge a course of treatment, or " +
                    "to adjust medication — only a doctor is qualified to do that.</p>" +

                    "<p>The protocol was reconstructed from community reverse-engineering, " +
                    "not from vendor documentation, so decoding errors are possible. The " +
                    "only write this app performs — setting the clock — touches the same " +
                    "memory region believed to hold the pressure-sensor calibration data. " +
                    "It is optional and confirmed, but it is your monitor and your risk.</p>")
            }

            SectionHeader { text: qsTr("Attribution") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                onLinkActivated: Qt.openUrlExternally(link)
                text: qsTr(
                    "<p>The wire protocol follows the community reverse-engineering in " +
                    "<a href='https://github.com/userx14/omblepy'>omblepy</a>, " +
                    "cross-checked against " +
                    "<a href='https://codeberg.org/LazyT/ubpm'>UBPM</a>. No code was " +
                    "copied from either; both are independent implementations of the same " +
                    "observed protocol.</p>" +

                    "<p>Licensed GPL-3.0-or-later. Source: " +
                    "<a href='https://github.com/JimKnopfIoT/harbour-somble'>" +
                    "github.com/JimKnopfIoT/harbour-somble</a></p>" +

                    "<p>“Omron” and “EVOLV” are trademarks of their respective owner. " +
                    "This project is not affiliated with, endorsed by, or supported by " +
                    "Omron.</p>")
            }
        }
    }
}
