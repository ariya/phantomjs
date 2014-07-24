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
#include "SVGTextChunkBuilder.h"

#include "RenderSVGInlineText.h"
#include "SVGElement.h"
#include "SVGInlineTextBox.h"
#include "SVGLengthContext.h"

namespace WebCore {

SVGTextChunkBuilder::SVGTextChunkBuilder()
{
}

void SVGTextChunkBuilder::transformationForTextBox(SVGInlineTextBox* textBox, AffineTransform& transform) const
{
    DEFINE_STATIC_LOCAL(const AffineTransform, s_identityTransform, ());
    if (!m_textBoxTransformations.contains(textBox)) {
        transform = s_identityTransform;
        return;
    }

    transform = m_textBoxTransformations.get(textBox);
}

void SVGTextChunkBuilder::buildTextChunks(Vector<SVGInlineTextBox*>& lineLayoutBoxes)
{
    if (lineLayoutBoxes.isEmpty())
        return;

    bool foundStart = false;
    unsigned lastChunkStartPosition = 0;
    unsigned boxPosition = 0;
    unsigned boxCount = lineLayoutBoxes.size();
    for (; boxPosition < boxCount; ++boxPosition) {
        SVGInlineTextBox* textBox = lineLayoutBoxes[boxPosition];
        if (!textBox->startsNewTextChunk())
            continue;

        if (!foundStart) {
            lastChunkStartPosition = boxPosition;
            foundStart = true;
        } else {
            ASSERT(boxPosition > lastChunkStartPosition);
            addTextChunk(lineLayoutBoxes, lastChunkStartPosition, boxPosition - lastChunkStartPosition);
            lastChunkStartPosition = boxPosition;
        }
    }

    if (!foundStart)
        return;

    if (boxPosition - lastChunkStartPosition > 0)
        addTextChunk(lineLayoutBoxes, lastChunkStartPosition, boxPosition - lastChunkStartPosition);
}

void SVGTextChunkBuilder::layoutTextChunks(Vector<SVGInlineTextBox*>& lineLayoutBoxes)
{
    buildTextChunks(lineLayoutBoxes);
    if (m_textChunks.isEmpty())
        return;

    unsigned chunkCount = m_textChunks.size();
    for (unsigned i = 0; i < chunkCount; ++i)
        processTextChunk(m_textChunks[i]);

    m_textChunks.clear();
}

void SVGTextChunkBuilder::addTextChunk(Vector<SVGInlineTextBox*>& lineLayoutBoxes, unsigned boxStart, unsigned boxCount)
{
    SVGInlineTextBox* textBox = lineLayoutBoxes[boxStart];
    ASSERT(textBox);

    RenderSVGInlineText* textRenderer = toRenderSVGInlineText(textBox->textRenderer());
    ASSERT(textRenderer);

    const RenderStyle* style = textRenderer->style();
    ASSERT(style);

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    // Build chunk style flags.
    unsigned chunkStyle = SVGTextChunk::DefaultStyle;

    // Handle 'direction' property.
    if (!style->isLeftToRightDirection())
        chunkStyle |= SVGTextChunk::RightToLeftText;

    // Handle 'writing-mode' property.
    if (svgStyle->isVerticalWritingMode())
        chunkStyle |= SVGTextChunk::VerticalText;

    // Handle 'text-anchor' property.
    switch (svgStyle->textAnchor()) {
    case TA_START:
        break;
    case TA_MIDDLE:
        chunkStyle |= SVGTextChunk::MiddleAnchor;
        break;
    case TA_END:
        chunkStyle |= SVGTextChunk::EndAnchor;
        break;
    };

    // Handle 'lengthAdjust' property.
    float desiredTextLength = 0;
    if (SVGTextContentElement* textContentElement = SVGTextContentElement::elementFromRenderer(textRenderer->parent())) {
        SVGLengthContext lengthContext(textContentElement);
        desiredTextLength = textContentElement->specifiedTextLength().value(lengthContext);

        switch (textContentElement->lengthAdjust()) {
        case SVGLengthAdjustUnknown:
            break;
        case SVGLengthAdjustSpacing:
            chunkStyle |= SVGTextChunk::LengthAdjustSpacing;
            break;
        case SVGLengthAdjustSpacingAndGlyphs:
            chunkStyle |= SVGTextChunk::LengthAdjustSpacingAndGlyphs;
            break;
        };
    }

    SVGTextChunk chunk(chunkStyle, desiredTextLength);

    Vector<SVGInlineTextBox*>& boxes = chunk.boxes();
    for (unsigned i = boxStart; i < boxStart + boxCount; ++i)
        boxes.append(lineLayoutBoxes[i]);

    m_textChunks.append(chunk);
}

void SVGTextChunkBuilder::processTextChunk(const SVGTextChunk& chunk)
{
    bool processTextLength = chunk.hasDesiredTextLength();
    bool processTextAnchor = chunk.hasTextAnchor();
    if (!processTextAnchor && !processTextLength)
        return;

    const Vector<SVGInlineTextBox*>& boxes = chunk.boxes();
    unsigned boxCount = boxes.size();
    if (!boxCount)
        return;

    // Calculate absolute length of whole text chunk (starting from text box 'start', spanning 'length' text boxes).
    float chunkLength = 0;
    unsigned chunkCharacters = 0;
    chunk.calculateLength(chunkLength, chunkCharacters);

    bool isVerticalText = chunk.isVerticalText();
    if (processTextLength) {
        if (chunk.hasLengthAdjustSpacing()) {
            float textLengthShift = (chunk.desiredTextLength() - chunkLength) / chunkCharacters;
            unsigned atCharacter = 0;
            for (unsigned boxPosition = 0; boxPosition < boxCount; ++boxPosition) {
                Vector<SVGTextFragment>& fragments = boxes[boxPosition]->textFragments();
                if (fragments.isEmpty())
                    continue;
                processTextLengthSpacingCorrection(isVerticalText, textLengthShift, fragments, atCharacter);
            }
        } else {
            ASSERT(chunk.hasLengthAdjustSpacingAndGlyphs());
            float textLengthScale = chunk.desiredTextLength() / chunkLength;
            AffineTransform spacingAndGlyphsTransform;

            bool foundFirstFragment = false;
            for (unsigned boxPosition = 0; boxPosition < boxCount; ++boxPosition) {
                SVGInlineTextBox* textBox = boxes[boxPosition];
                Vector<SVGTextFragment>& fragments = textBox->textFragments();
                if (fragments.isEmpty())
                    continue;

                if (!foundFirstFragment) {
                    foundFirstFragment = true;
                    buildSpacingAndGlyphsTransform(isVerticalText, textLengthScale, fragments.first(), spacingAndGlyphsTransform);
                }

                m_textBoxTransformations.set(textBox, spacingAndGlyphsTransform);
            }
        }
    }

    if (!processTextAnchor)
        return;

    // If we previously applied a lengthAdjust="spacing" correction, we have to recalculate the chunk length, to be able to apply the text-anchor shift.
    if (processTextLength && chunk.hasLengthAdjustSpacing()) {
        chunkLength = 0;
        chunkCharacters = 0;
        chunk.calculateLength(chunkLength, chunkCharacters);
    }

    float textAnchorShift = chunk.calculateTextAnchorShift(chunkLength);
    for (unsigned boxPosition = 0; boxPosition < boxCount; ++boxPosition) {
        Vector<SVGTextFragment>& fragments = boxes[boxPosition]->textFragments();
        if (fragments.isEmpty())
            continue;
        processTextAnchorCorrection(isVerticalText, textAnchorShift, fragments);
    }
}

void SVGTextChunkBuilder::processTextLengthSpacingCorrection(bool isVerticalText, float textLengthShift, Vector<SVGTextFragment>& fragments, unsigned& atCharacter)
{
    unsigned fragmentCount = fragments.size();
    for (unsigned i = 0; i < fragmentCount; ++i) {
        SVGTextFragment& fragment = fragments[i];

        if (isVerticalText)
            fragment.y += textLengthShift * atCharacter;
        else
            fragment.x += textLengthShift * atCharacter;

        atCharacter += fragment.length;
    }
}

void SVGTextChunkBuilder::processTextAnchorCorrection(bool isVerticalText, float textAnchorShift, Vector<SVGTextFragment>& fragments)
{
    unsigned fragmentCount = fragments.size();
    for (unsigned i = 0; i < fragmentCount; ++i) {
        SVGTextFragment& fragment = fragments[i];

        if (isVerticalText)
            fragment.y += textAnchorShift;
        else
            fragment.x += textAnchorShift;
    }
}

void SVGTextChunkBuilder::buildSpacingAndGlyphsTransform(bool isVerticalText, float scale, const SVGTextFragment& fragment, AffineTransform& spacingAndGlyphsTransform)
{
    spacingAndGlyphsTransform.translate(fragment.x, fragment.y);

    if (isVerticalText)
        spacingAndGlyphsTransform.scaleNonUniform(1, scale);
    else
        spacingAndGlyphsTransform.scaleNonUniform(scale, 1);

    spacingAndGlyphsTransform.translate(-fragment.x, -fragment.y);
}

}

#endif // ENABLE(SVG)
