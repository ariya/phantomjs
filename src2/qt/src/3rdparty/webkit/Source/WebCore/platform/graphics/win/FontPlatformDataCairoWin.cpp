/*
 * This file is part of the internal font implementation.  It should not be included by anyone other than
 * FontMac.cpp, FontWin.cpp and Font.cpp.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc.
 * Copyright (C) 2007 Alp Toker
 * Copyright (C) 2008, 2010, 2011 Brent Fulgham
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
 *
 */

#include "config.h"
#include "FontPlatformData.h"

#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

#include <cairo-win32.h>

using namespace std;

namespace WebCore {

void FontPlatformData::platformDataInit(HFONT font, float size, HDC hdc, WCHAR* faceName)
{
    cairo_font_face_t* fontFace = cairo_win32_font_face_create_for_hfont(font);

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, size, size);

    static cairo_font_options_t* fontOptions = 0;
    if (!fontOptions) {
       fontOptions = cairo_font_options_create();
       cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_SUBPIXEL);
    }

    m_scaledFont = cairo_scaled_font_create(fontFace, &sizeMatrix, &ctm, fontOptions);
    cairo_font_face_destroy(fontFace);
}

FontPlatformData::FontPlatformData(cairo_font_face_t* fontFace, float size, bool bold, bool oblique)
    : m_font(0)
    , m_size(size)
    , m_orientation(Horizontal)
    , m_textOrientation(TextOrientationVerticalRight)
    , m_widthVariant(RegularWidth)
    , m_scaledFont(0)
    , m_isColorBitmapFont(false)
    , m_syntheticBold(bold)
    , m_syntheticOblique(oblique)
    , m_useGDI(false)
{
   cairo_matrix_t fontMatrix;
   cairo_matrix_init_scale(&fontMatrix, size, size);
   cairo_matrix_t ctm;
   cairo_matrix_init_identity(&ctm);
   cairo_font_options_t* options = cairo_font_options_create();

   // We force antialiasing and disable hinting to provide consistent
   // typographic qualities for custom fonts on all platforms.
   cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
   cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);

   m_scaledFont = cairo_scaled_font_create(fontFace, &fontMatrix, &ctm, options);
   cairo_font_options_destroy(options);
}

FontPlatformData::~FontPlatformData()
{
    if (m_scaledFont && m_scaledFont != hashTableDeletedFontValue())
        cairo_scaled_font_destroy(m_scaledFont);
}

void FontPlatformData::platformDataInit(const FontPlatformData& source)
{
    m_font = source.m_font;
    m_useGDI = source.m_useGDI;
    m_scaledFont = 0;

    if (source.m_scaledFont)
        m_scaledFont = cairo_scaled_font_reference(source.m_scaledFont);
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& other)
{
    m_font = other.m_font;
    m_useGDI = other.m_useGDI;

    if (m_scaledFont && m_scaledFont != hashTableDeletedFontValue())
        cairo_scaled_font_destroy(m_scaledFont);

    m_scaledFont = cairo_scaled_font_reference(other.m_scaledFont);

    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    return m_font == other.m_font
        && m_scaledFont == other.m_scaledFont
        && m_useGDI == other.m_useGDI;
}

}
