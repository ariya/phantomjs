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

#include "qset.h"
#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"
#include "qalgorithms.h"
#include "private/qnet_unix_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#define IP_MULTICAST    // make AIX happy and define IFF_MULTICAST

#include <sys/types.h>
#include <sys/socket.h>

#ifdef Q_OS_SOLARIS
# include <sys/sockio.h>
#endif
#include <net/if.h>

#ifndef QT_NO_GETIFADDRS
# include <ifaddrs.h>
#endif

#ifdef QT_LINUXBASE
#  include <arpa/inet.h>
#  ifndef SIOCGIFBRDADDR
#    define SIOCGIFBRDADDR 0x8919
#  endif
#endif // QT_LINUXBASE

#include <qplatformdefs.h>

QT_BEGIN_NAMESPACE

static QHostAddress addressFromSockaddr(sockaddr *sa)
{
    QHostAddress address;
    if (!sa)
        return address;

    if (sa->sa_family == AF_INET)
        address.setAddress(htonl(((sockaddr_in *)sa)->sin_addr.s_addr));
#ifndef QT_NO_IPV6
    else if (sa->sa_family == AF_INET6)
        address.setAddress(((sockaddr_in6 *)sa)->sin6_addr.s6_addr);
#endif
    return address;

}

static QNetworkInterface::InterfaceFlags convertFlags(uint rawFlags)
{
    QNetworkInterface::InterfaceFlags flags = 0;
    flags |= (rawFlags & IFF_UP) ? QNetworkInterface::IsUp : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_RUNNING) ? QNetworkInterface::IsRunning : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_BROADCAST) ? QNetworkInterface::CanBroadcast : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_LOOPBACK) ? QNetworkInterface::IsLoopBack : QNetworkInterface::InterfaceFlag(0);
#ifdef IFF_POINTOPOINT //cygwin doesn't define IFF_POINTOPOINT
    flags |= (rawFlags & IFF_POINTOPOINT) ? QNetworkInterface::IsPointToPoint : QNetworkInterface::InterfaceFlag(0);
#endif

#ifdef IFF_MULTICAST
    flags |= (rawFlags & IFF_MULTICAST) ? QNetworkInterface::CanMulticast : QNetworkInterface::InterfaceFlag(0);
#endif
    return flags;
}

#ifdef QT_NO_GETIFADDRS
// getifaddrs not available

static const int STORAGEBUFFER_GROWTH = 256;

static QSet<QByteArray> interfaceNames(int socket)
{
    QSet<QByteArray> result;
#ifdef QT_NO_IPV6IFNAME
    QByteArray storageBuffer;
    struct ifconf interfaceList;

    forever {
        // grow the storage buffer
        storageBuffer.resize(storageBuffer.size() + STORAGEBUFFER_GROWTH);
        interfaceList.ifc_buf = storageBuffer.data();
        interfaceList.ifc_len = storageBuffer.size();

        // get the interface list
        if (qt_safe_ioctl(socket, SIOCGIFCONF, &interfaceList) >= 0) {
            if (int(interfaceList.ifc_len + sizeof(ifreq) + 64) < storageBuffer.size()) {
                // if the buffer was big enough, break
                storageBuffer.resize(interfaceList.ifc_len);
                break;
            }
        } else {
            // internal error
            return result;
        }
        if (storageBuffer.size() > 100000) {
            // out of space
            return result;
        }
    }

    int interfaceCount = interfaceList.ifc_len / sizeof(ifreq);
    for (int i = 0; i < interfaceCount; ++i) {
        QByteArray name = QByteArray(interfaceList.ifc_req[i].ifr_name);
        if (!name.isEmpty())
            result << name;
    }

    return result;
#else
    Q_UNUSED(socket);

    // use if_nameindex
    struct if_nameindex *interfaceList = ::if_nameindex();
    for (struct if_nameindex *ptr = interfaceList; ptr && ptr->if_name; ++ptr)
        result << ptr->if_name;

    if_freenameindex(interfaceList);
    return result;
#endif
}

static QNetworkInterfacePrivate *findInterface(int socket, QList<QNetworkInterfacePrivate *> &interfaces,
                                               struct ifreq &req)
{
    QNetworkInterfacePrivate *iface = 0;
    int ifindex = 0;

#ifndef QT_NO_IPV6IFNAME
    // Get the interface index
    ifindex = if_nametoindex(req.ifr_name);

