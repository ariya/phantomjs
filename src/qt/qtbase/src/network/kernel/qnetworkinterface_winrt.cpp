/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.networking.h>
#include <windows.networking.connectivity.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;

#include <qhostinfo.h>

QT_BEGIN_NAMESPACE

struct HostNameInfo {
    GUID adapterId;
    unsigned char prefixLength;
    QString address;
};

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    QList<QNetworkInterfacePrivate *> interfaces;

    QList<HostNameInfo> hostList;

    INetworkInformationStatics *hostNameStatics;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &hostNameStatics);

    IVectorView<HostName*> *hostNames = 0;
    hostNameStatics->GetHostNames(&hostNames);
    hostNameStatics->Release();
    if (!hostNames)
        return interfaces;

    unsigned int hostNameCount;
    hostNames->get_Size(&hostNameCount);
    for (unsigned i = 0; i < hostNameCount; ++i) {
        HostNameInfo hostInfo;
        IHostName *hostName;
        hostNames->GetAt(i, &hostName);

        HostNameType type;
        hostName->get_Type(&type);
        if (type == HostNameType_DomainName)
            continue;

        IIPInformation *ipInformation;
        hostName->get_IPInformation(&ipInformation);
        INetworkAdapter *currentAdapter;
        ipInformation->get_NetworkAdapter(&currentAdapter);

        currentAdapter->get_NetworkAdapterId(&hostInfo.adapterId);
        currentAdapter->Release();

        IReference<unsigned char> *prefixLengthReference;
        ipInformation->get_PrefixLength(&prefixLengthReference);
        ipInformation->Release();

        prefixLengthReference->get_Value(&hostInfo.prefixLength);
        prefixLengthReference->Release();

        // invalid prefixes
        if ((type == HostNameType_Ipv4 && hostInfo.prefixLength > 32)
                || (type == HostNameType_Ipv6 && hostInfo.prefixLength > 128))
            continue;

        HSTRING name;
        hostName->get_CanonicalName(&name);
        hostName->Release();
        UINT32 length;
        PCWSTR rawString = WindowsGetStringRawBuffer(name, &length);
        hostInfo.address = QString::fromWCharArray(rawString, length);

        hostList << hostInfo;
    }
    hostNames->Release();

    INetworkInformationStatics *networkInfoStatics;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &networkInfoStatics);
    IVectorView<ConnectionProfile *> *connectionProfiles = 0;
    networkInfoStatics->GetConnectionProfiles(&connectionProfiles);
    networkInfoStatics->Release();
    if (!connectionProfiles)
        return interfaces;

    unsigned int size;
    connectionProfiles->get_Size(&size);
    for (unsigned int i = 0; i < size; ++i) {
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        IConnectionProfile *profile;
        connectionProfiles->GetAt(i, &profile);

        NetworkConnectivityLevel connectivityLevel;
        profile->GetNetworkConnectivityLevel(&connectivityLevel);
        if (connectivityLevel != NetworkConnectivityLevel_None)
            iface->flags = QNetworkInterface::IsUp | QNetworkInterface::IsRunning;

        INetworkAdapter *adapter;
        profile->get_NetworkAdapter(&adapter);
        profile->Release();
        UINT32 type;
        adapter->get_IanaInterfaceType(&type);
        if (type == 23)
            iface->flags |= QNetworkInterface::IsPointToPoint;
        GUID id;
        adapter->get_NetworkAdapterId(&id);
        adapter->Release();
        OLECHAR adapterName[39]={0};
        StringFromGUID2(id, adapterName, 39);
        iface->name = QString::fromWCharArray(adapterName);

        // According to http://stackoverflow.com/questions/12936193/how-unique-is-the-ethernet-network-adapter-id-in-winrt-it-is-derived-from-the-m
        // obtaining the MAC address using WinRT API is impossible
        // iface->hardwareAddress = ?

        for (int i = 0; i < hostList.length(); ++i) {
            const HostNameInfo hostInfo = hostList.at(i);
            if (id != hostInfo.adapterId)
                continue;

            QNetworkAddressEntry entry;
            entry.setIp(QHostAddress(hostInfo.address));
            entry.setPrefixLength(hostInfo.prefixLength);
            iface->addressEntries << entry;

            hostList.takeAt(i);
            --i;
        }
    }
    connectionProfiles->Release();
    return interfaces;
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

QString QHostInfo::localDomainName()
{
    return QString();
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE
