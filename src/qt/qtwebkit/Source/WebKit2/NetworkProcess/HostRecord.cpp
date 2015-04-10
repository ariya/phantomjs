/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "HostRecord.h"

#include "Logging.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkProcess.h"
#include "NetworkResourceLoadParameters.h"
#include "NetworkResourceLoadScheduler.h"
#include "NetworkResourceLoader.h"
#include "SyncNetworkResourceLoader.h"
#include <wtf/MainThread.h>

#if ENABLE(NETWORK_PROCESS)

using namespace WebCore;

namespace WebKit {

HostRecord::HostRecord(const String& name, int maxRequestsInFlight)
    : m_name(name)
    , m_maxRequestsInFlight(maxRequestsInFlight)
{
}

HostRecord::~HostRecord()
{
#ifndef NDEBUG
    ASSERT(m_loadersInProgress.isEmpty());
    for (unsigned p = 0; p <= ResourceLoadPriorityHighest; p++)
        ASSERT(m_loadersPending[p].isEmpty());
#endif
}

void HostRecord::scheduleResourceLoader(PassRefPtr<SchedulableLoader> loader)
{
    ASSERT(isMainThread());

    loader->setHostRecord(this);
    
    if (loader->isSynchronous())
        m_syncLoadersPending.append(loader);
    else
        m_loadersPending[loader->priority()].append(loader);
}

void HostRecord::addLoaderInProgress(SchedulableLoader* loader)
{
    ASSERT(isMainThread());

    m_loadersInProgress.add(loader);
    loader->setHostRecord(this);
}

inline bool removeLoaderFromQueue(SchedulableLoader* loader, LoaderQueue& queue)
{
    LoaderQueue::iterator end = queue.end();
    for (LoaderQueue::iterator it = queue.begin(); it != end; ++it) {
        if (it->get() == loader) {
            loader->setHostRecord(0);
            queue.remove(it);
            return true;
        }
    }
    return false;
}

void HostRecord::removeLoader(SchedulableLoader* loader)
{
    ASSERT(isMainThread());

    // FIXME (NetworkProcess): Due to IPC race conditions, it's possible this HostRecord will be asked to remove the same loader twice.
    // It would be nice to know the loader has already been removed and treat it as a no-op.

    SchedulableLoaderSet::iterator i = m_loadersInProgress.find(loader);
    if (i != m_loadersInProgress.end()) {
        i->get()->setHostRecord(0);
        m_loadersInProgress.remove(i);
        return;
    }

    if (removeLoaderFromQueue(loader, m_syncLoadersPending))
        return;
    
    for (int priority = ResourceLoadPriorityHighest; priority >= ResourceLoadPriorityLowest; --priority) {
        if (removeLoaderFromQueue(loader, m_loadersPending[priority]))
            return;
    }
}

bool HostRecord::hasRequests() const
{
    if (!m_loadersInProgress.isEmpty())
        return true;

    for (unsigned p = 0; p <= ResourceLoadPriorityHighest; p++) {
        if (!m_loadersPending[p].isEmpty())
            return true;
    }

    return false;
}

uint64_t HostRecord::pendingRequestCount() const
{
    uint64_t count = 0;

    for (unsigned p = 0; p <= ResourceLoadPriorityHighest; p++)
        count += m_loadersPending[p].size();

    return count;
}

uint64_t HostRecord::activeLoadCount() const
{
    return m_loadersInProgress.size();
}

void HostRecord::servePendingRequestsForQueue(LoaderQueue& queue, ResourceLoadPriority priority)
{
    // We only enforce the connection limit for http(s) hosts, which are the only ones with names.
    bool shouldLimitRequests = !name().isNull();

    // For non-named hosts - everything but http(s) - we should only enforce the limit if the document
    // isn't done parsing and we don't know all stylesheets yet.

    // FIXME (NetworkProcess): The above comment about document parsing and stylesheets is a holdover
    // from the WebCore::ResourceLoadScheduler.
    // The behavior described was at one time important for WebCore's single threadedness.
    // It's possible that we don't care about it with the NetworkProcess.
    // We should either decide it's not important and change the above comment, or decide it is
    // still important and somehow account for it.
    
    // Very low priority loaders are only handled when no other loaders are in progress.
    if (shouldLimitRequests && priority == ResourceLoadPriorityVeryLow && !m_loadersInProgress.isEmpty())
        return;
    
    while (!queue.isEmpty()) {
        RefPtr<SchedulableLoader> loader = queue.first();
        ASSERT(loader->hostRecord() == this);

        // This request might be from WebProcess we've lost our connection to.
        // If so we should just skip it.
        if (!loader->connectionToWebProcess()) {
            removeLoader(loader.get());
            continue;
        }

        if (shouldLimitRequests && limitsRequests(priority, loader.get()))
            return;

        m_loadersInProgress.add(loader);
        queue.removeFirst();

        LOG(NetworkScheduling, "(NetworkProcess) HostRecord::servePendingRequestsForQueue - Starting load of %s\n", loader->request().url().string().utf8().data());
        loader->start();
    }
}

void HostRecord::servePendingRequests(ResourceLoadPriority minimumPriority)
{
    LOG(NetworkScheduling, "(NetworkProcess) HostRecord::servePendingRequests Host name='%s'", name().utf8().data());

    // We serve synchronous requests before any other requests to improve responsiveness in any
    // WebProcess that is waiting on a synchronous load.
    servePendingRequestsForQueue(m_syncLoadersPending, ResourceLoadPriorityHighest);
    
    for (int priority = ResourceLoadPriorityHighest; priority >= minimumPriority; --priority)
        servePendingRequestsForQueue(m_loadersPending[priority], (ResourceLoadPriority)priority);
}

bool HostRecord::limitsRequests(ResourceLoadPriority priority, SchedulableLoader* loader) const
{
    ASSERT(loader);
    ASSERT(loader->connectionToWebProcess());

    if (priority == ResourceLoadPriorityVeryLow && !m_loadersInProgress.isEmpty())
        return true;

    if (loader->connectionToWebProcess()->isSerialLoadingEnabled() && m_loadersInProgress.size() >= 1)
        return true;

    // If we're exactly at the limit for requests in flight, and this loader is asynchronous, then we're done serving new requests.
    // The synchronous loader exception handles the case where a sync XHR is made while 6 other requests are already in flight.
    if (m_loadersInProgress.size() == m_maxRequestsInFlight && !loader->isSynchronous())
        return true;

    // If we're already past the limit of the number of loaders in flight, we won't even serve new synchronous requests right now.
    if (m_loadersInProgress.size() > m_maxRequestsInFlight) {
#ifndef NDEBUG
        // If we have more loaders in progress than we should, at least one of them had better be synchronous.
        SchedulableLoaderSet::iterator i = m_loadersInProgress.begin();
        SchedulableLoaderSet::iterator end = m_loadersInProgress.end();
        for (; i != end; ++i) {
            if (i->get()->isSynchronous())
                break;
        }
        ASSERT(i != end);
#endif
        return true;
    }
    return false;
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
