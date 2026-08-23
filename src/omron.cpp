#include "omron.h"
#include "omronprotocol.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>
#include <algorithm>

Omron::Omron(QObject *parent)
    : QObject(parent)
    , m_proto(new OmronProtocol(this))
{
    connect(m_proto, &OmronProtocol::progress, this, &Omron::onProgress);
    connect(m_proto, &OmronProtocol::finished, this, &Omron::onFinished);
    connect(m_proto, &OmronProtocol::failed, this, &Omron::onFailed);
    connect(m_proto, &OmronProtocol::deviceClock, this, [this](const QString &c) {
        if (m_deviceClock == c) return;
        m_deviceClock = c;
        emit deviceClockChanged();
    });
    connect(m_proto, &OmronProtocol::paired, this, [this]() {
        setBusy(false);
        setStatus(tr("Paired. Now pull down and download."));
        emit actionInfo(tr("Paired with the monitor. Pull down → Download from device."));
    });
    connect(m_proto, &OmronProtocol::clockSet, this, [this]() {
        setBusy(false);
        setStatus(tr("Clock set."));
        emit actionInfo(tr("The monitor's clock is now set."));
    });
    connect(m_proto, &OmronProtocol::profileResolved, this,
            [this](const QString &id, const QString &label, bool detected) {
        if (m_detectedId != id) {
            m_detectedId = id;
            // Remember it: the model does not change between runs, and without
            // this every restart forgets the monitor and hides everything that
            // depends on knowing it until the next download.
            saveSettings();
        }
        emit modelChanged();
        // Only worth saying out loud when the app had to guess and could not.
        if (!detected)
            emit actionInfo(tr("This monitor did not identify itself as a model "
                               "somble knows (%1). The readings below may be "
                               "nonsense — pick the model on the Device page.")
                                .arg(label));
    });
    connect(m_proto, &OmronProtocol::clockWriteRefused, this, [this](const QString &why) {
        emit actionError(why);
    });
    connect(m_proto, &OmronProtocol::deviceInfo, this, [this](const QVariantList &rows) {
        m_deviceInfo = rows;
        emit deviceInfoChanged();
    });

    m_status = tr("Not connected");
    loadSettings();
    loadData();
}

// --- small helpers ---------------------------------------------------------

void Omron::setStatus(const QString &s)
{
    if (m_status != s) { m_status = s; emit statusTextChanged(); }
}

void Omron::setBusy(bool b)
{
    if (m_busy != b) { m_busy = b; emit busyChanged(); }
}

QString Omron::pairingKey() const
{
    return QString::fromLatin1(m_proto->unlockKey().toHex());
}

void Omron::setPairingKey(const QString &hex)
{
    const QByteArray key = QByteArray::fromHex(hex.trimmed().toLatin1());
    if (key.size() != 16) {
        emit actionError(tr("The pairing key must be 32 hex characters (16 bytes)."));
        return;
    }
    if (key == m_proto->unlockKey())
        return;
    m_proto->setUnlockKey(key);
    saveSettings();
    emit pairingKeyChanged();
}

// --- monitor model ---------------------------------------------------------

void Omron::setModelId(const QString &id)
{
    // "" and "auto" both mean: work it out from the monitor.
    const QString wanted = (id == QLatin1String("auto")) ? QString() : id;
    if (!wanted.isEmpty() && !OmronProtocol::profileById(wanted)) {
        emit actionError(tr("Unknown monitor model: %1").arg(id));
        return;
    }
    if (m_modelId == wanted)
        return;
    m_modelId = wanted;
    m_proto->setModelOverride(m_modelId);
    // A pinned model decides the record layout, so whatever was decoded with
    // the old one is not to be trusted. The raw capture survives — reparse()
    // on the raw page re-runs it through the new layout.
    m_detectedId = m_modelId;
    saveSettings();
    emit modelChanged();
}

QString Omron::modelLabel() const
{
    const QString id = m_modelId.isEmpty() ? m_detectedId : m_modelId;
    if (const DeviceProfile *p = OmronProtocol::profileById(id))
        return QCoreApplication::translate("OmronProtocol", p->label);
    if (!m_detectedId.isEmpty())        // a session ran and matched nothing
        return tr("unrecognised model");
    return tr("not identified yet");
}

bool Omron::clockWritable() const
{
    const QString id = m_modelId.isEmpty() ? m_detectedId : m_modelId;
    const DeviceProfile *p = OmronProtocol::profileById(id);
    return p && p->clockWritable;
}

bool Omron::modelIdentified() const
{
    const QString id = m_modelId.isEmpty() ? m_detectedId : m_modelId;
    return OmronProtocol::profileById(id) != nullptr
        || !m_detectedId.isEmpty();             // a session ran and matched nothing
}

