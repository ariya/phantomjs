/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QList>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCall>

#include "qnetworkmanagerservice.h"
#include "qnmdbushelper.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static QDBusConnection dbusConnection = QDBusConnection::systemBus();

class QNetworkManagerInterfacePrivate
{
public:
    QDBusInterface *connectionInterface;
    bool valid;
};

QNetworkManagerInterface::QNetworkManagerInterface(QObject *parent)
        : QObject(parent)
{
    d = new QNetworkManagerInterfacePrivate();
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                QLatin1String(NM_DBUS_PATH),
                                                QLatin1String(NM_DBUS_INTERFACE),
                                                dbusConnection);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForPropertiesChanged(QString,QMap<QString,QVariant>)),
                    this,SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)));
    connect(nmDBusHelper,SIGNAL(pathForStateChanged(QString,quint32)),
            this, SIGNAL(stateChanged(QString,quint32)));

}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterface::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterface::setConnections()
{
    if(!isValid() )
        return false;
    bool allOk = false;
    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("PropertiesChanged"),
                                nmDBusHelper,SLOT(slotPropertiesChanged(QMap<QString,QVariant>)))) {
        allOk = true;
    }
    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceAdded"),
                                this,SIGNAL(deviceAdded(QDBusObjectPath)))) {
        allOk = true;
    }
    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceRemoved"),
                                  this,SIGNAL(deviceRemoved(QDBusObjectPath)))) {
        allOk = true;
    }

    return allOk;
}

QDBusInterface *QNetworkManagerInterface::connectionInterface()  const
{
    return d->connectionInterface;
}

QList <QDBusObjectPath> QNetworkManagerInterface::getDevices() const
{
    QDBusReply<QList<QDBusObjectPath> > reply =  d->connectionInterface->call(QLatin1String("GetDevices"));
    return reply.value();
}

void QNetworkManagerInterface::activateConnection( const QString &serviceName,
                                                  QDBusObjectPath connectionPath,
                                                  QDBusObjectPath devicePath,
                                                  QDBusObjectPath specificObject)
{
    QDBusPendingCall pendingCall = d->connectionInterface->asyncCall(QLatin1String("ActivateConnection"),
                                                                    QVariant(serviceName),
                                                                    QVariant::fromValue(connectionPath),
                                                                    QVariant::fromValue(devicePath),
                                                                    QVariant::fromValue(specificObject));

   QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingCall, this);
   connect(callWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                    this, SIGNAL(activationFinished(QDBusPendingCallWatcher*)));
}

void QNetworkManagerInterface::deactivateConnection(QDBusObjectPath connectionPath)  const
{
    d->connectionInterface->call(QLatin1String("DeactivateConnection"), QVariant::fromValue(connectionPath));
}

bool QNetworkManagerInterface::wirelessEnabled()  const
{
    return d->connectionInterface->property("WirelessEnabled").toBool();
}

bool QNetworkManagerInterface::wirelessHardwareEnabled()  const
{
    return d->connectionInterface->property("WirelessHardwareEnabled").toBool();
}

QList <QDBusObjectPath> QNetworkManagerInterface::activeConnections() const
{
    QVariant prop = d->connectionInterface->property("ActiveConnections");
    return prop.value<QList<QDBusObjectPath> >();
}

quint32 QNetworkManagerInterface::state()
{
    return d->connectionInterface->property("State").toUInt();
}

class QNetworkManagerInterfaceAccessPointPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceAccessPoint::QNetworkManagerInterfaceAccessPoint(const QString &dbusPathName, QObject *parent)
        : QObject(parent), nmDBusHelper(0)
{
    d = new QNetworkManagerInterfaceAccessPointPrivate();
    d->path = dbusPathName;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT),
                                                dbusConnection);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;

}

QNetworkManagerInterfaceAccessPoint::~QNetworkManagerInterfaceAccessPoint()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceAccessPoint::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterfaceAccessPoint::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;
    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForPropertiesChanged(QString,QMap<QString,QVariant>)),
            this,SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)));

    if(dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT),
                              QLatin1String("PropertiesChanged"),
                              nmDBusHelper,SLOT(slotPropertiesChanged(QMap<QString,QVariant>))) ) {
        allOk = true;

    }
    return allOk;
}

