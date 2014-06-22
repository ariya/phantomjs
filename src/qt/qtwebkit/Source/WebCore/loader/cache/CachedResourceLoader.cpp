/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedResourceLoader.h"

#include "CachedCSSStyleSheet.h"
#include "CachedSVGDocument.h"
#include "CachedFont.h"
#include "CachedImage.h"
#include "CachedRawResource.h"
#include "CachedResourceRequest.h"
#include "CachedScript.h"
#include "CachedXSLStyleSheet.h"
#include "Console.h"
#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "LoaderStrategy.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "PingLoader.h"
#include "PlatformStrategies.h"
#include "ResourceLoadScheduler.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(VIDEO_TRACK)
#include "CachedTextTrack.h"
#endif

#if ENABLE(CSS_SHADERS)
#include "CachedShader.h"
#endif

#if ENABLE(RESOURCE_TIMING)
#include "Performance.h"
#endif

#define PRELOAD_DEBUG 0

namespace WebCore {

static CachedResource* createResource(CachedResource::Type type, ResourceRequest& request, const String& charset)
{
    switch (type) {
    case CachedResource::ImageResource:
        return new CachedImage(request);
    case CachedResource::CSSStyleSheet:
        return new CachedCSSStyleSheet(request, charset);
    case CachedResource::Script:
        return new CachedScript(request, charset);
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
        return new CachedSVGDocument(request);
#endif
    case CachedResource::FontResource:
        return new CachedFont(request);
    case CachedResource::RawResource:
    case CachedResource::MainResource:
        return new CachedRawResource(request, type);
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        return new CachedXSLStyleSheet(request);
#endif
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
        return new CachedResource(request, CachedResource::LinkPrefetch);
    case CachedResource::LinkSubresource:
        return new CachedResource(request, CachedResource::LinkSubresource);
#endif
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
        return new CachedTextTrack(request);
#endif
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
        return new CachedShader(request);
#endif
    }
    ASSERT_NOT_REACHED();
    return 0;
}

CachedResourceLoader::CachedResourceLoader(DocumentLoader* documentLoader)
    : m_document(0)
    , m_documentLoader(documentLoader)
    , m_requestCount(0)
    , m_garbageCollectDocumentResourcesTimer(this, &CachedResourceLoader::garbageCollectDocumentResourcesTimerFired)
    , m_autoLoadImages(true)
    , m_imagesEnabled(true)
    , m_allowStaleResources(false)
{
}

CachedResourceLoader::~CachedResourceLoader()
{
    m_documentLoader = 0;
    m_document = 0;

    clearPreloads();
    DocumentResourceMap::iterator end = m_documentResources.end();
    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != end; ++it)
        it->value->setOwningCachedResourceLoader(0);

    // Make sure no requests still point to this CachedResourceLoader
    ASSERT(m_requestCount == 0);
}

CachedResource* CachedResourceLoader::cachedResource(const String& resourceURL) const 
{
    KURL url = m_document->completeURL(resourceURL);
    return cachedResource(url); 
}

CachedResource* CachedResourceLoader::cachedResource(const KURL& resourceURL) const
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(resourceURL);
    return m_documentResources.get(url).get(); 
}

Frame* CachedResourceLoader::frame() const
{
    return m_documentLoader ? m_documentLoader->frame() : 0;
}

CachedResourceHandle<CachedImage> CachedResourceLoader::requestImage(CachedResourceRequest& request)
{
    if (Frame* f = frame()) {
        if (f->loader()->pageDismissalEventBeingDispatched() != FrameLoader::NoDismissal) {
            KURL requestURL = request.resourceRequest().url();
            if (requestURL.isValid() && canRequest(CachedResource::ImageResource, requestURL, request.options(), request.forPreload()))
                PingLoader::loadImage(f, requestURL);
            return 0;
        }
    }
    request.setDefer(clientDefersImage(request.resourceRequest().url()) ? CachedResourceRequest::DeferredByClient : CachedResourceRequest::NoDefer);
    return static_cast<CachedImage*>(requestResource(CachedResource::ImageResource, request).get());
}

