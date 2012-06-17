/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#include "qnetworkinterface_win_p.h"
#include <qhostinfo.h>
#include <qhash.h>
#include <qurl.h>
#include <private/qsystemlibrary_p.h>

QT_BEGIN_NAMESPACE

typedef DWORD (WINAPI *PtrGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);
static PtrGetAdaptersInfo ptrGetAdaptersInfo = 0;
typedef ULONG (WINAPI *PtrGetAdaptersAddresses)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
static PtrGetAdaptersAddresses ptrGetAdaptersAddresses = 0;
typedef DWORD (WINAPI *PtrGetNetworkParams)(PFIXED_INFO, PULONG);
static PtrGetNetworkParams ptrGetNetworkParams = 0;

static void resolveLibs()
{
    // try to find the functions we need from Iphlpapi.dll
    static bool done = false;
    if (!done) {
        QSystemLibrary iphlpapi(QLatin1String("iphlpapi"));
        if (iphlpapi.load()) {
            ptrGetAdaptersInfo = (PtrGetAdaptersInfo)iphlpapi.resolve("GetAdaptersInfo");
            ptrGetAdaptersAddresses = (PtrGetAdaptersAddresses)iphlpapi.resolve("GetAdaptersAddresses");
            ptrGetNetworkParams = (PtrGetNetworkParams)iphlpapi.resolve("GetNetworkParams");
        }
        done = true;
    }
}

static QHostAddress addressFromSockaddr(sockaddr *sa)
{
    QHostAddress address;
    if (!sa)
        return address;

    if (sa->sa_family == AF_INET)
        address.setAddress(htonl(((sockaddr_in *)sa)->sin_addr.s_addr));
    else if (sa->sa_family == AF_INET6)
        address.setAddress(((qt_sockaddr_in6 *)sa)->sin6_addr.qt_s6_addr);
    else
        qWarning("Got unknown socket family %d", sa->sa_family);
    return address;

}

static QHash<QHostAddress, QHostAddress> ipv4Netmasks()
{
    //Retrieve all the IPV4 addresses & netmasks
    IP_ADAPTER_INFO staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_INFO pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;
    QHash<QHostAddress, QHostAddress> ipv4netmasks; 

    DWORD retval = ptrGetAdaptersInfo(pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = (IP_ADAPTER_INFO *)qMalloc(bufSize);
        if (!pAdapter)
            return ipv4netmasks;
        // try again
        if (ptrGetAdaptersInfo(pAdapter, &bufSize) != ERROR_SUCCESS) {
            qFree(pAdapter);
            return ipv4netmasks;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return ipv4netmasks;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_INFO ptr = pAdapter; ptr; ptr = ptr->Next) {
        for (PIP_ADDR_STRING addr = &ptr->IpAddressList; addr; addr = addr->Next) {
            QHostAddress address(QLatin1String(addr->IpAddress.String));
            QHostAddress mask(QLatin1String(addr->IpMask.String));
            ipv4netmasks[address] = mask;
        }
    }
    if (pAdapter != staticBuf)
        qFree(pAdapter);

    return ipv4netmasks;

}

static QList<QNetworkInterfacePrivate *> interfaceListingWinXP()
{
    QList<QNetworkInterfacePrivate *> interfaces;
    IP_ADAPTER_ADDRESSES staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_ADDRESSES pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    const QHash<QHostAddress, QHostAddress> &ipv4netmasks = ipv4Netmasks();
    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES |
                  GAA_FLAG_INCLUDE_PREFIX |
                  GAA_FLAG_SKIP_DNS_SERVER |
                  GAA_FLAG_SKIP_MULTICAST;
    ULONG retval = ptrGetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = (IP_ADAPTER_ADDRESSES *)qMalloc(bufSize);
        if (!pAdapter)
            return interfaces;
        // try again
        if (ptrGetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize) != ERROR_SUCCESS) {
            qFree(pAdapter);
            return interfaces;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return interfaces;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_ADDRESSES ptr = pAdapter; ptr; ptr = ptr->Next) {
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, Ipv6IfIndex) && ptr->Ipv6IfIndex != 0)
            iface->index = ptr->Ipv6IfIndex;
        else if (ptr->IfIndex != 0)
            iface->index = ptr->IfIndex;

        iface->flags = QNetworkInterface::CanBroadcast;
        if (ptr->OperStatus == IfOperStatusUp)
            iface->flags |= QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if ((ptr->Flags & IP_ADAPTER_NO_MULTICAST) == 0)
            iface->flags |= QNetworkInterface::CanMulticast;

        iface->name = QString::fromLocal8Bit(ptr->AdapterName);
        iface->friendlyName = QString::fromWCharArray(ptr->FriendlyName);
        if (ptr->PhysicalAddressLength)
            iface->hardwareAddress = iface->makeHwAddress(ptr->PhysicalAddressLength,
                                                          ptr->PhysicalAddress);
        else
            // loopback if it has no address
            iface->flags |= QNetworkInterface::IsLoopBack;

        // The GetAdaptersAddresses call has an interesting semantic:
        // It can return a number N of addresses and a number M of prefixes.
        // But if you have IPv6 addresses, generally N > M.
        // I cannot find a way to relate the Address to the Prefix, aside from stopping
        // the iteration at the last Prefix entry and assume that it applies to all addresses
        // from that point on.
        PIP_ADAPTER_PREFIX pprefix = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, FirstPrefix))
            pprefix = ptr->FirstPrefix;
        for (PIP_ADAPTER_UNICAST_ADDRESS addr = ptr->FirstUnicastAddress; addr; addr = addr->Next) {
            QNetworkAddressEntry entry;
            entry.setIp(addressFromSockaddr(addr->Address.lpSockaddr));
            if (pprefix) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    entry.setNetmask(ipv4netmasks[entry.ip()]);

                    // broadcast address is set on postProcess()
                } else { //IPV6
                    entry.setPrefixLength(pprefix->PrefixLength);
                }
                pprefix = pprefix->Next ? pprefix->Next : pprefix;
            }
            iface->addressEntries << entry;
        }
    }

    if (pAdapter != staticBuf)
        qFree(pAdapter);

    return interfaces;
}

