/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2010 Google Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ResourceLoadScheduler.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "LoaderStrategy.h"
#include "Logging.h"
#include "NetscapePlugInStreamLoader.h"
#include "PlatformStrategies.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "SubresourceLoader.h"
#include <wtf/MainThread.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const unsigned maxRequestsInFlightForNonHTTPProtocols = 20;
// Match the parallel connection count used by the networking layer.
static unsigned maxRequestsInFlightPerHost;

ResourceLoadScheduler::HostInformation* ResourceLoadScheduler::hostForURL(const KURL& url, CreateHostPolicy createHostPolicy)
{
    if (!url.protocolIsInHTTPFamily())
        return m_nonHTTPProtocolHost;

    m_hosts.checkConsistency();
    String hostName = url.host();
    HostInformation* host = m_hosts.get(hostName);
    if (!host && createHostPolicy == CreateIfNotFound) {
        host = new HostInformation(hostName, maxRequestsInFlightPerHost);
        m_hosts.add(hostName, host);
    }
    return host;
}

ResourceLoadScheduler* resourceLoadScheduler()
{
    ASSERT(isMainThread());
    static ResourceLoadScheduler* globalScheduler = 0;
    
    if (!globalScheduler) {
        static bool isCallingOutToStrategy = false;
        
        // If we're re-entering resourceLoadScheduler() while calling out to the LoaderStrategy,
        // then the LoaderStrategy is trying to use the default resourceLoadScheduler.
        // So we'll create it here and start using it.
        if (isCallingOutToStrategy) {
            globalScheduler = new ResourceLoadScheduler;
            return globalScheduler;
        }
        
        TemporaryChange<bool> recursionGuard(isCallingOutToStrategy, true);
        globalScheduler = platformStrategies()->loaderStrategy()->resourceLoadScheduler();
    }

    return globalScheduler;
}

ResourceLoadScheduler::ResourceLoadScheduler()
    : m_nonHTTPProtocolHost(new HostInformation(String(), maxRequestsInFlightForNonHTTPProtocols))
    , m_requestTimer(this, &ResourceLoadScheduler::requestTimerFired)
    , m_suspendPendingRequestsCount(0)
    , m_isSerialLoadingEnabled(false)
{
    maxRequestsInFlightPerHost = initializeMaximumHTTPConnectionCountPerHost();
}

ResourceLoadScheduler::~ResourceLoadScheduler()
{
}

PassRefPtr<SubresourceLoader> ResourceLoadScheduler::scheduleSubresourceLoad(Frame* frame, CachedResource* resource, const ResourceRequest& request, ResourceLoadPriority priority, const ResourceLoaderOptions& options)
{
    RefPtr<SubresourceLoader> loader = SubresourceLoader::create(frame, resource, request, options);
    if (loader)
        scheduleLoad(loader.get(), priority);
    return loader.release();
}

PassRefPtr<NetscapePlugInStreamLoader> ResourceLoadScheduler::schedulePluginStreamLoad(Frame* frame, NetscapePlugInStreamLoaderClient* client, const ResourceRequest& request)
{
    PassRefPtr<NetscapePlugInStreamLoader> loader = NetscapePlugInStreamLoader::create(frame, client, request);
    if (loader)
        scheduleLoad(loader.get(), ResourceLoadPriorityLow);
    return loader;
}

void ResourceLoadScheduler::scheduleLoad(ResourceLoader* resourceLoader, ResourceLoadPriority priority)
{
    ASSERT(resourceLoader);
    ASSERT(priority != ResourceLoadPriorityUnresolved);

    LOG(ResourceLoading, "ResourceLoadScheduler::load resource %p '%s'", resourceLoader, resourceLoader->url().string().latin1().data());

    // If there's a web archive resource for this URL, we don't need to schedule the load since it will never touch the network.
    if (resourceLoader->documentLoader()->archiveResourceForURL(resourceLoader->request().url())) {
        resourceLoader->start();
        return;
    }

    HostInformation* host = hostForURL(resourceLoader->url(), CreateIfNotFound);    
    bool hadRequests = host->hasRequests();
    host->schedule(resourceLoader, priority);

    if (priority > ResourceLoadPriorityLow || !resourceLoader->url().protocolIsInHTTPFamily() || (priority == ResourceLoadPriorityLow && !hadRequests)) {
        // Try to request important resources immediately.
        servePendingRequests(host, priority);
        return;
    }

    notifyDidScheduleResourceRequest(resourceLoader);

    // Handle asynchronously so early low priority requests don't
    // get scheduled before later high priority ones.
    scheduleServePendingRequests();
}

void ResourceLoadScheduler::notifyDidScheduleResourceRequest(ResourceLoader* loader)
{
    InspectorInstrumentation::didScheduleResourceRequest(loader->frameLoader() ? loader->frameLoader()->frame()->document() : 0, loader->url());
}

void ResourceLoadScheduler::remove(ResourceLoader* resourceLoader)
{
    ASSERT(resourceLoader);

    HostInformation* host = hostForURL(resourceLoader->url());
    if (host)
        host->remove(resourceLoader);
    scheduleServePendingRequests();
}

void ResourceLoadScheduler::crossOriginRedirectReceived(ResourceLoader* resourceLoader, const KURL& redirectURL)
{
    HostInformation* oldHost = hostForURL(resourceLoader->url());
    ASSERT(oldHost);
    HostInformation* newHost = hostForURL(redirectURL, CreateIfNotFound);

    if (oldHost->name() == newHost->name())
        return;
    
    newHost->addLoadInProgress(resourceLoader);
    oldHost->remove(resourceLoader);
}