CachedResourceHandle<CachedFont> CachedResourceLoader::requestFont(CachedResourceRequest& request)
{
    return static_cast<CachedFont*>(requestResource(CachedResource::FontResource, request).get());
}

#if ENABLE(VIDEO_TRACK)
CachedResourceHandle<CachedTextTrack> CachedResourceLoader::requestTextTrack(CachedResourceRequest& request)
{
    return static_cast<CachedTextTrack*>(requestResource(CachedResource::TextTrackResource, request).get());
}
#endif

#if ENABLE(CSS_SHADERS)
CachedResourceHandle<CachedShader> CachedResourceLoader::requestShader(CachedResourceRequest& request)
{
    return static_cast<CachedShader*>(requestResource(CachedResource::ShaderResource, request).get());
}
#endif

CachedResourceHandle<CachedCSSStyleSheet> CachedResourceLoader::requestCSSStyleSheet(CachedResourceRequest& request)
{
    return static_cast<CachedCSSStyleSheet*>(requestResource(CachedResource::CSSStyleSheet, request).get());
}

CachedResourceHandle<CachedCSSStyleSheet> CachedResourceLoader::requestUserCSSStyleSheet(CachedResourceRequest& request)
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(request.resourceRequest().url());

#if ENABLE(CACHE_PARTITIONING)
    request.mutableResourceRequest().setCachePartition(document()->topOrigin()->cachePartition());
#endif

    if (CachedResource* existing = memoryCache()->resourceForRequest(request.resourceRequest())) {
        if (existing->type() == CachedResource::CSSStyleSheet)
            return static_cast<CachedCSSStyleSheet*>(existing);
        memoryCache()->remove(existing);
    }
    if (url.string() != request.resourceRequest().url())
        request.mutableResourceRequest().setURL(url);

    CachedResourceHandle<CachedCSSStyleSheet> userSheet = new CachedCSSStyleSheet(request.resourceRequest(), request.charset());

    memoryCache()->add(userSheet.get());
    // FIXME: loadResource calls setOwningCachedResourceLoader() if the resource couldn't be added to cache. Does this function need to call it, too?

    userSheet->load(this, ResourceLoaderOptions(DoNotSendCallbacks, SniffContent, BufferData, AllowStoredCredentials, AskClientForAllCredentials, SkipSecurityCheck, UseDefaultOriginRestrictionsForType));
    
    return userSheet;
}

CachedResourceHandle<CachedScript> CachedResourceLoader::requestScript(CachedResourceRequest& request)
{
    return static_cast<CachedScript*>(requestResource(CachedResource::Script, request).get());
}

#if ENABLE(XSLT)
CachedResourceHandle<CachedXSLStyleSheet> CachedResourceLoader::requestXSLStyleSheet(CachedResourceRequest& request)
{
    return static_cast<CachedXSLStyleSheet*>(requestResource(CachedResource::XSLStyleSheet, request).get());
}
#endif

#if ENABLE(SVG)
CachedResourceHandle<CachedSVGDocument> CachedResourceLoader::requestSVGDocument(CachedResourceRequest& request)
{
    return static_cast<CachedSVGDocument*>(requestResource(CachedResource::SVGDocumentResource, request).get());
}
#endif

#if ENABLE(LINK_PREFETCH)
CachedResourceHandle<CachedResource> CachedResourceLoader::requestLinkResource(CachedResource::Type type, CachedResourceRequest& request)
{
    ASSERT(frame());
    ASSERT(type == CachedResource::LinkPrefetch || type == CachedResource::LinkSubresource);
    return requestResource(type, request);
}
#endif

CachedResourceHandle<CachedRawResource> CachedResourceLoader::requestRawResource(CachedResourceRequest& request)
{
    return static_cast<CachedRawResource*>(requestResource(CachedResource::RawResource, request).get());
}

CachedResourceHandle<CachedRawResource> CachedResourceLoader::requestMainResource(CachedResourceRequest& request)
{
    return static_cast<CachedRawResource*>(requestResource(CachedResource::MainResource, request).get());
}

