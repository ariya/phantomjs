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

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QNetworkManagerEngine::QNetworkManagerEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    interface(new QNetworkManagerInterface(this)),
    systemSettings(new QNetworkManagerSettings(NM_DBUS_SERVICE_SYSTEM_SETTINGS, this)),
    userSettings(new QNetworkManagerSettings(NM_DBUS_SERVICE_USER_SETTINGS, this))
{
    if (!interface->isValid())
        return;

    interface->setConnections();
    connect(interface, SIGNAL(deviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(deviceRemoved(QDBusObjectPath)),
            this, SLOT(deviceRemoved(QDBusObjectPath)));
#if 0
    connect(interface, SIGNAL(stateChanged(QString,quint32)),
            this, SIGNAL(configurationsChanged()));
#endif
    connect(interface, SIGNAL(activationFinished(QDBusPendingCallWatcher*)),
            this, SLOT(activationFinished(QDBusPendingCallWatcher*)));
    connect(interface, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
            this, SLOT(interfacePropertiesChanged(QString,QMap<QString,QVariant>)));

    qDBusRegisterMetaType<QNmSettingsMap>();

    systemSettings->setConnections();
    connect(systemSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));

    userSettings->setConnections();
    connect(userSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));
}

QNetworkManagerEngine::~QNetworkManagerEngine()
{
    qDeleteAll(connections);
    qDeleteAll(accessPoints);
    qDeleteAll(wirelessDevices);
    qDeleteAll(activeConnections);
}

void QNetworkManagerEngine::initialize()
{
    QMutexLocker locker(&mutex);

    // Get current list of access points.
    foreach (const QDBusObjectPath &devicePath, interface->getDevices()) {
        locker.unlock();
        deviceAdded(devicePath);
        locker.relock();
    }

    // Get connections.
    foreach (const QDBusObjectPath &settingsPath, systemSettings->listConnections()) {
        locker.unlock();
        newConnection(settingsPath, systemSettings);
        locker.relock();
    }
    foreach (const QDBusObjectPath &settingsPath, userSettings->listConnections()) {
        locker.unlock();
        newConnection(settingsPath, userSettings);
        locker.relock();
    }

    // Get active connections.
    foreach (const QDBusObjectPath &acPath, interface->activeConnections()) {
        QNetworkManagerConnectionActive *activeConnection =
            new QNetworkManagerConnectionActive(acPath.path());
        activeConnections.insert(acPath.path(), activeConnection);

        activeConnection->setConnections();
        connect(activeConnection, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
                this, SLOT(activeConnectionPropertiesChanged(QString,QMap<QString,QVariant>)));
    }
}

bool QNetworkManagerEngine::networkManagerAvailable() const
{
    QMutexLocker locker(&mutex);

    return interface->isValid();
}

QString QNetworkManagerEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    foreach (const QDBusObjectPath &acPath, interface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        const QString identifier = QString::number(qHash(activeConnection.serviceName() + ' ' +
                                                         activeConnection.connection().path()));

        if (id == identifier) {
            QList<QDBusObjectPath> devices = activeConnection.devices();

            if (devices.isEmpty())
                continue;

            QNetworkManagerInterfaceDevice device(devices.at(0).path());
            return device.networkInterface();
        }
    }

    return QString();
}

bool QNetworkManagerEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (connectionFromId(id))
        return true;

    for (int i = 0; i < accessPoints.count(); ++i) {
        QNetworkManagerInterfaceAccessPoint *accessPoint = accessPoints.at(i);

        const QString identifier =
            QString::number(qHash(accessPoint->connectionInterface()->path()));

        if (id == identifier)
            return true;
    }

    return false;
}

void QNetworkManagerEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);

    if (!connection)
        return;

    QNmSettingsMap map = connection->getSettings();
    const QString connectionType = map.value("connection").value("type").toString();

    QString dbusDevicePath;
    foreach (const QDBusObjectPath &devicePath, interface->getDevices()) {
        QNetworkManagerInterfaceDevice device(devicePath.path());
        if (device.deviceType() == DEVICE_TYPE_802_3_ETHERNET &&
            connectionType == QLatin1String("802-3-ethernet")) {
            dbusDevicePath = devicePath.path();
            break;
        } else if (device.deviceType() == DEVICE_TYPE_802_11_WIRELESS &&
                   connectionType == QLatin1String("802-11-wireless")) {
            dbusDevicePath = devicePath.path();
            break;
        }
        else if (device.deviceType() == DEVICE_TYPE_GSM &&
                connectionType == QLatin1String("gsm")) {
            dbusDevicePath = devicePath.path();
            break;
        }
    }

    const QString service = connection->connectionInterface()->service();
    const QString settingsPath = connection->connectionInterface()->path();

    interface->activateConnection(service, QDBusObjectPath(settingsPath),
                                  QDBusObjectPath(dbusDevicePath), QDBusObjectPath("/"));
}

void QNetworkManagerEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    foreach (const QDBusObjectPath &acPath, interface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        const QString identifier = QString::number(qHash(activeConnection.serviceName() + ' ' +
                                                         activeConnection.connection().path()));

        if (id == identifier && accessPointConfigurations.contains(id)) {
            interface->deactivateConnection(acPath);
            break;
        }
    }
}

