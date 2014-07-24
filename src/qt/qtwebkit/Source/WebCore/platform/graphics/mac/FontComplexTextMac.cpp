/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Font.h"

#include "ComplexTextController.h"
#include "FontGlyphs.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int h,
                                            int from, int to) const
{
    ComplexTextController controller(this, run);
    controller.advance(from);
    float beforeWidth = controller.runWidthSoFar();
    controller.advance(to);
    float afterWidth = controller.runWidthSoFar();

    // Using roundf() rather than ceilf() for the right edge as a compromise to ensure correct caret positioning
    if (run.rtl()) {
        float totalWidth = controller.totalWidth();
        return FloatRect(floorf(point.x() + totalWidth - afterWidth), point.y(), roundf(point.x() + totalWidth - beforeWidth) - floorf(point.x() + totalWidth - afterWidth), h);
    } 

    return FloatRect(floorf(point.x() + beforeWidth), point.y(), roundf(point.x() + afterWidth) - floorf(point.x() + beforeWidth), h);
}

float Font::getGlyphsAndAdvancesForComplexText(const TextRun& run, int from, int to, GlyphBuffer& glyphBuffer, ForTextEmphasisOrNot forTextEmphasis) const
{
    float initialAdvance;

    ComplexTextController controller(this, run, false, 0, forTextEmphasis);
    controller.advance(from);
    float beforeWidth = controller.runWidthSoFar();
    controller.advance(to, &glyphBuffer);

    if (glyphBuffer.isEmpty())
        return 0;

    float afterWidth = controller.runWidthSoFar();

    if (run.rtl()) {
        initialAdvance = controller.totalWidth() + controller.finalRoundingWidth() - afterWidth;
        glyphBuffer.reverse(0, glyphBuffer.size());
    } else
        initialAdvance = beforeWidth;

    return initialAdvance;
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    // This glyph buffer holds our glyphs + advances + font data for each glyph.
    GlyphBuffer glyphBuffer;

    float startX = point.x() + getGlyphsAndAdvancesForComplexText(run, from, to, glyphBuffer);

    // We couldn't generate any glyphs for the run.  Give up.
    if (glyphBuffer.isEmpty())
        return;

    // Draw the glyph buffer now at the starting point returned in startX.
    FloatPoint startPoint(startX, point.y());
    drawGlyphBuffer(context, run, glyphBuffer, startPoint);
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    GlyphBuffer glyphBuffer;
    float initialAdvance = getGlyphsAndAdvancesForComplexText(run, from, to, glyphBuffer, ForTextEmphasis);

    if (glyphBuffer.isEmpty())
        return;

    drawEmphasisMarks(context, run, glyphBuffer, mark, FloatPoint(point.x() + initialAdvance, point.y()));
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    ComplexTextController controller(this, run, true, fallbackFonts);
    if (glyphOverflow) {
        glyphOverflow->top = max<int>(glyphOverflow->top, ceilf(-controller.minGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().ascent()));
        glyphOverflow->bottom = max<int>(glyphOverflow->bottom, ceilf(controller.maxGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().descent()));
        glyphOverflow->left = max<int>(0, ceilf(-controller.minGlyphBoundingBoxX()));
        glyphOverflow->right = max<int>(0, ceilf(controller.maxGlyphBoundingBoxX() - controller.totalWidth()));
    }
    return controller.totalWidth();
}

int Font::offsetForPositionForComplexText(const TextRun& run, float x, bool includePartialGlyphs) const
{
    ComplexTextController controller(this, run);
    return controller.offsetForPosition(x, includePartialGlyphs);
}

const SimpleFontData* Font::fontDataForCombiningCharacterSequence(const UChar* characters, size_t length, FontDataVariant variant) const
{
    UChar32 baseCharacter;
    size_t baseCharacterLength = 0;
    U16_NEXT(characters, baseCharacterLength, length, baseCharacter);

    GlyphData baseCharacterGlyphData = glyphDataForCharacter(baseCharacter, false, variant);

    if (!baseCharacterGlyphData.glyph)
        return 0;

    if (length == baseCharacterLength)
        return baseCharacterGlyphData.fontData;

    bool triedBaseCharacterFontData = false;

    unsigned i = 0;
    for (const FontData* fontData = fontDataAt(0); fontData; fontData = fontDataAt(++i)) {
        const SimpleFontData* simpleFontData = fontData->fontDataForCharacter(baseCharacter);
        if (variant == NormalVariant) {
            if (simpleFontData->platformData().orientation() == Vertical) {
                if (isCJKIdeographOrSymbol(baseCharacter) && !simpleFontData->hasVerticalGlyphs()) {
                    variant = BrokenIdeographVariant;
                    simpleFontData = simpleFontData->brokenIdeographFontData().get();
                } else if (m_fontDescription.nonCJKGlyphOrientation() == NonCJKGlyphOrientationVerticalRight) {
                    SimpleFontData* verticalRightFontData = simpleFontData->verticalRightOrientationFontData().get();
                    Glyph verticalRightGlyph = verticalRightFontData->glyphForCharacter(baseCharacter);
                    if (verticalRightGlyph == baseCharacterGlyphData.glyph)
                        simpleFontData = verticalRightFontData;
                } else {
                    SimpleFontData* uprightFontData = simpleFontData->uprightOrientationFontData().get();
                    Glyph uprightGlyph = uprightFontData->glyphForCharacter(baseCharacter);
                    if (uprightGlyph != baseCharacterGlyphData.glyph)
                        simpleFontData = uprightFontData;
                }
            }
        } else {
            if (const SimpleFontData* variantFontData = simpleFontData->variantFontData(m_fontDescription, variant).get())
                simpleFontData = variantFontData;
        }

        if (simpleFontData == baseCharacterGlyphData.fontData)
            triedBaseCharacterFontData = true;

        if (simpleFontData->canRenderCombiningCharacterSequence(characters, length))
            return simpleFontData;
    }

    if (!triedBaseCharacterFontData && baseCharacterGlyphData.fontData && baseCharacterGlyphData.fontData->canRenderCombiningCharacterSequence(characters, length))
        return baseCharacterGlyphData.fontData;

    return SimpleFontData::systemFallback();
}

} // namespace WebCore
