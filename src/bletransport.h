#ifndef SOMBLE_BLETRANSPORT_H
#define SOMBLE_BLETRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QHash>
#include <QString>
#include <QStringList>

class QDBusInterface;
class QTimer;

// Generic BLE central built on BlueZ's D-Bus GATT API (system bus, org.bluez).
//
// QtBluetooth is deliberately not used: the Sailfish build target ships no
// QtConnectivity development package, only the runtime library. BlueZ's D-Bus
// API is present on every device and needs nothing but Qt5DBus.
//
// The transport is protocol-agnostic — it finds a device by name prefix,
// brings the link up, resolves its characteristics by UUID and then offers
// read / write / notify on them. The Omron command layer sits on top.
class BleTransport : public QObject
{
    Q_OBJECT
public:
    explicit BleTransport(QObject *parent = nullptr);
    ~BleTransport() override;

    // A device matches if its Name or Alias starts with any of `prefixes`
    // (case-insensitive), or if its advertised service UUIDs contain
    // `serviceUuid`. Both are needed: the monitor advertises under different
    // names depending on its state, and an Alias set by a previous pairing
    // disappears when that pairing is removed — while the service UUID is
    // constant.
    void setNamePrefixes(const QStringList &prefixes) { m_namePrefixes = prefixes; }
    void setServiceUuid(const QString &uuid) { m_serviceUuid = uuid.toLower(); }
    // Characteristics the protocol layer needs; resolution fails unless all of
    // them are present on the connected device.
    void setRequiredUuids(const QStringList &uuids) { m_required = uuids; }

    // Connect (scanning first if the device is not yet known to BlueZ) and
    // resolve services. Emits ready() or errorOccurred().
    void open();
    void close();
    bool isOpen() const { return m_ready; }

    QString deviceAddress() const { return m_address; }
    QString deviceName() const { return m_name; }
    bool devicePaired() const { return m_paired; }
    int mtu() const { return m_mtu; }

    // All three are no-ops (returning false / empty) unless isOpen().
    bool startNotify(const QString &uuid);
    bool stopNotify(const QString &uuid);
    // withResponse=false uses a GATT "command" (write-without-response).
    bool writeValue(const QString &uuid, const QByteArray &data, bool withResponse = true);
    QByteArray readValue(const QString &uuid, bool *ok = nullptr);

signals:
    void ready();
    void closed();
    void errorOccurred(const QString &message);
    void progress(const QString &message);
    // A notification/indication arrived on `uuid`.
    void notified(const QString &uuid, const QByteArray &value);

private slots:
    // The trailing QDBusMessage is filled in by QtDBus and is the only way to
    // learn which object emitted the signal — PropertiesChanged does not carry
    // the path in its arguments.
    void onPropertiesChanged(const QString &interface,
                             const QVariantMap &changed,
                             const QStringList &invalidated,
                             const QDBusMessage &message);
    void onInterfacesAdded(const QDBusObjectPath &path,
                           const QMap<QString, QVariantMap> &interfaces);

private:
    bool findDevice();          // locate the device object path by name prefix
    bool resolveChars();        // map UUID -> characteristic object path
    void onServicesResolved();
    void startDiscovery();
    void stopDiscovery();
    void connectDevice();
    void onConnectFailed(const QString &errorName);
    void onResolveTimeout();    // connected, but the link never became usable
    void recoverStaleBond();    // best-effort Pair(), else tell the user
    void pairDevice();
    void fail(const QString &message);
    QString charPath(const QString &uuid) const;

    bool matchesDevice(const QVariantMap &properties) const;

    QStringList m_namePrefixes;
    QString m_serviceUuid;
    QStringList m_required;

    QString m_adapterPath;
    QString m_devicePath;
    QString m_address;
    QString m_name;
    bool m_paired = false;
    bool m_discovering = false;
    bool m_connecting = false;      // a Connect() is in flight
    bool m_rebonding = false;       // a recovery attempt is under way
    QTimer *m_resolveTimer = nullptr;
    int m_connectAttempts = 0;
    bool m_ready = false;
    int m_mtu = 20;                          // conservative GATT payload

    QHash<QString, QString> m_chars;         // lowercase UUID -> object path
    QHash<QString, QString> m_pathToUuid;    // object path -> lowercase UUID
    QStringList m_notifying;                 // UUIDs we hold a notify session on
};

#endif // SOMBLE_BLETRANSPORT_H
