/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Google, Inc. ("Google") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SecurityPolicy.h"

#include "KURL.h"
#include <wtf/MainThread.h>
#include "OriginAccessEntry.h"
#include "SecurityOrigin.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

static SecurityPolicy::LocalLoadPolicy localLoadPolicy = SecurityPolicy::AllowLocalLoadsForLocalOnly;

typedef Vector<OriginAccessEntry> OriginAccessWhiteList;
typedef HashMap<String, OwnPtr<OriginAccessWhiteList> > OriginAccessMap;

static OriginAccessMap& originAccessMap()
{
    DEFINE_STATIC_LOCAL(OriginAccessMap, originAccessMap, ());
    return originAccessMap;
}

bool SecurityPolicy::shouldHideReferrer(const KURL& url, const String& referrer)
{
    bool referrerIsSecureURL = protocolIs(referrer, "https");
    bool referrerIsWebURL = referrerIsSecureURL || protocolIs(referrer, "http");

    if (!referrerIsWebURL)
        return true;

    if (!referrerIsSecureURL)
        return false;

    bool URLIsSecureURL = url.protocolIs("https");

    return !URLIsSecureURL;
}

String SecurityPolicy::generateReferrerHeader(ReferrerPolicy referrerPolicy, const KURL& url, const String& referrer)
{
    if (referrer.isEmpty())
        return String();

    switch (referrerPolicy) {
    case ReferrerPolicyNever:
        return String();
    case ReferrerPolicyAlways:
        return referrer;
    case ReferrerPolicyOrigin: {
        String origin = SecurityOrigin::createFromString(referrer)->toString();
        if (origin == "null")
            return String();
        // A security origin is not a canonical URL as it lacks a path. Add /
        // to turn it into a canonical URL we can use as referrer.
        return origin + "/";
    }
    case ReferrerPolicyDefault:
        break;
    }

    return shouldHideReferrer(url, referrer) ? String() : referrer;
}

void SecurityPolicy::setLocalLoadPolicy(LocalLoadPolicy policy)
{
    localLoadPolicy = policy;
}

bool SecurityPolicy::restrictAccessToLocal()
{
    return localLoadPolicy != SecurityPolicy::AllowLocalLoadsForAll;
}

bool SecurityPolicy::allowSubstituteDataAccessToLocal()
{
    return localLoadPolicy != SecurityPolicy::AllowLocalLoadsForLocalOnly;
}

bool SecurityPolicy::isAccessWhiteListed(const SecurityOrigin* activeOrigin, const SecurityOrigin* targetOrigin)
{
    if (OriginAccessWhiteList* list = originAccessMap().get(activeOrigin->toString())) {
        for (size_t i = 0; i < list->size();  ++i) {
           if (list->at(i).matchesOrigin(*targetOrigin))
               return true;
       }
    }
    return false;
}

bool SecurityPolicy::isAccessToURLWhiteListed(const SecurityOrigin* activeOrigin, const KURL& url)
{
    RefPtr<SecurityOrigin> targetOrigin = SecurityOrigin::create(url);
    return isAccessWhiteListed(activeOrigin, targetOrigin.get());
}

void SecurityPolicy::addOriginAccessWhitelistEntry(const SecurityOrigin& sourceOrigin, const String& destinationProtocol, const String& destinationDomain, bool allowDestinationSubdomains)
{
    ASSERT(isMainThread());
    ASSERT(!sourceOrigin.isUnique());
    if (sourceOrigin.isUnique())
        return;

    String sourceString = sourceOrigin.toString();
    OriginAccessMap::AddResult result = originAccessMap().add(sourceString, nullptr);
    if (result.isNewEntry)
        result.iterator->value = adoptPtr(new OriginAccessWhiteList);

    OriginAccessWhiteList* list = result.iterator->value.get();
    list->append(OriginAccessEntry(destinationProtocol, destinationDomain, allowDestinationSubdomains ? OriginAccessEntry::AllowSubdomains : OriginAccessEntry::DisallowSubdomains));
}

void SecurityPolicy::removeOriginAccessWhitelistEntry(const SecurityOrigin& sourceOrigin, const String& destinationProtocol, const String& destinationDomain, bool allowDestinationSubdomains)
{
    ASSERT(isMainThread());
    ASSERT(!sourceOrigin.isUnique());
    if (sourceOrigin.isUnique())
        return;

    String sourceString = sourceOrigin.toString();
    OriginAccessMap& map = originAccessMap();
    OriginAccessMap::iterator it = map.find(sourceString);
    if (it == map.end())
        return;

    OriginAccessWhiteList* list = it->value.get();
    size_t index = list->find(OriginAccessEntry(destinationProtocol, destinationDomain, allowDestinationSubdomains ? OriginAccessEntry::AllowSubdomains : OriginAccessEntry::DisallowSubdomains));
    if (index == notFound)
        return;

    list->remove(index);

    if (list->isEmpty())
        map.remove(it);
}

void SecurityPolicy::resetOriginAccessWhitelists()
{
    ASSERT(isMainThread());
    originAccessMap().clear();
}

} // namespace WebCore
