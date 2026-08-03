# Neutral packaging metadata — no personal identifiers.
%define _buildhost reproducible-builder

Name:       harbour-somble
Summary:    Read & chart Omron blood-pressure measurements over Bluetooth LE
Version:    0.2.0
Release:    1
License:    GPL-3.0-or-later
URL:        https://github.com/JimKnopfIoT/harbour-somble
Source0:    %{name}-%{version}.tar.bz2
Vendor:     harbour-somble contributors
Packager:   harbour-somble contributors

Requires:   sailfishsilica-qt5
Requires:   bluez5
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: desktop-file-utils

%description
Native SailfishOS app that downloads the stored measurements from an Omron
blood-pressure monitor over Bluetooth LE (spoken directly to BlueZ over D-Bus),
charts systolic/diastolic/pulse over time, and exports CSV and an image.

%prep
%setup -q

%build
%qmake5
%make_build

%install
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog
* Mon Aug 03 2026 harbour-somble contributors 0.2.0-1
- Assigning a reading to person 1 or 2 no longer scrolls the list back to the top.
- Chart: pinch to zoom the time window, drag to move it, tap to inspect a reading.
- Chart: hypertension grades marked at 140/160/180; scale follows readings beyond 40-200.
- Chart: page header and range bar dropped, so the plot gets the height.

* Sat Aug 01 2026 harbour-somble contributors 0.1.0-1
- Initial: BlueZ GATT transport, Omron record download + parse, CSV export.
