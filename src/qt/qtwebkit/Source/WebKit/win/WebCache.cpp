/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#include "WebKitDLL.h"
#include "WebCache.h"

#include "CFDictionaryPropertyBag.h"
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/CrossOriginPreflightResultCache.h>

// WebCache ---------------------------------------------------------------------------

WebCache::WebCache()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebCache");
}

WebCache::~WebCache()
{
    gClassCount--;
    gClassNameCount.remove("WebCache");
}

WebCache* WebCache::createInstance()
{
    WebCache* instance = new WebCache();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCache::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<WebCache*>(this);
    else if (IsEqualGUID(riid, IID_IWebCache))
        *ppvObject = static_cast<WebCache*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebCache::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebCache::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebCache ------------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCache::statistics( 
    /* [in][out] */ int* count,
    /* [retval][out] */ IPropertyBag ** s)
{
    if (!count || (s && *count < 4))
        return E_FAIL;

    *count = 4;
    if (!s)
        return S_OK;

    WebCore::MemoryCache::Statistics stat = WebCore::memoryCache()->getStatistics();

    static CFStringRef imagesKey = CFSTR("images");
    static CFStringRef stylesheetsKey = CFSTR("style sheets");
    static CFStringRef xslKey = CFSTR("xsl");
    static CFStringRef scriptsKey = CFSTR("scripts");
#if !ENABLE(XSLT)
    const int zero = 0;
#endif

    RetainPtr<CFMutableDictionaryRef> dictionary = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    RetainPtr<CFNumberRef> value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.images.count));
    CFDictionaryAddValue(dictionary.get(), imagesKey, value.get());
    
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.cssStyleSheets.count));
    CFDictionaryAddValue(dictionary.get(), stylesheetsKey, value.get());
    
#if ENABLE(XSLT)
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.xslStyleSheets.count));
#else
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &zero));
#endif
    CFDictionaryAddValue(dictionary.get(), xslKey, value.get());

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.scripts.count));
    CFDictionaryAddValue(dictionary.get(), scriptsKey, value.get());

    COMPtr<CFDictionaryPropertyBag> propBag = CFDictionaryPropertyBag::createInstance();
    propBag->setDictionary(dictionary.get());
    s[0] = propBag.leakRef();

    dictionary = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.images.size));
    CFDictionaryAddValue(dictionary.get(), imagesKey, value.get());
    
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.cssStyleSheets.size));
    CFDictionaryAddValue(dictionary.get(), stylesheetsKey, value.get());
    
#if ENABLE(XSLT)
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.xslStyleSheets.size));
#else
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &zero));
#endif
    CFDictionaryAddValue(dictionary.get(), xslKey, value.get());

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.scripts.size));
    CFDictionaryAddValue(dictionary.get(), scriptsKey, value.get());

    propBag = CFDictionaryPropertyBag::createInstance();
    propBag->setDictionary(dictionary.get());
    s[1] = propBag.leakRef();

    dictionary = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.images.liveSize));
    CFDictionaryAddValue(dictionary.get(), imagesKey, value.get());
    
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.cssStyleSheets.liveSize));
    CFDictionaryAddValue(dictionary.get(), stylesheetsKey, value.get());
    
#if ENABLE(XSLT)
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.xslStyleSheets.liveSize));
#else
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &zero));
#endif
    CFDictionaryAddValue(dictionary.get(), xslKey, value.get());

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.scripts.liveSize));
    CFDictionaryAddValue(dictionary.get(), scriptsKey, value.get());

    propBag = CFDictionaryPropertyBag::createInstance();
    propBag->setDictionary(dictionary.get());
    s[2] = propBag.leakRef();

    dictionary = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.images.decodedSize));
    CFDictionaryAddValue(dictionary.get(), imagesKey, value.get());
    
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.cssStyleSheets.decodedSize));
    CFDictionaryAddValue(dictionary.get(), stylesheetsKey, value.get());
    
#if ENABLE(XSLT)
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.xslStyleSheets.decodedSize));
#else
    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &zero));
#endif
    CFDictionaryAddValue(dictionary.get(), xslKey, value.get());

    value = adoptCF(CFNumberCreate(0, kCFNumberIntType, &stat.scripts.decodedSize));
    CFDictionaryAddValue(dictionary.get(), scriptsKey, value.get());

    propBag = CFDictionaryPropertyBag::createInstance();
    propBag->setDictionary(dictionary.get());
    s[3] = propBag.leakRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCache::empty( void)
{
    if (WebCore::memoryCache()->disabled())
        return S_OK;
    WebCore::memoryCache()->setDisabled(true);
    WebCore::memoryCache()->setDisabled(false);

    // Empty the application cache.
    WebCore::cacheStorage().empty();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::shared().empty();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCache::setDisabled( 
    /* [in] */ BOOL disabled)
{
    WebCore::memoryCache()->setDisabled(!!disabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCache::disabled(
    /* [out][retval] */ BOOL* disabled)
{
    if (!disabled)
        return E_POINTER;
    *disabled = WebCore::memoryCache()->disabled();
    return S_OK;
}
