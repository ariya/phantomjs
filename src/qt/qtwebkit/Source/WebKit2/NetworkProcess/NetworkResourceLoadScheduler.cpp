#include "config.h"
#include "NetworkResourceLoadScheduler.h"

#include "HostRecord.h"
#include "Logging.h"
#include "NetworkProcess.h"
#include "NetworkResourceLoadParameters.h"
#include "NetworkResourceLoader.h"
#include "SyncNetworkResourceLoader.h"
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>

#if ENABLE(NETWORK_PROCESS)

using namespace WebCore;

namespace WebKit {

static const unsigned maxRequestsInFlightForNonHTTPProtocols = 20;

NetworkResourceLoadScheduler::NetworkResourceLoadScheduler()
    : m_nonHTTPProtocolHost(HostRecord::create(String(), maxRequestsInFlightForNonHTTPProtocols))
    , m_requestTimer(this, &NetworkResourceLoadScheduler::requestTimerFired)

{
    platformInitializeMaximumHTTPConnectionCountPerHost();
}

void NetworkResourceLoadScheduler::scheduleServePendingRequests()
{
    if (!m_requestTimer.isActive())
        m_requestTimer.startOneShot(0);
}

void NetworkResourceLoadScheduler::requestTimerFired(WebCore::Timer<NetworkResourceLoadScheduler>*)
{
    servePendingRequests();
}

void NetworkResourceLoadScheduler::scheduleLoader(PassRefPtr<SchedulableLoader> loader)
{
    ResourceLoadPriority priority = loader->priority();
    const ResourceRequest& resourceRequest = loader->request();
        
    LOG(NetworkScheduling, "(NetworkProcess) NetworkResourceLoadScheduler::scheduleLoader resource '%s'", resourceRequest.url().string().utf8().data());

    HostRecord* host = hostForURL(resourceRequest.url(), CreateIfNotFound);
    bool hadRequests = host->hasRequests();
    host->scheduleResourceLoader(loader);

    if (priority > ResourceLoadPriorityLow || !resourceRequest.url().protocolIsInHTTPFamily() || (priority == ResourceLoadPriorityLow && !hadRequests)) {
        // Try to request important resources immediately.
        host->servePendingRequests(priority);
        return;
    }
    
    // Handle asynchronously so early low priority requests don't get scheduled before later high priority ones.
    scheduleServePendingRequests();
}

HostRecord* NetworkResourceLoadScheduler::hostForURL(const WebCore::KURL& url, CreateHostPolicy createHostPolicy)
{
    if (!url.protocolIsInHTTPFamily())
        return m_nonHTTPProtocolHost.get();

    m_hosts.checkConsistency();
    String hostName = url.host();
    HostRecord* host = m_hosts.get(hostName);
    if (!host && createHostPolicy == CreateIfNotFound) {
        RefPtr<HostRecord> newHost = HostRecord::create(hostName, m_maxRequestsInFlightPerHost);
        host = newHost.get();
        m_hosts.add(hostName, newHost.release());
    }
    
    return host;
}

void NetworkResourceLoadScheduler::removeLoader(SchedulableLoader* loader)
{
    ASSERT(isMainThread());
    ASSERT(loader);

    LOG(NetworkScheduling, "(NetworkProcess) NetworkResourceLoadScheduler::removeLoadIdentifier removing loader %s", loader->request().url().string().utf8().data());

    HostRecord* host = loader->hostRecord();
    
    // Due to a race condition the WebProcess might have messaged the NetworkProcess to remove this identifier
    // after the NetworkProcess has already removed it internally.
    // In this situation we might not have a HostRecord to clean up.
    if (host)
        host->removeLoader(loader);

    scheduleServePendingRequests();
}

void NetworkResourceLoadScheduler::receivedRedirect(SchedulableLoader* loader, const WebCore::KURL& redirectURL)
{
    ASSERT(isMainThread());
    LOG(NetworkScheduling, "(NetworkProcess) NetworkResourceLoadScheduler::receivedRedirect loader originally for '%s' redirected to '%s'", loader->request().url().string().utf8().data(), redirectURL.string().utf8().data());

    HostRecord* oldHost = loader->hostRecord();

    // The load may have been cancelled while the message was in flight from network thread to main thread.
    if (!oldHost)
        return;

    HostRecord* newHost = hostForURL(redirectURL, CreateIfNotFound);
    
    if (oldHost->name() == newHost->name())
        return;

    oldHost->removeLoader(loader);
    newHost->addLoaderInProgress(loader);
}

void NetworkResourceLoadScheduler::servePendingRequests(ResourceLoadPriority minimumPriority)
{
    LOG(NetworkScheduling, "(NetworkProcess) NetworkResourceLoadScheduler::servePendingRequests Serving requests for up to %i hosts with minimum priority %i", m_hosts.size(), minimumPriority);

    m_requestTimer.stop();
    
    m_nonHTTPProtocolHost->servePendingRequests(minimumPriority);

    m_hosts.checkConsistency();
    Vector<RefPtr<HostRecord>> hostsToServe;
    copyValuesToVector(m_hosts, hostsToServe);

    size_t size = hostsToServe.size();
    for (size_t i = 0; i < size; ++i) {
        HostRecord* host = hostsToServe[i].get();
        if (host->hasRequests())
            host->servePendingRequests(minimumPriority);
        else
            m_hosts.remove(host->name());
    }
}

static bool removeScheduledLoadersCalled = false;

void NetworkResourceLoadScheduler::removeScheduledLoaders(void* context)
{
    ASSERT(isMainThread());
    ASSERT(removeScheduledLoadersCalled);

    NetworkResourceLoadScheduler* scheduler = static_cast<NetworkResourceLoadScheduler*>(context);
    scheduler->removeScheduledLoaders();
}

void NetworkResourceLoadScheduler::removeScheduledLoaders()
{
    Vector<RefPtr<SchedulableLoader>> loadersToRemove;
    {
        MutexLocker locker(m_loadersToRemoveMutex);
        loadersToRemove = m_loadersToRemove;
        m_loadersToRemove.clear();
        removeScheduledLoadersCalled = false;
    }
    
    for (size_t i = 0; i < loadersToRemove.size(); ++i)
        removeLoader(loadersToRemove[i].get());
}

void NetworkResourceLoadScheduler::scheduleRemoveLoader(SchedulableLoader* loader)
{
    MutexLocker locker(m_loadersToRemoveMutex);
    
    m_loadersToRemove.append(loader);
    
    if (!removeScheduledLoadersCalled) {
        removeScheduledLoadersCalled = true;
        callOnMainThread(NetworkResourceLoadScheduler::removeScheduledLoaders, this);
    }
}

uint64_t NetworkResourceLoadScheduler::hostsPendingCount() const
{
    uint64_t count = m_nonHTTPProtocolHost->pendingRequestCount() ? 1 : 0;

    HostMap::const_iterator end = m_hosts.end();
    for (HostMap::const_iterator i = m_hosts.begin(); i != end; ++i) {
        if (i->value->pendingRequestCount())
            ++count;
    }

    return count;
}

uint64_t NetworkResourceLoadScheduler::loadsPendingCount() const
{
    uint64_t count = m_nonHTTPProtocolHost->pendingRequestCount();

    HostMap::const_iterator end = m_hosts.end();
    for (HostMap::const_iterator i = m_hosts.begin(); i != end; ++i)
        count += i->value->pendingRequestCount();

    return count;
}

uint64_t NetworkResourceLoadScheduler::hostsActiveCount() const
{
    uint64_t count = 0;

    if (m_nonHTTPProtocolHost->activeLoadCount())
        count = 1;

    HostMap::const_iterator end = m_hosts.end();
    for (HostMap::const_iterator i = m_hosts.begin(); i != end; ++i) {
        if (i->value->activeLoadCount())
            ++count;
    }

    return count;
}

uint64_t NetworkResourceLoadScheduler::loadsActiveCount() const
{
    uint64_t count = m_nonHTTPProtocolHost->activeLoadCount();

    HostMap::const_iterator end = m_hosts.end();
    for (HostMap::const_iterator i = m_hosts.begin(); i != end; ++i)
        count += i->value->activeLoadCount();

    return count;
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