QDBusInterface *QNetworkManagerInterfaceAccessPoint::connectionInterface() const
{
    return d->connectionInterface;
}

quint32 QNetworkManagerInterfaceAccessPoint::flags() const
{
    return d->connectionInterface->property("Flags").toUInt();
}

quint32 QNetworkManagerInterfaceAccessPoint::wpaFlags() const
{
    return d->connectionInterface->property("WpaFlags").toUInt();
}

quint32 QNetworkManagerInterfaceAccessPoint::rsnFlags() const
{
    return d->connectionInterface->property("RsnFlags").toUInt();
}

QString QNetworkManagerInterfaceAccessPoint::ssid() const
{
    return d->connectionInterface->property("Ssid").toString();
}

quint32 QNetworkManagerInterfaceAccessPoint::frequency() const
{
    return d->connectionInterface->property("Frequency").toUInt();
}

QString QNetworkManagerInterfaceAccessPoint::hwAddress() const
{
    return d->connectionInterface->property("HwAddress").toString();
}

quint32 QNetworkManagerInterfaceAccessPoint::mode() const
{
    return d->connectionInterface->property("Mode").toUInt();
}

quint32 QNetworkManagerInterfaceAccessPoint::maxBitrate() const
{
    return d->connectionInterface->property("MaxBitrate").toUInt();
}

quint32 QNetworkManagerInterfaceAccessPoint::strength() const
{
    return d->connectionInterface->property("Strength").toUInt();
}

class QNetworkManagerInterfaceDevicePrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDevice::QNetworkManagerInterfaceDevice(const QString &deviceObjectPath, QObject *parent)
        : QObject(parent), nmDBusHelper(0)
{
    d = new QNetworkManagerInterfaceDevicePrivate();
    d->path = deviceObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE),
                                                dbusConnection);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerInterfaceDevice::~QNetworkManagerInterfaceDevice()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDevice::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterfaceDevice::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;
    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper,SIGNAL(pathForStateChanged(QString,quint32)),
            this, SIGNAL(stateChanged(QString,quint32)));
    if(dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE),
                              QLatin1String("StateChanged"),
                              nmDBusHelper,SLOT(deviceStateChanged(quint32)))) {
        allOk = true;
    }
    return allOk;
}

QDBusInterface *QNetworkManagerInterfaceDevice::connectionInterface() const
{
    return d->connectionInterface;
}

QString QNetworkManagerInterfaceDevice::udi() const
{
    return d->connectionInterface->property("Udi").toString();
}

QString QNetworkManagerInterfaceDevice::networkInterface() const
{
    return d->connectionInterface->property("Interface").toString();
}

quint32 QNetworkManagerInterfaceDevice::ip4Address() const
{
    return d->connectionInterface->property("Ip4Address").toUInt();
}

quint32 QNetworkManagerInterfaceDevice::state() const
{
    return d->connectionInterface->property("State").toUInt();
}

quint32 QNetworkManagerInterfaceDevice::deviceType() const
{
    return d->connectionInterface->property("DeviceType").toUInt();
}

QDBusObjectPath QNetworkManagerInterfaceDevice::ip4config() const
{
    QVariant prop = d->connectionInterface->property("Ip4Config");
    return prop.value<QDBusObjectPath>();
}

class QNetworkManagerInterfaceDeviceWiredPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDeviceWired::QNetworkManagerInterfaceDeviceWired(const QString &ifaceDevicePath, QObject *parent)
    : QObject(parent), nmDBusHelper(0)
{
    d = new QNetworkManagerInterfaceDeviceWiredPrivate();
    d->path = ifaceDevicePath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED),
                                                dbusConnection, parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerInterfaceDeviceWired::~QNetworkManagerInterfaceDeviceWired()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDeviceWired::isValid()
{

    return d->valid;
}

