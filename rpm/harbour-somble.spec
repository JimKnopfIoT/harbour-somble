# Neutral packaging metadata — no personal identifiers.
%define _buildhost reproducible-builder

Name:       harbour-somble
Summary:    Read & chart Omron blood-pressure measurements over Bluetooth LE
Version:    0.3.0
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
* Sun Aug 23 2026 harbour-somble contributors 0.3.0-1
- Per-model EEPROM profiles: EVOLV/HEM-7600T plus M700, RS7 Intelli IT,
  Complete, M500/M7 Intelli IT, BP7450, M400/X4 smart and BP7250. The model is
  identified from what the monitor reports, or chosen by hand on the Device
  page. Fixes downloads returning nothing on anything but an EVOLV.
- Reads both user memories on models that have two, and seeds P1/P2 from them.
- The clock is only written on a recognised model, at an address a community
  driver writes too, and only after the block read back from it verifies.
  Monitors whose clock location is unconfirmed are never written to.
- A clock-only session no longer discards the last raw capture.

* Mon Aug 03 2026 harbour-somble contributors 0.2.0-1
- Assigning a reading to person 1 or 2 no longer scrolls the list back to the top.
- Chart: pinch to zoom the time window, drag to move it, tap to inspect a reading.
- Chart: hypertension grades marked at 140/160/180; scale follows readings beyond 40-200.
- Chart: page header and range bar dropped, so the plot gets the height.

* Sat Aug 01 2026 harbour-somble contributors 0.1.0-1
- Initial: BlueZ GATT transport, Omron record download + parse, CSV export.
