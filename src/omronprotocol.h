#ifndef SOMBLE_OMRONPROTOCOL_H
#define SOMBLE_OMRONPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QVariantList>

class BleTransport;
class QTimer;

// The Omron "legacy" vendor GATT protocol, as used by the EVOLV (HEM-7600T)
// and its relatives. The wire format below is common to all of them; what
// differs per model is the EEPROM map — where the records live, how many user
// banks there are, and how the fields are packed into a record. That part is a
// DeviceProfile, picked from the monitor's Model Number String or set by hand.
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
//
// That write is address-critical: the clock block sits at a different offset on
// every model, and the same address means something else on the next one. So it
// happens only when the model is positively known AND the block read back from
// it verifies (see clockBlockTrusted). On an unrecognised monitor the app reads
// and never writes.

// How the monitor's clock block is laid out. The block is read from one
// address and written to another, and its internals differ by model family.
enum ClockStyle {
    ClockNone,      // this model's clock offset is not known — never touch it
    ClockBE10,      // 10 bytes: 2 opaque, month/year/hour/day/second/minute, pad, sum
    ClockLE16       // 16 bytes: 8 opaque, year/month/day/hour/minute/second, sum, pad
};

// A bit range inside a record, numbered the way the protocol description does:
// the record is one big integer (most models big-endian, some little-endian),
// bit 0 is its most significant bit. last < first marks a field this model does
// not have.
struct BitField { short first; short last; };

// Where each value sits inside one record.
struct RecordLayout {
    bool bigEndian;
    int  size;                  // bytes per record
    BitField dia, sys, year, bpm, mov, ihb;
    BitField month, day, hour, minute, second;
    BitField arrCount;          // arrhythmia counter, where it is known
};

// The EEPROM map of one monitor model. Everything the protocol needs to know
// beyond the wire format itself.
struct DeviceProfile {
    const char *id;             // stable slug, stored in the settings file
    const char *label;          // shown in the UI
    const char *matches;        // '|'-separated substrings, matched case-insensitively
                                // against the Model Number String and the BLE name
    quint16 recordBase[2];      // per-user ring buffers; [1] == 0 -> a single bank
    int recordCount;            // slots per bank
    int blockSize;              // EEPROM bytes per read request
    quint16 settingsRead;       // the settings block, through the read window...
    quint16 settingsWrite;      // ...and through the (different) write window
    int settingsLen;
    quint16 clockOffset;        // of the clock block within the settings block
    int clockLen;
    ClockStyle clockStyle;
    const RecordLayout *layout;
    bool clockWritable;         // a community driver writes this model's clock,
                                // so the offset is more than a guess

    int recordSize() const { return layout->size; }
    int bankCount() const { return recordBase[1] ? 2 : 1; }
    bool clockReadable() const { return clockStyle != ClockNone; }
    quint16 clockReadAddr() const { return quint16(settingsRead + clockOffset); }
    quint16 clockWriteAddr() const { return quint16(settingsWrite + clockOffset); }
};

class OmronProtocol : public QObject
{
    Q_OBJECT
public:
    explicit OmronProtocol(QObject *parent = nullptr);

    // The models this build knows how to decode, in menu order.
    static QList<const DeviceProfile *> knownProfiles();
    // By slug; null for an unknown one. "" / "auto" is not a profile.
    static const DeviceProfile *profileById(const QString &id);
    // The read geometry used when the model could not be identified: the EVOLV
    // map, which is the only one this app was developed against — but with the
    // clock write disabled, because a wrong write address is the one mistake
    // that cannot be taken back.
    static const DeviceProfile *fallbackProfile();

    // "" or "auto": identify the monitor from its Model Number String. Anything
    // else pins the profile and skips detection.
    void setModelOverride(const QString &id) { m_override = id; }
    QString modelOverride() const { return m_override; }
    // The profile in force. Valid before a session too — it is the override, or
    // whatever the last session detected, or the fallback.
    const DeviceProfile *profile() const { return m_profile; }

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

    // Decode records out of a raw EEPROM dump, which for a two-bank model is
    // the banks back to back in order. Static so the raw page can re-run it
    // without a live session; profileId picks the record layout.
    static QVariantList parseRecords(const QByteArray &dump, const QString &profileId = QString());

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
    // The profile this session settled on, once the monitor has identified
    // itself. detected is false when nothing matched and the fallback is in use.
    void profileResolved(const QString &id, const QString &label, bool detected);
    // Why the clock was not written, when the user asked for it and it was not.
    void clockWriteRefused(const QString &reason);

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
    // Match the monitor's Model Number String against the profile table.
    void resolveProfile(const QString &modelString);
    // Why the clock is not being written on this monitor, in one sentence.
    QString clockRefusalReason() const;
    // A clock block is only written back if it reads like one: the additive
    // checksum the monitor itself put in the last byte has to check out, or the
    // block has to decode to the documented factory default of a never-set
    // clock. Anything else means the read landed somewhere that is not a clock.
    static bool clockBlockTrusted(const QByteArray &block, ClockStyle style);
    // Start reading bank index i of the record area; false when there is none.
    bool startBank(int i);

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
    QString m_override;                             // "" / "auto" -> detect
    const DeviceProfile *m_profile = nullptr;       // never null after the ctor
    int m_bank = 0;                                 // record bank being read
    bool m_pairFirst = false;
    bool m_setClock = false;
    bool m_clockOnly = false;
    bool m_clockWritten = false;    // the write went out and was acknowledged
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
