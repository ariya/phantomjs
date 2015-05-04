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

#include "qnetworkmanagerengine.h"
#include "qnetworkmanagerservice.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtNetwork/qnetworksession.h>

#include <QtCore/qdebug.h>

#include <QtDBus>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include "../linux_common/qofonoservice_linux_p.h"

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QNetworkManagerEngine::QNetworkManagerEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    managerInterface(NULL),
    systemSettings(NULL),
    ofonoManager(NULL),
    nmAvailable(false)
{
    qDBusRegisterMetaType<QNmSettingsMap>();

    nmWatcher = new QDBusServiceWatcher(NM_DBUS_SERVICE,QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(nmWatcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(nmRegistered(QString)));
    connect(nmWatcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(nmUnRegistered(QString)));

    ofonoWatcher = new QDBusServiceWatcher("org.ofono",QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(ofonoWatcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(ofonoRegistered(QString)));
    connect(ofonoWatcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(ofonoUnRegistered(QString)));

    if (QDBusConnection::systemBus().interface()->isServiceRegistered("org.ofono"))
        ofonoRegistered();

    if (QDBusConnection::systemBus().interface()->isServiceRegistered(NM_DBUS_SERVICE))
        nmRegistered();
}

QNetworkManagerEngine::~QNetworkManagerEngine()
{
    qDeleteAll(connections);
    connections.clear();
    qDeleteAll(accessPoints);
    accessPoints.clear();
    qDeleteAll(wirelessDevices);
    wirelessDevices.clear();
    qDeleteAll(activeConnectionsList);
    activeConnectionsList.clear();
    qDeleteAll(interfaceDevices);
    interfaceDevices.clear();

    connectionInterfaces.clear();

    qDeleteAll(ofonoContextManagers);
    ofonoContextManagers.clear();

    qDeleteAll(wiredDevices);
    wiredDevices.clear();
}

void QNetworkManagerEngine::initialize()
{
    if (nmAvailable)
        setupConfigurations();
}

void QNetworkManagerEngine::setupConfigurations()
{
    QMutexLocker locker(&mutex);
    // Get active connections.
    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {

        QNetworkManagerConnectionActive *activeConnection =
                new QNetworkManagerConnectionActive(acPath.path(),this);
        activeConnectionsList.insert(acPath.path(), activeConnection);
        connect(activeConnection, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
                this, SLOT(activeConnectionPropertiesChanged(QMap<QString,QVariant>)));

        QStringList devices = activeConnection->devices();
        if (!devices.isEmpty()) {
            QNetworkManagerInterfaceDevice device(devices.at(0),this);
            connectionInterfaces.insert(activeConnection->connection().path(),device.networkInterface());
        }
    }

        // Get current list of access points.
    foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
        locker.unlock();
        deviceAdded(devicePath); //add all accesspoints
        locker.relock();
    }

    // Get connections.
    foreach (const QDBusObjectPath &settingsPath, systemSettings->listConnections()) {
        locker.unlock();
        if (!hasIdentifier(settingsPath.path()))
            newConnection(settingsPath, systemSettings); //add system connection configs
        locker.relock();
    }

    Q_EMIT updateCompleted();
}

bool QNetworkManagerEngine::networkManagerAvailable() const
{
    return nmAvailable;
}

QString QNetworkManagerEngine::getInterfaceFromId(const QString &settingsPath)
{
    return connectionInterfaces.value(settingsPath);
}

bool QNetworkManagerEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);
    return accessPointConfigurations.contains(id);
}

void QNetworkManagerEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);

    if (!connection)
        return;

    NMDeviceType connectionType = connection->getType();

    QString dbusDevicePath;
    const QString settingsPath = connection->path();
    QString specificPath = configuredAccessPoints.key(settingsPath);

    if (isConnectionActive(settingsPath))
        return;

    QHashIterator<QString, QNetworkManagerInterfaceDevice*> i(interfaceDevices);
    while (i.hasNext()) {
        i.next();
        if (i.value()->deviceType() == DEVICE_TYPE_ETHERNET &&
            connectionType == DEVICE_TYPE_ETHERNET) {
            dbusDevicePath = i.key();
            break;
        } else if (i.value()->deviceType() == DEVICE_TYPE_WIFI &&
                   connectionType == DEVICE_TYPE_WIFI) {
            dbusDevicePath = i.key();
            break;
        } else if (i.value()->deviceType() == DEVICE_TYPE_MODEM &&
                connectionType == DEVICE_TYPE_MODEM) {
            dbusDevicePath = i.key();
            break;
        }
    }

    if (specificPath.isEmpty())
        specificPath = "/";

    managerInterface->activateConnection(QDBusObjectPath(settingsPath),
                                  QDBusObjectPath(dbusDevicePath), QDBusObjectPath(specificPath));
}

void QNetworkManagerEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);
    QNmSettingsMap map = connection->getSettings();
    bool connectionAutoconnect = map.value("connection").value("autoconnect",true).toBool(); //if not present is true !!
    if (connectionAutoconnect) { //autoconnect connections will simply be reconnected by nm
        emit connectionError(id, QBearerEngineImpl::OperationNotSupported);
        return;
    }

    QHashIterator<QString, QNetworkManagerConnectionActive*> i(activeConnectionsList);
    while (i.hasNext()) {
        i.next();
        if (id == i.value()->connection().path() && accessPointConfigurations.contains(id)) {
            managerInterface->deactivateConnection(QDBusObjectPath(i.key()));
            break;
        }
    }
}

void QNetworkManagerEngine::requestUpdate()
{
    if (managerInterface && managerInterface->wirelessEnabled()) {
        QHashIterator<QString, QNetworkManagerInterfaceDeviceWireless *> i(wirelessDevices);
        while (i.hasNext()) {
            i.next();
            i.value()->requestScan();
        }
    }
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::scanFinished()
{
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::interfacePropertiesChanged(const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);
    QMapIterator<QString, QVariant> i(properties);
    while (i.hasNext()) {
        i.next();

        if (i.key() == QLatin1String("ActiveConnections")) {
            // Active connections changed, update configurations.

            QList<QDBusObjectPath> activeConnections =
                qdbus_cast<QList<QDBusObjectPath> >(i.value().value<QDBusArgument>());

            QStringList identifiers = accessPointConfigurations.keys();
            QStringList priorActiveConnections = activeConnectionsList.keys();

            foreach (const QDBusObjectPath &acPath, activeConnections) {
                priorActiveConnections.removeOne(acPath.path());
                QNetworkManagerConnectionActive *activeConnection =
                    activeConnectionsList.value(acPath.path());

                if (!activeConnection) {
                    activeConnection = new QNetworkManagerConnectionActive(acPath.path(),this);
                    activeConnectionsList.insert(acPath.path(), activeConnection);

                    connect(activeConnection, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
                            this, SLOT(activeConnectionPropertiesChanged(QMap<QString,QVariant>)));
                }

                const QString id = activeConnection->connection().path();

                identifiers.removeOne(id);

                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
                if (ptr) {
                    ptr->mutex.lock();
                    if (activeConnection->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
                            (ptr->state & QNetworkConfiguration::Active) != QNetworkConfiguration::Active) {

                        ptr->state |= QNetworkConfiguration::Active;

                        if (activeConnectionsList.value(id) && activeConnectionsList.value(id)->defaultRoute()
                                && managerInterface->state() < QNetworkManagerInterface::NM_STATE_CONNECTED_GLOBAL) {
                            ptr->purpose = QNetworkConfiguration::PrivatePurpose;
                        }
                        ptr->mutex.unlock();

                        locker.unlock();
                        emit configurationChanged(ptr);
                        locker.relock();
                    } else {
                        ptr->mutex.unlock();
                    }
                }
            }

            while (!priorActiveConnections.isEmpty())
                delete activeConnectionsList.take(priorActiveConnections.takeFirst());

            while (!identifiers.isEmpty()) {
                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(identifiers.takeFirst());

                ptr->mutex.lock();
                if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
                    QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
                    ptr->state = (flag | QNetworkConfiguration::Discovered);
                    ptr->mutex.unlock();

                    locker.unlock();
                    emit configurationChanged(ptr);
                    locker.relock();
                } else {
                    ptr->mutex.unlock();
                }
            }
        }
    }
}

