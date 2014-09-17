/**
 * Copyright (C) 2003, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Torch Mobile, Inc.
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
#include "Font.h"

#include "FloatRect.h"
#include "FontCache.h"
#include "FontFallbackList.h"
#include "GlyphBuffer.h"
#include "GlyphPageTreeNode.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include "WidthIterator.h"
#include <wtf/MathExtras.h>
#include <wtf/unicode/CharacterNames.h>
#include <wtf/unicode/Unicode.h>

using namespace WTF;
using namespace Unicode;

namespace WebCore {

GlyphData Font::glyphDataForCharacter(UChar32 c, bool mirror, FontDataVariant variant) const
{
    ASSERT(isMainThread());

    if (variant == AutoVariant) {
        if (m_fontDescription.smallCaps()) {
            UChar32 upperC = toUpper(c);
            if (upperC != c) {
                c = upperC;
                variant = SmallCapsVariant;
            } else
                variant = NormalVariant;
        } else
            variant = NormalVariant;
    }

    if (mirror)
        c = mirroredChar(c);

    unsigned pageNumber = (c / GlyphPage::size);

    GlyphPageTreeNode* node = pageNumber ? m_fontList->m_pages.get(pageNumber) : m_fontList->m_pageZero;
    if (!node) {
        node = GlyphPageTreeNode::getRootChild(fontDataAt(0), pageNumber);
        if (pageNumber)
            m_fontList->m_pages.set(pageNumber, node);
        else
            m_fontList->m_pageZero = node;
    }

    GlyphPage* page;
    if (variant == NormalVariant) {
        // Fastest loop, for the common case (normal variant).
        while (true) {
            page = node->page();
            if (page) {
                GlyphData data = page->glyphDataForCharacter(c);
                if (data.fontData && (data.fontData->platformData().orientation() == Horizontal || data.fontData->isTextOrientationFallback()))
                    return data;
                
                if (data.fontData) {
                    if (isCJKIdeographOrSymbol(c)) {
                        if (!data.fontData->hasVerticalGlyphs()) {
                            // Use the broken ideograph font data. The broken ideograph font will use the horizontal width of glyphs
                            // to make sure you get a square (even for broken glyphs like symbols used for punctuation).
                            const SimpleFontData* brokenIdeographFontData = data.fontData->brokenIdeographFontData();
                            GlyphPageTreeNode* brokenIdeographNode = GlyphPageTreeNode::getRootChild(brokenIdeographFontData, pageNumber);
                            const GlyphPage* brokenIdeographPage = brokenIdeographNode->page();
                            if (brokenIdeographPage) {
                                GlyphData brokenIdeographData = brokenIdeographPage->glyphDataForCharacter(c);
                                if (brokenIdeographData.fontData)
                                    return brokenIdeographData;
                            }
                            
                            // Shouldn't be possible to even reach this point.
                            ASSERT_NOT_REACHED();
                        }
                    } else {
                        if (m_fontDescription.textOrientation() == TextOrientationVerticalRight) {
                            const SimpleFontData* verticalRightFontData = data.fontData->verticalRightOrientationFontData();
                            GlyphPageTreeNode* verticalRightNode = GlyphPageTreeNode::getRootChild(verticalRightFontData, pageNumber);
                            const GlyphPage* verticalRightPage = verticalRightNode->page();
                            if (verticalRightPage) {
                                GlyphData verticalRightData = verticalRightPage->glyphDataForCharacter(c);
                                // If the glyphs are distinct, we will make the assumption that the font has a vertical-right glyph baked
                                // into it.
                                if (data.glyph != verticalRightData.glyph)
                                    return data;
                                // The glyphs are identical, meaning that we should just use the horizontal glyph.
                                if (verticalRightData.fontData)
                                    return verticalRightData;
                            }
                        } else if (m_fontDescription.textOrientation() == TextOrientationUpright) {
                            const SimpleFontData* uprightFontData = data.fontData->uprightOrientationFontData();
                            GlyphPageTreeNode* uprightNode = GlyphPageTreeNode::getRootChild(uprightFontData, pageNumber);
                            const GlyphPage* uprightPage = uprightNode->page();
                            if (uprightPage) {
                                GlyphData uprightData = uprightPage->glyphDataForCharacter(c);
                                // If the glyphs are the same, then we know we can just use the horizontal glyph rotated vertically to be upright.
                                if (data.glyph == uprightData.glyph)
                                    return data;
                                // The glyphs are distinct, meaning that the font has a vertical-right glyph baked into it. We can't use that
                                // glyph, so we fall back to the upright data and use the horizontal glyph.
                                if (uprightData.fontData)
                                    return uprightData;
                            }
                        }

                        // Shouldn't be possible to even reach this point.
                        ASSERT_NOT_REACHED();
                    }
                    return data;
                }

                if (node->isSystemFallback())
                    break;
            }

            // Proceed with the fallback list.
            node = node->getChild(fontDataAt(node->level()), pageNumber);
            if (pageNumber)
                m_fontList->m_pages.set(pageNumber, node);
            else
                m_fontList->m_pageZero = node;
        }
    } else {
        while (true) {
            page = node->page();
            if (page) {
                GlyphData data = page->glyphDataForCharacter(c);
                if (data.fontData) {
                    // The variantFontData function should not normally return 0.
                    // But if it does, we will just render the capital letter big.
                    const SimpleFontData* variantFontData = data.fontData->variantFontData(m_fontDescription, variant);
                    if (!variantFontData)
                        return data;

                    GlyphPageTreeNode* variantNode = GlyphPageTreeNode::getRootChild(variantFontData, pageNumber);
                    const GlyphPage* variantPage = variantNode->page();
                    if (variantPage) {
                        GlyphData data = variantPage->glyphDataForCharacter(c);
                        if (data.fontData)
                            return data;
                    }

                    // Do not attempt system fallback off the variantFontData. This is the very unlikely case that
                    // a font has the lowercase character but the small caps font does not have its uppercase version.
                    return variantFontData->missingGlyphData();
                }

                if (node->isSystemFallback())
                    break;
            }

            // Proceed with the fallback list.
            node = node->getChild(fontDataAt(node->level()), pageNumber);
            if (pageNumber)
                m_fontList->m_pages.set(pageNumber, node);
            else
                m_fontList->m_pageZero = node;
        }
    }

    ASSERT(page);
    ASSERT(node->isSystemFallback());

    // System fallback is character-dependent. When we get here, we
    // know that the character in question isn't in the system fallback
    // font's glyph page. Try to lazily create it here.
    UChar codeUnits[2];
    int codeUnitsLength;
    if (c <= 0xFFFF) {
        codeUnits[0] = Font::normalizeSpaces(c);
        codeUnitsLength = 1;
    } else {
        codeUnits[0] = U16_LEAD(c);
        codeUnits[1] = U16_TRAIL(c);
        codeUnitsLength = 2;
    }
    const SimpleFontData* characterFontData = fontCache()->getFontDataForCharacters(*this, codeUnits, codeUnitsLength);
    if (variant != NormalVariant && characterFontData)
        characterFontData = characterFontData->variantFontData(m_fontDescription, variant);
    if (characterFontData) {
        // Got the fallback glyph and font.
        GlyphPage* fallbackPage = GlyphPageTreeNode::getRootChild(characterFontData, pageNumber)->page();
        GlyphData data = fallbackPage && fallbackPage->fontDataForCharacter(c) ? fallbackPage->glyphDataForCharacter(c) : characterFontData->missingGlyphData();
        // Cache it so we don't have to do system fallback again next time.
        if (variant == NormalVariant) {
#if OS(WINCE)
            // missingGlyphData returns a null character, which is not suitable for GDI to display.
            // Also, sometimes we cannot map a font for the character on WINCE, but GDI can still
            // display the character, probably because the font package is not installed correctly.
            // So we just always set the glyph to be same as the character, and let GDI solve it.
            page->setGlyphDataForCharacter(c, c, characterFontData);
            return page->glyphDataForCharacter(c);
#else
            page->setGlyphDataForCharacter(c, data.glyph, data.fontData);
#endif
        }
        return data;
    }

    // Even system fallback can fail; use the missing glyph in that case.
    // FIXME: It would be nicer to use the missing glyph from the last resort font instead.
    GlyphData data = primaryFont()->missingGlyphData();
    if (variant == NormalVariant) {
#if OS(WINCE)
        // See comment about WINCE GDI handling near setGlyphDataForCharacter above.
        page->setGlyphDataForCharacter(c, c, data.fontData);
        return page->glyphDataForCharacter(c);
#else
        page->setGlyphDataForCharacter(c, data.glyph, data.fontData);
#endif
    }
    return data;
}

bool Font::primaryFontHasGlyphForCharacter(UChar32 character) const
{
    unsigned pageNumber = (character / GlyphPage::size);

    GlyphPageTreeNode* node = GlyphPageTreeNode::getRootChild(primaryFont(), pageNumber);
    GlyphPage* page = node->page();

    return page && page->fontDataForCharacter(character);
}

// FIXME: This function may not work if the emphasis mark uses a complex script, but none of the
// standard emphasis marks do so.
bool Font::getEmphasisMarkGlyphData(const AtomicString& mark, GlyphData& glyphData) const
{
    if (mark.isEmpty())
        return false;

#if ENABLE(SVG_FONTS)
    // FIXME: Implement for SVG fonts.
    if (primaryFont()->isSVGFont())
        return false;
#endif

    UChar32 character = mark[0];

    if (U16_IS_SURROGATE(character)) {
        if (!U16_IS_SURROGATE_LEAD(character))
            return false;

        if (mark.length() < 2)
            return false;

        UChar low = mark[1];
        if (!U16_IS_TRAIL(low))
            return false;

        character = U16_GET_SUPPLEMENTARY(character, low);
    }

    glyphData = glyphDataForCharacter(character, false, EmphasisMarkVariant);
    return true;
}

int Font::emphasisMarkAscent(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const SimpleFontData* markFontData = markGlyphData.fontData;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().ascent();
}

int Font::emphasisMarkDescent(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const SimpleFontData* markFontData = markGlyphData.fontData;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().descent();
}

int Font::emphasisMarkHeight(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const SimpleFontData* markFontData = markGlyphData.fontData;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().height();
}

float Font::getGlyphsAndAdvancesForSimpleText(const TextRun& run, int from, int to, GlyphBuffer& glyphBuffer, ForTextEmphasisOrNot forTextEmphasis) const
{
    float initialAdvance;

    WidthIterator it(this, run, 0, false, forTextEmphasis);
    it.advance(from);
    float beforeWidth = it.m_runWidthSoFar;
    it.advance(to, &glyphBuffer);

    if (glyphBuffer.isEmpty())
        return 0;

    float afterWidth = it.m_runWidthSoFar;

    if (run.rtl()) {
        it.advance(run.length());
        initialAdvance = it.m_runWidthSoFar - afterWidth;
    } else
        initialAdvance = beforeWidth;

    if (run.rtl()) {
        for (int i = 0, end = glyphBuffer.size() - 1; i < glyphBuffer.size() / 2; ++i, --end)
            glyphBuffer.swap(i, end);
    }

    return initialAdvance;
}

void Font::drawSimpleText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    // This glyph buffer holds our glyphs+advances+font data for each glyph.
    GlyphBuffer glyphBuffer;

    float startX = point.x() + getGlyphsAndAdvancesForSimpleText(run, from, to, glyphBuffer);

    if (glyphBuffer.isEmpty())
        return;

    FloatPoint startPoint(startX, point.y());
    drawGlyphBuffer(context, glyphBuffer, startPoint);
}

void Font::drawEmphasisMarksForSimpleText(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    GlyphBuffer glyphBuffer;
    float initialAdvance = getGlyphsAndAdvancesForSimpleText(run, from, to, glyphBuffer, ForTextEmphasis);

    if (glyphBuffer.isEmpty())
        return;

    drawEmphasisMarks(context, glyphBuffer, mark, FloatPoint(point.x() + initialAdvance, point.y()));
}

void Font::drawGlyphBuffer(GraphicsContext* context, const GlyphBuffer& glyphBuffer, const FloatPoint& point) const
{   
    // Draw each contiguous run of glyphs that use the same font data.
    const SimpleFontData* fontData = glyphBuffer.fontDataAt(0);
    FloatSize offset = glyphBuffer.offsetAt(0);
    FloatPoint startPoint(point);
    float nextX = startPoint.x();
    int lastFrom = 0;
    int nextGlyph = 0;
    while (nextGlyph < glyphBuffer.size()) {
        const SimpleFontData* nextFontData = glyphBuffer.fontDataAt(nextGlyph);
        FloatSize nextOffset = glyphBuffer.offsetAt(nextGlyph);
        if (nextFontData != fontData || nextOffset != offset) {
            drawGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);

            lastFrom = nextGlyph;
            fontData = nextFontData;
            offset = nextOffset;
            startPoint.setX(nextX);
        }
        nextX += glyphBuffer.advanceAt(nextGlyph);
        nextGlyph++;
    }

    drawGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);
}

inline static float offsetToMiddleOfGlyph(const SimpleFontData* fontData, Glyph glyph)
{
    if (fontData->platformData().orientation() == Horizontal) {
        FloatRect bounds = fontData->boundsForGlyph(glyph);
        return bounds.x() + bounds.width() / 2;
    }
    // FIXME: Use glyph bounds once they make sense for vertical fonts.
    return fontData->widthForGlyph(glyph) / 2;
}

inline static float offsetToMiddleOfGlyphAtIndex(const GlyphBuffer& glyphBuffer, size_t i)
{
    return offsetToMiddleOfGlyph(glyphBuffer.fontDataAt(i), glyphBuffer.glyphAt(i));
}

void Font::drawEmphasisMarks(GraphicsContext* context, const GlyphBuffer& glyphBuffer, const AtomicString& mark, const FloatPoint& point) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return;

    const SimpleFontData* markFontData = markGlyphData.fontData;
    ASSERT(markFontData);
    if (!markFontData)
        return;

    Glyph markGlyph = markGlyphData.glyph;
    Glyph spaceGlyph = markFontData->spaceGlyph();

    float middleOfLastGlyph = offsetToMiddleOfGlyphAtIndex(glyphBuffer, 0);
    FloatPoint startPoint(point.x() + middleOfLastGlyph - offsetToMiddleOfGlyph(markFontData, markGlyph), point.y());

    GlyphBuffer markBuffer;
    for (int i = 0; i + 1 < glyphBuffer.size(); ++i) {
        float middleOfNextGlyph = offsetToMiddleOfGlyphAtIndex(glyphBuffer, i + 1);
        float advance = glyphBuffer.advanceAt(i) - middleOfLastGlyph + middleOfNextGlyph;
        markBuffer.add(glyphBuffer.glyphAt(i) ? markGlyph : spaceGlyph, markFontData, advance);
        middleOfLastGlyph = middleOfNextGlyph;
    }
    markBuffer.add(glyphBuffer.glyphAt(glyphBuffer.size() - 1) ? markGlyph : spaceGlyph, markFontData, 0);

    drawGlyphBuffer(context, markBuffer, startPoint);
}

float Font::floatWidthForSimpleText(const TextRun& run, GlyphBuffer* glyphBuffer, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    WidthIterator it(this, run, fallbackFonts, glyphOverflow);
    it.advance(run.length(), glyphBuffer);

    if (glyphOverflow) {
        glyphOverflow->top = max<int>(glyphOverflow->top, ceilf(-it.minGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().ascent()));
        glyphOverflow->bottom = max<int>(glyphOverflow->bottom, ceilf(it.maxGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().descent()));
        glyphOverflow->left = ceilf(it.firstGlyphOverflow());
        glyphOverflow->right = ceilf(it.lastGlyphOverflow());
    }

    return it.m_runWidthSoFar;
}

FloatRect Font::selectionRectForSimpleText(const TextRun& run, const FloatPoint& point, int h, int from, int to) const
{
    WidthIterator it(this, run);
    it.advance(from);
    float beforeWidth = it.m_runWidthSoFar;
    it.advance(to);
    float afterWidth = it.m_runWidthSoFar;

    // Using roundf() rather than ceilf() for the right edge as a compromise to ensure correct caret positioning.
    if (run.rtl()) {
        it.advance(run.length());
        float totalWidth = it.m_runWidthSoFar;
        return FloatRect(floorf(point.x() + totalWidth - afterWidth), point.y(), roundf(point.x() + totalWidth - beforeWidth) - floorf(point.x() + totalWidth - afterWidth), h);
    }

    return FloatRect(floorf(point.x() + beforeWidth), point.y(), roundf(point.x() + afterWidth) - floorf(point.x() + beforeWidth), h);
}

int Font::offsetForPositionForSimpleText(const TextRun& run, float x, bool includePartialGlyphs) const
{
    float delta = x;

    WidthIterator it(this, run);
    GlyphBuffer localGlyphBuffer;
    unsigned offset;
    if (run.rtl()) {
        delta -= floatWidthForSimpleText(run, 0);
        while (1) {
            offset = it.m_currentCharacter;
            float w;
            if (!it.advanceOneCharacter(w, &localGlyphBuffer))
                break;
            delta += w;
            if (includePartialGlyphs) {
                if (delta - w / 2 >= 0)
                    break;
            } else {
                if (delta >= 0)
                    break;
            }
        }
    } else {
        while (1) {
            offset = it.m_currentCharacter;
            float w;
            if (!it.advanceOneCharacter(w, &localGlyphBuffer))
                break;
            delta -= w;
            if (includePartialGlyphs) {
                if (delta + w / 2 <= 0)
                    break;
            } else {
                if (delta <= 0)
                    break;
            }
        }
    }

    return offset;
}

}
