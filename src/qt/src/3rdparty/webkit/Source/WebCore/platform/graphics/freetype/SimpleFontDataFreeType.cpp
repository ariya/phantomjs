/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SimpleFontData.h"

#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "GlyphBuffer.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <fontconfig/fcfreetype.h>
#include <wtf/MathExtras.h>

namespace WebCore {

void SimpleFontData::platformInit()
{
    cairo_font_extents_t font_extents;
    cairo_text_extents_t text_extents;
    cairo_scaled_font_extents(m_platformData.scaledFont(), &font_extents);

    m_fontMetrics.setAscent(font_extents.ascent);
    m_fontMetrics.setDescent(font_extents.descent);

    // There seems to be some rounding error in cairo (or in how we
    // use cairo) with some fonts, like DejaVu Sans Mono, which makes
    // cairo report a height smaller than ascent + descent, which is
    // wrong and confuses WebCore's layout system. Workaround this
    // while we figure out what's going on.
    float lineSpacing = font_extents.height;
    if (lineSpacing < font_extents.ascent + font_extents.descent)
        lineSpacing = font_extents.ascent + font_extents.descent;

    m_fontMetrics.setLineSpacing(lroundf(lineSpacing));
    m_fontMetrics.setLineGap(lineSpacing - font_extents.ascent - font_extents.descent);

    cairo_scaled_font_text_extents(m_platformData.scaledFont(), "x", &text_extents);
    m_fontMetrics.setXHeight(text_extents.height);

    cairo_scaled_font_text_extents(m_platformData.scaledFont(), " ", &text_extents);
    m_spaceWidth = static_cast<float>(text_extents.x_advance);
    
    m_syntheticBoldOffset = m_platformData.syntheticBold() ? 1.0f : 0.f;
}

void SimpleFontData::platformCharWidthInit()
{
    m_avgCharWidth = 0.f;
    m_maxCharWidth = 0.f;
    initCharWidths();
}

void SimpleFontData::platformDestroy()
{
}

SimpleFontData* SimpleFontData::scaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    return new SimpleFontData(FontPlatformData(cairo_scaled_font_get_font_face(m_platformData.scaledFont()),
        scaleFactor * fontDescription.computedSize(), m_platformData.syntheticBold(), m_platformData.syntheticOblique()),
        isCustomFont(), false);
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    // FIXME: I think we want to ask FontConfig for the right font again.
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = scaledFontData(fontDescription, .7);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = scaledFontData(fontDescription, .5);

    return m_derivedFontData->emphasisMark.get();
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    FT_Face face = cairo_ft_scaled_font_lock_face(m_platformData.scaledFont());

    if (!face)
        return false;

    for (int i = 0; i < length; i++) {
        if (FcFreeTypeCharIndex(face, characters[i]) == 0) {
            cairo_ft_scaled_font_unlock_face(m_platformData.scaledFont());
            return false;
        }
    }

    cairo_ft_scaled_font_unlock_face(m_platformData.scaledFont());

    return true;
}

void SimpleFontData::determinePitch()
{
    m_treatAsFixedPitch = m_platformData.isFixedPitch();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph) const
{
    return FloatRect();
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    ASSERT(m_platformData.scaledFont());

    cairo_glyph_t cglyph = { glyph, 0, 0 };
    cairo_text_extents_t extents;
    cairo_scaled_font_glyph_extents(m_platformData.scaledFont(), &cglyph, 1, &extents);

    float w = (float)m_spaceWidth;
    if (cairo_scaled_font_status(m_platformData.scaledFont()) == CAIRO_STATUS_SUCCESS && extents.x_advance)
        w = (float)extents.x_advance;

    return w;    
}

}