QVariantList Omron::knownModels() const
{
    QVariantList out;
    const QList<const DeviceProfile *> profiles = OmronProtocol::knownProfiles();
    for (const DeviceProfile *p : profiles) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), QString::fromLatin1(p->id));
        m.insert(QStringLiteral("label"),
                 QCoreApplication::translate("OmronProtocol", p->label));
        m.insert(QStringLiteral("clockWritable"), p->clockWritable);
        out.append(m);
    }
    return out;
}

void Omron::setChartPerson(int person)
{
    const int p = (person == 2) ? 2 : 1;
    if (m_chartPerson == p)
        return;
    m_chartPerson = p;
    saveSettings();
    emit chartPersonChanged();
}

void Omron::assignPerson(int index, int person)
{
    if (index < 0 || index >= m_measurements.size())
        return;
    QVariantMap m = m_measurements.at(index).toMap();
    const int p = (person == 2) ? 2 : 1;
    if (m.value(QStringLiteral("person"), 1).toInt() == p)
        return;
    m.insert(QStringLiteral("person"), p);
    m_measurements[index] = m;
    saveData();
    // Deliberately not measurementsChanged: see personRevision in the header.
    ++m_personRevision;
    emit personRevisionChanged();
}

void Omron::deleteRecord(int index)
{
    if (index < 0 || index >= m_measurements.size())
        return;
    m_suppressed.insert(keyFor(m_measurements.at(index).toMap()));
    m_measurements.removeAt(index);
    saveData();
    saveSettings();
    emit measurementsChanged();
    emit deletedCountChanged();
}

int Omron::deleteAll()
{
    const int n = m_measurements.size();
    if (!n)
        return 0;
    for (const QVariant &v : m_measurements)
        m_suppressed.insert(keyFor(v.toMap()));
    m_measurements.clear();
    saveData();
    saveSettings();
    emit measurementsChanged();
    emit deletedCountChanged();
    emit actionInfo(tr("Deleted %1 reading(s). They will not come back on the "
                       "next download.").arg(n));
    return n;
}

void Omron::forgetDeletions()
{
    if (m_suppressed.isEmpty())
        return;
    m_suppressed.clear();
    saveSettings();
    emit deletedCountChanged();
    emit actionInfo(tr("Deleted readings will reappear on the next download."));
}

int Omron::personCount(int person) const
{
    int n = 0;
    for (const QVariant &v : m_measurements)
        if (v.toMap().value(QStringLiteral("person"), 1).toInt() == person)
            ++n;
    return n;
}

// --- session ---------------------------------------------------------------

void Omron::download()
{
    if (m_busy)
        return;
    setBusy(true);
    setStatus(tr("Connecting…"));
    m_proto->start(false, false);
}

void Omron::downloadAndSetClock()
{
    if (m_busy)
        return;
    setBusy(true);
    setStatus(tr("Connecting…"));
    m_proto->start(false, true);
}

void Omron::setDeviceClock()
{
    if (m_busy)
        return;
    setBusy(true);
    setStatus(tr("Setting the clock…"));
    m_proto->start(false, true, true);
}

void Omron::pair()
{
    if (m_busy)
        return;
    setBusy(true);
    setStatus(tr("Pairing…"));
    m_proto->start(true, false);
}

void Omron::cancel()
{
    if (!m_busy)
        return;
    m_proto->cancel();
    setBusy(false);
    setStatus(tr("Cancelled"));
}

void Omron::onProgress(const QString &msg)
{
    setStatus(msg);
}

void Omron::onFinished(const QVariantList &records, const QByteArray &dump)
{
    setBusy(false);

    m_dump = dump;
    rebuildRawHex();

    const int added = mergeRecords(records);
    saveData();
    setStatus(tr("Downloaded: %1 new, %2 total").arg(added).arg(m_measurements.size()));
    if (added == 0 && !records.isEmpty())
        emit actionInfo(tr("No new readings — the archive is already up to date."));
}

void Omron::onFailed(const QString &err)
{
    setBusy(false);
    setStatus(tr("Failed: %1").arg(err));
    emit actionError(err);
}

void Omron::reparse()
{
    if (m_dump.isEmpty()) {
        emit actionError(tr("Nothing captured yet."));
        return;
    }
    const QString id = m_modelId.isEmpty() ? m_detectedId : m_modelId;
    const int added = mergeRecords(OmronProtocol::parseRecords(m_dump, id));
    saveData();
    emit actionInfo(tr("Re-parsed: +%1 (%2 total)").arg(added).arg(m_measurements.size()));
}

