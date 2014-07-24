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

#include "qgenericengine.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstringlist.h>

#include <QtCore/qdebug.h>
#include <QtCore/private/qcoreapplication_p.h>

#ifdef Q_OS_WIN
#include "../platformdefs_win.h"
#endif

#ifdef Q_OS_WINCE
typedef ULONG NDIS_OID, *PNDIS_OID;
#include <nuiouser.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>
#endif

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

#ifndef QT_NO_NETWORKINTERFACE
static QNetworkConfiguration::BearerType qGetInterfaceType(const QString &interface)
{
#if defined(Q_OS_WIN)
    DWORD bytesWritten;
    NDIS_MEDIUM medium;
    NDIS_PHYSICAL_MEDIUM physicalMedium;

#ifdef Q_OS_WINCE
    NDISUIO_QUERY_OID nicGetOid;
    HANDLE handle = CreateFile((PTCHAR)NDISUIO_DEVICE_NAME, 0,
                               FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#else
    unsigned long oid;
    HANDLE handle = CreateFile((TCHAR *)QString::fromLatin1("\\\\.\\%1").arg(interface).utf16(), 0,
                               FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#endif
    if (handle == INVALID_HANDLE_VALUE)
        return QNetworkConfiguration::BearerUnknown;

    bytesWritten = 0;

#ifdef Q_OS_WINCE
    ZeroMemory(&nicGetOid, sizeof(NDISUIO_QUERY_OID));
    nicGetOid.Oid = OID_GEN_MEDIA_SUPPORTED;
    nicGetOid.ptcDeviceName = (PTCHAR)interface.utf16();
    bool result = DeviceIoControl(handle, IOCTL_NDISUIO_QUERY_OID_VALUE, &nicGetOid, sizeof(nicGetOid),
                                  &nicGetOid, sizeof(nicGetOid), &bytesWritten, 0);
#else
    oid = OID_GEN_MEDIA_SUPPORTED;
    bool result = DeviceIoControl(handle, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof(oid),
                                  &medium, sizeof(medium), &bytesWritten, 0);
#endif
    if (!result) {
        CloseHandle(handle);
        return QNetworkConfiguration::BearerUnknown;
    }

    bytesWritten = 0;

#ifdef Q_OS_WINCE
    medium = NDIS_MEDIUM( *(LPDWORD)nicGetOid.Data );

    ZeroMemory(&nicGetOid, sizeof(NDISUIO_QUERY_OID));
    nicGetOid.Oid = OID_GEN_PHYSICAL_MEDIUM;
    nicGetOid.ptcDeviceName = (PTCHAR)interface.utf16();

    result = DeviceIoControl(handle, IOCTL_NDISUIO_QUERY_OID_VALUE, &nicGetOid, sizeof(nicGetOid),
                             &nicGetOid, sizeof(nicGetOid), &bytesWritten, 0);

    physicalMedium = NDIS_PHYSICAL_MEDIUM( *(LPDWORD)nicGetOid.Data );
#else
    oid = OID_GEN_PHYSICAL_MEDIUM;
    result = DeviceIoControl(handle, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof(oid),
                             &physicalMedium, sizeof(physicalMedium), &bytesWritten, 0);
#endif

    if (!result) {
        CloseHandle(handle);

        if (medium == NdisMedium802_3)
            return QNetworkConfiguration::BearerEthernet;
        else
            return QNetworkConfiguration::BearerUnknown;
    }

    CloseHandle(handle);

    if (medium == NdisMedium802_3) {
        switch (physicalMedium) {
        case NdisPhysicalMediumWirelessLan:
            return QNetworkConfiguration::BearerWLAN;
        case NdisPhysicalMediumBluetooth:
            return QNetworkConfiguration::BearerBluetooth;
        case NdisPhysicalMediumWiMax:
            return QNetworkConfiguration::BearerWiMAX;
        default:
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "Physical Medium" << physicalMedium;
#endif
            return QNetworkConfiguration::BearerEthernet;
        }
    }

#ifdef BEARER_MANAGEMENT_DEBUG
    qDebug() << medium << physicalMedium;
#endif
#elif defined(Q_OS_LINUX)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    ifreq request;
    strncpy(request.ifr_name, interface.toLocal8Bit().data(), sizeof(request.ifr_name));
    int result = ioctl(sock, SIOCGIFHWADDR, &request);
    close(sock);

    if (result >= 0 && request.ifr_hwaddr.sa_family == ARPHRD_ETHER)
        return QNetworkConfiguration::BearerEthernet;
#else
    Q_UNUSED(interface);
#endif

    return QNetworkConfiguration::BearerUnknown;
}
#endif

QGenericEngine::QGenericEngine(QObject *parent)
:   QBearerEngineImpl(parent)
{
    //workaround for deadlock in __cxa_guard_acquire with webkit on macos x
    //initialise the Q_GLOBAL_STATIC in same thread as the AtomicallyInitializedStatic
    (void)QNetworkInterface::interfaceFromIndex(0);
}

QGenericEngine::~QGenericEngine()
{
}

QString QGenericEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    return configurationInterface.value(id);
}

bool QGenericEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    return configurationInterface.contains(id);
}

