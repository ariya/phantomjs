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

#include "qdnslookup_p.h"

#include <qlibrary.h>
#include <qscopedpointer.h>
#include <qurl.h>
#include <private/qmutexpool_p.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <resolv.h>

#if defined(__GNU_LIBRARY__) && !defined(__UCLIBC__)
#  include <gnu/lib-names.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY

typedef int (*dn_expand_proto)(const unsigned char *, const unsigned char *, const unsigned char *, char *, int);
static dn_expand_proto local_dn_expand = 0;
typedef void (*res_nclose_proto)(res_state);
static res_nclose_proto local_res_nclose = 0;
typedef int (*res_ninit_proto)(res_state);
static res_ninit_proto local_res_ninit = 0;
typedef int (*res_nquery_proto)(res_state, const char *, int, int, unsigned char *, int);
static res_nquery_proto local_res_nquery = 0;

// Custom deleter to close resolver state.

struct QDnsLookupStateDeleter
{
    static inline void cleanup(struct __res_state *pointer)
    {
        local_res_nclose(pointer);
    }
};

static void resolveLibrary()
{
    QLibrary lib;
#ifdef LIBRESOLV_SO
    lib.setFileName(QStringLiteral(LIBRESOLV_SO));
    if (!lib.load())
#endif
    {
        lib.setFileName(QLatin1String("resolv"));
        if (!lib.load())
            return;
    }

    local_dn_expand = dn_expand_proto(lib.resolve("__dn_expand"));
    if (!local_dn_expand)
        local_dn_expand = dn_expand_proto(lib.resolve("dn_expand"));

    local_res_nclose = res_nclose_proto(lib.resolve("__res_nclose"));
    if (!local_res_nclose)
        local_res_nclose = res_nclose_proto(lib.resolve("res_9_nclose"));
    if (!local_res_nclose)
        local_res_nclose = res_nclose_proto(lib.resolve("res_nclose"));

    local_res_ninit = res_ninit_proto(lib.resolve("__res_ninit"));
    if (!local_res_ninit)
        local_res_ninit = res_ninit_proto(lib.resolve("res_9_ninit"));
    if (!local_res_ninit)
        local_res_ninit = res_ninit_proto(lib.resolve("res_ninit"));

    local_res_nquery = res_nquery_proto(lib.resolve("__res_nquery"));
    if (!local_res_nquery)
        local_res_nquery = res_nquery_proto(lib.resolve("res_9_nquery"));
    if (!local_res_nquery)
        local_res_nquery = res_nquery_proto(lib.resolve("res_nquery"));
}

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    // Load dn_expand, res_ninit and res_nquery on demand.
    static QBasicAtomicInt triedResolve = Q_BASIC_ATOMIC_INITIALIZER(false);
    if (!triedResolve.loadAcquire()) {
        QMutexLocker locker(QMutexPool::globalInstanceGet(&local_res_ninit));
        if (!triedResolve.load()) {
            resolveLibrary();
            triedResolve.storeRelease(true);
        }
    }

    // If dn_expand, res_ninit or res_nquery is missing, fail.
    if (!local_dn_expand || !local_res_nclose || !local_res_ninit || !local_res_nquery) {
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = tr("Resolver functions not found");
        return;
    }

    // Initialize state.
    struct __res_state state;
    memset(&state, 0, sizeof(state));
    if (local_res_ninit(&state) < 0) {
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = tr("Resolver initialization failed");
        return;
    }

    //Check if a nameserver was set. If so, use it
    if (!nameserver.isNull()) {
        if (nameserver.protocol() == QAbstractSocket::IPv4Protocol) {
            state.nsaddr_list[0].sin_addr.s_addr = htonl(nameserver.toIPv4Address());
            state.nscount = 1;
        } else if (nameserver.protocol() == QAbstractSocket::IPv6Protocol) {
#if defined(Q_OS_LINUX)
            struct sockaddr_in6 *ns;
            ns = state._u._ext.nsaddrs[0];
            // nsaddrs will be NULL if no nameserver is set in /etc/resolv.conf
            if (!ns) {
                // Memory allocated here will be free'd in res_close() as we
                // have done res_init() above.
                ns = (struct sockaddr_in6*) calloc(1, sizeof(struct sockaddr_in6));
                Q_CHECK_PTR(ns);
                state._u._ext.nsaddrs[0] = ns;
            }
            // Set nsmap[] to indicate that nsaddrs[0] is an IPv6 address
            // See: https://sourceware.org/ml/libc-hacker/2002-05/msg00035.html
            state._u._ext.nsmap[0] = MAXNS + 1;
            state._u._ext.nscount6 = 1;
            ns->sin6_family = AF_INET6;
            ns->sin6_port = htons(53);

            Q_IPV6ADDR ipv6Address = nameserver.toIPv6Address();
            for (int i=0; i<16; i++) {
                ns->sin6_addr.s6_addr[i] = ipv6Address[i];
            }
#else
            qWarning() << Q_FUNC_INFO << "IPv6 addresses for nameservers is currently not supported";
            reply->error = QDnsLookup::ResolverError;
            reply->errorString = tr("IPv6 addresses for nameservers is currently not supported");
            return;
#endif
        }
    }
