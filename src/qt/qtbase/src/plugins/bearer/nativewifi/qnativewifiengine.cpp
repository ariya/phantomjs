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

#include "qnativewifiengine.h"
#include "platformdefs.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qcoreapplication.h>

#include <QtCore/qdebug.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

WlanOpenHandleProto local_WlanOpenHandle = 0;
WlanRegisterNotificationProto local_WlanRegisterNotification = 0;
WlanEnumInterfacesProto local_WlanEnumInterfaces = 0;
WlanGetAvailableNetworkListProto local_WlanGetAvailableNetworkList = 0;
WlanQueryInterfaceProto local_WlanQueryInterface = 0;
WlanConnectProto local_WlanConnect = 0;
WlanDisconnectProto local_WlanDisconnect = 0;
WlanScanProto local_WlanScan = 0;
WlanFreeMemoryProto local_WlanFreeMemory = 0;
WlanCloseHandleProto local_WlanCloseHandle = 0;

void qNotificationCallback(WLAN_NOTIFICATION_DATA *data, QNativeWifiEngine *d)
{
    Q_UNUSED(d);

    if (data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM) {
        switch (data->NotificationCode) {
        case wlan_notification_acm_connection_complete:
        case wlan_notification_acm_disconnected:
        case wlan_notification_acm_scan_complete:
        case wlan_notification_acm_scan_fail:
            QMetaObject::invokeMethod(d, "scanComplete", Qt::QueuedConnection);
            break;
        default:
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "wlan acm notification" << (int)data->NotificationCode;
#endif
            break;
        }
    } else {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "wlan notification source" << (int)data->NotificationSource << "code" << (int)data->NotificationCode;
#endif
    }
}

QNativeWifiEngine::QNativeWifiEngine(QObject *parent)
:   QBearerEngineImpl(parent), handle(INVALID_HANDLE_VALUE)
{
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(closeHandle()));
}

QNativeWifiEngine::~QNativeWifiEngine()
{
    closeHandle();
}

void QNativeWifiEngine::scanComplete()
{
    QMutexLocker locker(&mutex);

    if (!available()) {
        locker.unlock();
        emit updateCompleted();
        return;
    }

    // enumerate interfaces
    WLAN_INTERFACE_INFO_LIST *interfaceList;
    DWORD result = local_WlanEnumInterfaces(handle, 0, &interfaceList);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanEnumInterfaces failed with error %ld\n", __FUNCTION__, result);
#endif

        locker.unlock();
        emit updateCompleted();

        return;
    }

    QStringList previous = accessPointConfigurations.keys();

    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO &interface = interfaceList->InterfaceInfo[i];

        WLAN_AVAILABLE_NETWORK_LIST *networkList;
        result = local_WlanGetAvailableNetworkList(handle, &interface.InterfaceGuid,
                                                   3, 0, &networkList);
        if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("%s: WlanGetAvailableNetworkList failed with error %ld\n",
                   __FUNCTION__, result);
