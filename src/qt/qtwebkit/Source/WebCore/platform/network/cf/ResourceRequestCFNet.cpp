/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "ResourceRequestCFNet.h"

#include "ResourceRequest.h"

#if ENABLE(PUBLIC_SUFFIX_LIST)
#include "PublicSuffix.h"
#endif

#if USE(CFNETWORK)
#include "FormDataStreamCFNet.h"
#include <CFNetwork/CFURLRequestPriv.h>
#include <wtf/text/CString.h>
#endif

#if PLATFORM(MAC)
#include "ResourceLoadPriority.h"
#include "WebCoreSystemInterface.h"
#include <dlfcn.h>
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

bool ResourceRequest::s_httpPipeliningEnabled = false;

#if USE(CFNETWORK)

typedef void (*CFURLRequestSetContentDispositionEncodingFallbackArrayFunction)(CFMutableURLRequestRef, CFArrayRef);
typedef CFArrayRef (*CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction)(CFURLRequestRef);

#if PLATFORM(WIN)
static HMODULE findCFNetworkModule()
{
#ifndef DEBUG_ALL
    return GetModuleHandleA("CFNetwork");
#else
    return GetModuleHandleA("CFNetwork_debug");
#endif
}

static CFURLRequestSetContentDispositionEncodingFallbackArrayFunction findCFURLRequestSetContentDispositionEncodingFallbackArrayFunction()
{
    return reinterpret_cast<CFURLRequestSetContentDispositionEncodingFallbackArrayFunction>(GetProcAddress(findCFNetworkModule(), "_CFURLRequestSetContentDispositionEncodingFallbackArray"));
}

static CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction findCFURLRequestCopyContentDispositionEncodingFallbackArrayFunction()
{
    return reinterpret_cast<CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction>(GetProcAddress(findCFNetworkModule(), "_CFURLRequestCopyContentDispositionEncodingFallbackArray"));
}
#elif PLATFORM(MAC)
static CFURLRequestSetContentDispositionEncodingFallbackArrayFunction findCFURLRequestSetContentDispositionEncodingFallbackArrayFunction()
{
    return reinterpret_cast<CFURLRequestSetContentDispositionEncodingFallbackArrayFunction>(dlsym(RTLD_DEFAULT, "_CFURLRequestSetContentDispositionEncodingFallbackArray"));
}

static CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction findCFURLRequestCopyContentDispositionEncodingFallbackArrayFunction()
{
    return reinterpret_cast<CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction>(dlsym(RTLD_DEFAULT, "_CFURLRequestCopyContentDispositionEncodingFallbackArray"));
}
#endif

static void setContentDispositionEncodingFallbackArray(CFMutableURLRequestRef request, CFArrayRef fallbackArray)
{
    static CFURLRequestSetContentDispositionEncodingFallbackArrayFunction function = findCFURLRequestSetContentDispositionEncodingFallbackArrayFunction();
    if (function)
        function(request, fallbackArray);
}

static CFArrayRef copyContentDispositionEncodingFallbackArray(CFURLRequestRef request)
{
    static CFURLRequestCopyContentDispositionEncodingFallbackArrayFunction function = findCFURLRequestCopyContentDispositionEncodingFallbackArrayFunction();
    if (!function)
        return 0;
    return function(request);
}

CFURLRequestRef ResourceRequest::cfURLRequest(HTTPBodyUpdatePolicy bodyPolicy) const
{
    updatePlatformRequest(bodyPolicy);

    return m_cfRequest.get();
}

static inline void setHeaderFields(CFMutableURLRequestRef request, const HTTPHeaderMap& requestHeaders) 
{
    // Remove existing headers first, as some of them may no longer be present in the map.
    RetainPtr<CFDictionaryRef> oldHeaderFields = adoptCF(CFURLRequestCopyAllHTTPHeaderFields(request));
    CFIndex oldHeaderFieldCount = CFDictionaryGetCount(oldHeaderFields.get());
    if (oldHeaderFieldCount) {
        Vector<CFStringRef> oldHeaderFieldNames(oldHeaderFieldCount);
        CFDictionaryGetKeysAndValues(oldHeaderFields.get(), reinterpret_cast<const void**>(&oldHeaderFieldNames[0]), 0);
        for (CFIndex i = 0; i < oldHeaderFieldCount; ++i)
            CFURLRequestSetHTTPHeaderFieldValue(request, oldHeaderFieldNames[i], 0);
    }

    for (HTTPHeaderMap::const_iterator it = requestHeaders.begin(), end = requestHeaders.end(); it != end; ++it)
        CFURLRequestSetHTTPHeaderFieldValue(request, it->key.string().createCFString().get(), it->value.createCFString().get());
}

