/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#include "WebPlatformStrategies.h"

#include "BlockingResponseMap.h"
#include "DataReference.h"
#include "NetworkResourceLoadParameters.h"
#include "PluginInfoStore.h"
#include "StorageNamespaceImpl.h"
#include "WebContextMessages.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebErrors.h"
#include "WebPage.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/Color.h>
#include <WebCore/KURL.h>
#include <WebCore/LoaderStrategy.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/NetworkingContext.h>
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PlatformCookieJar.h>
#include <WebCore/PlatformPasteboard.h>
#include <WebCore/ResourceError.h>
#include <WebCore/StorageNamespace.h>
#include <wtf/Atomics.h>

#if ENABLE(NETWORK_PROCESS)
#include "BlobRegistryProxy.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "WebResourceLoadScheduler.h"
#endif

// FIXME: Remove this #ifdef once we don't need the ability to turn the feature off.
#define ENABLE_UI_PROCESS_STORAGE 1

using namespace WebCore;

namespace WebKit {

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

WebPlatformStrategies::WebPlatformStrategies()
#if ENABLE(NETSCAPE_PLUGIN_API)
    : m_pluginCacheIsPopulated(false)
    , m_shouldRefreshPlugins(false)
#endif // ENABLE(NETSCAPE_PLUGIN_API)
{
}

CookiesStrategy* WebPlatformStrategies::createCookiesStrategy()
{
    return this;
}

DatabaseStrategy* WebPlatformStrategies::createDatabaseStrategy()
{
    return this;
}

LoaderStrategy* WebPlatformStrategies::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* WebPlatformStrategies::createPasteboardStrategy()
{
    return this;
}

PluginStrategy* WebPlatformStrategies::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* WebPlatformStrategies::createSharedWorkerStrategy()
{
    return this;
}

StorageStrategy* WebPlatformStrategies::createStorageStrategy()
{
    return this;
}

VisitedLinkStrategy* WebPlatformStrategies::createVisitedLinkStrategy()
{
    return this;
}

// CookiesStrategy

String WebPlatformStrategies::cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        String result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookiesForDOM(session.isPrivateBrowsingSession(), firstParty, url), Messages::NetworkConnectionToWebProcess::CookiesForDOM::Reply(result), 0))
            return String();
        return result;
    }
#endif

    return WebCore::cookiesForDOM(session, firstParty, url);
}

void WebPlatformStrategies::setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieString)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::SetCookiesFromDOM(session.isPrivateBrowsingSession(), firstParty, url, cookieString), 0);
        return;
    }
#endif

    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool WebPlatformStrategies::cookiesEnabled(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        bool result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookiesEnabled(session.isPrivateBrowsingSession(), firstParty, url), Messages::NetworkConnectionToWebProcess::CookiesEnabled::Reply(result), 0))
            return false;
        return result;
    }
#endif

    return WebCore::cookiesEnabled(session, firstParty, url);
}

String WebPlatformStrategies::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        String result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue(session.isPrivateBrowsingSession(), firstParty, url), Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue::Reply(result), 0))
            return String();
        return result;
    }
#endif

    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool WebPlatformStrategies::getRawCookies(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::GetRawCookies(session.isPrivateBrowsingSession(), firstParty, url), Messages::NetworkConnectionToWebProcess::GetRawCookies::Reply(rawCookies), 0))
            return false;
        return true;
    }
#endif

    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void WebPlatformStrategies::deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::DeleteCookie(session.isPrivateBrowsingSession(), url, cookieName), 0);
        return;
    }
#endif

    WebCore::deleteCookie(session, url, cookieName);
}

// DatabaseStrategy

#if ENABLE(SQL_DATABASE)
AbstractDatabaseServer* WebPlatformStrategies::getDatabaseServer()
{
    return DatabaseStrategy::getDatabaseServer(); // Use the default for now.
}
#endif

// LoaderStrategy

#if ENABLE(NETWORK_PROCESS)
ResourceLoadScheduler* WebPlatformStrategies::resourceLoadScheduler()
{
    static ResourceLoadScheduler* scheduler;
    if (!scheduler) {
        if (WebProcess::shared().usesNetworkProcess())
            scheduler = &WebProcess::shared().webResourceLoadScheduler();
        else
            scheduler = WebCore::resourceLoadScheduler();
    }
    
    return scheduler;
}

