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
#include "WebBatteryManager.h"

#if ENABLE(BATTERY_STATUS)

#include "WebBatteryManagerMessages.h"
#include "WebBatteryManagerProxyMessages.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/BatteryController.h>
#include <WebCore/BatteryStatus.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

const char* WebBatteryManager::supplementName()
{
    return "WebBatteryManager";
}

WebBatteryManager::WebBatteryManager(WebProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebBatteryManager::messageReceiverName(), this);
}

WebBatteryManager::~WebBatteryManager()
{
}

void WebBatteryManager::registerWebPage(WebPage* page)
{
    bool wasEmpty = m_pageSet.isEmpty();

    m_pageSet.add(page);

    if (wasEmpty)
        m_process->parentProcessConnection()->send(Messages::WebBatteryManagerProxy::StartUpdating(), 0);
}

void WebBatteryManager::unregisterWebPage(WebPage* page)
{
    m_pageSet.remove(page);

    if (m_pageSet.isEmpty())
        m_process->parentProcessConnection()->send(Messages::WebBatteryManagerProxy::StopUpdating(), 0);
}

void WebBatteryManager::didChangeBatteryStatus(const WTF::AtomicString& eventType, const WebBatteryStatus::Data& data)
{
    RefPtr<BatteryStatus> status = BatteryStatus::create(data.isCharging, data.chargingTime, data.dischargingTime, data.level);

    HashSet<WebPage*>::const_iterator it = m_pageSet.begin();
    HashSet<WebPage*>::const_iterator end = m_pageSet.end();
    for (; it != end; ++it) {
        WebPage* page = *it;
        if (page->corePage())
            BatteryController::from(page->corePage())->didChangeBatteryStatus(eventType, status.get());
    }
}

void WebBatteryManager::updateBatteryStatus(const WebBatteryStatus::Data& data)
{
    RefPtr<BatteryStatus> status = BatteryStatus::create(data.isCharging, data.chargingTime, data.dischargingTime, data.level);

    HashSet<WebPage*>::const_iterator it = m_pageSet.begin();
    HashSet<WebPage*>::const_iterator end = m_pageSet.end();
    for (; it != end; ++it) {
        WebPage* page = *it;
        if (page->corePage())
            BatteryController::from(page->corePage())->updateBatteryStatus(status.get());
    }
}

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)