void Omron::rebuildRawHex()
{
    QString hex;
    if (!m_dump.isEmpty()) {
        const QString id = m_modelId.isEmpty() ? m_detectedId : m_modelId;
        const DeviceProfile *p = OmronProtocol::profileById(id);
        const int recSize = p ? p->recordSize() : OmronProtocol::fallbackProfile()->recordSize();
        hex += tr("--- EEPROM dump (%1 bytes, %2-byte records) ---\n")
                   .arg(m_dump.size()).arg(recSize);
        for (int i = 0; i < m_dump.size(); ++i) {
            if (i && i % recSize == 0) hex += QLatin1Char('\n');
            else if (i) hex += QLatin1Char(' ');
            hex += QString::asprintf("%02X", quint8(m_dump.at(i)));
        }
        hex += QLatin1Char('\n');
    }
    m_rawHex = hex;
    emit rawHexChanged();
}

// --- archive ---------------------------------------------------------------

// De-duplication key. The reading values are part of it on purpose: a monitor
// whose clock was never set stamps every record with the same timestamp, and a
// timestamp-only key would collapse a whole history into a single entry.
QString Omron::keyFor(const QVariantMap &m)
{
    return m.value(QStringLiteral("user")).toString() + QLatin1Char(':')
         + QString::number(m.value(QStringLiteral("epoch")).toLongLong()) + QLatin1Char(':')
         + QString::number(m.value(QStringLiteral("systolic")).toInt()) + QLatin1Char('/')
         + QString::number(m.value(QStringLiteral("diastolic")).toInt()) + QLatin1Char('/')
         + QString::number(m.value(QStringLiteral("pulse")).toInt());
}

int Omron::mergeRecords(const QVariantList &recs)
{
    QSet<QString> keys;
    for (const QVariant &v : m_measurements)
        keys.insert(keyFor(v.toMap()));
    int added = 0;
    for (const QVariant &v : recs) {
        QVariantMap m = v.toMap();
        const QString k = keyFor(m);
        if (m_suppressed.contains(k)) continue;
        if (keys.contains(k)) continue;      // keeps the existing record, and
        keys.insert(k);                      // with it any person assignment
        if (!m.contains(QStringLiteral("person")))
            m.insert(QStringLiteral("person"), 1);
        m_measurements.append(m);
        ++added;
    }
    std::sort(m_measurements.begin(), m_measurements.end(),
              [](const QVariant &a, const QVariant &b) {
                  return a.toMap().value(QStringLiteral("epoch")).toLongLong()
                       > b.toMap().value(QStringLiteral("epoch")).toLongLong();
              });
    emit measurementsChanged();
    return added;
}

static QJsonObject archiveToJson(const QVariantList &measurements)
{
    QJsonArray arr;
    for (const QVariant &v : measurements)
        arr.append(QJsonObject::fromVariantMap(v.toMap()));
    QJsonObject obj;
    obj.insert(QStringLiteral("app"), QStringLiteral("harbour-somble"));
    obj.insert(QStringLiteral("measurements"), arr);
    obj.insert(QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return obj;
}

QString Omron::dataFile() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    return dir + QStringLiteral("/archive.json");
}

QString Omron::settingsFile() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    return dir + QStringLiteral("/settings.json");
}

void Omron::saveData() const
{
    QDir().mkpath(QFileInfo(dataFile()).absolutePath());
    QFile f(dataFile());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(archiveToJson(m_measurements)).toJson(QJsonDocument::Compact));
}

void Omron::loadData()
{
    QFile f(dataFile());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll())
                               .object().value(QStringLiteral("measurements")).toArray();
    m_measurements.clear();
    for (const QJsonValue &v : arr)
        m_measurements.append(v.toObject().toVariantMap());
    std::sort(m_measurements.begin(), m_measurements.end(),
              [](const QVariant &a, const QVariant &b) {
                  return a.toMap().value(QStringLiteral("epoch")).toLongLong()
                       > b.toMap().value(QStringLiteral("epoch")).toLongLong();
              });
    emit measurementsChanged();
    if (!m_measurements.isEmpty())
        setStatus(tr("Loaded %1 saved measurement(s)").arg(m_measurements.size()));
}

