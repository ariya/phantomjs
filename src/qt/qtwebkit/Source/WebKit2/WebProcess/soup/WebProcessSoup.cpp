/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProcess.h"

#if PLATFORM(EFL)
#include "SeccompFiltersWebProcessEfl.h"
#endif

#include "WebCookieManager.h"
#include "WebProcessCreationParameters.h"
#include "WebSoupRequestManager.h"
#include <WebCore/FileSystem.h>
#include <WebCore/Language.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/PageCache.h>
#include <WebCore/ResourceHandle.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebKit {

static uint64_t getCacheDiskFreeSize(SoupCache* cache)
{
    ASSERT(cache);

    GOwnPtr<char> cacheDir;
    g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
    if (!cacheDir)
        return 0;

    return WebCore::getVolumeFreeSizeForPath(cacheDir.get());
}

static uint64_t getMemorySize()
{
    static uint64_t kDefaultMemorySize = 512;
#if !OS(WINDOWS)
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize == -1)
        return kDefaultMemorySize;

    long physPages = sysconf(_SC_PHYS_PAGES);
    if (physPages == -1)
        return kDefaultMemorySize;

    return ((pageSize / 1024) * physPages) / 1024;
#else
    // Fallback to default for other platforms.
    return kDefaultMemorySize;
#endif
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;
    unsigned pageCacheCapacity = 0;

    unsigned long urlCacheMemoryCapacity = 0;
    unsigned long urlCacheDiskCapacity = 0;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* cache = SOUP_CACHE(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    uint64_t diskFreeSize = getCacheDiskFreeSize(cache) / 1024 / 1024;

    uint64_t memSize = getMemorySize();
    calculateCacheSizes(cacheModel, memSize, diskFreeSize,
                        cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval,
                        pageCacheCapacity, urlCacheMemoryCapacity, urlCacheDiskCapacity);

    WebCore::memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    WebCore::pageCache()->setCapacity(pageCacheCapacity);

    if (urlCacheDiskCapacity > soup_cache_get_max_size(cache))
        soup_cache_set_max_size(cache, urlCacheDiskCapacity);
}

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    soup_cache_clear(SOUP_CACHE(soup_session_get_feature(session, SOUP_TYPE_CACHE)));
}

// This function is based on Epiphany code in ephy-embed-prefs.c.
static CString buildAcceptLanguages(Vector<String> languages)
{
    // Ignore "C" locale.
    size_t position = languages.find("c");
    if (position != notFound)
        languages.remove(position);

    // Fallback to "en" if the list is empty.
    if (languages.isEmpty())
        return "en";

    // Calculate deltas for the quality values.
    int delta;
    if (languages.size() < 10)
        delta = 10;
    else if (languages.size() < 20)
        delta = 5;
    else
        delta = 1;

    // Set quality values for each language.
    StringBuilder builder;
    for (size_t i = 0; i < languages.size(); ++i) {
        if (i)
            builder.append(", ");

        builder.append(languages[i]);

        int quality = 100 - i * delta;
        if (quality > 0 && quality < 100) {
            char buffer[8];
            g_ascii_formatd(buffer, 8, "%.2f", quality / 100.0);
            builder.append(String::format(";q=%s", buffer));
        }
    }

    return builder.toString().utf8();
}

static void setSoupSessionAcceptLanguage(Vector<String> languages)
{
    g_object_set(WebCore::ResourceHandle::defaultSession(), "accept-language", buildAcceptLanguages(languages).data(), NULL);
}

static void languageChanged(void*)
{
    setSoupSessionAcceptLanguage(WebCore::userPreferredLanguages());
}

void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::MessageDecoder&)
{
#if ENABLE(SECCOMP_FILTERS)
    {
#if PLATFORM(EFL)
        SeccompFiltersWebProcessEfl seccompFilters(parameters);
#endif
        seccompFilters.initialize();
    }
#endif

    ASSERT(!parameters.diskCacheDirectory.isEmpty());
    GRefPtr<SoupCache> soupCache = adoptGRef(soup_cache_new(parameters.diskCacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER));
    soup_session_add_feature(WebCore::ResourceHandle::defaultSession(), SOUP_SESSION_FEATURE(soupCache.get()));
    soup_cache_load(soupCache.get());

    if (!parameters.languages.isEmpty())
        setSoupSessionAcceptLanguage(parameters.languages);

    for (size_t i = 0; i < parameters.urlSchemesRegistered.size(); i++)
        supplement<WebSoupRequestManager>()->registerURIScheme(parameters.urlSchemesRegistered[i]);

    if (!parameters.cookiePersistentStoragePath.isEmpty()) {
        supplement<WebCookieManager>()->setCookiePersistentStorage(parameters.cookiePersistentStoragePath,
            parameters.cookiePersistentStorageType);
    }
    supplement<WebCookieManager>()->setHTTPCookieAcceptPolicy(parameters.cookieAcceptPolicy);

    setIgnoreTLSErrors(parameters.ignoreTLSErrors);

    WebCore::addLanguageChangeObserver(this, languageChanged);
}

void WebProcess::platformTerminate()
{
    WebCore::removeLanguageChangeObserver(this);
}

void WebProcess::setIgnoreTLSErrors(bool ignoreTLSErrors)
{
    WebCore::ResourceHandle::setIgnoreSSLErrors(ignoreTLSErrors);
}

} // namespace WebKit
