#ifndef SOMBLE_OMRONPROTOCOL_H
#define SOMBLE_OMRONPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QVariantList>

class BleTransport;
class QTimer;

// The Omron "legacy" vendor GATT protocol, as used by the EVOLV (HEM-7600T).
//
// Wire format (both directions), built by buildCommand() / buildWrite():
//
//   [0]      total packet length (header 6 + data + 2 trailer)
//   [1..2]   packet type, big endian
//   [3..4]   EEPROM address, big endian (always, regardless of record endianess)
//   [5]      data length
//   [6..]    data
//   [n-2]    0x00 padding
//   [n-1]    XOR checksum, chosen so XOR over the whole packet is 0
//
// A packet is striped over four 16-byte GATT channels: bytes [0:16] go to TX
// channel 0, [16:32] to channel 1 and so on, and responses are reassembled from
// the matching RX channels. That 16-byte limit is the protocol's own framing,
// not the ATT MTU, so a larger MTU does not remove it.
//
// A session is: unlock with a 16-byte key -> start transmission -> read EEPROM
// in blocks -> end transmission. The key must be programmed into the monitor
// once while it is in pairing mode.
//
// The only EEPROM write this implementation performs is the clock block, and
// only when explicitly asked (setClock). In particular it never resets the
// unread-record counter: every download reads the full ring buffer and merges
// the result into a local archive, so the counter is not needed, and the
// settings region is believed to also hold pressure-sensor calibration data.
class OmronProtocol : public QObject
{
    Q_OBJECT
public:
    explicit OmronProtocol(QObject *parent = nullptr);

    // 16 bytes. Programmed into the monitor once, then presented on every
    // session. Configurable so a key programmed by another app can be reused.
    static QByteArray defaultKey();
    void setUnlockKey(const QByteArray &key) { m_key = key; }
    QByteArray unlockKey() const { return m_key; }

    // pairFirst: the monitor is in pairing mode and the key is to be programmed
    // into it before the session proper.
    // setClock: also write the phone's date and time into the monitor. Opt-in,
    // because it is the app's only write to the monitor's EEPROM.
    // clockOnly: write the clock and nothing else — no records are read. The
    // monitor still has to open and close a transmission for the write, so it
    // will show its transfer symbol, but the session is over in a second.
    void start(bool pairFirst, bool setClock = false, bool clockOnly = false);
    void cancel();
    bool busy() const { return m_state != StIdle; }

    // Decode 14-byte records out of a raw EEPROM dump. Static so the raw page
    // can re-run it without a live session.
    static QVariantList parseRecords(const QByteArray &dump);

signals:
    void progress(const QString &message);
    // records: newest-last list of QVariantMaps; dump: the raw EEPROM bytes.
    void finished(const QVariantList &records, const QByteArray &dump);
    void failed(const QString &message);
    // The monitor's own clock, as read at the start of the session. Empty if
    // the block held an implausible date (an unset clock).
    void deviceClock(const QString &humanReadable);
    // The key was stored successfully. Pairing does not read any records —
    // that is what a download is for.
    void paired();
    // The clock was written; no measurements were read.
    void clockSet();
    // Label/value rows from the standard Device Information and Battery
    // services, for display only.
    void deviceInfo(const QVariantList &rows);

private slots:
    void onBleReady();
    void onBleNotified(const QString &uuid, const QByteArray &value);
    void onBleError(const QString &message);
    void onBleClosed();
    void onTimeout();

private:
    enum State {
        StIdle,
        StPairEnterMode,    // wrote 02 + 16x00, waiting for 82 00
        StPairWriteKey,     // wrote 00 + key,   waiting for 80 00
        StUnlock,           // wrote 01 + key,   waiting for 81 00
        StStart,            // start of transmission, waiting for type 8000
        StSettings,         // read of the ring-buffer settings block
        StTimeRead,         // read of the clock block (always, to report it)
        StRecords,          // reading the record area block by block
        StTimeWrite,        // write of the clock block (only when asked)
        StEnd               // end of transmission, waiting for type 8f00
    };

    void fail(const QString &message);
    void done();
    void readDeviceInfo();

    void sendUnlockFrame(quint8 opcode, const QByteArray &payload);
    void sendPacket(const QByteArray &packet);   // stripe over TX channels
    void resend();
    void requestNextBlock();                     // next EEPROM read of the plan
    void handlePacket(const QByteArray &packet); // a reassembled RX packet

    // Read commands are all the same 8-byte shape: no data bytes, with the
    // block length carried in the data-length field.
    static QByteArray buildCommand(quint16 type, quint16 address, quint8 length);
    // A write carries its payload; the packet grows past 16 bytes and is
    // striped over two TX channels.
    static QByteArray buildWrite(quint16 address, const QByteArray &data);

    BleTransport *m_ble = nullptr;
    QTimer *m_timer = nullptr;

    QByteArray m_key;
    bool m_pairFirst = false;
    bool m_setClock = false;
    bool m_clockOnly = false;
    State m_state = StIdle;

    QByteArray m_lastPacket;        // for the retry
    int m_retries = 0;

    QList<QByteArray> m_rx;         // per-channel reassembly buffers

    quint16 m_readAddress = 0;      // next EEPROM address to fetch
    int m_bytesLeft = 0;            // of the record area
    QByteArray m_dump;              // accumulated record bytes
    QByteArray m_settings;          // ring-buffer settings block
    QByteArray m_timeBlock;         // clock block, as read from the monitor
};

#endif // SOMBLE_OMRONPROTOCOL_H
