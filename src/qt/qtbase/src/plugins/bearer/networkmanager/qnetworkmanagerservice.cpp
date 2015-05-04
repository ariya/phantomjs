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

#ifndef QT_NO_DBUS

#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"

QT_BEGIN_NAMESPACE


QNetworkManagerInterface::QNetworkManagerInterface(QObject *parent)
        : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                                 QLatin1String(NM_DBUS_PATH),
                                 NM_DBUS_INTERFACE,
                                 QDBusConnection::systemBus(),parent)
{
    if (!isValid()) {
        return;
    }

    PropertiesDBusInterface managerPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  QLatin1String(NM_DBUS_PATH),
                                                  DBUS_PROPERTIES_INTERFACE,
                                                  QDBusConnection::systemBus());
    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE);
    QDBusPendingReply<QVariantMap> propsReply
            = managerPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    } else {
        qWarning() << Q_FUNC_INFO << "propsReply"<<propsReply.error().message();
    }

    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = call(QLatin1String("GetDevices"));
    nmReply.waitForFinished();
    if (!nmReply.isError()) {
        devicesPathList = nmReply.value();
    } else {
        qWarning() << Q_FUNC_INFO <<"nmReply"<<nmReply.error().message();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
}

bool QNetworkManagerInterface::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));

    bool allOk = false;
    if (QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceAdded"),
                                this,SIGNAL(deviceAdded(QDBusObjectPath)))) {
        allOk = true;
    }
    if (QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceRemoved"),
                                  this,SIGNAL(deviceRemoved(QDBusObjectPath)))) {
        allOk = true;
    }

    return allOk;
}

QList <QDBusObjectPath> QNetworkManagerInterface::getDevices()
{
    if (devicesPathList.isEmpty()) {
        //qWarning() << "using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply = call(QLatin1String("GetDevices"));
        devicesPathList = reply.value();
    }
    return devicesPathList;
}

void QNetworkManagerInterface::activateConnection(QDBusObjectPath connectionPath,
                                                  QDBusObjectPath devicePath,
                                                  QDBusObjectPath specificObject)
{
    QDBusPendingCall pendingCall = asyncCall(QLatin1String("ActivateConnection"),
                                                                    QVariant::fromValue(connectionPath),
                                                                    QVariant::fromValue(devicePath),
                                                                    QVariant::fromValue(specificObject));

   QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingCall);
   connect(callWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                    this, SIGNAL(activationFinished(QDBusPendingCallWatcher*)));
}

void QNetworkManagerInterface::deactivateConnection(QDBusObjectPath connectionPath)
{
    asyncCall(QLatin1String("DeactivateConnection"), QVariant::fromValue(connectionPath));
}

bool QNetworkManagerInterface::wirelessEnabled()  const
{
    if (propertyMap.contains("WirelessEnabled"))
        return propertyMap.value("WirelessEnabled").toBool();
    return false;
}

bool QNetworkManagerInterface::wirelessHardwareEnabled()  const
{
    if (propertyMap.contains("WirelessHardwareEnabled"))
        return propertyMap.value("WirelessHardwareEnabled").toBool();
    return false;
}

QList <QDBusObjectPath> QNetworkManagerInterface::activeConnections() const
{
    if (propertyMap.contains("ActiveConnections")) {

        const QDBusArgument &dbusArgs = propertyMap.value("ActiveConnections").value<QDBusArgument>();
        QDBusObjectPath path;
        QList <QDBusObjectPath> list;

        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list.append(path);
        }
        dbusArgs.endArray();

        return list;
    }

    QList <QDBusObjectPath> list;
    list << QDBusObjectPath();
    return list;
}

QNetworkManagerInterface::NMState QNetworkManagerInterface::state()
{
    if (propertyMap.contains("State"))
        return static_cast<QNetworkManagerInterface::NMState>(propertyMap.value("State").toUInt());
    return QNetworkManagerInterface::NM_STATE_UNKNOWN;
}

QString QNetworkManagerInterface::version() const
{
    if (propertyMap.contains("Version"))
        return propertyMap.value("Version").toString();
    return QString();
}