void QGenericEngine::connectToId(const QString &id)
{
    emit connectionError(id, OperationNotSupported);
}

void QGenericEngine::disconnectFromId(const QString &id)
{
    emit connectionError(id, OperationNotSupported);
}

void QGenericEngine::initialize()
{
    doRequestUpdate();
}

void QGenericEngine::requestUpdate()
{
    doRequestUpdate();
}

void QGenericEngine::doRequestUpdate()
{
#ifndef QT_NO_NETWORKINTERFACE
    QMutexLocker locker(&mutex);

    // Immediately after connecting with a wireless access point
    // QNetworkInterface::allInterfaces() will sometimes return an empty list. Calling it again a
    // second time results in a non-empty list. If we loose interfaces we will end up removing
    // network configurations which will break current sessions.
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    if (interfaces.isEmpty())
        interfaces = QNetworkInterface::allInterfaces();

    QStringList previous = accessPointConfigurations.keys();

    // create configuration for each interface
    while (!interfaces.isEmpty()) {
        QNetworkInterface interface = interfaces.takeFirst();

        if (!interface.isValid())
            continue;

        // ignore loopback interface
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        // ignore WLAN interface handled in separate engine
        if (qGetInterfaceType(interface.name()) == QNetworkConfiguration::BearerWLAN)
            continue;

        uint identifier;
        if (interface.index())
            identifier = qHash(QLatin1String("generic:") + QString::number(interface.index()));
        else
            identifier = qHash(QLatin1String("generic:") + interface.hardwareAddress());

        const QString id = QString::number(identifier);

        previous.removeAll(id);

        QString name = interface.humanReadableName();
        if (name.isEmpty())
            name = interface.name();

        QNetworkConfiguration::StateFlags state = QNetworkConfiguration::Defined;
        if ((interface.flags() & QNetworkInterface::IsRunning) && !interface.addressEntries().isEmpty())
            state |= QNetworkConfiguration::Active;

        if (accessPointConfigurations.contains(id)) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

            bool changed = false;

            ptr->mutex.lock();

            if (!ptr->isValid) {
                ptr->isValid = true;
                changed = true;
            }

            if (ptr->name != name) {
                ptr->name = name;
                changed = true;
            }

            if (ptr->id != id) {
                ptr->id = id;
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

            ptr->name = name;
            ptr->isValid = true;
            ptr->id = id;
            ptr->state = state;
            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            ptr->bearerType = qGetInterfaceType(interface.name());

            accessPointConfigurations.insert(id, ptr);
            configurationInterface.insert(id, interface.name());

            locker.unlock();
            emit configurationAdded(ptr);
            locker.relock();
        }
    }

    while (!previous.isEmpty()) {
        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.take(previous.takeFirst());

        configurationInterface.remove(ptr->id);

        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }

    locker.unlock();
#endif

    emit updateCompleted();
}

QNetworkSession::State QGenericEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    QMutexLocker configLocker(&ptr->mutex);

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

QNetworkConfigurationManager::Capabilities QGenericEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming;
}

QNetworkSessionPrivate *QGenericEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QGenericEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}


bool QGenericEngine::requiresPolling() const
{
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
