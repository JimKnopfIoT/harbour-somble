#include "bletransport.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>
#include <QDebug>

namespace {
const QString kBluez         = QStringLiteral("org.bluez");
const QString kObjMgrIface   = QStringLiteral("org.freedesktop.DBus.ObjectManager");
const QString kPropsIface    = QStringLiteral("org.freedesktop.DBus.Properties");
const QString kAdapterIface  = QStringLiteral("org.bluez.Adapter1");
const QString kDeviceIface   = QStringLiteral("org.bluez.Device1");
const QString kGattCharIface = QStringLiteral("org.bluez.GattCharacteristic1");

// How long we scan before giving up on finding the monitor, and how long the
// whole open() may take. The Omron only advertises for a short while after the
// Bluetooth button is pressed, so the scan window is the generous one.
const int kScanMs   = 25000;
// Generous, because open() may have to run a whole re-bond cycle (retries,
// RemoveDevice, a fresh scan, Pair, Connect) before it gives up.
const int kOpenMs   = 90000;
}

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;
Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

BleTransport::BleTransport(QObject *parent) : QObject(parent)
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    // A peer that has forgotten its bond still lets BlueZ complete Connect():
    // the link comes up, encryption with the stale key is refused, and the
    // connection is dropped again — without any D-Bus error. The only visible
    // symptom is that ServicesResolved never arrives, so watch for that.
    m_resolveTimer = new QTimer(this);
    m_resolveTimer->setSingleShot(true);
    m_resolveTimer->setInterval(12000);
    connect(m_resolveTimer, &QTimer::timeout, this, &BleTransport::onResolveTimeout);
}

void BleTransport::onResolveTimeout()
{
    if (m_ready)
        return;
    qWarning() << "somble: connected but services never resolved — stale bond?";
    recoverStaleBond();
}

BleTransport::~BleTransport()
{
    close();
}

void BleTransport::fail(const QString &message)
{
    close();
    emit errorOccurred(message);
}

// Pull a QByteArray out of a D-Bus "ay", which QtDBus may hand us either
// already converted or still wrapped in a QDBusArgument.
static QByteArray toByteArray(const QVariant &v)
{
    if (v.canConvert<QByteArray>())
        return v.toByteArray();
    if (v.canConvert<QDBusArgument>()) {
        QByteArray out;
        QDBusArgument a = v.value<QDBusArgument>();
        a >> out;
        return out;
    }
    return QByteArray();
}

bool BleTransport::matchesDevice(const QVariantMap &dev) const
{
    const QString name  = dev.value(QStringLiteral("Name")).toString();
    const QString alias = dev.value(QStringLiteral("Alias")).toString();
    for (const QString &prefix : m_namePrefixes) {
        if (name.startsWith(prefix, Qt::CaseInsensitive)
            || alias.startsWith(prefix, Qt::CaseInsensitive))
            return true;
    }
    if (!m_serviceUuid.isEmpty()) {
        const QStringList uuids = dev.value(QStringLiteral("UUIDs")).toStringList();
        for (const QString &u : uuids)
            if (u.compare(m_serviceUuid, Qt::CaseInsensitive) == 0)
                return true;
    }
    return false;
}

bool BleTransport::findDevice()
{
    QDBusInterface om(kBluez, QStringLiteral("/"), kObjMgrIface, QDBusConnection::systemBus());
    QDBusReply<ManagedObjectList> reply = om.call(QStringLiteral("GetManagedObjects"));
    if (!reply.isValid())
        return false;

    const ManagedObjectList objects = reply.value();
    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        if (!it.value().contains(kDeviceIface))
            continue;
        const QVariantMap dev = it.value().value(kDeviceIface);
        if (!matchesDevice(dev))
            continue;
        const QString name  = dev.value(QStringLiteral("Name")).toString();
        const QString alias = dev.value(QStringLiteral("Alias")).toString();
        m_devicePath = it.key().path();
        m_address    = dev.value(QStringLiteral("Address")).toString();
        m_name       = name.isEmpty() ? alias : name;
        m_paired     = dev.value(QStringLiteral("Paired")).toBool();
        return true;
    }
    return false;
}