void QNetworkManagerInterface::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());

        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_DEVICE_STATE_ACTIVATED
                || state == NM_DEVICE_STATE_DISCONNECTED
                || state == NM_DEVICE_STATE_UNAVAILABLE
                || state == NM_DEVICE_STATE_FAILED) {
                Q_EMIT propertiesChanged(map);
                Q_EMIT stateChanged(state);
            }
        } else if (i.key() == QStringLiteral("ActiveConnections")) {
            Q_EMIT propertiesChanged(map);
        }
    }
}

QNetworkManagerInterfaceAccessPoint::QNetworkManagerInterfaceAccessPoint(const QString &dbusPathName, QObject *parent)
        : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                  dbusPathName,
                  NM_DBUS_INTERFACE_ACCESS_POINT,
                  QDBusConnection::systemBus(),parent)
{
    if (!isValid()) {
        return;
    }
    PropertiesDBusInterface *accessPointPropertiesInterface = new PropertiesDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  dbusPathName,
                                                  DBUS_PROPERTIES_INTERFACE,
                                                  QDBusConnection::systemBus());

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT);
    QDBusPendingReply<QVariantMap> propsReply
            = accessPointPropertiesInterface->callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  dbusPathName,
                                  QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterfaceAccessPoint::~QNetworkManagerInterfaceAccessPoint()
{
}

quint32 QNetworkManagerInterfaceAccessPoint::flags() const
{
    if (propertyMap.contains("Flags"))
        return propertyMap.value("Flags").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::wpaFlags() const
{
    if (propertyMap.contains("WpaFlags"))
        return propertyMap.value("WpaFlags").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::rsnFlags() const
{
    if (propertyMap.contains("RsnFlags"))
        return propertyMap.value("RsnFlags").toUInt();
    return 0;
}

QString QNetworkManagerInterfaceAccessPoint::ssid() const
{
    if (propertyMap.contains("Ssid"))
        return propertyMap.value("Ssid").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceAccessPoint::frequency() const
{
    if (propertyMap.contains("Frequency"))
        return propertyMap.value("Frequency").toUInt();
    return 0;
}

QString QNetworkManagerInterfaceAccessPoint::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceAccessPoint::mode() const
{
    if (propertyMap.contains("Mode"))
        return propertyMap.value("Mode").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::maxBitrate() const
{
    if (propertyMap.contains("MaxBitrate"))
        return propertyMap.value("MaxBitrate").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::strength() const
{
    if (propertyMap.contains("Strength"))
        return propertyMap.value("Strength").toUInt();
    return 0;
}

void QNetworkManagerInterfaceAccessPoint::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
    }
}

QNetworkManagerInterfaceDevice::QNetworkManagerInterfaceDevice(const QString &deviceObjectPath, QObject *parent)
        : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                                 deviceObjectPath,
                                 NM_DBUS_INTERFACE_DEVICE,
                                 QDBusConnection::systemBus(),parent)
{

    if (!isValid()) {
        return;
    }
    PropertiesDBusInterface devicePropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  deviceObjectPath,
                                                  DBUS_PROPERTIES_INTERFACE,
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE);
    QDBusPendingReply<QVariantMap> propsReply
            = devicePropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  deviceObjectPath,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterfaceDevice::~QNetworkManagerInterfaceDevice()
{
}

QString QNetworkManagerInterfaceDevice::udi() const
{
    if (propertyMap.contains("Udi"))
        return propertyMap.value("Udi").toString();
    return QString();
}

QString QNetworkManagerInterfaceDevice::networkInterface() const
{
    if (propertyMap.contains("Interface"))
        return propertyMap.value("Interface").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDevice::ip4Address() const
{
    if (propertyMap.contains("Ip4Address"))
        return propertyMap.value("Ip4Address").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDevice::state() const
{
    if (propertyMap.contains("State"))
        return propertyMap.value("State").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDevice::deviceType() const
{
    if (propertyMap.contains("DeviceType"))
        return propertyMap.value("DeviceType").toUInt();
    return 0;
}

QDBusObjectPath QNetworkManagerInterfaceDevice::ip4config() const
{
    if (propertyMap.contains("Ip4Config"))
        return propertyMap.value("Ip4Config").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

void QNetworkManagerInterfaceDevice::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        if (i.key() == QStringLiteral("AvailableConnections")) { //Device
            const QDBusArgument &dbusArgs = i.value().value<QDBusArgument>();
            QDBusObjectPath path;
            QStringList paths;
            dbusArgs.beginArray();
            while (!dbusArgs.atEnd()) {
                dbusArgs >> path;
                paths << path.path();
            }
            dbusArgs.endArray();
            Q_EMIT connectionsChanged(paths);
        }
        propertyMap.insert(i.key(),i.value());
    }
    Q_EMIT propertiesChanged(map);
}

QNetworkManagerInterfaceDeviceWired::QNetworkManagerInterfaceDeviceWired(const QString &ifaceDevicePath, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                             ifaceDevicePath,
                             NM_DBUS_INTERFACE_DEVICE_WIRED,
                             QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }
    PropertiesDBusInterface deviceWiredPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                           ifaceDevicePath,
                                                           DBUS_PROPERTIES_INTERFACE,
                                                           QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceWiredPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  ifaceDevicePath,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterfaceDeviceWired::~QNetworkManagerInterfaceDeviceWired()
{
}

QString QNetworkManagerInterfaceDeviceWired::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDeviceWired::speed() const
{
    if (propertyMap.contains("Speed"))
        return propertyMap.value("Speed").toUInt();
    return 0;
}

bool QNetworkManagerInterfaceDeviceWired::carrier() const
{
    if (propertyMap.contains("Carrier"))
        return propertyMap.value("Carrier").toBool();
    return false;
}

QStringList QNetworkManagerInterfaceDeviceWired::availableConnections()
{
    QStringList list;
    if (propertyMap.contains("AvailableConnections")) {
        const QDBusArgument &dbusArgs = propertyMap.value("Carrier").value<QDBusArgument>();
        QDBusObjectPath path;
        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list << path.path();
        }
        dbusArgs.endArray();
    }

    return list;
}

void QNetworkManagerInterfaceDeviceWired::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("Carrier")) {
            Q_EMIT carrierChanged(i.value().toBool());
        }
    }
    Q_EMIT propertiesChanged(map);
}

