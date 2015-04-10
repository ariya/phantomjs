/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Holger Hans Peter Freyther
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

#include "AffineTransform.h"
#include "CairoUtilities.h"
#include "GlyphBuffer.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "ShadowBlur.h"
#include "SimpleFontData.h"

namespace WebCore {

static void drawGlyphsToContext(cairo_t* context, const SimpleFontData* font, GlyphBufferGlyph* glyphs, int numGlyphs)
{
    cairo_matrix_t originalTransform;
    float syntheticBoldOffset = font->syntheticBoldOffset();
    if (syntheticBoldOffset)
        cairo_get_matrix(context, &originalTransform);

    cairo_set_scaled_font(context, font->platformData().scaledFont());
    cairo_show_glyphs(context, glyphs, numGlyphs);

    if (syntheticBoldOffset) {
        cairo_translate(context, syntheticBoldOffset, 0);
        cairo_show_glyphs(context, glyphs, numGlyphs);
    }

    if (syntheticBoldOffset)
        cairo_set_matrix(context, &originalTransform);
}

static void drawGlyphsShadow(GraphicsContext* graphicsContext, const FloatPoint& point, const SimpleFontData* font, GlyphBufferGlyph* glyphs, int numGlyphs)
{
    ShadowBlur& shadow = graphicsContext->platformContext()->shadowBlur();

    if (!(graphicsContext->textDrawingMode() & TextModeFill) || shadow.type() == ShadowBlur::NoShadow)
        return;

    if (!graphicsContext->mustUseShadowBlur()) {
        // Optimize non-blurry shadows, by just drawing text without the ShadowBlur.
        cairo_t* context = graphicsContext->platformContext()->cr();
        cairo_save(context);

        FloatSize shadowOffset(graphicsContext->state().shadowOffset);
        cairo_translate(context, shadowOffset.width(), shadowOffset.height());
        setSourceRGBAFromColor(context, graphicsContext->state().shadowColor);
        drawGlyphsToContext(context, font, glyphs, numGlyphs);

        cairo_restore(context);
        return;
    }

    cairo_text_extents_t extents;
    cairo_scaled_font_glyph_extents(font->platformData().scaledFont(), glyphs, numGlyphs, &extents);
    FloatRect fontExtentsRect(point.x() + extents.x_bearing, point.y() + extents.y_bearing, extents.width, extents.height);

    if (GraphicsContext* shadowContext = shadow.beginShadowLayer(graphicsContext, fontExtentsRect)) {
        drawGlyphsToContext(shadowContext->platformContext()->cr(), font, glyphs, numGlyphs);
        shadow.endShadowLayer(graphicsContext);
    }
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point) const
{
    if (!font->platformData().size())
        return;

    GlyphBufferGlyph* glyphs = const_cast<GlyphBufferGlyph*>(glyphBuffer.glyphs(from));

    float offset = point.x();
    for (int i = 0; i < numGlyphs; i++) {
        glyphs[i].x = offset;
        glyphs[i].y = point.y();
        offset += glyphBuffer.advanceAt(from + i).width();
    }

    PlatformContextCairo* platformContext = context->platformContext();
    drawGlyphsShadow(context, point, font, glyphs, numGlyphs);

    cairo_t* cr = platformContext->cr();
    cairo_save(cr);

    if (context->textDrawingMode() & TextModeFill) {
        platformContext->prepareForFilling(context->state(), PlatformContextCairo::AdjustPatternForGlobalAlpha);
        drawGlyphsToContext(cr, font, glyphs, numGlyphs);
    }

    // Prevent running into a long computation within cairo. If the stroke width is
    // twice the size of the width of the text we will not ask cairo to stroke
    // the text as even one single stroke would cover the full wdth of the text.
    //  See https://bugs.webkit.org/show_bug.cgi?id=33759.
    if (context->textDrawingMode() & TextModeStroke && context->strokeThickness() < 2 * offset) {
        platformContext->prepareForStroking(context->state());
        cairo_set_line_width(cr, context->strokeThickness());

        // This may disturb the CTM, but we are going to call cairo_restore soon after.
        cairo_set_scaled_font(cr, font->platformData().scaledFont());
        cairo_glyph_path(cr, glyphs, numGlyphs);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}

}