void QNetworkManagerEngine::activeConnectionPropertiesChanged(const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(properties)

    QNetworkManagerConnectionActive *activeConnection = qobject_cast<QNetworkManagerConnectionActive *>(sender());

    if (!activeConnection)
        return;

    const QString id = activeConnection->connection().path();

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr) {
        if (properties.contains(QStringLiteral("State"))) {
            ptr->mutex.lock();
            if (properties.value("State").toUInt() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
                QStringList devices = activeConnection->devices();
                if (!devices.isEmpty()) {
                    QNetworkManagerInterfaceDevice device(devices.at(0),this);
                    connectionInterfaces.insert(id,device.networkInterface());
                }

                ptr->state |= QNetworkConfiguration::Active;
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            } else {
                connectionInterfaces.remove(id);
                ptr->mutex.unlock();
            }
        }
    }
}

void QNetworkManagerEngine::deviceConnectionsChanged(const QStringList &connectionsList)
{
    QMutexLocker locker(&mutex);
    for (int i = 0; i < connections.count(); ++i) {
        if (connectionsList.contains(connections.at(i)->path()))
            continue;

        const QString settingsPath = connections.at(i)->path();

        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.value(settingsPath);
        ptr->mutex.lock();
        QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
        ptr->state = (flag | QNetworkConfiguration::Discovered);
        ptr->mutex.unlock();

        locker.unlock();
        emit configurationChanged(ptr);
        locker.relock();
        Q_EMIT updateCompleted();
    }
}

void QNetworkManagerEngine::deviceAdded(const QDBusObjectPath &path)
{
    QNetworkManagerInterfaceDevice *iDevice;
    iDevice = new QNetworkManagerInterfaceDevice(path.path(),this);
    connect(iDevice,SIGNAL(connectionsChanged(QStringList)),
            this,SLOT(deviceConnectionsChanged(QStringList)));

    interfaceDevices.insert(path.path(),iDevice);
    if (iDevice->deviceType() == DEVICE_TYPE_WIFI) {
        QNetworkManagerInterfaceDeviceWireless *wirelessDevice =
            new QNetworkManagerInterfaceDeviceWireless(iDevice->path(),this);

        connect(wirelessDevice, SIGNAL(accessPointAdded(QString)),
                this, SLOT(newAccessPoint(QString)));
        connect(wirelessDevice, SIGNAL(accessPointRemoved(QString)),
                this, SLOT(removeAccessPoint(QString)));
        connect(wirelessDevice,SIGNAL(scanDone()),this,SLOT(scanFinished()));
        wirelessDevice->setConnections();

        wirelessDevices.insert(path.path(), wirelessDevice);
    }

    if (iDevice->deviceType() == DEVICE_TYPE_ETHERNET) {
        QNetworkManagerInterfaceDeviceWired *wiredDevice =
                new QNetworkManagerInterfaceDeviceWired(iDevice->path(),this);
        connect(wiredDevice,SIGNAL(carrierChanged(bool)),this,SLOT(wiredCarrierChanged(bool)));
        wiredDevices.insert(iDevice->path(), wiredDevice);
    }
}

void QNetworkManagerEngine::deviceRemoved(const QDBusObjectPath &path)
{
    QMutexLocker locker(&mutex);

    if (interfaceDevices.contains(path.path())) {
        locker.unlock();
        delete interfaceDevices.take(path.path());
        locker.relock();
    }
    if (wirelessDevices.contains(path.path())) {
        locker.unlock();
        delete wirelessDevices.take(path.path());
        locker.relock();
    }
    if (wiredDevices.contains(path.path())) {
        locker.unlock();
        delete wiredDevices.take(path.path());
        locker.relock();
    }
}