void BleTransport::open()
{
    if (m_ready)
        return;
    m_connectAttempts = 0;
    m_rebonding = false;

    QDBusConnection bus = QDBusConnection::systemBus();

    // Find the adapter first — we may have to scan before the device exists as
    // a BlueZ object at all.
    {
        QDBusInterface om(kBluez, QStringLiteral("/"), kObjMgrIface, bus);
        QDBusReply<ManagedObjectList> reply = om.call(QStringLiteral("GetManagedObjects"));
        if (!reply.isValid()) {
            emit errorOccurred(tr("Cannot talk to BlueZ."));
            return;
        }
        const ManagedObjectList objects = reply.value();
        for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
            if (it.value().contains(kAdapterIface)) {
                m_adapterPath = it.key().path();
                break;
            }
        }
    }
    if (m_adapterPath.isEmpty()) {
        emit errorOccurred(tr("No Bluetooth adapter found."));
        return;
    }
    qWarning() << "somble: adapter" << m_adapterPath;

    // Watch for the monitor showing up while we scan.
    bus.connect(kBluez, QString(), kObjMgrIface, QStringLiteral("InterfacesAdded"),
                this, SLOT(onInterfacesAdded(QDBusObjectPath, QMap<QString, QVariantMap>)));

    // BlueZ frequently fails to connect a known-but-not-currently-advertising LE
    // device unless a scan is running, so scan first either way.
    startDiscovery();

    if (findDevice()) {
        qWarning() << "somble: known device" << m_devicePath << m_name << "paired" << m_paired;
        emit progress(tr("Found %1, connecting…").arg(m_name));
        QTimer::singleShot(1500, this, [this]() { connectDevice(); });
    } else {
        qWarning() << "somble: device unknown to BlueZ, scanning";
        emit progress(tr("Scanning for the monitor…"));
        QTimer::singleShot(kScanMs, this, [this]() {
            if (!m_ready && m_devicePath.isEmpty())
                fail(tr("Monitor not found. Press its Bluetooth button so it starts "
                        "advertising, then try again."));
        });
    }

    QTimer::singleShot(kOpenMs, this, [this]() {
        if (!m_ready)
            fail(tr("Bluetooth timed out. Wake the monitor and try again."));
    });
}

void BleTransport::onInterfacesAdded(const QDBusObjectPath &path,
                                     const QMap<QString, QVariantMap> &interfaces)
{
    if (m_ready || !m_devicePath.isEmpty())
        return;
    if (!interfaces.contains(kDeviceIface))
        return;
    const QVariantMap dev = interfaces.value(kDeviceIface);
    if (!matchesDevice(dev))
        return;
    const QString name  = dev.value(QStringLiteral("Name")).toString();
    const QString alias = dev.value(QStringLiteral("Alias")).toString();

    qWarning() << "somble: InterfacesAdded" << path.path() << name << alias;
    m_devicePath = path.path();
    m_address    = dev.value(QStringLiteral("Address")).toString();
    m_name       = name.isEmpty() ? alias : name;
    m_paired     = dev.value(QStringLiteral("Paired")).toBool();
    emit progress(tr("Found %1, connecting…").arg(m_name));
    if (m_rebonding)
        pairDevice();
    else
        connectDevice();
}

void BleTransport::startDiscovery()
{
    if (m_discovering || m_adapterPath.isEmpty())
        return;
    QDBusInterface adapter(kBluez, m_adapterPath, kAdapterIface, QDBusConnection::systemBus());
    QVariantMap filter;
    filter.insert(QStringLiteral("Transport"), QStringLiteral("le"));
    adapter.call(QStringLiteral("SetDiscoveryFilter"), QVariant::fromValue(filter));
    adapter.call(QStringLiteral("StartDiscovery"));
    m_discovering = true;
}

