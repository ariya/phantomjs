/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DNSResolveQueue.h"

#include <wtf/CurrentTime.h>

namespace WebCore {

// When resolve queue is empty, we fire async resolution requests immediately (which is important if the prefetch is triggered by hovering).
// But during page parsing, we should coalesce identical requests to avoid stressing out the DNS resolver.
static const int gNamesToResolveImmediately = 4;

// Coalesce prefetch requests for this long before sending them out.
static const double gCoalesceDelayInSeconds = 1.0;

// Sending many DNS requests at once can overwhelm some gateways. See <rdar://8105550> for specific CFNET issues with CFHost throttling.
static const int gMaxSimultaneousRequests = 8;

// For a page has links to many outside sites, it is likely that the system DNS resolver won't be able to cache them all anyway, and we don't want
// to negatively affect other applications' performance by pushing their cached entries out.
// If we end up with lots of names to prefetch, some will be dropped.
static const int gMaxRequestsToQueue = 64;

// If there were queued names that couldn't be sent simultaneously, check the state of resolvers after this delay.
static const double gRetryResolvingInSeconds = 0.1;

DNSResolveQueue::DNSResolveQueue()
    : m_requestsInFlight(0)
    , m_cachedProxyEnabledStatus(false)
    , m_lastProxyEnabledStatusCheckTime(0)
{
}

bool DNSResolveQueue::isUsingProxy()
{
    double time = currentTime();
    static const double minimumProxyCheckDelay = 5;
    if (time - m_lastProxyEnabledStatusCheckTime > minimumProxyCheckDelay) {
        m_lastProxyEnabledStatusCheckTime = time;
        m_cachedProxyEnabledStatus = platformProxyIsEnabledInSystemPreferences();
    }
    return m_cachedProxyEnabledStatus;
}

void DNSResolveQueue::add(const String& hostname)
{
    // If there are no names queued, and few enough are in flight, resolve immediately (the mouse may be over a link).
    if (!m_names.size()) {
        if (isUsingProxy())
            return;
        if (atomicIncrement(&m_requestsInFlight) <= gNamesToResolveImmediately) {
            platformResolve(hostname);
            return;
        }
        atomicDecrement(&m_requestsInFlight);
    }

    // It's better to not prefetch some names than to clog the queue.
    // Dropping the newest names, because on a single page, these are likely to be below oldest ones.
    if (m_names.size() < gMaxRequestsToQueue) {
        m_names.add(hostname);
        if (!isActive())
            startOneShot(gCoalesceDelayInSeconds);
    }
}

void DNSResolveQueue::fired()
{
    if (isUsingProxy()) {
        m_names.clear();
        return;
    }

    int requestsAllowed = gMaxSimultaneousRequests - m_requestsInFlight;

    for (; !m_names.isEmpty() && requestsAllowed > 0; --requestsAllowed) {
        atomicIncrement(&m_requestsInFlight);
        HashSet<String>::iterator currentName = m_names.begin();
        platformResolve(*currentName);
        m_names.remove(currentName);
    }

    if (!m_names.isEmpty())
        startOneShot(gRetryResolvingInSeconds);
}

} // namespace WebCore