bool QNetworkManagerInterfaceDeviceWired::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;

    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForPropertiesChanged(QString,QMap<QString,QVariant>)),
            this,SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)));
    if(dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED),
                              QLatin1String("PropertiesChanged"),
                              nmDBusHelper,SLOT(slotPropertiesChanged(QMap<QString,QVariant>))) )  {
        allOk = true;
    }
    return allOk;
}

QDBusInterface *QNetworkManagerInterfaceDeviceWired::connectionInterface() const
{
    return d->connectionInterface;
}

QString QNetworkManagerInterfaceDeviceWired::hwAddress() const
{
    return d->connectionInterface->property("HwAddress").toString();
}

quint32 QNetworkManagerInterfaceDeviceWired::speed() const
{
    return d->connectionInterface->property("Speed").toUInt();
}

bool QNetworkManagerInterfaceDeviceWired::carrier() const
{
    return d->connectionInterface->property("Carrier").toBool();
}

class QNetworkManagerInterfaceDeviceWirelessPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDeviceWireless::QNetworkManagerInterfaceDeviceWireless(const QString &ifaceDevicePath, QObject *parent)
    : QObject(parent), nmDBusHelper(0)
{
    d = new QNetworkManagerInterfaceDeviceWirelessPrivate();
    d->path = ifaceDevicePath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                                                dbusConnection, parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerInterfaceDeviceWireless::~QNetworkManagerInterfaceDeviceWireless()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDeviceWireless::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterfaceDeviceWireless::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;
    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForPropertiesChanged(QString,QMap<QString,QVariant>)),
            this,SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)));

    connect(nmDBusHelper, SIGNAL(pathForAccessPointAdded(QString,QDBusObjectPath)),
            this,SIGNAL(accessPointAdded(QString,QDBusObjectPath)));

    connect(nmDBusHelper, SIGNAL(pathForAccessPointRemoved(QString,QDBusObjectPath)),
            this,SIGNAL(accessPointRemoved(QString,QDBusObjectPath)));

    if(!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("AccessPointAdded"),
                              nmDBusHelper, SLOT(slotAccessPointAdded(QDBusObjectPath)))) {
        allOk = true;
    }


    if(!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("AccessPointRemoved"),
                              nmDBusHelper, SLOT(slotAccessPointRemoved(QDBusObjectPath)))) {
        allOk = true;
    }


    if(!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("PropertiesChanged"),
                              nmDBusHelper,SLOT(slotPropertiesChanged(QMap<QString,QVariant>)))) {
        allOk = true;
    }

    return allOk;
}

QDBusInterface *QNetworkManagerInterfaceDeviceWireless::connectionInterface() const
{
    return d->connectionInterface;
}

QList <QDBusObjectPath> QNetworkManagerInterfaceDeviceWireless::getAccessPoints()
{
    QDBusReply<QList<QDBusObjectPath> > reply = d->connectionInterface->call(QLatin1String("GetAccessPoints"));
    return reply.value();
}

QString QNetworkManagerInterfaceDeviceWireless::hwAddress() const
{
    return d->connectionInterface->property("HwAddress").toString();
}

quint32 QNetworkManagerInterfaceDeviceWireless::mode() const
{
    return d->connectionInterface->property("Mode").toUInt();
}

quint32 QNetworkManagerInterfaceDeviceWireless::bitrate() const
{
    return d->connectionInterface->property("Bitrate").toUInt();
}

QDBusObjectPath QNetworkManagerInterfaceDeviceWireless::activeAccessPoint() const
{
    return d->connectionInterface->property("ActiveAccessPoint").value<QDBusObjectPath>();
}

quint32 QNetworkManagerInterfaceDeviceWireless::wirelessCapabilities() const
{
    return d->connectionInterface->property("WirelelessCapabilities").toUInt();
}

class QNetworkManagerSettingsPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerSettings::QNetworkManagerSettings(const QString &settingsService, QObject *parent)
        : QObject(parent)
{
    d = new QNetworkManagerSettingsPrivate();
    d->path = settingsService;
    d->connectionInterface = new QDBusInterface(settingsService,
                                                QLatin1String(NM_DBUS_PATH_SETTINGS),
                                                QLatin1String(NM_DBUS_IFACE_SETTINGS),
                                                dbusConnection);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerSettings::~QNetworkManagerSettings()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerSettings::isValid()
{
    return d->valid;
}

bool QNetworkManagerSettings::setConnections()
{
    bool allOk = false;

    if (!dbusConnection.connect(d->path, QLatin1String(NM_DBUS_PATH_SETTINGS),
                           QLatin1String(NM_DBUS_IFACE_SETTINGS), QLatin1String("NewConnection"),
                           this, SIGNAL(newConnection(QDBusObjectPath)))) {
        allOk = true;
    }

    return allOk;
}

QList <QDBusObjectPath> QNetworkManagerSettings::listConnections()
{
    QDBusReply<QList<QDBusObjectPath> > reply = d->connectionInterface->call(QLatin1String("ListConnections"));
    return  reply.value();
}

QDBusInterface *QNetworkManagerSettings::connectionInterface() const
{
    return d->connectionInterface;
}


class QNetworkManagerSettingsConnectionPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    QString service;
    QNmSettingsMap settingsMap;
    bool valid;
};

QNetworkManagerSettingsConnection::QNetworkManagerSettingsConnection(const QString &settingsService, const QString &connectionObjectPath, QObject *parent)
    : QObject(parent), nmDBusHelper(0)
{
    qDBusRegisterMetaType<QNmSettingsMap>();
    d = new QNetworkManagerSettingsConnectionPrivate();
    d->path = connectionObjectPath;
    d->service = settingsService;
    d->connectionInterface = new QDBusInterface(settingsService,
                                                d->path,
                                                QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                                                dbusConnection, parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
    QDBusReply< QNmSettingsMap > rep = d->connectionInterface->call(QLatin1String("GetSettings"));
    d->settingsMap = rep.value();
}

QNetworkManagerSettingsConnection::~QNetworkManagerSettingsConnection()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerSettingsConnection::isValid()
{
    return d->valid;
}

bool QNetworkManagerSettingsConnection::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;
    if(!dbusConnection.connect(d->service, d->path,
                           QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION), QLatin1String("Updated"),
                           this, SIGNAL(updated(QNmSettingsMap)))) {
        allOk = true;
    } else {
        QDBusError error = dbusConnection.lastError();
    }

    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForSettingsRemoved(QString)),
            this,SIGNAL(removed(QString)));

    if (!dbusConnection.connect(d->service, d->path,
                           QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION), QLatin1String("Removed"),
                           nmDBusHelper, SIGNAL(slotSettingsRemoved()))) {
        allOk = true;
    }

    return allOk;
}

QDBusInterface *QNetworkManagerSettingsConnection::connectionInterface() const
{
    return d->connectionInterface;
}

QNmSettingsMap QNetworkManagerSettingsConnection::getSettings()
{
    QDBusReply< QNmSettingsMap > rep = d->connectionInterface->call(QLatin1String("GetSettings"));
    d->settingsMap = rep.value();
    return d->settingsMap;
}

NMDeviceType QNetworkManagerSettingsConnection::getType()
{
    const QString devType =
        d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("type")).toString();

    if (devType == QLatin1String("802-3-ethernet"))
        return DEVICE_TYPE_802_3_ETHERNET;
    else if (devType == QLatin1String("802-11-wireless"))
        return DEVICE_TYPE_802_11_WIRELESS;
    else
        return DEVICE_TYPE_UNKNOWN;
}

bool QNetworkManagerSettingsConnection::isAutoConnect()
{
    const QVariant autoConnect =
        d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("autoconnect"));

    // NetworkManager default is to auto connect
    if (!autoConnect.isValid())
        return true;

    return autoConnect.toBool();
}

quint64 QNetworkManagerSettingsConnection::getTimestamp()
{
    return d->settingsMap.value(QLatin1String("connection"))
                         .value(QLatin1String("timestamp")).toUInt();
}