void WebPlatformStrategies::loadResourceSynchronously(NetworkingContext* context, unsigned long resourceLoadIdentifier, const ResourceRequest& request, StoredCredentials storedCredentials, ClientCredentialPolicy clientCredentialPolicy, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    if (!WebProcess::shared().usesNetworkProcess()) {
        LoaderStrategy::loadResourceSynchronously(context, resourceLoadIdentifier, request, storedCredentials, clientCredentialPolicy, error, response, data);
        return;
    }

    CoreIPC::DataReference dataReference;

    NetworkResourceLoadParameters loadParameters;
    loadParameters.identifier = resourceLoadIdentifier;
    loadParameters.request = request;
    loadParameters.priority = ResourceLoadPriorityHighest;
    loadParameters.contentSniffingPolicy = SniffContent;
    loadParameters.allowStoredCredentials = storedCredentials;
    loadParameters.clientCredentialPolicy = clientCredentialPolicy;
    loadParameters.inPrivateBrowsingMode = context->storageSession().isPrivateBrowsingSession();
    loadParameters.shouldClearReferrerOnHTTPSToHTTPRedirect = context->shouldClearReferrerOnHTTPSToHTTPRedirect();

    if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad(loadParameters), Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::Reply(error, response, dataReference), 0)) {
        response = ResourceResponse();
        error = internalError(request.url());
        data.resize(0);

        return;
    }

    data.resize(dataReference.size());
    memcpy(data.data(), dataReference.data(), dataReference.size());
}

#if ENABLE(BLOB)
BlobRegistry* WebPlatformStrategies::createBlobRegistry()
{
    if (!WebProcess::shared().usesNetworkProcess())
        return LoaderStrategy::createBlobRegistry();
    return new BlobRegistryProxy;    
}
#endif
#endif

// PluginStrategy

void WebPlatformStrategies::refreshPlugins()
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    m_cachedPlugins.clear();
    m_pluginCacheIsPopulated = false;
    m_shouldRefreshPlugins = true;

    populatePluginCache();
#endif // ENABLE(NETSCAPE_PLUGIN_API)
}

void WebPlatformStrategies::getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>& plugins)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    populatePluginCache();
    plugins = m_cachedPlugins;
#endif // ENABLE(NETSCAPE_PLUGIN_API)
}

#if ENABLE(NETSCAPE_PLUGIN_API)
void WebPlatformStrategies::populatePluginCache()
{
    if (m_pluginCacheIsPopulated)
        return;

    ASSERT(m_cachedPlugins.isEmpty());
    
    // FIXME: Should we do something in case of error here?
    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebProcessProxy::GetPlugins(m_shouldRefreshPlugins), Messages::WebProcessProxy::GetPlugins::Reply(m_cachedPlugins), 0))
        return;

    m_shouldRefreshPlugins = false;
    m_pluginCacheIsPopulated = true;
}
#endif // ENABLE(NETSCAPE_PLUGIN_API)

// SharedWorkerStrategy.

bool WebPlatformStrategies::isAvailable() const
{
    // Shared workers do not work across multiple processes, and using network process is tied to multiple secondary process model.
#if ENABLE(NETWORK_PROCESS)
    return !WebProcess::shared().usesNetworkProcess();
#else
    return true;
#endif
}

// StorageStrategy

PassRefPtr<StorageNamespace> WebPlatformStrategies::localStorageNamespace(PageGroup* pageGroup)
{
#if ENABLE(UI_PROCESS_STORAGE)
    return StorageNamespaceImpl::createLocalStorageNamespace(pageGroup);
#else
    return StorageStrategy::localStorageNamespace(pageGroup);
#endif
}

PassRefPtr<StorageNamespace> WebPlatformStrategies::transientLocalStorageNamespace(PageGroup* pageGroup, SecurityOrigin*securityOrigin)
{
#if ENABLE(UI_PROCESS_STORAGE)
    // FIXME: This could be more clever and made to work across processes.
    return StorageStrategy::sessionStorageNamespace(*pageGroup->pages().begin());
#else
    return StorageStrategy::transientLocalStorageNamespace(pageGroup, securityOrigin);
#endif
}

