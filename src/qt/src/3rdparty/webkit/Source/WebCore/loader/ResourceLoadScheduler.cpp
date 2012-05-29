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
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "Logging.h"
#include "NetscapePlugInStreamLoader.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "SubresourceLoader.h"
#include <wtf/text/CString.h>

#define REQUEST_MANAGEMENT_ENABLED 1

namespace WebCore {

#if REQUEST_MANAGEMENT_ENABLED
static const unsigned maxRequestsInFlightForNonHTTPProtocols = 20;
// Match the parallel connection count used by the networking layer.
static unsigned maxRequestsInFlightPerHost;
#else
static const unsigned maxRequestsInFlightForNonHTTPProtocols = 10000;
static const unsigned maxRequestsInFlightPerHost = 10000;
#endif

ResourceLoadScheduler::HostInformation* ResourceLoadScheduler::hostForURL(const KURL& url, CreateHostPolicy createHostPolicy)
{
    if (!url.protocolInHTTPFamily())
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
    DEFINE_STATIC_LOCAL(ResourceLoadScheduler, resourceLoadScheduler, ());
    return &resourceLoadScheduler;
}

ResourceLoadScheduler::ResourceLoadScheduler()
    : m_nonHTTPProtocolHost(new HostInformation(String(), maxRequestsInFlightForNonHTTPProtocols))
    , m_requestTimer(this, &ResourceLoadScheduler::requestTimerFired)
    , m_isSuspendingPendingRequests(false)
    , m_isSerialLoadingEnabled(false)
{
#if REQUEST_MANAGEMENT_ENABLED
    maxRequestsInFlightPerHost = initializeMaximumHTTPConnectionCountPerHost();
#endif
}

PassRefPtr<SubresourceLoader> ResourceLoadScheduler::scheduleSubresourceLoad(Frame* frame, SubresourceLoaderClient* client, const ResourceRequest& request, ResourceLoadPriority priority, SecurityCheckPolicy securityCheck,
                                                                             bool sendResourceLoadCallbacks, bool shouldContentSniff, const String& optionalOutgoingReferrer)
{
    RefPtr<SubresourceLoader> loader = SubresourceLoader::create(frame, client, request, securityCheck, sendResourceLoadCallbacks, shouldContentSniff, optionalOutgoingReferrer);
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

void ResourceLoadScheduler::addMainResourceLoad(ResourceLoader* resourceLoader)
{
    hostForURL(resourceLoader->url(), CreateIfNotFound)->addLoadInProgress(resourceLoader);
}

void ResourceLoadScheduler::scheduleLoad(ResourceLoader* resourceLoader, ResourceLoadPriority priority)
{
    ASSERT(resourceLoader);
    ASSERT(priority != ResourceLoadPriorityUnresolved);
#if !REQUEST_MANAGEMENT_ENABLED
    priority = ResourceLoadPriorityHighest;
#endif

    LOG(ResourceLoading, "ResourceLoadScheduler::load resource %p '%s'", resourceLoader, resourceLoader->url().string().latin1().data());
    HostInformation* host = hostForURL(resourceLoader->url(), CreateIfNotFound);    
    bool hadRequests = host->hasRequests();
    host->schedule(resourceLoader, priority);

    if (priority > ResourceLoadPriorityLow || !resourceLoader->url().protocolInHTTPFamily() || (priority == ResourceLoadPriorityLow && !hadRequests)) {
        // Try to request important resources immediately.
        servePendingRequests(host, priority);
        return;
    }

    // Handle asynchronously so early low priority requests don't get scheduled before later high priority ones.
    InspectorInstrumentation::didScheduleResourceRequest(resourceLoader->frameLoader() ? resourceLoader->frameLoader()->frame()->document() : 0, resourceLoader->url());
    scheduleServePendingRequests();
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
    LOG(ResourceLoading, "ResourceLoadScheduler::servePendingRequests. m_isSuspendingPendingRequests=%d", m_isSuspendingPendingRequests); 
    if (m_isSuspendingPendingRequests)
        return;

    m_requestTimer.stop();
    
    servePendingRequests(m_nonHTTPProtocolHost, minimumPriority);

    Vector<HostInformation*> hostsToServe;
    m_hosts.checkConsistency();
    HostMap::iterator end = m_hosts.end();
    for (HostMap::iterator iter = m_hosts.begin(); iter != end; ++iter)
        hostsToServe.append(iter->second);

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
    ASSERT(!m_isSuspendingPendingRequests);
    m_isSuspendingPendingRequests = true;
}

void ResourceLoadScheduler::resumePendingRequests()
{
    ASSERT(m_isSuspendingPendingRequests);
    m_isSuspendingPendingRequests = false;
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