#ifdef QDNSLOOKUP_DEBUG
    state.options |= RES_DEBUG;
#endif
    QScopedPointer<struct __res_state, QDnsLookupStateDeleter> state_ptr(&state);

    // Perform DNS query.
    unsigned char response[PACKETSZ];
    memset(response, 0, sizeof(response));
    const int responseLength = local_res_nquery(&state, requestName, C_IN, requestType, response, sizeof(response));

    // Check the response header.
    HEADER *header = (HEADER*)response;
    const int answerCount = ntohs(header->ancount);
    switch (header->rcode) {
    case NOERROR:
        break;
    case FORMERR:
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = tr("Server could not process query");
        return;
    case SERVFAIL:
        reply->error = QDnsLookup::ServerFailureError;
        reply->errorString = tr("Server failure");
        return;
    case NXDOMAIN:
        reply->error = QDnsLookup::NotFoundError;
        reply->errorString = tr("Non existent domain");
        return;
    case REFUSED:
        reply->error = QDnsLookup::ServerRefusedError;
        reply->errorString = tr("Server refused to answer");
        return;
    default:
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    // Check the reply is valid.
    if (responseLength < int(sizeof(HEADER))) {
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    // Skip the query host, type (2 bytes) and class (2 bytes).
    char host[PACKETSZ], answer[PACKETSZ];
    unsigned char *p = response + sizeof(HEADER);
    int status = local_dn_expand(response, response + responseLength, p, host, sizeof(host));
    if (status < 0) {
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Could not expand domain name");
        return;
    }
    p += status + 4;

    // Extract results.
    int answerIndex = 0;
    while ((p < response + responseLength) && (answerIndex < answerCount)) {
        status = local_dn_expand(response, response + responseLength, p, host, sizeof(host));
        if (status < 0) {
            reply->error = QDnsLookup::InvalidReplyError;
            reply->errorString = tr("Could not expand domain name");
            return;
        }
        const QString name = QUrl::fromAce(host);

        p += status;
        const quint16 type = (p[0] << 8) | p[1];
        p += 2; // RR type
        p += 2; // RR class
        const quint32 ttl = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p += 4;
        const quint16 size = (p[0] << 8) | p[1];
        p += 2;

        if (type == QDnsLookup::A) {
            if (size != 4) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid IPv4 address record");
                return;
            }
            const quint32 addr = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(addr);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::AAAA) {
            if (size != 16) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid IPv6 address record");
                return;
            }
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(p);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::CNAME) {
            status = local_dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid canonical name record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->canonicalNameRecords.append(record);
        } else if (type == QDnsLookup::NS) {
            status = local_dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid name server record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->nameServerRecords.append(record);
        } else if (type == QDnsLookup::PTR) {
            status = local_dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid pointer record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->pointerRecords.append(record);
        } else if (type == QDnsLookup::MX) {
            const quint16 preference = (p[0] << 8) | p[1];
            status = local_dn_expand(response, response + responseLength, p + 2, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid mail exchange record");
                return;
            }
            QDnsMailExchangeRecord record;
            record.d->exchange = QUrl::fromAce(answer);
            record.d->name = name;
            record.d->preference = preference;
            record.d->timeToLive = ttl;
            reply->mailExchangeRecords.append(record);
        } else if (type == QDnsLookup::SRV) {
            const quint16 priority = (p[0] << 8) | p[1];
            const quint16 weight = (p[2] << 8) | p[3];
            const quint16 port = (p[4] << 8) | p[5];
            status = local_dn_expand(response, response + responseLength, p + 6, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid service record");
                return;
            }
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = QUrl::fromAce(answer);
            record.d->port = port;
            record.d->priority = priority;
            record.d->timeToLive = ttl;
            record.d->weight = weight;
            reply->serviceRecords.append(record);
        } else if (type == QDnsLookup::TXT) {
            unsigned char *txt = p;
            QDnsTextRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            while (txt < p + size) {
                const unsigned char length = *txt;
                txt++;
                if (txt + length > p + size) {
                    reply->error = QDnsLookup::InvalidReplyError;
                    reply->errorString = tr("Invalid text record");
                    return;
                }
                record.d->values << QByteArray((char*)txt, length);
                txt += length;
            }
            reply->textRecords.append(record);
        }
        p += size;
        answerIndex++;
    }
}

#else

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, QDnsLookupReply *reply)
{
    Q_UNUSED(requestType)
    Q_UNUSED(requestName)
    reply->error = QDnsLookup::ResolverError;
    reply->errorString = tr("Resolver library can't be loaded: No runtime library loading support");
    return;
}

#endif /* ifndef QT_NO_LIBRARY */

QT_END_NAMESPACE