#endif
            continue;
        }

        QStringList seenNetworks;

        for (unsigned int j = 0; j < networkList->dwNumberOfItems; ++j) {
            WLAN_AVAILABLE_NETWORK &network = networkList->Network[j];

            QString networkName;

            if (network.strProfileName[0] != 0) {
                networkName = QString::fromWCharArray(network.strProfileName);
            } else {
                networkName = QByteArray(reinterpret_cast<char *>(network.dot11Ssid.ucSSID),
                                         network.dot11Ssid.uSSIDLength);
            }

            const QString id = QString::number(qHash(QLatin1String("WLAN:") + networkName));

            previous.removeAll(id);

            QNetworkConfiguration::StateFlags state = QNetworkConfiguration::Undefined;

            if (!(network.dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE))
                state = QNetworkConfiguration::Undefined;

            if (network.strProfileName[0] != 0) {
                if (network.bNetworkConnectable) {
                    if (network.dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
                        state = QNetworkConfiguration::Active;
                    else
                        state = QNetworkConfiguration::Discovered;
                } else {
                    state = QNetworkConfiguration::Defined;
                }
            }

            if (seenNetworks.contains(networkName))
                continue;
            else
                seenNetworks.append(networkName);

            if (accessPointConfigurations.contains(id)) {
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

                bool changed = false;

                ptr->mutex.lock();

                if (!ptr->isValid) {
                    ptr->isValid = true;
                    changed = true;
                }

                if (ptr->name != networkName) {
                    ptr->name = networkName;
                    changed = true;
                }

                if (ptr->state != state) {
                    ptr->state = state;
                    changed = true;
                }

                ptr->mutex.unlock();

                if (changed) {
                    locker.unlock();
                    emit configurationChanged(ptr);
                    locker.relock();
                }
            } else {
                QNetworkConfigurationPrivatePointer ptr(new QNetworkConfigurationPrivate);

                ptr->name = networkName;
                ptr->isValid = true;
                ptr->id = id;
                ptr->state = state;
                ptr->type = QNetworkConfiguration::InternetAccessPoint;
                ptr->bearerType = QNetworkConfiguration::BearerWLAN;

                accessPointConfigurations.insert(id, ptr);

                locker.unlock();
                emit configurationAdded(ptr);
                locker.relock();
            }
        }

        local_WlanFreeMemory(networkList);
    }

    local_WlanFreeMemory(interfaceList);

    while (!previous.isEmpty()) {
        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.take(previous.takeFirst());

        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }

    locker.unlock();
    emit updateCompleted();
}

QString QNativeWifiEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (!available())
        return QString();

    // enumerate interfaces
    WLAN_INTERFACE_INFO_LIST *interfaceList;
    DWORD result = local_WlanEnumInterfaces(handle, 0, &interfaceList);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanEnumInterfaces failed with error %ld\n", __FUNCTION__, result);
#endif
        return QString();
    }

    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO &interface = interfaceList->InterfaceInfo[i];

        DWORD dataSize;
        WLAN_CONNECTION_ATTRIBUTES *connectionAttributes;
        result = local_WlanQueryInterface(handle, &interface.InterfaceGuid,
                                          wlan_intf_opcode_current_connection, 0, &dataSize,
                                          reinterpret_cast<PVOID *>(&connectionAttributes), 0);
        if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
            if (result != ERROR_INVALID_STATE)
                qDebug("%s: WlanQueryInterface failed with error %ld\n", __FUNCTION__, result);
#endif

            continue;
        }

        if (qHash(QLatin1String("WLAN:") +
                  QString::fromWCharArray(connectionAttributes->strProfileName)) == id.toUInt()) {
            QString guid("{%1-%2-%3-%4%5-%6%7%8%9%10%11}");

            guid = guid.arg(interface.InterfaceGuid.Data1, 8, 16, QChar('0'));
            guid = guid.arg(interface.InterfaceGuid.Data2, 4, 16, QChar('0'));
            guid = guid.arg(interface.InterfaceGuid.Data3, 4, 16, QChar('0'));
            for (int i = 0; i < 8; ++i)
                guid = guid.arg(interface.InterfaceGuid.Data4[i], 2, 16, QChar('0'));

            local_WlanFreeMemory(connectionAttributes);
            local_WlanFreeMemory(interfaceList);

            return guid.toUpper();
        }

        local_WlanFreeMemory(connectionAttributes);
    }

    local_WlanFreeMemory(interfaceList);

    return QString();
}

bool QNativeWifiEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (!available())
        return false;

    // enumerate interfaces
    WLAN_INTERFACE_INFO_LIST *interfaceList;
    DWORD result = local_WlanEnumInterfaces(handle, 0, &interfaceList);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanEnumInterfaces failed with error %ld\n", __FUNCTION__, result);