QNetworkManagerInterfaceDeviceWireless::QNetworkManagerInterfaceDeviceWireless(const QString &ifaceDevicePath, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                             ifaceDevicePath,
                             NM_DBUS_INTERFACE_DEVICE_WIRELESS,
                             QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }

    interfacePath = ifaceDevicePath;
    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = call(QLatin1String("GetAccessPoints"));

    if (!nmReply.isError()) {
        accessPointsList = nmReply.value();
    }

    PropertiesDBusInterface deviceWirelessPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  interfacePath,
                                                  DBUS_PROPERTIES_INTERFACE,
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceWirelessPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                 interfacePath,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));

    QDBusPendingReply<QList<QDBusObjectPath> > reply
            = asyncCall(QLatin1String("GetAccessPoints"));

    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(reply);
    connect(callWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(accessPointsFinished(QDBusPendingCallWatcher*)));
}

QNetworkManagerInterfaceDeviceWireless::~QNetworkManagerInterfaceDeviceWireless()
{
}

void QNetworkManagerInterfaceDeviceWireless::slotAccessPointAdded(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        Q_EMIT accessPointAdded(path.path());
}

void QNetworkManagerInterfaceDeviceWireless::slotAccessPointRemoved(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        Q_EMIT accessPointRemoved(path.path());
}

bool QNetworkManagerInterfaceDeviceWireless::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection dbusConnection = QDBusConnection::systemBus();
    bool allOk = true;

    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                               interfacePath,
                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                                QLatin1String("AccessPointAdded"),
                                this, SLOT(slotAccessPointAdded(QDBusObjectPath)))) {
        allOk = false;
    }


    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              interfacePath,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("AccessPointRemoved"),
                              this, SLOT(slotAccessPointRemoved(QDBusObjectPath)))) {
        allOk = false;
    }

    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                               interfacePath,
                               QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                               QLatin1String("ScanDone"),
                               this, SLOT(scanIsDone()))) {
        allOk = false;
    }
    return allOk;
}

