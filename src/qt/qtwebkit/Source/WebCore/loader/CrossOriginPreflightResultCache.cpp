/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "CrossOriginPreflightResultCache.h"

#include "CrossOriginAccessControl.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace std;

// These values are at the discretion of the user agent.
static const unsigned defaultPreflightCacheTimeoutSeconds = 5;
static const unsigned maxPreflightCacheTimeoutSeconds = 600; // Should be short enough to minimize the risk of using a poisoned cache after switching to a secure network.

static bool parseAccessControlMaxAge(const String& string, unsigned& expiryDelta)
{
    // FIXME: this will not do the correct thing for a number starting with a '+'
    bool ok = false;
    expiryDelta = string.toUIntStrict(&ok);
    return ok;
}

template<class HashType>
static void addToAccessControlAllowList(const String& string, unsigned start, unsigned end, HashSet<String, HashType>& set)
{
    StringImpl* stringImpl = string.impl();
    if (!stringImpl)
        return;

    // Skip white space from start.
    while (start <= end && isSpaceOrNewline((*stringImpl)[start]))
        ++start;

    // only white space
    if (start > end) 
        return;

    // Skip white space from end.
    while (end && isSpaceOrNewline((*stringImpl)[end]))
        --end;

    set.add(string.substring(start, end - start + 1));
}

template<class HashType>
static bool parseAccessControlAllowList(const String& string, HashSet<String, HashType>& set)
{
    unsigned start = 0;
    size_t end;
    while ((end = string.find(',', start)) != notFound) {
        if (start != end)
            addToAccessControlAllowList(string, start, end - 1, set);
        start = end + 1;
    }
    if (start != string.length())
        addToAccessControlAllowList(string, start, string.length() - 1, set);

    return true;
}

bool CrossOriginPreflightResultCacheItem::parse(const ResourceResponse& response, String& errorDescription)
{
    m_methods.clear();
    if (!parseAccessControlAllowList(response.httpHeaderField("Access-Control-Allow-Methods"), m_methods)) {
        errorDescription = "Cannot parse Access-Control-Allow-Methods response header field.";
        return false;
    }

    m_headers.clear();
    if (!parseAccessControlAllowList(response.httpHeaderField("Access-Control-Allow-Headers"), m_headers)) {
        errorDescription = "Cannot parse Access-Control-Allow-Headers response header field.";
        return false;
    }

    unsigned expiryDelta;
    if (parseAccessControlMaxAge(response.httpHeaderField("Access-Control-Max-Age"), expiryDelta)) {
        if (expiryDelta > maxPreflightCacheTimeoutSeconds)
            expiryDelta = maxPreflightCacheTimeoutSeconds;
    } else
        expiryDelta = defaultPreflightCacheTimeoutSeconds;

    m_absoluteExpiryTime = currentTime() + expiryDelta;
    return true;
}

bool CrossOriginPreflightResultCacheItem::allowsCrossOriginMethod(const String& method, String& errorDescription) const
{
    if (m_methods.contains(method) || isOnAccessControlSimpleRequestMethodWhitelist(method))
        return true;

    errorDescription = "Method " + method + " is not allowed by Access-Control-Allow-Methods.";
    return false;
}

bool CrossOriginPreflightResultCacheItem::allowsCrossOriginHeaders(const HTTPHeaderMap& requestHeaders, String& errorDescription) const
{
    HTTPHeaderMap::const_iterator end = requestHeaders.end();
    for (HTTPHeaderMap::const_iterator it = requestHeaders.begin(); it != end; ++it) {
        if (!m_headers.contains(it->key) && !isOnAccessControlSimpleRequestHeaderWhitelist(it->key, it->value)) {
            errorDescription = "Request header field " + it->key.string() + " is not allowed by Access-Control-Allow-Headers.";
            return false;
        }
    }
    return true;
}

bool CrossOriginPreflightResultCacheItem::allowsRequest(StoredCredentials includeCredentials, const String& method, const HTTPHeaderMap& requestHeaders) const
{
    String ignoredExplanation;
    if (m_absoluteExpiryTime < currentTime())
        return false;
    if (includeCredentials == AllowStoredCredentials && m_credentials == DoNotAllowStoredCredentials)
        return false;
    if (!allowsCrossOriginMethod(method, ignoredExplanation))
        return false;
    if (!allowsCrossOriginHeaders(requestHeaders, ignoredExplanation))
        return false;
    return true;
}

CrossOriginPreflightResultCache& CrossOriginPreflightResultCache::shared()
{
    DEFINE_STATIC_LOCAL(CrossOriginPreflightResultCache, cache, ());
    ASSERT(isMainThread());
    return cache;
}

void CrossOriginPreflightResultCache::appendEntry(const String& origin, const KURL& url, PassOwnPtr<CrossOriginPreflightResultCacheItem> preflightResult)
{
    ASSERT(isMainThread());
    m_preflightHashMap.set(make_pair(origin, url), preflightResult);
}

bool CrossOriginPreflightResultCache::canSkipPreflight(const String& origin, const KURL& url, StoredCredentials includeCredentials, const String& method, const HTTPHeaderMap& requestHeaders)
{
    ASSERT(isMainThread());
    CrossOriginPreflightResultHashMap::iterator cacheIt = m_preflightHashMap.find(make_pair(origin, url));
    if (cacheIt == m_preflightHashMap.end())
        return false;

    if (cacheIt->value->allowsRequest(includeCredentials, method, requestHeaders))
        return true;

    m_preflightHashMap.remove(cacheIt);
    return false;
}

void CrossOriginPreflightResultCache::empty()
{
    ASSERT(isMainThread());
    m_preflightHashMap.clear();
}

} // namespace WebCore
