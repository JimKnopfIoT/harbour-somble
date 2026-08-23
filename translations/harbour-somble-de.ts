<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de">
<context>
    <name>AboutPage</name>
    <message>
        <source>About somble</source>
        <translation>Über somble</translation>
    </message>
    <message>
        <source>Version %1</source>
        <translation>Version %1</translation>
    </message>
    <message>
        <source>Status &amp; responsible use</source>
        <translation>Status &amp; verantwortungsvolle Nutzung</translation>
    </message>
    <message>
        <source>Attribution</source>
        <translation>Danksagung</translation>
    </message>
    <message>
        <source>&lt;p&gt;The wire protocol follows the community reverse-engineering in &lt;a href=&apos;https://github.com/userx14/omblepy&apos;&gt;omblepy&lt;/a&gt;, cross-checked against &lt;a href=&apos;https://codeberg.org/LazyT/ubpm&apos;&gt;UBPM&lt;/a&gt;. No code was copied from either; both are independent implementations of the same observed protocol.&lt;/p&gt;&lt;p&gt;Licensed GPL-3.0-or-later. Source: &lt;a href=&apos;https://github.com/JimKnopfIoT/harbour-somble&apos;&gt;github.com/JimKnopfIoT/harbour-somble&lt;/a&gt;&lt;/p&gt;&lt;p&gt;“Omron” and “EVOLV” are trademarks of their respective owner. This project is not affiliated with, endorsed by, or supported by Omron.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Das Übertragungsprotokoll folgt dem Reverse-Engineering der Community in &lt;a href=&apos;https://github.com/userx14/omblepy&apos;&gt;omblepy&lt;/a&gt;, gegengeprüft an &lt;a href=&apos;https://codeberg.org/LazyT/ubpm&apos;&gt;UBPM&lt;/a&gt;. Aus keinem der beiden Projekte wurde Quelltext übernommen; beide sind unabhängige Umsetzungen desselben beobachteten Protokolls.&lt;/p&gt;&lt;p&gt;Lizenziert unter GPL-3.0-or-later. Quelltext: &lt;a href=&apos;https://github.com/JimKnopfIoT/harbour-somble&apos;&gt;github.com/JimKnopfIoT/harbour-somble&lt;/a&gt;&lt;/p&gt;&lt;p&gt;„Omron“ und „EVOLV“ sind Marken des jeweiligen Inhabers. Dieses Projekt steht in keiner Verbindung zu Omron und wird von Omron weder unterstützt noch gebilligt.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>&lt;p&gt;Reads the measurements stored in an &lt;b&gt;Omron&lt;/b&gt; blood-pressure monitor over Bluetooth LE, charts them and exports them. Developed against an &lt;b&gt;EVOLV (HEM-7600T)&lt;/b&gt;; the memory maps of several related models are built in and picked from what the monitor reports about itself, or by hand under &lt;i&gt;Device → Monitor model&lt;/i&gt;. Only the EVOLV has been tested on real hardware — on the others the readings may decode wrongly, which shows up as missing or absurd values rather than as anything harmful.&lt;/p&gt;&lt;p&gt;It speaks to &lt;tt&gt;BlueZ&lt;/tt&gt; directly over D-Bus rather than through QtBluetooth, whose development headers the Sailfish target does not ship.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Getting started.&lt;/b&gt; The monitor has to be bonded once in Settings → Bluetooth — Sailfish does not let an ordinary app drive the pairing itself. Then &lt;i&gt;Pair with monitor&lt;/i&gt; here to store this app&apos;s key in it, and &lt;i&gt;Device → Set the monitor&apos;s clock&lt;/i&gt; where that is offered.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Set the clock first.&lt;/b&gt; The monitor is delivered with its clock unset and reports 2015-01-01 00:00 until it is set — and it stamps every reading from that clock. The timestamp lives inside the record and cannot be corrected afterwards, so readings taken beforehand keep no usable date. They are shown as &lt;i&gt;no date&lt;/i&gt; and left out of the chart rather than plotted on a made-up one. On models whose clock location is not confirmed the app refuses to write it at all — set the date and time on the monitor itself there.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Two people.&lt;/b&gt; Some models keep two separate user memories, and there the P1/P2 switch on each reading starts out as the memory the reading came from. Others — the EVOLV among them — store no user information at all and combine everyone&apos;s readings, so there P1/P2 is purely your own note. Either way it is kept in the archive and survives further downloads.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Liest die in einem &lt;b&gt;Omron&lt;/b&gt;-Blutdruckmessgerät gespeicherten Messwerte über Bluetooth LE aus, stellt sie dar und exportiert sie. Entwickelt für ein &lt;b&gt;EVOLV (HEM-7600T)&lt;/b&gt;; die Speicherbelegungen mehrerer verwandter Modelle sind eingebaut und werden aus dem gewählt, was das Messgerät über sich meldet — oder von Hand unter &lt;i&gt;Gerät → Gerätemodell&lt;/i&gt;. Nur das EVOLV wurde an echter Hardware getestet; bei den anderen können die Messwerte falsch dekodiert werden, was sich als fehlende oder absurde Werte zeigt, nicht als Schaden.&lt;/p&gt;&lt;p&gt;Die App spricht direkt über D-Bus mit &lt;tt&gt;BlueZ&lt;/tt&gt; statt über QtBluetooth, dessen Entwicklungsdateien im Sailfish-Target fehlen.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Erste Schritte.&lt;/b&gt; Das Messgerät muss einmalig in Einstellungen → Bluetooth gekoppelt werden — Sailfish erlaubt einer gewöhnlichen App nicht, die Kopplung selbst durchzuführen. Danach hier &lt;i&gt;Mit Messgerät koppeln&lt;/i&gt;, um den Schlüssel dieser App im Gerät zu hinterlegen, und &lt;i&gt;Gerät → Uhr des Messgeräts stellen&lt;/i&gt;, sofern das angeboten wird.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Zuerst die Uhr stellen.&lt;/b&gt; Das Messgerät wird mit ungestellter Uhr ausgeliefert und meldet bis dahin 2015-01-01 00:00 — und es stempelt jede Messung mit dieser Uhr. Der Zeitstempel steckt im Datensatz und lässt sich nachträglich nicht korrigieren; vorher vorgenommene Messungen behalten also kein brauchbares Datum. Sie werden als &lt;i&gt;ohne Datum&lt;/i&gt; angezeigt und aus dem Diagramm herausgehalten, statt sie auf ein erfundenes Datum zu setzen. Bei Modellen, deren Uhr-Speicherort nicht bestätigt ist, schreibt die App sie gar nicht — stelle Datum und Uhrzeit dort am Messgerät selbst ein.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Zwei Personen.&lt;/b&gt; Manche Modelle führen zwei getrennte Benutzerspeicher; dort steht der P1/P2-Schalter an jeder Messung zunächst auf dem Speicher, aus dem sie stammt. Andere — darunter das EVOLV — speichern überhaupt keine Benutzerkennung und vermischen die Messwerte aller Personen, dort ist P1/P2 rein deine eigene Zuordnung. So oder so wird sie im Archiv gespeichert und übersteht weitere Downloads.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>&lt;p&gt;&lt;b&gt;Proof of concept / work in progress.&lt;/b&gt; A hobby project, shared &lt;b&gt;as is&lt;/b&gt; with &lt;b&gt;no warranty&lt;/b&gt; of any kind (see the GPLv3). It may be incomplete, rough around the edges, or change without notice — use it at your own risk.&lt;/p&gt;&lt;p&gt;&lt;b&gt;This is not a medical device and displays no medically validated data.&lt;/b&gt; It only reads back what the monitor already stored. Never use it to diagnose, to judge a course of treatment, or to adjust medication — only a doctor is qualified to do that.&lt;/p&gt;&lt;p&gt;The protocol was reconstructed from community reverse-engineering, not from vendor documentation, so decoding errors are possible. The only write this app performs — setting the clock — touches the same memory region believed to hold the pressure-sensor calibration data, and its address differs per model. It therefore happens only on a monitor this app recognised, at an address a community driver writes too, and only after the block read back from that address checks out. Everything else is read-only. It is still your monitor and your risk.&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Machbarkeitsnachweis / in Arbeit.&lt;/b&gt; Ein Hobbyprojekt, bereitgestellt &lt;b&gt;wie es ist&lt;/b&gt;, &lt;b&gt;ohne jede Gewährleistung&lt;/b&gt; (siehe GPLv3). Es kann unvollständig oder unfertig sein und sich ohne Ankündigung ändern — die Nutzung erfolgt auf eigene Gefahr.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Dies ist kein Medizinprodukt und zeigt keine medizinisch validierten Daten an.&lt;/b&gt; Die App liest lediglich zurück, was das Messgerät bereits gespeichert hat. Nutze sie niemals zur Diagnose, zur Beurteilung einer Behandlung oder zur Anpassung von Medikamenten — dazu ist nur ein Arzt qualifiziert.&lt;/p&gt;&lt;p&gt;Das Protokoll wurde aus Reverse-Engineering der Community rekonstruiert, nicht aus Herstellerdokumentation; Dekodierfehler sind daher möglich. Der einzige Schreibvorgang dieser App — das Stellen der Uhr — berührt denselben Speicherbereich, in dem vermutlich die Kalibrierdaten des Drucksensors liegen, und seine Adresse ist von Modell zu Modell verschieden. Er findet deshalb nur an einem Messgerät statt, das die App erkannt hat, an einer Adresse, auf die auch ein Community-Treiber schreibt, und erst nachdem der von dort gelesene Block stimmig ist. Alles andere ist rein lesend. Es bleibt dein Messgerät und dein Risiko.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>BleTransport</name>
    <message>
        <source>Cannot talk to BlueZ.</source>
        <translation>Keine Verbindung zu BlueZ.</translation>
    </message>
    <message>
        <source>No Bluetooth adapter found.</source>
        <translation>Kein Bluetooth-Adapter gefunden.</translation>
    </message>
    <message>
        <source>Found %1, connecting…</source>
        <translation>%1 gefunden, verbinde …</translation>
    </message>
    <message>
        <source>Scanning for the monitor…</source>
        <translation>Suche das Messgerät …</translation>
    </message>
    <message>
        <source>Monitor not found. Press its Bluetooth button so it starts advertising, then try again.</source>
        <translation>Messgerät nicht gefunden. Drücke die Verbindungstaste am Gerät, damit es sichtbar wird, und versuche es erneut.</translation>
    </message>
    <message>
        <source>Bluetooth timed out. Wake the monitor and try again.</source>
        <translation>Zeitüberschreitung bei Bluetooth. Wecke das Messgerät und versuche es erneut.</translation>
    </message>
    <message>
        <source>This device does not expose the expected Omron data service.</source>
        <translation>Dieses Gerät bietet den erwarteten Omron-Datendienst nicht an.</translation>
    </message>
    <message>
        <source>Re-pairing with the monitor…</source>
        <translation>Koppele erneut mit dem Messgerät …</translation>
    </message>
    <message>
        <source>Could not connect to the monitor. Press its Bluetooth button, then try again.</source>
        <translation>Verbindung zum Messgerät fehlgeschlagen. Drücke die Verbindungstaste und versuche es erneut.</translation>
    </message>
    <message>
        <source>The monitor no longer accepts this phone&apos;s Bluetooth pairing. Open Settings → Bluetooth, remove “%1”, then pair it again (press the monitor&apos;s Bluetooth button so it appears).</source>
        <translation>Das Messgerät akzeptiert die Bluetooth-Kopplung dieses Telefons nicht mehr. Öffne Einstellungen → Bluetooth, entferne „%1“ und koppele erneut (drücke die Verbindungstaste, damit das Gerät erscheint).</translation>
    </message>
