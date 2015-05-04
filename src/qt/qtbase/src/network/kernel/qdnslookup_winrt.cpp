/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qdnslookup_p.h"

#include <qurl.h>
#include <qdebug.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Networking::Sockets;

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    // TODO: Add nameserver support for winRT
    if (!nameserver.isNull())
        qWarning() << "Ignoring nameserver as its currently not supported on WinRT";

    // TODO: is there any way to do "proper" dns lookup?
    if (requestType != QDnsLookup::A && requestType != QDnsLookup::AAAA
            && requestType != QDnsLookup::ANY) {
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = QLatin1String("WinRT only supports IPv4 and IPv6 requests");
        return;
    }

    QString aceHostname = QUrl::fromAce(requestName);
    if (aceHostname.isEmpty()) {
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = requestName.isEmpty() ? tr("No hostname given") : tr("Invalid hostname");
        return;
    }

    ComPtr<IHostNameFactory> hostnameFactory;
    HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                        IID_PPV_ARGS(&hostnameFactory));
    if (FAILED(hr)) {
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = QLatin1String("Could not obtain hostname factory");
        return;
    }
    ComPtr<IHostName> host;
    HStringReference hostNameRef((const wchar_t*)aceHostname.utf16());
    hostnameFactory->CreateHostName(hostNameRef.Get(), &host);

    ComPtr<IDatagramSocketStatics> datagramSocketStatics;
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(), &datagramSocketStatics);

    ComPtr<IAsyncOperation<IVectorView<EndpointPair *> *>> op;
    datagramSocketStatics->GetEndpointPairsAsync(host.Get(),
                                                 HString::MakeReference(L"0").Get(),
                                                 &op);

    ComPtr<IVectorView<EndpointPair *>> endpointPairs;
    hr = op->GetResults(&endpointPairs);
    int waitCount = 0;
    while (hr == E_ILLEGAL_METHOD_CALL) {
        WaitForSingleObjectEx(GetCurrentThread(), 50, FALSE);
        hr = op->GetResults(&endpointPairs);
        if (++waitCount > 1200) // Wait for 1 minute max
            return;
    }

    if (!endpointPairs)
        return;

    unsigned int size;
    endpointPairs->get_Size(&size);
    // endpoint pairs might contain duplicates so we temporarily store addresses in a QSet
    QSet<QHostAddress> addresses;
    for (unsigned int i = 0; i < size; ++i) {
        ComPtr<IEndpointPair> endpointpair;
        endpointPairs->GetAt(i, &endpointpair);
        ComPtr<IHostName> remoteHost;
        endpointpair->get_RemoteHostName(&remoteHost);
        HostNameType type;
        remoteHost->get_Type(&type);
        if (type == HostNameType_Bluetooth || type == HostNameType_DomainName
                || (requestType != QDnsLookup::ANY
                && ((type == HostNameType_Ipv4 && requestType == QDnsLookup::AAAA)
                || (type == HostNameType_Ipv6 && requestType == QDnsLookup::A))))
            continue;

        HString name;
        remoteHost->get_CanonicalName(name.GetAddressOf());
        UINT32 length;
        PCWSTR rawString = name.GetRawBuffer(&length);
        addresses.insert(QHostAddress(QString::fromWCharArray(rawString, length)));
    }
    foreach (const QHostAddress &address, addresses) {
        QDnsHostAddressRecord record;
        record.d->name = aceHostname;
        record.d->value = address;
        reply->hostAddressRecords.append(record);
    }
}

QT_END_NAMESPACE
