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
#include "CSSFontSelector.h"
#include "FontDescription.h"
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
    HashMap<unsigned, SegmentedFontData*>::iterator end = m_fontDataTable.end();
    for (HashMap<unsigned, SegmentedFontData*>::iterator it = m_fontDataTable.begin(); it != end; ++it)
        GlyphPageTreeNode::pruneTreeCustomFontData(it->second);
    deleteAllValues(m_fontDataTable);
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
}

void CSSSegmentedFontFace::appendFontFace(PassRefPtr<CSSFontFace> fontFace)
{
    pruneTable();
    fontFace->addedToSegmentedFontFace(this);
    m_fontFaces.append(fontFace);
}

FontData* CSSSegmentedFontFace::getFontData(const FontDescription& fontDescription)
{
    if (!isValid())
        return 0;

    FontTraitsMask desiredTraitsMask = fontDescription.traitsMask();
    unsigned hashKey = ((fontDescription.computedPixelSize() + 1) << (FontTraitsMaskWidth + 1)) | ((fontDescription.orientation() == Vertical ? 1 : 0) << FontTraitsMaskWidth) | desiredTraitsMask;

    SegmentedFontData* fontData = m_fontDataTable.get(hashKey);
    if (fontData)
        return fontData;

    fontData = new SegmentedFontData();

    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (!m_fontFaces[i]->isValid())
            continue;
        FontTraitsMask traitsMask = m_fontFaces[i]->traitsMask();
        bool syntheticBold = !(traitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask)) && (desiredTraitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask));
        bool syntheticItalic = !(traitsMask & FontStyleItalicMask) && (desiredTraitsMask & FontStyleItalicMask);
        if (const SimpleFontData* faceFontData = m_fontFaces[i]->getFontData(fontDescription, syntheticBold, syntheticItalic)) {
            ASSERT(!faceFontData->isSegmented());
            const Vector<CSSFontFace::UnicodeRange>& ranges = m_fontFaces[i]->ranges();
            unsigned numRanges = ranges.size();
            if (!numRanges)
                fontData->appendRange(FontDataRange(0, 0x7FFFFFFF, faceFontData));
            else {
                for (unsigned j = 0; j < numRanges; ++j)
                    fontData->appendRange(FontDataRange(ranges[j].from(), ranges[j].to(), faceFontData));
            }
        }
    }
    if (fontData->numRanges())
        m_fontDataTable.set(hashKey, fontData);
    else {
        delete fontData;
        fontData = 0;
    }

    return fontData;
}

}