void ResourceRequest::doUpdatePlatformRequest()
{
    CFMutableURLRequestRef cfRequest;

    RetainPtr<CFURLRef> url = ResourceRequest::url().createCFURL();
    RetainPtr<CFURLRef> firstPartyForCookies = ResourceRequest::firstPartyForCookies().createCFURL();
    if (m_cfRequest) {
        cfRequest = CFURLRequestCreateMutableCopy(0, m_cfRequest.get());
        CFURLRequestSetURL(cfRequest, url.get());
        CFURLRequestSetMainDocumentURL(cfRequest, firstPartyForCookies.get());
        CFURLRequestSetCachePolicy(cfRequest, (CFURLRequestCachePolicy)cachePolicy());
        CFURLRequestSetTimeoutInterval(cfRequest, timeoutInterval());
    } else
        cfRequest = CFURLRequestCreateMutable(0, url.get(), (CFURLRequestCachePolicy)cachePolicy(), timeoutInterval(), firstPartyForCookies.get());

    CFURLRequestSetHTTPRequestMethod(cfRequest, httpMethod().createCFString().get());

    if (httpPipeliningEnabled())
        wkSetHTTPPipeliningPriority(cfRequest, toHTTPPipeliningPriority(m_priority));
#if !PLATFORM(WIN)
    wkCFURLRequestAllowAllPostCaching(cfRequest);
#endif

    setHeaderFields(cfRequest, httpHeaderFields());

    CFURLRequestSetShouldHandleHTTPCookies(cfRequest, allowCookies());

    unsigned fallbackCount = m_responseContentDispositionEncodingFallbackArray.size();
    RetainPtr<CFMutableArrayRef> encodingFallbacks = adoptCF(CFArrayCreateMutable(kCFAllocatorDefault, fallbackCount, 0));
    for (unsigned i = 0; i != fallbackCount; ++i) {
        RetainPtr<CFStringRef> encodingName = m_responseContentDispositionEncodingFallbackArray[i].createCFString();
        CFStringEncoding encoding = CFStringConvertIANACharSetNameToEncoding(encodingName.get());
        if (encoding != kCFStringEncodingInvalidId)
            CFArrayAppendValue(encodingFallbacks.get(), reinterpret_cast<const void*>(encoding));
    }
    setContentDispositionEncodingFallbackArray(cfRequest, encodingFallbacks.get());

    if (m_cfRequest) {
        RetainPtr<CFHTTPCookieStorageRef> cookieStorage = adoptCF(CFURLRequestCopyHTTPCookieStorage(m_cfRequest.get()));
        if (cookieStorage)
            CFURLRequestSetHTTPCookieStorage(cfRequest, cookieStorage.get());
        CFURLRequestSetHTTPCookieStorageAcceptPolicy(cfRequest, CFURLRequestGetHTTPCookieStorageAcceptPolicy(m_cfRequest.get()));
        CFURLRequestSetSSLProperties(cfRequest, CFURLRequestGetSSLProperties(m_cfRequest.get()));
    }

#if ENABLE(CACHE_PARTITIONING)
    String partition = cachePartition();
    if (!partition.isNull() && !partition.isEmpty()) {
        CString utf8String = partition.utf8();
        RetainPtr<CFStringRef> partitionValue = adoptCF(CFStringCreateWithBytes(0, reinterpret_cast<const UInt8*>(utf8String.data()), utf8String.length(), kCFStringEncodingUTF8, false));
        _CFURLRequestSetProtocolProperty(cfRequest, wkCachePartitionKey(), partitionValue.get());
    }
#endif

    m_cfRequest = adoptCF(cfRequest);
#if PLATFORM(MAC)
    updateNSURLRequest();
#endif
}