</context>
<context>
    <name>ChartPage</name>
    <message>
        <source>Export image (JPG)</source>
        <translation>Bild exportieren (JPG)</translation>
    </message>
    <message>
        <source>Image saved: </source>
        <translation>Bild gespeichert: </translation>
    </message>
    <message>
        <source>Image export failed</source>
        <translation>Bildexport fehlgeschlagen</translation>
    </message>
    <message>
        <source>Export CSV</source>
        <translation>CSV exportieren</translation>
    </message>
    <message>
        <source>no data in range</source>
        <translation>keine Daten im Bereich</translation>
    </message>
    <message>
        <source>● Sys</source>
        <translation>● Sys</translation>
    </message>
    <message>
        <source>● Dia</source>
        <translation>● Dia</translation>
    </message>
    <message>
        <source>● Pulse</source>
        <translation>● Puls</translation>
    </message>
    <message>
        <source>Average: %1/%2 mmHg · ♥ %3 · n=%4</source>
        <translation>Mittelwert: %1/%2 mmHg · ♥ %3 · n=%4</translation>
    </message>
    <message>
        <source>No measurements in this range</source>
        <translation>Keine Messwerte in diesem Bereich</translation>
    </message>
    <message>
        <source>%1 reading(s) not shown: taken while the monitor&apos;s clock was unset, so they have no date. Device → Set the monitor&apos;s clock fixes this for future readings.</source>
        <translation>%1 Messwert(e) nicht dargestellt: aufgezeichnet, während die Uhr des Messgeräts nicht gestellt war, daher ohne Datum. Gerät → Uhr des Messgeräts stellen behebt das für künftige Messungen.</translation>
    </message>
    <message>
        <source>P%1 (%2)</source>
        <translation>P%1 (%2)</translation>
    </message>