void BleTransport::stopDiscovery()
{
    if (!m_discovering || m_adapterPath.isEmpty())
        return;
    QDBusInterface adapter(kBluez, m_adapterPath, kAdapterIface, QDBusConnection::systemBus());
    adapter.asyncCall(QStringLiteral("StopDiscovery"));
    m_discovering = false;
}

void BleTransport::connectDevice()
{
    // Two Connect() calls in flight make BlueZ abort one of them with
    // "le-connection-abort-by-local", so only ever have one outstanding.
    if (m_ready || m_devicePath.isEmpty() || m_connecting)
        return;
    m_connecting = true;

    // Connecting while the controller is still scanning is the other half of
    // that same abort, so stop discovery before dialling out rather than after
    // the services resolve.
    stopDiscovery();

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(kBluez, m_devicePath, kPropsIface, QStringLiteral("PropertiesChanged"),
                this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));

    QDBusInterface dev(kBluez, m_devicePath, kDeviceIface, bus);

    // If services are already resolved from an earlier session we can go
    // straight ahead; otherwise Connect() and wait for ServicesResolved.
    if (dev.property("Connected").toBool() && dev.property("ServicesResolved").toBool()) {
        m_connecting = false;
        onServicesResolved();
        return;
    }

    qWarning() << "somble: Connect() attempt" << m_connectAttempts;
    QDBusPendingCall pc = dev.asyncCall(QStringLiteral("Connect"));
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(pc, this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        QDBusPendingReply<> reply = *self;
        m_connecting = false;
        qWarning() << "somble: Connect() returned, error =" << reply.isError();
        if (reply.isError() && !m_ready) {
            qWarning() << "somble: Connect failed:" << reply.error().name()
                       << reply.error().message();
            onConnectFailed(reply.error().name());
        }
        self->deleteLater();
    });
}

// Most connect failures just mean the monitor is not advertising right now, so
// retry a few times with a scan running. Only an error that actually points at
// the bond is worth escalating.
void BleTransport::onConnectFailed(const QString &errorName)
{
    if (m_ready)
        return;

    const bool bondProblem = errorName.contains(QStringLiteral("Authentication"))
                          || errorName.contains(QStringLiteral("NotAuthorized"));
    if (bondProblem) {
        recoverStaleBond();
        return;
    }
    if (++m_connectAttempts <= 3) {
        startDiscovery();
        QTimer::singleShot(2500, this, [this]() { connectDevice(); });
        return;
    }
    fail(tr("Could not connect to the monitor. Press its Bluetooth button, "
            "then try again."));
}

// The app runs as the ordinary user, and Sailfish's BlueZ policy does not let
// that user call Adapter1.RemoveDevice or AgentManager1.RegisterAgent — both
// are silently refused. So the app cannot drop a stale bond or drive a pairing
// on its own; the most it can do is ask BlueZ to bond and, failing that, say
// precisely what has to be done in the system settings.
void BleTransport::recoverStaleBond()
{
    if (m_rebonding) {
        fail(tr("The monitor no longer accepts this phone's Bluetooth pairing. "
                "Open Settings → Bluetooth, remove “%1”, then pair it again "
                "(press the monitor's Bluetooth button so it appears).")
                 .arg(m_name));
        return;
    }
    m_rebonding = true;
    emit progress(tr("Re-pairing with the monitor…"));
    pairDevice();
}