void ResourceRequest::doUpdatePlatformHTTPBody()
{
    CFMutableURLRequestRef cfRequest;

    RetainPtr<CFURLRef> url = ResourceRequest::url().createCFURL();
    RetainPtr<CFURLRef> firstPartyForCookies = ResourceRequest::firstPartyForCookies().createCFURL();
    if (m_cfRequest) {
        cfRequest = CFURLRequestCreateMutableCopy(0, m_cfRequest.get());
        CFURLRequestSetURL(cfRequest, url.get());
        CFURLRequestSetMainDocumentURL(cfRequest, firstPartyForCookies.get());
        CFURLRequestSetCachePolicy(cfRequest, (CFURLRequestCachePolicy)cachePolicy());
        CFURLRequestSetTimeoutInterval(cfRequest, timeoutInterval());
    } else
        cfRequest = CFURLRequestCreateMutable(0, url.get(), (CFURLRequestCachePolicy)cachePolicy(), timeoutInterval(), firstPartyForCookies.get());

    RefPtr<FormData> formData = httpBody();
    if (formData && !formData->isEmpty())
        WebCore::setHTTPBody(cfRequest, formData);

    if (RetainPtr<CFReadStreamRef> bodyStream = adoptCF(CFURLRequestCopyHTTPRequestBodyStream(cfRequest))) {
        // For streams, provide a Content-Length to avoid using chunked encoding, and to get accurate total length in callbacks.
        RetainPtr<CFStringRef> lengthString = adoptCF(static_cast<CFStringRef>(CFReadStreamCopyProperty(bodyStream.get(), formDataStreamLengthPropertyName())));
        if (lengthString) {
            CFURLRequestSetHTTPHeaderFieldValue(cfRequest, CFSTR("Content-Length"), lengthString.get());
            // Since resource request is already marked updated, we need to keep it up to date too.
            ASSERT(m_resourceRequestUpdated);
            m_httpHeaderFields.set("Content-Length", lengthString.get());
        }
    }

    m_cfRequest = adoptCF(cfRequest);
#if PLATFORM(MAC)
    updateNSURLRequest();
#endif
}

void ResourceRequest::doUpdateResourceRequest()
{
    if (!m_cfRequest) {
        *this = ResourceRequest();
        return;
    }

    m_url = CFURLRequestGetURL(m_cfRequest.get());

    m_cachePolicy = (ResourceRequestCachePolicy)CFURLRequestGetCachePolicy(m_cfRequest.get());
    m_timeoutInterval = CFURLRequestGetTimeoutInterval(m_cfRequest.get());
    m_firstPartyForCookies = CFURLRequestGetMainDocumentURL(m_cfRequest.get());
    if (CFStringRef method = CFURLRequestCopyHTTPRequestMethod(m_cfRequest.get())) {
        m_httpMethod = method;
        CFRelease(method);
    }
    m_allowCookies = CFURLRequestShouldHandleHTTPCookies(m_cfRequest.get());

    if (httpPipeliningEnabled())
        m_priority = toResourceLoadPriority(wkGetHTTPPipeliningPriority(m_cfRequest.get()));

    m_httpHeaderFields.clear();
    if (CFDictionaryRef headers = CFURLRequestCopyAllHTTPHeaderFields(m_cfRequest.get())) {
        CFIndex headerCount = CFDictionaryGetCount(headers);
        Vector<const void*, 128> keys(headerCount);
        Vector<const void*, 128> values(headerCount);
        CFDictionaryGetKeysAndValues(headers, keys.data(), values.data());
        for (int i = 0; i < headerCount; ++i)
            m_httpHeaderFields.set((CFStringRef)keys[i], (CFStringRef)values[i]);
        CFRelease(headers);
    }

    m_responseContentDispositionEncodingFallbackArray.clear();
    RetainPtr<CFArrayRef> encodingFallbacks = adoptCF(copyContentDispositionEncodingFallbackArray(m_cfRequest.get()));
    if (encodingFallbacks) {
        CFIndex count = CFArrayGetCount(encodingFallbacks.get());
        for (CFIndex i = 0; i < count; ++i) {
            CFStringEncoding encoding = reinterpret_cast<CFIndex>(CFArrayGetValueAtIndex(encodingFallbacks.get(), i));
            if (encoding != kCFStringEncodingInvalidId)
                m_responseContentDispositionEncodingFallbackArray.append(CFStringConvertEncodingToIANACharSetName(encoding));
        }
    }

#if ENABLE(CACHE_PARTITIONING)
    RetainPtr<CFStringRef> cachePartition = adoptCF(static_cast<CFStringRef>(_CFURLRequestCopyProtocolPropertyForKey(m_cfRequest.get(), wkCachePartitionKey())));
    if (cachePartition)
        m_cachePartition = cachePartition.get();
#endif
}