void QNetworkManagerEngine::wiredCarrierChanged(bool carrier)
{
    QNetworkManagerInterfaceDeviceWired *deviceWired = qobject_cast<QNetworkManagerInterfaceDeviceWired *>(sender());
    if (!deviceWired)
        return;
    QMutexLocker locker(&mutex);
    foreach (const QDBusObjectPath &settingsPath, systemSettings->listConnections()) {
        for (int i = 0; i < connections.count(); ++i) {
            QNetworkManagerSettingsConnection *connection = connections.at(i);
            if (connection->getType() == DEVICE_TYPE_ETHERNET
                    && settingsPath.path() == connection->path()) {
                QNetworkConfigurationPrivatePointer ptr =
                        accessPointConfigurations.value(settingsPath.path());

                if (ptr) {
                    ptr->mutex.lock();
                    if (carrier)
                        ptr->state |= QNetworkConfiguration::Discovered;
                    else
                        ptr->state = QNetworkConfiguration::Defined;
                    ptr->mutex.unlock();
                    locker.unlock();
                    emit configurationChanged(ptr);
                    return;
                }
            }
        }
    }
}

void QNetworkManagerEngine::newConnection(const QDBusObjectPath &path,
                                          QNetworkManagerSettings *settings)
{
    QMutexLocker locker(&mutex);
    if (!settings)
        settings = qobject_cast<QNetworkManagerSettings *>(sender());

    if (!settings) {
        return;
    }

    QNetworkManagerSettingsConnection *connection =
        new QNetworkManagerSettingsConnection(settings->service(),
                                              path.path(),this);
    const QString settingsPath = connection->path();
    if (accessPointConfigurations.contains(settingsPath)) {
        return;
    }

    connections.append(connection);

    connect(connection,SIGNAL(removed(QString)),this,SLOT(removeConnection(QString)));
    connect(connection,SIGNAL(updated()),this,SLOT(updateConnection()));
    connection->setConnections();

    NMDeviceType deviceType = connection->getType();

    if (deviceType == DEVICE_TYPE_WIFI) {
        QString apPath;
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connection->getSsid() == accessPoints.at(i)->ssid()) {
                // remove the corresponding accesspoint from configurations
                apPath = accessPoints.at(i)->path();
                QNetworkConfigurationPrivatePointer ptr
                        = accessPointConfigurations.take(apPath);
                if (ptr) {
                    locker.unlock();
                    emit configurationRemoved(ptr);
                    locker.relock();
                }
            }
        }
        if (!configuredAccessPoints.contains(settingsPath))
            configuredAccessPoints.insert(apPath,settingsPath);
    }

    QNetworkConfigurationPrivate *cpPriv =
        parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    if (isConnectionActive(settingsPath))
        cpPriv->state |= QNetworkConfiguration::Active;

    if (deviceType == DEVICE_TYPE_ETHERNET) {
        QHashIterator<QString, QNetworkManagerInterfaceDevice*> i(interfaceDevices);
        while (i.hasNext()) {
             i.next();
             if (i.value()->deviceType() == deviceType) {
                QNetworkManagerInterfaceDeviceWired *wiredDevice
                        = wiredDevices.value(i.value()->path());
                 if (wiredDevice->carrier()) {
                     cpPriv->state |= QNetworkConfiguration::Discovered;
                 }
             }
         }
     }

    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    accessPointConfigurations.insert(ptr->id, ptr);
    locker.unlock();
    emit configurationAdded(ptr);
}