static QList<QNetworkInterfacePrivate *> interfaceListingWin2k()
{
    QList<QNetworkInterfacePrivate *> interfaces;
    IP_ADAPTER_INFO staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_INFO pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    DWORD retval = ptrGetAdaptersInfo(pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = (IP_ADAPTER_INFO *)qMalloc(bufSize);
        if (!pAdapter)
            return interfaces;
        // try again
        if (ptrGetAdaptersInfo(pAdapter, &bufSize) != ERROR_SUCCESS) {
            qFree(pAdapter);
            return interfaces;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return interfaces;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_INFO ptr = pAdapter; ptr; ptr = ptr->Next) {
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = ptr->Index;
        iface->flags = QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if (ptr->Type == MIB_IF_TYPE_PPP)
            iface->flags |= QNetworkInterface::IsPointToPoint;
        else
            iface->flags |= QNetworkInterface::CanBroadcast;
        iface->name = QString::fromLocal8Bit(ptr->AdapterName);
        iface->hardwareAddress = QNetworkInterfacePrivate::makeHwAddress(ptr->AddressLength,
                                                                         ptr->Address);

        for (PIP_ADDR_STRING addr = &ptr->IpAddressList; addr; addr = addr->Next) {
            QNetworkAddressEntry entry;
            entry.setIp(QHostAddress(QLatin1String(addr->IpAddress.String)));
            entry.setNetmask(QHostAddress(QLatin1String(addr->IpMask.String)));
            // broadcast address is set on postProcess()

            iface->addressEntries << entry;
        }
    }

    if (pAdapter != staticBuf)
        qFree(pAdapter);

    return interfaces;
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    resolveLibs();
    if (ptrGetAdaptersAddresses != NULL)
        return interfaceListingWinXP();
    else if (ptrGetAdaptersInfo != NULL)
        return interfaceListingWin2k();

    // failed
    return QList<QNetworkInterfacePrivate *>();
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QString QHostInfo::localDomainName()
{
    resolveLibs();
    if (ptrGetNetworkParams == NULL)
        return QString();       // couldn't resolve

    FIXED_INFO info, *pinfo;
    ULONG bufSize = sizeof info;
    pinfo = &info;
    if (ptrGetNetworkParams(pinfo, &bufSize) == ERROR_BUFFER_OVERFLOW) {
        pinfo = (FIXED_INFO *)qMalloc(bufSize);
        if (!pinfo)
            return QString();
        // try again
        if (ptrGetNetworkParams(pinfo, &bufSize) != ERROR_SUCCESS) {
            qFree(pinfo);
            return QString();   // error
        }
    }

    QString domainName = QUrl::fromAce(pinfo->DomainName);

    if (pinfo != &info)
        qFree(pinfo);

    return domainName;
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