bool CachedResourceLoader::checkInsecureContent(CachedResource::Type type, const KURL& url) const
{
    switch (type) {
    case CachedResource::Script:
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
#endif
    case CachedResource::CSSStyleSheet:
        // These resource can inject script into the current document (Script,
        // XSL) or exfiltrate the content of the current document (CSS).
        if (Frame* f = frame())
            if (!f->loader()->mixedContentChecker()->canRunInsecureContent(m_document->securityOrigin(), url))
                return false;
        break;
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
#endif
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
#endif
    case CachedResource::RawResource:
    case CachedResource::ImageResource:
    case CachedResource::FontResource: {
        // These resources can corrupt only the frame's pixels.
        if (Frame* f = frame()) {
            Frame* top = f->tree()->top();
            if (!top->loader()->mixedContentChecker()->canDisplayInsecureContent(top->document()->securityOrigin(), url))
                return false;
        }
        break;
    }
    case CachedResource::MainResource:
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
        // Prefetch cannot affect the current document.
#endif
        break;
    }
    return true;
}

bool CachedResourceLoader::canRequest(CachedResource::Type type, const KURL& url, const ResourceLoaderOptions& options, bool forPreload)
{
    if (document() && !document()->securityOrigin()->canDisplay(url)) {
        if (!forPreload)
            FrameLoader::reportLocalLoadFailed(frame(), url.stringCenterEllipsizedToLength());
        LOG(ResourceLoading, "CachedResourceLoader::requestResource URL was not allowed by SecurityOrigin::canDisplay");
        return 0;
    }

    // FIXME: Convert this to check the isolated world's Content Security Policy once webkit.org/b/104520 is solved.
    bool shouldBypassMainWorldContentSecurityPolicy = (frame() && frame()->script()->shouldBypassMainWorldContentSecurityPolicy());

    // Some types of resources can be loaded only from the same origin.  Other
    // types of resources, like Images, Scripts, and CSS, can be loaded from
    // any URL.
    switch (type) {
    case CachedResource::MainResource:
    case CachedResource::ImageResource:
    case CachedResource::CSSStyleSheet:
    case CachedResource::Script:
    case CachedResource::FontResource:
    case CachedResource::RawResource:
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
#endif
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
#endif
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
#endif
        if (options.requestOriginPolicy == RestrictToSameOrigin && !m_document->securityOrigin()->canRequest(url)) {
            printAccessDeniedMessage(url);
            return false;
        }
        break;
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
#endif
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        if (!m_document->securityOrigin()->canRequest(url)) {
            printAccessDeniedMessage(url);
            return false;
        }
#endif
        break;
    }

    switch (type) {
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowScriptFromSource(url))
            return false;
        break;
#endif
    case CachedResource::Script:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowScriptFromSource(url))
            return false;

        if (frame()) {
            Settings* settings = frame()->settings();
            if (!frame()->loader()->client()->allowScriptFromSource(!settings || settings->isScriptEnabled(), url)) {
                frame()->loader()->client()->didNotAllowScript();
                return false;
            }
        }
        break;
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
        // Since shaders are referenced from CSS Styles use the same rules here.
#endif
    case CachedResource::CSSStyleSheet:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowStyleFromSource(url))
            return false;
        break;
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
#endif
    case CachedResource::ImageResource:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowImageFromSource(url))
            return false;
        break;
    case CachedResource::FontResource: {
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowFontFromSource(url))
            return false;
        break;
    }
    case CachedResource::MainResource:
    case CachedResource::RawResource:
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
#endif
        break;
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
        // Cues aren't called out in the CPS spec yet, but they only work with a media element
        // so use the media policy.
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowMediaFromSource(url))
            return false;
        break;
#endif
    }

    // Last of all, check for insecure content. We do this last so that when
    // folks block insecure content with a CSP policy, they don't get a warning.
    // They'll still get a warning in the console about CSP blocking the load.

    // FIXME: Should we consider forPreload here?
    if (!checkInsecureContent(type, url))
        return false;

    return true;
}

