/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork of the Qt Toolkit.
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

//#define QNETWORKINTERFACE_DEBUG

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"
#include <private/qcore_symbian_p.h>

#ifndef QT_NO_NETWORKINTERFACE

#include <in_sock.h>
#include <in_iface.h>
#include <es_sock.h>

QT_BEGIN_NAMESPACE


static QNetworkInterface::InterfaceFlags convertFlags(const TSoInetInterfaceInfo& aInfo)
{
    QNetworkInterface::InterfaceFlags flags = 0;
    flags |= (aInfo.iState == EIfUp) ? QNetworkInterface::IsUp : QNetworkInterface::InterfaceFlag(0);
    // We do not have separate flag for running in Symbian OS
    flags |= (aInfo.iState == EIfUp) ? QNetworkInterface::IsRunning : QNetworkInterface::InterfaceFlag(0);
    flags |= (aInfo.iFeatures & KIfCanBroadcast) ? QNetworkInterface::CanBroadcast : QNetworkInterface::InterfaceFlag(0);
    flags |= (aInfo.iFeatures & KIfIsLoopback) ? QNetworkInterface::IsLoopBack : QNetworkInterface::InterfaceFlag(0);
    flags |= (aInfo.iFeatures & KIfIsPointToPoint) ? QNetworkInterface::IsPointToPoint : QNetworkInterface::InterfaceFlag(0);
    flags |= (aInfo.iFeatures & KIfCanMulticast) ? QNetworkInterface::CanMulticast : QNetworkInterface::InterfaceFlag(0);
    return flags;
}

