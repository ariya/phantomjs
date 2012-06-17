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

#include "qnetworkproxy.h"

#ifndef QT_NO_NETWORKPROXY

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/qendian.h>
#include <QtCore/qstringlist.h>
#include "private/qcore_mac_p.h"

/*
 * MacOS X has a proxy configuration module in System Preferences (on
 * MacOS X 10.5, it's in Network, Advanced), where one can set the
 * proxy settings for:
 *
 * \list
 *   \o FTP proxy
 *   \o Web Proxy (HTTP)
 *   \o Secure Web Proxy (HTTPS)
 *   \o Streaming Proxy (RTSP)
 *   \o SOCKS Proxy
 *   \o Gopher Proxy
 *   \o URL for Automatic Proxy Configuration (PAC scripts)
 *   \o Bypass list (by default: *.local, 169.254/16)
 * \endlist
 *
 * The matching configuration can be obtained by calling SCDynamicStoreCopyProxies
 * (from <SystemConfiguration/SCDynamicStoreCopySpecific.h>). See
 * Apple's documentation:
 *
 * http://developer.apple.com/DOCUMENTATION/Networking/Reference/SysConfig/SCDynamicStoreCopySpecific/CompositePage.html#//apple_ref/c/func/SCDynamicStoreCopyProxies
 *
 */

QT_BEGIN_NAMESPACE

static bool isHostExcluded(CFDictionaryRef dict, const QString &host)
{
    if (host.isEmpty())
        return true;

    bool isSimple = !host.contains(QLatin1Char('.')) && !host.contains(QLatin1Char(':'));
    CFNumberRef excludeSimples;
    if (isSimple &&
        (excludeSimples = (CFNumberRef)CFDictionaryGetValue(dict, kSCPropNetProxiesExcludeSimpleHostnames))) {
        int enabled;
        if (CFNumberGetValue(excludeSimples, kCFNumberIntType, &enabled) && enabled)
            return true;
    }

    QHostAddress ipAddress;
    bool isIpAddress = ipAddress.setAddress(host);

    // not a simple host name
    // does it match the list of exclusions?
    CFArrayRef exclusionList = (CFArrayRef)CFDictionaryGetValue(dict, kSCPropNetProxiesExceptionsList);
    if (!exclusionList)
        return false;

    CFIndex size = CFArrayGetCount(exclusionList);
    for (CFIndex i = 0; i < size; ++i) {
        CFStringRef cfentry = (CFStringRef)CFArrayGetValueAtIndex(exclusionList, i);
        QString entry = QCFString::toQString(cfentry);

        if (isIpAddress && ipAddress.isInSubnet(QHostAddress::parseSubnet(entry))) {
            return true;        // excluded
        } else {
            // do wildcard matching
            QRegExp rx(entry, Qt::CaseInsensitive, QRegExp::Wildcard);
            if (rx.exactMatch(host))
                return true;
        }
    }

    // host was not excluded
    return false;
}

static QNetworkProxy proxyFromDictionary(CFDictionaryRef dict, QNetworkProxy::ProxyType type,
                                         CFStringRef enableKey, CFStringRef hostKey,
                                         CFStringRef portKey)
{
    CFNumberRef protoEnabled;
    CFNumberRef protoPort;
    CFStringRef protoHost;
    if (enableKey
        && (protoEnabled = (CFNumberRef)CFDictionaryGetValue(dict, enableKey))
        && (protoHost = (CFStringRef)CFDictionaryGetValue(dict, hostKey))
        && (protoPort = (CFNumberRef)CFDictionaryGetValue(dict, portKey))) {
        int enabled;
        if (CFNumberGetValue(protoEnabled, kCFNumberIntType, &enabled) && enabled) {
            QString host = QCFString::toQString(protoHost);

            int port;
            CFNumberGetValue(protoPort, kCFNumberIntType, &port);

            return QNetworkProxy(type, host, port);
        }
    }

    // proxy not enabled
    return QNetworkProxy();
}

QList<QNetworkProxy> macQueryInternal(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result;

    // obtain a dictionary to the proxy settings:
    CFDictionaryRef dict = SCDynamicStoreCopyProxies(NULL);
    if (!dict) {
        qWarning("QNetworkProxyFactory::systemProxyForQuery: SCDynamicStoreCopyProxies returned NULL");
        return result;          // failed
    }

    if (isHostExcluded(dict, query.peerHostName())) {
        CFRelease(dict);
        return result;          // no proxy for this host
    }

    // is there a PAC enabled? If so, use it first.
    CFNumberRef pacEnabled;
    if ((pacEnabled = (CFNumberRef)CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigEnable))) {
        int enabled;
        if (CFNumberGetValue(pacEnabled, kCFNumberIntType, &enabled) && enabled) {
            // PAC is enabled
            CFStringRef pacUrl =
                (CFStringRef)CFDictionaryGetValue(dict, kSCPropNetProxiesProxyAutoConfigURLString);
            QString url = QCFString::toQString(pacUrl);

            // ### TODO: Use PAC somehow
        }
    }

    // no PAC, decide which proxy we're looking for based on the query
    bool isHttps = false;
    QString protocol = query.protocolTag().toLower();

    // try the protocol-specific proxy
    QNetworkProxy protocolSpecificProxy;
    if (protocol == QLatin1String("ftp")) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::FtpCachingProxy,
                                kSCPropNetProxiesFTPEnable,
                                kSCPropNetProxiesFTPProxy,
                                kSCPropNetProxiesFTPPort);
    } else if (protocol == QLatin1String("http")) {
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kSCPropNetProxiesHTTPEnable,
                                kSCPropNetProxiesHTTPProxy,
                                kSCPropNetProxiesHTTPPort);
    } else if (protocol == QLatin1String("https")) {
        isHttps = true;
        protocolSpecificProxy =
            proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                kSCPropNetProxiesHTTPSEnable,
                                kSCPropNetProxiesHTTPSProxy,
                                kSCPropNetProxiesHTTPSPort);
    }
    if (protocolSpecificProxy.type() != QNetworkProxy::DefaultProxy)
        result << protocolSpecificProxy;

    // let's add SOCKSv5 if present too
    QNetworkProxy socks5 = proxyFromDictionary(dict, QNetworkProxy::Socks5Proxy,
                                               kSCPropNetProxiesSOCKSEnable,
                                               kSCPropNetProxiesSOCKSProxy,
                                               kSCPropNetProxiesSOCKSPort);
    if (socks5.type() != QNetworkProxy::DefaultProxy)
        result << socks5;

    // let's add the HTTPS proxy if present (and if we haven't added
    // yet)
    if (!isHttps) {
        QNetworkProxy https = proxyFromDictionary(dict, QNetworkProxy::HttpProxy,
                                                  kSCPropNetProxiesHTTPSEnable,
                                                  kSCPropNetProxiesHTTPSProxy,
                                                  kSCPropNetProxiesHTTPSPort);
        if (https.type() != QNetworkProxy::DefaultProxy && https != protocolSpecificProxy)
            result << https;
    }

    CFRelease(dict);
    return result;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> result = macQueryInternal(query);
    if (result.isEmpty())
        result << QNetworkProxy::NoProxy;

    return result;
}

#endif

QT_END_NAMESPACE
