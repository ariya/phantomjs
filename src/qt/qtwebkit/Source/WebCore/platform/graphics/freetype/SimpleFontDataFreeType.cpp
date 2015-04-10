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

#include "FloatConversion.h"
#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "GlyphBuffer.h"
#include "UTF16UChar32Iterator.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <fontconfig/fcfreetype.h>
#include <unicode/normlzr.h>
#include <wtf/MathExtras.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

void SimpleFontData::platformInit()
{
    if (!m_platformData.m_size)
        return;

    ASSERT(m_platformData.scaledFont());
    cairo_font_extents_t font_extents;
    cairo_text_extents_t text_extents;
    cairo_scaled_font_extents(m_platformData.scaledFont(), &font_extents);

    float ascent = narrowPrecisionToFloat(font_extents.ascent);
    float descent = narrowPrecisionToFloat(font_extents.descent);
    float lineGap = narrowPrecisionToFloat(font_extents.height - font_extents.ascent - font_extents.descent);

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);

#if PLATFORM(EFL)
    m_fontMetrics.setLineSpacing(ascent + descent + lineGap);
#else
    // Match CoreGraphics metrics.
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));
#endif
    m_fontMetrics.setLineGap(lineGap);

    cairo_scaled_font_text_extents(m_platformData.scaledFont(), "x", &text_extents);
    m_fontMetrics.setXHeight(narrowPrecisionToFloat(text_extents.height));

    cairo_scaled_font_text_extents(m_platformData.scaledFont(), " ", &text_extents);
    m_spaceWidth = narrowPrecisionToFloat(text_extents.x_advance);

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

PassRefPtr<SimpleFontData> SimpleFontData::platformCreateScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    ASSERT(m_platformData.scaledFont());
    return SimpleFontData::create(FontPlatformData(cairo_scaled_font_get_font_face(m_platformData.scaledFont()),
                                                        scaleFactor * fontDescription.computedSize(),
                                                        m_platformData.syntheticBold(),
                                                        m_platformData.syntheticOblique()), isCustomFont(), false);
}

bool SimpleFontData::containsCharacters(const UChar* characters, int bufferLength) const
{
    ASSERT(m_platformData.scaledFont());
    FT_Face face = cairo_ft_scaled_font_lock_face(m_platformData.scaledFont());
    if (!face)
        return false;

    UTF16UChar32Iterator iterator(characters, bufferLength);
    UChar32 character = iterator.next();
    while (character != iterator.end()) {
        if (!FcFreeTypeCharIndex(face, character)) {
            cairo_ft_scaled_font_unlock_face(m_platformData.scaledFont());
            return false;
        }
        character = iterator.next();
    }

    cairo_ft_scaled_font_unlock_face(m_platformData.scaledFont());
    return true;
}

void SimpleFontData::determinePitch()
{
    m_treatAsFixedPitch = m_platformData.isFixedPitch();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    if (!m_platformData.size())
        return FloatRect();

    cairo_glyph_t cglyph = { glyph, 0, 0 };
    cairo_text_extents_t extents;
    cairo_scaled_font_glyph_extents(m_platformData.scaledFont(), &cglyph, 1, &extents);

    if (cairo_scaled_font_status(m_platformData.scaledFont()) == CAIRO_STATUS_SUCCESS)
        return FloatRect(extents.x_bearing, extents.y_bearing, extents.width, extents.height);

    return FloatRect();
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (!m_platformData.size())
        return 0;

    cairo_glyph_t cglyph = { glyph, 0, 0 };
    cairo_text_extents_t extents;
    cairo_scaled_font_glyph_extents(m_platformData.scaledFont(), &cglyph, 1, &extents);

    float w = (float)m_spaceWidth;
    if (cairo_scaled_font_status(m_platformData.scaledFont()) == CAIRO_STATUS_SUCCESS && extents.x_advance)
        w = (float)extents.x_advance;

    return w;    
}

#if USE(HARFBUZZ)
bool SimpleFontData::canRenderCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    if (!m_combiningCharacterSequenceSupport)
        m_combiningCharacterSequenceSupport = adoptPtr(new HashMap<String, bool>);

    WTF::HashMap<String, bool>::AddResult addResult = m_combiningCharacterSequenceSupport->add(String(characters, length), false);
    if (!addResult.isNewEntry)
        return addResult.iterator->value;

    UErrorCode error = U_ZERO_ERROR;
    Vector<UChar, 4> normalizedCharacters(length);
    int32_t normalizedLength = unorm_normalize(characters, length, UNORM_NFC, UNORM_UNICODE_3_2, &normalizedCharacters[0], length, &error);
    // Can't render if we have an error or no composition occurred.
    if (U_FAILURE(error) || (static_cast<size_t>(normalizedLength) == length))
        return false;

    FT_Face face = cairo_ft_scaled_font_lock_face(m_platformData.scaledFont());
    if (!face)
        return false;

    if (FcFreeTypeCharIndex(face, normalizedCharacters[0]))
        addResult.iterator->value = true;

    cairo_ft_scaled_font_unlock_face(m_platformData.scaledFont());
    return addResult.iterator->value;
}
#endif

}