void QNetworkManagerInterfaceDeviceWireless::accessPointsFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QList<QDBusObjectPath> > reply(*watcher);
    watcher->deleteLater();
    if (!reply.isError()) {
        accessPointsList = reply.value();
    }

    for (int i = 0; i < accessPointsList.size(); i++) {
        Q_EMIT accessPointAdded(accessPointsList.at(i).path());
    }
}

QList <QDBusObjectPath> QNetworkManagerInterfaceDeviceWireless::getAccessPoints()
{
    if (accessPointsList.isEmpty()) {
        //qWarning() << "Using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply
                = call(QLatin1String("GetAccessPoints"));
        accessPointsList = reply.value();
    }
    return accessPointsList;
}

QString QNetworkManagerInterfaceDeviceWireless::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDeviceWireless::mode() const
{
    if (propertyMap.contains("Mode"))
        return propertyMap.value("Mode").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDeviceWireless::bitrate() const
{
    if (propertyMap.contains("Bitrate"))
        return propertyMap.value("Bitrate").toUInt();
    return 0;
}

QDBusObjectPath QNetworkManagerInterfaceDeviceWireless::activeAccessPoint() const
{
    if (propertyMap.contains("ActiveAccessPoint"))
        return propertyMap.value("ActiveAccessPoint").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

quint32 QNetworkManagerInterfaceDeviceWireless::wirelessCapabilities() const
{
    if (propertyMap.contains("WirelelessCapabilities"))
        return propertyMap.value("WirelelessCapabilities").toUInt();
    return 0;
}

void QNetworkManagerInterfaceDeviceWireless::scanIsDone()
{
    Q_EMIT scanDone();
}

void QNetworkManagerInterfaceDeviceWireless::requestScan()
{
    asyncCall(QLatin1String("RequestScan"));
}

void QNetworkManagerInterfaceDeviceWireless::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("ActiveAccessPoint")) { //DeviceWireless
            Q_EMIT propertiesChanged(map);
        }
    }
}

QNetworkManagerInterfaceDeviceModem::QNetworkManagerInterfaceDeviceModem(const QString &ifaceDevicePath, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                             ifaceDevicePath,
                             NM_DBUS_INTERFACE_DEVICE_MODEM,
                             QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }
    PropertiesDBusInterface deviceModemPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  ifaceDevicePath,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_MODEM);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceModemPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  ifaceDevicePath,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_MODEM),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterfaceDeviceModem::~QNetworkManagerInterfaceDeviceModem()
{
}

QNetworkManagerInterfaceDeviceModem::ModemCapabilities QNetworkManagerInterfaceDeviceModem::modemCapabilities() const
{
    if (propertyMap.contains("ModemCapabilities"))
        return static_cast<QNetworkManagerInterfaceDeviceModem::ModemCapabilities>(propertyMap.value("ModemCapabilities").toUInt());
    return QNetworkManagerInterfaceDeviceModem::None;
}

QNetworkManagerInterfaceDeviceModem::ModemCapabilities QNetworkManagerInterfaceDeviceModem::currentCapabilities() const
{
    if (propertyMap.contains("CurrentCapabilities"))
        return static_cast<QNetworkManagerInterfaceDeviceModem::ModemCapabilities>(propertyMap.value("CurrentCapabilities").toUInt());
    return QNetworkManagerInterfaceDeviceModem::None;
}

void QNetworkManagerInterfaceDeviceModem::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
    }
    Q_EMIT propertiesChanged(map);
}


QNetworkManagerSettings::QNetworkManagerSettings(const QString &settingsService, QObject *parent)
        : QDBusAbstractInterface(settingsService,
                                 NM_DBUS_PATH_SETTINGS,
                                 NM_DBUS_IFACE_SETTINGS,
                                 QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }
    interfacePath = settingsService;
    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = call(QLatin1String("ListConnections"));

    if (!nmReply.isError()) {
        connectionsList = nmReply.value();
    }
}