</context>
<context>
    <name>CoverPage</name>
    <message>
        <source> readings</source>
        <translation> Messwerte</translation>
    </message>
    <message>
        <source>no data</source>
        <translation>keine Daten</translation>
    </message>
</context>
<context>
    <name>DevicePage</name>
    <message>
        <source>Set the monitor&apos;s clock</source>
        <translation>Uhr des Messgeräts stellen</translation>
    </message>
    <message>
        <source>Writing the phone&apos;s time to the monitor</source>
        <translation>Telefonzeit wird ins Messgerät geschrieben</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Abbrechen</translation>
    </message>
    <message>
        <source>Read from device</source>
        <translation>Vom Gerät lesen</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Gerät</translation>
    </message>
    <message>
        <source>Clock</source>
        <translation>Uhr</translation>
    </message>
    <message>
        <source>not set — every reading is stored with the same meaningless timestamp until the clock is set</source>
        <translation>nicht gestellt — bis die Uhr gestellt ist, erhält jede Messung denselben bedeutungslosen Zeitstempel</translation>
    </message>
    <message>
        <source>Setting the clock is the only thing this app ever writes to the monitor. It only affects readings taken afterwards — already-stored ones keep their timestamp.</source>
        <translation>Das Stellen der Uhr ist das Einzige, was diese App jemals ins Messgerät schreibt. Es wirkt nur auf danach vorgenommene Messungen — bereits gespeicherte behalten ihren Zeitstempel.</translation>
    </message>
    <message>
        <source>Identification</source>
        <translation>Kennung</translation>
    </message>
    <message>
        <source>Nothing read yet</source>
        <translation>Noch nichts gelesen</translation>
    </message>
    <message>
        <source>Pull down and read from the device.</source>
        <translation>Herunterziehen und vom Gerät lesen.</translation>
    </message>
    <message>
        <source>Model</source>
        <translation>Modell</translation>
    </message>
    <message>
        <source>Monitor model</source>
        <translation>Gerätemodell</translation>
    </message>
    <message>
        <source>Where the readings and the clock sit in the monitor&apos;s memory differs per model. Leave this automatic unless the monitor is not recognised.</source>
        <translation>Wo die Messwerte und die Uhr im Speicher des Messgeräts liegen, ist von Modell zu Modell verschieden. Lass das auf automatisch, solange das Gerät erkannt wird.</translation>
    </message>
    <message>
        <source>Identify automatically</source>
        <translation>Automatisch erkennen</translation>
    </message>
    <message>
        <source>Decoding as: %1</source>
        <translation>Wird gelesen als: %1</translation>
    </message>
    <message>
        <source>cannot be read on this model</source>
        <translation>bei diesem Modell nicht lesbar</translation>
    </message>
    <message>
        <source>Setting the clock is the only thing this app would ever write to the monitor, and where the clock is stored is not confirmed for this model — a write to the wrong address could damage it. So somble does not write at all here: please set the date and time on the monitor itself, before taking readings.</source>
        <translation>Das Stellen der Uhr wäre das Einzige, was diese App jemals auf das Messgerät schreiben würde, und wo die Uhr bei diesem Modell liegt, ist nicht bestätigt — ein Schreibzugriff auf die falsche Adresse könnte es beschädigen. Deshalb schreibt somble hier gar nicht: Stelle Datum und Uhrzeit bitte am Messgerät selbst ein, bevor du misst.</translation>
    </message>
    <message>
        <source>not read yet</source>
        <translation>noch nicht gelesen</translation>
    </message>
    <message>
        <source>Whether the clock can be set depends on the model, and the monitor has not been read yet. Read from the device once and this will say.</source>
        <translation>Ob die Uhr gestellt werden kann, hängt vom Modell ab, und das Messgerät wurde noch nicht gelesen. Lies einmal vom Gerät, dann steht es hier.</translation>
    </message>
