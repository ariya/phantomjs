/*
 * Copyright (C) 2003, 2006, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "WidthIterator.h"

#include "Font.h"
#include "GlyphBuffer.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include <wtf/MathExtras.h>

#if USE(ICU_UNICODE)
#include <unicode/unorm.h>
#endif

using namespace WTF;
using namespace Unicode;
using namespace std;

namespace WebCore {

// According to http://www.unicode.org/Public/UNIDATA/UCD.html#Canonical_Combining_Class_Values
static const uint8_t hiraganaKatakanaVoicingMarksCombiningClass = 8;

WidthIterator::WidthIterator(const Font* font, const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, bool accountForGlyphBounds, bool forTextEmphasis)
    : m_font(font)
    , m_run(run)
    , m_end(run.length())
    , m_currentCharacter(0)
    , m_runWidthSoFar(0)
    , m_isAfterExpansion(!run.allowsLeadingExpansion())
    , m_fallbackFonts(fallbackFonts)
    , m_accountForGlyphBounds(accountForGlyphBounds)
    , m_maxGlyphBoundingBoxY(numeric_limits<float>::min())
    , m_minGlyphBoundingBoxY(numeric_limits<float>::max())
    , m_firstGlyphOverflow(0)
    , m_lastGlyphOverflow(0)
    , m_forTextEmphasis(forTextEmphasis)
{
    // If the padding is non-zero, count the number of spaces in the run
    // and divide that by the padding for per space addition.
    m_expansion = m_run.expansion();
    if (!m_expansion)
        m_expansionPerOpportunity = 0;
    else {
        bool isAfterExpansion = m_isAfterExpansion;
        unsigned expansionOpportunityCount = Font::expansionOpportunityCount(m_run.characters(), m_end, m_run.ltr() ? LTR : RTL, isAfterExpansion);
        if (isAfterExpansion && !m_run.allowsTrailingExpansion())
            expansionOpportunityCount--;

        if (!expansionOpportunityCount)
            m_expansionPerOpportunity = 0;
        else
            m_expansionPerOpportunity = m_expansion / expansionOpportunityCount;
    }
}

void WidthIterator::advance(int offset, GlyphBuffer* glyphBuffer)
{
    if (offset > m_end)
        offset = m_end;

    int currentCharacter = m_currentCharacter;
    if (currentCharacter >= offset)
        return;

    const UChar* cp = m_run.data(currentCharacter);

    bool rtl = m_run.rtl();
    bool hasExtraSpacing = (m_font->letterSpacing() || m_font->wordSpacing() || m_expansion) && !m_run.spacingDisabled();

    FloatRect bounds;

    const SimpleFontData* primaryFont = m_font->primaryFont();
    const SimpleFontData* lastFontData = primaryFont;

    while (currentCharacter < offset) {
        UChar32 c = *cp;
        unsigned clusterLength = 1;
        if (c >= 0x3041) {
            if (c <= 0x30FE) {
                // Deal with Hiragana and Katakana voiced and semi-voiced syllables.
                // Normalize into composed form, and then look for glyph with base + combined mark.
                // Check above for character range to minimize performance impact.
                UChar32 normalized = normalizeVoicingMarks(currentCharacter);
                if (normalized) {
                    c = normalized;
                    clusterLength = 2;
                }
            } else if (U16_IS_SURROGATE(c)) {
                if (!U16_IS_SURROGATE_LEAD(c))
                    break;

                // Do we have a surrogate pair?  If so, determine the full Unicode (32 bit)
                // code point before glyph lookup.
                // Make sure we have another character and it's a low surrogate.
                if (currentCharacter + 1 >= m_run.length())
                    break;
                UChar low = cp[1];
                if (!U16_IS_TRAIL(low))
                    break;
                c = U16_GET_SUPPLEMENTARY(c, low);
                clusterLength = 2;
            }
        }

        const GlyphData& glyphData = m_font->glyphDataForCharacter(c, rtl);
        Glyph glyph = glyphData.glyph;
        const SimpleFontData* fontData = glyphData.fontData;

        ASSERT(fontData);

        // Now that we have a glyph and font data, get its width.
        float width;
        if (c == '\t' && m_run.allowTabs()) {
            float tabWidth = m_font->tabWidth(*fontData);
            width = tabWidth - fmodf(m_run.xPos() + m_runWidthSoFar, tabWidth);
        } else {
            width = fontData->widthForGlyph(glyph);

#if ENABLE(SVG)
            // SVG uses horizontalGlyphStretch(), when textLength is used to stretch/squeeze text.
            width *= m_run.horizontalGlyphStretch();
#endif
        }

        if (fontData != lastFontData && width) {
            lastFontData = fontData;
            if (m_fallbackFonts && fontData != primaryFont) {
                // FIXME: This does a little extra work that could be avoided if
                // glyphDataForCharacter() returned whether it chose to use a small caps font.
                if (!m_font->isSmallCaps() || c == toUpper(c))
                    m_fallbackFonts->add(fontData);
                else {
                    const GlyphData& uppercaseGlyphData = m_font->glyphDataForCharacter(toUpper(c), rtl);
                    if (uppercaseGlyphData.fontData != primaryFont)
                        m_fallbackFonts->add(uppercaseGlyphData.fontData);
                }
            }
        }

        if (hasExtraSpacing) {
            // Account for letter-spacing.
            if (width && m_font->letterSpacing())
                width += m_font->letterSpacing();

            static bool expandAroundIdeographs = Font::canExpandAroundIdeographsInComplexText();
            bool treatAsSpace = Font::treatAsSpace(c);
            if (treatAsSpace || (expandAroundIdeographs && Font::isCJKIdeographOrSymbol(c))) {
                // Distribute the run's total expansion evenly over all expansion opportunities in the run.
                if (m_expansion) {
                    if (!treatAsSpace && !m_isAfterExpansion) {
                        // Take the expansion opportunity before this ideograph.
                        m_expansion -= m_expansionPerOpportunity;
                        m_runWidthSoFar += m_expansionPerOpportunity;
                        if (glyphBuffer) {
                            if (glyphBuffer->isEmpty())
                                glyphBuffer->add(fontData->spaceGlyph(), fontData, m_expansionPerOpportunity);
                            else
                                glyphBuffer->expandLastAdvance(m_expansionPerOpportunity);
                        }
                    }
                    if (m_run.allowsTrailingExpansion() || (m_run.ltr() && currentCharacter + clusterLength < static_cast<size_t>(m_run.length()))
                        || (m_run.rtl() && currentCharacter)) {
                        m_expansion -= m_expansionPerOpportunity;
                        width += m_expansionPerOpportunity;
                        m_isAfterExpansion = true;
                    }
                } else
                    m_isAfterExpansion = false;

                // Account for word spacing.
                // We apply additional space between "words" by adding width to the space character.
                if (treatAsSpace && currentCharacter && !Font::treatAsSpace(cp[-1]) && m_font->wordSpacing())
                    width += m_font->wordSpacing();
            } else
                m_isAfterExpansion = false;
        }

        if (m_accountForGlyphBounds) {
            bounds = fontData->boundsForGlyph(glyph);
            if (!currentCharacter)
                m_firstGlyphOverflow = max<float>(0, -bounds.x());
        }

        if (m_forTextEmphasis && !Font::canReceiveTextEmphasis(c))
            glyph = 0;

        // Advance past the character we just dealt with.
        cp += clusterLength;
        currentCharacter += clusterLength;

        m_runWidthSoFar += width;

        if (glyphBuffer)
            glyphBuffer->add(glyph, fontData, width);

        if (m_accountForGlyphBounds) {
            m_maxGlyphBoundingBoxY = max(m_maxGlyphBoundingBoxY, bounds.maxY());
            m_minGlyphBoundingBoxY = min(m_minGlyphBoundingBoxY, bounds.y());
            m_lastGlyphOverflow = max<float>(0, bounds.maxX() - width);
        }
    }

    m_currentCharacter = currentCharacter;
}

bool WidthIterator::advanceOneCharacter(float& width, GlyphBuffer* glyphBuffer)
{
    int oldSize = glyphBuffer->size();
    advance(m_currentCharacter + 1, glyphBuffer);
    float w = 0;
    for (int i = oldSize; i < glyphBuffer->size(); ++i)
        w += glyphBuffer->advanceAt(i);
    width = w;
    return glyphBuffer->size() > oldSize;
}

UChar32 WidthIterator::normalizeVoicingMarks(int currentCharacter)
{
    if (currentCharacter + 1 < m_end) {
        if (combiningClass(m_run[currentCharacter + 1]) == hiraganaKatakanaVoicingMarksCombiningClass) {
#if USE(ICU_UNICODE)
            // Normalize into composed form using 3.2 rules.
            UChar normalizedCharacters[2] = { 0, 0 };
            UErrorCode uStatus = U_ZERO_ERROR;  
            int32_t resultLength = unorm_normalize(m_run.data(currentCharacter), 2,
                UNORM_NFC, UNORM_UNICODE_3_2, &normalizedCharacters[0], 2, &uStatus);
            if (resultLength == 1 && uStatus == 0)
                return normalizedCharacters[0];
#elif USE(QT4_UNICODE)
            QString tmp(reinterpret_cast<const QChar*>(m_run.data(currentCharacter)), 2);
            QString res = tmp.normalized(QString::NormalizationForm_C, QChar::Unicode_3_2);
            if (res.length() == 1)
                return res.at(0).unicode();
#endif
        }
    }
    return 0;
}

}