QNetworkManagerSettings::~QNetworkManagerSettings()
{
}

bool QNetworkManagerSettings::setConnections()
{
    bool allOk = true;
    if (!QDBusConnection::systemBus().connect(interfacePath,
                                             QLatin1String(NM_DBUS_PATH_SETTINGS),
                                             QLatin1String(NM_DBUS_IFACE_SETTINGS),
                                             QLatin1String("NewConnection"),
                                             this, SIGNAL(newConnection(QDBusObjectPath)))) {
        allOk = false;
        qWarning() << Q_FUNC_INFO << "NewConnection could not be connected";
    }

    return allOk;
}

QList <QDBusObjectPath> QNetworkManagerSettings::listConnections()
{
    if (connectionsList.isEmpty()) {
        //qWarning() << "Using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply
                = call(QLatin1String("ListConnections"));
        connectionsList = reply.value();
    }
    return connectionsList;
}


QString QNetworkManagerSettings::getConnectionByUuid(const QString &uuid)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(uuid);
    QDBusReply<QDBusObjectPath > reply = callWithArgumentList(QDBus::Block,QLatin1String("GetConnectionByUuid"), argumentList);
    return reply.value().path();
}

QNetworkManagerSettingsConnection::QNetworkManagerSettingsConnection(const QString &settingsService, const QString &connectionObjectPath, QObject *parent)
    : QDBusAbstractInterface(settingsService,
                             connectionObjectPath,
                             NM_DBUS_IFACE_SETTINGS_CONNECTION,
                             QDBusConnection::systemBus(), parent)
{
    qDBusRegisterMetaType<QNmSettingsMap>();
    if (!isValid()) {
        return;
    }
    interfacepath = connectionObjectPath;
    QDBusPendingReply<QNmSettingsMap> nmReply
            = call(QLatin1String("GetSettings"));
    if (!nmReply.isError()) {
        settingsMap = nmReply.value();
    }
}

QNetworkManagerSettingsConnection::~QNetworkManagerSettingsConnection()
{
}

bool QNetworkManagerSettingsConnection::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection dbusConnection = QDBusConnection::systemBus();
    bool allOk = true;
    if (!dbusConnection.connect(service(),
                               interfacepath,
                               QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                               QLatin1String("Updated"),
                               this, SIGNAL(updated()))) {
        allOk = false;
    }

    if (!dbusConnection.connect(service(),
                               interfacepath,
                               QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                               QLatin1String("Removed"),
                               this, SIGNAL(slotSettingsRemoved()))) {
        allOk = false;
    }
    return allOk;
}

void QNetworkManagerSettingsConnection::slotSettingsRemoved()
{
    Q_EMIT removed(interfacepath);
}

QNmSettingsMap QNetworkManagerSettingsConnection::getSettings()
{
    if (settingsMap.isEmpty()) {
        //qWarning() << "Using blocking call!";
        QDBusReply<QNmSettingsMap> reply = call(QLatin1String("GetSettings"));
        settingsMap = reply.value();
    }
    return settingsMap;
}

NMDeviceType QNetworkManagerSettingsConnection::getType()
{
    const QString devType =
        settingsMap.value(QLatin1String("connection")).value(QLatin1String("type")).toString();

    if (devType == QLatin1String("802-3-ethernet"))
        return DEVICE_TYPE_ETHERNET;
    else if (devType == QLatin1String("802-11-wireless"))
        return DEVICE_TYPE_WIFI;
    else if (devType == QLatin1String("gsm"))
        return DEVICE_TYPE_MODEM;
    else
        return DEVICE_TYPE_UNKNOWN;
}

bool QNetworkManagerSettingsConnection::isAutoConnect()
{
    const QVariant autoConnect =
        settingsMap.value(QLatin1String("connection")).value(QLatin1String("autoconnect"));

    // NetworkManager default is to auto connect
    if (!autoConnect.isValid())
        return true;

    return autoConnect.toBool();
}

