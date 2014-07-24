/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
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

#ifndef CachedResourceLoader_h
#define CachedResourceLoader_h

#include "CachePolicy.h"
#include "CachedResource.h"
#include "CachedResourceHandle.h"
#include "CachedResourceRequest.h"
#include "ResourceLoadPriority.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CachedCSSStyleSheet;
class CachedSVGDocument;
class CachedFont;
class CachedImage;
class CachedRawResource;
class CachedScript;
class CachedShader;
class CachedTextTrack;
class CachedXSLStyleSheet;
class Document;
class DocumentLoader;
class Frame;
class ImageLoader;
class KURL;

// The CachedResourceLoader provides a per-context interface to the MemoryCache
// and enforces a bunch of security checks and rules for resource revalidation.
// Its lifetime is roughly per-DocumentLoader, in that it is generally created
// in the DocumentLoader constructor and loses its ability to generate network
// requests when the DocumentLoader is destroyed. Documents also hold a 
// RefPtr<CachedResourceLoader> for their lifetime (and will create one if they
// are initialized without a Frame), so a Document can keep a CachedResourceLoader
// alive past detach if scripts still reference the Document.
class CachedResourceLoader : public RefCounted<CachedResourceLoader> {
    WTF_MAKE_NONCOPYABLE(CachedResourceLoader); WTF_MAKE_FAST_ALLOCATED;
friend class ImageLoader;
friend class ResourceCacheValidationSuppressor;

public:
    static PassRefPtr<CachedResourceLoader> create(DocumentLoader* documentLoader) { return adoptRef(new CachedResourceLoader(documentLoader)); }
    ~CachedResourceLoader();

    CachedResourceHandle<CachedImage> requestImage(CachedResourceRequest&);
    CachedResourceHandle<CachedCSSStyleSheet> requestCSSStyleSheet(CachedResourceRequest&);
    CachedResourceHandle<CachedCSSStyleSheet> requestUserCSSStyleSheet(CachedResourceRequest&);
    CachedResourceHandle<CachedScript> requestScript(CachedResourceRequest&);
    CachedResourceHandle<CachedFont> requestFont(CachedResourceRequest&);
    CachedResourceHandle<CachedRawResource> requestRawResource(CachedResourceRequest&);
    CachedResourceHandle<CachedRawResource> requestMainResource(CachedResourceRequest&);

#if ENABLE(SVG)
    CachedResourceHandle<CachedSVGDocument> requestSVGDocument(CachedResourceRequest&);
#endif
#if ENABLE(XSLT)
    CachedResourceHandle<CachedXSLStyleSheet> requestXSLStyleSheet(CachedResourceRequest&);
#endif
#if ENABLE(LINK_PREFETCH)
    CachedResourceHandle<CachedResource> requestLinkResource(CachedResource::Type, CachedResourceRequest&);
#endif
#if ENABLE(VIDEO_TRACK)
    CachedResourceHandle<CachedTextTrack> requestTextTrack(CachedResourceRequest&);
#endif
#if ENABLE(CSS_SHADERS)
    CachedResourceHandle<CachedShader> requestShader(CachedResourceRequest&);
#endif

    // Logs an access denied message to the console for the specified URL.
    void printAccessDeniedMessage(const KURL& url) const;

    CachedResource* cachedResource(const String& url) const;
    CachedResource* cachedResource(const KURL& url) const;
    
    typedef HashMap<String, CachedResourceHandle<CachedResource> > DocumentResourceMap;
    const DocumentResourceMap& allCachedResources() const { return m_documentResources; }

    bool autoLoadImages() const { return m_autoLoadImages; }
    void setAutoLoadImages(bool);

    void setImagesEnabled(bool);

    bool shouldDeferImageLoad(const KURL&) const;
    
    CachePolicy cachePolicy(CachedResource::Type) const;
    