bool QNetworkManagerEngine::isConnectionActive(const QString &settingsPath)
{
    QHashIterator<QString, QNetworkManagerConnectionActive*> i(activeConnectionsList);
    while (i.hasNext()) {
        i.next();
        if (i.value()->connection().path() == settingsPath) {
            if (i.value()->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATING
                    || i.value()->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
                return true;
            } else {
                break;
            }
        }
    }

    QNetworkManagerSettingsConnection *settingsConnection = connectionFromId(settingsPath);
    if (settingsConnection->getType() == DEVICE_TYPE_MODEM) {
        return isActiveContext(settingsConnection->path());
    }

    return false;
}

void QNetworkManagerEngine::removeConnection(const QString &path)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());

    if (!connection)
        return;

    connection->deleteLater();
    connections.removeAll(connection);

    const QString id = path;

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(id);

    if (ptr) {
        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }
    // add base AP back into configurations
    QMapIterator<QString, QString> i(configuredAccessPoints);
    while (i.hasNext()) {
        i.next();
        if (i.value() == path) {
            configuredAccessPoints.remove(i.key());
            newAccessPoint(i.key());
        }
    }
}

void QNetworkManagerEngine::updateConnection()
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;
    const QString settingsPath = connection->path();

    QNetworkConfigurationPrivate *cpPriv = parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        if (activeConnection.connection().path() == settingsPath &&
            activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
            cpPriv->state |= QNetworkConfiguration::Active;
            break;
        }
    }

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(cpPriv->id);

    ptr->mutex.lock();

    ptr->isValid = cpPriv->isValid;
    ptr->name = cpPriv->name;
    ptr->id = cpPriv->id;
    ptr->state = cpPriv->state;

    ptr->mutex.unlock();

    locker.unlock();
    emit configurationChanged(ptr);
    locker.relock();
    delete cpPriv;
}

void QNetworkManagerEngine::activationFinished(QDBusPendingCallWatcher *watcher)
{
    QMutexLocker locker(&mutex);
    QDBusPendingReply<QDBusObjectPath> reply(*watcher);
    watcher->deleteLater();

    if (!reply.isError()) {
        QDBusObjectPath result = reply.value();

        QNetworkManagerConnectionActive activeConnection(result.path());

        const QString id = activeConnection.connection().path();

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
        if (ptr) {
            ptr->mutex.lock();
            if (activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
                ptr->state != QNetworkConfiguration::Active) {
                ptr->state |= QNetworkConfiguration::Active;
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            } else {
                ptr->mutex.unlock();
            }
        }
    }
}

void QNetworkManagerEngine::newAccessPoint(const QString &path)
{
    QMutexLocker locker(&mutex);
    QNetworkManagerInterfaceAccessPoint *accessPoint =
        new QNetworkManagerInterfaceAccessPoint(path,this);

    bool okToAdd = true;
    for (int i = 0; i < accessPoints.count(); ++i) {
        if (accessPoints.at(i)->path() == path) {
            okToAdd = false;
        }
    }
    if (okToAdd) {
        accessPoints.append(accessPoint);
    }
    // Check if configuration exists for connection.
    if (!accessPoint->ssid().isEmpty()) {

        for (int i = 0; i < connections.count(); ++i) {
            QNetworkManagerSettingsConnection *connection = connections.at(i);
            const QString settingsPath = connection->path();

            if (accessPoint->ssid() == connection->getSsid()) {
                if (!configuredAccessPoints.contains(path)) {
                    configuredAccessPoints.insert(path,settingsPath);
                }

                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(settingsPath);
                ptr->mutex.lock();
                QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
                ptr->state = (flag | QNetworkConfiguration::Discovered);

                if (isConnectionActive(settingsPath))
                    ptr->state = (flag | QNetworkConfiguration::Active);
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                return;
            }
        }
    }

    // New access point.
    QNetworkConfigurationPrivatePointer ptr(new QNetworkConfigurationPrivate);

    ptr->name = accessPoint->ssid();
    ptr->isValid = true;
    ptr->id = path;
    ptr->type = QNetworkConfiguration::InternetAccessPoint;
    ptr->purpose = QNetworkConfiguration::PublicPurpose;
    ptr->state = QNetworkConfiguration::Undefined;
    ptr->bearerType = QNetworkConfiguration::BearerWLAN;

    accessPointConfigurations.insert(ptr->id, ptr);

    locker.unlock();
    emit configurationAdded(ptr);
}

