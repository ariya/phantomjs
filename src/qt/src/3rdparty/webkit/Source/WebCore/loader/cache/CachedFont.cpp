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

// FIXME: This should really be a blacklist instead of a whitelist
#if USE(CG) || PLATFORM(QT) || PLATFORM(GTK) || (PLATFORM(CHROMIUM) && (!OS(DARWIN) || USE(SKIA_MAC_ON_CHROME))) || PLATFORM(HAIKU) || OS(WINCE) || PLATFORM(ANDROID) || PLATFORM(BREWMP)
#define STORE_FONT_CUSTOM_PLATFORM_DATA
#endif

#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "FontPlatformData.h"
#include "MemoryCache.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/Vector.h>

#ifdef STORE_FONT_CUSTOM_PLATFORM_DATA
#include "FontCustomPlatformData.h"
#endif

#if ENABLE(SVG_FONTS)
#include "NodeList.h"
#include "SVGElement.h"
#include "SVGFontElement.h"
#include "SVGGElement.h"
#include "SVGNames.h"
#endif

namespace WebCore {

CachedFont::CachedFont(const String &url)
    : CachedResource(url, FontResource)
    , m_fontData(0)
    , m_loadInitiated(false)
{
}

CachedFont::~CachedFont()
{
#ifdef STORE_FONT_CUSTOM_PLATFORM_DATA
    delete m_fontData;
#endif
}

void CachedFont::load(CachedResourceLoader*)
{
    // Don't load the file yet.  Wait for an access before triggering the load.
    setLoading(true);
}

void CachedFont::didAddClient(CachedResourceClient* c)
{
    if (!isLoading())
        c->fontLoaded(this);
}

void CachedFont::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    if (!allDataReceived)
        return;

    m_data = data;     
    setEncodedSize(m_data.get() ? m_data->size() : 0);
    setLoading(false);
    checkNotify();
}

void CachedFont::beginLoadIfNeeded(CachedResourceLoader* dl)
{
    if (!m_loadInitiated) {
        m_loadInitiated = true;
        dl->load(this, false);
    }
}

bool CachedFont::ensureCustomFontData()
{
#ifdef STORE_FONT_CUSTOM_PLATFORM_DATA
    if (!m_fontData && !errorOccurred() && !isLoading() && m_data) {
        m_fontData = createFontCustomPlatformData(m_data.get());
        if (!m_fontData)
            setStatus(DecodeError);
    }
#endif
    return m_fontData;
}

FontPlatformData CachedFont::platformDataFromCustomData(float size, bool bold, bool italic, FontOrientation orientation, TextOrientation textOrientation, FontWidthVariant widthVariant, FontRenderingMode renderingMode)
{
#if ENABLE(SVG_FONTS)
    if (m_externalSVGDocument)
        return FontPlatformData(size, bold, italic);
#endif
#ifdef STORE_FONT_CUSTOM_PLATFORM_DATA
    ASSERT(m_fontData);
    return m_fontData->fontPlatformData(static_cast<int>(size), bold, italic, orientation, textOrientation, widthVariant, renderingMode);
#else
    return FontPlatformData();
#endif
}

#if ENABLE(SVG_FONTS)
bool CachedFont::ensureSVGFontData()
{
    if (!m_externalSVGDocument && !errorOccurred() && !isLoading() && m_data) {
        m_externalSVGDocument = SVGDocument::create(0, KURL());

        RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("application/xml");
        String svgSource = decoder->decode(m_data->data(), m_data->size());
        svgSource += decoder->flush();
        
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
        ASSERT(list->item(i)->hasTagName(SVGNames::fontTag));
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
#ifdef STORE_FONT_CUSTOM_PLATFORM_DATA
    if (m_fontData) {
        delete m_fontData;
        m_fontData = 0;
    }
#endif
}

void CachedFont::checkNotify()
{
    if (isLoading())
        return;
    
    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient *c = w.next())
         c->fontLoaded(this);
}


void CachedFont::error(CachedResource::Status status)
{
    setStatus(status);
    ASSERT(errorOccurred());
    setLoading(false);
    checkNotify();
}

}