bool CachedResourceLoader::shouldContinueAfterNotifyingLoadedFromMemoryCache(CachedResource* resource)
{
    if (!resource || !frame() || resource->status() != CachedResource::Cached)
        return true;

    ResourceRequest newRequest;
    frame()->loader()->loadedResourceFromMemoryCache(resource, newRequest);
    
    // FIXME <http://webkit.org/b/113251>: If the delegate modifies the request's
    // URL, it is no longer appropriate to use this CachedResource.
    return !newRequest.isNull();
}

CachedResourceHandle<CachedResource> CachedResourceLoader::requestResource(CachedResource::Type type, CachedResourceRequest& request)
{
    KURL url = request.resourceRequest().url();
    
    LOG(ResourceLoading, "CachedResourceLoader::requestResource '%s', charset '%s', priority=%d, forPreload=%u", url.stringCenterEllipsizedToLength().latin1().data(), request.charset().latin1().data(), request.priority(), request.forPreload());
    
    // If only the fragment identifiers differ, it is the same resource.
    url = MemoryCache::removeFragmentIdentifierIfNeeded(url);

    if (!url.isValid())
        return 0;

    if (!canRequest(type, url, request.options(), request.forPreload()))
        return 0;

    if (Frame* f = frame())
        f->loader()->client()->dispatchWillRequestResource(&request);

    if (memoryCache()->disabled()) {
        DocumentResourceMap::iterator it = m_documentResources.find(url.string());
        if (it != m_documentResources.end()) {
            it->value->setOwningCachedResourceLoader(0);
            m_documentResources.remove(it);
        }
    }

    // See if we can use an existing resource from the cache.
    CachedResourceHandle<CachedResource> resource;
#if ENABLE(CACHE_PARTITIONING)
    if (document())
        request.mutableResourceRequest().setCachePartition(document()->topOrigin()->cachePartition());
#endif

    resource = memoryCache()->resourceForRequest(request.resourceRequest());

    const RevalidationPolicy policy = determineRevalidationPolicy(type, request.mutableResourceRequest(), request.forPreload(), resource.get(), request.defer());
    switch (policy) {
    case Reload:
        memoryCache()->remove(resource.get());
        // Fall through
    case Load:
        resource = loadResource(type, request, request.charset());
        break;
    case Revalidate:
        resource = revalidateResource(request, resource.get());
        break;
    case Use:
        if (!shouldContinueAfterNotifyingLoadedFromMemoryCache(resource.get()))
            return 0;
        memoryCache()->resourceAccessed(resource.get());
        break;
    }

    if (!resource)
        return 0;

    if (!request.forPreload() || policy != Use)
        resource->setLoadPriority(request.priority());

    if ((policy != Use || resource->stillNeedsLoad()) && CachedResourceRequest::NoDefer == request.defer()) {
        resource->load(this, request.options());

        // We don't support immediate loads, but we do support immediate failure.
        if (resource->errorOccurred()) {
            if (resource->inCache())
                memoryCache()->remove(resource.get());
            return 0;
        }
    }

    if (!request.resourceRequest().url().protocolIsData())
        m_validatedURLs.add(request.resourceRequest().url());

    ASSERT(resource->url() == url.string());
    m_documentResources.set(resource->url(), resource);
    return resource;
}

CachedResourceHandle<CachedResource> CachedResourceLoader::revalidateResource(const CachedResourceRequest& request, CachedResource* resource)
{
    ASSERT(resource);
    ASSERT(resource->inCache());
    ASSERT(!memoryCache()->disabled());
    ASSERT(resource->canUseCacheValidator());
    ASSERT(!resource->resourceToRevalidate());
    
    // Copy the URL out of the resource to be revalidated in case it gets deleted by the remove() call below.
    String url = resource->url();
    CachedResourceHandle<CachedResource> newResource = createResource(resource->type(), resource->resourceRequest(), resource->encoding());
    
    LOG(ResourceLoading, "Resource %p created to revalidate %p", newResource.get(), resource);
    newResource->setResourceToRevalidate(resource);
    
    memoryCache()->remove(resource);
    memoryCache()->add(newResource.get());
#if ENABLE(RESOURCE_TIMING)
    storeResourceTimingInitiatorInformation(resource, request);
#else
    UNUSED_PARAM(request);
#endif
    return newResource;
}

