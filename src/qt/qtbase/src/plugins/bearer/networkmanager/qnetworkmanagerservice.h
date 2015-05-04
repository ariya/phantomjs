/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKMANAGERSERVICE_H
#define QNETWORKMANAGERSERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>

#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusAbstractInterface>
#include <QMap>

#ifndef QT_NO_DBUS

#ifndef NETWORK_MANAGER_H
typedef enum NMDeviceType
{
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_ETHERNET,
    DEVICE_TYPE_WIFI,
    DEVICE_TYPE_MODEM = 8
} NMDeviceType;

typedef enum
{
    NM_DEVICE_STATE_UNKNOWN = 0,
    NM_DEVICE_STATE_UNMANAGED = 10,
    NM_DEVICE_STATE_UNAVAILABLE = 20,
    NM_DEVICE_STATE_DISCONNECTED = 30,
    NM_DEVICE_STATE_PREPARE = 40,
    NM_DEVICE_STATE_CONFIG = 50,
    NM_DEVICE_STATE_NEED_AUTH = 60,
    NM_DEVICE_STATE_IP_CONFIG = 70,
    NM_DEVICE_STATE_ACTIVATED = 100,
    NM_DEVICE_STATE_DEACTIVATING = 110,
    NM_DEVICE_STATE_FAILED = 120
} NMDeviceState;

typedef enum
{
    NM_ACTIVE_CONNECTION_STATE_UNKNOWN = 0,
    NM_ACTIVE_CONNECTION_STATE_ACTIVATING,
    NM_ACTIVE_CONNECTION_STATE_ACTIVATED,
    NM_ACTIVE_CONNECTION_STATE_DEACTIVATED = 4
} NMActiveConnectionState;

#define NM_DBUS_SERVICE                     "org.freedesktop.NetworkManager"

#define NM_DBUS_PATH                        "/org/freedesktop/NetworkManager"
#define NM_DBUS_INTERFACE                   "org.freedesktop.NetworkManager"
#define NM_DBUS_INTERFACE_DEVICE            NM_DBUS_INTERFACE ".Device"
#define NM_DBUS_INTERFACE_DEVICE_WIRED      NM_DBUS_INTERFACE_DEVICE ".Wired"
#define NM_DBUS_INTERFACE_DEVICE_WIRELESS   NM_DBUS_INTERFACE_DEVICE ".Wireless"
#define NM_DBUS_INTERFACE_DEVICE_MODEM      NM_DBUS_INTERFACE_DEVICE ".Modem"
#define NM_DBUS_PATH_ACCESS_POINT           NM_DBUS_PATH "/AccessPoint"
#define NM_DBUS_INTERFACE_ACCESS_POINT      NM_DBUS_INTERFACE ".AccessPoint"

#define NM_DBUS_PATH_SETTINGS               "/org/freedesktop/NetworkManager/Settings"

#define NM_DBUS_IFACE_SETTINGS_CONNECTION   "org.freedesktop.NetworkManager.Settings.Connection"
#define NM_DBUS_IFACE_SETTINGS              "org.freedesktop.NetworkManager.Settings"
#define NM_DBUS_INTERFACE_ACTIVE_CONNECTION NM_DBUS_INTERFACE ".Connection.Active"
#define NM_DBUS_INTERFACE_IP4_CONFIG        NM_DBUS_INTERFACE ".IP4Config"

#define NM_DBUS_SERVICE_USER_SETTINGS       "org.freedesktop.NetworkManagerUserSettings"
#define NM_DBUS_SERVICE_SYSTEM_SETTINGS     "org.freedesktop.NetworkManagerSystemSettings"

#define NM_802_11_AP_FLAGS_NONE             0x00000000
#define NM_802_11_AP_FLAGS_PRIVACY          0x00000001
#endif

QT_BEGIN_NAMESPACE

typedef QMap< QString, QMap<QString,QVariant> > QNmSettingsMap;
typedef QList<quint32> ServerThing;

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QNmSettingsMap))
Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(ServerThing))

QT_BEGIN_NAMESPACE

class QNetworkManagerInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    typedef enum
    {
        NM_STATE_UNKNOWN = 0,
        NM_STATE_ASLEEP = 10,
        NM_STATE_DISCONNECTED = 20,
        NM_STATE_DISCONNECTING = 30,
        NM_STATE_CONNECTING = 40,
        NM_STATE_CONNECTED_LOCAL = 50,
        NM_STATE_CONNECTED_SITE = 60,
        NM_STATE_CONNECTED_GLOBAL = 70
    } NMState;

    QNetworkManagerInterface(QObject *parent = 0);
    ~QNetworkManagerInterface();

    QList <QDBusObjectPath> getDevices();
    void activateConnection(QDBusObjectPath connection,QDBusObjectPath device, QDBusObjectPath specificObject);
    void deactivateConnection(QDBusObjectPath connectionPath);

    QDBusObjectPath path() const;

    bool wirelessEnabled() const;
    bool wirelessHardwareEnabled() const;
    QList <QDBusObjectPath> activeConnections() const;
    NMState state();
    QString version() const;
    bool setConnections();

Q_SIGNALS:
    void deviceAdded(QDBusObjectPath);
    void deviceRemoved(QDBusObjectPath);
    void propertiesChanged(QMap<QString,QVariant>);
    void stateChanged(quint32);
    void activationFinished(QDBusPendingCallWatcher*);
    void propertiesReady();
    void devicesListReady();

private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);

private:
    QVariantMap propertyMap;
    QList<QDBusObjectPath> devicesPathList;

};

class QNetworkManagerInterfaceAccessPoint : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    enum DeviceState {
        Unknown = 0,
        Unmanaged,
        Unavailable,
        Disconnected,
        Prepare,
        Config,
        NeedAuthentication,
        IpConfig,
        Activated,
        Failed
    };

    enum ApFlag {
        ApNone = 0x0,
        Privacy = 0x1
    };

    Q_DECLARE_FLAGS(ApFlags, ApFlag)

    enum ApSecurityFlag {
        ApSecurityNone = 0x0,
        PairWep40 = 0x1,
        PairWep104 = 0x2,
        PairTkip = 0x4,
        PairCcmp = 0x8,
        GroupWep40 = 0x10,
        GroupWep104 = 0x20,
        GroupTkip = 0x40,
        GroupCcmp = 0x80,
        KeyPsk = 0x100,
        Key8021x = 0x200
    };

    Q_DECLARE_FLAGS(ApSecurityFlags, ApSecurityFlag)

    explicit QNetworkManagerInterfaceAccessPoint(const QString &dbusPathName, QObject *parent = 0);
    ~QNetworkManagerInterfaceAccessPoint();

    quint32 flags() const;
    quint32 wpaFlags() const;
    quint32 rsnFlags() const;
    QString ssid() const;
    quint32 frequency() const;
    QString hwAddress() const;
    quint32 mode() const;
    quint32 maxBitrate() const;
    quint32 strength() const;
  //  bool setConnections();

Q_SIGNALS:
    void propertiesChanged(QMap <QString,QVariant>);
    void propertiesReady();

private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);

private:
    QVariantMap propertyMap;
};

class QNetworkManagerInterfaceDevice : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QNetworkManagerInterfaceDevice(const QString &deviceObjectPath, QObject *parent = 0);
    ~QNetworkManagerInterfaceDevice();

    QString udi() const;
    QString networkInterface() const;
    quint32 ip4Address() const;
    quint32 state() const;
    quint32 deviceType() const;

    QDBusObjectPath ip4config() const;

Q_SIGNALS:
    void stateChanged(const QString &, quint32);
    void propertiesChanged(QMap<QString,QVariant>);
    void connectionsChanged(QStringList);
    void propertiesReady();
private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);
private:
    QVariantMap propertyMap;
};

class QNetworkManagerInterfaceDeviceWired : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QNetworkManagerInterfaceDeviceWired(const QString &ifaceDevicePath,
                                                 QObject *parent = 0);
    ~QNetworkManagerInterfaceDeviceWired();

    QString hwAddress() const;
    quint32 speed() const;
    bool carrier() const;
    QStringList availableConnections();

Q_SIGNALS:
    void propertiesChanged(QMap<QString,QVariant>);
    void propertiesReady();
    void carrierChanged(bool);

private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);

private:
    QVariantMap propertyMap;
};

class QNetworkManagerInterfaceDeviceWireless : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    enum DeviceCapability {
        None = 0x0,
        Wep40 = 0x1,
        Wep104 = 0x2,
        Tkip = 0x4,
        Ccmp = 0x8,
        Wpa = 0x10,
        Rsn = 0x20
       };

    explicit QNetworkManagerInterfaceDeviceWireless(const QString &ifaceDevicePath,
                                                    QObject *parent = 0);
    ~QNetworkManagerInterfaceDeviceWireless();

    QDBusObjectPath path() const;
    QList <QDBusObjectPath> getAccessPoints();

    QString hwAddress() const;
    quint32 mode() const;
    quint32 bitrate() const;
    QDBusObjectPath activeAccessPoint() const;
    quint32 wirelessCapabilities() const;
    bool setConnections();

    void requestScan();
