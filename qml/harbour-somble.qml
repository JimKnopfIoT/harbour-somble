import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import "pages"

ApplicationWindow {
    initialPage: Component { MainPage {} }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.All

    Notification {
        id: banner
        isTransient: true
        appName: "somble"
    }
    function notify(msg) {
        banner.close()
        banner.previewSummary = "somble"
        banner.previewBody = msg
        banner.publish()
    }

    Connections {
        target: omron
        onActionError: notify(message)
        onActionInfo: notify(message)
    }
}