CachedResourceHandle<CachedResource> CachedResourceLoader::loadResource(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    ASSERT(!memoryCache()->resourceForRequest(request.resourceRequest()));

    LOG(ResourceLoading, "Loading CachedResource for '%s'.", request.resourceRequest().url().stringCenterEllipsizedToLength().latin1().data());

    CachedResourceHandle<CachedResource> resource = createResource(type, request.mutableResourceRequest(), charset);

    if (!memoryCache()->add(resource.get()))
        resource->setOwningCachedResourceLoader(this);
#if ENABLE(RESOURCE_TIMING)
    storeResourceTimingInitiatorInformation(resource, request);
#endif
    return resource;
}

#if ENABLE(RESOURCE_TIMING)
void CachedResourceLoader::storeResourceTimingInitiatorInformation(const CachedResourceHandle<CachedResource>& resource, const CachedResourceRequest& request)
{
    if (resource->type() == CachedResource::MainResource) {
        // <iframe>s should report the initial navigation requested by the parent document, but not subsequent navigations.
        if (frame()->ownerElement() && m_documentLoader->frameLoader()->stateMachine()->committingFirstRealLoad()) {
            InitiatorInfo info = { frame()->ownerElement()->localName(), monotonicallyIncreasingTime() };
            m_initiatorMap.add(resource.get(), info);
        }
    } else {
        InitiatorInfo info = { request.initiatorName(), monotonicallyIncreasingTime() };
        m_initiatorMap.add(resource.get(), info);
    }
}
#endif // ENABLE(RESOURCE_TIMING)

CachedResourceLoader::RevalidationPolicy CachedResourceLoader::determineRevalidationPolicy(CachedResource::Type type, ResourceRequest& request, bool forPreload, CachedResource* existingResource, CachedResourceRequest::DeferOption defer) const
{
    if (!existingResource)
        return Load;

    // We already have a preload going for this URL.
    if (forPreload && existingResource->isPreloaded())
        return Use;

    // If the same URL has been loaded as a different type, we need to reload.
    if (existingResource->type() != type) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to type mismatch.");
        return Reload;
    }

    if (!existingResource->canReuse(request))
        return Reload;

    // Certain requests (e.g., XHRs) might have manually set headers that require revalidation.
    // FIXME: In theory, this should be a Revalidate case. In practice, the MemoryCache revalidation path assumes a whole bunch
    // of things about how revalidation works that manual headers violate, so punt to Reload instead.
    if (request.isConditional())
        return Reload;

    // Do not load from cache if images are not enabled. The load for this image will be blocked
    // in CachedImage::load.
    if (CachedResourceRequest::DeferredByClient == defer)
        return Reload;
    
    // Don't reload resources while pasting.
    if (m_allowStaleResources)
        return Use;
    
    // Alwaus use preloads.
    if (existingResource->isPreloaded())
        return Use;
    
    // CachePolicyHistoryBuffer uses the cache no matter what.
    if (cachePolicy(type) == CachePolicyHistoryBuffer)
        return Use;

    // Don't reuse resources with Cache-control: no-store.
    if (existingResource->response().cacheControlContainsNoStore()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to Cache-control: no-store.");
        return Reload;
    }

    // If credentials were sent with the previous request and won't be
    // with this one, or vice versa, re-fetch the resource.
    //
    // This helps with the case where the server sends back
    // "Access-Control-Allow-Origin: *" all the time, but some of the
    // client's requests are made without CORS and some with.
    if (existingResource->resourceRequest().allowCookies() != request.allowCookies()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to difference in credentials settings.");
        return Reload;
    }

    // During the initial load, avoid loading the same resource multiple times for a single document, even if the cache policies would tell us to.
    if (document() && !document()->loadEventFinished() && m_validatedURLs.contains(existingResource->url()))
        return Use;

    // CachePolicyReload always reloads
    if (cachePolicy(type) == CachePolicyReload) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to CachePolicyReload.");
        return Reload;
    }
    
    // We'll try to reload the resource if it failed last time.
    if (existingResource->errorOccurred()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicye reloading due to resource being in the error state");
        return Reload;
    }
    
    // For resources that are not yet loaded we ignore the cache policy.
    if (existingResource->isLoading())
        return Use;

    // Check if the cache headers requires us to revalidate (cache expiration for example).
    if (existingResource->mustRevalidateDueToCacheHeaders(cachePolicy(type))) {
        // See if the resource has usable ETag or Last-modified headers.
        if (existingResource->canUseCacheValidator())
            return Revalidate;
        
        // No, must reload.
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to missing cache validators.");            
        return Reload;
    }

    return Use;
}

