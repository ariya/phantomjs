/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "Font.h"

#include "FontFallbackList.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "Logging.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include "UniscribeController.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

bool Font::canReturnFallbackFontsForComplexText()
{
    return true;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int h,
                                            int from, int to) const
{
    UniscribeController it(this, run);
    it.advance(from);
    float beforeWidth = it.runWidthSoFar();
    it.advance(to);
    float afterWidth = it.runWidthSoFar();

    // Using roundf() rather than ceilf() for the right edge as a compromise to ensure correct caret positioning
    if (run.rtl()) {
        it.advance(run.length());
        float totalWidth = it.runWidthSoFar();
        return FloatRect(point.x() + floorf(totalWidth - afterWidth), point.y(), roundf(totalWidth - beforeWidth) - floorf(totalWidth - afterWidth), h);
    } 
    
    return FloatRect(point.x() + floorf(beforeWidth), point.y(), roundf(afterWidth) - floorf(beforeWidth), h);
}

float Font::getGlyphsAndAdvancesForComplexText(const TextRun& run, int from, int to, GlyphBuffer& glyphBuffer, ForTextEmphasisOrNot forTextEmphasis) const
{
    if (forTextEmphasis) {
        // FIXME: Add forTextEmphasis paremeter to UniscribeController and use it.
        LOG_ERROR("Not implemented for text emphasis.");
        return 0;
    }

    UniscribeController controller(this, run);
    controller.advance(from);
    float beforeWidth = controller.runWidthSoFar();
    controller.advance(to, &glyphBuffer);

    if (glyphBuffer.isEmpty())
        return 0;

    float afterWidth = controller.runWidthSoFar();

    if (run.rtl()) {
        controller.advance(run.length());
        return controller.runWidthSoFar() - afterWidth;
    }
    return beforeWidth;
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point,
                           int from, int to) const
{
    // This glyph buffer holds our glyphs + advances + font data for each glyph.
    GlyphBuffer glyphBuffer;

    float startX = point.x() + getGlyphsAndAdvancesForComplexText(run, from, to, glyphBuffer);

    // We couldn't generate any glyphs for the run.  Give up.
    if (glyphBuffer.isEmpty())
        return;

    // Draw the glyph buffer now at the starting point returned in startX.
    FloatPoint startPoint(startX, point.y());
    drawGlyphBuffer(context, glyphBuffer, startPoint);
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    GlyphBuffer glyphBuffer;
    float initialAdvance = getGlyphsAndAdvancesForComplexText(run, from, to, glyphBuffer, ForTextEmphasis);

    if (glyphBuffer.isEmpty())
        return;

    drawEmphasisMarks(context, glyphBuffer, mark, FloatPoint(point.x() + initialAdvance, point.y()));
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    UniscribeController controller(this, run, fallbackFonts);
    controller.advance(run.length());
    if (glyphOverflow) {
        glyphOverflow->top = max<int>(glyphOverflow->top, ceilf(-controller.minGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().ascent()));
        glyphOverflow->bottom = max<int>(glyphOverflow->bottom, ceilf(controller.maxGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0  : fontMetrics().descent()));
        glyphOverflow->left = max<int>(0, ceilf(-controller.minGlyphBoundingBoxX()));
        glyphOverflow->right = max<int>(0, ceilf(controller.maxGlyphBoundingBoxX() - controller.runWidthSoFar()));
    }
    return controller.runWidthSoFar();
}

int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat, bool includePartialGlyphs) const
{
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int x = static_cast<int>(xFloat);

    UniscribeController controller(this, run);
    return controller.offsetForPosition(x, includePartialGlyphs);
}

}