    // find the interface data
    QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
    for ( ; if_it != interfaces.end(); ++if_it)
        if ((*if_it)->index == ifindex) {
            // existing interface
            iface = *if_it;
            break;
        }
#else
    // Search by name
    QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
    for ( ; if_it != interfaces.end(); ++if_it)
        if ((*if_it)->name == QLatin1String(req.ifr_name)) {
            // existing interface
            iface = *if_it;
            break;
        }
#endif

    if (!iface) {
        // new interface, create data:
        iface = new QNetworkInterfacePrivate;
        iface->index = ifindex;
        interfaces << iface;

#ifdef SIOCGIFNAME
        // Get the canonical name
        QByteArray oldName = req.ifr_name;
        if (qt_safe_ioctl(socket, SIOCGIFNAME, &req) >= 0) {
            iface->name = QString::fromLatin1(req.ifr_name);

            // reset the name:
            memcpy(req.ifr_name, oldName, qMin<int>(oldName.length() + 1, sizeof(req.ifr_name) - 1));
        } else
#endif
	{
            // use this name anyways
            iface->name = QString::fromLatin1(req.ifr_name);
        }

        // Get interface flags
        if (qt_safe_ioctl(socket, SIOCGIFFLAGS, &req) >= 0) {
            iface->flags = convertFlags(req.ifr_flags);
        }

#ifdef SIOCGIFHWADDR
        // Get the HW address
        if (qt_safe_ioctl(socket, SIOCGIFHWADDR, &req) >= 0) {
            uchar *addr = (uchar *)req.ifr_addr.sa_data;
            iface->hardwareAddress = iface->makeHwAddress(6, addr);
        }
#endif
    }

    return iface;
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    int socket;
    if ((socket = qt_safe_socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
        return interfaces;      // error

    QSet<QByteArray> names = interfaceNames(socket);
    QSet<QByteArray>::ConstIterator it = names.constBegin();
    for ( ; it != names.constEnd(); ++it) {
        ifreq req;
        memset(&req, 0, sizeof(ifreq));
        memcpy(req.ifr_name, *it, qMin<int>(it->length() + 1, sizeof(req.ifr_name) - 1));

        QNetworkInterfacePrivate *iface = findInterface(socket, interfaces, req);

        // Get the interface broadcast address
        QNetworkAddressEntry entry;
        if (iface->flags & QNetworkInterface::CanBroadcast) {
            if (qt_safe_ioctl(socket, SIOCGIFBRDADDR, &req) >= 0) {
                sockaddr *sa = &req.ifr_addr;
                if (sa->sa_family == AF_INET)
                    entry.setBroadcast(addressFromSockaddr(sa));
            }
        }

        // Get the interface netmask
        if (qt_safe_ioctl(socket, SIOCGIFNETMASK, &req) >= 0) {
            sockaddr *sa = &req.ifr_addr;
	    entry.setNetmask(addressFromSockaddr(sa));
        }

        // Get the address of the interface
        if (qt_safe_ioctl(socket, SIOCGIFADDR, &req) >= 0) {
            sockaddr *sa = &req.ifr_addr;
            entry.setIp(addressFromSockaddr(sa));
        }

        iface->addressEntries << entry;
    }

    ::close(socket);
    return interfaces;
}

#else
// use getifaddrs

// platform-specific defs:
# ifdef Q_OS_LINUX
QT_BEGIN_INCLUDE_NAMESPACE
#  include <features.h>
QT_END_INCLUDE_NAMESPACE
# endif

# if defined(Q_OS_LINUX) &&  __GLIBC__ - 0 >= 2 && __GLIBC_MINOR__ - 0 >= 1
#  include <netpacket/packet.h>

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    QList<QNetworkInterfacePrivate *> interfaces;

    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next) {
        if ( !ptr->ifa_addr )
            continue;

        // Get the interface index
        int ifindex = if_nametoindex(ptr->ifa_name);

        // on Linux we use AF_PACKET and sockaddr_ll to obtain hHwAddress
        QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
        for ( ; if_it != interfaces.end(); ++if_it)
            if ((*if_it)->index == ifindex) {
                // this one has been added already
                if ( ptr->ifa_addr->sa_family == AF_PACKET
                        && (*if_it)->hardwareAddress.isEmpty()) {
                    sockaddr_ll *sll = (sockaddr_ll *)ptr->ifa_addr;
                    (*if_it)->hardwareAddress = (*if_it)->makeHwAddress(sll->sll_halen, (uchar*)sll->sll_addr);
                }
                break;
            }
        if ( if_it != interfaces.end() ) 
            continue;
        
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;
        iface->index = ifindex;
        iface->name = QString::fromLatin1(ptr->ifa_name);
        iface->flags = convertFlags(ptr->ifa_flags);

        if ( ptr->ifa_addr->sa_family == AF_PACKET ) {
            sockaddr_ll *sll = (sockaddr_ll *)ptr->ifa_addr;
            iface->hardwareAddress = iface->makeHwAddress(sll->sll_halen, (uchar*)sll->sll_addr);
        }
    }

