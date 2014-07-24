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

namespace WebCore {

SVGTextLayoutAttributesBuilder::SVGTextLayoutAttributesBuilder()
    : m_textLength(0)
{
}

void SVGTextLayoutAttributesBuilder::buildLayoutAttributesForTextRenderer(RenderSVGInlineText* text)
{
    ASSERT(text);

    RenderSVGText* textRoot = RenderSVGText::locateRenderSVGTextAncestor(text);
    if (!textRoot)
        return;

    if (m_textPositions.isEmpty()) {
        m_characterDataMap.clear();

        m_textLength = 0;
        const UChar* lastCharacter = 0;
        collectTextPositioningElements(textRoot, lastCharacter);

        if (!m_textLength)
            return;

        buildCharacterDataMap(textRoot);
    }

    m_metricsBuilder.buildMetricsAndLayoutAttributes(textRoot, text, m_characterDataMap);
}

bool SVGTextLayoutAttributesBuilder::buildLayoutAttributesForForSubtree(RenderSVGText* textRoot)
{
    ASSERT(textRoot);

    m_characterDataMap.clear();

    if (m_textPositions.isEmpty()) {
        m_textLength = 0;
        const UChar* lastCharacter = 0;
        collectTextPositioningElements(textRoot, lastCharacter);
    }

    if (!m_textLength)
        return false;

    buildCharacterDataMap(textRoot);
    m_metricsBuilder.buildMetricsAndLayoutAttributes(textRoot, 0, m_characterDataMap);
    return true;
}

void SVGTextLayoutAttributesBuilder::rebuildMetricsForTextRenderer(RenderSVGInlineText* text)
{
    ASSERT(text);
    m_metricsBuilder.measureTextRenderer(text);
}

static inline void processRenderSVGInlineText(RenderSVGInlineText* text, unsigned& atCharacter, const UChar*& lastCharacter)
{
    if (text->style()->whiteSpace() == PRE) {
        atCharacter += text->textLength();
        return;
    }

    const UChar* characters = text->characters();
    unsigned textLength = text->textLength();    
    for (unsigned textPosition = 0; textPosition < textLength; ++textPosition) {
        const UChar* currentCharacter = characters + textPosition;
        if (*currentCharacter == ' ' && (!lastCharacter || *lastCharacter == ' '))
            continue;

        lastCharacter = currentCharacter;
        ++atCharacter;
    }
}

void SVGTextLayoutAttributesBuilder::collectTextPositioningElements(RenderObject* start, const UChar*& lastCharacter)
{
    ASSERT(!start->isSVGText() || m_textPositions.isEmpty());

    for (RenderObject* child = start->firstChild(); child; child = child->nextSibling()) { 
        if (child->isSVGInlineText()) {
            processRenderSVGInlineText(toRenderSVGInlineText(child), m_textLength, lastCharacter);
            continue;
        }

        if (!child->isSVGInline())
            continue;

        SVGTextPositioningElement* element = SVGTextPositioningElement::elementFromRenderer(child);
        unsigned atPosition = m_textPositions.size();
        if (element)
            m_textPositions.append(TextPosition(element, m_textLength));

        collectTextPositioningElements(child, lastCharacter);

        if (!element)
            continue;

        // Update text position, after we're back from recursion.
        TextPosition& position = m_textPositions[atPosition];
        ASSERT(!position.length);
        position.length = m_textLength - position.start;
    }
}

void SVGTextLayoutAttributesBuilder::buildCharacterDataMap(RenderSVGText* textRoot)
{
    SVGTextPositioningElement* outermostTextElement = SVGTextPositioningElement::elementFromRenderer(textRoot);
    ASSERT(outermostTextElement);

    // Grab outermost <text> element value lists and insert them in the character data map.
    TextPosition wholeTextPosition(outermostTextElement, 0, m_textLength);
    fillCharacterDataMap(wholeTextPosition);

    // Handle x/y default attributes.
    SVGCharacterDataMap::iterator it = m_characterDataMap.find(1);
    if (it == m_characterDataMap.end()) {
        SVGCharacterData data;
        data.x = 0;
        data.y = 0;
        m_characterDataMap.set(1, data);
    } else {
        SVGCharacterData& data = it->value;
        if (data.x == SVGTextLayoutAttributes::emptyValue())
            data.x = 0;
        if (data.y == SVGTextLayoutAttributes::emptyValue())
            data.y = 0;
    }

    // Fill character data map using child text positioning elements in top-down order. 
    unsigned size = m_textPositions.size();
    for (unsigned i = 0; i < size; ++i)
        fillCharacterDataMap(m_textPositions[i]);
}

static inline void updateCharacterData(unsigned i, float& lastRotation, SVGCharacterData& data, const SVGLengthContext& lengthContext, const SVGLengthList* xList, const SVGLengthList* yList, const SVGLengthList* dxList, const SVGLengthList* dyList, const SVGNumberList* rotateList)
{
    if (xList)
        data.x = xList->at(i).value(lengthContext);
    if (yList)
        data.y = yList->at(i).value(lengthContext);
    if (dxList)
        data.dx = dxList->at(i).value(lengthContext);
    if (dyList)
        data.dy = dyList->at(i).value(lengthContext);
    if (rotateList) {
        data.rotate = rotateList->at(i);
        lastRotation = data.rotate;
    }
}

void SVGTextLayoutAttributesBuilder::fillCharacterDataMap(const TextPosition& position)
{
    const SVGLengthList& xList = position.element->x();
    const SVGLengthList& yList = position.element->y();
    const SVGLengthList& dxList = position.element->dx();
    const SVGLengthList& dyList = position.element->dy();
    const SVGNumberList& rotateList = position.element->rotate();

    unsigned xListSize = xList.size();
    unsigned yListSize = yList.size();
    unsigned dxListSize = dxList.size();
    unsigned dyListSize = dyList.size();
    unsigned rotateListSize = rotateList.size();
    if (!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize)
        return;

    float lastRotation = SVGTextLayoutAttributes::emptyValue();
    SVGLengthContext lengthContext(position.element);
    for (unsigned i = 0; i < position.length; ++i) {
        const SVGLengthList* xListPtr = i < xListSize ? &xList : 0;
        const SVGLengthList* yListPtr = i < yListSize ? &yList : 0;
        const SVGLengthList* dxListPtr = i < dxListSize ? &dxList : 0;
        const SVGLengthList* dyListPtr = i < dyListSize ? &dyList : 0;
        const SVGNumberList* rotateListPtr = i < rotateListSize ? &rotateList : 0;
        if (!xListPtr && !yListPtr && !dxListPtr && !dyListPtr && !rotateListPtr)
            break;

        SVGCharacterDataMap::iterator it = m_characterDataMap.find(position.start + i + 1);
        if (it == m_characterDataMap.end()) {
            SVGCharacterData data;
            updateCharacterData(i, lastRotation, data, lengthContext, xListPtr, yListPtr, dxListPtr, dyListPtr, rotateListPtr);
            m_characterDataMap.set(position.start + i + 1, data);
            continue;
        }

        updateCharacterData(i, lastRotation, it->value, lengthContext, xListPtr, yListPtr, dxListPtr, dyListPtr, rotateListPtr);
    }

    // The last rotation value always spans the whole scope.
    if (lastRotation == SVGTextLayoutAttributes::emptyValue())
        return;

    for (unsigned i = rotateList.size(); i < position.length; ++i) {
        SVGCharacterDataMap::iterator it = m_characterDataMap.find(position.start + i + 1);
        if (it == m_characterDataMap.end()) {
            SVGCharacterData data;
            data.rotate = lastRotation;
            m_characterDataMap.set(position.start + i + 1, data);
            continue;
        }

        it->value.rotate = lastRotation;
    }
}

}

#endif // ENABLE(SVG)
