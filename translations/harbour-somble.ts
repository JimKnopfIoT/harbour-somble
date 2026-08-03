<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en">
<context>
    <name>AboutPage</name>
    <message>
        <source>About somble</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Version %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;p&gt;Reads the measurements stored in an &lt;b&gt;Omron&lt;/b&gt; blood-pressure monitor over Bluetooth LE, charts them and exports them. Developed against an &lt;b&gt;EVOLV (HEM-7600T)&lt;/b&gt;.&lt;/p&gt;&lt;p&gt;It speaks to &lt;tt&gt;BlueZ&lt;/tt&gt; directly over D-Bus rather than through QtBluetooth, whose development headers the Sailfish target does not ship.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Getting started.&lt;/b&gt; The monitor has to be bonded once in Settings → Bluetooth — Sailfish does not let an ordinary app drive the pairing itself. Then &lt;i&gt;Pair with monitor&lt;/i&gt; here to store this app&apos;s key in it, and &lt;i&gt;Device → Set the monitor&apos;s clock&lt;/i&gt;.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Set the clock first.&lt;/b&gt; The monitor is delivered with its clock unset and reports 2015-01-01 00:00 until it is set — and it stamps every reading from that clock. The timestamp lives inside the record and cannot be corrected afterwards, so readings taken beforehand keep no usable date. They are shown as &lt;i&gt;no date&lt;/i&gt; and left out of the chart rather than plotted on a made-up one.&lt;/p&gt;&lt;p&gt;&lt;b&gt;One memory, two people.&lt;/b&gt; The monitor stores no user information and combines everyone&apos;s readings, so the P1/P2 switch on each reading is your own note. It is kept in the archive and survives further downloads.&lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Status &amp; responsible use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;p&gt;&lt;b&gt;Proof of concept / work in progress.&lt;/b&gt; A hobby project, shared &lt;b&gt;as is&lt;/b&gt; with &lt;b&gt;no warranty&lt;/b&gt; of any kind (see the GPLv3). It may be incomplete, rough around the edges, or change without notice — use it at your own risk.&lt;/p&gt;&lt;p&gt;&lt;b&gt;This is not a medical device and displays no medically validated data.&lt;/b&gt; It only reads back what the monitor already stored. Never use it to diagnose, to judge a course of treatment, or to adjust medication — only a doctor is qualified to do that.&lt;/p&gt;&lt;p&gt;The protocol was reconstructed from community reverse-engineering, not from vendor documentation, so decoding errors are possible. The only write this app performs — setting the clock — touches the same memory region believed to hold the pressure-sensor calibration data. It is optional and confirmed, but it is your monitor and your risk.&lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Attribution</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;p&gt;The wire protocol follows the community reverse-engineering in &lt;a href=&apos;https://github.com/userx14/omblepy&apos;&gt;omblepy&lt;/a&gt;, cross-checked against &lt;a href=&apos;https://codeberg.org/LazyT/ubpm&apos;&gt;UBPM&lt;/a&gt;. No code was copied from either; both are independent implementations of the same observed protocol.&lt;/p&gt;&lt;p&gt;Licensed GPL-3.0-or-later. Source: &lt;a href=&apos;https://github.com/JimKnopfIoT/harbour-somble&apos;&gt;github.com/JimKnopfIoT/harbour-somble&lt;/a&gt;&lt;/p&gt;&lt;p&gt;“Omron” and “EVOLV” are trademarks of their respective owner. This project is not affiliated with, endorsed by, or supported by Omron.&lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BleTransport</name>
    <message>
        <source>Cannot talk to BlueZ.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No Bluetooth adapter found.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Found %1, connecting…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Scanning for the monitor…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Monitor not found. Press its Bluetooth button so it starts advertising, then try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bluetooth timed out. Wake the monitor and try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>This device does not expose the expected Omron data service.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Re-pairing with the monitor…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not connect to the monitor. Press its Bluetooth button, then try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor no longer accepts this phone&apos;s Bluetooth pairing. Open Settings → Bluetooth, remove “%1”, then pair it again (press the monitor&apos;s Bluetooth button so it appears).</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ChartPage</name>
    <message>
        <source>Export image (JPG)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Image saved: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Image export failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Export CSV</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>no data in range</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>● Sys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>● Dia</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>● Pulse</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Average: %1/%2 mmHg · ♥ %3 · n=%4</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No measurements in this range</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>%1 reading(s) not shown: taken while the monitor&apos;s clock was unset, so they have no date. Device → Set the monitor&apos;s clock fixes this for future readings.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>P%1 (%2)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>CoverPage</name>
    <message>
        <source> readings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>no data</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>DevicePage</name>
    <message>
        <source>Set the monitor&apos;s clock</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Writing the phone&apos;s time to the monitor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Read from device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Clock</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>not set — every reading is stored with the same meaningless timestamp until the clock is set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Setting the clock is the only thing this app ever writes to the monitor. It only affects readings taken afterwards — already-stored ones keep their timestamp.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Identification</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Nothing read yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pull down and read from the device.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MainPage</name>
    <message>
        <source>Select a saved archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Omron</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Raw data / pairing key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Export CSV</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Load archive file…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Save archive to file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pair with monitor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Download from device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pairing — put the monitor in pairing mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No measurements</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pair the monitor once (pull down → Pair with monitor, with the monitor in pairing mode), then download.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>arrhythmia</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Clear archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Clearing the local archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>no date — clock was unset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Restore deleted (%1)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Delete this reading</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Deleting</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Delete all</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Keep</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Really delete all readings?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>All %1 reading(s) are removed from the archive and will not reappear on the next download. The monitor&apos;s own memory is not touched. Export a CSV first if you want to keep them.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Delete all readings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>About</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Omron</name>
    <message>
        <source>Not connected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The pairing key must be 32 hex characters (16 bytes).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Connecting…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pairing…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cancelled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Downloaded: %1 new, %2 total</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No new readings — the archive is already up to date.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Failed: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Nothing captured yet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Re-parsed: +%1 (%2 total)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>--- EEPROM dump (%1 bytes, %2-byte records) ---
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Loaded %1 saved measurement(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Nothing to save.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cannot write %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Saved %1 readings: %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cannot open %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No measurements in that file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Loaded %1: +%2 (%3 total)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Archive cleared</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No measurements to export.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>CSV exported: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Paired. Now pull down and download.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Paired with the monitor. Pull down → Download from device.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Deleted readings will reappear on the next download.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Clock set.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor&apos;s clock is now set.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Setting the clock…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Deleted %1 reading(s). They will not come back on the next download.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>OmronProtocol</name>
    <message>
        <source>The pairing key must be exactly 16 bytes.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Looking for the monitor…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not subscribe to the monitor&apos;s control channel.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not subscribe to the monitor&apos;s data channels.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pairing: storing the key…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unlocking…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor did not accept pairing. Hold its Bluetooth button until the display shows a blinking “P”, then try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor stopped responding.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Finishing…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Reading measurements… %1%</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unexpected pairing response from the monitor.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor refused the pairing key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unlocked. Starting transfer…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor rejected the key. Pair with it again (pull down → Pair with monitor).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor did not start a transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unexpected reply while reading measurements.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The Bluetooth link dropped.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bluetooth address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Negotiated MTU</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Setting the monitor&apos;s clock…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor&apos;s clock was unset — setting it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Using the open connection…</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Manufacturer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Model</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Serial number</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Hardware revision</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Firmware revision</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Software revision</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Battery</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>RawPage</name>
    <message>
        <source>Re-parse capture</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Download from device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Raw data / pairing key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Pairing key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>The monitor stores a 16-byte key and only talks to whoever presents it. Pairing writes this key into the monitor. Change it only to re-use a key another tool already programmed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>16 bytes, 32 hex characters</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Apply key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Raw EEPROM capture</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>(nothing captured yet)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
