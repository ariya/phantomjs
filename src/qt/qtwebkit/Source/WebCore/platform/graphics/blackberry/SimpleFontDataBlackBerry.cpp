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
#include "SimpleFontData.h"

#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "ITypeUtils.h"

#include <fs_api.h>
#include <unicode/normlzr.h>

namespace WebCore {

static inline float FSFixedToFloat(FS_FIXED n) { return n / 65536.0; }

#define openTypeTag(a, b, c, d) (FS_ULONG)((a << 24) | (b << 16) | (c << 8) | d)

void SimpleFontData::platformInit()
{
    FS_FIXED ascender = 0;
    FS_FIXED descender = 0;
    FS_FIXED leading = 0;
    FsAscDescLeadSource source;
    FS_LONG result;
    if (m_platformData.size() > 0) {
        // FIXME: hack! FS_get_ascender_descender_leading() returns ERR_NO_CURRENT_SFNT when size is 0, even though we called FS_set_scale and m_platformData.font()->cur_sfnt is not 0
        result = FS_get_ascender_descender_leading(m_platformData.font(), &ascender, &descender, &leading, &source);
        ASSERT_UNUSED(result, result == SUCCESS);
    }

    m_fontMetrics.setAscent(FS_ROUND(ascender));
    m_fontMetrics.setDescent(FS_ROUND(descender));
    m_fontMetrics.setLineGap(iTypeFixedToFloat(leading));
    m_fontMetrics.setLineSpacing(lroundf(m_fontMetrics.ascent()) + lroundf(m_fontMetrics.descent()) + lroundf(m_fontMetrics.lineGap()));

    FONT_METRICS metrics;
    result = FS_font_metrics(m_platformData.font(), &metrics);
    ASSERT_UNUSED(result, result == SUCCESS);

    // m_fontMetrics.setUnitsPerEm(FS_get_design_units(m_platformData().font()));
    m_fontMetrics.setUnitsPerEm(metrics.unitsPerEm);

    FS_USHORT xRange = metrics.font_bbox.xMax - metrics.font_bbox.xMin;
    m_maxCharWidth = roundf((xRange * roundf(m_platformData.size())) / metrics.unitsPerEm);

    TTF_OS2 os2;
    if (FS_get_table_structure(m_platformData.font(), TAG_OS2, &os2) == SUCCESS && os2.sxHeight && os2.xAvgCharWidth) {
        FS_USHORT yppem = m_platformData.font()->lpm;
        m_fontMetrics.setXHeight(static_cast<float>(os2.sxHeight) * yppem / metrics.unitsPerEm);
        m_avgCharWidth = static_cast<float>(os2.xAvgCharWidth) * yppem / metrics.unitsPerEm;
    } else {
        // HACK
        m_fontMetrics.setXHeight(m_fontMetrics.ascent() * 0.56);
        m_fontMetrics.setHasXHeight(false);
        m_avgCharWidth = m_fontMetrics.xHeight();

        GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();

        if (glyphPageZero) {
            static const UChar32 xChar = 'x';
            const Glyph xGlyph = glyphPageZero->glyphDataForCharacter(xChar).glyph;

            if (xGlyph) {
                // In widthForGlyph(), xGlyph will be compared with
                // m_zeroWidthSpaceGlyph, which isn't initialized yet here.
                // Initialize it with zero to make sure widthForGlyph() returns
                // the right width.
                m_zeroWidthSpaceGlyph = 0;
                m_avgCharWidth = widthForGlyph(xGlyph);
            }
        }
    }

    if (m_platformData.orientation() == Vertical && !isTextOrientationFallback())
        m_hasVerticalGlyphs = FS_get_table(m_platformData.font(), openTypeTag('v', 'h', 'e', 'a'), TBL_QUERY, 0)
            || FS_get_table(m_platformData.font(), openTypeTag('V', 'O', 'R', 'G'), TBL_QUERY, 0);
}

void SimpleFontData::platformCharWidthInit()
{
}

void SimpleFontData::platformDestroy()
{
}

PassRefPtr<SimpleFontData> SimpleFontData::platformCreateScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    const float scaledSize = lroundf(fontDescription.computedSize() * scaleFactor);
    return adoptRef(new SimpleFontData(
        FontPlatformData(m_platformData.font()->cur_lfnt->name,
            scaledSize,
            m_platformData.syntheticBold(),
            m_platformData.syntheticOblique(),
            m_platformData.orientation(),
            m_platformData.widthVariant()),
        isCustomFont(), false));
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    int position = 0;

    while (position < length) {
        // FIXME: use shaper?
        UChar32 character;
        int nextPosition = position;
        U16_NEXT(characters, nextPosition, length, character);

        FS_USHORT glyph = FS_map_char(m_platformData.font(), static_cast<FS_ULONG>(character));
        if (!glyph)
            return false;

        position = nextPosition;
    }

    return true;
}

void SimpleFontData::determinePitch()
{
    m_treatAsFixedPitch = m_platformData.isFixedPitch();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    FS_GLYPHMAP* glyphmap = FS_get_glyphmap(m_platformData.font(), glyph, FS_MAP_DISTANCEFIELD | FS_MAP_GRAYMAP8);
    if (!glyphmap)
        return FloatRect();

    FloatRect bounds(glyphmap->lo_x, glyphmap->height - glyphmap->hi_y, glyphmap->width, glyphmap->height);
    FS_free_char(m_platformData.font(), glyphmap);

    return bounds;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    FS_SHORT idx, idy;
    FS_FIXED dx, dy;
    FS_FIXED s00, s01, s10, s11;
    bool needsFakeBoldReset = m_platformData.syntheticBold() && m_treatAsFixedPitch;

    if (needsFakeBoldReset) {
        FS_get_scale(m_platformData.font(), &s00, &s01, &s10, &s11);
        FS_set_bold_pct(m_platformData.font(), 0);
        FS_set_scale(m_platformData.font(), s00, s01, s10, s11);
    }

    if (FS_get_advance(m_platformData.font(), glyph, FS_MAP_DISTANCEFIELD | FS_MAP_GRAYMAP8, &idx, &idy, &dx, &dy) != SUCCESS)
        dx = 0;

    if (needsFakeBoldReset) {
        FS_set_bold_pct(m_platformData.font(), ITYPEFAKEBOLDAMOUNT);
        FS_set_scale(m_platformData.font(), s00, s01, s10, s11);
    }

    return iTypeFixedToFloat(dx);
}

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
    if (U_FAILURE(error))
        return false;

    int position = 0;
    while (position < normalizedLength) {
        UChar32 character;
        int nextPosition = position;
        U16_NEXT(normalizedCharacters, nextPosition, normalizedLength, character);

        if (!u_hasBinaryProperty(character, UCHAR_DEFAULT_IGNORABLE_CODE_POINT)) {
            FS_USHORT glyph = FS_map_char(m_platformData.font(), static_cast<FS_ULONG>(character));
            if (!glyph)
                return false;
        }

        position = nextPosition;
    }

    addResult.iterator->value = true;
    return true;
}

} // namespace WebCore
