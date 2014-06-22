/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
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

#include <winsock2.h>
#include "qdnslookup_p.h"

#include <qurl.h>
#include <private/qmutexpool_p.h>
#include <private/qsystemerror_p.h>

#include <qt_windows.h>
#include <windns.h>
#include <memory.h>

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    // Perform DNS query.
    PDNS_RECORD dns_records = 0;
    const QString requestNameUtf16 = QString::fromUtf8(requestName.data(), requestName.size());
    IP4_ARRAY srvList;
    memset(&srvList, 0, sizeof(IP4_ARRAY));
    if (!nameserver.isNull()) {
        if (nameserver.protocol() == QAbstractSocket::IPv4Protocol) {
            // The below code is referenced from: http://support.microsoft.com/kb/831226
            srvList.AddrCount = 1;
            srvList.AddrArray[0] = htonl(nameserver.toIPv4Address());
        } else if (nameserver.protocol() == QAbstractSocket::IPv6Protocol) {
            // For supoprting IPv6 nameserver addresses, we'll need to switch
            // from DnsQuey() to DnsQueryEx() as it supports passing an IPv6
            // address in the nameserver list
            qWarning() << Q_FUNC_INFO << "IPv6 addresses for nameservers is currently not supported";
            reply->error = QDnsLookup::ResolverError;
            reply->errorString = tr("IPv6 addresses for nameservers is currently not supported");
            return;
        }
    }
    const DNS_STATUS status = DnsQuery_W(reinterpret_cast<const wchar_t*>(requestNameUtf16.utf16()), requestType, DNS_QUERY_STANDARD, &srvList, &dns_records, NULL);
    switch (status) {
    case ERROR_SUCCESS:
        break;
    case DNS_ERROR_RCODE_FORMAT_ERROR:
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = tr("Server could not process query");
        return;
    case DNS_ERROR_RCODE_SERVER_FAILURE:
        reply->error = QDnsLookup::ServerFailureError;
        reply->errorString = tr("Server failure");
        return;
    case DNS_ERROR_RCODE_NAME_ERROR:
        reply->error = QDnsLookup::NotFoundError;
        reply->errorString = tr("Non existent domain");
        return;
    case DNS_ERROR_RCODE_REFUSED:
        reply->error = QDnsLookup::ServerRefusedError;
        reply->errorString = tr("Server refused to answer");
        return;
    default:
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = QSystemError(status, QSystemError::NativeError).toString();
        return;
    }

    // Extract results.
    for (PDNS_RECORD ptr = dns_records; ptr != NULL; ptr = ptr->pNext) {
        const QString name = QUrl::fromAce( QString::fromWCharArray( ptr->pName ).toLatin1() );
        if (ptr->wType == QDnsLookup::A) {
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QHostAddress(ntohl(ptr->Data.A.IpAddress));
            reply->hostAddressRecords.append(record);
        } else if (ptr->wType == QDnsLookup::AAAA) {
            Q_IPV6ADDR addr;
            memcpy(&addr, &ptr->Data.AAAA.Ip6Address, sizeof(Q_IPV6ADDR));

            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QHostAddress(addr);
            reply->hostAddressRecords.append(record);
        } else if (ptr->wType == QDnsLookup::CNAME) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QUrl::fromAce(QString::fromWCharArray(ptr->Data.Cname.pNameHost).toLatin1());
            reply->canonicalNameRecords.append(record);
        } else if (ptr->wType == QDnsLookup::MX) {
            QDnsMailExchangeRecord record;
            record.d->name = name;
            record.d->exchange = QUrl::fromAce(QString::fromWCharArray(ptr->Data.Mx.pNameExchange).toLatin1());
            record.d->preference = ptr->Data.Mx.wPreference;
            record.d->timeToLive = ptr->dwTtl;
            reply->mailExchangeRecords.append(record);
        } else if (ptr->wType == QDnsLookup::NS) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QUrl::fromAce(QString::fromWCharArray(ptr->Data.Ns.pNameHost).toLatin1());
            reply->nameServerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::PTR) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QUrl::fromAce(QString::fromWCharArray(ptr->Data.Ptr.pNameHost).toLatin1());
            reply->pointerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::SRV) {
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = QUrl::fromAce(QString::fromWCharArray(ptr->Data.Srv.pNameTarget).toLatin1());
            record.d->port = ptr->Data.Srv.wPort;
            record.d->priority = ptr->Data.Srv.wPriority;
            record.d->timeToLive = ptr->dwTtl;
            record.d->weight = ptr->Data.Srv.wWeight;
            reply->serviceRecords.append(record);
        } else if (ptr->wType == QDnsLookup::TXT) {
            QDnsTextRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            for (unsigned int i = 0; i < ptr->Data.Txt.dwStringCount; ++i) {
                record.d->values << QString::fromWCharArray((ptr->Data.Txt.pStringArray[i])).toLatin1();;
            }
            reply->textRecords.append(record);
        }
    }

    DnsRecordListFree(dns_records, DnsFreeRecordList);
}

QT_END_NAMESPACE
