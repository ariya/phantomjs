/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebFindClient.h"
#include "WKAPICast.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebFindClient::didFindString(WebPageProxy* page, const String& string, uint32_t matchCount)
{
    if (!m_client.didFindString)
        return;

    m_client.didFindString(toAPI(page), toAPI(string.impl()), matchCount, m_client.clientInfo);

}

void WebFindClient::didFailToFindString(WebPageProxy* page, const String& string)
{
    if (!m_client.didFailToFindString)
        return;
    
    m_client.didFailToFindString(toAPI(page), toAPI(string.impl()), m_client.clientInfo);
}

void WebFindClient::didCountStringMatches(WebPageProxy* page, const String& string, uint32_t matchCount)
{
    if (!m_client.didCountStringMatches)
        return;

    m_client.didCountStringMatches(toAPI(page), toAPI(string.impl()), matchCount, m_client.clientInfo);
}

void WebFindMatchesClient::didFindStringMatches(WebPageProxy* page, const String& string, ImmutableArray* matches, int firstIndex)
{
    if (!m_client.didFindStringMatches)
        return;

    m_client.didFindStringMatches(toAPI(page), toAPI(string.impl()), toAPI(matches), firstIndex, m_client.clientInfo);
}

void WebFindMatchesClient::didGetImageForMatchResult(WebPageProxy* page, WebImage* image, uint32_t index)
{
    if (!m_client.didGetImageForMatchResult)
        return;
    m_client.didGetImageForMatchResult(toAPI(page), toAPI(image), index, m_client.clientInfo);
}

} // namespace WebKit

