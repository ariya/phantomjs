/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef UniscribeController_h
#define UniscribeController_h

#include <usp10.h>
#include "Font.h"
#include "GlyphBuffer.h"
#include "Vector.h"

namespace WebCore {

class UniscribeController {
public:
    UniscribeController(const Font*, const TextRun&, HashSet<const SimpleFontData*>* fallbackFonts = 0);

    // Advance and measure/place up to the specified character.
    void advance(unsigned to, GlyphBuffer* = 0);

    // Compute the character offset for a given x coordinate.
    int offsetForPosition(int x, bool includePartialGlyphs);

    // Returns the width of everything we've consumed so far.
    float runWidthSoFar() const { return m_runWidthSoFar; }

    float minGlyphBoundingBoxX() const { return m_minGlyphBoundingBoxX; }
    float maxGlyphBoundingBoxX() const { return m_maxGlyphBoundingBoxX; }
    float minGlyphBoundingBoxY() const { return m_minGlyphBoundingBoxY; }
    float maxGlyphBoundingBoxY() const { return m_maxGlyphBoundingBoxY; }

private:    
    void resetControlAndState();

    void itemizeShapeAndPlace(const UChar*, unsigned length, const SimpleFontData*, GlyphBuffer*);
    bool shapeAndPlaceItem(const UChar*, unsigned index, const SimpleFontData*, GlyphBuffer*);
    bool shape(const UChar* str, int len, SCRIPT_ITEM item, const SimpleFontData* fontData,
               Vector<WORD>& glyphs, Vector<WORD>& clusters,
               Vector<SCRIPT_VISATTR>& visualAttributes);

    const Font& m_font;
    const TextRun& m_run;
    HashSet<const SimpleFontData*>* m_fallbackFonts;
    FloatPoint m_glyphOrigin;
    float m_minGlyphBoundingBoxX;
    float m_maxGlyphBoundingBoxX;
    float m_minGlyphBoundingBoxY;
    float m_maxGlyphBoundingBoxY;

    SCRIPT_CONTROL m_control;
    SCRIPT_STATE m_state;
    Vector<SCRIPT_ITEM> m_items;
 
    unsigned m_currentCharacter;
    int m_end;

    float m_runWidthSoFar;
    float m_padding;
    float m_padPerSpace;

    bool m_computingOffsetPosition;
    bool m_includePartialGlyphs;
    float m_offsetX;
    int m_offsetPosition;
};

}
#endif
