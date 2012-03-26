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
#include <wtf/unicode/Unicode.h>

typedef unsigned short CGGlyph;

#if USE(CORE_TEXT)
typedef const struct __CTRun * CTRunRef;
typedef const struct __CTLine * CTLineRef;
#endif
#if USE(ATSUI)
typedef struct OpaqueATSUTextLayout*    ATSUTextLayout;
typedef struct ATSGlyphVector*          ATSULineRef;
typedef UInt32 ATSULayoutOperationSelector;
typedef UInt32 ATSULayoutOperationCallbackStatus;
#endif

namespace WebCore {

class Font;
class SimpleFontData;
class TextRun;

// ComplexTextController is responsible for rendering and measuring glyphs for
// complex scripts on OS X.
// The underlying API can be selected at compile time based on USE(ATSUI) and
// USE(CORE_TEXT).  If both are defined then the Core Text APIs are used for
// OS Versions >= 10.6, ATSUI is used otherwise.
class ComplexTextController {
public:
    ComplexTextController(const Font*, const TextRun&, bool mayUseNaturalWritingDirection = false, HashSet<const SimpleFontData*>* fallbackFonts = 0, bool forTextEmphasis = false);

    // Advance and emit glyphs up to the specified character.
    void advance(unsigned to, GlyphBuffer* = 0);

    // Compute the character offset for a given x coordinate.
    int offsetForPosition(float x, bool includePartialGlyphs);

    // Returns the width of everything we've consumed so far.
    float runWidthSoFar() const { return m_runWidthSoFar; }

    float totalWidth() const { return m_totalWidth; }

    float minGlyphBoundingBoxX() const { return m_minGlyphBoundingBoxX; }
    float maxGlyphBoundingBoxX() const { return m_maxGlyphBoundingBoxX; }
    float minGlyphBoundingBoxY() const { return m_minGlyphBoundingBoxY; }
    float maxGlyphBoundingBoxY() const { return m_maxGlyphBoundingBoxY; }
    
private:
    class ComplexTextRun : public RefCounted<ComplexTextRun> {
    public:
#if USE(CORE_TEXT)
        static PassRefPtr<ComplexTextRun> create(CTRunRef ctRun, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange)
        {
            return adoptRef(new ComplexTextRun(ctRun, fontData, characters, stringLocation, stringLength, runRange));
        }
#endif
#if USE(ATSUI)
        static PassRefPtr<ComplexTextRun> create(ATSUTextLayout atsuTextLayout, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr, bool directionalOverride)
        {
            return adoptRef(new ComplexTextRun(atsuTextLayout, fontData, characters, stringLocation, stringLength, ltr, directionalOverride));
        }
#endif
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
        CFIndex indexEnd() const { return m_indexEnd; }
        CFIndex endOffsetAt(size_t i) const { ASSERT(!m_isMonotonic); return m_glyphEndOffsets[i]; }
        const CGGlyph* glyphs() const { return m_glyphs; }
        const CGSize* advances() const { return m_advances; }
        bool isMonotonic() const { return m_isMonotonic; }
        void setIsNonMonotonic();

    private:
#if USE(CORE_TEXT)
        ComplexTextRun(CTRunRef, const SimpleFontData*, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange);
        void createTextRunFromFontDataCoreText(bool ltr);
#endif
#if USE(ATSUI)
        ComplexTextRun(ATSUTextLayout, const SimpleFontData*, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr, bool directionalOverride);
        void createTextRunFromFontDataATSUI(bool ltr);
#endif
        ComplexTextRun(const SimpleFontData*, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr);

#if USE(ATSUI)
        static OSStatus overrideLayoutOperation(ATSULayoutOperationSelector, ATSULineRef, URefCon, void*, ATSULayoutOperationCallbackStatus*);
#endif

        unsigned m_glyphCount;
        const SimpleFontData* m_fontData;
        const UChar* m_characters;
        unsigned m_stringLocation;
        size_t m_stringLength;
#if USE(CORE_TEXT)
        Vector<CFIndex, 64> m_coreTextIndicesVector;
        const CFIndex* m_coreTextIndices;
#endif
#if USE(ATSUI)
        Vector<CFIndex, 64> m_atsuiIndices;
#endif
        CFIndex m_indexEnd;
        Vector<CFIndex, 64> m_glyphEndOffsets;
        Vector<CGGlyph, 64> m_glyphsVector;
        const CGGlyph* m_glyphs;
        Vector<CGSize, 64> m_advancesVector;
        const CGSize* m_advances;
#if USE(ATSUI)
        bool m_directionalOverride;
#endif
        bool m_isMonotonic;
    };

    void collectComplexTextRuns();

    // collectComplexTextRunsForCharacters() is a stub function that calls through to the ATSUI or Core Text variants based
    // on the API in use.
    void collectComplexTextRunsForCharacters(const UChar*, unsigned length, unsigned stringLocation, const SimpleFontData*);
    void collectComplexTextRunsForCharactersATSUI(const UChar*, unsigned length, unsigned stringLocation, const SimpleFontData*);
    void collectComplexTextRunsForCharactersCoreText(const UChar*, unsigned length, unsigned stringLocation, const SimpleFontData*);
    void adjustGlyphsAndAdvances();

    const Font& m_font;
    const TextRun& m_run;
    bool m_mayUseNaturalWritingDirection;
    bool m_forTextEmphasis;

    Vector<UChar, 256> m_smallCapsBuffer;

#if USE(CORE_TEXT)
    // Retain lines rather than their runs for better performance.
    Vector<RetainPtr<CTLineRef> > m_coreTextLines;
#endif
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
    float m_expansion;
    float m_expansionPerOpportunity;
    float m_leadingExpansion;
    bool m_afterExpansion;

    HashSet<const SimpleFontData*>* m_fallbackFonts;

    float m_minGlyphBoundingBoxX;
    float m_maxGlyphBoundingBoxX;
    float m_minGlyphBoundingBoxY;
    float m_maxGlyphBoundingBoxY;
};

} // namespace WebCore

#endif // ComplexTextController_h