void QNetworkManagerEngine::removeAccessPoint(const QString &path)
{
    QMutexLocker locker(&mutex);
    for (int i = 0; i < accessPoints.count(); ++i) {
        QNetworkManagerInterfaceAccessPoint *accessPoint = accessPoints.at(i);
        if (accessPoint->path() == path) {
            accessPoints.removeOne(accessPoint);

            if (configuredAccessPoints.contains(accessPoint->path())) {
                // find connection and change state to Defined
                configuredAccessPoints.remove(accessPoint->path());

                for (int i = 0; i < connections.count(); ++i) {
                    QNetworkManagerSettingsConnection *connection = connections.at(i);

                    if (accessPoint->ssid() == connection->getSsid()) {//might not have bssid yet
                        const QString settingsPath = connection->path();
                        const QString connectionId = settingsPath;

                        QNetworkConfigurationPrivatePointer ptr =
                            accessPointConfigurations.value(connectionId);
                        ptr->mutex.lock();
                        ptr->state = QNetworkConfiguration::Defined;
                        ptr->mutex.unlock();

                        locker.unlock();
                        emit configurationChanged(ptr);
                        locker.relock();
                        break;
                    }
                }
            } else {
                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.take(path);

                if (ptr) {
                    locker.unlock();
                    emit configurationRemoved(ptr);
                    locker.relock();
                }
            }
            delete accessPoint;
            break;
        }
    }
}

QNetworkConfigurationPrivate *QNetworkManagerEngine::parseConnection(const QString &settingsPath,
                                                                     const QNmSettingsMap &map)
{
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivate *cpPriv = new QNetworkConfigurationPrivate;
    cpPriv->name = map.value("connection").value("id").toString();
    cpPriv->isValid = true;
    cpPriv->id = settingsPath;
    cpPriv->type = QNetworkConfiguration::InternetAccessPoint;

    cpPriv->purpose = QNetworkConfiguration::PublicPurpose;

    cpPriv->state = QNetworkConfiguration::Defined;
    const QString connectionType = map.value("connection").value("type").toString();

    if (connectionType == QLatin1String("802-3-ethernet")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;

        foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
            QNetworkManagerInterfaceDevice device(devicePath.path(),this);
            if (device.deviceType() == DEVICE_TYPE_ETHERNET) {
                QNetworkManagerInterfaceDeviceWired *wiredDevice = wiredDevices.value(device.path());
                if (wiredDevice->carrier()) {
                    cpPriv->state |= QNetworkConfiguration::Discovered;
                    break;
                }
            }
        }
    } else if (connectionType == QLatin1String("802-11-wireless")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;

        const QString connectionSsid = map.value("802-11-wireless").value("ssid").toString();
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connectionSsid == accessPoints.at(i)->ssid()
                    && map.value("802-11-wireless").value("seen-bssids").toStringList().contains(accessPoints.at(i)->hwAddress())) {
                cpPriv->state |= QNetworkConfiguration::Discovered;
                if (!configuredAccessPoints.contains(accessPoints.at(i)->path())) {
                    configuredAccessPoints.insert(accessPoints.at(i)->path(),settingsPath);

                    const QString accessPointId = accessPoints.at(i)->path();
                    QNetworkConfigurationPrivatePointer ptr =
                        accessPointConfigurations.take(accessPointId);

                    if (ptr) {
                        locker.unlock();
                        emit configurationRemoved(ptr);
                        locker.relock();
                    }
                }
                break;
            }
        }
    } else if (connectionType == QLatin1String("gsm")) {

        const QString connectionPath = map.value("connection").value("id").toString();
        cpPriv->name = contextName(connectionPath);
        cpPriv->bearerType = currentBearerType(connectionPath);

        if (ofonoManager && ofonoManager->isValid()) {
            const QString contextPart = connectionPath.section('/', -1);
            QHashIterator<QString, QOfonoDataConnectionManagerInterface*> i(ofonoContextManagers);
            while (i.hasNext()) {
                i.next();
                const QString path = i.key() +"/"+contextPart;
                if (isActiveContext(path)) {
                    cpPriv->state |= QNetworkConfiguration::Active;
                    break;
                }
            }
        }
    }

    return cpPriv;
}

