/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProxyServer.h"

#include "KURL.h"
#include "NetworkingContext.h"

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QUrl>

namespace WebCore {

Vector<ProxyServer> proxyServersForURL(const KURL& url, const NetworkingContext* context)
{
    Vector<ProxyServer> servers;
    
    const QNetworkAccessManager* accessManager = context ? context->networkAccessManager() : 0;
    QNetworkProxyFactory* proxyFactory = accessManager ? accessManager->proxyFactory() : 0;

    if (proxyFactory) {
        const QList<QNetworkProxy> proxies = proxyFactory->queryProxy(QNetworkProxyQuery(url));
        Q_FOREACH(const QNetworkProxy& proxy, proxies) {
            ProxyServer::Type proxyType;
            switch (proxy.type()) {
            case QNetworkProxy::Socks5Proxy:
                proxyType = ProxyServer::SOCKS;
                break;
            case QNetworkProxy::HttpProxy:
            case QNetworkProxy::HttpCachingProxy:
            case QNetworkProxy::FtpCachingProxy:
                proxyType = ProxyServer::HTTP;
                break;
            case QNetworkProxy::DefaultProxy:
            case QNetworkProxy::NoProxy:
            default:
                proxyType = ProxyServer::Direct;
                break;
            }
            servers.append(ProxyServer(proxyType, proxy.hostName(), proxy.port()));
        }
    }

    return servers;
}

} // namespace WebCore