QHostAddress qt_QHostAddressFromTInetAddr(const TInetAddr& addr)
{
    if (addr.IsV4Mapped() || addr.Family() == KAfInet) {
        //convert v4 host address
        return QHostAddress(addr.Address());
    } else {
        //convert v6 host address
        return QHostAddress((quint8 *)(addr.Ip6Address().u.iAddr8));
    }
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    TInt err(KErrNone);
    QList<QNetworkInterfacePrivate *> interfaces;
    QList<QHostAddress> addressesWithEstimatedNetmasks;

    // Open dummy socket for interface queries
    RSocket socket;
    err = socket.Open(qt_symbianGetSocketServer(), _L("udp"));
    if (err) {
        return interfaces;
    }

    // Ask socket to start enumerating interfaces
    err =  socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
    if (err) {
        socket.Close();
        return interfaces;
    }

    int ifindex = 0;
    TPckgBuf<TSoInetInterfaceInfo> infoPckg;
    TSoInetInterfaceInfo &info = infoPckg();
    while (socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, infoPckg) == KErrNone) {
        if (info.iName != KNullDesC) {
            TName address;
            QNetworkAddressEntry entry;
            QNetworkInterfacePrivate *iface = 0;

            iface = new QNetworkInterfacePrivate;
            iface->index = ifindex++;
            interfaces << iface;
            iface->name = qt_TDesC2QString(info.iName);
            iface->flags = convertFlags(info);

            if (/*info.iFeatures&KIfHasHardwareAddr &&*/ info.iHwAddr.Family() != KAFUnspec) {
                for (TInt i = sizeof(SSockAddr); i < sizeof(SSockAddr) + info.iHwAddr.GetUserLen(); i++) {
                    address.AppendNumFixedWidth(info.iHwAddr[i], EHex, 2);
                    if ((i + 1) < sizeof(SSockAddr) + info.iHwAddr.GetUserLen())
                        address.Append(_L(":"));
                }
                address.UpperCase();
                iface->hardwareAddress = qt_TDesC2QString(address);
            }

            // Get the address of the interface
            entry.setIp(qt_QHostAddressFromTInetAddr(info.iAddress));

#if defined(QNETWORKINTERFACE_DEBUG)
            qDebug() << "address is" << info.iAddress.Family() << entry.ip();
            qDebug() << "netmask is" << info.iNetMask.Family() << qt_QHostAddressFromTInetAddr( info.iNetMask );
#endif

            // Get the interface netmask
            if (info.iNetMask.IsUnspecified()) {
                // For some reason netmask is always 0.0.0.0 for IPv4 interfaces
                // and loopback interfaces (which we statically know)
                if (info.iAddress.IsV4Mapped()) {
                    if (info.iFeatures & KIfIsLoopback) {
                        entry.setPrefixLength(32);
                    } else {
                        // Workaround: Let Symbian determine netmask based on IP address class (IPv4 only API)
                        TInetAddr netmask;
                        netmask.NetMask(info.iAddress);
                        entry.setNetmask(QHostAddress(netmask.Address())); //binary convert v4 address
                        addressesWithEstimatedNetmasks << entry.ip();
#if defined(QNETWORKINTERFACE_DEBUG)
                        qDebug() << "address class determined netmask" << entry.netmask();
#endif
                    }
                } else {
                    // For IPv6 interfaces
                    if (info.iFeatures & KIfIsLoopback) {
                        entry.setPrefixLength(128);
                    } else if (info.iNetMask.IsUnspecified()) {
                        //Don't see this error for IPv6, but try to handle it if it happens
                        entry.setPrefixLength(64); //most common
#if defined(QNETWORKINTERFACE_DEBUG)
                        qDebug() << "total guess netmask" << entry.netmask();
#endif
                        addressesWithEstimatedNetmasks << entry.ip();
                    }
                }
            } else {
                //Expected code path for IPv6 non loopback interfaces (IPv4 could come here if symbian is fixed)
                entry.setNetmask(qt_QHostAddressFromTInetAddr(info.iNetMask));
#if defined(QNETWORKINTERFACE_DEBUG)
                qDebug() << "reported netmask" << entry.netmask();
#endif
            }

            // broadcast address is determined from the netmask in postProcess()

            // Add new entry to interface address entries
            iface->addressEntries << entry;

#if defined(QNETWORKINTERFACE_DEBUG)
            qDebug("\n       Found network interface %s, interface flags:\n\
                IsUp = %d, IsRunning = %d, CanBroadcast = %d,\n\
                IsLoopBack = %d, IsPointToPoint = %d, CanMulticast = %d, \n\
                ip = %s, netmask = %s, broadcast = %s,\n\
                hwaddress = %s",
                   iface->name.toLatin1().constData(),
                   iface->flags & QNetworkInterface::IsUp, iface->flags & QNetworkInterface::IsRunning, iface->flags & QNetworkInterface::CanBroadcast,
                   iface->flags & QNetworkInterface::IsLoopBack, iface->flags & QNetworkInterface::IsPointToPoint, iface->flags & QNetworkInterface::CanMulticast,
                   entry.ip().toString().toLatin1().constData(), entry.netmask().toString().toLatin1().constData(), entry.broadcast().toString().toLatin1().constData(),
                   iface->hardwareAddress.toLatin1().constData());
#endif
        }
    }

    // if we didn't have to guess any netmasks, then we're done.
    if (addressesWithEstimatedNetmasks.isEmpty()) {
        socket.Close();
        return interfaces;
    }

    // we will try to use routing info to detect more precisely
    // estimated netmasks and then ::postProcess() should calculate
    // broadcast addresses

    // use dummy socket to start enumerating routes
    err =  socket.SetOpt(KSoInetEnumRoutes, KSolInetRtCtrl);
    if (err) {
        socket.Close();
        // return what we have
        // up to this moment
        return interfaces;
    }

    TSoInetRouteInfo routeInfo;
    TPckg<TSoInetRouteInfo> routeInfoPkg(routeInfo);
    while (socket.GetOpt(KSoInetNextRoute, KSolInetRtCtrl, routeInfoPkg) == KErrNone) {
        // get interface address
        QHostAddress ifAddr(qt_QHostAddressFromTInetAddr(routeInfo.iIfAddr));
        if (ifAddr.isNull())
            continue;
        if (!addressesWithEstimatedNetmasks.contains(ifAddr)) {
#if defined(QNETWORKINTERFACE_DEBUG)
            qDebug() << "skipping route from" << ifAddr << "because it wasn't an estimated netmask";
#endif
            continue;
        }

        QHostAddress destination(qt_QHostAddressFromTInetAddr(routeInfo.iDstAddr));
#if defined(QNETWORKINTERFACE_DEBUG)
        qDebug() << "route from" << ifAddr << "to" << destination;
#endif
        if (destination.isNull() || destination != ifAddr)
            continue;

        // search interfaces
        for (int ifindex = 0; ifindex < interfaces.size(); ++ifindex) {
            QNetworkInterfacePrivate *iface = interfaces.at(ifindex);
            for (int eindex = 0; eindex < iface->addressEntries.size(); ++eindex) {
                QNetworkAddressEntry entry = iface->addressEntries.at(eindex);
                if (entry.ip() != ifAddr) {
                    continue;
                } else if (!routeInfo.iNetMask.IsUnspecified()) {
                    //the route may also return 0.0.0.0 netmask, in which case don't use it.
                    QHostAddress netmask(qt_QHostAddressFromTInetAddr(routeInfo.iNetMask));
                    entry.setNetmask(netmask);
#if defined(QNETWORKINTERFACE_DEBUG)
                    qDebug() << " - route netmask" << routeInfo.iNetMask.Family() << netmask << " (using route determined netmask)";
#endif
                    iface->addressEntries.replace(eindex, entry);
                }
            }
        }
    }

    socket.Close();

    return interfaces;
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