bool QNetworkManagerEngine::isActiveContext(const QString &contextPath)
{
    if (ofonoManager && ofonoManager->isValid()) {
        const QString contextPart = contextPath.section('/', -1);
        QHashIterator<QString, QOfonoDataConnectionManagerInterface*> i(ofonoContextManagers);
        while (i.hasNext()) {
            i.next();
            PathPropertiesList list = i.value()->contextsWithProperties();
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i).path.path().contains(contextPart)) {
                    return list.at(i).properties.value(QStringLiteral("Active")).toBool();

                }
            }
        }
    }
    return false;
}

QNetworkManagerSettingsConnection *QNetworkManagerEngine::connectionFromId(const QString &id) const
{
    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);
        if (id == connection->path())
            return connection;
    }

    return 0;
}

QNetworkSession::State QNetworkManagerEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    if (!ptr->isValid)
        return QNetworkSession::Invalid;

    foreach (const QString &acPath, activeConnectionsList.keys()) {
        QNetworkManagerConnectionActive *activeConnection = activeConnectionsList.value(acPath);

        const QString identifier = activeConnection->connection().path();

        if (id == identifier) {
            switch (activeConnection->state()) {
            case 0:
                return QNetworkSession::Disconnected;
            case 1:
                return QNetworkSession::Connecting;
            case 2:
                return QNetworkSession::Connected;
            }
        }
    }

    if ((ptr->state & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered)
        return QNetworkSession::Disconnected;
    else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined)
        return QNetworkSession::NotAvailable;
    else if ((ptr->state & QNetworkConfiguration::Undefined) == QNetworkConfiguration::Undefined)
        return QNetworkSession::NotAvailable;

    return QNetworkSession::Invalid;
}

quint64 QNetworkManagerEngine::bytesWritten(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr && (ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        const QString networkInterface = connectionInterfaces.value(id);
        if (!networkInterface.isEmpty()) {
            const QString devFile = QLatin1String("/sys/class/net/") +
                                    networkInterface +
                                    QLatin1String("/statistics/tx_bytes");

            quint64 result = Q_UINT64_C(0);

            QFile tx(devFile);
            if (tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&tx);
                in >> result;
                tx.close();
            }

            return result;
        }
    }

    return Q_UINT64_C(0);
}

quint64 QNetworkManagerEngine::bytesReceived(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr && (ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        const QString networkInterface = connectionInterfaces.value(id);
        if (!networkInterface.isEmpty()) {
            const QString devFile = QLatin1String("/sys/class/net/") +
                                    networkInterface +
                                    QLatin1String("/statistics/rx_bytes");

            quint64 result = Q_UINT64_C(0);

            QFile tx(devFile);
            if (tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&tx);
                in >> result;
                tx.close();
            }

            return result;
        }
    }

    return Q_UINT64_C(0);
}

quint64 QNetworkManagerEngine::startTime(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);
    if (connection)
        return connection->getTimestamp();
    else
        return Q_UINT64_C(0);
}

QNetworkConfigurationManager::Capabilities QNetworkManagerEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming |
            QNetworkConfigurationManager::DataStatistics |
            QNetworkConfigurationManager::CanStartAndStopInterfaces;
}

