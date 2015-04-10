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

#include "qhostinfo_p.h"

#include <qurl.h>

#include <ppltasks.h>
#include <wrl.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>
#include <windows.networking.connectivity.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Networking::Sockets;

QT_BEGIN_NAMESPACE

//#define QHOSTINFO_DEBUG

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QHostInfo results;

    QHostAddress address;
    if (address.setAddress(hostName)) {
        // Reverse lookup
        // TODO: is there a replacement for getnameinfo for winrt?
        Q_UNIMPLEMENTED();
        return results;
    }

    QByteArray aceHostname = QUrl::toAce(hostName);
    results.setHostName(hostName);
    if (aceHostname.isEmpty()) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(hostName.isEmpty() ? tr("No host name given") : tr("Invalid hostname"));
        return results;
    }

    IHostNameFactory *hostnameFactory;

    HStringReference classId(RuntimeClass_Windows_Networking_HostName);
    if (FAILED(GetActivationFactory(classId.Get(), &hostnameFactory)))
        Q_ASSERT(false, "Could not obtain hostname factory.");

    IHostName *host;
    HStringReference hostNameRef((const wchar_t*)hostName.utf16());
    hostnameFactory->CreateHostName(hostNameRef.Get(), &host);
    hostnameFactory->Release();

    IDatagramSocketStatics *datagramSocketStatics;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(), &datagramSocketStatics);

    IAsyncOperation<IVectorView<EndpointPair*> *> *op;
    HSTRING proto;
    WindowsCreateString(L"0", 1, &proto);
    datagramSocketStatics->GetEndpointPairsAsync(host, proto, &op);
    datagramSocketStatics->Release();
    host->Release();

    IVectorView<EndpointPair*> *endpointPairs = 0;
    HRESULT hr = op->GetResults(&endpointPairs);
    int waitCount = 0;
    while (hr == E_ILLEGAL_METHOD_CALL) {
        WaitForSingleObjectEx(GetCurrentThread(), 50, FALSE);
        hr = op->GetResults(&endpointPairs);
        if (++waitCount > 1200) // Wait for 1 minute max
            return results;
    }
    op->Release();

    if (!endpointPairs)
        return results;

    unsigned int size;
    endpointPairs->get_Size(&size);
    QList<QHostAddress> addresses;
    for (unsigned int i = 0; i < size; ++i) {
        IEndpointPair *endpointpair;
        endpointPairs->GetAt(i, &endpointpair);
        IHostName *remoteHost;
        endpointpair->get_RemoteHostName(&remoteHost);
        endpointpair->Release();
        if (!remoteHost)
            continue;
        HostNameType type;
        remoteHost->get_Type(&type);
        if (type == HostNameType_DomainName)
            continue;

        HSTRING name;
        remoteHost->get_CanonicalName(&name);
        remoteHost->Release();
        UINT32 length;
        PCWSTR rawString = WindowsGetStringRawBuffer(name, &length);
        QHostAddress addr;
        addr.setAddress(QString::fromWCharArray(rawString, length));
        if (!addresses.contains(addr))
            addresses.append(addr);
    }
    results.setAddresses(addresses);

    return results;
}

QString QHostInfo::localHostName()
{
    INetworkInformationStatics *statics;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &statics);

    IVectorView<HostName*> *hostNames = 0;
    statics->GetHostNames(&hostNames);
    statics->Release();
    if (!hostNames)
        return QString();

    unsigned int size;
    hostNames->get_Size(&size);
    if (size == 0)
        return QString();

    for (unsigned int i = 0; i < size; ++i) {
        IHostName *hostName;
        hostNames->GetAt(i, &hostName);
        HostNameType type;
        hostName->get_Type(&type);
        if (type != HostNameType_DomainName)
            continue;

        HSTRING name;
        hostName->get_CanonicalName(&name);
        hostName->Release();
        UINT32 length;
        PCWSTR rawString = WindowsGetStringRawBuffer(name, &length);
        return QString::fromWCharArray(rawString, length);
    }
    IHostName *firstHost;
    hostNames->GetAt(0, &firstHost);
    hostNames->Release();

    HSTRING name;
    firstHost->get_CanonicalName(&name);
    firstHost->Release();
    UINT32 length;
    PCWSTR rawString = WindowsGetStringRawBuffer(name, &length);
    return QString::fromWCharArray(rawString, length);
}

// QString QHostInfo::localDomainName() defined in qnetworkinterface_win.cpp

QT_END_NAMESPACE
