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
#include "FontPlatformData.h"

#include "HarfBuzzFace.h"
#include "ITypeUtils.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <fs_api.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

FontPlatformData::FontPlatformData(FILECHAR* name, float size, bool syntheticBold, bool syntheticOblique, FontOrientation orientation, FontWidthVariant widthVariant)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_orientation(orientation)
    , m_size(size)
    , m_widthVariant(widthVariant)
    , m_font(0)
    , m_name(fastStrDup(name))
    , m_scaledFont(0)
    , m_harfBuzzFace()
    , m_isColorBitmapFont(false)
{
    ASSERT(name);
    m_font = FS_new_client(BlackBerry::Platform::Graphics::getIType(), 0);
    ASSERT(m_font);
    applyState(m_font);
}

FontPlatformData::~FontPlatformData()
{
    if (m_scaledFont) {
        BlackBerry::Platform::Graphics::clearGlyphCacheForFont(m_scaledFont);
        FS_end_client(m_scaledFont);
    }
    if (m_font && m_font != hashTableDeletedFontValue())
        FS_end_client(m_font);
    fastFree(m_name);
}

const char* FontPlatformData::name() const
{
    return m_name;
}

bool FontPlatformData::applyState(FS_STATE* font, float scale) const
{
    ASSERT(font);
    ASSERT(m_name);
    if (FS_set_font(font, m_name) != SUCCESS)
        return false;

    if (FS_set_cmap(font, 3, 10) != SUCCESS) // First try Windows Unicode with surrogates...
        if (FS_set_cmap(font, 3, 1) != SUCCESS) // try normal Windows Unicode
            if (FS_set_cmap(font, 1, 0) != SUCCESS)
                return false;

    if (m_syntheticBold) {
        if (FS_set_bold_pct(font, ITYPEFAKEBOLDAMOUNT) != SUCCESS)
            return false;
        FS_set_flags(font, FLAGS_CHECK_CONTOUR_WINDING_ON); // we need correctly-wound contours to fake bold
    } else {
        if (FS_set_bold_pct(font, 0) != SUCCESS)
            return false;
        FS_set_flags(font, FLAGS_CHECK_CONTOUR_WINDING_OFF);
    }

    FS_FIXED skew = 0;
    if (m_syntheticOblique)
        skew = 13930; // 12 degrees

    FS_FIXED fixedScale = std::min(FixMul(floatToITypeFixed(scale), floatToITypeFixed(m_size)), MAXITYPEFONTSCALE);
    if (FS_set_scale(font, fixedScale, FixMul(fixedScale, skew), 0, fixedScale) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_CMAP_OFF) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_HINTS_OFF) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_DEFAULT_CSM_OFF) != SUCCESS)
        return false;

    if (m_orientation == Vertical) {
        if (FS_set_flags(font, FLAGS_VERTICAL_ON) != SUCCESS)
            return false;
    }
    return true;
}

void FontPlatformData::platformDataInit(const FontPlatformData& source)
{
    m_harfBuzzFace = source.m_harfBuzzFace;
    m_scaledFont = 0;
    if (source.m_font && source.m_font != hashTableDeletedFontValue()) {
        m_font = FS_new_client(source.m_font, 0);
        m_name = fastStrDup(source.name());
        bool ret = applyState(m_font);
        ASSERT_UNUSED(ret, ret);
    } else
        m_font = source.m_font;
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& other)
{
    m_harfBuzzFace = other.m_harfBuzzFace;
    m_scaledFont = 0;
    if (other.m_font && other.m_font != hashTableDeletedFontValue()) {
        m_font = FS_new_client(other.m_font, 0);
        fastFree(m_name);
        m_name = fastStrDup(other.name());
        bool ret = applyState(m_font);
        ASSERT_UNUSED(ret, ret);
    } else
        m_font = other.m_font;

    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    if (m_font == other.m_font)
        return true;

    if (!m_font || m_font == hashTableDeletedFontValue() || !other.m_font || other.m_font == hashTableDeletedFontValue())
        return false;

    return m_font->cur_sfnt == other.m_font->cur_sfnt;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return "iType Font Data";
}
#endif

HarfBuzzFace* FontPlatformData::harfBuzzFace()
{
    if (!m_harfBuzzFace) {
        uint64_t uniqueID = reinterpret_cast<uintptr_t>(m_font);
        m_harfBuzzFace = HarfBuzzFace::create(const_cast<FontPlatformData*>(this), uniqueID);
    }

    return m_harfBuzzFace.get();
}

FS_STATE* FontPlatformData::scaledFont(float scale) const
{
    ASSERT(scale > 0.0);
    if (m_scale == scale && m_scaledFont)
        return m_scaledFont;

    // This must be an SVG Font. It seems like there is an upstream bug where web and SVG fonts
    // are used in one TextRun. Other ports seem to be able to deal with having an SVG Font in
    // this path so bail out instead of crash.
    if (!m_font)
        return 0;

    if (m_scaledFont) {
        BlackBerry::Platform::Graphics::clearGlyphCacheForFont(m_scaledFont);
        if (FS_end_client(m_scaledFont) != SUCCESS)
            CRASH();
    }

    m_scaledFont = FS_new_client(m_font, 0);
    if (!applyState(m_scaledFont, scale)) {
        if (FS_end_client(m_scaledFont) != SUCCESS)
            CRASH();
        m_scaledFont = 0;
        return 0;
    }

    m_scale = scale;

    return m_scaledFont;
}

void FontPlatformData::setFakeBold(bool fakeBold)
{
    m_syntheticBold = fakeBold;
    applyState(m_font);
}

void FontPlatformData::setFakeItalic(bool fakeItalic)
{
    m_syntheticOblique = fakeItalic;
    applyState(m_font);
}

const void* FontPlatformData::platformFontHandle() const
{
    ASSERT(m_font);
    return m_font->cur_sfnt;
}

bool FontPlatformData::isFixedPitch() const
{
    FS_BYTE* postTable;
    FS_ULONG length;
    if (m_font && (postTable = FS_get_table(m_font, TAG_post, TBL_EXTRACT, &length))) {
        bool fixed = reinterpret_cast<TTF_POST*>(postTable)->isFixedPitch;
        FS_free_table(m_font, postTable);
        return fixed;
    }
    return false;
}

}
