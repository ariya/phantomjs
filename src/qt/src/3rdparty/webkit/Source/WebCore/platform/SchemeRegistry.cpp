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

    // This is a willful violation of HTML5.
    // See https://bugs.webkit.org/show_bug.cgi?id=11885
    if (schemesWithUniqueOrigins.isEmpty())
        schemesWithUniqueOrigins.add("data");

    return schemesWithUniqueOrigins;
}

static URLSchemesMap& emptyDocumentSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, emptyDocumentSchemes, ());

    if (emptyDocumentSchemes.isEmpty())
        emptyDocumentSchemes.add("about");

    return emptyDocumentSchemes;
}

static URLSchemesMap& canDisplayOnlyIfCanRequestSchemes()
{
    DEFINE_STATIC_LOCAL(URLSchemesMap, canDisplayOnlyIfCanRequestSchemes, ());

#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
    if (canDisplayOnlyIfCanRequestSchemes.isEmpty()) {
#if ENABLE(BLOB)
        canDisplayOnlyIfCanRequestSchemes.add("blob");
#endif
#if ENABLE(FILE_SYSTEM)
        canDisplayOnlyIfCanRequestSchemes.add("filesystem");
#endif
    }
#endif // ENABLE(BLOB) || ENABLE(FILE_SYSTEM)

    return canDisplayOnlyIfCanRequestSchemes;
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

} // namespace WebCore
