/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SVG)
#include "WebKitCSSSVGDocumentValue.h"

#include "CSSParser.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedResourceRequestInitiators.h"
#include "CachedSVGDocument.h"
#include "Document.h"

namespace WebCore {

WebKitCSSSVGDocumentValue::WebKitCSSSVGDocumentValue(const String& url)
    : CSSValue(WebKitCSSSVGDocumentClass)
    , m_url(url)
    , m_loadRequested(false)
{
}

WebKitCSSSVGDocumentValue::~WebKitCSSSVGDocumentValue()
{
}

CachedSVGDocument* WebKitCSSSVGDocumentValue::load(CachedResourceLoader* loader)
{
    ASSERT(loader);

    if (!m_loadRequested) {
        m_loadRequested = true;

        CachedResourceRequest request(ResourceRequest(loader->document()->completeURL(m_url)));
        request.setInitiator(cachedResourceRequestInitiators().css);
        m_document = loader->requestSVGDocument(request);
    }

    return m_document.get();
}

String WebKitCSSSVGDocumentValue::customCssText() const
{
    return quoteCSSStringIfNeeded(m_url);
}

bool WebKitCSSSVGDocumentValue::equals(const WebKitCSSSVGDocumentValue& other) const
{
    return m_url == other.m_url;
}

} // namespace WebCore

#endif // ENABLE(SVG)