</context>
<context>
    <name>MainPage</name>
    <message>
        <source>Select a saved archive</source>
        <translation>Gespeichertes Archiv auswählen</translation>
    </message>
    <message>
        <source>Omron</source>
        <translation>Omron</translation>
    </message>
    <message>
        <source>Raw data / pairing key</source>
        <translation>Rohdaten / Kopplungsschlüssel</translation>
    </message>
    <message>
        <source>Export CSV</source>
        <translation>CSV exportieren</translation>
    </message>
    <message>
        <source>Load archive file…</source>
        <translation>Archivdatei laden …</translation>
    </message>
    <message>
        <source>Save archive to file</source>
        <translation>Archiv in Datei speichern</translation>
    </message>
    <message>
        <source>Pair with monitor</source>
        <translation>Mit Messgerät koppeln</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Abbrechen</translation>
    </message>
    <message>
        <source>Download from device</source>
        <translation>Vom Gerät herunterladen</translation>
    </message>
    <message>
        <source>Pairing — put the monitor in pairing mode</source>
        <translation>Kopplung — Messgerät in den Kopplungsmodus bringen</translation>
    </message>
    <message>
        <source>No measurements</source>
        <translation>Keine Messwerte</translation>
    </message>
    <message>
        <source>Pair the monitor once (pull down → Pair with monitor, with the monitor in pairing mode), then download.</source>
        <translation>Koppele das Messgerät einmalig (herunterziehen → Mit Messgerät koppeln, Gerät im Kopplungsmodus) und lade dann herunter.</translation>
    </message>
    <message>
        <source>arrhythmia</source>
        <translation>Arrhythmie</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Gerät</translation>
    </message>
    <message>
        <source>Clear archive</source>
        <translation>Archiv leeren</translation>
    </message>
    <message>
        <source>Clearing the local archive</source>
        <translation>Lokales Archiv wird geleert</translation>
    </message>
    <message>
        <source>no date — clock was unset</source>
        <translation>ohne Datum — Uhr war nicht gestellt</translation>
    </message>
    <message>
        <source>Restore deleted (%1)</source>
        <translation>Gelöschte wiederherstellen (%1)</translation>
    </message>
    <message>
        <source>Delete this reading</source>
        <translation>Diesen Messwert löschen</translation>
    </message>
    <message>
        <source>Deleting</source>
        <translation>Wird gelöscht</translation>
    </message>
    <message>
        <source>Delete all</source>
        <translation>Alle löschen</translation>
    </message>
    <message>
        <source>Keep</source>
        <translation>Behalten</translation>
    </message>
    <message>
        <source>Really delete all readings?</source>
        <translation>Wirklich alle Daten löschen?</translation>
    </message>
    <message>
        <source>All %1 reading(s) are removed from the archive and will not reappear on the next download. The monitor&apos;s own memory is not touched. Export a CSV first if you want to keep them.</source>
        <translation>Alle %1 Messwerte werden aus dem Archiv entfernt und erscheinen beim nächsten Download nicht wieder. Der Speicher des Messgeräts bleibt unangetastet. Exportiere vorher eine CSV, wenn du sie behalten möchtest.</translation>
    </message>
    <message>
        <source>Delete all readings</source>
        <translation>Alle Messwerte löschen</translation>
    </message>
    <message>
        <source>About</source>
        <translation>Über</translation>
    </message>