void CachedResourceLoader::printAccessDeniedMessage(const KURL& url) const
{
    if (url.isNull())
        return;

    if (!frame())
        return;

    String message;
    if (!m_document || m_document->url().isNull())
        message = "Unsafe attempt to load URL " + url.stringCenterEllipsizedToLength() + '.';
    else
        message = "Unsafe attempt to load URL " + url.stringCenterEllipsizedToLength() + " from frame with URL " + m_document->url().stringCenterEllipsizedToLength() + ". Domains, protocols and ports must match.\n";

    frame()->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, message);
}

void CachedResourceLoader::setAutoLoadImages(bool enable)
{
    if (enable == m_autoLoadImages)
        return;

    m_autoLoadImages = enable;

    if (!m_autoLoadImages)
        return;

    reloadImagesIfNotDeferred();
}

void CachedResourceLoader::setImagesEnabled(bool enable)
{
    if (enable == m_imagesEnabled)
        return;

    m_imagesEnabled = enable;

    if (!m_imagesEnabled)
        return;

    reloadImagesIfNotDeferred();
}

bool CachedResourceLoader::clientDefersImage(const KURL& url) const
{
    return frame() && !frame()->loader()->client()->allowImage(m_imagesEnabled, url);
}

bool CachedResourceLoader::shouldDeferImageLoad(const KURL& url) const
{
    return clientDefersImage(url) || !m_autoLoadImages;
}

void CachedResourceLoader::reloadImagesIfNotDeferred()
{
    DocumentResourceMap::iterator end = m_documentResources.end();
    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != end; ++it) {
        CachedResource* resource = it->value.get();
        if (resource->type() == CachedResource::ImageResource && resource->stillNeedsLoad() && !clientDefersImage(resource->url()))
            const_cast<CachedResource*>(resource)->load(this, defaultCachedResourceOptions());
    }
}

CachePolicy CachedResourceLoader::cachePolicy(CachedResource::Type type) const
{
    if (!frame())
        return CachePolicyVerify;

    if (type != CachedResource::MainResource)
        return frame()->loader()->subresourceCachePolicy();
    
    if (frame()->loader()->loadType() == FrameLoadTypeReloadFromOrigin || frame()->loader()->loadType() == FrameLoadTypeReload)
        return CachePolicyReload;
    return CachePolicyVerify;
}

void CachedResourceLoader::removeCachedResource(CachedResource* resource) const
{
#ifndef NDEBUG
    DocumentResourceMap::iterator it = m_documentResources.find(resource->url());
    if (it != m_documentResources.end())
        ASSERT(it->value.get() == resource);
#endif
    m_documentResources.remove(resource->url());
}