PassRefPtr<StorageNamespace> WebPlatformStrategies::sessionStorageNamespace(Page* page)
{
#if ENABLE(UI_PROCESS_STORAGE)
    return StorageNamespaceImpl::createSessionStorageNamespace(WebPage::fromCorePage(page));
#else
    return StorageStrategy::sessionStorageNamespace(page);
#endif
}

// VisitedLinkStrategy

bool WebPlatformStrategies::isLinkVisited(Page*, LinkHash linkHash, const KURL&, const AtomicString&)
{
    return WebProcess::shared().isLinkVisited(linkHash);
}

void WebPlatformStrategies::addVisitedLink(Page*, LinkHash linkHash)
{
    WebProcess::shared().addVisitedLink(linkHash);
}

#if PLATFORM(MAC)
// PasteboardStrategy

void WebPlatformStrategies::getTypes(Vector<String>& types, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardTypes(pasteboardName),
                                                Messages::WebContext::GetPasteboardTypes::Reply(types), 0);
}

PassRefPtr<WebCore::SharedBuffer> WebPlatformStrategies::bufferForType(const String& pasteboardType, const String& pasteboardName)
{
    SharedMemory::Handle handle;
    uint64_t size = 0;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardBufferForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardBufferForType::Reply(handle, size), 0);
    if (handle.isNull())
        return 0;
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(handle, SharedMemory::ReadOnly);
    return SharedBuffer::create(static_cast<unsigned char *>(sharedMemoryBuffer->data()), size);
}

void WebPlatformStrategies::getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardPathnamesForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardPathnamesForType::Reply(pathnames), 0);
}

String WebPlatformStrategies::stringForType(const String& pasteboardType, const String& pasteboardName)
{
    String value;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardStringForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardStringForType::Reply(value), 0);
    return value;
}

void WebPlatformStrategies::copy(const String& fromPasteboard, const String& toPasteboard)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::PasteboardCopy(fromPasteboard, toPasteboard), 0);
}

int WebPlatformStrategies::changeCount(const WTF::String &pasteboardName)
{
    uint64_t changeCount;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardChangeCount(pasteboardName),
                                                Messages::WebContext::GetPasteboardChangeCount::Reply(changeCount), 0);
    return changeCount;
}

String WebPlatformStrategies::uniqueName()
{
    String pasteboardName;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardUniqueName(),
                                                Messages::WebContext::GetPasteboardUniqueName::Reply(pasteboardName), 0);
    return pasteboardName;
}

Color WebPlatformStrategies::color(const String& pasteboardName)
{
    Color color;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardColor(pasteboardName),
                                                Messages::WebContext::GetPasteboardColor::Reply(color), 0);
    return color;
}

KURL WebPlatformStrategies::url(const String& pasteboardName)
{
    String urlString;
    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebContext::GetPasteboardURL(pasteboardName),
                                                Messages::WebContext::GetPasteboardURL::Reply(urlString), 0);
    return KURL(ParsedURLString, urlString);
}

void WebPlatformStrategies::addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::AddPasteboardTypes(pasteboardName, pasteboardTypes), 0);
}

void WebPlatformStrategies::setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardTypes(pasteboardName, pasteboardTypes), 0);
}

void WebPlatformStrategies::setBufferForType(PassRefPtr<SharedBuffer> buffer, const String& pasteboardType, const String& pasteboardName)
{
    SharedMemory::Handle handle;
    if (buffer) {
        RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(buffer->size());
        memcpy(sharedMemoryBuffer->data(), buffer->data(), buffer->size());
        sharedMemoryBuffer->createHandle(handle, SharedMemory::ReadOnly);
    }
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardBufferForType(pasteboardName, pasteboardType, handle, buffer ? buffer->size() : 0), 0);
}

void WebPlatformStrategies::setPathnamesForType(const Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardPathnamesForType(pasteboardName, pasteboardType, pathnames), 0);
}

void WebPlatformStrategies::setStringForType(const String& string, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardStringForType(pasteboardName, pasteboardType, string), 0);
}
#endif

} // namespace WebKit
