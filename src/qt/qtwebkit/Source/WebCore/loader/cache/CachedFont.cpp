/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.
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
#include "CachedFont.h"

#include "CachedFontClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "FontCustomPlatformData.h"
#include "FontPlatformData.h"
#include "MemoryCache.h"
#include "ResourceBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/Vector.h>

#if ENABLE(SVG_FONTS)
#include "NodeList.h"
#include "SVGDocument.h"
#include "SVGElement.h"
#include "SVGFontElement.h"
#include "SVGGElement.h"
#include "SVGNames.h"
#endif

namespace WebCore {

CachedFont::CachedFont(const ResourceRequest& resourceRequest)
    : CachedResource(resourceRequest, FontResource)
    , m_fontData(0)
    , m_loadInitiated(false)
    , m_hasCreatedFontData(false)
{
}

CachedFont::~CachedFont()
{
    delete m_fontData;
}

void CachedFont::load(CachedResourceLoader*, const ResourceLoaderOptions& options)
{
    // Don't load the file yet.  Wait for an access before triggering the load.
    setLoading(true);
    m_options = options;
}

void CachedFont::didAddClient(CachedResourceClient* c)
{
    ASSERT(c->resourceClientType() == CachedFontClient::expectedType());
    if (!isLoading())
        static_cast<CachedFontClient*>(c)->fontLoaded(this);
}

void CachedFont::finishLoading(ResourceBuffer* data)
{
    m_data = data;
    setEncodedSize(m_data.get() ? m_data->size() : 0);
    setLoading(false);
    checkNotify();
}

void CachedFont::beginLoadIfNeeded(CachedResourceLoader* dl)
{
    if (!m_loadInitiated) {
        m_loadInitiated = true;
        CachedResource::load(dl, m_options);
    }
}

bool CachedFont::ensureCustomFontData()
{
    if (!m_fontData && !errorOccurred() && !isLoading() && m_data) {
        m_fontData = createFontCustomPlatformData(m_data.get()->sharedBuffer());
        if (m_fontData)
            m_hasCreatedFontData = true;
        else
            setStatus(DecodeError);
    }
    return m_fontData;
}

FontPlatformData CachedFont::platformDataFromCustomData(float size, bool bold, bool italic, FontOrientation orientation, FontWidthVariant widthVariant, FontRenderingMode renderingMode)
{
#if ENABLE(SVG_FONTS)
    if (m_externalSVGDocument)
        return FontPlatformData(size, bold, italic);
#endif
    ASSERT(m_fontData);
    return m_fontData->fontPlatformData(static_cast<int>(size), bold, italic, orientation, widthVariant, renderingMode);
}

#if ENABLE(SVG_FONTS)
bool CachedFont::ensureSVGFontData()
{
    if (!m_externalSVGDocument && !errorOccurred() && !isLoading() && m_data) {
        m_externalSVGDocument = SVGDocument::create(0, KURL());

        RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("application/xml");
        String svgSource = decoder->decode(m_data->data(), m_data->size());
        svgSource.append(decoder->flush());
        
        m_externalSVGDocument->setContent(svgSource);
        
        if (decoder->sawError())
            m_externalSVGDocument = 0;
    }

    return m_externalSVGDocument;
}

SVGFontElement* CachedFont::getSVGFontById(const String& fontName) const
{
    RefPtr<NodeList> list = m_externalSVGDocument->getElementsByTagNameNS(SVGNames::fontTag.namespaceURI(), SVGNames::fontTag.localName());
    if (!list)
        return 0;

    unsigned listLength = list->length();
    if (!listLength)
        return 0;

#ifndef NDEBUG
    for (unsigned i = 0; i < listLength; ++i) {
        ASSERT(list->item(i));
        ASSERT(isSVGFontElement(list->item(i)));
    }
#endif

    if (fontName.isEmpty())
        return static_cast<SVGFontElement*>(list->item(0));

    for (unsigned i = 0; i < listLength; ++i) {
        SVGFontElement* element = static_cast<SVGFontElement*>(list->item(i));
        if (element->getIdAttribute() == fontName)
            return element;
    }

    return 0;
}
#endif

void CachedFont::allClientsRemoved()
{
    if (m_fontData) {
        delete m_fontData;
        m_fontData = 0;
    }
}

void CachedFont::checkNotify()
{
    if (isLoading())
        return;
    
    CachedResourceClientWalker<CachedFontClient> w(m_clients);
    while (CachedFontClient* c = w.next())
         c->fontLoaded(this);
}

bool CachedFont::mayTryReplaceEncodedData() const
{
    // If the FontCustomPlatformData has ever been constructed then it still might be in use somewhere.
    // That platform font object might directly reference the encoded data buffer behind this CachedFont,
    // so replacing it is unsafe.

    return !m_hasCreatedFontData;
}

}
