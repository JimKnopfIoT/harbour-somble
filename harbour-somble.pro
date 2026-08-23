# harbour-somble — native SailfishOS app to read Omron blood-pressure
# measurements over Bluetooth LE (BlueZ GATT via D-Bus — no QtBluetooth, which
# ships no development headers in the Sailfish target), chart them and export
# CSV / image.

TARGET = harbour-somble

CONFIG += sailfishapp sailfishapp_i18n c++17
QT += dbus quick gui

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Keep the About page's version tied to the packaged one.
VERSION = 0.3.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += src

HEADERS += \
    src/bletransport.h \
    src/omronprotocol.h \
    src/omron.h

SOURCES += \
    src/harbour-somble.cpp \
    src/bletransport.cpp \
    src/omronprotocol.cpp \
    src/omron.cpp

# Runs unsandboxed via [X-Sailjail] Sandboxing=Disabled in the .desktop — see
# the comment there for why the stock Bluetooth permission does not suffice.

TRANSLATIONS += translations/harbour-somble-de.ts
lupdate_only {
    SOURCES += qml/*.qml qml/pages/*.qml qml/cover/*.qml
}

DISTFILES += \
    qml/harbour-somble.qml \
    qml/pages/MainPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/ChartPage.qml \
    qml/pages/DevicePage.qml \
    qml/pages/RawPage.qml \
    qml/cover/CoverPage.qml \
    harbour-somble.desktop \
    rpm/harbour-somble.spec
