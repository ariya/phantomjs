/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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

#include "CachedResource.h"
#include "CachedResourceHandle.h"
#include "CachePolicy.h"
#include "ResourceLoadPriority.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CachedCSSStyleSheet;
class CachedFont;
class CachedImage;
class CachedResourceRequest;
class CachedScript;
class CachedXSLStyleSheet;
class Document;
class Frame;
class ImageLoader;
class KURL;

// The CachedResourceLoader manages the loading of scripts/images/stylesheets for a single document.
class CachedResourceLoader {
    WTF_MAKE_NONCOPYABLE(CachedResourceLoader); WTF_MAKE_FAST_ALLOCATED;
friend class ImageLoader;

public:
    CachedResourceLoader(Document*);
    ~CachedResourceLoader();

    CachedImage* requestImage(const String& url);
    CachedCSSStyleSheet* requestCSSStyleSheet(const String& url, const String& charset, ResourceLoadPriority priority = ResourceLoadPriorityUnresolved);
    CachedCSSStyleSheet* requestUserCSSStyleSheet(const String& url, const String& charset);
    CachedScript* requestScript(const String& url, const String& charset);
    CachedFont* requestFont(const String& url);

#if ENABLE(XSLT)
    CachedXSLStyleSheet* requestXSLStyleSheet(const String& url);
#endif
#if ENABLE(LINK_PREFETCH)
    CachedResource* requestLinkResource(const String &url, ResourceLoadPriority priority = ResourceLoadPriorityUnresolved);
#endif

    // Logs an access denied message to the console for the specified URL.
    void printAccessDeniedMessage(const KURL& url) const;

    CachedResource* cachedResource(const String& url) const;
    CachedResource* cachedResource(const KURL& url) const;
    
    typedef HashMap<String, CachedResourceHandle<CachedResource> > DocumentResourceMap;
    const DocumentResourceMap& allCachedResources() const { return m_documentResources; }

    bool autoLoadImages() const { return m_autoLoadImages; }
    void setAutoLoadImages(bool);
    
    CachePolicy cachePolicy() const;
    
    Frame* frame() const; // Can be NULL
    Document* document() const { return m_document; }

    void removeCachedResource(CachedResource*) const;

    void load(CachedResource*, bool incremental = false, SecurityCheckPolicy = DoSecurityCheck, bool sendResourceLoadCallbacks = true);
    void loadFinishing() { m_loadFinishing = true; }
    void loadDone(CachedResourceRequest*);
    void cancelRequests();
    
    void setAllowStaleResources(bool allowStaleResources) { m_allowStaleResources = allowStaleResources; }

    void incrementRequestCount(const CachedResource*);
    void decrementRequestCount(const CachedResource*);
    int requestCount();
    
    void clearPreloads();
    void clearPendingPreloads();
    void preload(CachedResource::Type, const String& url, const String& charset, bool referencedFromBody);
    void checkForPendingPreloads();
    void printPreloadStats();
    
private:
    CachedResource* requestResource(CachedResource::Type, const String& url, const String& charset, ResourceLoadPriority priority = ResourceLoadPriorityUnresolved, bool isPreload = false);
    CachedResource* revalidateResource(CachedResource*, ResourceLoadPriority priority);
    CachedResource* loadResource(CachedResource::Type, const KURL&, const String& charset, ResourceLoadPriority priority);
    void requestPreload(CachedResource::Type, const String& url, const String& charset);

    enum RevalidationPolicy { Use, Revalidate, Reload, Load };
    RevalidationPolicy determineRevalidationPolicy(CachedResource::Type, bool forPreload, CachedResource* existingResource) const;
    
    void notifyLoadedFromMemoryCache(CachedResource*);
    bool canRequest(CachedResource::Type, const KURL&, bool forPreload = false);

    void loadDoneActionTimerFired(Timer<CachedResourceLoader>*);

    void performPostLoadActions();
    
    HashSet<String> m_validatedURLs;
    mutable DocumentResourceMap m_documentResources;
    Document* m_document;

    typedef HashSet<RefPtr<CachedResourceRequest> > RequestSet;
    RequestSet m_requests;
    
    int m_requestCount;
    
    OwnPtr<ListHashSet<CachedResource*> > m_preloads;
    struct PendingPreload {
        CachedResource::Type m_type;
        String m_url;
        String m_charset;
    };
    Deque<PendingPreload> m_pendingPreloads;

    Timer<CachedResourceLoader> m_loadDoneActionTimer;
    
    //29 bits left
    bool m_autoLoadImages : 1;
    bool m_loadFinishing : 1;
    bool m_allowStaleResources : 1;
};

}

#endif
