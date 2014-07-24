/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "Font.h"

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "FontCache.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#include "HarfBuzzShaper.h"
#include "ShadowBlur.h"
#include "TextRun.h"

#include <BlackBerryPlatformGraphicsContext.h>

#include <fs_api.h>
#include <wtf/MathExtras.h>

static inline float FSFixedToFloat(FS_FIXED n) { return n / 65536.0; }
static inline FS_FIXED FloatToFSFixed(float n) { return static_cast<FS_FIXED>(n * 65536); }

namespace WebCore {

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    if (!run.length())
        return;

    GlyphBuffer glyphBuffer;
    HarfBuzzShaper shaper(this, run);
    shaper.setDrawRange(from, to);
    if (!shaper.shape(&glyphBuffer))
        return;
    FloatPoint adjustedPoint = shaper.adjustStartPoint(point);

    drawGlyphBuffer(context, run, glyphBuffer, adjustedPoint);
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* /* fallbackFonts */, GlyphOverflow* /* glyphOverflow */) const
{
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return 0;
    return shaper.totalWidth();
}

// Return the code point index for the given |x| offset into the text run.
int Font::offsetForPositionForComplexText(const TextRun& run, float position, bool) const
{
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int targetX = static_cast<int>(position);

    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return 0;
    return shaper.offsetForPosition(targetX);
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int height, int from, int to) const
{
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return FloatRect();
    return shaper.selectionRect(point, height, from, to);
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    FS_STATE* iType = font->platformData().scaledFont(context->getCTM().yScale());
    if (!iType)
        return;

    FloatPoint adjustedPoint = point;

    if (font->platformData().orientation() == Vertical)
        adjustedPoint.move(-(font->fontMetrics().floatAscent(IdeographicBaseline) - font->fontMetrics().floatAscent()), 0);

    bool softwareBlurRequired = context->state().shadowBlur / font->fontMetrics().xHeight() > 0.5;

    FloatSize currentShadowOffset;
    float currentShadowBlur;
    Color currentShadowColor;
    ColorSpace currentColorSpace;

    // If we have a shadow blur, and it is too big to apply at text-render time, we must render it now.
    if (context->hasShadow() && softwareBlurRequired) {
        context->getShadow(currentShadowOffset, currentShadowBlur, currentShadowColor, currentColorSpace);

        const GraphicsContextState state = context->state();
        FloatSize offset = state.shadowOffset;
        if (state.shadowsIgnoreTransforms)
            offset.setHeight(-offset.height());
        ShadowBlur shadow(FloatSize(state.shadowBlur, state.shadowBlur), offset, state.shadowColor, state.shadowColorSpace);
        FloatPoint minPoint(adjustedPoint.x(), adjustedPoint.y() - fontMetrics().ascent());
        FloatPoint maxPoint(adjustedPoint.x(), adjustedPoint.y() + fontMetrics().descent());
        FloatPoint currentPoint = adjustedPoint;
        for (int i = 0; i < numGlyphs; ++i) {
            currentPoint += *glyphBuffer.advances(from + i);
            minPoint.setX(std::min(minPoint.x(), currentPoint.x()));
            minPoint.setY(std::min(minPoint.y(), currentPoint.y()));
            maxPoint = maxPoint.expandedTo(currentPoint);
        }
        const FloatRect boundingRect(minPoint.x(), minPoint.y(), maxPoint.x() - minPoint.x(), maxPoint.y() - minPoint.y());
        GraphicsContext* shadowContext = shadow.beginShadowLayer(context, boundingRect);
        if (shadowContext) {
            iType = font->platformData().scaledFont(shadowContext->getCTM().yScale());
            shadowContext->platformContext()->addGlyphs(glyphBuffer.glyphs(from),
                reinterpret_cast<const BlackBerry::Platform::FloatSize*>(glyphBuffer.advances(from)),
                numGlyphs, adjustedPoint, iType, 0, 0);
            iType = font->platformData().scaledFont(context->getCTM().yScale());
            shadow.endShadowLayer(context);
        }
        context->platformContext()->clearShadow();
    }

    context->platformContext()->addGlyphs(glyphBuffer.glyphs(from),
        reinterpret_cast<const BlackBerry::Platform::FloatSize*>(glyphBuffer.advances(from)),
        numGlyphs, adjustedPoint, iType,
        context->fillGradient() ? context->fillGradient()->platformGradient() : (context->fillPattern() ? context->fillPattern()->platformPattern(AffineTransform()) : static_cast<BlackBerry::Platform::Graphics::Paint*>(0)),
        context->strokeGradient() ? context->strokeGradient()->platformGradient() : (context->strokePattern() ? context->strokePattern()->platformPattern(AffineTransform()) : static_cast<BlackBerry::Platform::Graphics::Paint*>(0)));

    if (softwareBlurRequired)
        context->platformContext()->setShadow(currentShadowOffset, currentShadowBlur, currentShadowColor.isValid() ? currentShadowColor.rgb() : makeRGBA(0, 0, 0, 0xFF / 3), context->state().shadowsIgnoreTransforms);
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    // notImplemented();
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

}
