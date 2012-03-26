/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
 * Copyright (C) 2009 Igalia S.L.
 * All rights reserved.
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
#include "FontPlatformData.h"

#include "PlatformString.h"
#include "FontDescription.h"
#include <wtf/text/CString.h>
#include <cairo.h>
#include <assert.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

// Use cairo-ft i a recent enough Pango version isn't available
#if !PANGO_VERSION_CHECK(1,18,0)
#include <cairo-ft.h>
#include <pango/pangofc-fontmap.h>
#endif

namespace WebCore {

PangoFontMap* FontPlatformData::m_fontMap = 0;
GHashTable* FontPlatformData::m_hashTable = 0;

FontPlatformData::FontPlatformData(const FontDescription& fontDescription, const AtomicString& familyName)
    : m_context(0)
    , m_font(0)
    , m_size(fontDescription.computedSize())
    , m_syntheticBold(false)
    , m_syntheticOblique(false)
    , m_scaledFont(0)
{
    FontPlatformData::init();

    CString stored_family = familyName.string().utf8();
    char const* families[] = {
      stored_family.data(),
      NULL
    };

    switch (fontDescription.genericFamily()) {
    case FontDescription::SerifFamily:
        families[1] = "serif";
        break;
    case FontDescription::SansSerifFamily:
        families[1] = "sans";
        break;
    case FontDescription::MonospaceFamily:
        families[1] = "monospace";
        break;
    case FontDescription::NoFamily:
    case FontDescription::StandardFamily:
    default:
        families[1] = "sans";
        break;
    }

    PangoFontDescription* description = pango_font_description_new();
    pango_font_description_set_absolute_size(description, fontDescription.computedSize() * PANGO_SCALE);

    // FIXME: Map all FontWeight values to Pango font weights.
    if (fontDescription.weight() >= FontWeight600)
        pango_font_description_set_weight(description, PANGO_WEIGHT_BOLD);
    if (fontDescription.italic())
        pango_font_description_set_style(description, PANGO_STYLE_ITALIC);

#if PANGO_VERSION_CHECK(1,21,5)   // deprecated in 1.21
    m_context = pango_font_map_create_context(m_fontMap);
#else
    m_context = pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(m_fontMap));
#endif
    for (unsigned int i = 0; !m_font && i < G_N_ELEMENTS(families); i++) {
        pango_font_description_set_family(description, families[i]);
        pango_context_set_font_description(m_context, description);
        m_font = pango_font_map_load_font(m_fontMap, m_context, description);
    }

#if PANGO_VERSION_CHECK(1,18,0)
    if (m_font)
        m_scaledFont = cairo_scaled_font_reference(pango_cairo_font_get_scaled_font(PANGO_CAIRO_FONT(m_font)));
#else
    // This compatibility code for older versions of Pango is not well-tested.
    if (m_font) {
        PangoFcFont* fcfont = PANGO_FC_FONT(m_font);
        cairo_font_face_t* face = cairo_ft_font_face_create_for_pattern(fcfont->font_pattern);
        double size;
        if (FcPatternGetDouble(fcfont->font_pattern, FC_PIXEL_SIZE, 0, &size) != FcResultMatch)
            size = 12.0;
        cairo_matrix_t fontMatrix;
        cairo_matrix_init_scale(&fontMatrix, size, size);
        cairo_font_options_t* fontOptions;
        if (pango_cairo_context_get_font_options(m_context))
            fontOptions = cairo_font_options_copy(pango_cairo_context_get_font_options(m_context));
        else
            fontOptions = cairo_font_options_create();
        cairo_matrix_t ctm;
        cairo_matrix_init_identity(&ctm);
        m_scaledFont = cairo_scaled_font_create(face, &fontMatrix, &ctm, fontOptions);
        cairo_font_options_destroy(fontOptions);
        cairo_font_face_destroy(face);
    }
#endif
    pango_font_description_free(description);
}

FontPlatformData::FontPlatformData(float size, bool bold, bool italic)
    : m_context(0)
    , m_font(0)
    , m_size(size)
    , m_syntheticBold(bold)
    , m_syntheticOblique(italic)
    , m_scaledFont(0)
{
}

FontPlatformData::FontPlatformData(cairo_font_face_t* fontFace, float size, bool bold, bool italic)
    : m_context(0)
    , m_font(0)
    , m_size(size)
    , m_syntheticBold(bold)
    , m_syntheticOblique(italic)
    , m_scaledFont(0)
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

bool FontPlatformData::init()
{
    static bool initialized = false;
    if (initialized)
        return true;
    initialized = true;

    if (!m_fontMap)
        m_fontMap = pango_cairo_font_map_get_default();
    if (!m_hashTable) {
        PangoFontFamily** families = 0;
        int n_families = 0;

        m_hashTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

        pango_font_map_list_families(m_fontMap, &families, &n_families);

        for (int family = 0; family < n_families; family++)
                g_hash_table_insert(m_hashTable,
                                    g_strdup(pango_font_family_get_name(families[family])),
                                    g_object_ref(families[family]));

        g_free(families);
    }

    return true;
}

FontPlatformData::~FontPlatformData()
{
    if (m_font && m_font != reinterpret_cast<PangoFont*>(-1)) {
        g_object_unref(m_font);
        m_font = 0;
    }

    if (m_context) {
        g_object_unref(m_context);
        m_context = 0;
    }

    if (m_scaledFont) {
        cairo_scaled_font_destroy(m_scaledFont);
        m_scaledFont = 0;
    }
}

bool FontPlatformData::isFixedPitch()
{
    PangoFontDescription* description = pango_font_describe_with_absolute_size(m_font);
    PangoFontFamily* family = reinterpret_cast<PangoFontFamily*>(g_hash_table_lookup(m_hashTable, pango_font_description_get_family(description)));
    pango_font_description_free(description);
    return pango_font_family_is_monospace(family);
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    // Check for self-assignment.
    if (this == &other)
        return *this;

    m_size = other.m_size;
    m_syntheticBold = other.m_syntheticBold;
    m_syntheticOblique = other.m_syntheticOblique;

    if (other.m_scaledFont)
        cairo_scaled_font_reference(other.m_scaledFont);
    if (m_scaledFont)
        cairo_scaled_font_destroy(m_scaledFont);
    m_scaledFont = other.m_scaledFont;

    if (other.m_font)
        g_object_ref(other.m_font);
    if (m_font)
        g_object_unref(m_font);
    m_font = other.m_font;

    if (other.m_context)
        g_object_ref(other.m_context);
    if (m_context)
        g_object_unref(m_context);
    m_context = other.m_context;

    return *this;
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
    : m_context(0)
    , m_font(0)
    , m_scaledFont(0)
{
    *this = other;
}

bool FontPlatformData::operator==(const FontPlatformData& other) const
{
    if (m_font == other.m_font)
        return true;
    if (m_font == 0 || m_font == reinterpret_cast<PangoFont*>(-1)
        || other.m_font == 0 || other.m_font == reinterpret_cast<PangoFont*>(-1))
        return false;
    PangoFontDescription* thisDesc = pango_font_describe(m_font);
    PangoFontDescription* otherDesc = pango_font_describe(other.m_font);
    bool result = pango_font_description_equal(thisDesc, otherDesc);
    pango_font_description_free(otherDesc);
    pango_font_description_free(thisDesc);
    return result;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return String();
}
#endif

}
