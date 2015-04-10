/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
#include "SchemeRegistry.h"
#include <wtf/MainThread.h>

namespace WebCore {

static URLSchemesMap& localURLSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, localSchemes, ());

    if (localSchemes.isEmpty()) {
        localSchemes.add("file");
#if PLATFORM(MAC)
        localSchemes.add("applewebdata");
#endif
#if PLATFORM(QT)
        localSchemes.add("qrc");
#endif
    }

    return localSchemes;
}

static URLSchemesMap& displayIsolatedURLSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, displayIsolatedSchemes, ());
    return displayIsolatedSchemes;
}

static URLSchemesMap& secureSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, secureSchemes, ());

    if (secureSchemes.isEmpty()) {
        secureSchemes.add("https");
        secureSchemes.add("about");
        secureSchemes.add("data");
    }

    return secureSchemes;
}

static URLSchemesMap& schemesWithUniqueOrigins()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, schemesWithUniqueOrigins, ());

    if (schemesWithUniqueOrigins.isEmpty()) {
        schemesWithUniqueOrigins.add("about");
        schemesWithUniqueOrigins.add("javascript");
        // This is a willful violation of HTML5.
        // See https://bugs.webkit.org/show_bug.cgi?id=11885
        schemesWithUniqueOrigins.add("data");
    }

    return schemesWithUniqueOrigins;
}

static URLSchemesMap& emptyDocumentSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, emptyDocumentSchemes, ());

    if (emptyDocumentSchemes.isEmpty())
        emptyDocumentSchemes.add("about");

    return emptyDocumentSchemes;
}

static HashSet<String>& schemesForbiddenFromDomainRelaxation()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, schemes, ());
    return schemes;
}

static URLSchemesMap& canDisplayOnlyIfCanRequestSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, canDisplayOnlyIfCanRequestSchemes, ());

#if ENABLE(BLOB)
    if (canDisplayOnlyIfCanRequestSchemes.isEmpty()) {
        canDisplayOnlyIfCanRequestSchemes.add("blob");
#if ENABLE(FILE_SYSTEM)
        canDisplayOnlyIfCanRequestSchemes.add("filesystem");
#endif
    }
#endif // ENABLE(BLOB)

    return canDisplayOnlyIfCanRequestSchemes;
}

static URLSchemesMap& notAllowingJavascriptURLsSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, notAllowingJavascriptURLsSchemes, ());
    return notAllowingJavascriptURLsSchemes;
}

void SchemeRegistry::registerURLSchemeAsLocal(const String& scheme)
{
    localURLSchemes().add(scheme);
}

void SchemeRegistry::removeURLSchemeRegisteredAsLocal(const String& scheme)
{
    if (scheme == "file")
        return;
#if PLATFORM(MAC)
    if (scheme == "applewebdata")
        return;
#endif
    localURLSchemes().remove(scheme);
}

const URLSchemesMap& SchemeRegistry::localSchemes()
{
    return localURLSchemes();
}

static URLSchemesMap& schemesAllowingLocalStorageAccessInPrivateBrowsing()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, schemesAllowingLocalStorageAccessInPrivateBrowsing, ());
    return schemesAllowingLocalStorageAccessInPrivateBrowsing;
}

static URLSchemesMap& schemesAllowingDatabaseAccessInPrivateBrowsing()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, schemesAllowingDatabaseAccessInPrivateBrowsing, ());
    return schemesAllowingDatabaseAccessInPrivateBrowsing;
}

static URLSchemesMap& CORSEnabledSchemes()
{
    // FIXME: http://bugs.webkit.org/show_bug.cgi?id=77160
    DEFINE_STATIC_LOCAL(URLSchemesMap, CORSEnabledSchemes, ());

    if (CORSEnabledSchemes.isEmpty()) {
        CORSEnabledSchemes.add("http");
        CORSEnabledSchemes.add("https");
    }

    return CORSEnabledSchemes;
}

static URLSchemesMap& ContentSecurityPolicyBypassingSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, schemes, ());
    return schemes;
}

bool SchemeRegistry::shouldTreatURLSchemeAsLocal(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return localURLSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsNoAccess(const String& scheme)
{
    schemesWithUniqueOrigins().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsNoAccess(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesWithUniqueOrigins().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsDisplayIsolated(const String& scheme)
{
    displayIsolatedURLSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsDisplayIsolated(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return displayIsolatedURLSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsSecure(const String& scheme)
{
    secureSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsSecure(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return secureSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsEmptyDocument(const String& scheme)
{
    emptyDocumentSchemes().add(scheme);
}

bool SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return emptyDocumentSchemes().contains(scheme);
}

void SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const String& scheme)
{
    if (scheme.isEmpty())
        return;

    if (forbidden)
        schemesForbiddenFromDomainRelaxation().add(scheme);
    else
        schemesForbiddenFromDomainRelaxation().remove(scheme);
}

bool SchemeRegistry::isDomainRelaxationForbiddenForURLScheme(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesForbiddenFromDomainRelaxation().contains(scheme);
}

bool SchemeRegistry::canDisplayOnlyIfCanRequest(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return canDisplayOnlyIfCanRequestSchemes().contains(scheme);
}

void SchemeRegistry::registerAsCanDisplayOnlyIfCanRequest(const String& scheme)
{
    canDisplayOnlyIfCanRequestSchemes().add(scheme);
}

void SchemeRegistry::registerURLSchemeAsNotAllowingJavascriptURLs(const String& scheme)
{
    notAllowingJavascriptURLsSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsNotAllowingJavascriptURLs(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return notAllowingJavascriptURLsSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(const String& scheme)
{
    schemesAllowingLocalStorageAccessInPrivateBrowsing().add(scheme);
}

bool SchemeRegistry::allowsLocalStorageAccessInPrivateBrowsing(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesAllowingLocalStorageAccessInPrivateBrowsing().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(const String& scheme)
{
    schemesAllowingDatabaseAccessInPrivateBrowsing().add(scheme);
}

bool SchemeRegistry::allowsDatabaseAccessInPrivateBrowsing(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesAllowingDatabaseAccessInPrivateBrowsing().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsCORSEnabled(const String& scheme)
{
    CORSEnabledSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return CORSEnabledSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme)
{
    ContentSecurityPolicyBypassingSchemes().add(scheme);
}

void SchemeRegistry::removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme)
{
    ContentSecurityPolicyBypassingSchemes().remove(scheme);
}

bool SchemeRegistry::schemeShouldBypassContentSecurityPolicy(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return ContentSecurityPolicyBypassingSchemes().contains(scheme);
}

bool SchemeRegistry::shouldCacheResponsesFromURLSchemeIndefinitely(const String& scheme)
{
#if PLATFORM(MAC)
    if (equalIgnoringCase(scheme, "applewebdata"))
        return true;
#endif
    return equalIgnoringCase(scheme, "data");
}

} // namespace WebCore
