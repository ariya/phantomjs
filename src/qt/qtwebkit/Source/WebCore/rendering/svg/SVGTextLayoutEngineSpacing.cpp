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
#include "SVGTextLayoutEngineSpacing.h"

#include "Font.h"
#include "SVGLengthContext.h"
#include "SVGRenderStyle.h"

#if ENABLE(SVG_FONTS)
#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#endif

namespace WebCore {

SVGTextLayoutEngineSpacing::SVGTextLayoutEngineSpacing(const Font& font)
    : m_font(font)
    , m_lastCharacter(0)
{
}

float SVGTextLayoutEngineSpacing::calculateSVGKerning(bool isVerticalText, const SVGTextMetrics::Glyph& currentGlyph)
{
#if ENABLE(SVG_FONTS)
    const SimpleFontData* fontData = m_font.primaryFont();
    if (!fontData->isSVGFont()) {
        m_lastGlyph.isValid = false;
        return 0;
    }

    ASSERT(fontData->isCustomFont());
    ASSERT(fontData->isSVGFont());

    const SVGFontData* svgFontData = static_cast<const SVGFontData*>(fontData->fontData());
    SVGFontFaceElement* svgFontFace = svgFontData->svgFontFaceElement();
    ASSERT(svgFontFace);

    SVGFontElement* svgFont = svgFontFace->associatedFontElement();
    if (!svgFont) {
        m_lastGlyph.isValid = false;
        return 0;
    }

    float kerning = 0;
    if (m_lastGlyph.isValid) {
        if (isVerticalText)
            kerning = svgFont->verticalKerningForPairOfStringsAndGlyphs(m_lastGlyph.unicodeString, m_lastGlyph.name, currentGlyph.unicodeString, currentGlyph.name);
        else
            kerning = svgFont->horizontalKerningForPairOfStringsAndGlyphs(m_lastGlyph.unicodeString, m_lastGlyph.name, currentGlyph.unicodeString, currentGlyph.name);
    }

    m_lastGlyph = currentGlyph;
    m_lastGlyph.isValid = true;
    kerning *= m_font.size() / m_font.fontMetrics().unitsPerEm();
    return kerning;
#else
    UNUSED_PARAM(isVerticalText);
    UNUSED_PARAM(currentGlyph);
    return false;
#endif
}

float SVGTextLayoutEngineSpacing::calculateCSSKerningAndSpacing(const SVGRenderStyle* style, SVGElement* contextElement, const UChar* currentCharacter)
{
    float kerning = 0;
    SVGLength kerningLength = style->kerning();
    if (kerningLength.unitType() == LengthTypePercentage)
        kerning = kerningLength.valueAsPercentage() * m_font.pixelSize();
    else {
        SVGLengthContext lengthContext(contextElement);
        kerning = kerningLength.value(lengthContext);
    }

    const UChar* lastCharacter = m_lastCharacter;
    m_lastCharacter = currentCharacter;

    if (!kerning && !m_font.letterSpacing() && !m_font.wordSpacing())
        return 0;

    float spacing = m_font.letterSpacing() + kerning;
    if (currentCharacter && lastCharacter && m_font.wordSpacing()) {
        if (Font::treatAsSpace(*currentCharacter) && !Font::treatAsSpace(*lastCharacter))
            spacing += m_font.wordSpacing();
    }

    return spacing;
}

}

#endif // ENABLE(SVG)