QString QNetworkManagerSettingsConnection::getId()
{
    return d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("id")).toString();
}

QString QNetworkManagerSettingsConnection::getUuid()
{
    const QString id = d->settingsMap.value(QLatin1String("connection"))
                                     .value(QLatin1String("uuid")).toString();

    // is no uuid, return the connection path
    return id.isEmpty() ? d->connectionInterface->path() : id;
}

QString QNetworkManagerSettingsConnection::getSsid()
{
    return d->settingsMap.value(QLatin1String("802-11-wireless"))
                         .value(QLatin1String("ssid")).toString();
}

QString QNetworkManagerSettingsConnection::getMacAddress()
{
    NMDeviceType type = getType();

    if (type == DEVICE_TYPE_802_3_ETHERNET) {
        return d->settingsMap.value(QLatin1String("802-3-ethernet"))
                             .value(QLatin1String("mac-address")).toString();
    } else if (type == DEVICE_TYPE_802_11_WIRELESS) {
        return d->settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("mac-address")).toString();
    } else {
        return QString();
    }
}

QStringList QNetworkManagerSettingsConnection::getSeenBssids()
{
    if (getType() == DEVICE_TYPE_802_11_WIRELESS) {
        return d->settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("seen-bssids")).toStringList();
    } else {
        return QStringList();
    }
}

class QNetworkManagerConnectionActivePrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerConnectionActive::QNetworkManagerConnectionActive( const QString &activeConnectionObjectPath, QObject *parent)
    : QObject(parent), nmDBusHelper(0)
{
    d = new QNetworkManagerConnectionActivePrivate();
    d->path = activeConnectionObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION),
                                                dbusConnection, parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerConnectionActive::~QNetworkManagerConnectionActive()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerConnectionActive::isValid()
{
    return d->valid;
}

bool QNetworkManagerConnectionActive::setConnections()
{
    if(!isValid() )
        return false;

    bool allOk = false;
    delete nmDBusHelper;
    nmDBusHelper = new QNmDBusHelper(this);
    connect(nmDBusHelper, SIGNAL(pathForPropertiesChanged(QString,QMap<QString,QVariant>)),
            this,SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)));
    if(dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION),
                              QLatin1String("PropertiesChanged"),
                              nmDBusHelper,SLOT(slotPropertiesChanged(QMap<QString,QVariant>))) )  {
        allOk = true;
    }

    return allOk;
}

QDBusInterface *QNetworkManagerConnectionActive::connectionInterface() const
{
    return d->connectionInterface;
}

QString QNetworkManagerConnectionActive::serviceName() const
{
    return d->connectionInterface->property("ServiceName").toString();
}

QDBusObjectPath QNetworkManagerConnectionActive::connection() const
{
    QVariant prop = d->connectionInterface->property("Connection");
    return prop.value<QDBusObjectPath>();
}

QDBusObjectPath QNetworkManagerConnectionActive::specificObject() const
{
    QVariant prop = d->connectionInterface->property("SpecificObject");
    return prop.value<QDBusObjectPath>();
}

QList<QDBusObjectPath> QNetworkManagerConnectionActive::devices() const
{
    QVariant prop = d->connectionInterface->property("Devices");
    return prop.value<QList<QDBusObjectPath> >();
}

quint32 QNetworkManagerConnectionActive::state() const
{
    return d->connectionInterface->property("State").toUInt();
}

bool QNetworkManagerConnectionActive::defaultRoute() const
{
    return d->connectionInterface->property("Default").toBool();
}

class QNetworkManagerIp4ConfigPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerIp4Config::QNetworkManagerIp4Config( const QString &deviceObjectPath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerIp4ConfigPrivate();
    d->path = deviceObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_IP4_CONFIG),
                                                dbusConnection, parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerIp4Config::~QNetworkManagerIp4Config()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerIp4Config::isValid()
{
    return d->valid;
}

QStringList QNetworkManagerIp4Config::domains() const
{
    return d->connectionInterface->property("Domains").toStringList();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