</context>
<context>
    <name>Omron</name>
    <message>
        <source>Not connected</source>
        <translation>Nicht verbunden</translation>
    </message>
    <message>
        <source>The pairing key must be 32 hex characters (16 bytes).</source>
        <translation>Der Kopplungsschlüssel muss 32 Hex-Zeichen (16 Byte) haben.</translation>
    </message>
    <message>
        <source>Connecting…</source>
        <translation>Verbinde …</translation>
    </message>
    <message>
        <source>Pairing…</source>
        <translation>Koppele …</translation>
    </message>
    <message>
        <source>Cancelled</source>
        <translation>Abgebrochen</translation>
    </message>
    <message>
        <source>Downloaded: %1 new, %2 total</source>
        <translation>Heruntergeladen: %1 neu, %2 gesamt</translation>
    </message>
    <message>
        <source>No new readings — the archive is already up to date.</source>
        <translation>Keine neuen Messwerte — das Archiv ist bereits aktuell.</translation>
    </message>
    <message>
        <source>Failed: %1</source>
        <translation>Fehlgeschlagen: %1</translation>
    </message>
    <message>
        <source>Nothing captured yet.</source>
        <translation>Noch nichts aufgezeichnet.</translation>
    </message>
    <message>
        <source>Re-parsed: +%1 (%2 total)</source>
        <translation>Neu ausgewertet: +%1 (%2 gesamt)</translation>
    </message>
    <message>
        <source>--- EEPROM dump (%1 bytes, %2-byte records) ---
