#ifndef SOMBLE_OMRON_H
#define SOMBLE_OMRON_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QSet>
#include <QVariantList>

class OmronProtocol;

// QML facade (context property `omron`): download measurements from an Omron
// monitor over BLE, merge them into a persistent archive and export them.
//
// The monitor only keeps 100 readings in a ring buffer, so every download is
// merged (de-duplicated by timestamp) into a local archive that can grow well
// past that.
class Omron : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantList measurements READ measurements NOTIFY measurementsChanged)
    Q_PROPERTY(int count READ count NOTIFY measurementsChanged)
    Q_PROPERTY(QString rawHex READ rawHex NOTIFY rawHexChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString deviceClock READ deviceClock NOTIFY deviceClockChanged)
    Q_PROPERTY(QVariantList deviceInfo READ deviceInfo NOTIFY deviceInfoChanged)
    Q_PROPERTY(QString pairingKey READ pairingKey WRITE setPairingKey NOTIFY pairingKeyChanged)
    // Monitor model. "" means identify it from what the monitor reports;
    // anything else is a slug from knownModels and pins the EEPROM map.
    Q_PROPERTY(QString modelId READ modelId WRITE setModelId NOTIFY modelChanged)
    // What the app is currently decoding with, for display.
    Q_PROPERTY(QString modelLabel READ modelLabel NOTIFY modelChanged)
    // Whether the clock can be set on the model in force. False on an
    // unidentified monitor and on models whose clock offset is unconfirmed.
    Q_PROPERTY(bool clockWritable READ clockWritable NOTIFY modelChanged)
    // Whether a model is known at all — pinned, or identified by some earlier
    // session. False only before the monitor has ever been read, which is a
    // different thing from "this model cannot do it".
    Q_PROPERTY(bool modelIdentified READ modelIdentified NOTIFY modelChanged)
    // [{id, label}] for the model picker; the "identify it" entry is not in it.
    Q_PROPERTY(QVariantList knownModels READ knownModels CONSTANT)
    // Which person the chart is currently showing (1 or 2). Persisted.
    Q_PROPERTY(int chartPerson READ chartPerson WRITE setChartPerson NOTIFY chartPersonChanged)
    // Bumped on every P1/P2 assignment. Reassigning does not change the list
    // itself, and re-emitting measurementsChanged for it would rebuild the
    // whole model — which scrolls the list back to the top under the user's
    // finger. Views that care about the split watch this instead.
    Q_PROPERTY(int personRevision READ personRevision NOTIFY personRevisionChanged)

public:
    explicit Omron(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString statusText() const { return m_status; }
    QVariantList measurements() const { return m_measurements; }
    int count() const { return m_measurements.size(); }
    QString rawHex() const { return m_rawHex; }
    QString version() const { return QStringLiteral(APP_VERSION); }
    // Empty until a session has read it, or when the monitor's clock is unset.
    QString deviceClock() const { return m_deviceClock; }
    // Label/value rows, populated on the first session of the run.
    QVariantList deviceInfo() const { return m_deviceInfo; }
    int chartPerson() const { return m_chartPerson; }
    void setChartPerson(int person);
    int personRevision() const { return m_personRevision; }
    QString pairingKey() const;                 // 32 hex chars
    void setPairingKey(const QString &hex);
    QString modelId() const { return m_modelId; }
    void setModelId(const QString &id);
    QString modelLabel() const;
    bool clockWritable() const;
    bool modelIdentified() const;
    QVariantList knownModels() const;

    // Read the stored measurements. The monitor must already know our key.
    Q_INVOKABLE void download();
    // Program our key into the monitor; it must be in pairing mode. On success
    // a download follows automatically.
    Q_INVOKABLE void pair();
    // Download, and write the phone's date and time into the monitor on the
    // way out. The monitor stamps every reading from its own clock, and an
    // unset clock gives every reading the same meaningless timestamp.
    Q_INVOKABLE void downloadAndSetClock();
    // Write the clock and nothing else.
    Q_INVOKABLE void setDeviceClock();
    Q_INVOKABLE void cancel();

    // Assign a reading to person 1 or 2. The monitor has a single memory and
    // stores no user information, so this is the user's own annotation — kept
    // in the archive so it survives further downloads.
    Q_INVOKABLE void assignPerson(int index, int person);
    Q_INVOKABLE int personCount(int person) const;

    // Drop a reading and remember that it was dropped, so the next download —
    // which always re-reads the monitor's whole ring buffer — does not bring
    // it back. The monitor's own memory is left untouched.
    Q_INVOKABLE void deleteRecord(int index);
    // Delete every reading currently held, and remember all of them as
    // deleted so the next download does not bring them back.
    Q_INVOKABLE int deleteAll();
    Q_INVOKABLE void forgetDeletions();
    Q_PROPERTY(int deletedCount READ deletedCount NOTIFY deletedCountChanged)
    int deletedCount() const { return m_suppressed.size(); }

    // Re-decode the last raw capture (after changing nothing but the parser —
    // useful when checking the record layout against a different model).
    Q_INVOKABLE void reparse();

    Q_INVOKABLE QString exportCsv();
    Q_INVOKABLE QString saveToFile();
    Q_INVOKABLE void loadFromFile(const QString &path, bool merge);
    Q_INVOKABLE void clearArchive();
    Q_INVOKABLE QString reserveImagePath();
    Q_INVOKABLE void notify(const QString &msg) { emit actionInfo(msg); }

signals:
    void busyChanged();
    void statusTextChanged();
    void measurementsChanged();
    void rawHexChanged();
    void deviceClockChanged();
    void deviceInfoChanged();
    void pairingKeyChanged();
    void modelChanged();
    void chartPersonChanged();
    void personRevisionChanged();
    void deletedCountChanged();
    void actionError(const QString &message);
    void actionInfo(const QString &message);

private slots:
    void onProgress(const QString &msg);
    void onFinished(const QVariantList &records, const QByteArray &dump);
    void onFailed(const QString &err);

private:
    void setStatus(const QString &s);
    void setBusy(bool b);
    int mergeRecords(const QVariantList &recs);   // returns #added; sorts + emits
    void rebuildRawHex();
    void saveData() const;
    void loadData();
    void saveSettings() const;
    void loadSettings();
    QString dataFile() const;
    QString settingsFile() const;
    static QString keyFor(const QVariantMap &m);

    OmronProtocol *m_proto = nullptr;
    bool m_busy = false;
    QString m_status;
    QString m_rawHex;
    QString m_deviceClock;
    QVariantList m_deviceInfo;
    QString m_modelId;              // "" -> identify from the monitor
    QString m_detectedId;           // what the last session settled on
    int m_chartPerson = 1;
    int m_personRevision = 0;
    QSet<QString> m_suppressed;   // keys of readings deliberately deleted
    QByteArray m_dump;
    QVariantList m_measurements;
};

#endif // SOMBLE_OMRON_H