void Omron::saveSettings() const
{
    QDir().mkpath(QFileInfo(settingsFile()).absolutePath());
    QJsonObject obj;
    obj.insert(QStringLiteral("pairingKey"),
               QString::fromLatin1(m_proto->unlockKey().toHex()));
    obj.insert(QStringLiteral("chartPerson"), m_chartPerson);
    obj.insert(QStringLiteral("modelId"), m_modelId);
    obj.insert(QStringLiteral("detectedModelId"), m_detectedId);
    QJsonArray dropped;
    for (const QString &k : m_suppressed)
        dropped.append(k);
    obj.insert(QStringLiteral("deleted"), dropped);
    QFile f(settingsFile());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void Omron::loadSettings()
{
    QFile f(settingsFile());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    const QByteArray key = QByteArray::fromHex(
        obj.value(QStringLiteral("pairingKey")).toString().toLatin1());
    if (key.size() == 16)
        m_proto->setUnlockKey(key);
    m_modelId = obj.value(QStringLiteral("modelId")).toString();
    if (!m_modelId.isEmpty() && !OmronProtocol::profileById(m_modelId))
        m_modelId.clear();                       // a slug from a newer build
    m_proto->setModelOverride(m_modelId);
    m_detectedId = obj.value(QStringLiteral("detectedModelId")).toString();
    if (!m_detectedId.isEmpty() && m_detectedId != QLatin1String("unknown")
        && !OmronProtocol::profileById(m_detectedId))
        m_detectedId.clear();
    const int p = obj.value(QStringLiteral("chartPerson")).toInt(1);
    m_chartPerson = (p == 2) ? 2 : 1;
    m_suppressed.clear();
    for (const QJsonValue &v : obj.value(QStringLiteral("deleted")).toArray())
        m_suppressed.insert(v.toString());
}

QString Omron::saveToFile()
{
    if (m_measurements.isEmpty()) {
        emit actionError(tr("Nothing to save."));
        return QString();
    }
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/somble-archive-")
                       + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
                       + QStringLiteral(".json");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit actionError(tr("Cannot write %1").arg(path));
        return QString();
    }
    f.write(QJsonDocument(archiveToJson(m_measurements)).toJson(QJsonDocument::Indented));
    emit actionInfo(tr("Saved %1 readings: %2").arg(m_measurements.size()).arg(path));
    return path;
}

void Omron::loadFromFile(const QString &path, bool merge)
{
    QString p = path;
    if (p.startsWith(QStringLiteral("file://")))
        p = p.mid(7);
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly)) {
        emit actionError(tr("Cannot open %1").arg(p));
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    QJsonArray arr = doc.object().value(QStringLiteral("measurements")).toArray();
    if (arr.isEmpty() && doc.isArray())
        arr = doc.array();                      // tolerate a bare array
    if (arr.isEmpty()) {
        emit actionError(tr("No measurements in that file."));
        return;
    }
    QVariantList recs;
    for (const QJsonValue &v : arr)
        recs.append(v.toObject().toVariantMap());
    if (!merge)
        m_measurements.clear();
    const int added = mergeRecords(recs);
    saveData();
    emit actionInfo(tr("Loaded %1: +%2 (%3 total)")
                        .arg(QFileInfo(p).fileName()).arg(added).arg(m_measurements.size()));
}

void Omron::clearArchive()
{
    // Clearing only empties the list; it does not mark everything as deleted,
    // so a later download restores whatever the monitor still holds.
    m_measurements.clear();
    saveData();
    emit measurementsChanged();
    setStatus(tr("Archive cleared"));
}

// --- exports ---------------------------------------------------------------

QString Omron::exportCsv()
{
    if (m_measurements.isEmpty()) {
        emit actionError(tr("No measurements to export."));
        return QString();
    }
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/somble-")
                       + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
                       + QStringLiteral(".csv");

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit actionError(tr("Cannot write %1").arg(path));
        return QString();
    }
    // An unset monitor clock has no meaningful timestamp; leave the column
    // empty rather than exporting a fabricated date.
    f.write("timestamp,person,systolic,diastolic,pulse,arrhythmia,movement\n");
    for (const QVariant &v : m_measurements) {
        const QVariantMap m = v.toMap();
        const QString line = QStringLiteral("%1,P%2,%3,%4,%5,%6,%7\n")
            .arg(m.value(QStringLiteral("timeValid"), true).toBool()
                     ? m.value(QStringLiteral("timestamp")).toString()
                     : QString())
            .arg(m.value(QStringLiteral("person"), 1).toInt())
            .arg(m.value(QStringLiteral("systolic")).toInt())
            .arg(m.value(QStringLiteral("diastolic")).toInt())
            .arg(m.value(QStringLiteral("pulse")).toInt())
            .arg(m.value(QStringLiteral("arrhythmia")).toBool() ? 1 : 0)
            .arg(m.value(QStringLiteral("movement")).toBool() ? 1 : 0);
        f.write(line.toUtf8());
    }
    f.close();
    emit actionInfo(tr("CSV exported: %1").arg(path));
    return path;
}

QString Omron::reserveImagePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    QDir().mkpath(dir);
    return dir + QStringLiteral("/somble-chart-")
         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
         + QStringLiteral(".jpg");
}
