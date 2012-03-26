/*
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
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
#include "SVGTextLayoutAttributesBuilder.h"

#include "RenderSVGInlineText.h"
#include "RenderSVGText.h"
#include "SVGTextPositioningElement.h"

// Set to a value > 0 to dump the text layout attributes
#define DUMP_TEXT_LAYOUT_ATTRIBUTES 0

namespace WebCore {

SVGTextLayoutAttributesBuilder::SVGTextLayoutAttributesBuilder()
{
}

void SVGTextLayoutAttributesBuilder::buildLayoutAttributesForTextSubtree(RenderSVGText* textRoot)
{
    ASSERT(textRoot);

    // We always clear our current attribute as we don't want to keep any stale ones that could survive DOM modification.
    Vector<SVGTextLayoutAttributes>& allAttributes = textRoot->layoutAttributes();
    allAttributes.clear();

     // Build list of x/y/dx/dy/rotate values for each subtree element that may define these values (tspan/textPath etc).
    unsigned atCharacter = 0;
    UChar lastCharacter = '\0';
    collectTextPositioningElements(textRoot, atCharacter, lastCharacter);

    if (!atCharacter)
        return;

    // Collect x/y/dx/dy/rotate values for each character, stored in the m_positioningLists.xValues()/etc. lists.
    buildLayoutAttributesForAllCharacters(textRoot, atCharacter);

    // Propagate layout attributes to each RenderSVGInlineText object, and the whole list to the RenderSVGText root.
    atCharacter = 0;
    lastCharacter = '\0';
    propagateLayoutAttributes(textRoot, allAttributes, atCharacter, lastCharacter);
}

static inline void extractFloatValuesFromSVGLengthList(SVGElement* lengthContext, const SVGLengthList& list, Vector<float>& floatValues, unsigned textContentLength)
{
    ASSERT(lengthContext);

    unsigned length = list.size();
    if (length > textContentLength)
        length = textContentLength;
    floatValues.reserveCapacity(length);

    for (unsigned i = 0; i < length; ++i) {
        const SVGLength& length = list.at(i);
        floatValues.append(length.value(lengthContext));
    }
}

static inline void extractFloatValuesFromSVGNumberList(const SVGNumberList& list, Vector<float>& floatValues, unsigned textContentLength)
{
    unsigned length = list.size();
    if (length > textContentLength)
        length = textContentLength;
    floatValues.reserveCapacity(length);

    for (unsigned i = 0; i < length; ++i)
        floatValues.append(list.at(i));
}


static inline bool characterIsSpace(const UChar& character)
{
    return character == ' ';
}

static inline bool characterIsSpaceOrNull(const UChar& character)
{
    return character == ' ' || character == '\0';
}

static inline bool shouldPreserveAllWhiteSpace(RenderStyle* style)
{
    ASSERT(style);
    return style->whiteSpace() == PRE;
}
 
static inline void processRenderSVGInlineText(RenderSVGInlineText* text, unsigned& atCharacter, UChar& lastCharacter)
{
    if (shouldPreserveAllWhiteSpace(text->style())) {
        atCharacter += text->textLength();
        return;
    }

    const UChar* characters = text->characters();
    unsigned textLength = text->textLength();    
    for (unsigned textPosition = 0; textPosition < textLength; ++textPosition) {
        const UChar& currentCharacter = characters[textPosition];
        if (characterIsSpace(currentCharacter) && characterIsSpaceOrNull(lastCharacter))
            continue;

        lastCharacter = currentCharacter;
        ++atCharacter;
    }
}

void SVGTextLayoutAttributesBuilder::collectTextPositioningElements(RenderObject* start, unsigned& atCharacter, UChar& lastCharacter)
{
    ASSERT(!start->isSVGText() || m_textPositions.isEmpty());

    for (RenderObject* child = start->firstChild(); child; child = child->nextSibling()) { 
        if (child->isSVGInlineText()) {
            processRenderSVGInlineText(toRenderSVGInlineText(child), atCharacter, lastCharacter);
            continue;
        }

        if (!child->isSVGInline())
            continue;

        SVGTextPositioningElement* element = SVGTextPositioningElement::elementFromRenderer(child);
        unsigned atPosition = m_textPositions.size();
        if (element)
            m_textPositions.append(TextPosition(element, atCharacter));

        collectTextPositioningElements(child, atCharacter, lastCharacter);

        if (!element)
            continue;

        // Update text position, after we're back from recursion.
        TextPosition& position = m_textPositions[atPosition];
        ASSERT(!position.length);
        position.length = atCharacter - position.start;
    }
}

void SVGTextLayoutAttributesBuilder::buildLayoutAttributesForAllCharacters(RenderSVGText* textRoot, unsigned textLength)
{
    ASSERT(textLength);

    SVGTextPositioningElement* outermostTextElement = SVGTextPositioningElement::elementFromRenderer(textRoot);
    ASSERT(outermostTextElement);

    // Fill the lists with the special emptyValue marker.
    m_positioningLists.fillWithEmptyValues(textLength);

    // Grab outermost <text> element value lists and insert them in the m_positioningLists.
    TextPosition wholeTextPosition(outermostTextElement, 0, textLength);
    fillAttributesAtPosition(wholeTextPosition);

    // Handle x/y default attributes.
    float& xFirst = m_positioningLists.xValues.first();
    if (xFirst == SVGTextLayoutAttributes::emptyValue())
        xFirst = 0;

    float& yFirst = m_positioningLists.yValues.first();
    if (yFirst == SVGTextLayoutAttributes::emptyValue())
        yFirst = 0;

    // Fill m_positioningLists using child text positioning elements in top-down order. 
    unsigned size = m_textPositions.size();
    for (unsigned i = 0; i < size; ++i)
        fillAttributesAtPosition(m_textPositions[i]);

    // Now m_positioningLists.contains a x/y/dx/dy/rotate value for each character in the <text> subtree.
}

void SVGTextLayoutAttributesBuilder::propagateLayoutAttributes(RenderObject* start, Vector<SVGTextLayoutAttributes>& allAttributes, unsigned& atCharacter, UChar& lastCharacter) const
{
    for (RenderObject* child = start->firstChild(); child; child = child->nextSibling()) { 
        if (child->isSVGInlineText()) {
            RenderSVGInlineText* text = toRenderSVGInlineText(child);
            const UChar* characters = text->characters();
            unsigned textLength = text->textLength();
            bool preserveWhiteSpace = shouldPreserveAllWhiteSpace(text->style());

            SVGTextLayoutAttributes attributes(text);
            attributes.reserveCapacity(textLength);
    
            unsigned valueListPosition = atCharacter;
            unsigned metricsLength = 1;
            SVGTextMetrics lastMetrics = SVGTextMetrics::emptyMetrics();

            for (unsigned textPosition = 0; textPosition < textLength; textPosition += metricsLength) {
                const UChar& currentCharacter = characters[textPosition];

                SVGTextMetrics startToCurrentMetrics = SVGTextMetrics::measureCharacterRange(text, 0, textPosition + 1);
                SVGTextMetrics currentMetrics = SVGTextMetrics::measureCharacterRange(text, textPosition, 1);

                // Frequent case for Arabic text: when measuring a single character the arabic isolated form is taken
                // when rendering the glyph "in context" (with it's surrounding characters) it changes due to shaping.
                // So whenever runWidthAdvance != currentMetrics.width(), we are processing a text run whose length is
                // not equal to the sum of the individual lengths of the glyphs, when measuring them isolated.
                float runWidthAdvance = startToCurrentMetrics.width() - lastMetrics.width();
                if (runWidthAdvance != currentMetrics.width())
                    currentMetrics.setWidth(runWidthAdvance);

                lastMetrics = startToCurrentMetrics;
                metricsLength = currentMetrics.length();

                if (!preserveWhiteSpace && characterIsSpace(currentCharacter) && characterIsSpaceOrNull(lastCharacter)) {
                    attributes.positioningLists().appendEmptyValues();
                    attributes.textMetricsValues().append(SVGTextMetrics::emptyMetrics());
                    continue;
                }

                SVGTextLayoutAttributes::PositioningLists& positioningLists = attributes.positioningLists();
                positioningLists.appendValuesFromPosition(m_positioningLists, valueListPosition);
                attributes.textMetricsValues().append(currentMetrics);

                // Pad x/y/dx/dy/rotate value lists with empty values, if the metrics span more than one character.
                if (metricsLength > 1) {
                    for (unsigned i = 0; i < metricsLength - 1; ++i)
                        positioningLists.appendEmptyValues();
                }

                lastCharacter = currentCharacter;
                valueListPosition += metricsLength;
            }

#if DUMP_TEXT_LAYOUT_ATTRIBUTES > 0
            fprintf(stderr, "\nDumping layout attributes for RenderSVGInlineText, renderer=%p, node=%p (atCharacter: %i)\n", text, text->node(), atCharacter);
            fprintf(stderr, "BiDi properties: unicode-bidi=%i, block direction=%i\n", text->style()->unicodeBidi(), text->style()->direction());
            attributes.dump();
#endif

            text->storeLayoutAttributes(attributes);
            allAttributes.append(attributes);
            atCharacter = valueListPosition;
            continue;
        }

        if (!child->isSVGInline())
            continue;

        propagateLayoutAttributes(child, allAttributes, atCharacter, lastCharacter);
    }
}

static inline void fillListAtPosition(Vector<float>& allValues, Vector<float>& values, unsigned start)
{
    unsigned valuesSize = values.size();
    for (unsigned i = 0; i < valuesSize; ++i)
        allValues[start + i] = values[i];
}

void SVGTextLayoutAttributesBuilder::fillAttributesAtPosition(const TextPosition& position)
{
    Vector<float> values;
    extractFloatValuesFromSVGLengthList(position.element, position.element->x(), values, position.length);
    fillListAtPosition(m_positioningLists.xValues, values, position.start);

    values.clear();
    extractFloatValuesFromSVGLengthList(position.element, position.element->y(), values, position.length);
    fillListAtPosition(m_positioningLists.yValues, values, position.start);

    values.clear();
    extractFloatValuesFromSVGLengthList(position.element, position.element->dx(), values, position.length);
    fillListAtPosition(m_positioningLists.dxValues, values, position.start);

    values.clear();
    extractFloatValuesFromSVGLengthList(position.element, position.element->dy(), values, position.length);
    fillListAtPosition(m_positioningLists.dyValues, values, position.start);

    values.clear();
    extractFloatValuesFromSVGNumberList(position.element->rotate(), values, position.length);
    fillListAtPosition(m_positioningLists.rotateValues, values, position.start);

    // The last rotation value always spans the whole scope.
    if (values.isEmpty())
        return;

    float lastValue = values.last();
    for (unsigned i = values.size(); i < position.length; ++i)
        m_positioningLists.rotateValues[position.start + i] = lastValue;
}

}

#endif // ENABLE(SVG)