#endif
        return false;
    }

    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO &interface = interfaceList->InterfaceInfo[i];

        WLAN_AVAILABLE_NETWORK_LIST *networkList;
        result = local_WlanGetAvailableNetworkList(handle, &interface.InterfaceGuid,
                                                   3, 0, &networkList);
        if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("%s: WlanGetAvailableNetworkList failed with error %ld\n",
                   __FUNCTION__, result);
#endif
            continue;
        }

        for (unsigned int j = 0; j < networkList->dwNumberOfItems; ++j) {
            WLAN_AVAILABLE_NETWORK &network = networkList->Network[j];

            QString networkName;

            if (network.strProfileName[0] != 0) {
                networkName = QString::fromWCharArray(network.strProfileName);
            } else {
                networkName = QByteArray(reinterpret_cast<char *>(network.dot11Ssid.ucSSID),
                                         network.dot11Ssid.uSSIDLength);
            }

            if (qHash(QLatin1String("WLAN:") + networkName) == id.toUInt()) {
                local_WlanFreeMemory(networkList);
                local_WlanFreeMemory(interfaceList);
                return true;
            }
        }

        local_WlanFreeMemory(networkList);
    }

    local_WlanFreeMemory(interfaceList);

    return false;
}

void QNativeWifiEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (!available()) {
        locker.unlock();
        emit connectionError(id, InterfaceLookupError);
        return;
    }

    WLAN_INTERFACE_INFO_LIST *interfaceList;
    DWORD result = local_WlanEnumInterfaces(handle, 0, &interfaceList);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanEnumInterfaces failed with error %ld\n", __FUNCTION__, result);
#endif
        locker.unlock();
        emit connectionError(id, InterfaceLookupError);
        return;
    }

    QString profile;

    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO &interface = interfaceList->InterfaceInfo[i];

        WLAN_AVAILABLE_NETWORK_LIST *networkList;
        result = local_WlanGetAvailableNetworkList(handle, &interface.InterfaceGuid,
                                                   3, 0, &networkList);
        if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("%s: WlanGetAvailableNetworkList failed with error %ld\n",
                   __FUNCTION__, result);
#endif
            continue;
        }

        for (unsigned int j = 0; j < networkList->dwNumberOfItems; ++j) {
            WLAN_AVAILABLE_NETWORK &network = networkList->Network[j];

            profile = QString::fromWCharArray(network.strProfileName);

            if (qHash(QLatin1String("WLAN:") + profile) == id.toUInt())
                break;
            else
                profile.clear();
        }

        local_WlanFreeMemory(networkList);

        if (!profile.isEmpty()) {
            WLAN_CONNECTION_PARAMETERS parameters;
            parameters.wlanConnectionMode = wlan_connection_mode_profile;
            parameters.strProfile = reinterpret_cast<LPCWSTR>(profile.utf16());
            parameters.pDot11Ssid = 0;
            parameters.pDesiredBssidList = 0;
            parameters.dot11BssType = dot11_BSS_type_any;
            parameters.dwFlags = 0;

            DWORD result = local_WlanConnect(handle, &interface.InterfaceGuid, &parameters, 0);
            if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
                qDebug("%s: WlanConnect failed with error %ld\n", __FUNCTION__, result);
#endif
                locker.unlock();
                emit connectionError(id, ConnectError);
                locker.relock();
                break;
            }

            break;
        }
    }

    local_WlanFreeMemory(interfaceList);

    if (profile.isEmpty()) {
        locker.unlock();
        emit connectionError(id, InterfaceLookupError);
    }
}

void QNativeWifiEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (!available()) {
        locker.unlock();
        emit connectionError(id, InterfaceLookupError);
        return;
    }

    QString interface = getInterfaceFromId(id);

    if (interface.isEmpty()) {
        locker.unlock();
        emit connectionError(id, InterfaceLookupError);
        return;
    }

    QStringList split = interface.mid(1, interface.length() - 2).split('-');

    GUID guid;
    guid.Data1 = split.at(0).toUInt(0, 16);
    guid.Data2 = split.at(1).toUShort(0, 16);
    guid.Data3 = split.at(2).toUShort(0, 16);
    guid.Data4[0] = split.at(3).left(2).toUShort(0, 16);
    guid.Data4[1] = split.at(3).right(2).toUShort(0, 16);
    for (int i = 0; i < 6; ++i)
        guid.Data4[i + 2] = split.at(4).mid(i*2, 2).toUShort(0, 16);

    DWORD result = local_WlanDisconnect(handle, &guid, 0);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanDisconnect failed with error %ld\n", __FUNCTION__, result);
#endif
        locker.unlock();
        emit connectionError(id, DisconnectionError);
        return;
    }
}

void QNativeWifiEngine::initialize()
{
    scanComplete();
}

void QNativeWifiEngine::requestUpdate()
{
    QMutexLocker locker(&mutex);

    if (!available()) {
        locker.unlock();
        emit updateCompleted();
        return;
    }

    // enumerate interfaces
    WLAN_INTERFACE_INFO_LIST *interfaceList;
    DWORD result = local_WlanEnumInterfaces(handle, 0, &interfaceList);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("%s: WlanEnumInterfaces failed with error %ld\n", __FUNCTION__, result);
#endif

        locker.unlock();
        emit updateCompleted();

        return;
    }

    bool requested = false;
    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        result = local_WlanScan(handle, &interfaceList->InterfaceInfo[i].InterfaceGuid, 0, 0, 0);
        if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("%s: WlanScan failed with error %ld\n", __FUNCTION__, result);
#endif
        } else {
            requested = true;
        }
    }

    local_WlanFreeMemory(interfaceList);

    if (!requested) {
        locker.unlock();
        emit updateCompleted();
    }
}

QNetworkSession::State QNativeWifiEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    if (!ptr->isValid) {
        return QNetworkSession::Invalid;
    } else if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        return QNetworkSession::Connected;
    } else if ((ptr->state & QNetworkConfiguration::Discovered) ==
                QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) ==
                QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

QNetworkConfigurationManager::Capabilities QNativeWifiEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming |
            QNetworkConfigurationManager::CanStartAndStopInterfaces;
}

QNetworkSessionPrivate *QNativeWifiEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QNativeWifiEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

bool QNativeWifiEngine::available()
{
    if (handle != INVALID_HANDLE_VALUE)
        return true;

    DWORD clientVersion;

    DWORD result = local_WlanOpenHandle(1, 0, &clientVersion, &handle);
    if (result != ERROR_SUCCESS) {
#ifdef BEARER_MANAGEMENT_DEBUG
        if (result != ERROR_SERVICE_NOT_ACTIVE)
            qDebug("%s: WlanOpenHandle failed with error %ld\n", __FUNCTION__, result);
#endif

        return false;
    }

    result = local_WlanRegisterNotification(handle, WLAN_NOTIFICATION_SOURCE_ALL, true,
                                            WLAN_NOTIFICATION_CALLBACK(qNotificationCallback),
                                            this, 0, 0);
#ifdef BEARER_MANAGEMENT_DEBUG
    if (result != ERROR_SUCCESS)
        qDebug("%s: WlanRegisterNotification failed with error %ld\n", __FUNCTION__, result);
#endif

    return handle != INVALID_HANDLE_VALUE;
}

void QNativeWifiEngine::closeHandle()
{
    if (handle != INVALID_HANDLE_VALUE) {
        local_WlanCloseHandle(handle, 0);
        handle = INVALID_HANDLE_VALUE;
    }
}

bool QNativeWifiEngine::requiresPolling() const
{
    // On Windows XP SP2 and SP3 only connection and disconnection notifications are available.
    // We need to poll for changes in available wireless networks.
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