void CachedResourceLoader::loadDone(CachedResource* resource)
{
    RefPtr<DocumentLoader> protectDocumentLoader(m_documentLoader);
    RefPtr<Document> protectDocument(m_document);

#if ENABLE(RESOURCE_TIMING)
    if (resource && resource->response().isHTTP() && ((!resource->errorOccurred() && !resource->wasCanceled()) || resource->response().httpStatusCode() == 304)) {
        HashMap<CachedResource*, InitiatorInfo>::iterator initiatorIt = m_initiatorMap.find(resource);
        if (initiatorIt != m_initiatorMap.end()) {
            ASSERT(document());
            Document* initiatorDocument = document();
            if (resource->type() == CachedResource::MainResource)
                initiatorDocument = document()->parentDocument();
            ASSERT(initiatorDocument);
            const InitiatorInfo& info = initiatorIt->value;
            initiatorDocument->domWindow()->performance()->addResourceTiming(info.name, initiatorDocument, resource->resourceRequest(), resource->response(), info.startTime, resource->loadFinishTime());
            m_initiatorMap.remove(initiatorIt);
        }
    }
#else
    UNUSED_PARAM(resource);
#endif // ENABLE(RESOURCE_TIMING)

    if (frame())
        frame()->loader()->loadDone();
    performPostLoadActions();

    if (!m_garbageCollectDocumentResourcesTimer.isActive())
        m_garbageCollectDocumentResourcesTimer.startOneShot(0);
}

// Garbage collecting m_documentResources is a workaround for the
// CachedResourceHandles on the RHS being strong references. Ideally this
// would be a weak map, however CachedResourceHandles perform additional
// bookkeeping on CachedResources, so instead pseudo-GC them -- when the
// reference count reaches 1, m_documentResources is the only reference, so
// remove it from the map.
void CachedResourceLoader::garbageCollectDocumentResourcesTimerFired(Timer<CachedResourceLoader>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_garbageCollectDocumentResourcesTimer);
    garbageCollectDocumentResources();
}

void CachedResourceLoader::garbageCollectDocumentResources()
{
    typedef Vector<String, 10> StringVector;
    StringVector resourcesToDelete;

    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != m_documentResources.end(); ++it) {
        if (it->value->hasOneHandle()) {
            resourcesToDelete.append(it->key);
            it->value->setOwningCachedResourceLoader(0);
        }
    }

    for (StringVector::const_iterator it = resourcesToDelete.begin(); it != resourcesToDelete.end(); ++it)
        m_documentResources.remove(*it);
}

void CachedResourceLoader::performPostLoadActions()
{
    checkForPendingPreloads();

    platformStrategies()->loaderStrategy()->resourceLoadScheduler()->servePendingRequests();
}

void CachedResourceLoader::incrementRequestCount(const CachedResource* res)
{
    if (res->ignoreForRequestCount())
        return;

    ++m_requestCount;
}

void CachedResourceLoader::decrementRequestCount(const CachedResource* res)
{
    if (res->ignoreForRequestCount())
        return;

    --m_requestCount;
    ASSERT(m_requestCount > -1);
}

void CachedResourceLoader::preload(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    bool delaySubresourceLoad = true;
#if PLATFORM(IOS)
    delaySubresourceLoad = false;
#endif
    if (delaySubresourceLoad) {
        bool hasRendering = m_document->body() && m_document->body()->renderer();
        bool canBlockParser = type == CachedResource::Script || type == CachedResource::CSSStyleSheet;
        if (!hasRendering && !canBlockParser) {
            // Don't preload subresources that can't block the parser before we have something to draw.
            // This helps prevent preloads from delaying first display when bandwidth is limited.
            PendingPreload pendingPreload = { type, request, charset };
            m_pendingPreloads.append(pendingPreload);
            return;
        }
    }
    requestPreload(type, request, charset);
}

void CachedResourceLoader::checkForPendingPreloads() 
{
    if (m_pendingPreloads.isEmpty() || !m_document->body() || !m_document->body()->renderer())
        return;
    while (!m_pendingPreloads.isEmpty()) {
        PendingPreload preload = m_pendingPreloads.takeFirst();
        // Don't request preload if the resource already loaded normally (this will result in double load if the page is being reloaded with cached results ignored).
        if (!cachedResource(preload.m_request.resourceRequest().url()))
            requestPreload(preload.m_type, preload.m_request, preload.m_charset);
    }
    m_pendingPreloads.clear();
}