quint64 QNetworkManagerSettingsConnection::getTimestamp()
{
    return settingsMap.value(QLatin1String("connection"))
                         .value(QLatin1String("timestamp")).toUInt();
}

QString QNetworkManagerSettingsConnection::getId()
{
    return settingsMap.value(QLatin1String("connection")).value(QLatin1String("id")).toString();
}

QString QNetworkManagerSettingsConnection::getUuid()
{
    const QString id = settingsMap.value(QLatin1String("connection"))
                                     .value(QLatin1String("uuid")).toString();

    // is no uuid, return the connection path
    return id.isEmpty() ? path() : id;
}

QString QNetworkManagerSettingsConnection::getSsid()
{
    return settingsMap.value(QLatin1String("802-11-wireless"))
                         .value(QLatin1String("ssid")).toString();
}

QString QNetworkManagerSettingsConnection::getMacAddress()
{
    NMDeviceType type = getType();

    if (type == DEVICE_TYPE_ETHERNET) {
        return settingsMap.value(QLatin1String("802-3-ethernet"))
                             .value(QLatin1String("mac-address")).toString();
    } else if (type == DEVICE_TYPE_WIFI) {
        return settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("mac-address")).toString();
    } else {
        return QString();
    }
}

QStringList QNetworkManagerSettingsConnection::getSeenBssids()
{
    if (getType() == DEVICE_TYPE_WIFI) {
        return settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("seen-bssids")).toStringList();
    } else {
        return QStringList();
    }
}

QNetworkManagerConnectionActive::QNetworkManagerConnectionActive(const QString &activeConnectionObjectPath, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                             activeConnectionObjectPath,
                             NM_DBUS_INTERFACE_ACTIVE_CONNECTION,
                             QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }
    PropertiesDBusInterface connectionActivePropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  activeConnectionObjectPath,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus());


    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION);
    QDBusPendingReply<QVariantMap> propsReply
            = connectionActivePropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    } else {
        qWarning() << propsReply.error().message();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  activeConnectionObjectPath,
                                  QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerConnectionActive::~QNetworkManagerConnectionActive()
{
}

QDBusObjectPath QNetworkManagerConnectionActive::connection() const
{
    if (propertyMap.contains("Connection"))
        return propertyMap.value("Connection").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

QDBusObjectPath QNetworkManagerConnectionActive::specificObject() const
{
    if (propertyMap.contains("SpecificObject"))
        return propertyMap.value("SpecificObject").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

QStringList QNetworkManagerConnectionActive::devices() const
{
    QStringList list;
    if (propertyMap.contains("Devices")) {
        const QDBusArgument &dbusArgs = propertyMap.value("Devices").value<QDBusArgument>();
        QDBusObjectPath path;

        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list.append(path.path());
        }
        dbusArgs.endArray();
    }
    return list;
}

quint32 QNetworkManagerConnectionActive::state() const
{
    if (propertyMap.contains("State"))
        return propertyMap.value("State").toUInt();
    return 0;
}

bool QNetworkManagerConnectionActive::defaultRoute() const
{
    if (propertyMap.contains("Default"))
        return propertyMap.value("Default").toBool();
    return false;
}

bool QNetworkManagerConnectionActive::default6Route() const
{
    if (propertyMap.contains("Default6"))
        return propertyMap.value("Default6").toBool();
    return false;
}

void QNetworkManagerConnectionActive::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED
                || state == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED) {
                Q_EMIT propertiesChanged(map);
            }
        }
    }
}

QNetworkManagerIp4Config::QNetworkManagerIp4Config( const QString &deviceObjectPath, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE),
                             deviceObjectPath,
                             NM_DBUS_INTERFACE_IP4_CONFIG,
                             QDBusConnection::systemBus(), parent)
{
    if (!isValid()) {
        return;
    }
}

QNetworkManagerIp4Config::~QNetworkManagerIp4Config()
{
}

QStringList QNetworkManagerIp4Config::domains() const
{
    return property("Domains").toStringList();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