</source>
        <translation>--- EEPROM-Abzug (%1 Byte, %2-Byte-Datensätze) ---
</translation>
    </message>
    <message>
        <source>Loaded %1 saved measurement(s)</source>
        <translation>%1 gespeicherte Messwert(e) geladen</translation>
    </message>
    <message>
        <source>Nothing to save.</source>
        <translation>Nichts zu speichern.</translation>
    </message>
    <message>
        <source>Cannot write %1</source>
        <translation>Kann %1 nicht schreiben</translation>
    </message>
    <message>
        <source>Saved %1 readings: %2</source>
        <translation>%1 Messwerte gespeichert: %2</translation>
    </message>
    <message>
        <source>Cannot open %1</source>
        <translation>Kann %1 nicht öffnen</translation>
    </message>
    <message>
        <source>No measurements in that file.</source>
        <translation>Keine Messwerte in dieser Datei.</translation>
    </message>
    <message>
        <source>Loaded %1: +%2 (%3 total)</source>
        <translation>%1 geladen: +%2 (%3 gesamt)</translation>
    </message>
    <message>
        <source>Archive cleared</source>
        <translation>Archiv geleert</translation>
    </message>
    <message>
        <source>No measurements to export.</source>
        <translation>Keine Messwerte zum Exportieren.</translation>
    </message>
    <message>
        <source>CSV exported: %1</source>
        <translation>CSV exportiert: %1</translation>
    </message>
    <message>
        <source>Paired. Now pull down and download.</source>
        <translation>Gekoppelt. Jetzt herunterziehen und herunterladen.</translation>
    </message>
    <message>
        <source>Paired with the monitor. Pull down → Download from device.</source>
        <translation>Mit dem Messgerät gekoppelt. Herunterziehen → Vom Gerät herunterladen.</translation>
    </message>
    <message>
        <source>Deleted readings will reappear on the next download.</source>
        <translation>Gelöschte Messwerte erscheinen beim nächsten Download wieder.</translation>
    </message>
    <message>
        <source>Clock set.</source>
        <translation>Uhr gestellt.</translation>
    </message>
    <message>
        <source>The monitor&apos;s clock is now set.</source>
        <translation>Die Uhr des Messgeräts ist jetzt gestellt.</translation>
    </message>
    <message>
        <source>Setting the clock…</source>
        <translation>Stelle die Uhr …</translation>
    </message>
    <message>
        <source>Deleted %1 reading(s). They will not come back on the next download.</source>
        <translation>%1 Messwert(e) gelöscht. Sie kommen beim nächsten Download nicht zurück.</translation>
    </message>
    <message>
        <source>This monitor did not identify itself as a model somble knows (%1). The readings below may be nonsense — pick the model on the Device page.</source>
        <translation>Dieses Messgerät hat sich nicht als ein Modell zu erkennen gegeben, das somble kennt (%1). Die Messwerte unten können Unsinn sein — wähle das Modell auf der Geräteseite aus.</translation>
    </message>
    <message>
        <source>Unknown monitor model: %1</source>
        <translation>Unbekanntes Gerätemodell: %1</translation>
    </message>
    <message>
        <source>unrecognised model</source>
        <translation>nicht erkanntes Modell</translation>
    </message>
    <message>
        <source>not identified yet</source>
        <translation>noch nicht bestimmt</translation>
    </message>
