/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "ManifestParser.h"

#include "KURL.h"
#include "TextResourceDecoder.h"
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

enum Mode { Explicit, Fallback, OnlineWhitelist, Unknown };
    
bool parseManifest(const KURL& manifestURL, const char* data, int length, Manifest& manifest)
{
    ASSERT(manifest.explicitURLs.isEmpty());
    ASSERT(manifest.onlineWhitelistedURLs.isEmpty());
    ASSERT(manifest.fallbackURLs.isEmpty());
    manifest.allowAllNetworkRequests = false;

    Mode mode = Explicit;

    RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("text/cache-manifest", "UTF-8");
    String s = decoder->decode(data, length);
    s.append(decoder->flush());
    
    // Look for the magic signature: "^\xFEFF?CACHE MANIFEST[ \t]?" (the BOM is removed by TextResourceDecoder).
    // Example: "CACHE MANIFEST #comment" is a valid signature.
    // Example: "CACHE MANIFEST;V2" is not.
    if (!s.startsWith("CACHE MANIFEST"))
        return false;
    
    const UChar* end = s.characters() + s.length();    
    const UChar* p = s.characters() + 14; // "CACHE MANIFEST" is 14 characters.

    if (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
        return false;

    // Skip to the end of the line.
    while (p < end && *p != '\r' && *p != '\n')
        p++;

    while (1) {
        // Skip whitespace
        while (p < end && (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t'))
            p++;
        
        if (p == end)
            break;
        
        const UChar* lineStart = p;
        
        // Find the end of the line
        while (p < end && *p != '\r' && *p != '\n')
            p++;
        
        // Check if we have a comment
        if (*lineStart == '#')
            continue;
        
        // Get rid of trailing whitespace
        const UChar* tmp = p - 1;
        while (tmp > lineStart && (*tmp == ' ' || *tmp == '\t'))
            tmp--;
        
        String line(lineStart, tmp - lineStart + 1);

        if (line == "CACHE:") 
            mode = Explicit;
        else if (line == "FALLBACK:")
            mode = Fallback;
        else if (line == "NETWORK:")
            mode = OnlineWhitelist;
        else if (line.endsWith(':'))
            mode = Unknown;
        else if (mode == Unknown)
            continue;
        else if (mode == Explicit || mode == OnlineWhitelist) {
            const UChar* p = line.characters();
            const UChar* lineEnd = p + line.length();
            
            // Look for whitespace separating the URL from subsequent ignored tokens.
            while (p < lineEnd && *p != '\t' && *p != ' ') 
                p++;

            if (mode == OnlineWhitelist && p - line.characters() == 1 && *line.characters() == '*') {
                // Wildcard was found.
                manifest.allowAllNetworkRequests = true;
                continue;
            }

            KURL url(manifestURL, String(line.characters(), p - line.characters()));
            
            if (!url.isValid())
                continue;

            if (url.hasFragmentIdentifier())
                url.removeFragmentIdentifier();
            
            if (!equalIgnoringCase(url.protocol(), manifestURL.protocol()))
                continue;
            
            if (mode == Explicit && manifestURL.protocolIs("https") && !protocolHostAndPortAreEqual(manifestURL, url))
                continue;
            
            if (mode == Explicit)
                manifest.explicitURLs.add(url.string());
            else
                manifest.onlineWhitelistedURLs.append(url);
            
        } else if (mode == Fallback) {
            const UChar* p = line.characters();
            const UChar* lineEnd = p + line.length();
            
            // Look for whitespace separating the two URLs
            while (p < lineEnd && *p != '\t' && *p != ' ') 
                p++;

            if (p == lineEnd) {
                // There was no whitespace separating the URLs.
                continue;
            }
            
            KURL namespaceURL(manifestURL, String(line.characters(), p - line.characters()));
            if (!namespaceURL.isValid())
                continue;
            if (namespaceURL.hasFragmentIdentifier())
                namespaceURL.removeFragmentIdentifier();

            if (!protocolHostAndPortAreEqual(manifestURL, namespaceURL))
                continue;
                                   
            // Skip whitespace separating fallback namespace from URL.
            while (p < lineEnd && (*p == '\t' || *p == ' '))
                p++;

            // Look for whitespace separating the URL from subsequent ignored tokens.
            const UChar* fallbackStart = p;
            while (p < lineEnd && *p != '\t' && *p != ' ') 
                p++;

            KURL fallbackURL(manifestURL, String(fallbackStart, p - fallbackStart));
            if (!fallbackURL.isValid())
                continue;
            if (fallbackURL.hasFragmentIdentifier())
                fallbackURL.removeFragmentIdentifier();

            if (!protocolHostAndPortAreEqual(manifestURL, fallbackURL))
                continue;

            manifest.fallbackURLs.append(make_pair(namespaceURL, fallbackURL));            
        } else 
            ASSERT_NOT_REACHED();
    }

    return true;
}

}