QNetworkSessionPrivate *QNetworkManagerEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QNetworkManagerEngine::defaultConfiguration()
{
    QHashIterator<QString, QNetworkManagerConnectionActive*> i(activeConnectionsList);
    while (i.hasNext()) {
        i.next();
        QNetworkManagerConnectionActive *activeConnection = i.value();
        if ((activeConnection->defaultRoute() || activeConnection->default6Route())) {
            return accessPointConfigurations.value(activeConnection->connection().path());
        }
    }

    return QNetworkConfigurationPrivatePointer();
}

QNetworkConfiguration::BearerType QNetworkManagerEngine::currentBearerType(const QString &id)
{
    QString contextPart = id.section('/', -1);
    QHashIterator<QString, QOfonoDataConnectionManagerInterface*> i(ofonoContextManagers);
    while (i.hasNext()) {
        i.next();
        QString contextPath = i.key() +"/"+contextPart;

        if (i.value()->contexts().contains(contextPath)) {

            QString bearer = i.value()->bearer();

            if (bearer == QStringLiteral("gsm")) {
                return QNetworkConfiguration::Bearer2G;
            } else if (bearer == QStringLiteral("edge")) {
                return QNetworkConfiguration::Bearer2G;
            } else if (bearer == QStringLiteral("umts")) {
                return QNetworkConfiguration::BearerWCDMA;
            } else if (bearer == QStringLiteral("hspa")
                       || bearer == QStringLiteral("hsdpa")
                       || bearer == QStringLiteral("hsupa")) {
                return QNetworkConfiguration::BearerHSPA;
            } else if (bearer == QStringLiteral("lte")) {
                return QNetworkConfiguration::BearerLTE;
            }
        }
    }

    return QNetworkConfiguration::BearerUnknown;
}

QString QNetworkManagerEngine::contextName(const QString &path)
{
    QString contextPart = path.section('/', -1);
    QHashIterator<QString, QOfonoDataConnectionManagerInterface*> i(ofonoContextManagers);
    while (i.hasNext()) {
        i.next();
        PathPropertiesList list = i.value()->contextsWithProperties();
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).path.path().contains(contextPart)) {
                return list.at(i).properties.value(QStringLiteral("Name")).toString();
            }
        }
    }
    return path;
}

void QNetworkManagerEngine::nmRegistered(const QString &)
{
    if (ofonoManager) {
        delete ofonoManager;
        ofonoManager = NULL;
    }
    managerInterface = new QNetworkManagerInterface(this);
    systemSettings = new QNetworkManagerSettings(NM_DBUS_SERVICE, this);

    connect(managerInterface, SIGNAL(deviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(managerInterface, SIGNAL(deviceRemoved(QDBusObjectPath)),
            this, SLOT(deviceRemoved(QDBusObjectPath)));
    connect(managerInterface, SIGNAL(activationFinished(QDBusPendingCallWatcher*)),
            this, SLOT(activationFinished(QDBusPendingCallWatcher*)));
    connect(managerInterface, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
            this, SLOT(interfacePropertiesChanged(QMap<QString,QVariant>)));
    managerInterface->setConnections();

    connect(systemSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));
    systemSettings->setConnections();
    nmAvailable = true;

    setupConfigurations();
}

void QNetworkManagerEngine::nmUnRegistered(const QString &)
{
    if (systemSettings) {
        delete systemSettings;
        systemSettings = NULL;
    }
    if (managerInterface) {
        delete managerInterface;
        managerInterface = NULL;
    }
}

void QNetworkManagerEngine::ofonoRegistered(const QString &)
{
    if (ofonoManager) {
        delete ofonoManager;
        ofonoManager = NULL;
    }
    ofonoManager = new QOfonoManagerInterface(this);
    if (ofonoManager && ofonoManager->isValid()) {
        Q_FOREACH (const QString &modem, ofonoManager->getModems()) {
            QOfonoDataConnectionManagerInterface *ofonoContextManager
                    = new QOfonoDataConnectionManagerInterface(modem,this);
            ofonoContextManagers.insert(modem, ofonoContextManager);
        }
    }
}

void QNetworkManagerEngine::ofonoUnRegistered(const QString &)
{
    ofonoContextManagers.clear();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT
