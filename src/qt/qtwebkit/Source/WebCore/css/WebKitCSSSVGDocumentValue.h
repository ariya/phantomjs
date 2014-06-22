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

#ifndef WebKitCSSSVGDocumentValue_h
#define WebKitCSSSVGDocumentValue_h

#include "CSSValue.h"
#include "CachedResourceHandle.h"

namespace WebCore {

class CachedResourceLoader;
class CachedSVGDocument;

class WebKitCSSSVGDocumentValue : public CSSValue {
public:
    static PassRefPtr<WebKitCSSSVGDocumentValue> create(const String& url) { return adoptRef(new WebKitCSSSVGDocumentValue(url)); }
    ~WebKitCSSSVGDocumentValue();

    CachedSVGDocument* cachedSVGDocument() const { return m_document.get(); }
    CachedSVGDocument* load(CachedResourceLoader*);

    String customCssText() const;
    const String& url() const { return m_url; }
    bool loadRequested() const { return m_loadRequested; }
    bool equals(const WebKitCSSSVGDocumentValue&) const;

private:
    WebKitCSSSVGDocumentValue(const String& url);

    String m_url;
    CachedResourceHandle<CachedSVGDocument> m_document;
    bool m_loadRequested;
};

} // namespace WebCore

#endif // WebKitCSSSVGDocumentValue_h
