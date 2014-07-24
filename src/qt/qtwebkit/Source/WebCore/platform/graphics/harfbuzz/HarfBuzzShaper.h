/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HarfBuzzShaper_h
#define HarfBuzzShaper_h

#include "FloatPoint.h"
#include "GlyphBuffer.h"
#include "TextRun.h"
#include "hb.h"
#include <wtf/HashSet.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

class Font;
class SimpleFontData;

class HarfBuzzShaper {
public:
    enum NormalizeMode {
        DoNotNormalizeMirrorChars,
        NormalizeMirrorChars
    };

    HarfBuzzShaper(const Font*, const TextRun&);
    virtual ~HarfBuzzShaper();

    void setDrawRange(int from, int to);
    bool shape(GlyphBuffer* = 0);
    FloatPoint adjustStartPoint(const FloatPoint&);
    float totalWidth() { return m_totalWidth; }
    int offsetForPosition(float targetX);
    FloatRect selectionRect(const FloatPoint&, int height, int from, int to);

private:
    class HarfBuzzRun {
    public:
        static PassOwnPtr<HarfBuzzRun> create(const SimpleFontData* fontData, unsigned startIndex, unsigned numCharacters, TextDirection direction, hb_script_t script)
        {
            return adoptPtr(new HarfBuzzRun(fontData, startIndex, numCharacters, direction, script));
        }

        void applyShapeResult(hb_buffer_t*);
        void setGlyphAndPositions(unsigned index, uint16_t glyphId, float advance, float offsetX, float offsetY);
        void setWidth(float width) { m_width = width; }

        int characterIndexForXPosition(float targetX);
        float xPositionForOffset(unsigned offset);

        const SimpleFontData* fontData() { return m_fontData; }
        unsigned startIndex() const { return m_startIndex; }
        unsigned numCharacters() const { return m_numCharacters; }
        unsigned numGlyphs() const { return m_numGlyphs; }
        uint16_t* glyphs() { return &m_glyphs[0]; }
        float* advances() { return &m_advances[0]; }
        FloatPoint* offsets() { return &m_offsets[0]; }
        uint16_t* glyphToCharacterIndexes() { return &m_glyphToCharacterIndexes[0]; }
        float width() { return m_width; }
        bool rtl() { return m_direction == RTL; }
        hb_script_t script() { return m_script; }

    private:
        HarfBuzzRun(const SimpleFontData*, unsigned startIndex, unsigned numCharacters, TextDirection, hb_script_t);

        const SimpleFontData* m_fontData;
        unsigned m_startIndex;
        size_t m_numCharacters;
        unsigned m_numGlyphs;
        TextDirection m_direction;
        hb_script_t m_script;
        Vector<uint16_t, 256> m_glyphs;
        Vector<float, 256> m_advances;
        Vector<uint16_t, 256> m_glyphToCharacterIndexes;
        Vector<FloatPoint, 256> m_offsets;
        float m_width;
    };

    void setNormalizedBuffer(NormalizeMode = DoNotNormalizeMirrorChars);

    bool isWordEnd(unsigned);
    int determineWordBreakSpacing();
    // setPadding sets a number of pixels to be distributed across the TextRun.
    // WebKit uses this to justify text.
    void setPadding(int);

    // In complex text word-spacing affects each line-break, space (U+0020) and non-breaking space (U+00A0).
    static bool isCodepointSpace(UChar c) { return c == ' ' || c == noBreakSpace || c == '\n'; }

    void setFontFeatures();

    bool collectHarfBuzzRuns();
    bool shapeHarfBuzzRuns(bool shouldSetDirection);
    bool fillGlyphBuffer(GlyphBuffer*);
    void fillGlyphBufferFromHarfBuzzRun(GlyphBuffer*, HarfBuzzRun*, FloatPoint& firstOffsetOfNextRun);
    void setGlyphPositionsForHarfBuzzRun(HarfBuzzRun*, hb_buffer_t*);

    GlyphBufferAdvance createGlyphBufferAdvance(float, float);

    const Font* m_font;
    OwnArrayPtr<UChar> m_normalizedBuffer;
    unsigned m_normalizedBufferLength;
    const TextRun& m_run;

    int m_wordSpacingAdjustment; // Delta adjustment (pixels) for each word break.
    float m_padding; // Pixels to be distributed over the line at word breaks.
    float m_padPerWordBreak; // Pixels to be added to each word break.
    float m_padError; // m_padPerWordBreak might have a fractional component. Since we only add a whole number of padding pixels at each word break we accumulate error. This is the number of pixels that we are behind so far.
    int m_letterSpacing; // Pixels to be added after each glyph.

    Vector<hb_feature_t, 4> m_features;
    Vector<OwnPtr<HarfBuzzRun>, 16> m_harfBuzzRuns;

    FloatPoint m_startOffset;

    int m_fromIndex;
    int m_toIndex;

    float m_totalWidth;
};

} // namespace WebCore

#endif // HarfBuzzShaper_h