void ResourceLoadScheduler::servePendingRequests(ResourceLoadPriority minimumPriority)
{
    LOG(ResourceLoading, "ResourceLoadScheduler::servePendingRequests. m_suspendPendingRequestsCount=%d", m_suspendPendingRequestsCount); 
    if (isSuspendingPendingRequests())
        return;

    m_requestTimer.stop();
    
    servePendingRequests(m_nonHTTPProtocolHost, minimumPriority);

    Vector<HostInformation*> hostsToServe;
    m_hosts.checkConsistency();
    HostMap::iterator end = m_hosts.end();
    for (HostMap::iterator iter = m_hosts.begin(); iter != end; ++iter)
        hostsToServe.append(iter->value);

    int size = hostsToServe.size();
    for (int i = 0; i < size; ++i) {
        HostInformation* host = hostsToServe[i];
        if (host->hasRequests())
            servePendingRequests(host, minimumPriority);
        else
            delete m_hosts.take(host->name());
    }
}

void ResourceLoadScheduler::servePendingRequests(HostInformation* host, ResourceLoadPriority minimumPriority)
{
    LOG(ResourceLoading, "ResourceLoadScheduler::servePendingRequests HostInformation.m_name='%s'", host->name().latin1().data());

    for (int priority = ResourceLoadPriorityHighest; priority >= minimumPriority; --priority) {
        HostInformation::RequestQueue& requestsPending = host->requestsPending(ResourceLoadPriority(priority));

        while (!requestsPending.isEmpty()) {
            RefPtr<ResourceLoader> resourceLoader = requestsPending.first();

            // For named hosts - which are only http(s) hosts - we should always enforce the connection limit.
            // For non-named hosts - everything but http(s) - we should only enforce the limit if the document isn't done parsing 
            // and we don't know all stylesheets yet.
            Document* document = resourceLoader->frameLoader() ? resourceLoader->frameLoader()->frame()->document() : 0;
            bool shouldLimitRequests = !host->name().isNull() || (document && (document->parsing() || !document->haveStylesheetsLoaded()));
            if (shouldLimitRequests && host->limitRequests(ResourceLoadPriority(priority)))
                return;

            requestsPending.removeFirst();
            host->addLoadInProgress(resourceLoader.get());
            resourceLoader->start();
        }
    }
}

void ResourceLoadScheduler::suspendPendingRequests()
{
    ++m_suspendPendingRequestsCount;
}

void ResourceLoadScheduler::resumePendingRequests()
{
    ASSERT(m_suspendPendingRequestsCount);
    --m_suspendPendingRequestsCount;
    if (m_suspendPendingRequestsCount)
        return;
    if (!m_hosts.isEmpty() || m_nonHTTPProtocolHost->hasRequests())
        scheduleServePendingRequests();
}
    
void ResourceLoadScheduler::scheduleServePendingRequests()
{
    LOG(ResourceLoading, "ResourceLoadScheduler::scheduleServePendingRequests, m_requestTimer.isActive()=%u", m_requestTimer.isActive());
    if (!m_requestTimer.isActive())
        m_requestTimer.startOneShot(0);
}

void ResourceLoadScheduler::requestTimerFired(Timer<ResourceLoadScheduler>*) 
{
    LOG(ResourceLoading, "ResourceLoadScheduler::requestTimerFired\n");
    servePendingRequests();
}

ResourceLoadScheduler::HostInformation::HostInformation(const String& name, unsigned maxRequestsInFlight)
    : m_name(name)
    , m_maxRequestsInFlight(maxRequestsInFlight)
{
}

ResourceLoadScheduler::HostInformation::~HostInformation()
{
    ASSERT(m_requestsLoading.isEmpty());
    for (unsigned p = 0; p <= ResourceLoadPriorityHighest; p++)
        ASSERT(m_requestsPending[p].isEmpty());
}
    
void ResourceLoadScheduler::HostInformation::schedule(ResourceLoader* resourceLoader, ResourceLoadPriority priority)
{
    m_requestsPending[priority].append(resourceLoader);
}
    
void ResourceLoadScheduler::HostInformation::addLoadInProgress(ResourceLoader* resourceLoader)
{
    LOG(ResourceLoading, "HostInformation '%s' loading '%s'. Current count %d", m_name.latin1().data(), resourceLoader->url().string().latin1().data(), m_requestsLoading.size());
    m_requestsLoading.add(resourceLoader);
}
    
void ResourceLoadScheduler::HostInformation::remove(ResourceLoader* resourceLoader)
{
    if (m_requestsLoading.contains(resourceLoader)) {
        m_requestsLoading.remove(resourceLoader);
        return;
    }
    
    for (int priority = ResourceLoadPriorityHighest; priority >= ResourceLoadPriorityLowest; --priority) {  
        RequestQueue::iterator end = m_requestsPending[priority].end();
        for (RequestQueue::iterator it = m_requestsPending[priority].begin(); it != end; ++it) {
            if (*it == resourceLoader) {
                m_requestsPending[priority].remove(it);
                return;
            }
        }
    }
}

bool ResourceLoadScheduler::HostInformation::hasRequests() const
{
    if (!m_requestsLoading.isEmpty())
        return true;
    for (unsigned p = 0; p <= ResourceLoadPriorityHighest; p++) {
        if (!m_requestsPending[p].isEmpty())
            return true;
    }
    return false;
}

bool ResourceLoadScheduler::HostInformation::limitRequests(ResourceLoadPriority priority) const 
{
    if (priority == ResourceLoadPriorityVeryLow && !m_requestsLoading.isEmpty())
        return true;
    return m_requestsLoading.size() >= (resourceLoadScheduler()->isSerialLoadingEnabled() ? 1 : m_maxRequestsInFlight);
}

} // namespace WebCore