void QNetworkManagerEngine::requestUpdate()
{
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::interfacePropertiesChanged(const QString &path,
                                                       const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(path)

    QMapIterator<QString, QVariant> i(properties);
    while (i.hasNext()) {
        i.next();

        if (i.key() == QLatin1String("ActiveConnections")) {
            // Active connections changed, update configurations.

            QList<QDBusObjectPath> activeConnections =
                qdbus_cast<QList<QDBusObjectPath> >(i.value().value<QDBusArgument>());

            QStringList identifiers = accessPointConfigurations.keys();
            foreach (const QString &id, identifiers)
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

            QStringList priorActiveConnections = this->activeConnections.keys();

            foreach (const QDBusObjectPath &acPath, activeConnections) {
                priorActiveConnections.removeOne(acPath.path());
                QNetworkManagerConnectionActive *activeConnection =
                    this->activeConnections.value(acPath.path());
                if (!activeConnection) {
                    activeConnection = new QNetworkManagerConnectionActive(acPath.path());
                    this->activeConnections.insert(acPath.path(), activeConnection);

                    activeConnection->setConnections();
                    connect(activeConnection, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
                            this, SLOT(activeConnectionPropertiesChanged(QString,QMap<QString,QVariant>)));
                }

                const QString id = QString::number(qHash(activeConnection->serviceName() + ' ' +
                                                         activeConnection->connection().path()));

                identifiers.removeOne(id);

                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
                if (ptr) {
                    ptr->mutex.lock();
                    if (activeConnection->state() == 2 &&
                        ptr->state != QNetworkConfiguration::Active) {
                        ptr->state = QNetworkConfiguration::Active;
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
                delete this->activeConnections.take(priorActiveConnections.takeFirst());

            while (!identifiers.isEmpty()) {
                // These configurations are not active
                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(identifiers.takeFirst());

                ptr->mutex.lock();
                if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
                    ptr->state = QNetworkConfiguration::Discovered;
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

void QNetworkManagerEngine::activeConnectionPropertiesChanged(const QString &path,
                                                              const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(properties)

    QNetworkManagerConnectionActive *activeConnection = activeConnections.value(path);

    if (!activeConnection)
        return;

    const QString id = QString::number(qHash(activeConnection->serviceName() + ' ' +
                                             activeConnection->connection().path()));

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr) {
        ptr->mutex.lock();
        if (activeConnection->state() == 2 &&
            ptr->state != QNetworkConfiguration::Active) {
            ptr->state = QNetworkConfiguration::Active;
            ptr->mutex.unlock();

            locker.unlock();
            emit configurationChanged(ptr);
            locker.relock();
        } else {
            ptr->mutex.unlock();
        }
    }
}

void QNetworkManagerEngine::devicePropertiesChanged(const QString &path,
                                                    const QMap<QString, QVariant> &properties)
{
    Q_UNUSED(path);
    Q_UNUSED(properties);
}

void QNetworkManagerEngine::deviceAdded(const QDBusObjectPath &path)
{
    QNetworkManagerInterfaceDevice device(path.path());
    if (device.deviceType() == DEVICE_TYPE_802_11_WIRELESS) {
        QNetworkManagerInterfaceDeviceWireless *wirelessDevice =
            new QNetworkManagerInterfaceDeviceWireless(device.connectionInterface()->path());

        wirelessDevice->setConnections();
        connect(wirelessDevice, SIGNAL(accessPointAdded(QString,QDBusObjectPath)),
                this, SLOT(newAccessPoint(QString,QDBusObjectPath)));
        connect(wirelessDevice, SIGNAL(accessPointRemoved(QString,QDBusObjectPath)),
                this, SLOT(removeAccessPoint(QString,QDBusObjectPath)));
        connect(wirelessDevice, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
                this, SLOT(devicePropertiesChanged(QString,QMap<QString,QVariant>)));

        foreach (const QDBusObjectPath &apPath, wirelessDevice->getAccessPoints())
            newAccessPoint(QString(), apPath);

        mutex.lock();
        wirelessDevices.insert(path.path(), wirelessDevice);
        mutex.unlock();
    }
}

void QNetworkManagerEngine::deviceRemoved(const QDBusObjectPath &path)
{
    QMutexLocker locker(&mutex);

    delete wirelessDevices.take(path.path());
}

void QNetworkManagerEngine::newConnection(const QDBusObjectPath &path,
                                          QNetworkManagerSettings *settings)
{
    QMutexLocker locker(&mutex);

    if (!settings)
        settings = qobject_cast<QNetworkManagerSettings *>(sender());

    if (!settings)
        return;

    QNetworkManagerSettingsConnection *connection =
        new QNetworkManagerSettingsConnection(settings->connectionInterface()->service(),
                                              path.path());
    connections.append(connection);

    connect(connection, SIGNAL(removed(QString)), this, SLOT(removeConnection(QString)));
    connect(connection, SIGNAL(updated(QNmSettingsMap)),
            this, SLOT(updateConnection(QNmSettingsMap)));

    const QString service = connection->connectionInterface()->service();
    const QString settingsPath = connection->connectionInterface()->path();

    QNetworkConfigurationPrivate *cpPriv =
        parseConnection(service, settingsPath, connection->getSettings());

    // Check if connection is active.
    foreach (const QDBusObjectPath &acPath, interface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        if (activeConnection.serviceName() == service &&
            activeConnection.connection().path() == settingsPath &&
            activeConnection.state() == 2) {
            cpPriv->state |= QNetworkConfiguration::Active;
            break;
        }
    }

    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    accessPointConfigurations.insert(ptr->id, ptr);

    locker.unlock();
    emit configurationAdded(ptr);
}

void QNetworkManagerEngine::removeConnection(const QString &path)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(path)

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;

    connections.removeAll(connection);

    const QString id = QString::number(qHash(connection->connectionInterface()->service() + ' ' +
                                             connection->connectionInterface()->path()));

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(id);

    connection->deleteLater();

    locker.unlock();
    emit configurationRemoved(ptr);
}

void QNetworkManagerEngine::updateConnection(const QNmSettingsMap &settings)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;

    const QString service = connection->connectionInterface()->service();
    const QString settingsPath = connection->connectionInterface()->path();

    QNetworkConfigurationPrivate *cpPriv = parseConnection(service, settingsPath, settings);

    // Check if connection is active.
    foreach (const QDBusObjectPath &acPath, interface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        if (activeConnection.serviceName() == service &&
            activeConnection.connection().path() == settingsPath &&
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
    delete cpPriv;
}

void QNetworkManagerEngine::activationFinished(QDBusPendingCallWatcher *watcher)
{
    QMutexLocker locker(&mutex);

    QDBusPendingReply<QDBusObjectPath> reply(*watcher);
    if (!reply.isError()) {
        QDBusObjectPath result = reply.value();

        QNetworkManagerConnectionActive activeConnection(result.path());

        const QString id = QString::number(qHash(activeConnection.serviceName() + ' ' +
                                                 activeConnection.connection().path()));

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
        if (ptr) {
            ptr->mutex.lock();
            if (activeConnection.state() == 2 &&
                ptr->state != QNetworkConfiguration::Active) {
                ptr->state = QNetworkConfiguration::Active;
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

void QNetworkManagerEngine::newAccessPoint(const QString &path, const QDBusObjectPath &objectPath)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(path)

    QNetworkManagerInterfaceAccessPoint *accessPoint =
        new QNetworkManagerInterfaceAccessPoint(objectPath.path());
    accessPoints.append(accessPoint);

    accessPoint->setConnections();
    connect(accessPoint, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
            this, SLOT(updateAccessPoint(QMap<QString,QVariant>)));

    // Check if configuration for this SSID already exists.
    for (int i = 0; i < accessPoints.count(); ++i) {
        if (accessPoint != accessPoints.at(i) &&
            accessPoint->ssid() == accessPoints.at(i)->ssid()) {
            return;
        }
    }

    // Check if configuration exists for connection.
    if (!accessPoint->ssid().isEmpty()) {
        for (int i = 0; i < connections.count(); ++i) {
            QNetworkManagerSettingsConnection *connection = connections.at(i);

            if (accessPoint->ssid() == connection->getSsid()) {
                const QString service = connection->connectionInterface()->service();
                const QString settingsPath = connection->connectionInterface()->path();
                const QString connectionId = QString::number(qHash(service + ' ' + settingsPath));

                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(connectionId);
                ptr->mutex.lock();
                ptr->state = QNetworkConfiguration::Discovered;
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
    ptr->id = QString::number(qHash(objectPath.path()));
    ptr->type = QNetworkConfiguration::InternetAccessPoint;
    if(accessPoint->flags() == NM_802_11_AP_FLAGS_PRIVACY) {
        ptr->purpose = QNetworkConfiguration::PrivatePurpose;
    } else {
        ptr->purpose = QNetworkConfiguration::PublicPurpose;
    }
    ptr->state = QNetworkConfiguration::Undefined;
    ptr->bearerType = QNetworkConfiguration::BearerWLAN;

    accessPointConfigurations.insert(ptr->id, ptr);

    locker.unlock();
    emit configurationAdded(ptr);
}

void QNetworkManagerEngine::removeAccessPoint(const QString &path,
                                              const QDBusObjectPath &objectPath)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(path)

    for (int i = 0; i < accessPoints.count(); ++i) {
        QNetworkManagerInterfaceAccessPoint *accessPoint = accessPoints.at(i);

        if (accessPoint->connectionInterface()->path() == objectPath.path()) {
            accessPoints.removeOne(accessPoint);

            if (configuredAccessPoints.contains(accessPoint)) {
                // find connection and change state to Defined
                configuredAccessPoints.removeOne(accessPoint);
                for (int i = 0; i < connections.count(); ++i) {
                    QNetworkManagerSettingsConnection *connection = connections.at(i);

                    if (accessPoint->ssid() == connection->getSsid()) {
                        const QString service = connection->connectionInterface()->service();
                        const QString settingsPath = connection->connectionInterface()->path();
                        const QString connectionId =
                            QString::number(qHash(service + ' ' + settingsPath));

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
                    accessPointConfigurations.take(QString::number(qHash(objectPath.path())));

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

void QNetworkManagerEngine::updateAccessPoint(const QMap<QString, QVariant> &map)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(map)

    QNetworkManagerInterfaceAccessPoint *accessPoint =
        qobject_cast<QNetworkManagerInterfaceAccessPoint *>(sender());
    if (!accessPoint)
        return;

    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);

        if (accessPoint->ssid() == connection->getSsid()) {
            const QString service = connection->connectionInterface()->service();
            const QString settingsPath = connection->connectionInterface()->path();
            const QString connectionId = QString::number(qHash(service + ' ' + settingsPath));

            QNetworkConfigurationPrivatePointer ptr =
                accessPointConfigurations.value(connectionId);
            ptr->mutex.lock();
            ptr->state = QNetworkConfiguration::Discovered;
            ptr->mutex.unlock();

            locker.unlock();
            emit configurationChanged(ptr);
            return;
        }
    }
}

QNetworkConfigurationPrivate *QNetworkManagerEngine::parseConnection(const QString &service,
                                                                     const QString &settingsPath,
                                                                     const QNmSettingsMap &map)
{
    QNetworkConfigurationPrivate *cpPriv = new QNetworkConfigurationPrivate;
    cpPriv->name = map.value("connection").value("id").toString();
    cpPriv->isValid = true;
    cpPriv->id = QString::number(qHash(service + ' ' + settingsPath));
    cpPriv->type = QNetworkConfiguration::InternetAccessPoint;

    cpPriv->purpose = QNetworkConfiguration::PublicPurpose;

    cpPriv->state = QNetworkConfiguration::Defined;

    const QString connectionType = map.value("connection").value("type").toString();

    if (connectionType == QLatin1String("802-3-ethernet")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;
        cpPriv->purpose = QNetworkConfiguration::PublicPurpose;

        foreach (const QDBusObjectPath &devicePath, interface->getDevices()) {
            QNetworkManagerInterfaceDevice device(devicePath.path());
            if (device.deviceType() == DEVICE_TYPE_802_3_ETHERNET) {
                QNetworkManagerInterfaceDeviceWired wiredDevice(device.connectionInterface()->path());
                if (wiredDevice.carrier()) {
                    cpPriv->state |= QNetworkConfiguration::Discovered;
                    break;
                }

            }
        }
    } else if (connectionType == QLatin1String("802-11-wireless")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;

        const QString connectionSsid = map.value("802-11-wireless").value("ssid").toString();
        const QString connectionSecurity = map.value("802-11-wireless").value("security").toString();
        if(!connectionSecurity.isEmpty()) {
            cpPriv->purpose = QNetworkConfiguration::PrivatePurpose;
        } else {
            cpPriv->purpose = QNetworkConfiguration::PublicPurpose;
        }
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connectionSsid == accessPoints.at(i)->ssid()) {
                cpPriv->state |= QNetworkConfiguration::Discovered;
                if (!configuredAccessPoints.contains(accessPoints.at(i))) {
                    configuredAccessPoints.append(accessPoints.at(i));

                    const QString accessPointId =
                        QString::number(qHash(accessPoints.at(i)->connectionInterface()->path()));
                    QNetworkConfigurationPrivatePointer ptr =
                        accessPointConfigurations.take(accessPointId);

                    if (ptr) {
                        mutex.unlock();
                        emit configurationRemoved(ptr);
                        mutex.lock();
                    }
                }
                break;
            }
        }
    } else if (connectionType == "gsm") {
        cpPriv->bearerType = QNetworkConfiguration::Bearer2G;
    } else if (connectionType == "cdma") {
        cpPriv->bearerType = QNetworkConfiguration::BearerCDMA2000;
    }

    return cpPriv;
}

QNetworkManagerSettingsConnection *QNetworkManagerEngine::connectionFromId(const QString &id) const
{
    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);
        const QString service = connection->connectionInterface()->service();
        const QString settingsPath = connection->connectionInterface()->path();

        const QString identifier = QString::number(qHash(service + ' ' + settingsPath));

        if (id == identifier)
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

    foreach (const QString &acPath, activeConnections.keys()) {
        QNetworkManagerConnectionActive *activeConnection = activeConnections.value(acPath);

        const QString identifier = QString::number(qHash(activeConnection->serviceName() + ' ' +
                                                         activeConnection->connection().path()));

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
        const QString networkInterface = getInterfaceFromId(id);
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
        const QString networkInterface = getInterfaceFromId(id);
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
           QNetworkConfigurationManager::CanStartAndStopInterfaces;
}

QNetworkSessionPrivate *QNetworkManagerEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QNetworkManagerEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT
