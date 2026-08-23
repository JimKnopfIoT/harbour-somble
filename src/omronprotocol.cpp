#include "omronprotocol.h"
#include "bletransport.h"

#include <QDateTime>
#include <QTimer>
#include <QVariantMap>
#include <QCoreApplication>
#include <QDebug>

namespace {

// --- GATT layout (Omron "legacy" vendor service) ---------------------------
const QString kParentServiceUuid = QStringLiteral("ecbe3980-c9a2-11e1-b1bd-0002a5d5c51b");
const QString kUnlockUuid        = QStringLiteral("b305b680-aee7-11e1-a730-0002a5d5c51b");

const char *kTxUuids[4] = {
    "db5b55e0-aee7-11e1-965e-0002a5d5c51b",
    "e0b8a060-aee7-11e1-92f4-0002a5d5c51b",
    "0ae12b00-aee8-11e1-a192-0002a5d5c51b",
    "10e1ba60-aee8-11e1-89e5-0002a5d5c51b",
};
const char *kRxUuids[4] = {
    "49123040-aee8-11e1-a74d-0002a5d5c51b",
    "4d0bf320-aee8-11e1-a0d9-0002a5d5c51b",
    "5128ce60-aee8-11e1-b84b-0002a5d5c51b",
    "560f1420-aee8-11e1-8184-0002a5d5c51b",
};

// --- record layouts --------------------------------------------------------
//
// Bit numbering follows the protocol description: the record is one integer,
// bit 0 is its most significant bit. Most models store it big-endian, the
// M4/M5/M7 family little-endian, which is why the layouts below look so
// different despite describing the same handful of values.
const RecordLayout kLayoutBE14 = {
    true, 14,
    /*dia*/    {  0,   7 }, /*sys*/ {  8,  15 }, /*year*/ { 16, 23 }, /*bpm*/ { 24, 31 },
    /*mov*/    { 32,  32 }, /*ihb*/ { 33,  33 },
    /*month*/  { 34,  37 }, /*day*/ { 38,  42 }, /*hour*/ { 43, 47 },
    /*minute*/ { 52,  57 }, /*second*/ { 58, 63 },
    /*arrCnt*/ { 64,  71 },
};

// Same, but the year shares its byte with two other bits, so it is six wide.
const RecordLayout kLayoutBE14y6 = {
    true, 14,
    {  0,   7 }, {  8,  15 }, { 18, 23 }, { 24, 31 },
    /*mov*/ { 32,  32 }, /*ihb*/ { 33,  33 },
    { 34,  37 }, { 38,  42 }, { 43, 47 },
    { 52,  57 }, { 58,  63 },
    { 64,  71 },
};

// ...and on the RS7 the movement and irregular-heartbeat flags are the other
// way round. One bit, but it decides which warning a reading carries.
const RecordLayout kLayoutBE14y6Swapped = {
    true, 14,
    {  0,   7 }, {  8,  15 }, { 18, 23 }, { 24, 31 },
    /*mov*/ { 33,  33 }, /*ihb*/ { 32,  32 },
    { 34,  37 }, { 38,  42 }, { 43, 47 },
    { 52,  57 }, { 58,  63 },
    { 64,  71 },
};

// The 16-byte little-endian record. No arrhythmia counter is known here, so it
// is left out rather than guessed at.
const RecordLayout kLayoutLE16 = {
    false, 16,
    /*dia*/    { 112, 119 }, /*sys*/ { 120, 127 }, /*year*/ { 98, 103 }, /*bpm*/ { 104, 111 },
    /*mov*/    {  80,  80 }, /*ihb*/ {  81,  81 },
    /*month*/  {  82,  85 }, /*day*/ {  86,  90 }, /*hour*/ { 91, 95 },
    /*minute*/ {  68,  73 }, /*second*/ { 74, 79 },
    /*arrCnt*/ {   0,  -1 },
};

// --- device profiles -------------------------------------------------------
//
// The addresses follow omblepy's per-device drivers, which is where they were
// reverse-engineered; the marketing names come from its device table, because
// that is what is printed on the monitor.
//
// Note the two windows: the same settings block is read at settingsRead and
// written at settingsWrite, and both are model-specific. Deriving the clock
// write address from the wrong model's window is how a harmless clock set turns
// into a write into whatever else lives there — hence clockWritable, which is
// set only where a community driver writes that address too. Even then the
// block has to verify at run time before anything is sent (clockBlockTrusted).
//
// Not in this table: HEM-7380T1 (X7 Smart AFib) and HEM-7377T1 (BP5360). They
// speak the same commands over a different GATT service with one channel
// instead of four and no unlock step, which is a transport change rather than a
// map change.
const DeviceProfile kProfiles[] = {
    // The unit this app was developed against, and the only one whose clock
    // write has been confirmed here rather than inferred.
    { "hem-7600t", QT_TRANSLATE_NOOP("OmronProtocol", "EVOLV (HEM-7600T)"),
      "HEM-7600|7600T|EVOLV",
      { 0x02ac, 0x0000 }, 100, 0x38, 0x0260, 0x0286, 8, 0x14, 10, ClockBE10,
      &kLayoutBE14, true },

    { "hem-7322t", QT_TRANSLATE_NOOP("OmronProtocol", "M700 Intelli IT (HEM-7322T)"),
      "HEM-7322|7322T|M700",
      { 0x02ac, 0x0824 }, 100, 0x38, 0x0260, 0x0286, 8, 0x14, 10, ClockBE10,
      &kLayoutBE14, true },

    // RS7 Intelli IT. Two user banks instead of one, a six-bit year, and the
    // movement / irregular-heartbeat flags swapped. omblepy marks this model's
    // clock offset as unconfirmed and refuses to sync time on it; so does this.
    { "hem-6232t", QT_TRANSLATE_NOOP("OmronProtocol", "RS7 Intelli IT (HEM-6232T)"),
      "HEM-6232|6232T|RS7",
      { 0x02e8, 0x0860 }, 100, 0x38, 0x0260, 0x02a4, 8, 0x14, 10, ClockNone,
      &kLayoutBE14y6Swapped, false },

    // Omron Complete. 90 slots, and its clock offset is commented out upstream.
    { "hem-7530t", QT_TRANSLATE_NOOP("OmronProtocol", "Complete (HEM-7530T)"),
      "HEM-7530|7530T|COMPLETE",
      { 0x02e8, 0x0000 }, 90, 0x10, 0x0260, 0x02a4, 8, 0x14, 10, ClockNone,
      &kLayoutBE14y6, false },

    // The little-endian family: same commands, a 16-byte record, and the
    // settings block in an entirely different part of the EEPROM.
    { "hem-7361t", QT_TRANSLATE_NOOP("OmronProtocol", "M500 / M7 Intelli IT (HEM-7361T)"),
      "HEM-7361|7361T|M500|M7 INTELLI",
      { 0x0098, 0x06d8 }, 100, 0x10, 0x0010, 0x0054, 16, 0x2c, 16, ClockLE16,
      &kLayoutLE16, true },

    { "hem-7342t", QT_TRANSLATE_NOOP("OmronProtocol", "BP7450 (HEM-7342T)"),
      "HEM-7342|7342T|BP7450",
      { 0x0098, 0x06d8 }, 100, 0x10, 0x0010, 0x0054, 16, 0x2c, 16, ClockLE16,
      &kLayoutLE16, true },

    { "hem-7155t", QT_TRANSLATE_NOOP("OmronProtocol", "M400 / X4 smart (HEM-7155T)"),
      "HEM-7155|7155T|M400|X4 SMART|M4 SMART",
      { 0x0098, 0x0458 }, 60, 0x10, 0x0010, 0x0054, 16, 0x2c, 16, ClockLE16,
      &kLayoutLE16, true },

    { "hem-7150t", QT_TRANSLATE_NOOP("OmronProtocol", "BP7250 (HEM-7150T)"),
      "HEM-7150|7150T|BP7250",
      { 0x0098, 0x0000 }, 60, 0x10, 0x0010, 0x0054, 16, 0x2c, 16, ClockLE16,
      &kLayoutLE16, true },
};

// Used when the monitor's model string matches nothing: read with the EVOLV
// geometry — it is the only one this app was developed against — but never
// write, and expect the records to be nonsense if the guess is wrong.
const DeviceProfile kFallbackProfile = {
    "unknown", QT_TRANSLATE_NOOP("OmronProtocol", "unrecognised model"),
    "",
    { 0x02ac, 0x0000 }, 100, 0x38, 0x0260, 0x0286, 8, 0x14, 10, ClockBE10,
    &kLayoutBE14, false,
};


// --- packet types ----------------------------------------------------------
const quint16 kTypeStart    = 0x0000;
const quint16 kTypeRead     = 0x0100;
const quint16 kTypeWrite    = 0x01c0;
const quint16 kTypeEnd      = 0x0f00;
const quint16 kRespStart    = 0x8000;
const quint16 kRespRead     = 0x8100;
const quint16 kRespWrite    = 0x81c0;
const quint16 kRespEnd      = 0x8f00;

const int kChannelSize = 16;    // protocol framing, not the ATT MTU
const int kTimeoutMs   = 1500;
const int kMaxRetries  = 5;

QStringList allUuids()
{
    QStringList l;
    l << kUnlockUuid;
    for (int i = 0; i < 4; ++i) l << QString::fromLatin1(kTxUuids[i]);
    for (int i = 0; i < 4; ++i) l << QString::fromLatin1(kRxUuids[i]);
    return l;
}

// Standard Device Information (0x180A) and Battery (0x180F) characteristics.
// Read once per session and reported for display; none of them are required
// for the transfer, so a missing one is simply left out.
struct InfoField { const char *uuid; const char *label; bool numeric; };
// The labels are looked up at runtime, which lupdate cannot see through, so
// they are marked here. QT_TRANSLATE_NOOP rather than QT_TR_NOOP because this
// table sits outside the class and would otherwise land in no context at all.
const InfoField kInfoFields[] = {
    { "00002a29-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Manufacturer"),      false },
    { "00002a24-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Model"),             false },
    { "00002a25-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Serial number"),     false },
    { "00002a27-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Hardware revision"), false },
    { "00002a26-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Firmware revision"), false },
    { "00002a28-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Software revision"), false },
    { "00002a19-0000-1000-8000-00805f9b34fb", QT_TRANSLATE_NOOP("OmronProtocol", "Battery"),           true  },
};

