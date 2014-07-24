/*
 * Copyright (C) 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ComplexTextController_h
#define ComplexTextController_h

#include "GlyphBuffer.h"
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/Unicode.h>

typedef unsigned short CGGlyph;

typedef const struct __CTRun * CTRunRef;
typedef const struct __CTLine * CTLineRef;

namespace WebCore {

class Font;
class SimpleFontData;
class TextRun;

enum GlyphIterationStyle { IncludePartialGlyphs, ByWholeGlyphs };

// ComplexTextController is responsible for rendering and measuring glyphs for
// complex scripts on OS X.
class ComplexTextController {
public:
    ComplexTextController(const Font*, const TextRun&, bool mayUseNaturalWritingDirection = false, HashSet<const SimpleFontData*>* fallbackFonts = 0, bool forTextEmphasis = false);

    // Advance and emit glyphs up to the specified character.
    void advance(unsigned to, GlyphBuffer* = 0, GlyphIterationStyle = IncludePartialGlyphs, HashSet<const SimpleFontData*>* fallbackFonts = 0);

    // Compute the character offset for a given x coordinate.
    int offsetForPosition(float x, bool includePartialGlyphs);

    // Returns the width of everything we've consumed so far.
    float runWidthSoFar() const { return m_runWidthSoFar; }

    float totalWidth() const { return m_totalWidth; }

    float finalRoundingWidth() const { return m_finalRoundingWidth; }

    float minGlyphBoundingBoxX() const { return m_minGlyphBoundingBoxX; }
    float maxGlyphBoundingBoxX() const { return m_maxGlyphBoundingBoxX; }
    float minGlyphBoundingBoxY() const { return m_minGlyphBoundingBoxY; }
    float maxGlyphBoundingBoxY() const { return m_maxGlyphBoundingBoxY; }
    
private:
    class ComplexTextRun : public RefCounted<ComplexTextRun> {
    public:
        static PassRefPtr<ComplexTextRun> create(CTRunRef ctRun, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange)
        {
            return adoptRef(new ComplexTextRun(ctRun, fontData, characters, stringLocation, stringLength, runRange));
        }

        static PassRefPtr<ComplexTextRun> create(const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr)
        {
            return adoptRef(new ComplexTextRun(fontData, characters, stringLocation, stringLength, ltr));
        }

        unsigned glyphCount() const { return m_glyphCount; }
        const SimpleFontData* fontData() const { return m_fontData; }
        const UChar* characters() const { return m_characters; }
        unsigned stringLocation() const { return m_stringLocation; }
        size_t stringLength() const { return m_stringLength; }
        ALWAYS_INLINE CFIndex indexAt(size_t i) const;
        CFIndex indexBegin() const { return m_indexBegin; }
        CFIndex indexEnd() const { return m_indexEnd; }
        CFIndex endOffsetAt(size_t i) const { ASSERT(!m_isMonotonic); return m_glyphEndOffsets[i]; }
        const CGGlyph* glyphs() const { return m_glyphs; }
        CGSize initialAdvance() const { return m_initialAdvance; }
        const CGSize* advances() const { return m_advances; }
        bool isLTR() const { return m_isLTR; }
        bool isMonotonic() const { return m_isMonotonic; }
        void setIsNonMonotonic();

    private:
        ComplexTextRun(CTRunRef, const SimpleFontData*, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange);
        ComplexTextRun(const SimpleFontData*, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr);

        unsigned m_glyphCount;
        const SimpleFontData* m_fontData;
        const UChar* m_characters;
        unsigned m_stringLocation;
        size_t m_stringLength;
        Vector<CFIndex, 64> m_coreTextIndicesVector;
        const CFIndex* m_coreTextIndices;
        CFIndex m_indexBegin;
        CFIndex m_indexEnd;
        Vector<CFIndex, 64> m_glyphEndOffsets;
        Vector<CGGlyph, 64> m_glyphsVector;
        const CGGlyph* m_glyphs;
        CGSize m_initialAdvance;
        Vector<CGSize, 64> m_advancesVector;
        const CGSize* m_advances;
        bool m_isLTR;
        bool m_isMonotonic;
    };
    
    static unsigned stringBegin(const ComplexTextRun& run) { return run.stringLocation() + run.indexBegin(); }
    static unsigned stringEnd(const ComplexTextRun& run) { return run.stringLocation() + run.indexEnd(); }

    void collectComplexTextRuns();

    void collectComplexTextRunsForCharacters(const UChar*, unsigned length, unsigned stringLocation, const SimpleFontData*);
    void adjustGlyphsAndAdvances();

    unsigned indexOfCurrentRun(unsigned& leftmostGlyph);
    unsigned incrementCurrentRun(unsigned& leftmostGlyph);

    // The initial capacity of these vectors was selected as being the smallest power of two greater than
    // the average (3.5) plus one standard deviation (7.5) of nonzero sizes used on Arabic Wikipedia.
    Vector<unsigned, 16> m_runIndices;
    Vector<unsigned, 16> m_glyphCountFromStartToIndex;

    const Font& m_font;
    const TextRun& m_run;
    bool m_isLTROnly;
    bool m_mayUseNaturalWritingDirection;
    bool m_forTextEmphasis;

    Vector<String> m_stringsFor8BitRuns;
    Vector<UChar, 256> m_smallCapsBuffer;

    // Retain lines rather than their runs for better performance.
    Vector<RetainPtr<CTLineRef> > m_coreTextLines;
    Vector<RefPtr<ComplexTextRun>, 16> m_complexTextRuns;
    Vector<CGSize, 256> m_adjustedAdvances;
    Vector<CGGlyph, 256> m_adjustedGlyphs;
 
    unsigned m_currentCharacter;
    int m_end;

    CGFloat m_totalWidth;

    float m_runWidthSoFar;
    unsigned m_numGlyphsSoFar;
    size_t m_currentRun;
    unsigned m_glyphInCurrentRun;
    unsigned m_characterInCurrentGlyph;
    float m_finalRoundingWidth;
    float m_expansion;
    float m_expansionPerOpportunity;
    float m_leadingExpansion;
    bool m_afterExpansion;

    HashSet<const SimpleFontData*>* m_fallbackFonts;

    float m_minGlyphBoundingBoxX;
    float m_maxGlyphBoundingBoxX;
    float m_minGlyphBoundingBoxY;
    float m_maxGlyphBoundingBoxY;

    unsigned m_lastRoundingGlyph;
};

} // namespace WebCore

#endif // ComplexTextController_h