    Frame* frame() const; // Can be null
    Document* document() const { return m_document; } // Can be null
    void setDocument(Document* document) { m_document = document; }
    void clearDocumentLoader() { m_documentLoader = 0; }

    void removeCachedResource(CachedResource*) const;
    void loadDone(CachedResource*);
    void garbageCollectDocumentResources();
    
    void incrementRequestCount(const CachedResource*);
    void decrementRequestCount(const CachedResource*);
    int requestCount() const { return m_requestCount; }

    bool isPreloaded(const String& urlString) const;
    void clearPreloads();
    void clearPendingPreloads();
    void preload(CachedResource::Type, CachedResourceRequest&, const String& charset);
    void checkForPendingPreloads();
    void printPreloadStats();
    bool canRequest(CachedResource::Type, const KURL&, const ResourceLoaderOptions&, bool forPreload = false);

    static const ResourceLoaderOptions& defaultCachedResourceOptions();

private:
    explicit CachedResourceLoader(DocumentLoader*);

    CachedResourceHandle<CachedResource> requestResource(CachedResource::Type, CachedResourceRequest&);
    CachedResourceHandle<CachedResource> revalidateResource(const CachedResourceRequest&, CachedResource*);
    CachedResourceHandle<CachedResource> loadResource(CachedResource::Type, CachedResourceRequest&, const String& charset);
#if ENABLE(RESOURCE_TIMING)
    void storeResourceTimingInitiatorInformation(const CachedResourceHandle<CachedResource>&, const CachedResourceRequest&);
#endif
    void requestPreload(CachedResource::Type, CachedResourceRequest&, const String& charset);

    enum RevalidationPolicy { Use, Revalidate, Reload, Load };
    RevalidationPolicy determineRevalidationPolicy(CachedResource::Type, ResourceRequest&, bool forPreload, CachedResource* existingResource, CachedResourceRequest::DeferOption) const;
    
    bool shouldContinueAfterNotifyingLoadedFromMemoryCache(CachedResource*);
    bool checkInsecureContent(CachedResource::Type, const KURL&) const;

    void garbageCollectDocumentResourcesTimerFired(Timer<CachedResourceLoader>*);
    void performPostLoadActions();

    bool clientDefersImage(const KURL&) const;
    void reloadImagesIfNotDeferred();
    
    HashSet<String> m_validatedURLs;
    mutable DocumentResourceMap m_documentResources;
    Document* m_document;
    DocumentLoader* m_documentLoader;
    
    int m_requestCount;
    
    OwnPtr<ListHashSet<CachedResource*> > m_preloads;
    struct PendingPreload {
        CachedResource::Type m_type;
        CachedResourceRequest m_request;
        String m_charset;
    };
    Deque<PendingPreload> m_pendingPreloads;

    Timer<CachedResourceLoader> m_garbageCollectDocumentResourcesTimer;

#if ENABLE(RESOURCE_TIMING)
    struct InitiatorInfo {
        AtomicString name;
        double startTime;
    };
    HashMap<CachedResource*, InitiatorInfo> m_initiatorMap;
#endif

    // 29 bits left
    bool m_autoLoadImages : 1;
    bool m_imagesEnabled : 1;
    bool m_allowStaleResources : 1;
};

class ResourceCacheValidationSuppressor {
    WTF_MAKE_NONCOPYABLE(ResourceCacheValidationSuppressor);
    WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceCacheValidationSuppressor(CachedResourceLoader* loader)
        : m_loader(loader)
        , m_previousState(false)
    {
        if (m_loader) {
            m_previousState = m_loader->m_allowStaleResources;
            m_loader->m_allowStaleResources = true;
        }
    }
    ~ResourceCacheValidationSuppressor()
    {
        if (m_loader)
            m_loader->m_allowStaleResources = m_previousState;
    }
private:
    CachedResourceLoader* m_loader;
    bool m_previousState;
};

} // namespace WebCore

#endif