void CachedResourceLoader::requestPreload(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    String encoding;
    if (type == CachedResource::Script || type == CachedResource::CSSStyleSheet)
        encoding = charset.isEmpty() ? m_document->charset() : charset;

    request.setCharset(encoding);
    request.setForPreload(true);

    CachedResourceHandle<CachedResource> resource = requestResource(type, request);
    if (!resource || (m_preloads && m_preloads->contains(resource.get())))
        return;
    resource->increasePreloadCount();

    if (!m_preloads)
        m_preloads = adoptPtr(new ListHashSet<CachedResource*>);
    m_preloads->add(resource.get());

#if PRELOAD_DEBUG
    printf("PRELOADING %s\n",  resource->url().latin1().data());
#endif
}

bool CachedResourceLoader::isPreloaded(const String& urlString) const
{
    const KURL& url = m_document->completeURL(urlString);

    if (m_preloads) {
        ListHashSet<CachedResource*>::iterator end = m_preloads->end();
        for (ListHashSet<CachedResource*>::iterator it = m_preloads->begin(); it != end; ++it) {
            CachedResource* resource = *it;
            if (resource->url() == url)
                return true;
        }
    }

    Deque<PendingPreload>::const_iterator dequeEnd = m_pendingPreloads.end();
    for (Deque<PendingPreload>::const_iterator it = m_pendingPreloads.begin(); it != dequeEnd; ++it) {
        PendingPreload pendingPreload = *it;
        if (pendingPreload.m_request.resourceRequest().url() == url)
            return true;
    }
    return false;
}

void CachedResourceLoader::clearPreloads()
{
#if PRELOAD_DEBUG
    printPreloadStats();
#endif
    if (!m_preloads)
        return;

    ListHashSet<CachedResource*>::iterator end = m_preloads->end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads->begin(); it != end; ++it) {
        CachedResource* res = *it;
        res->decreasePreloadCount();
        bool deleted = res->deleteIfPossible();
        if (!deleted && res->preloadResult() == CachedResource::PreloadNotReferenced)
            memoryCache()->remove(res);
    }
    m_preloads.clear();
}

void CachedResourceLoader::clearPendingPreloads()
{
    m_pendingPreloads.clear();
}

#if PRELOAD_DEBUG
void CachedResourceLoader::printPreloadStats()
{
    unsigned scripts = 0;
    unsigned scriptMisses = 0;
    unsigned stylesheets = 0;
    unsigned stylesheetMisses = 0;
    unsigned images = 0;
    unsigned imageMisses = 0;
    ListHashSet<CachedResource*>::iterator end = m_preloads.end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads.begin(); it != end; ++it) {
        CachedResource* res = *it;
        if (res->preloadResult() == CachedResource::PreloadNotReferenced)
            printf("!! UNREFERENCED PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileComplete)
            printf("HIT COMPLETE PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileLoading)
            printf("HIT LOADING PRELOAD %s\n", res->url().latin1().data());
        
        if (res->type() == CachedResource::Script) {
            scripts++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                scriptMisses++;
        } else if (res->type() == CachedResource::CSSStyleSheet) {
            stylesheets++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                stylesheetMisses++;
        } else {
            images++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                imageMisses++;
        }
        
        if (res->errorOccurred())
            memoryCache()->remove(res);
        
        res->decreasePreloadCount();
    }
    m_preloads.clear();
    
    if (scripts)
        printf("SCRIPTS: %d (%d hits, hit rate %d%%)\n", scripts, scripts - scriptMisses, (scripts - scriptMisses) * 100 / scripts);
    if (stylesheets)
        printf("STYLESHEETS: %d (%d hits, hit rate %d%%)\n", stylesheets, stylesheets - stylesheetMisses, (stylesheets - stylesheetMisses) * 100 / stylesheets);
    if (images)
        printf("IMAGES:  %d (%d hits, hit rate %d%%)\n", images, images - imageMisses, (images - imageMisses) * 100 / images);
}
#endif

const ResourceLoaderOptions& CachedResourceLoader::defaultCachedResourceOptions()
{
    static ResourceLoaderOptions options(SendCallbacks, SniffContent, BufferData, AllowStoredCredentials, AskClientForAllCredentials, DoSecurityCheck, UseDefaultOriginRestrictionsForType);
    return options;
}

}
