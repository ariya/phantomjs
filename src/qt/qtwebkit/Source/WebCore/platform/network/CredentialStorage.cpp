/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CredentialStorage.h"

#include "Credential.h"
#include "KURL.h"
#include "ProtectionSpaceHash.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

typedef HashMap<ProtectionSpace, Credential> ProtectionSpaceToCredentialMap;
static ProtectionSpaceToCredentialMap& protectionSpaceToCredentialMap()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(ProtectionSpaceToCredentialMap, map, ());
    return map;
}

static HashSet<String>& originsWithCredentials()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(HashSet<String>, set, ());
    return set;
}

typedef HashMap<String, ProtectionSpace> PathToDefaultProtectionSpaceMap;
static PathToDefaultProtectionSpaceMap& pathToDefaultProtectionSpaceMap()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(PathToDefaultProtectionSpaceMap, map, ());
    return map;
}

static String originStringFromURL(const KURL& url)
{
    if (url.port())
        return url.protocol() + "://" + url.host() + ':' + String::number(url.port()) + '/';

    return url.protocol() + "://" + url.host() + '/';
}

static String protectionSpaceMapKeyFromURL(const KURL& url)
{
    ASSERT(url.isValid());

    // Remove the last path component that is not a directory to determine the subtree for which credentials will apply.
    // We keep a leading slash, but remove a trailing one.
    String directoryURL = url.string().substring(0, url.pathEnd());
    unsigned directoryURLPathStart = url.pathStart();
    ASSERT(directoryURL[directoryURLPathStart] == '/');
    if (directoryURL.length() > directoryURLPathStart + 1) {
        size_t index = directoryURL.reverseFind('/');
        ASSERT(index != notFound);
        directoryURL = directoryURL.substring(0, (index != directoryURLPathStart) ? index : directoryURLPathStart + 1);
    }

    return directoryURL;
}

void CredentialStorage::set(const Credential& credential, const ProtectionSpace& protectionSpace, const KURL& url)
{
    ASSERT(protectionSpace.isProxy() || url.protocolIsInHTTPFamily());
    ASSERT(protectionSpace.isProxy() || url.isValid());

    protectionSpaceToCredentialMap().set(protectionSpace, credential);
    if (!protectionSpace.isProxy()) {
        originsWithCredentials().add(originStringFromURL(url));

        ProtectionSpaceAuthenticationScheme scheme = protectionSpace.authenticationScheme();
        if (scheme == ProtectionSpaceAuthenticationSchemeHTTPBasic || scheme == ProtectionSpaceAuthenticationSchemeDefault) {
            // The map can contain both a path and its subpath - while redundant, this makes lookups faster.
            pathToDefaultProtectionSpaceMap().set(protectionSpaceMapKeyFromURL(url), protectionSpace);
        }
    }
}

Credential CredentialStorage::get(const ProtectionSpace& protectionSpace)
{
    return protectionSpaceToCredentialMap().get(protectionSpace);
}

void CredentialStorage::remove(const ProtectionSpace& protectionSpace)
{
    protectionSpaceToCredentialMap().remove(protectionSpace);
}

static PathToDefaultProtectionSpaceMap::iterator findDefaultProtectionSpaceForURL(const KURL& url)
{
    ASSERT(url.protocolIsInHTTPFamily());
    ASSERT(url.isValid());

    PathToDefaultProtectionSpaceMap& map = pathToDefaultProtectionSpaceMap();

    // Don't spend time iterating the path for origins that don't have any credentials.
    if (!originsWithCredentials().contains(originStringFromURL(url)))
        return map.end();

    String directoryURL = protectionSpaceMapKeyFromURL(url);
    unsigned directoryURLPathStart = url.pathStart();
    while (true) {
        PathToDefaultProtectionSpaceMap::iterator iter = map.find(directoryURL);
        if (iter != map.end())
            return iter;

        if (directoryURL.length() == directoryURLPathStart + 1)  // path is "/" already, cannot shorten it any more
            return map.end();

        size_t index = directoryURL.reverseFind('/', directoryURL.length() - 2);
        ASSERT(index != notFound);
        directoryURL = directoryURL.substring(0, (index == directoryURLPathStart) ? index + 1 : index);
        ASSERT(directoryURL.length() > directoryURLPathStart);
        ASSERT(directoryURL.length() == directoryURLPathStart + 1 || directoryURL[directoryURL.length() - 1] != '/');
    }
}

bool CredentialStorage::set(const Credential& credential, const KURL& url)
{
    ASSERT(url.protocolIsInHTTPFamily());
    ASSERT(url.isValid());
    PathToDefaultProtectionSpaceMap::iterator iter = findDefaultProtectionSpaceForURL(url);
    if (iter == pathToDefaultProtectionSpaceMap().end())
        return false;
    ASSERT(originsWithCredentials().contains(originStringFromURL(url)));
    protectionSpaceToCredentialMap().set(iter->value, credential);
    return true;
}

Credential CredentialStorage::get(const KURL& url)
{
    PathToDefaultProtectionSpaceMap::iterator iter = findDefaultProtectionSpaceForURL(url);
    if (iter == pathToDefaultProtectionSpaceMap().end())
        return Credential();
    return protectionSpaceToCredentialMap().get(iter->value);
}

void CredentialStorage::setPrivateMode(bool mode)
{
    if (!mode)
        protectionSpaceToCredentialMap().clear();
}

} // namespace WebCore
