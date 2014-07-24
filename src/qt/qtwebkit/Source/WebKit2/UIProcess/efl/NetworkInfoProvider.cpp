/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "NetworkInfoProvider.h"

#if ENABLE(NETWORK_INFO)

#include "WKNetworkInfoManager.h"
#include <NotImplemented.h>

using namespace WebKit;

static inline NetworkInfoProvider* toNetworkInfoProvider(const void* clientInfo)
{
    return static_cast<NetworkInfoProvider*>(const_cast<void*>(clientInfo));
}

static void startUpdatingCallback(WKNetworkInfoManagerRef, const void* clientInfo)
{
    toNetworkInfoProvider(clientInfo)->startUpdating();
}

static void stopUpdatingCallback(WKNetworkInfoManagerRef, const void* clientInfo)
{
    toNetworkInfoProvider(clientInfo)->stopUpdating();
}

static double getBandwidthCallback(WKNetworkInfoManagerRef, const void* clientInfo)
{
    return toNetworkInfoProvider(clientInfo)->bandwidth();
}

static bool isMeteredCallback(WKNetworkInfoManagerRef, const void* clientInfo)
{
    return toNetworkInfoProvider(clientInfo)->metered();
}

PassRefPtr<NetworkInfoProvider> NetworkInfoProvider::create(WKContextRef context)
{
    return adoptRef(new NetworkInfoProvider(context));
}

NetworkInfoProvider::NetworkInfoProvider(WKContextRef context)
    : m_context(context)
{
    ASSERT(m_context);

    WKNetworkInfoManagerRef wkNetworkInfoManager = WKContextGetNetworkInfoManager(m_context.get());
    ASSERT(wkNetworkInfoManager);

    WKNetworkInfoProvider wkNetworkInfoProvider = {
        kWKNetworkInfoProviderCurrentVersion,
        this, // clientInfo
        startUpdatingCallback,
        stopUpdatingCallback,
        getBandwidthCallback,
        isMeteredCallback
    };

    WKNetworkInfoManagerSetProvider(wkNetworkInfoManager, &wkNetworkInfoProvider);
}

NetworkInfoProvider::~NetworkInfoProvider()
{
    WKNetworkInfoManagerRef wkNetworkInfoManager = WKContextGetNetworkInfoManager(m_context.get());
    ASSERT(wkNetworkInfoManager);

    WKNetworkInfoManagerSetProvider(wkNetworkInfoManager, 0);
}

void NetworkInfoProvider::networkInfoControllerDestroyed()
{
    delete this;
}

double NetworkInfoProvider::bandwidth() const
{
    return m_provider.bandwidth();
}

bool NetworkInfoProvider::metered() const
{
    notImplemented();

    return false;
}

void NetworkInfoProvider::startUpdating()
{
    m_provider.startUpdating();
}

void NetworkInfoProvider::stopUpdating()
{
    m_provider.stopUpdating();
}

#endif // ENABLE(NETWORK_INFO)