    return interfaces;
}

# elif defined(Q_OS_BSD4)
QT_BEGIN_INCLUDE_NAMESPACE
#  include <net/if_dl.h>
QT_END_INCLUDE_NAMESPACE

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    QList<QNetworkInterfacePrivate *> interfaces;

    // on NetBSD we use AF_LINK and sockaddr_dl
    // scan the list for that family
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next)
        if (ptr->ifa_addr && ptr->ifa_addr->sa_family == AF_LINK) {
            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;

            sockaddr_dl *sdl = (sockaddr_dl *)ptr->ifa_addr;
            iface->index = sdl->sdl_index;
            iface->name = QString::fromLatin1(ptr->ifa_name);
            iface->flags = convertFlags(ptr->ifa_flags);
            iface->hardwareAddress = iface->makeHwAddress(sdl->sdl_alen, (uchar*)LLADDR(sdl));
        }

    return interfaces;
}

# else  // Generic version

static QList<QNetworkInterfacePrivate *> createInterfaces(ifaddrs *rawList)
{
    QList<QNetworkInterfacePrivate *> interfaces;

    // make sure there's one entry for each interface
    for (ifaddrs *ptr = rawList; ptr; ptr = ptr->ifa_next) {
        // Get the interface index
        int ifindex = if_nametoindex(ptr->ifa_name);

        QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
        for ( ; if_it != interfaces.end(); ++if_it)
            if ((*if_it)->index == ifindex)
                // this one has been added already
                break;

        if (if_it == interfaces.end()) {
            // none found, create
            QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
            interfaces << iface;

            iface->index = ifindex;
            iface->name = QString::fromLatin1(ptr->ifa_name);
            iface->flags = convertFlags(ptr->ifa_flags);
        }
    }

    return interfaces;
}

# endif


static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    int socket;
    if ((socket = qt_safe_socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1)
        return interfaces;      // error

    ifaddrs *interfaceListing;
    if (getifaddrs(&interfaceListing) == -1) {
        // error
        ::close(socket);
        return interfaces;
    }

    interfaces = createInterfaces(interfaceListing);
    for (ifaddrs *ptr = interfaceListing; ptr; ptr = ptr->ifa_next) {
        // Get the interface index
        int ifindex = if_nametoindex(ptr->ifa_name);
        QNetworkInterfacePrivate *iface = 0;
        QList<QNetworkInterfacePrivate *>::Iterator if_it = interfaces.begin();
        for ( ; if_it != interfaces.end(); ++if_it)
            if ((*if_it)->index == ifindex) {
                // found this interface already
                iface = *if_it;
                break;
            }
        if (!iface) {
            // skip all non-IP interfaces
            continue;
        }

        QNetworkAddressEntry entry;
        entry.setIp(addressFromSockaddr(ptr->ifa_addr));
        if (entry.ip().isNull())
            // could not parse the address
            continue;

        entry.setNetmask(addressFromSockaddr(ptr->ifa_netmask));
        if (iface->flags & QNetworkInterface::CanBroadcast)
            entry.setBroadcast(addressFromSockaddr(ptr->ifa_broadaddr));

        iface->addressEntries << entry;
    }

    freeifaddrs(interfaceListing);
    ::close(socket);
    return interfaces;
}
#endif

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
