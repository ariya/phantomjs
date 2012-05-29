/*
 * Copyright (C) 2003, 2006, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WidthIterator_h
#define WidthIterator_h

#include <wtf/HashSet.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

class Font;
class GlyphBuffer;
class SimpleFontData;
class TextRun;

struct WidthIterator {
    WidthIterator(const Font*, const TextRun&, HashSet<const SimpleFontData*>* fallbackFonts = 0, bool accountForGlyphBounds = false, bool forTextEmphasis = false);

    void advance(int to, GlyphBuffer* = 0);
    bool advanceOneCharacter(float& width, GlyphBuffer* = 0);

    float maxGlyphBoundingBoxY() const { ASSERT(m_accountForGlyphBounds); return m_maxGlyphBoundingBoxY; }
    float minGlyphBoundingBoxY() const { ASSERT(m_accountForGlyphBounds); return m_minGlyphBoundingBoxY; }
    float firstGlyphOverflow() const { ASSERT(m_accountForGlyphBounds); return m_firstGlyphOverflow; }
    float lastGlyphOverflow() const { ASSERT(m_accountForGlyphBounds); return m_lastGlyphOverflow; }

    const Font* m_font;

    const TextRun& m_run;
    int m_end;

    unsigned m_currentCharacter;
    float m_runWidthSoFar;
    float m_expansion;
    float m_expansionPerOpportunity;
    bool m_isAfterExpansion;

private:
    UChar32 normalizeVoicingMarks(int currentCharacter);

    HashSet<const SimpleFontData*>* m_fallbackFonts;
    bool m_accountForGlyphBounds;
    float m_maxGlyphBoundingBoxY;
    float m_minGlyphBoundingBoxY;
    float m_firstGlyphOverflow;
    float m_lastGlyphOverflow;
    bool m_forTextEmphasis;
};

}

#endif
