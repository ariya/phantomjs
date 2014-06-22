/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "CSSSegmentedFontFace.h"

#include "CSSFontFace.h"
#include "CSSFontFaceSource.h"
#include "CSSFontSelector.h"
#include "Document.h"
#include "FontDescription.h"
#include "RuntimeEnabledFeatures.h"
#include "SegmentedFontData.h"
#include "SimpleFontData.h"

namespace WebCore {

CSSSegmentedFontFace::CSSSegmentedFontFace(CSSFontSelector* fontSelector)
    : m_fontSelector(fontSelector)
{
}

CSSSegmentedFontFace::~CSSSegmentedFontFace()
{
    pruneTable();
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++)
        m_fontFaces[i]->removedFromSegmentedFontFace(this);
}

void CSSSegmentedFontFace::pruneTable()
{
    // Make sure the glyph page tree prunes out all uses of this custom font.
    if (m_fontDataTable.isEmpty())
        return;

    m_fontDataTable.clear();
}

bool CSSSegmentedFontFace::isValid() const
{
    // Valid if at least one font face is valid.
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->isValid())
            return true;
    }
    return false;
}

void CSSSegmentedFontFace::fontLoaded(CSSFontFace*)
{
    pruneTable();

#if ENABLE(FONT_LOAD_EVENTS)
    if (RuntimeEnabledFeatures::fontLoadEventsEnabled() && !isLoading()) {
        Vector<RefPtr<LoadFontCallback> > callbacks;
        m_callbacks.swap(callbacks);
        for (size_t index = 0; index < callbacks.size(); ++index) {
            if (checkFont())
                callbacks[index]->notifyLoaded();
            else
                callbacks[index]->notifyError();
        }
    }
#endif
}

void CSSSegmentedFontFace::appendFontFace(PassRefPtr<CSSFontFace> fontFace)
{
    pruneTable();
    fontFace->addedToSegmentedFontFace(this);
    m_fontFaces.append(fontFace);
}

static void appendFontDataWithInvalidUnicodeRangeIfLoading(SegmentedFontData* newFontData, PassRefPtr<SimpleFontData> prpFaceFontData, const Vector<CSSFontFace::UnicodeRange>& ranges)
{
    RefPtr<SimpleFontData> faceFontData = prpFaceFontData;
    if (faceFontData->isLoading()) {
        newFontData->appendRange(FontDataRange(0, 0, faceFontData));
        return;
    }

    unsigned numRanges = ranges.size();
    if (!numRanges) {
        newFontData->appendRange(FontDataRange(0, 0x7FFFFFFF, faceFontData));
        return;
    }

    for (unsigned j = 0; j < numRanges; ++j)
        newFontData->appendRange(FontDataRange(ranges[j].from(), ranges[j].to(), faceFontData));
}

PassRefPtr<FontData> CSSSegmentedFontFace::getFontData(const FontDescription& fontDescription)
{
    if (!isValid())
        return 0;

    FontTraitsMask desiredTraitsMask = fontDescription.traitsMask();
    unsigned hashKey = ((fontDescription.computedPixelSize() + 1) << (FontTraitsMaskWidth + FontWidthVariantWidth + 1))
        | ((fontDescription.orientation() == Vertical ? 1 : 0) << (FontTraitsMaskWidth + FontWidthVariantWidth))
        | fontDescription.widthVariant() << FontTraitsMaskWidth
        | desiredTraitsMask;

    RefPtr<SegmentedFontData>& fontData = m_fontDataTable.add(hashKey, 0).iterator->value;
    if (fontData && fontData->numRanges())
        return fontData; // No release, we have a reference to an object in the cache which should retain the ref count it has.

    if (!fontData)
        fontData = SegmentedFontData::create();

    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (!m_fontFaces[i]->isValid())
            continue;
        FontTraitsMask traitsMask = m_fontFaces[i]->traitsMask();
        bool syntheticBold = !(traitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask)) && (desiredTraitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask));
        bool syntheticItalic = !(traitsMask & FontStyleItalicMask) && (desiredTraitsMask & FontStyleItalicMask);
        if (RefPtr<SimpleFontData> faceFontData = m_fontFaces[i]->getFontData(fontDescription, syntheticBold, syntheticItalic)) {
            ASSERT(!faceFontData->isSegmented());
            appendFontDataWithInvalidUnicodeRangeIfLoading(fontData.get(), faceFontData.release(), m_fontFaces[i]->ranges());
        }
    }
    if (fontData->numRanges())
        return fontData; // No release, we have a reference to an object in the cache which should retain the ref count it has.

    return 0;
}

#if ENABLE(FONT_LOAD_EVENTS)
bool CSSSegmentedFontFace::isLoading() const
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->loadState() == CSSFontFace::Loading)
            return true;
    }
    return false;
}

bool CSSSegmentedFontFace::checkFont() const
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->loadState() != CSSFontFace::Loaded)
            return false;
    }
    return true;
}

void CSSSegmentedFontFace::loadFont(const FontDescription& fontDescription, PassRefPtr<LoadFontCallback> callback)
{
    getFontData(fontDescription); // Kick off the load.

    if (callback) {
        if (isLoading())
            m_callbacks.append(callback);
        else if (checkFont())
            callback->notifyLoaded();
        else
            callback->notifyError();
    }
}
#endif

}
