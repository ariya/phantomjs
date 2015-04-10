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
#include "WebNetworkInfoManager.h"

#if ENABLE(NETWORK_INFO)

#include "WebNetworkInfoManagerMessages.h"
#include "WebNetworkInfoManagerProxyMessages.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/NetworkInfo.h>
#include <WebCore/NetworkInfoController.h>
#include <limits.h>

using namespace WebCore;

namespace WebKit {

const char* WebNetworkInfoManager::supplementName()
{
    return "WebNetworkInfoManager";
}

WebNetworkInfoManager::WebNetworkInfoManager(WebProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebNetworkInfoManager::messageReceiverName(), this);
}

WebNetworkInfoManager::~WebNetworkInfoManager()
{
}

void WebNetworkInfoManager::registerWebPage(WebPage* page)
{
    bool wasEmpty = m_pageSet.isEmpty();

    m_pageSet.add(page);

    if (wasEmpty)
        m_process->parentProcessConnection()->send(Messages::WebNetworkInfoManagerProxy::StartUpdating(), 0);
}

void WebNetworkInfoManager::unregisterWebPage(WebPage* page)
{
    m_pageSet.remove(page);

    if (m_pageSet.isEmpty())
        m_process->parentProcessConnection()->send(Messages::WebNetworkInfoManagerProxy::StopUpdating(), 0);
}

double WebNetworkInfoManager::bandwidth(WebPage* page) const
{
    // The spec indicates that we should return "infinity" if the bandwidth is unknown.
    double bandwidth = std::numeric_limits<double>::infinity();
    m_process->parentProcessConnection()->sendSync(Messages::WebNetworkInfoManagerProxy::GetBandwidth(), Messages::WebNetworkInfoManagerProxy::GetBandwidth::Reply(bandwidth), page->pageID());
    return bandwidth;
}

bool WebNetworkInfoManager::metered(WebPage* page) const
{
    bool metered = false;
    m_process->parentProcessConnection()->sendSync(Messages::WebNetworkInfoManagerProxy::IsMetered(), Messages::WebNetworkInfoManagerProxy::IsMetered::Reply(metered), page->pageID());
    return metered;
}

void WebNetworkInfoManager::didChangeNetworkInformation(const AtomicString& eventType, const WebNetworkInfo::Data& data)
{
    RefPtr<NetworkInfo> networkInformation = NetworkInfo::create(data.bandwidth, data.metered);

    HashSet<WebPage*>::const_iterator it = m_pageSet.begin();
    HashSet<WebPage*>::const_iterator end = m_pageSet.end();
    for (; it != end; ++it) {
        WebPage* page = *it;
        if (page->corePage())
            NetworkInfoController::from(page->corePage())->didChangeNetworkInformation(eventType, networkInformation.get());
    }
}

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)
