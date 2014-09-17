/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGTextMetrics.h"

#include "RenderSVGInlineText.h"
#include "TextRun.h"

namespace WebCore {

SVGTextMetrics::SVGTextMetrics()
    : m_width(0)
    , m_height(0)
    , m_length(0)
{
}

SVGTextMetrics::SVGTextMetrics(RenderSVGInlineText* textRenderer, const TextRun& run, unsigned position, unsigned textLength)
{
    ASSERT(textRenderer);

    float scalingFactor = textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    const Font& scaledFont = textRenderer->scaledFont();

    int extraCharsAvailable = textLength - (position + run.length());
    int length = 0;

    // Calculate width/height using the scaled font, divide this result by the scalingFactor afterwards.
    m_width = scaledFont.width(run, extraCharsAvailable, length, m_glyph.name) / scalingFactor;
    m_height = scaledFont.fontMetrics().floatHeight() / scalingFactor;

    m_glyph.unicodeString = String(run.characters(), length);
    m_glyph.isValid = true;

    ASSERT(length >= 0);
    m_length = static_cast<unsigned>(length);
}

bool SVGTextMetrics::operator==(const SVGTextMetrics& other)
{
    return m_width == other.m_width
        && m_height == other.m_height
        && m_length == other.m_length
        && m_glyph == other.m_glyph;
}

SVGTextMetrics SVGTextMetrics::emptyMetrics()
{
    DEFINE_STATIC_LOCAL(SVGTextMetrics, s_emptyMetrics, ());
    s_emptyMetrics.m_length = 1;
    return s_emptyMetrics;
}

static TextRun constructTextRun(RenderSVGInlineText* text, const UChar* characters, unsigned position, unsigned length)
{
    RenderStyle* style = text->style();
    ASSERT(style);

    TextRun run(characters + position
                , length
                , false /* allowTabs */
                , 0 /* xPos, only relevant with allowTabs=true */
                , 0 /* padding, only relevant for justified text, not relevant for SVG */
                , TextRun::AllowTrailingExpansion
                , style->direction()
                , style->unicodeBidi() == Override /* directionalOverride */);

#if ENABLE(SVG_FONTS)
    run.setReferencingRenderObject(text);
#endif

    // We handle letter & word spacing ourselves.
    run.disableSpacing();
    return run;
}

SVGTextMetrics SVGTextMetrics::measureCharacterRange(RenderSVGInlineText* text, unsigned position, unsigned length)
{
    ASSERT(text);
    TextRun run(constructTextRun(text, text->characters(), position, length));
    return SVGTextMetrics(text, run, position, text->textLength());
}

}

#endif // ENABLE(SVG)