</context>
<context>
    <name>OmronProtocol</name>
    <message>
        <source>The pairing key must be exactly 16 bytes.</source>
        <translation>Der Kopplungsschlüssel muss genau 16 Byte lang sein.</translation>
    </message>
    <message>
        <source>Looking for the monitor…</source>
        <translation>Suche das Messgerät …</translation>
    </message>
    <message>
        <source>Could not subscribe to the monitor&apos;s control channel.</source>
        <translation>Der Steuerkanal des Messgeräts ließ sich nicht abonnieren.</translation>
    </message>
    <message>
        <source>Could not subscribe to the monitor&apos;s data channels.</source>
        <translation>Die Datenkanäle des Messgeräts ließen sich nicht abonnieren.</translation>
    </message>
    <message>
        <source>Pairing: storing the key…</source>
        <translation>Kopplung: Schlüssel wird gespeichert …</translation>
    </message>
    <message>
        <source>Unlocking…</source>
        <translation>Entsperre …</translation>
    </message>
    <message>
        <source>The monitor did not accept pairing. Hold its Bluetooth button until the display shows a blinking “P”, then try again.</source>
        <translation>Das Messgerät hat die Kopplung nicht angenommen. Halte die Verbindungstaste gedrückt, bis im Display ein blinkendes „P“ erscheint, und versuche es erneut.</translation>
    </message>
    <message>
        <source>The monitor stopped responding.</source>
        <translation>Das Messgerät antwortet nicht mehr.</translation>
    </message>
    <message>
        <source>Finishing…</source>
        <translation>Schließe ab …</translation>
    </message>
    <message>
        <source>Reading measurements… %1%</source>
        <translation>Lese Messwerte … %1 %</translation>
    </message>
    <message>
        <source>Unexpected pairing response from the monitor.</source>
        <translation>Unerwartete Kopplungsantwort vom Messgerät.</translation>
    </message>
    <message>
        <source>The monitor refused the pairing key.</source>
        <translation>Das Messgerät hat den Kopplungsschlüssel abgelehnt.</translation>
    </message>
    <message>
        <source>Unlocked. Starting transfer…</source>
        <translation>Entsperrt. Starte Übertragung …</translation>
    </message>
    <message>
        <source>The monitor rejected the key. Pair with it again (pull down → Pair with monitor).</source>
        <translation>Das Messgerät hat den Schlüssel abgelehnt. Koppele erneut (herunterziehen → Mit Messgerät koppeln).</translation>
    </message>
    <message>
        <source>The monitor did not start a transfer.</source>
        <translation>Das Messgerät hat keine Übertragung gestartet.</translation>
    </message>
    <message>
        <source>Unexpected reply while reading measurements.</source>
        <translation>Unerwartete Antwort beim Lesen der Messwerte.</translation>
    </message>
    <message>
        <source>The Bluetooth link dropped.</source>
        <translation>Die Bluetooth-Verbindung wurde getrennt.</translation>
    </message>
    <message>
        <source>Bluetooth address</source>
        <translation>Bluetooth-Adresse</translation>
    </message>
    <message>
        <source>Negotiated MTU</source>
        <translation>Ausgehandelte MTU</translation>
    </message>
    <message>
        <source>Setting the monitor&apos;s clock…</source>
        <translation>Stelle die Uhr des Messgeräts …</translation>
    </message>
    <message>
        <source>The monitor&apos;s clock was unset — setting it.</source>
        <translation>Die Uhr des Messgeräts war nicht gestellt — sie wird jetzt gestellt.</translation>
    </message>
    <message>
        <source>Using the open connection…</source>
        <translation>Nutze die bestehende Verbindung …</translation>
    </message>
    <message>
        <source>Manufacturer</source>
        <translation>Hersteller</translation>
    </message>
    <message>
        <source>Model</source>
        <translation>Modell</translation>
    </message>
    <message>
        <source>Serial number</source>
        <translation>Seriennummer</translation>
    </message>
    <message>
        <source>Hardware revision</source>
        <translation>Hardware-Version</translation>
    </message>
    <message>
        <source>Firmware revision</source>
        <translation>Firmware-Version</translation>
    </message>
    <message>
        <source>Software revision</source>
        <translation>Software-Version</translation>
    </message>
    <message>
        <source>Battery</source>
        <translation>Batterie</translation>
    </message>
    <message>
        <source>EVOLV (HEM-7600T)</source>
        <translation>EVOLV (HEM-7600T)</translation>
    </message>
    <message>
        <source>M700 Intelli IT (HEM-7322T)</source>
        <translation>M700 Intelli IT (HEM-7322T)</translation>
    </message>
    <message>
        <source>RS7 Intelli IT (HEM-6232T)</source>
        <translation>RS7 Intelli IT (HEM-6232T)</translation>
    </message>
    <message>
        <source>Complete (HEM-7530T)</source>
        <translation>Complete (HEM-7530T)</translation>
    </message>
    <message>
        <source>M500 / M7 Intelli IT (HEM-7361T)</source>
        <translation>M500 / M7 Intelli IT (HEM-7361T)</translation>
    </message>
    <message>
        <source>BP7450 (HEM-7342T)</source>
        <translation>BP7450 (HEM-7342T)</translation>
    </message>
    <message>
        <source>M400 / X4 smart (HEM-7155T)</source>
        <translation>M400 / X4 smart (HEM-7155T)</translation>
    </message>
    <message>
        <source>BP7250 (HEM-7150T)</source>
        <translation>BP7250 (HEM-7150T)</translation>
    </message>
    <message>
        <source>unrecognised model</source>
        <translation>nicht erkanntes Modell</translation>
    </message>
    <message>
        <source>Record layout</source>
        <translation>Datenformat</translation>
    </message>
    <message>
        <source>This monitor did not identify itself as a model somble knows, so it does not know where the clock is stored. Set the date and time on the monitor itself, or pick the model by hand on the Device page.</source>
        <translation>Dieses Messgerät hat sich nicht als ein Modell zu erkennen gegeben, das somble kennt — somble weiß also nicht, wo die Uhr gespeichert ist. Stelle Datum und Uhrzeit am Messgerät selbst ein oder wähle das Modell von Hand auf der Geräteseite.</translation>
    </message>
    <message>
        <source>Setting the clock is not supported on the %1: the place it is stored has not been confirmed for this model, and writing to the wrong address could damage it. Please set the date and time on the monitor itself.</source>
        <translation>Das Stellen der Uhr wird beim %1 nicht unterstützt: Wo sie gespeichert ist, ist für dieses Modell nicht bestätigt, und ein Schreibzugriff auf die falsche Adresse könnte es beschädigen. Stelle Datum und Uhrzeit bitte am Messgerät selbst ein.</translation>
    </message>
    <message>
        <source>The monitor&apos;s clock did not read back the way this model stores it, so somble did not write to it. Please set the date and time on the monitor itself.</source>
        <translation>Die Uhr des Messgeräts hat sich nicht so gelesen, wie dieses Modell sie ablegt — somble hat deshalb nicht geschrieben. Stelle Datum und Uhrzeit bitte am Messgerät selbst ein.</translation>
    </message>
    <message>
        <source>The monitor did not confirm the clock write.</source>
        <translation>Das Messgerät hat das Stellen der Uhr nicht bestätigt.</translation>
    </message>
</context>
<context>
    <name>RawPage</name>
    <message>
        <source>Re-parse capture</source>
        <translation>Aufzeichnung neu auswerten</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Abbrechen</translation>
    </message>
    <message>
        <source>Download from device</source>
        <translation>Vom Gerät herunterladen</translation>
    </message>
    <message>
        <source>Raw data / pairing key</source>
        <translation>Rohdaten / Kopplungsschlüssel</translation>
    </message>
    <message>
        <source>Pairing key</source>
        <translation>Kopplungsschlüssel</translation>
    </message>
    <message>
        <source>The monitor stores a 16-byte key and only talks to whoever presents it. Pairing writes this key into the monitor. Change it only to re-use a key another tool already programmed.</source>
        <translation>Das Messgerät speichert einen 16-Byte-Schlüssel und spricht nur mit dem, der ihn vorzeigt. Beim Koppeln wird dieser Schlüssel ins Gerät geschrieben. Ändere ihn nur, um einen Schlüssel wiederzuverwenden, den ein anderes Werkzeug bereits programmiert hat.</translation>
    </message>
    <message>
        <source>16 bytes, 32 hex characters</source>
        <translation>16 Byte, 32 Hex-Zeichen</translation>
    </message>
    <message>
        <source>Apply key</source>
        <translation>Schlüssel übernehmen</translation>
    </message>
    <message>
        <source>Raw EEPROM capture</source>
        <translation>EEPROM-Rohabzug</translation>
    </message>
    <message>
        <source>(nothing captured yet)</source>
        <translation>(noch nichts aufgezeichnet)</translation>
    </message>
</context>
</TS>