int rxChannelOf(const QString &uuid)
{
    for (int i = 0; i < 4; ++i)
        if (uuid.compare(QLatin1String(kRxUuids[i]), Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

} // namespace

QList<const DeviceProfile *> OmronProtocol::knownProfiles()
{
    QList<const DeviceProfile *> l;
    for (const DeviceProfile &p : kProfiles)
        l.append(&p);
    return l;
}

const DeviceProfile *OmronProtocol::fallbackProfile()
{
    return &kFallbackProfile;
}

const DeviceProfile *OmronProtocol::profileById(const QString &id)
{
    for (const DeviceProfile &p : kProfiles)
        if (id.compare(QLatin1String(p.id), Qt::CaseInsensitive) == 0)
            return &p;
    return nullptr;
}

OmronProtocol::OmronProtocol(QObject *parent)
    : QObject(parent)
    , m_ble(new BleTransport(this))
    , m_timer(new QTimer(this))
    , m_key(defaultKey())
{
    m_profile = &kFallbackProfile;

    // The monitor advertises under more than one name — "EVOLV" in some states,
    // the generic "BLEsmart_<model><mac>" in others — and any friendly Alias a
    // previous pairing left behind vanishes when that pairing is removed. So
    // match on either form, and fall back to the vendor service UUID, which is
    // the only genuinely stable identifier.
    m_ble->setNamePrefixes(QStringList()
                           << QStringLiteral("EVOLV")
                           << QStringLiteral("BLEsmart_")
                           << QStringLiteral("OMRON"));
    m_ble->setServiceUuid(kParentServiceUuid);
    m_ble->setRequiredUuids(allUuids());

    m_timer->setSingleShot(true);
    m_timer->setInterval(kTimeoutMs);

    connect(m_ble, &BleTransport::ready, this, &OmronProtocol::onBleReady);
    connect(m_ble, &BleTransport::notified, this, &OmronProtocol::onBleNotified);
    connect(m_ble, &BleTransport::errorOccurred, this, &OmronProtocol::onBleError);
    connect(m_ble, &BleTransport::closed, this, &OmronProtocol::onBleClosed);
    connect(m_ble, &BleTransport::progress, this, &OmronProtocol::progress);
    connect(m_timer, &QTimer::timeout, this, &OmronProtocol::onTimeout);

    for (int i = 0; i < 4; ++i)
        m_rx.append(QByteArray());
}

QByteArray OmronProtocol::defaultKey()
{
    // Arbitrary — the monitor stores whatever 16 bytes it is given and only
    // compares them back. Kept identical to omblepy's so a monitor already
    // paired with that tool works here without reprogramming.
    return QByteArray::fromHex("deadbeaf12341234deadbeaf12341234");
}

// --- packet construction ---------------------------------------------------

QByteArray OmronProtocol::buildCommand(quint16 type, quint16 address, quint8 length)
{
    QByteArray p;
    p.append(char(0x08));                        // total size: header + trailer
    p.append(char((type >> 8) & 0xff));
    p.append(char(type & 0xff));
    p.append(char((address >> 8) & 0xff));
    p.append(char(address & 0xff));
    p.append(char(length));
    p.append(char(0x00));                        // padding

    quint8 crc = 0;
    for (int i = 0; i < p.size(); ++i)
        crc ^= quint8(p.at(i));
    p.append(char(crc));            // XOR over the whole packet is now 0
    return p;
}

QByteArray OmronProtocol::buildWrite(quint16 address, const QByteArray &data)
{
    QByteArray p;
    p.append(char(8 + data.size()));
    p.append(char((kTypeWrite >> 8) & 0xff));
    p.append(char(kTypeWrite & 0xff));
    p.append(char((address >> 8) & 0xff));
    p.append(char(address & 0xff));
    p.append(char(data.size()));
    p.append(data);
    p.append(char(0x00));

    quint8 crc = 0;
    for (int i = 0; i < p.size(); ++i)
        crc ^= quint8(p.at(i));
    p.append(char(crc));
    return p;
}

// --- clock block -----------------------------------------------------------
//
// Two shapes, both a run of opaque bytes the monitor wrote and we must hand
// back untouched, then the date and time as plain bytes, then an additive
// checksum over everything before it — not the XOR the packet layer uses.
//
// ClockBE10, ten bytes (EVOLV, M700, and the rest of the big-endian family):
//   [0..1] opaque   [2] month   [3] year-2000   [4] hour
//   [5] day         [6] second  [7] minute      [8] 0x00 pad
//   [9] sum of bytes 0..7, low byte
//
// ClockLE16, sixteen bytes (M400/M500/M7 family):
//   [0..7] opaque   [8] year-2000   [9] month   [10] day
//   [11] hour       [12] minute     [13] second
//   [14] sum of bytes 0..13, low byte   [15] 0x00 pad

namespace {

// Where the date starts, how many bytes precede the checksum, and where it sits.
struct ClockShape { int opaque; int sumEnd; int csum; int len; };

ClockShape clockShape(ClockStyle style)
{
    return style == ClockLE16 ? ClockShape{ 8, 14, 14, 16 }
                              : ClockShape{ 2,  8,  9, 10 };
}

} // namespace

static QDateTime clockFromBlock(const QByteArray &b, ClockStyle style)
{
    const ClockShape sh = clockShape(style);
    if (b.size() < sh.csum)
        return QDateTime();
    int year, month, day, hour, minute, second;
    if (style == ClockLE16) {
        year   = 2000 + quint8(b.at(8));
        month  = quint8(b.at(9));
        day    = quint8(b.at(10));
        hour   = quint8(b.at(11));
        minute = quint8(b.at(12));
        second = quint8(b.at(13));
    } else {
        month  = quint8(b.at(2));
        year   = 2000 + quint8(b.at(3));
        hour   = quint8(b.at(4));
        day    = quint8(b.at(5));
        second = quint8(b.at(6));
        minute = quint8(b.at(7));
    }
    const QDate date(year, month, day);
    const QTime time(hour, minute, qMin(second, 59));
    if (!date.isValid() || !time.isValid())
        return QDateTime();
    return QDateTime(date, time);
}

static QByteArray clockToBlock(const QByteArray &previous, const QDateTime &now,
                               ClockStyle style)
{
    const ClockShape sh = clockShape(style);
    QByteArray b = previous.left(sh.opaque);
    while (b.size() < sh.opaque)
        b.append(char(0));

    if (style == ClockLE16) {
        b.append(char(now.date().year() - 2000));
        b.append(char(now.date().month()));
        b.append(char(now.date().day()));
        b.append(char(now.time().hour()));
        b.append(char(now.time().minute()));
        b.append(char(now.time().second()));
    } else {
        b.append(char(now.date().month()));
        b.append(char(now.date().year() - 2000));
        b.append(char(now.time().hour()));
        b.append(char(now.date().day()));
        b.append(char(now.time().second()));
        b.append(char(now.time().minute()));
        b.append(char(0x00));                    // pad, before the checksum
    }

    int sum = 0;
    for (int i = 0; i < sh.sumEnd && i < b.size(); ++i)
        sum += quint8(b.at(i));
    b.append(char(sum & 0xff));
    while (b.size() < sh.len)
        b.append(char(0));                       // trailing pad, where there is one
    return b;
}

// --- session ---------------------------------------------------------------

void OmronProtocol::start(bool pairFirst, bool setClock, bool clockOnly)
{
    if (m_state != StIdle)
        return;
    if (m_key.size() != 16) {
        emit failed(tr("The pairing key must be exactly 16 bytes."));
        return;
    }
    m_pairFirst = pairFirst;
    m_setClock = setClock;
    m_clockOnly = clockOnly;
    m_clockWritten = false;
    m_retries = 0;
    m_bank = 0;
    // A pinned model wins outright; otherwise start from the fallback geometry
    // and let readDeviceInfo() narrow it down once the monitor answers.
    if (const DeviceProfile *p = profileById(m_override))
        m_profile = p;
    else
        m_profile = &kFallbackProfile;
    m_dump.clear();
    m_settings.clear();
    m_timeBlock.clear();
    for (int i = 0; i < 4; ++i)
        m_rx[i].clear();

    if (m_ble->isOpen()) {
        // A previous step (pairing) left the link up — use it rather than
        // making the user wake the monitor again.
        emit progress(tr("Using the open connection…"));
        QTimer::singleShot(0, this, &OmronProtocol::onBleReady);
        return;
    }
    emit progress(tr("Looking for the monitor…"));
    m_ble->open();
}

void OmronProtocol::cancel()
{
    m_timer->stop();
    m_state = StIdle;
    m_ble->close();
}

void OmronProtocol::fail(const QString &message)
{
    m_timer->stop();
    m_state = StIdle;
    m_ble->close();
    emit failed(message);
}

void OmronProtocol::done()
{
    m_timer->stop();
    m_state = StIdle;
    m_ble->close();
    // A clock-only session read no records. Reporting it as a download would
    // overwrite the last raw capture with nothing and announce "0 new" — so it
    // reports what it actually did, or why it did not.
    if (m_clockOnly) {
        if (m_clockWritten)
            emit clockSet();
        else if (!m_setClock)           // decided against it, and why
            emit failed(clockRefusalReason());
        else                            // sent, but the monitor never said so
            emit failed(tr("The monitor did not confirm the clock write."));
        return;
    }
    emit finished(parseRecords(m_dump, QString::fromLatin1(m_profile->id)), m_dump);
}

void OmronProtocol::onBleReady()
{
    readDeviceInfo();

    if (!m_ble->startNotify(kUnlockUuid)) {
        fail(tr("Could not subscribe to the monitor's control channel."));
        return;
    }
    for (int i = 0; i < 4; ++i) {
        if (!m_ble->startNotify(QString::fromLatin1(kRxUuids[i]))) {
            fail(tr("Could not subscribe to the monitor's data channels."));
            return;
        }
    }

    if (m_pairFirst) {
        emit progress(tr("Pairing: storing the key…"));
        m_state = StPairEnterMode;
        sendUnlockFrame(0x02, QByteArray(16, '\0'));
    } else {
        emit progress(tr("Unlocking…"));
        m_state = StUnlock;
        sendUnlockFrame(0x01, m_key);
    }
}

void OmronProtocol::readDeviceInfo()
{
    QVariantList rows;
    QString modelString;
    for (const InfoField &f : kInfoFields) {
        bool ok = false;
        const QByteArray v = m_ble->readValue(QString::fromLatin1(f.uuid), &ok);
        if (!ok || v.isEmpty())
            continue;
        const QString text = QString::fromLatin1(v).trimmed();
        if (qstrcmp(f.label, "Model") == 0)
            modelString = text;
        QVariantMap row;
        row.insert(QStringLiteral("label"),
                   QCoreApplication::translate("OmronProtocol", f.label));
        row.insert(QStringLiteral("value"),
                   f.numeric ? QString::number(quint8(v.at(0))) + QStringLiteral(" %")
                             : text);
        rows.append(row);
    }

    resolveProfile(modelString);
    QVariantMap prof;
    prof.insert(QStringLiteral("label"), tr("Record layout"));
    prof.insert(QStringLiteral("value"),
                QCoreApplication::translate("OmronProtocol", m_profile->label));
    rows.append(prof);

    QVariantMap addr;
    addr.insert(QStringLiteral("label"), tr("Bluetooth address"));
    addr.insert(QStringLiteral("value"), m_ble->deviceAddress());
    rows.append(addr);

    QVariantMap mtu;
    mtu.insert(QStringLiteral("label"), tr("Negotiated MTU"));
    mtu.insert(QStringLiteral("value"), QString::number(m_ble->mtu() + 3));
    rows.append(mtu);

    emit deviceInfo(rows);
}

// The Model Number String is the only self-description these monitors offer,
// and it is not guaranteed to spell the model out — so the BLE name, which
// carries a model code on the "BLEsmart_" form, gets a look too. Anything that
// matches nothing keeps the fallback: readable, never written to.
void OmronProtocol::resolveProfile(const QString &modelString)
{
    if (profileById(m_override)) {
        // Pinned by the user in the UI. Say so, but do not second-guess it:
        // the whole point of the override is a monitor whose model string this
        // table does not recognise.
        emit profileResolved(QString::fromLatin1(m_profile->id),
                             QCoreApplication::translate("OmronProtocol", m_profile->label),
                             true);
        return;
    }

    const QString haystack = (modelString + QLatin1Char(' ') + m_ble->deviceName()).toUpper();
    for (const DeviceProfile &p : kProfiles) {
        const QStringList needles =
            QString::fromLatin1(p.matches).split(QLatin1Char('|'), QString::SkipEmptyParts);
        for (const QString &n : needles) {
            if (!n.isEmpty() && haystack.contains(n.toUpper())) {
                m_profile = &p;
                qWarning() << "somble: model" << modelString << "-> profile" << p.id;
                emit profileResolved(QString::fromLatin1(p.id),
                                     QCoreApplication::translate("OmronProtocol", p.label),
                                     true);
                return;
            }
        }
    }

    qWarning() << "somble: model" << modelString
               << "matches no profile, falling back to the EVOLV layout (read-only)";
    m_profile = &kFallbackProfile;
    emit profileResolved(QString::fromLatin1(kFallbackProfile.id),
                         QCoreApplication::translate("OmronProtocol", kFallbackProfile.label),
                         false);
}

bool OmronProtocol::clockBlockTrusted(const QByteArray &block, ClockStyle style)
{
    if (style == ClockNone)
        return false;
    const ClockShape sh = clockShape(style);
    if (block.size() < sh.csum + 1)
        return false;

    // The checksum the monitor itself left in the block. If it adds up, the
    // read landed on a real clock block and the matching write address is the
    // one this profile names. The pad byte is included in the second variant
    // because it is not certain every firmware leaves it at zero.
    int sum = 0;
    for (int i = 0; i < sh.sumEnd; ++i)
        sum += quint8(block.at(i));
    if ((sum & 0xff) == quint8(block.at(sh.csum)))
        return true;
    for (int i = sh.sumEnd; i < sh.csum; ++i)
        sum += quint8(block.at(i));
    if ((sum & 0xff) == quint8(block.at(sh.csum)))
        return true;

    // A monitor delivered with its clock never set carries the factory default
    // instead. That one specific reading is still proof the layout is right.
    const QDateTime factory = clockFromBlock(block, style);
    return factory.isValid() && factory.date() == QDate(2015, 1, 1);
}

QString OmronProtocol::clockRefusalReason() const
{
    if (m_profile == &kFallbackProfile)
        return tr("This monitor did not identify itself as a model somble knows, "
                  "so it does not know where the clock is stored. Set the date and "
                  "time on the monitor itself, or pick the model by hand on the "
                  "Device page.");
    if (!m_profile->clockWritable)
        return tr("Setting the clock is not supported on the %1: the place it is "
                  "stored has not been confirmed for this model, and writing to the "
                  "wrong address could damage it. Please set the date and time on "
                  "the monitor itself.")
            .arg(QCoreApplication::translate("OmronProtocol", m_profile->label));
    return tr("The monitor's clock did not read back the way this model stores it, "
              "so somble did not write to it. Please set the date and time on the "
              "monitor itself.");
}

void OmronProtocol::sendUnlockFrame(quint8 opcode, const QByteArray &payload)
{
    QByteArray frame;
    frame.append(char(opcode));
    frame.append(payload);
    m_lastPacket = frame;
    qWarning() << "somble: unlock write" << frame.toHex();
    m_ble->writeValue(kUnlockUuid, frame, true);
    // Key programming waits on the monitor's own pairing state; give it longer.
    m_timer->start(opcode == 0x01 ? kTimeoutMs : 5000);
}

void OmronProtocol::sendPacket(const QByteArray &packet)
{
    m_lastPacket = packet;
    qWarning() << "somble: tx packet" << packet.toHex() << "state" << m_state;
    for (int i = 0; i < 4; ++i)
        m_rx[i].clear();

    for (int off = 0, ch = 0; off < packet.size() && ch < 4; off += kChannelSize, ++ch)
        m_ble->writeValue(QString::fromLatin1(kTxUuids[ch]),
                          packet.mid(off, kChannelSize), true);
    m_timer->start(kTimeoutMs);
}

void OmronProtocol::resend()
{
    if (m_state == StPairEnterMode || m_state == StPairWriteKey || m_state == StUnlock)
        m_ble->writeValue(kUnlockUuid, m_lastPacket, true);
    else
        sendPacket(m_lastPacket);
    m_timer->start(m_state == StPairEnterMode ? 5000 : kTimeoutMs);
}

void OmronProtocol::onTimeout()
{
    if (m_state == StIdle)
        return;
    if (++m_retries > kMaxRetries) {
        if (m_state == StPairEnterMode)
            fail(tr("The monitor did not accept pairing. Hold its Bluetooth button "
                    "until the display shows a blinking “P”, then try again."));
        else
            fail(tr("The monitor stopped responding."));
        return;
    }
    resend();
}

bool OmronProtocol::startBank(int i)
{
    if (i >= m_profile->bankCount())
        return false;
    m_bank        = i;
    m_readAddress = m_profile->recordBase[i];
    m_bytesLeft   = m_profile->recordCount * m_profile->recordSize();
    m_retries     = 0;
    return true;
}

void OmronProtocol::requestNextBlock()
{
    if (m_bytesLeft <= 0) {
        // A two-user model keeps a second ring buffer somewhere else entirely;
        // read it straight into the same dump, banks back to back, which is
        // what parseRecords() expects.
        if (startBank(m_bank + 1)) {
            requestNextBlock();
            return;
        }
        m_retries = 0;
        if (m_setClock && m_timeBlock.size() >= m_profile->clockLen) {
            emit progress(tr("Setting the monitor's clock…"));
            m_state = StTimeWrite;
            sendPacket(buildWrite(m_profile->clockWriteAddr(),
                                  clockToBlock(m_timeBlock, QDateTime::currentDateTime(), m_profile->clockStyle)));
            return;
        }
        emit progress(tr("Finishing…"));
        m_state = StEnd;
        sendPacket(buildCommand(kTypeEnd, 0x0000, 0x00));
        return;
    }
    const int chunk = qMin(m_bytesLeft, m_profile->blockSize);
    m_retries = 0;
    const int total = m_profile->bankCount() * m_profile->recordCount * m_profile->recordSize();
    emit progress(tr("Reading measurements… %1%")
                      .arg(total ? (100 * m_dump.size() / total) : 0));
    sendPacket(buildCommand(kTypeRead, m_readAddress, quint8(chunk)));
}

void OmronProtocol::onBleNotified(const QString &uuid, const QByteArray &value)
{
    // The unlock channel is unframed: a status byte pair, not a striped packet.
    if (uuid.compare(kUnlockUuid, Qt::CaseInsensitive) == 0) {
        if (value.size() < 2)
            return;
        const quint8 a = quint8(value.at(0));
        const quint8 b = quint8(value.at(1));
        qWarning() << "somble: unlock reply" << value.toHex() << "state" << m_state;
        m_timer->stop();
        m_retries = 0;

        switch (m_state) {
        case StPairEnterMode:
            if (a == 0x82 && b == 0x00) {
                m_state = StPairWriteKey;
                sendUnlockFrame(0x00, m_key);
            } else if (a == 0x82) {
                // 82 0f: not in pairing mode / not ready — keep retrying until
                // the watchdog gives up, the user may still be pressing buttons.
                m_timer->start(1000);
            } else {
                fail(tr("Unexpected pairing response from the monitor."));
            }
            return;
        case StPairWriteKey:
            if (a == 0x80 && b == 0x00) {
                // Stop here. Opening and closing a transmission would prove the
                // key took, but the monitor cannot tell an empty transfer from
                // a real one: it shows the transfer, then OK, then powers off —
                // costing another button press before anything can be read.
                //
                // Deliberately leave the link up: a download started now can
                // reuse it, so no second press of the monitor's button is
                // needed. The monitor drops the link itself once it sleeps.
                m_timer->stop();
                m_state = StIdle;
                emit paired();
            } else {
                fail(tr("The monitor refused the pairing key."));
            }
            return;
        case StUnlock:
            if (a == 0x81 && b == 0x00) {
                emit progress(tr("Unlocked. Starting transfer…"));
                m_state = StStart;
                sendPacket(buildCommand(kTypeStart, 0x0000, 0x10));
            } else {
                fail(tr("The monitor rejected the key. Pair with it again "
                        "(pull down → Pair with monitor)."));
            }
            return;
        default:
            return;
        }
    }

    // Data channels: buffer, then reassemble once every required channel is in.
    const int ch = rxChannelOf(uuid);
    if (ch < 0)
        return;
    m_rx[ch] = value;

    if (m_rx[0].isEmpty())
        return;                                  // channel 0 carries the length
    const int packetSize = quint8(m_rx[0].at(0));
    if (packetSize < 8)
        return;
    const int needed = (packetSize + kChannelSize - 1) / kChannelSize;
    if (needed > 4)
        return;
    for (int i = 0; i < needed; ++i)
        if (m_rx[i].isEmpty())
            return;

    QByteArray packet;
    for (int i = 0; i < needed; ++i)
        packet.append(m_rx[i]);
    packet.truncate(packetSize);
    for (int i = 0; i < 4; ++i)
        m_rx[i].clear();

    quint8 crc = 0;
    for (int i = 0; i < packet.size(); ++i)
        crc ^= quint8(packet.at(i));
    if (crc != 0) {
        qWarning() << "harbour-somble: checksum mismatch, dropping packet";
        return;                                  // let the retry timer resend
    }

    m_timer->stop();
    qWarning() << "somble: rx packet" << packet.toHex() << "state" << m_state;
    handlePacket(packet);
}

void OmronProtocol::handlePacket(const QByteArray &packet)
{
    const quint16 type = (quint8(packet.at(1)) << 8) | quint8(packet.at(2));
    const quint16 addr = (quint8(packet.at(3)) << 8) | quint8(packet.at(4));
    const int dataLen  = quint8(packet.at(5));
    const QByteArray data = packet.mid(6, dataLen);

    switch (m_state) {
    case StStart:
        if (type != kRespStart) {
            fail(tr("The monitor did not start a transfer."));
            return;
        }
        m_retries = 0;
        if (m_clockOnly) {
            if (!m_profile->clockWritable) {
                // Nothing to do, and the transmission is already open — close
                // it politely rather than dropping the link on the monitor.
                // done() turns the closed session into the refusal message.
                m_setClock = false;
                emit progress(tr("Finishing…"));
                m_state = StEnd;
                sendPacket(buildCommand(kTypeEnd, 0x0000, 0x00));
                return;
            }
            // The clock block has to be read before it can be written: its
            // first two bytes are opaque and must go back unchanged.
            m_state = StTimeRead;
            sendPacket(buildCommand(kTypeRead, m_profile->clockReadAddr(),
                                    quint8(m_profile->clockLen)));
            return;
        }
        // Informational only; we never write the counter back.
        m_state = StSettings;
        sendPacket(buildCommand(kTypeRead, m_profile->settingsRead,
                                quint8(m_profile->settingsLen)));
        return;

    case StSettings:
        if (type == kRespRead && addr == m_profile->settingsRead)
            m_settings = data;
        m_retries = 0;
        if (!m_profile->clockReadable()) {
            // The clock offset for this model is not confirmed, so reading it
            // would only produce a plausible-looking wrong date. Report no
            // clock and get on with the records, which is what was asked for.
            emit deviceClock(QString());
            if (m_setClock)
                emit clockWriteRefused(clockRefusalReason());
            m_setClock = false;
            m_state = StRecords;
            startBank(0);
            requestNextBlock();
            return;
        }
        m_state = StTimeRead;
        sendPacket(buildCommand(kTypeRead, m_profile->clockReadAddr(),
                                quint8(m_profile->clockLen)));
        return;

    case StTimeRead: {
        if (type == kRespRead && addr == m_profile->clockReadAddr())
            m_timeBlock = data;
        const QDateTime clock = clockFromBlock(m_timeBlock, m_profile->clockStyle);
        const bool clockUnset = !clock.isValid() || clock.date().year() < 2016;
        emit deviceClock(clockUnset
                             ? QString()
                             : clock.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

        // Last line of defence before the app's only EEPROM write: the block we
        // are about to hand back has to look like the clock block it was read
        // from. If it does not, the read address was wrong for this monitor,
        // and so the write address would be too.
        const bool trusted = m_profile->clockWritable && clockBlockTrusted(m_timeBlock, m_profile->clockStyle);
        if (m_setClock && !trusted) {
            qWarning() << "somble: refusing the clock write, block"
                       << m_timeBlock.toHex() << "profile" << m_profile->id;
            m_setClock = false;
            if (!m_clockOnly)
                emit clockWriteRefused(clockRefusalReason());
        }
        // An unset clock makes every reading the monitor takes worthless, and
        // the monitor powers off moments after a transfer — far too soon for
        // the user to start a second session just to set it. So set it here,
        // in the session we already have — but only where writing is safe.
        if (clockUnset && !m_setClock && trusted) {
            qWarning() << "somble: monitor clock is unset, setting it in this session";
            m_setClock = true;
            emit progress(tr("The monitor's clock was unset — setting it."));
        }
        if (m_clockOnly) {
            if (!m_setClock) {
                emit progress(tr("Finishing…"));
                m_state = StEnd;
                m_retries = 0;
                sendPacket(buildCommand(kTypeEnd, 0x0000, 0x00));
                return;
            }
            emit progress(tr("Setting the monitor's clock…"));
            m_state = StTimeWrite;
            m_retries = 0;
            sendPacket(buildWrite(m_profile->clockWriteAddr(),
                                  clockToBlock(m_timeBlock, QDateTime::currentDateTime(), m_profile->clockStyle)));
            return;
        }
        // Even if these reads fail we can still dump the whole ring buffer,
        // which is what the archive wants anyway.
        m_state = StRecords;
        startBank(0);
        requestNextBlock();
        return;
    }

    case StTimeWrite:
        m_clockWritten = (type == kRespWrite);
        if (!m_clockWritten)
            qWarning() << "somble: clock write was not acknowledged";
        emit progress(tr("Finishing…"));
        m_state = StEnd;
        m_retries = 0;
        sendPacket(buildCommand(kTypeEnd, 0x0000, 0x00));
        return;

    case StRecords:
        if (type != kRespRead || addr != m_readAddress) {
            fail(tr("Unexpected reply while reading measurements."));
            return;
        }
        m_dump.append(data);
        m_readAddress = quint16(m_readAddress + data.size());
        m_bytesLeft -= data.size();
        requestNextBlock();
        return;

    case StEnd:
        if (type == kRespEnd && dataLen >= 1 && quint8(data.at(0)) != 0)
            qWarning() << "harbour-somble: monitor reported end status"
                       << quint8(data.at(0));
        done();
        return;

    default:
        return;
    }
}

void OmronProtocol::onBleError(const QString &message)
{
    if (m_state == StIdle)
        return;
    fail(message);
}

void OmronProtocol::onBleClosed()
{
    if (m_state == StIdle)
        return;
    fail(tr("The Bluetooth link dropped."));
}

// --- record decoding -------------------------------------------------------

namespace {

// Pull a bit range out of a record, numbered the way the protocol description
// numbers it: the whole record is one integer, bit 0 is its most significant
// bit. For a little-endian model that integer is the record read backwards,
// which is the only thing the endianess flag changes here.
quint32 bitsOf(const quint8 *rec, int size, bool bigEndian, BitField f)
{
    if (f.last < f.first)
        return 0;
    quint32 v = 0;
    for (int b = f.first; b <= f.last; ++b) {
        const int byteIdx = bigEndian ? (b / 8) : (size - 1 - b / 8);
        if (byteIdx < 0 || byteIdx >= size)
            return 0;
        v = (v << 1) | ((rec[byteIdx] >> (7 - (b % 8))) & 0x01);
    }
    return v;
}

} // namespace

QVariantList OmronProtocol::parseRecords(const QByteArray &dump, const QString &profileId)
{
    const DeviceProfile *prof = profileById(profileId);
    if (!prof)
        prof = fallbackProfile();
    const RecordLayout &l = *prof->layout;
    const int recSize   = l.size;
    const int perBank   = prof->recordCount * recSize;

    QVariantList out;
    for (int off = 0; off + recSize <= dump.size(); off += recSize) {
        const quint8 *r = reinterpret_cast<const quint8 *>(dump.constData()) + off;

        bool empty = true;
        for (int i = 0; i < recSize; ++i)
            if (r[i] != 0xff) { empty = false; break; }
        if (empty)
            continue;                            // unused ring-buffer slot

        // Which user bank this slot came from. The banks were read one after
        // the other into the same dump, so the offset says which.
        const int bank = (perBank > 0 && prof->bankCount() > 1)
                             ? qMin(off / perBank, prof->bankCount() - 1) : 0;

        const int dia    = int(bitsOf(r, recSize, l.bigEndian, l.dia));
        const int sys    = int(bitsOf(r, recSize, l.bigEndian, l.sys)) + 25;
        const int year   = int(bitsOf(r, recSize, l.bigEndian, l.year)) + 2000;
        const int pulse  = int(bitsOf(r, recSize, l.bigEndian, l.bpm));
        const bool mov   = bitsOf(r, recSize, l.bigEndian, l.mov) != 0;
        const bool ihb   = bitsOf(r, recSize, l.bigEndian, l.ihb) != 0;
        const int month  = int(bitsOf(r, recSize, l.bigEndian, l.month));
        const int day    = int(bitsOf(r, recSize, l.bigEndian, l.day));
        const int hour   = int(bitsOf(r, recSize, l.bigEndian, l.hour));
        const int minute = int(bitsOf(r, recSize, l.bigEndian, l.minute));
        const int second = qMin<int>(int(bitsOf(r, recSize, l.bigEndian, l.second)), 59);

        // Plausibility gate — a partly-filled ring buffer holds garbage, and a
        // bad record must not pollute the averages. It is also the only thing
        // standing between a wrong device profile and a chart full of noise.
        const bool bpOk = sys >= 60 && sys <= 260 && dia >= 30 && dia <= 200
                       && sys > dia && pulse >= 30 && pulse <= 220;
        const QDate date(year, month, day);
        const QTime time(hour, minute, second);
        if (!bpOk || !date.isValid() || !time.isValid()
            || year < 2000 || year > 2100)
            continue;

        const QDateTime dt(date, time);
        // A monitor whose clock has never been set stamps every reading with
        // its factory default (2015-01-01 00:00 on the unit this was developed
        // against). Those readings are real, their timestamps are not — flag
        // them so the UI can say so instead of inventing a date, and so they
        // do not all pile up on one spot of the time axis.
        const bool timeValid = year >= 2016;

        QVariantMap m;
        m.insert(QStringLiteral("user"), bank + 1);
        m.insert(QStringLiteral("systolic"), sys);
        m.insert(QStringLiteral("diastolic"), dia);
        m.insert(QStringLiteral("pulse"), pulse);
        m.insert(QStringLiteral("arrhythmia"), ihb);
        m.insert(QStringLiteral("movement"), mov);
        if (l.arrCount.last >= l.arrCount.first)
            m.insert(QStringLiteral("arrhythmiaCount"),
                     int(bitsOf(r, recSize, l.bigEndian, l.arrCount)));
        m.insert(QStringLiteral("timeValid"), timeValid);
        // On a monitor with two user banks the bank is the person, so seed the
        // P1/P2 note from it. Where there is only one it stays the user's own
        // annotation, defaulting to P1.
        if (prof->bankCount() > 1)
            m.insert(QStringLiteral("person"), bank + 1);
        m.insert(QStringLiteral("timestamp"),
                 dt.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        m.insert(QStringLiteral("epoch"), dt.toMSecsSinceEpoch() / 1000);
        out.append(m);
    }
    return out;
}