void BleTransport::pairDevice()
{
    if (m_ready || m_devicePath.isEmpty())
        return;
    stopDiscovery();

    QDBusInterface dev(kBluez, m_devicePath, kDeviceIface, QDBusConnection::systemBus());
    QDBusPendingCall pc = dev.asyncCall(QStringLiteral("Pair"));
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(pc, this);
    connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        QDBusPendingReply<> reply = *self;
        const bool failed = reply.isError();
        if (failed)
            qWarning() << "somble: Pair failed:" << reply.error().name()
                       << reply.error().message();
        self->deleteLater();
        if (failed) {
            // AlreadyExists means BlueZ still holds the stale bond it will not
            // let us drop, so no amount of retrying here can help.
            fail(tr("The monitor no longer accepts this phone's Bluetooth pairing. "
                    "Open Settings → Bluetooth, remove “%1”, then pair it again "
                    "(press the monitor's Bluetooth button so it appears).")
                     .arg(m_name));
            return;
        }
        connectDevice();
    });
}

QString BleTransport::charPath(const QString &uuid) const
{
    return m_chars.value(uuid.toLower());
}

bool BleTransport::resolveChars()
{
    m_chars.clear();
    m_pathToUuid.clear();

    QDBusInterface om(kBluez, QStringLiteral("/"), kObjMgrIface, QDBusConnection::systemBus());
    QDBusReply<ManagedObjectList> reply = om.call(QStringLiteral("GetManagedObjects"));
    if (!reply.isValid())
        return false;

    const ManagedObjectList objects = reply.value();
    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const QString path = it.key().path();
        if (!path.startsWith(m_devicePath + QLatin1Char('/')))
            continue;
        if (!it.value().contains(kGattCharIface))
            continue;
        const QString uuid = it.value().value(kGattCharIface)
                                 .value(QStringLiteral("UUID")).toString().toLower();
        m_chars.insert(uuid, path);
        m_pathToUuid.insert(path, uuid);
    }

    for (const QString &need : m_required)
        if (!m_chars.contains(need.toLower()))
            return false;
    return true;
}

void BleTransport::onServicesResolved()
{
    if (m_ready)
        return;

    stopDiscovery();

    if (!resolveChars()) {
        qWarning() << "somble: resolveChars failed;" << m_chars.size() << "chars found";
        fail(tr("This device does not expose the expected Omron data service."));
        return;
    }
    qWarning() << "somble: services resolved," << m_chars.size() << "characteristics";

    // Best-effort: BlueZ exposes the negotiated ATT MTU on the characteristic.
    const QString any = m_chars.constBegin().value();
    QDBusInterface c(kBluez, any, kGattCharIface, QDBusConnection::systemBus());
    const QVariant mtu = c.property("MTU");
    if (mtu.isValid() && mtu.toInt() > 23)
        m_mtu = mtu.toInt() - 3;

    QDBusInterface dev(kBluez, m_devicePath, kDeviceIface, QDBusConnection::systemBus());
    m_paired = dev.property("Paired").toBool();
    // Trusted devices are auto-authorised by BlueZ on every later reconnect,
    // which removes one more reason for a connect to be refused.
    if (!dev.property("Trusted").toBool())
        dev.setProperty("Trusted", true);

    m_ready = true;
    emit ready();
}

void BleTransport::onPropertiesChanged(const QString &interface,
                                       const QVariantMap &changed,
                                       const QStringList &invalidated,
                                       const QDBusMessage &message)
{
    Q_UNUSED(invalidated)

    if (interface == kDeviceIface) {
        qWarning() << "somble: device props" << changed.keys();
        if (changed.contains(QStringLiteral("Connected"))
            && changed.value(QStringLiteral("Connected")).toBool() && !m_ready)
            m_resolveTimer->start();
        if (changed.value(QStringLiteral("ServicesResolved")).toBool()) {
            m_resolveTimer->stop();
            onServicesResolved();
        }
        if (changed.contains(QStringLiteral("Connected"))
            && !changed.value(QStringLiteral("Connected")).toBool() && m_ready) {
            m_ready = false;
            m_notifying.clear();
            emit closed();
        }
        return;
    }

    if (interface == kGattCharIface && changed.contains(QStringLiteral("Value"))) {
        // PropertiesChanged carries no object path, so QDBusConnection::connect()
        // was made per characteristic path; map back via the sender path.
        const QString path = message.path();
        const QString uuid = m_pathToUuid.value(path);
        const QByteArray bytes = toByteArray(changed.value(QStringLiteral("Value")));
        if (!uuid.isEmpty() && !bytes.isEmpty())
            emit notified(uuid, bytes);
    }
}

