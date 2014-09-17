/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
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
#include "CSSFontFaceSource.h"

#include "CachedFont.h"
#include "CSSFontFace.h"
#include "CSSFontSelector.h"
#include "CachedResourceLoader.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "GlyphPageTreeNode.h"
#include "SimpleFontData.h"

#if ENABLE(SVG_FONTS)
#include "FontCustomPlatformData.h"
#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#include "SVGURIReference.h"
#endif

namespace WebCore {

CSSFontFaceSource::CSSFontFaceSource(const String& str, CachedFont* font)
    : m_string(str)
    , m_font(font)
    , m_face(0)
#if ENABLE(SVG_FONTS)
    , m_hasExternalSVGFont(false)
#endif
{
    if (m_font)
        m_font->addClient(this);
}

CSSFontFaceSource::~CSSFontFaceSource()
{
    if (m_font)
        m_font->removeClient(this);
    pruneTable();
}

void CSSFontFaceSource::pruneTable()
{
    if (m_fontDataTable.isEmpty())
        return;
    HashMap<unsigned, SimpleFontData*>::iterator end = m_fontDataTable.end();
    for (HashMap<unsigned, SimpleFontData*>::iterator it = m_fontDataTable.begin(); it != end; ++it)
        GlyphPageTreeNode::pruneTreeCustomFontData(it->second);
    deleteAllValues(m_fontDataTable);
    m_fontDataTable.clear();
}

bool CSSFontFaceSource::isLoaded() const
{
    if (m_font)
        return m_font->isLoaded();
    return true;
}

bool CSSFontFaceSource::isValid() const
{
    if (m_font)
        return !m_font->errorOccurred();
    return true;
}

void CSSFontFaceSource::fontLoaded(CachedFont*)
{
    pruneTable();
    if (m_face)
        m_face->fontLoaded(this);
}

SimpleFontData* CSSFontFaceSource::getFontData(const FontDescription& fontDescription, bool syntheticBold, bool syntheticItalic, CSSFontSelector* fontSelector)
{
    // If the font hasn't loaded or an error occurred, then we've got nothing.
    if (!isValid())
        return 0;

#if ENABLE(SVG_FONTS)
    if (!m_font && !m_svgFontFaceElement) {
#else
    if (!m_font) {
#endif
        SimpleFontData* fontData = fontCache()->getCachedFontData(fontDescription, m_string);

        // We're local. Just return a SimpleFontData from the normal cache.
        return fontData;
    }

    // See if we have a mapping in our FontData cache.
    unsigned hashKey = (fontDescription.computedPixelSize() + 1) << 6 | fontDescription.widthVariant() << 4
                       | (fontDescription.textOrientation() == TextOrientationUpright ? 8 : 0) | (fontDescription.orientation() == Vertical ? 4 : 0) | (syntheticBold ? 2 : 0) | (syntheticItalic ? 1 : 0);
    if (SimpleFontData* cachedData = m_fontDataTable.get(hashKey))
        return cachedData;

    OwnPtr<SimpleFontData> fontData;

    // If we are still loading, then we let the system pick a font.
    if (isLoaded()) {
        if (m_font) {
#if ENABLE(SVG_FONTS)
            if (m_hasExternalSVGFont) {
                // For SVG fonts parse the external SVG document, and extract the <font> element.
                if (!m_font->ensureSVGFontData())
                    return 0;

                if (!m_externalSVGFontElement)
                    m_externalSVGFontElement = m_font->getSVGFontById(SVGURIReference::getTarget(m_string));

                if (!m_externalSVGFontElement)
                    return 0;

                SVGFontFaceElement* fontFaceElement = 0;

                // Select first <font-face> child
                for (Node* fontChild = m_externalSVGFontElement->firstChild(); fontChild; fontChild = fontChild->nextSibling()) {
                    if (fontChild->hasTagName(SVGNames::font_faceTag)) {
                        fontFaceElement = static_cast<SVGFontFaceElement*>(fontChild);
                        break;
                    }
                }

                if (fontFaceElement) {
                    if (!m_svgFontFaceElement) {
                        // We're created using a CSS @font-face rule, that means we're not associated with a SVGFontFaceElement.
                        // Use the imported <font-face> tag as referencing font-face element for these cases.
                        m_svgFontFaceElement = fontFaceElement;
                    }

                    fontData = adoptPtr(new SimpleFontData(adoptPtr(new SVGFontData(fontFaceElement)), fontDescription.computedPixelSize(), syntheticBold, syntheticItalic));
                }
            } else
#endif
            {
                // Create new FontPlatformData from our CGFontRef, point size and ATSFontRef.
                if (!m_font->ensureCustomFontData())
                    return 0;

                fontData = adoptPtr(new SimpleFontData(m_font->platformDataFromCustomData(fontDescription.computedPixelSize(), syntheticBold, syntheticItalic, fontDescription.orientation(),
                                                                                   fontDescription.textOrientation(), fontDescription.widthVariant(), fontDescription.renderingMode()), true, false));
            }
        } else {
#if ENABLE(SVG_FONTS)
            // In-Document SVG Fonts
            if (m_svgFontFaceElement)
                fontData = adoptPtr(new SimpleFontData(adoptPtr(new SVGFontData(m_svgFontFaceElement.get())), fontDescription.computedPixelSize(), syntheticBold, syntheticItalic));
#endif
        }
    } else {
        // Kick off the load now.
        if (CachedResourceLoader* cachedResourceLoader = fontSelector->cachedResourceLoader())
            m_font->beginLoadIfNeeded(cachedResourceLoader);
        // FIXME: m_string is a URL so it makes no sense to pass it as a family name.
        SimpleFontData* tempData = fontCache()->getCachedFontData(fontDescription, m_string);
        if (!tempData)
            tempData = fontCache()->getLastResortFallbackFont(fontDescription);

        fontData = adoptPtr(new SimpleFontData(tempData->platformData(), true, true));
    }

    SimpleFontData* fontDataRawPtr = fontData.leakPtr();
    m_fontDataTable.set(hashKey, fontDataRawPtr);

    return fontDataRawPtr;
}

#if ENABLE(SVG_FONTS)
SVGFontFaceElement* CSSFontFaceSource::svgFontFaceElement() const
{
    return m_svgFontFaceElement.get();
}

void CSSFontFaceSource::setSVGFontFaceElement(PassRefPtr<SVGFontFaceElement> element) 
{ 
    m_svgFontFaceElement = element;
}

bool CSSFontFaceSource::isSVGFontFaceSource() const
{
    return m_svgFontFaceElement || m_hasExternalSVGFont;
}
#endif

}
