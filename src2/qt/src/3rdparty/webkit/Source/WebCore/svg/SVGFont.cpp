/*
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#if ENABLE(SVG_FONTS)
#include "Font.h"

#include "CSSFontSelector.h"
#include "GraphicsContext.h"
#include "RenderObject.h"
#include "RenderSVGInlineText.h"
#include "RenderSVGResourceSolidColor.h"
#include "SVGAltGlyphElement.h"
#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SVGGlyphElement.h"
#include "SVGGlyphMap.h"
#include "SVGMissingGlyphElement.h"
#include "SVGNames.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include "XMLNames.h"

using namespace WTF::Unicode;

namespace WebCore {

static inline float convertEmUnitToPixel(float fontSize, float unitsPerEm, float value)
{
    if (!unitsPerEm)
        return 0.0f;

    return value * fontSize / unitsPerEm;
}

static inline bool isVerticalWritingMode(const SVGRenderStyle* style)
{
    return style->writingMode() == WM_TBRL || style->writingMode() == WM_TB; 
}

static inline const SVGFontData* svgFontAndFontFaceElementForFontData(const SimpleFontData* fontData, SVGFontFaceElement*& fontFace, SVGFontElement*& font)
{
    ASSERT(fontData->isCustomFont());
    ASSERT(fontData->isSVGFont());

    const SVGFontData* svgFontData = static_cast<const SVGFontData*>(fontData->svgFontData());

    fontFace = svgFontData->svgFontFaceElement();
    ASSERT(fontFace);

    font = fontFace->associatedFontElement();
    return svgFontData;
}

// Helper class to walk a text run. Lookup a SVGGlyph for each character
// - also respecting possibly defined ligatures - and invoke a callback for each found glyph.
template<typename SVGTextRunData>
struct SVGTextRunWalker {
    typedef bool (*SVGTextRunWalkerCallback)(const SVGGlyph&, SVGTextRunData&);
    typedef void (*SVGTextRunWalkerMissingGlyphCallback)(const TextRun&, SVGTextRunData&);

    SVGTextRunWalker(const SVGFontData* fontData, SVGFontElement* fontElement, SVGTextRunData& data,
                     SVGTextRunWalkerCallback callback, SVGTextRunWalkerMissingGlyphCallback missingGlyphCallback)
        : m_fontData(fontData)
        , m_fontElement(fontElement)
        , m_walkerData(data)
        , m_walkerCallback(callback)
        , m_walkerMissingGlyphCallback(missingGlyphCallback)
    {
    }

    void walk(const TextRun& run, bool isVerticalText, const String& language, int from, int to)
    {
        if (from < 0 || to < 0 || from > to || from >= run.length() || to > run.length())
            return;

        const String text = Font::normalizeSpaces(run.data(from), to - from);
        Vector<SVGGlyph::ArabicForm> chars(charactersWithArabicForm(text, run.rtl()));

        SVGGlyph identifier;
        bool foundGlyph = false;
        int characterLookupRange;
        int endOfScanRange = to + m_walkerData.extraCharsAvailable;

        bool haveAltGlyph = false;
        SVGGlyph altGlyphIdentifier;
        if (RenderObject* renderObject = run.referencingRenderObject()) {
            RenderObject* parentRenderer = renderObject->parent();
            ASSERT(parentRenderer);
            if (parentRenderer->node() && parentRenderer->node()->hasTagName(SVGNames::altGlyphTag)) {
                SVGGlyphElement* glyphElement = static_cast<SVGAltGlyphElement*>(parentRenderer->node())->glyphElement();
                if (glyphElement) {
                    haveAltGlyph = true;
                    altGlyphIdentifier = glyphElement->buildGlyphIdentifier();
                    altGlyphIdentifier.isValid = true;
                    altGlyphIdentifier.unicodeStringLength = to - from;
                }
            }
        }

        for (int i = from; i < to; ++i) {
            // If characterLookupRange is > 0, then the font defined ligatures (length of unicode property value > 1).
            // We have to check wheter the current character & the next character define a ligature. This needs to be
            // extended to the n-th next character (where n is 'characterLookupRange'), to check for any possible ligature.
            characterLookupRange = endOfScanRange - i;

            String lookupString = Font::normalizeSpaces(run.data(i), characterLookupRange);

            Vector<SVGGlyph> glyphs;
            if (haveAltGlyph)
                glyphs.append(altGlyphIdentifier);
            else
                m_fontElement->getGlyphIdentifiersForString(lookupString, glyphs);

            Vector<SVGGlyph>::iterator it = glyphs.begin();
            Vector<SVGGlyph>::iterator end = glyphs.end();
            
            for (; it != end; ++it) {
                identifier = *it;
                if (identifier.isValid && isCompatibleGlyph(identifier, isVerticalText, language, chars, i, i + identifier.unicodeStringLength)) {
                    ASSERT(characterLookupRange > 0);
                    i += identifier.unicodeStringLength - 1;
                    m_walkerData.charsConsumed += identifier.unicodeStringLength;
                    m_walkerData.glyphName = identifier.glyphName;

                    foundGlyph = true;
                    SVGGlyphElement::inheritUnspecifiedAttributes(identifier, m_fontData);
                    break;
                }
            }

            if (!foundGlyph) {
                ++m_walkerData.charsConsumed;
                if (SVGMissingGlyphElement* element = m_fontElement->firstMissingGlyphElement()) {
                    // <missing-glyph> element support
                    identifier = SVGGlyphElement::buildGenericGlyphIdentifier(element);
                    SVGGlyphElement::inheritUnspecifiedAttributes(identifier, m_fontData);
                    identifier.isValid = true;
                } else {
                    // Fallback to system font fallback
                    TextRun subRun(run);
                    subRun.setText(subRun.data(i), 1);

                    (*m_walkerMissingGlyphCallback)(subRun, m_walkerData);
                    continue;
                }
            }

            if (!(*m_walkerCallback)(identifier, m_walkerData))
                break;

            foundGlyph = false;
        }
    }

private:
    const SVGFontData* m_fontData;
    SVGFontElement* m_fontElement;
    SVGTextRunData& m_walkerData;
    SVGTextRunWalkerCallback m_walkerCallback;
    SVGTextRunWalkerMissingGlyphCallback m_walkerMissingGlyphCallback;
};

// Callback & data structures to compute the width of text using SVG Fonts
struct SVGTextRunWalkerMeasuredLengthData {
    int at;
    int from;
    int to;
    int extraCharsAvailable;
    int charsConsumed;
    String glyphName;

    float scale;
    float length;
    const Font* font;
};

static bool floatWidthUsingSVGFontCallback(const SVGGlyph& identifier, SVGTextRunWalkerMeasuredLengthData& data)
{
    if (data.at >= data.from && data.at < data.to)
        data.length += identifier.horizontalAdvanceX * data.scale;

    data.at++;
    return data.at < data.to;
}

static void floatWidthMissingGlyphCallback(const TextRun& run, SVGTextRunWalkerMeasuredLengthData& data)
{
    // Handle system font fallback
    FontDescription fontDescription(data.font->fontDescription());
    fontDescription.setFamily(FontFamily());
    Font font(fontDescription, 0, 0); // spacing handled by SVG text code.
    font.update(data.font->fontSelector());

    data.length += font.width(run);
}


SVGFontElement* Font::svgFont() const
{ 
    if (!isSVGFont())
        return 0;

    SVGFontElement* fontElement = 0;
    SVGFontFaceElement* fontFaceElement = 0;
    if (svgFontAndFontFaceElementForFontData(primaryFont(), fontFaceElement, fontElement))
        return fontElement;
    
    return 0;
}

static float floatWidthOfSubStringUsingSVGFont(const Font* font, const TextRun& run, int extraCharsAvailable, int from, int to, int& charsConsumed, String& glyphName)
{
    int newFrom = to > from ? from : to;
    int newTo = to > from ? to : from;

    from = newFrom;
    to = newTo;

    SVGFontElement* fontElement = 0;
    SVGFontFaceElement* fontFaceElement = 0;

    if (const SVGFontData* fontData = svgFontAndFontFaceElementForFontData(font->primaryFont(), fontFaceElement, fontElement)) {
        if (!fontElement)
            return 0.0f;

        SVGTextRunWalkerMeasuredLengthData data;

        data.font = font;
        data.at = from;
        data.from = from;
        data.to = to;
        data.extraCharsAvailable = extraCharsAvailable;
        data.charsConsumed = 0;
        data.scale = convertEmUnitToPixel(font->size(), fontFaceElement->unitsPerEm(), 1.0f);
        data.length = 0.0f;

        String language;
        bool isVerticalText = false; // Holds true for HTML text

        // TODO: language matching & svg glyphs should be possible for HTML text, too.
        if (RenderObject* renderObject = run.referencingRenderObject()) {
            RenderObject* parentRenderer = renderObject->parent();
            ASSERT(parentRenderer);
            isVerticalText = isVerticalWritingMode(parentRenderer->style()->svgStyle());

            if (SVGElement* element = static_cast<SVGElement*>(parentRenderer->node()))
                language = element->getAttribute(XMLNames::langAttr);
        }

        SVGTextRunWalker<SVGTextRunWalkerMeasuredLengthData> runWalker(fontData, fontElement, data, floatWidthUsingSVGFontCallback, floatWidthMissingGlyphCallback);
        runWalker.walk(run, isVerticalText, language, from, to);
        charsConsumed = data.charsConsumed;
        glyphName = data.glyphName;
        return data.length;
    }

    return 0.0f;
}

float Font::floatWidthUsingSVGFont(const TextRun& run) const
{
    int charsConsumed;
    String glyphName;
    return floatWidthOfSubStringUsingSVGFont(this, run, 0, 0, run.length(), charsConsumed, glyphName);
}

float Font::floatWidthUsingSVGFont(const TextRun& run, int extraCharsAvailable, int& charsConsumed, String& glyphName) const
{
    return floatWidthOfSubStringUsingSVGFont(this, run, extraCharsAvailable, 0, run.length(), charsConsumed, glyphName);
}

// Callback & data structures to draw text using SVG Fonts
struct SVGTextRunWalkerDrawTextData {
    int extraCharsAvailable;
    int charsConsumed;
    String glyphName;
    Vector<SVGGlyph> glyphIdentifiers;
    Vector<UChar> fallbackCharacters;
};

static bool drawTextUsingSVGFontCallback(const SVGGlyph& identifier, SVGTextRunWalkerDrawTextData& data)
{
    data.glyphIdentifiers.append(identifier);
    return true;
}

static void drawTextMissingGlyphCallback(const TextRun& run, SVGTextRunWalkerDrawTextData& data)
{
    ASSERT(run.length() == 1);
    data.glyphIdentifiers.append(SVGGlyph());
    data.fallbackCharacters.append(run[0]);
}

void Font::drawTextUsingSVGFont(GraphicsContext* context, const TextRun& run, 
                                const FloatPoint& point, int from, int to) const
{
    SVGFontElement* fontElement = 0;
    SVGFontFaceElement* fontFaceElement = 0;

    if (const SVGFontData* fontData = svgFontAndFontFaceElementForFontData(primaryFont(), fontFaceElement, fontElement)) {
        if (!fontElement)
            return;

        SVGTextRunWalkerDrawTextData data;
        FloatPoint currentPoint = point;
        float scale = convertEmUnitToPixel(size(), fontFaceElement->unitsPerEm(), 1.0f);

        RenderSVGResource* activePaintingResource = run.activePaintingResource();

        // If renderObject is not set, we're dealing for HTML text rendered using SVG Fonts.
        if (!run.referencingRenderObject()) {
            ASSERT(!activePaintingResource);

            // TODO: We're only supporting simple filled HTML text so far.
            RenderSVGResourceSolidColor* solidPaintingResource = RenderSVGResource::sharedSolidPaintingResource();
            solidPaintingResource->setColor(context->fillColor());

            activePaintingResource = solidPaintingResource;
        }

        ASSERT(activePaintingResource);

        int charsConsumed;
        String glyphName;
        bool isVerticalText = false;
        float xStartOffset = floatWidthOfSubStringUsingSVGFont(this, run, 0, run.rtl() ? to : 0, run.rtl() ? run.length() : from, charsConsumed, glyphName);
        FloatPoint glyphOrigin;

        String language;

        // TODO: language matching & svg glyphs should be possible for HTML text, too.
        RenderObject* referencingRenderObject = run.referencingRenderObject();
        RenderObject* referencingRenderObjectParent = referencingRenderObject ? referencingRenderObject->parent() : 0;
        RenderStyle* referencingRenderObjectParentStyle = 0;
        if (referencingRenderObject) {
            ASSERT(referencingRenderObjectParent);
            referencingRenderObjectParentStyle = referencingRenderObjectParent->style();

            isVerticalText = isVerticalWritingMode(referencingRenderObjectParentStyle->svgStyle());    
            if (SVGElement* element = static_cast<SVGElement*>(referencingRenderObjectParent->node()))
                language = element->getAttribute(XMLNames::langAttr);
        }

        if (!isVerticalText) {
            glyphOrigin.setX(fontData->horizontalOriginX() * scale);
            glyphOrigin.setY(fontData->horizontalOriginY() * scale);
        }

        data.extraCharsAvailable = 0;
        data.charsConsumed = 0;

        SVGTextRunWalker<SVGTextRunWalkerDrawTextData> runWalker(fontData, fontElement, data, drawTextUsingSVGFontCallback, drawTextMissingGlyphCallback);
        runWalker.walk(run, isVerticalText, language, from, to);

        RenderSVGResourceMode resourceMode = context->textDrawingMode() == TextModeStroke ? ApplyToStrokeMode : ApplyToFillMode;

        unsigned numGlyphs = data.glyphIdentifiers.size();
        unsigned fallbackCharacterIndex = 0;
        for (unsigned i = 0; i < numGlyphs; ++i) {
            const SVGGlyph& identifier = data.glyphIdentifiers[run.rtl() ? numGlyphs - i - 1 : i];
            if (identifier.isValid) {
                // FIXME: Support arbitary SVG content as glyph (currently limited to <glyph d="..."> situations).
                if (!identifier.pathData.isEmpty()) {
                    GraphicsContextStateSaver stateSaver(*context);

                    if (isVerticalText) {
                        glyphOrigin.setX(identifier.verticalOriginX * scale);
                        glyphOrigin.setY(identifier.verticalOriginY * scale);
                    }

                    AffineTransform glyphPathTransform;
                    glyphPathTransform.translate(xStartOffset + currentPoint.x() + glyphOrigin.x(), currentPoint.y() + glyphOrigin.y());
                    glyphPathTransform.scale(scale, -scale);

                    Path glyphPath = identifier.pathData;
                    glyphPath.transform(glyphPathTransform);

                    if (activePaintingResource->applyResource(referencingRenderObjectParent, referencingRenderObjectParentStyle, context, resourceMode)) {
                        if (referencingRenderObject) {
                            RenderSVGInlineText* textRenderer = toRenderSVGInlineText(referencingRenderObject);
                            context->setStrokeThickness(context->strokeThickness() * textRenderer->scalingFactor());
                        }
                        activePaintingResource->postApplyResource(referencingRenderObjectParent, context, resourceMode, &glyphPath);
                    }
                }

                if (isVerticalText)
                    currentPoint.move(0.0f, identifier.verticalAdvanceY * scale);
                else
                    currentPoint.move(identifier.horizontalAdvanceX * scale, 0.0f);
            } else {
                // Handle system font fallback
                FontDescription fontDescription(m_fontDescription);
                fontDescription.setFamily(FontFamily());
                Font font(fontDescription, 0, 0); // spacing handled by SVG text code.
                font.update(fontSelector());

                TextRun fallbackCharacterRun(run);
                fallbackCharacterRun.setText(&data.fallbackCharacters[run.rtl() ? data.fallbackCharacters.size() - fallbackCharacterIndex - 1 : fallbackCharacterIndex], 1);
                font.drawText(context, fallbackCharacterRun, currentPoint);

                if (isVerticalText)
                    currentPoint.move(0.0f, font.width(fallbackCharacterRun));
                else
                    currentPoint.move(font.width(fallbackCharacterRun), 0.0f);

                fallbackCharacterIndex++;
            }
        }
    }
}

FloatRect Font::selectionRectForTextUsingSVGFont(const TextRun& run, const FloatPoint& point, int height, int from, int to) const
{
    int charsConsumed;
    String glyphName;

    return FloatRect(point.x() + floatWidthOfSubStringUsingSVGFont(this, run, 0, run.rtl() ? to : 0, run.rtl() ? run.length() : from, charsConsumed, glyphName),
                     point.y(), floatWidthOfSubStringUsingSVGFont(this, run, 0, from, to, charsConsumed, glyphName), height);
}

int Font::offsetForPositionForTextUsingSVGFont(const TextRun&, float, bool) const
{
    // TODO: Fix text selection when HTML text is drawn using a SVG Font
    // We need to integrate the SVG text selection code in the offsetForPosition() framework.
    // This will also fix a major issue, that SVG Text code can't select arabic strings properly.
    return 0;
}

}

#endif