void ResourceRequest::doUpdateResourceHTTPBody()
{
    if (!m_cfRequest) {
        m_httpBody = 0;
        return;
    }

    if (RetainPtr<CFDataRef> bodyData = adoptCF(CFURLRequestCopyHTTPRequestBody(m_cfRequest.get())))
        m_httpBody = FormData::create(CFDataGetBytePtr(bodyData.get()), CFDataGetLength(bodyData.get()));
    else if (RetainPtr<CFReadStreamRef> bodyStream = adoptCF(CFURLRequestCopyHTTPRequestBodyStream(m_cfRequest.get()))) {
        FormData* formData = httpBodyFromStream(bodyStream.get());
        // There is no FormData object if a client provided a custom data stream.
        // We shouldn't be looking at http body after client callbacks.
        ASSERT(formData);
        if (formData)
            m_httpBody = formData;
    }
}


void ResourceRequest::setStorageSession(CFURLStorageSessionRef storageSession)
{
    updatePlatformRequest();

    CFMutableURLRequestRef cfRequest = CFURLRequestCreateMutableCopy(0, m_cfRequest.get());
    wkSetRequestStorageSession(storageSession, cfRequest);
    m_cfRequest = adoptCF(cfRequest);
#if PLATFORM(MAC)
    updateNSURLRequest();
#endif
}

#if PLATFORM(MAC)
void ResourceRequest::applyWebArchiveHackForMail()
{
    // Hack because Mail checks for this property to detect data / archive loads
    _CFURLRequestSetProtocolProperty(cfURLRequest(DoNotUpdateHTTPBody), CFSTR("WebDataRequest"), CFSTR(""));
}
#endif

#endif // USE(CFNETWORK)

bool ResourceRequest::httpPipeliningEnabled()
{
    return s_httpPipeliningEnabled;
}

void ResourceRequest::setHTTPPipeliningEnabled(bool flag)
{
    s_httpPipeliningEnabled = flag;
}

#if ENABLE(CACHE_PARTITIONING)
String ResourceRequest::partitionName(const String& domain)
{
    if (domain.isNull())
        return emptyString();
#if ENABLE(PUBLIC_SUFFIX_LIST)
    String highLevel = topPrivatelyControlledDomain(domain);
    if (highLevel.isNull())
        return emptyString();
    return highLevel;
#else
    return domain;
#endif
}
#endif

PassOwnPtr<CrossThreadResourceRequestData> ResourceRequest::doPlatformCopyData(PassOwnPtr<CrossThreadResourceRequestData> data) const
{
#if ENABLE(CACHE_PARTITIONING)
    data->m_cachePartition = m_cachePartition;
#endif
    return data;
}

void ResourceRequest::doPlatformAdopt(PassOwnPtr<CrossThreadResourceRequestData> data)
{
#if ENABLE(CACHE_PARTITIONING)
    m_cachePartition = data->m_cachePartition;
#else
    UNUSED_PARAM(data);
#endif
}

unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    static const unsigned preferredConnectionCount = 6;

    // Always set the connection count per host, even when pipelining.
    unsigned maximumHTTPConnectionCountPerHost = wkInitializeMaximumHTTPConnectionCountPerHost(preferredConnectionCount);

    static const unsigned unlimitedConnectionCount = 10000;

    Boolean keyExistsAndHasValidFormat = false;
    Boolean prefValue = CFPreferencesGetAppBooleanValue(CFSTR("WebKitEnableHTTPPipelining"), kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);
    if (keyExistsAndHasValidFormat)
        ResourceRequest::setHTTPPipeliningEnabled(prefValue);

    if (ResourceRequest::httpPipeliningEnabled()) {
        wkSetHTTPPipeliningMaximumPriority(toHTTPPipeliningPriority(ResourceLoadPriorityHighest));
#if !PLATFORM(WIN)
        // FIXME: <rdar://problem/9375609> Implement minimum fast lane priority setting on Windows
        wkSetHTTPPipeliningMinimumFastLanePriority(toHTTPPipeliningPriority(ResourceLoadPriorityMedium));
#endif
        // When pipelining do not rate-limit requests sent from WebCore since CFNetwork handles that.
        return unlimitedConnectionCount;
    }

    return maximumHTTPConnectionCountPerHost;
}

} // namespace WebCore
