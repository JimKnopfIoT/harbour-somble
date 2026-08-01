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

// --- device profile: EVOLV / HEM-7600T -------------------------------------
const quint16 kRecordBase   = 0x02ac;   // start of the single user's ring buffer
const int     kRecordCount  = 100;      // slots in that ring buffer
const int     kRecordSize   = 14;       // bytes per record
const quint16 kSettingsAddr = 0x0260;   // ring-buffer bookkeeping
const int     kSettingsLen  = 8;
// The clock lives 0x14 into the settings area, and — as everywhere in this
// protocol — is read and written through two different windows.
const quint16 kClockReadAddr  = 0x0274;
const quint16 kClockWriteAddr = 0x029a;
const int     kClockLen       = 10;
// 0x38 = 56 data bytes -> a 64-byte response, exactly four 16-byte channels.
const int     kBlockSize    = 0x38;

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

OmronProtocol::OmronProtocol(QObject *parent)
    : QObject(parent)
    , m_ble(new BleTransport(this))
    , m_timer(new QTimer(this))
    , m_key(defaultKey())
{
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
// Ten bytes, of which the first two are opaque and must be written back
// unchanged. The field order is not the obvious one:
//
//   [0..1] opaque   [2] month   [3] year-2000   [4] hour
//   [5] day         [6] second  [7] minute      [8] 0x00
//   [9] sum of bytes 0..7, low byte (an additive checksum, not the XOR the
//       packet layer uses)

static QDateTime clockFromBlock(const QByteArray &b)
{
    if (b.size() < 8)
        return QDateTime();
    const QDate date(2000 + quint8(b.at(3)), quint8(b.at(2)), quint8(b.at(5)));
    const QTime time(quint8(b.at(4)), quint8(b.at(7)), qMin<int>(quint8(b.at(6)), 59));
    if (!date.isValid() || !time.isValid())
        return QDateTime();
    return QDateTime(date, time);
}

static QByteArray clockToBlock(const QByteArray &previous, const QDateTime &now)
{
    QByteArray b = previous.left(2);
    if (b.size() < 2)
        b = QByteArray(2, '\0');
    b.append(char(now.date().month()));
    b.append(char(now.date().year() - 2000));
    b.append(char(now.time().hour()));
    b.append(char(now.date().day()));
    b.append(char(now.time().second()));
    b.append(char(now.time().minute()));
    b.append(char(0x00));

    int sum = 0;
    for (int i = 0; i < b.size(); ++i)
        sum += quint8(b.at(i));
    b.append(char(sum & 0xff));
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
    m_retries = 0;
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
    emit finished(parseRecords(m_dump), m_dump);
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
    for (const InfoField &f : kInfoFields) {
        bool ok = false;
        const QByteArray v = m_ble->readValue(QString::fromLatin1(f.uuid), &ok);
        if (!ok || v.isEmpty())
            continue;
        QVariantMap row;
        row.insert(QStringLiteral("label"),
                   QCoreApplication::translate("OmronProtocol", f.label));
        row.insert(QStringLiteral("value"),
                   f.numeric ? QString::number(quint8(v.at(0))) + QStringLiteral(" %")
                             : QString::fromLatin1(v).trimmed());
        rows.append(row);
    }

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

void OmronProtocol::requestNextBlock()
{
    if (m_bytesLeft <= 0) {
        m_retries = 0;
        if (m_setClock && m_timeBlock.size() >= kClockLen) {
            emit progress(tr("Setting the monitor's clock…"));
            m_state = StTimeWrite;
            sendPacket(buildWrite(kClockWriteAddr,
                                  clockToBlock(m_timeBlock, QDateTime::currentDateTime())));
            return;
        }
        emit progress(tr("Finishing…"));
        m_state = StEnd;
        sendPacket(buildCommand(kTypeEnd, 0x0000, 0x00));
        return;
    }
    const int chunk = qMin(m_bytesLeft, kBlockSize);
    m_retries = 0;
    const int total = kRecordCount * kRecordSize;
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
            // The clock block has to be read before it can be written: its
            // first two bytes are opaque and must go back unchanged.
            m_state = StTimeRead;
            sendPacket(buildCommand(kTypeRead, kClockReadAddr, kClockLen));
            return;
        }
        // Informational only; we never write the counter back.
        m_state = StSettings;
        sendPacket(buildCommand(kTypeRead, kSettingsAddr, kSettingsLen));
        return;

    case StSettings:
        if (type == kRespRead && addr == kSettingsAddr)
            m_settings = data;
        m_state = StTimeRead;
        m_retries = 0;
        sendPacket(buildCommand(kTypeRead, kClockReadAddr, kClockLen));
        return;

    case StTimeRead: {
        if (type == kRespRead && addr == kClockReadAddr)
            m_timeBlock = data;
        const QDateTime clock = clockFromBlock(m_timeBlock);
        const bool clockUnset = !clock.isValid() || clock.date().year() < 2016;
        emit deviceClock(clockUnset
                             ? QString()
                             : clock.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        // An unset clock makes every reading the monitor takes worthless, and
        // the monitor powers off moments after a transfer — far too soon for
        // the user to start a second session just to set it. So set it here,
        // in the session we already have.
        if (clockUnset && !m_setClock) {
            qWarning() << "somble: monitor clock is unset, setting it in this session";
            m_setClock = true;
            emit progress(tr("The monitor's clock was unset — setting it."));
        }
        if (m_clockOnly) {
            emit progress(tr("Setting the monitor's clock…"));
            m_state = StTimeWrite;
            m_retries = 0;
            sendPacket(buildWrite(kClockWriteAddr,
                                  clockToBlock(m_timeBlock, QDateTime::currentDateTime())));
            return;
        }
        // Even if these reads fail we can still dump the whole ring buffer,
        // which is what the archive wants anyway.
        m_readAddress = kRecordBase;
        m_bytesLeft   = kRecordCount * kRecordSize;
        m_state       = StRecords;
        requestNextBlock();
        return;
    }

    case StTimeWrite:
        if (type != kRespWrite)
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

// Records are 14 bytes; the fields are packed MSB-first across the record, so
// the bit offsets from the protocol description translate to these masks:
//
//   bits  0.. 7  diastolic          bits 32     movement error
//   bits  8..15  systolic - 25      bit  33     irregular heartbeat
//   bits 16..23  year - 2000        bits 34..37 month
//   bits 24..31  pulse              bits 38..42 day
//                                   bits 43..47 hour
//   bits 52..57  minute             bits 58..63 second (can read up to 63)
//   byte 8       arrhythmia counter (the vendor app flags it from ~11 up)
QVariantList OmronProtocol::parseRecords(const QByteArray &dump)
{
    QVariantList out;
    for (int off = 0; off + kRecordSize <= dump.size(); off += kRecordSize) {
        const quint8 *r = reinterpret_cast<const quint8 *>(dump.constData()) + off;

        bool empty = true;
        for (int i = 0; i < kRecordSize; ++i)
            if (r[i] != 0xff) { empty = false; break; }
        if (empty)
            continue;                            // unused ring-buffer slot

        const int dia    =  r[0];
        const int sys    =  r[1] + 25;
        const int year   =  r[2] + 2000;
        const int pulse  =  r[3];
        const bool mov   = (r[4] >> 7) & 0x01;
        const bool ihb   = (r[4] >> 6) & 0x01;
        const int month  = (r[4] >> 2) & 0x0f;
        const int day    = ((r[4] & 0x03) << 3) | (r[5] >> 5);
        const int hour   =  r[5] & 0x1f;
        const int minute = ((r[6] & 0x0f) << 2) | (r[7] >> 6);
        const int second = qMin<int>(r[7] & 0x3f, 59);
        const int arrCnt =  r[8];

        // Plausibility gate — a partly-filled ring buffer holds garbage, and a
        // bad record must not pollute the averages.
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
        m.insert(QStringLiteral("user"), 1);          // the EVOLV has one user
        m.insert(QStringLiteral("systolic"), sys);
        m.insert(QStringLiteral("diastolic"), dia);
        m.insert(QStringLiteral("pulse"), pulse);
        m.insert(QStringLiteral("arrhythmia"), ihb);
        m.insert(QStringLiteral("movement"), mov);
        m.insert(QStringLiteral("arrhythmiaCount"), arrCnt);
        m.insert(QStringLiteral("timeValid"), timeValid);
        m.insert(QStringLiteral("timestamp"),
                 dt.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        m.insert(QStringLiteral("epoch"), dt.toMSecsSinceEpoch() / 1000);
        out.append(m);
    }
    return out;
}
