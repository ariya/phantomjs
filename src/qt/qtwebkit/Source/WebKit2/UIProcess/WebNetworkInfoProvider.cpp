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
#include "WebNetworkInfoProvider.h"

#if ENABLE(NETWORK_INFO)

#include "WKAPICast.h"
#include "WebNetworkInfoManagerProxy.h"
#include <limits.h>

namespace WebKit {

void WebNetworkInfoProvider::startUpdating(WebNetworkInfoManagerProxy* networkInfoManager)
{
    if (!m_client.startUpdating)
        return;

    m_client.startUpdating(toAPI(networkInfoManager), m_client.clientInfo);
}

void WebNetworkInfoProvider::stopUpdating(WebNetworkInfoManagerProxy* networkInfoManager)
{
    if (!m_client.stopUpdating)
        return;

    m_client.stopUpdating(toAPI(networkInfoManager), m_client.clientInfo);
}

double WebNetworkInfoProvider::bandwidth(WebNetworkInfoManagerProxy* networkInfoManager) const
{
    // The spec indicates that we should return "infinity" if the bandwidth is unknown.
    if (!m_client.bandwidth)
        return std::numeric_limits<double>::infinity();

    return m_client.bandwidth(toAPI(networkInfoManager), m_client.clientInfo);
}

bool WebNetworkInfoProvider::isMetered(WebNetworkInfoManagerProxy* networkInfoManager) const
{
    if (!m_client.isMetered)
        return false;

    return m_client.isMetered(toAPI(networkInfoManager), m_client.clientInfo);
}

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)