bool BleTransport::startNotify(const QString &uuid)
{
    if (!m_ready)
        return false;
    const QString path = charPath(uuid);
    if (path.isEmpty())
        return false;
    if (m_notifying.contains(uuid.toLower()))
        return true;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(kBluez, path, kPropsIface, QStringLiteral("PropertiesChanged"),
                this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));

    QDBusInterface c(kBluez, path, kGattCharIface, bus);
    QDBusReply<void> r = c.call(QStringLiteral("StartNotify"));
    if (!r.isValid()) {
        qWarning() << "harbour-somble: StartNotify" << uuid << r.error().message();
        return false;
    }
    m_notifying.append(uuid.toLower());
    return true;
}

bool BleTransport::stopNotify(const QString &uuid)
{
    const QString path = charPath(uuid);
    if (path.isEmpty())
        return false;
    QDBusConnection bus = QDBusConnection::systemBus();
    bus.disconnect(kBluez, path, kPropsIface, QStringLiteral("PropertiesChanged"),
                   this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));
    if (m_notifying.removeAll(uuid.toLower()) > 0) {
        QDBusInterface c(kBluez, path, kGattCharIface, bus);
        c.asyncCall(QStringLiteral("StopNotify"));
    }
    return true;
}

bool BleTransport::writeValue(const QString &uuid, const QByteArray &data, bool withResponse)
{
    if (!m_ready)
        return false;
    const QString path = charPath(uuid);
    if (path.isEmpty())
        return false;

    QVariantMap options;
    options.insert(QStringLiteral("type"),
                   withResponse ? QStringLiteral("request") : QStringLiteral("command"));

    QDBusInterface c(kBluez, path, kGattCharIface, QDBusConnection::systemBus());
    // Asynchronous on purpose: the Omron answers on a notification channel, and
    // a blocking WriteValue would stall the event loop that delivers it.
    c.asyncCall(QStringLiteral("WriteValue"),
                QVariant::fromValue(data), QVariant::fromValue(options));
    return true;
}

QByteArray BleTransport::readValue(const QString &uuid, bool *ok)
{
    if (ok) *ok = false;
    if (!m_ready)
        return QByteArray();
    const QString path = charPath(uuid);
    if (path.isEmpty())
        return QByteArray();

    QDBusInterface c(kBluez, path, kGattCharIface, QDBusConnection::systemBus());
    QDBusReply<QByteArray> r = c.call(QStringLiteral("ReadValue"), QVariant::fromValue(QVariantMap()));
    if (!r.isValid())
        return QByteArray();
    if (ok) *ok = true;
    return r.value();
}

void BleTransport::close()
{
    QDBusConnection bus = QDBusConnection::systemBus();

    const QStringList notifying = m_notifying;
    for (const QString &uuid : notifying)
        stopNotify(uuid);

    stopDiscovery();
    m_resolveTimer->stop();
    bus.disconnect(kBluez, QString(), kObjMgrIface, QStringLiteral("InterfacesAdded"),
                   this, SLOT(onInterfacesAdded(QDBusObjectPath, QMap<QString, QVariantMap>)));

    if (!m_devicePath.isEmpty()) {
        bus.disconnect(kBluez, m_devicePath, kPropsIface, QStringLiteral("PropertiesChanged"),
                       this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));
        QDBusInterface dev(kBluez, m_devicePath, kDeviceIface, bus);
        dev.asyncCall(QStringLiteral("Disconnect"));
    }

    m_chars.clear();
    m_pathToUuid.clear();
    m_devicePath.clear();

    const bool was = m_ready;
    m_ready = false;
    if (was)
        emit closed();
}