Q_SIGNALS:
    void propertiesChanged(QMap<QString,QVariant>);
    void accessPointAdded(const QString &);
    void accessPointRemoved(const QString &);
    void scanDone();
    void propertiesReady();
    void accessPointsReady();

private Q_SLOTS:
    void scanIsDone();
    void propertiesSwap(QMap<QString,QVariant>);

    void slotAccessPointAdded(QDBusObjectPath);
    void slotAccessPointRemoved(QDBusObjectPath);

    void accessPointsFinished(QDBusPendingCallWatcher *watcher);

private:
    QVariantMap propertyMap;
    QList <QDBusObjectPath> accessPointsList;
    QString interfacePath;
};

class QNetworkManagerInterfaceDeviceModem : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    enum ModemCapability {
        None = 0x0,
        Pots = 0x1,
        Cmda_Edvo = 0x2,
        Gsm_Umts = 0x4,
        Lte = 0x08
       };
    Q_DECLARE_FLAGS(ModemCapabilities, ModemCapability)

    explicit QNetworkManagerInterfaceDeviceModem(const QString &ifaceDevicePath,
                                                    QObject *parent = 0);
    ~QNetworkManagerInterfaceDeviceModem();

    ModemCapabilities modemCapabilities() const;
    ModemCapabilities currentCapabilities() const;

Q_SIGNALS:
    void propertiesChanged(QMap<QString,QVariant>);
    void propertiesReady();

private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);

private:
    QVariantMap propertyMap;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QNetworkManagerInterfaceDeviceModem::ModemCapabilities)

class QNetworkManagerSettings : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QNetworkManagerSettings(const QString &settingsService, QObject *parent = 0);
    ~QNetworkManagerSettings();

    QList <QDBusObjectPath> listConnections();
    QString getConnectionByUuid(const QString &uuid);
    bool setConnections();

Q_SIGNALS:
    void newConnection(QDBusObjectPath);
    void connectionsListReady();
private:
    QList <QDBusObjectPath> connectionsList;
    QString interfacePath;
};

class QNetworkManagerSettingsConnection : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    QNetworkManagerSettingsConnection(const QString &settingsService, const QString &connectionObjectPath, QObject *parent = 0);
    ~QNetworkManagerSettingsConnection();

    QNmSettingsMap getSettings();
    bool setConnections();
    NMDeviceType getType();
    bool isAutoConnect();
    quint64 getTimestamp();
    QString getId();
    QString getUuid();
    QString getSsid();
    QString getMacAddress();
    QStringList getSeenBssids();

Q_SIGNALS:
    void updated();
    void removed(const QString &path);
    void settingsReady();

private Q_SLOTS:
    void slotSettingsRemoved();
private:
    QNmSettingsMap settingsMap;
    QString interfacepath;
};

class QNetworkManagerConnectionActive : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    enum ActiveConnectionState {
        Unknown = 0,
        Activating = 1,
        Activated = 2
       };

    explicit QNetworkManagerConnectionActive(const QString &dbusPathName, QObject *parent = 0);
    ~ QNetworkManagerConnectionActive();

    QDBusObjectPath connection() const;
    QDBusObjectPath specificObject() const;
    QStringList devices() const;
    quint32 state() const;
    bool defaultRoute() const;
    bool default6Route() const;


Q_SIGNALS:
    void propertiesChanged(QMap<QString,QVariant>);
    void propertiesReady();

private Q_SLOTS:
    void propertiesSwap(QMap<QString,QVariant>);

private:
    QVariantMap propertyMap;
};

class QNetworkManagerIp4Config : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    explicit QNetworkManagerIp4Config(const QString &dbusPathName, QObject *parent = 0);
    ~QNetworkManagerIp4Config();

    QStringList domains() const;
};

class PropertiesDBusInterface : public QDBusAbstractInterface
{
public:
    PropertiesDBusInterface(const QString &service, const QString &path,
                            const QString &interface, const QDBusConnection &connection,
                            QObject *parent = 0)
        : QDBusAbstractInterface(service, path, interface.toLatin1().data(), connection, parent)
    {}
    ~PropertiesDBusInterface() = default;
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif //QNETWORKMANAGERSERVICE_H
