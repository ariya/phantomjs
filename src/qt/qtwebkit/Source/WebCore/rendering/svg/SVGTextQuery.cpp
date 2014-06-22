/*
 * Copyright (C) Research In Motion Limited 2010-2012. All rights reserved.
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
#include "SVGTextQuery.h"

#if ENABLE(SVG)
#include "FloatConversion.h"
#include "InlineFlowBox.h"
#include "RenderBlock.h"
#include "RenderInline.h"
#include "RenderSVGInlineText.h"
#include "SVGInlineTextBox.h"
#include "SVGTextMetrics.h"
#include "VisiblePosition.h"

#include <wtf/MathExtras.h>

namespace WebCore {

// Base structure for callback user data
struct SVGTextQuery::Data {
    Data()
        : isVerticalText(false)
        , processedCharacters(0)
        , textRenderer(0)
        , textBox(0)
    {
    }

    bool isVerticalText;
    unsigned processedCharacters;
    RenderSVGInlineText* textRenderer;
    const SVGInlineTextBox* textBox;
};

static inline InlineFlowBox* flowBoxForRenderer(RenderObject* renderer)
{
    if (!renderer)
        return 0;

    if (renderer->isRenderBlock()) {
        // If we're given a block element, it has to be a RenderSVGText.
        ASSERT(renderer->isSVGText());
        RenderBlock* renderBlock = toRenderBlock(renderer);

        // RenderSVGText only ever contains a single line box.
        InlineFlowBox* flowBox = renderBlock->firstLineBox();
        ASSERT(flowBox == renderBlock->lastLineBox());
        return flowBox;
    }

    if (renderer->isRenderInline()) {
        // We're given a RenderSVGInline or objects that derive from it (RenderSVGTSpan / RenderSVGTextPath)
        RenderInline* renderInline = toRenderInline(renderer);

        // RenderSVGInline only ever contains a single line box.
        InlineFlowBox* flowBox = renderInline->firstLineBox();
        ASSERT(flowBox == renderInline->lastLineBox());
        return flowBox;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

SVGTextQuery::SVGTextQuery(RenderObject* renderer)
{
    collectTextBoxesInFlowBox(flowBoxForRenderer(renderer));
}

void SVGTextQuery::collectTextBoxesInFlowBox(InlineFlowBox* flowBox)
{
    if (!flowBox)
        return;

    for (InlineBox* child = flowBox->firstChild(); child; child = child->nextOnLine()) {
        if (child->isInlineFlowBox()) {
            // Skip generated content.
            if (!child->renderer()->node())
                continue;

            collectTextBoxesInFlowBox(static_cast<InlineFlowBox*>(child));
            continue;
        }

        if (child->isSVGInlineTextBox())
            m_textBoxes.append(toSVGInlineTextBox(child));
    }
}

bool SVGTextQuery::executeQuery(Data* queryData, ProcessTextFragmentCallback fragmentCallback) const
{
    ASSERT(!m_textBoxes.isEmpty());

    unsigned processedCharacters = 0;
    unsigned textBoxCount = m_textBoxes.size();

    // Loop over all text boxes
    for (unsigned textBoxPosition = 0; textBoxPosition < textBoxCount; ++textBoxPosition) {
        queryData->textBox = m_textBoxes.at(textBoxPosition);
        queryData->textRenderer = toRenderSVGInlineText(queryData->textBox->textRenderer());
        ASSERT(queryData->textRenderer);
        ASSERT(queryData->textRenderer->style());
        ASSERT(queryData->textRenderer->style()->svgStyle());

        queryData->isVerticalText = queryData->textRenderer->style()->svgStyle()->isVerticalWritingMode();
        const Vector<SVGTextFragment>& fragments = queryData->textBox->textFragments();
    
        // Loop over all text fragments in this text box, firing a callback for each.
        unsigned fragmentCount = fragments.size();
        for (unsigned i = 0; i < fragmentCount; ++i) {
            const SVGTextFragment& fragment = fragments.at(i);
            if ((this->*fragmentCallback)(queryData, fragment))
                return true;

            processedCharacters += fragment.length;
        }

        queryData->processedCharacters = processedCharacters;
    }

    return false;
}

bool SVGTextQuery::mapStartEndPositionsIntoFragmentCoordinates(Data* queryData, const SVGTextFragment& fragment, int& startPosition, int& endPosition) const
{
    // Reuse the same logic used for text selection & painting, to map our query start/length into start/endPositions of the current text fragment.
    startPosition -= queryData->processedCharacters;
    endPosition -= queryData->processedCharacters;

    if (startPosition >= endPosition || startPosition < 0 || endPosition < 0)
        return false;

    modifyStartEndPositionsRespectingLigatures(queryData, startPosition, endPosition);
    if (!queryData->textBox->mapStartEndPositionsIntoFragmentCoordinates(fragment, startPosition, endPosition))
        return false;

    ASSERT(startPosition < endPosition);
    return true;
}

void SVGTextQuery::modifyStartEndPositionsRespectingLigatures(Data* queryData, int& startPosition, int& endPosition) const
{
    SVGTextLayoutAttributes* layoutAttributes = queryData->textRenderer->layoutAttributes();
    Vector<SVGTextMetrics>& textMetricsValues = layoutAttributes->textMetricsValues();
    unsigned boxStart = queryData->textBox->start();
    unsigned boxLength = queryData->textBox->len();

    unsigned textMetricsOffset = 0;
    unsigned textMetricsSize = textMetricsValues.size();

    unsigned positionOffset = 0;
    unsigned positionSize = layoutAttributes->context()->textLength();

    bool alterStartPosition = true;
    bool alterEndPosition = true;

    int lastPositionOffset = -1;
    for (; textMetricsOffset < textMetricsSize && positionOffset < positionSize; ++textMetricsOffset) {
        SVGTextMetrics& metrics = textMetricsValues[textMetricsOffset];

        // Advance to text box start location.
        if (positionOffset < boxStart) {
            positionOffset += metrics.length();
            continue;
        }

        // Stop if we've finished processing this text box.
        if (positionOffset >= boxStart + boxLength)
            break;

        // If the start position maps to a character in the metrics list, we don't need to modify it.
        if (startPosition == static_cast<int>(positionOffset))
            alterStartPosition = false;

        // If the start position maps to a character in the metrics list, we don't need to modify it.
        if (endPosition == static_cast<int>(positionOffset))
            alterEndPosition = false;

        // Detect ligatures.
        if (lastPositionOffset != -1 && lastPositionOffset - positionOffset > 1) {
            if (alterStartPosition && startPosition > lastPositionOffset && startPosition < static_cast<int>(positionOffset)) {
                startPosition = lastPositionOffset;
                alterStartPosition = false;
            }

            if (alterEndPosition && endPosition > lastPositionOffset && endPosition < static_cast<int>(positionOffset)) {
                endPosition = positionOffset;
                alterEndPosition = false;
            }
        }

        if (!alterStartPosition && !alterEndPosition)
            break;

        lastPositionOffset = positionOffset;
        positionOffset += metrics.length();
    }

    if (!alterStartPosition && !alterEndPosition)
        return;

    if (lastPositionOffset != -1 && lastPositionOffset - positionOffset > 1) {
        if (alterStartPosition && startPosition > lastPositionOffset && startPosition < static_cast<int>(positionOffset)) {
            startPosition = lastPositionOffset;
            alterStartPosition = false;
        }

        if (alterEndPosition && endPosition > lastPositionOffset && endPosition < static_cast<int>(positionOffset)) {
            endPosition = positionOffset;
            alterEndPosition = false;
        }
    }
}

// numberOfCharacters() implementation
bool SVGTextQuery::numberOfCharactersCallback(Data*, const SVGTextFragment&) const
{
    // no-op
    return false;
}

unsigned SVGTextQuery::numberOfCharacters() const
{
    if (m_textBoxes.isEmpty())
        return 0;

    Data data;
    executeQuery(&data, &SVGTextQuery::numberOfCharactersCallback);
    return data.processedCharacters;
}

// textLength() implementation
struct TextLengthData : SVGTextQuery::Data {
    TextLengthData()
        : textLength(0)
    {
    }

    float textLength;
};

bool SVGTextQuery::textLengthCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    TextLengthData* data = static_cast<TextLengthData*>(queryData);
    data->textLength += queryData->isVerticalText ? fragment.height : fragment.width;
    return false;
}

float SVGTextQuery::textLength() const
{
    if (m_textBoxes.isEmpty())
        return 0;

    TextLengthData data;
    executeQuery(&data, &SVGTextQuery::textLengthCallback);
    return data.textLength;
}

// subStringLength() implementation
struct SubStringLengthData : SVGTextQuery::Data {
    SubStringLengthData(unsigned queryStartPosition, unsigned queryLength)
        : startPosition(queryStartPosition)
        , length(queryLength)
        , subStringLength(0)
    {
    }

    unsigned startPosition;
    unsigned length;

    float subStringLength;
};

bool SVGTextQuery::subStringLengthCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    SubStringLengthData* data = static_cast<SubStringLengthData*>(queryData);

    int startPosition = data->startPosition;
    int endPosition = startPosition + data->length;
    if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
        return false;

    SVGTextMetrics metrics = SVGTextMetrics::measureCharacterRange(queryData->textRenderer, fragment.characterOffset + startPosition, endPosition - startPosition);
    data->subStringLength += queryData->isVerticalText ? metrics.height() : metrics.width();
    return false;
}

float SVGTextQuery::subStringLength(unsigned startPosition, unsigned length) const
{
    if (m_textBoxes.isEmpty())
        return 0;

    SubStringLengthData data(startPosition, length);
    executeQuery(&data, &SVGTextQuery::subStringLengthCallback);
    return data.subStringLength;
}

// startPositionOfCharacter() implementation
struct StartPositionOfCharacterData : SVGTextQuery::Data {
    StartPositionOfCharacterData(unsigned queryPosition)
        : position(queryPosition)
    {
    }

    unsigned position;
    FloatPoint startPosition;
};

bool SVGTextQuery::startPositionOfCharacterCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    StartPositionOfCharacterData* data = static_cast<StartPositionOfCharacterData*>(queryData);

    int startPosition = data->position;
    int endPosition = startPosition + 1;
    if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
        return false;

    data->startPosition = FloatPoint(fragment.x, fragment.y);

    if (startPosition) {
        SVGTextMetrics metrics = SVGTextMetrics::measureCharacterRange(queryData->textRenderer, fragment.characterOffset, startPosition);
        if (queryData->isVerticalText)
            data->startPosition.move(0, metrics.height());
        else
            data->startPosition.move(metrics.width(), 0);
    }

    AffineTransform fragmentTransform;
    fragment.buildFragmentTransform(fragmentTransform, SVGTextFragment::TransformIgnoringTextLength);
    if (fragmentTransform.isIdentity())
        return true;

    data->startPosition = fragmentTransform.mapPoint(data->startPosition);
    return true;
}

SVGPoint SVGTextQuery::startPositionOfCharacter(unsigned position) const
{
    if (m_textBoxes.isEmpty())
        return SVGPoint();

    StartPositionOfCharacterData data(position);
    executeQuery(&data, &SVGTextQuery::startPositionOfCharacterCallback);
    return data.startPosition;
}

// endPositionOfCharacter() implementation
struct EndPositionOfCharacterData : SVGTextQuery::Data {
    EndPositionOfCharacterData(unsigned queryPosition)
        : position(queryPosition)
    {
    }

    unsigned position;
    FloatPoint endPosition;
};

bool SVGTextQuery::endPositionOfCharacterCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    EndPositionOfCharacterData* data = static_cast<EndPositionOfCharacterData*>(queryData);

    int startPosition = data->position;
    int endPosition = startPosition + 1;
    if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
        return false;

    data->endPosition = FloatPoint(fragment.x, fragment.y);

    SVGTextMetrics metrics = SVGTextMetrics::measureCharacterRange(queryData->textRenderer, fragment.characterOffset, startPosition + 1);
    if (queryData->isVerticalText)
        data->endPosition.move(0, metrics.height());
    else
        data->endPosition.move(metrics.width(), 0);

    AffineTransform fragmentTransform;
    fragment.buildFragmentTransform(fragmentTransform, SVGTextFragment::TransformIgnoringTextLength);
    if (fragmentTransform.isIdentity())
        return true;

    data->endPosition = fragmentTransform.mapPoint(data->endPosition);
    return true;
}

SVGPoint SVGTextQuery::endPositionOfCharacter(unsigned position) const
{
    if (m_textBoxes.isEmpty())
        return SVGPoint();

    EndPositionOfCharacterData data(position);
    executeQuery(&data, &SVGTextQuery::endPositionOfCharacterCallback);
    return data.endPosition;
}

// rotationOfCharacter() implementation
struct RotationOfCharacterData : SVGTextQuery::Data {
    RotationOfCharacterData(unsigned queryPosition)
        : position(queryPosition)
        , rotation(0)
    {
    }

    unsigned position;
    float rotation;
};

bool SVGTextQuery::rotationOfCharacterCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    RotationOfCharacterData* data = static_cast<RotationOfCharacterData*>(queryData);

    int startPosition = data->position;
    int endPosition = startPosition + 1;
    if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
        return false;

    AffineTransform fragmentTransform;
    fragment.buildFragmentTransform(fragmentTransform, SVGTextFragment::TransformIgnoringTextLength);
    if (fragmentTransform.isIdentity())
        data->rotation = 0;
    else {
        fragmentTransform.scale(1 / fragmentTransform.xScale(), 1 / fragmentTransform.yScale());
        data->rotation = narrowPrecisionToFloat(rad2deg(atan2(fragmentTransform.b(), fragmentTransform.a())));
    }

    return true;
}

float SVGTextQuery::rotationOfCharacter(unsigned position) const
{
    if (m_textBoxes.isEmpty())
        return 0;

    RotationOfCharacterData data(position);
    executeQuery(&data, &SVGTextQuery::rotationOfCharacterCallback);
    return data.rotation;
}

// extentOfCharacter() implementation
struct ExtentOfCharacterData : SVGTextQuery::Data {
    ExtentOfCharacterData(unsigned queryPosition)
        : position(queryPosition)
    {
    }

    unsigned position;
    FloatRect extent;
};

static inline void calculateGlyphBoundaries(SVGTextQuery::Data* queryData, const SVGTextFragment& fragment, int startPosition, FloatRect& extent)
{
    float scalingFactor = queryData->textRenderer->scalingFactor();
    ASSERT(scalingFactor);

    extent.setLocation(FloatPoint(fragment.x, fragment.y - queryData->textRenderer->scaledFont().fontMetrics().floatAscent() / scalingFactor));

    if (startPosition) {
        SVGTextMetrics metrics = SVGTextMetrics::measureCharacterRange(queryData->textRenderer, fragment.characterOffset, startPosition);
        if (queryData->isVerticalText)
            extent.move(0, metrics.height());
        else
            extent.move(metrics.width(), 0);
    }

    SVGTextMetrics metrics = SVGTextMetrics::measureCharacterRange(queryData->textRenderer, fragment.characterOffset + startPosition, 1);
    extent.setSize(FloatSize(metrics.width(), metrics.height()));

    AffineTransform fragmentTransform;
    fragment.buildFragmentTransform(fragmentTransform, SVGTextFragment::TransformIgnoringTextLength);
    if (fragmentTransform.isIdentity())
        return;

    extent = fragmentTransform.mapRect(extent);
}

bool SVGTextQuery::extentOfCharacterCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    ExtentOfCharacterData* data = static_cast<ExtentOfCharacterData*>(queryData);

    int startPosition = data->position;
    int endPosition = startPosition + 1;
    if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
        return false;

    calculateGlyphBoundaries(queryData, fragment, startPosition, data->extent);
    return true;
}

FloatRect SVGTextQuery::extentOfCharacter(unsigned position) const
{
    if (m_textBoxes.isEmpty())
        return FloatRect();

    ExtentOfCharacterData data(position);
    executeQuery(&data, &SVGTextQuery::extentOfCharacterCallback);
    return data.extent;
}

// characterNumberAtPosition() implementation
struct CharacterNumberAtPositionData : SVGTextQuery::Data {
    CharacterNumberAtPositionData(const FloatPoint& queryPosition)
        : position(queryPosition)
    {
    }

    FloatPoint position;
};

bool SVGTextQuery::characterNumberAtPositionCallback(Data* queryData, const SVGTextFragment& fragment) const
{
    CharacterNumberAtPositionData* data = static_cast<CharacterNumberAtPositionData*>(queryData);

    FloatRect extent;
    for (unsigned i = 0; i < fragment.length; ++i) {
        int startPosition = data->processedCharacters + i;
        int endPosition = startPosition + 1;
        if (!mapStartEndPositionsIntoFragmentCoordinates(queryData, fragment, startPosition, endPosition))
            continue;

        calculateGlyphBoundaries(queryData, fragment, startPosition, extent);
        if (extent.contains(data->position)) {
            data->processedCharacters += i;
            return true;
        }
    }

    return false;
}

int SVGTextQuery::characterNumberAtPosition(const SVGPoint& position) const
{
    if (m_textBoxes.isEmpty())
        return -1;

    CharacterNumberAtPositionData data(position);
    if (!executeQuery(&data, &SVGTextQuery::characterNumberAtPositionCallback))
        return -1;

    return data.processedCharacters;
}

}

#endif
