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
#include "SVGTextLayoutEngine.h"

#include "RenderSVGInlineText.h"
#include "RenderSVGTextPath.h"
#include "SVGElement.h"
#include "SVGInlineTextBox.h"
#include "SVGTextLayoutEngineBaseline.h"
#include "SVGTextLayoutEngineSpacing.h"

// Set to a value > 0 to dump the text fragments
#define DUMP_TEXT_FRAGMENTS 0

namespace WebCore {

SVGTextLayoutEngine::SVGTextLayoutEngine(Vector<SVGTextLayoutAttributes>& layoutAttributes)
    : m_layoutAttributes(layoutAttributes)
    , m_logicalCharacterOffset(0)
    , m_logicalMetricsListOffset(0)
    , m_visualCharacterOffset(0)
    , m_visualMetricsListOffset(0)
    , m_x(0)
    , m_y(0)
    , m_dx(0)
    , m_dy(0)
    , m_isVerticalText(false)
    , m_inPathLayout(false)
    , m_textPathLength(0)
    , m_textPathCurrentOffset(0)
    , m_textPathSpacing(0)
    , m_textPathScaling(1)
{
    ASSERT(!m_layoutAttributes.isEmpty());
}

void SVGTextLayoutEngine::updateCharacerPositionIfNeeded(float& x, float& y)
{
    if (m_inPathLayout)
        return;

    // Replace characters x/y position, with the current text position plus any
    // relative adjustments, if it doesn't specify an absolute position itself.
    if (x == SVGTextLayoutAttributes::emptyValue()) 
        x = m_x + m_dx;

    if (y == SVGTextLayoutAttributes::emptyValue())
        y = m_y + m_dy;

    m_dx = 0;
    m_dy = 0;
}

void SVGTextLayoutEngine::updateCurrentTextPosition(float x, float y, float glyphAdvance)
{
    // Update current text position after processing the character.
    if (m_isVerticalText) {
        m_x = x;
        m_y = y + glyphAdvance;
    } else {
        m_x = x + glyphAdvance;
        m_y = y;
    }
}

void SVGTextLayoutEngine::updateRelativePositionAdjustmentsIfNeeded(Vector<float>& dxValues, Vector<float>& dyValues)
{
    // Update relative positioning information.
    if (dxValues.isEmpty() && dyValues.isEmpty())
        return;

    float dx = 0;
    if (!dxValues.isEmpty()) {
        float& dxCurrent = dxValues.at(m_logicalCharacterOffset);
        if (dxCurrent != SVGTextLayoutAttributes::emptyValue())
            dx = dxCurrent;
    }

    float dy = 0;
    if (!dyValues.isEmpty()) {
        float& dyCurrent = dyValues.at(m_logicalCharacterOffset);
        if (dyCurrent != SVGTextLayoutAttributes::emptyValue())
            dy = dyCurrent;
    }

    if (m_inPathLayout) {
        if (m_isVerticalText) {
            m_dx += dx;
            m_dy = dy;
        } else {
            m_dx = dx;
            m_dy += dy;
        }

        return;
    }

    m_dx = dx;
    m_dy = dy;
}

void SVGTextLayoutEngine::recordTextFragment(SVGInlineTextBox* textBox, Vector<SVGTextMetrics>& textMetricsValues)
{
    ASSERT(!m_currentTextFragment.length);
    ASSERT(m_visualMetricsListOffset > 0);

    // Figure out length of fragment.
    m_currentTextFragment.length = m_visualCharacterOffset - m_currentTextFragment.characterOffset;

    // Figure out fragment metrics.
    SVGTextMetrics& lastCharacterMetrics = textMetricsValues.at(m_visualMetricsListOffset - 1);
    m_currentTextFragment.width = lastCharacterMetrics.width();
    m_currentTextFragment.height = lastCharacterMetrics.height();

    if (m_currentTextFragment.length > 1) {
        // SVGTextLayoutAttributesBuilder assures that the length of the range is equal to the sum of the individual lengths of the glyphs.
        float length = 0;
        if (m_isVerticalText) {
            for (unsigned i = m_currentTextFragment.metricsListOffset; i < m_visualMetricsListOffset; ++i)
                length += textMetricsValues.at(i).height();
            m_currentTextFragment.height = length;
        } else {
            for (unsigned i = m_currentTextFragment.metricsListOffset; i < m_visualMetricsListOffset; ++i)
                length += textMetricsValues.at(i).width();
            m_currentTextFragment.width = length;
        }
    }

    textBox->textFragments().append(m_currentTextFragment);
    m_currentTextFragment = SVGTextFragment();
}

bool SVGTextLayoutEngine::parentDefinesTextLength(RenderObject* parent) const
{
    RenderObject* currentParent = parent;
    while (currentParent) {
        SVGTextContentElement* textContentElement = SVGTextContentElement::elementFromRenderer(currentParent);
        if (textContentElement) {
            SVGTextContentElement::SVGLengthAdjustType lengthAdjust = static_cast<SVGTextContentElement::SVGLengthAdjustType>(textContentElement->lengthAdjust());
            if (lengthAdjust == SVGTextContentElement::LENGTHADJUST_SPACING && textContentElement->specifiedTextLength().value(textContentElement) > 0)
                return true;
        }

        if (currentParent->isSVGText())
            return false;

        currentParent = currentParent->parent();
    }

    ASSERT_NOT_REACHED();
    return false;
}

void SVGTextLayoutEngine::beginTextPathLayout(RenderObject* object, SVGTextLayoutEngine& lineLayout)
{
    ASSERT(object);

    m_inPathLayout = true;
    RenderSVGTextPath* textPath = toRenderSVGTextPath(object);

    m_textPath = textPath->layoutPath();
    m_textPathStartOffset = textPath->startOffset();
    m_textPathLength = m_textPath.length();
    if (m_textPathStartOffset > 0 && m_textPathStartOffset <= 1)
        m_textPathStartOffset *= m_textPathLength;

    float totalLength = 0;
    unsigned totalCharacters = 0;

    lineLayout.m_chunkLayoutBuilder.buildTextChunks(lineLayout.m_lineLayoutBoxes);
    const Vector<SVGTextChunk>& textChunks = lineLayout.m_chunkLayoutBuilder.textChunks();

    unsigned size = textChunks.size();
    for (unsigned i = 0; i < size; ++i) {
        const SVGTextChunk& chunk = textChunks.at(i);

        float length = 0;
        unsigned characters = 0;
        chunk.calculateLength(length, characters);

        // Handle text-anchor as additional start offset for text paths.
        m_textPathStartOffset += chunk.calculateTextAnchorShift(length);

        totalLength += length;
        totalCharacters += characters;
    }

    m_textPathCurrentOffset = m_textPathStartOffset;

    // Eventually handle textLength adjustments.
    SVGTextContentElement::SVGLengthAdjustType lengthAdjust = SVGTextContentElement::LENGTHADJUST_UNKNOWN;
    float desiredTextLength = 0;

    if (SVGTextContentElement* textContentElement = SVGTextContentElement::elementFromRenderer(textPath)) {
        lengthAdjust = static_cast<SVGTextContentElement::SVGLengthAdjustType>(textContentElement->lengthAdjust());
        desiredTextLength = textContentElement->specifiedTextLength().value(textContentElement);
    }

    if (!desiredTextLength)
        return;

    if (lengthAdjust == SVGTextContentElement::LENGTHADJUST_SPACING)
        m_textPathSpacing = (desiredTextLength - totalLength) / totalCharacters;
    else
        m_textPathScaling = desiredTextLength / totalLength;
}

void SVGTextLayoutEngine::endTextPathLayout()
{
    m_inPathLayout = false;
    m_textPath = Path();
    m_textPathLength = 0;
    m_textPathStartOffset = 0;
    m_textPathCurrentOffset = 0;
    m_textPathSpacing = 0;
    m_textPathScaling = 1;
}

void SVGTextLayoutEngine::layoutInlineTextBox(SVGInlineTextBox* textBox)
{
    ASSERT(textBox);

    RenderSVGInlineText* text = toRenderSVGInlineText(textBox->textRenderer());
    ASSERT(text);
    ASSERT(text->parent());
    ASSERT(text->parent()->node());
    ASSERT(text->parent()->node()->isSVGElement());

    const RenderStyle* style = text->style();
    ASSERT(style);

    textBox->clearTextFragments();
    m_isVerticalText = style->svgStyle()->isVerticalWritingMode();
    layoutTextOnLineOrPath(textBox, text, style);

    if (m_inPathLayout) {
        m_pathLayoutBoxes.append(textBox);
        return;
    }

    m_lineLayoutBoxes.append(textBox);
}

#if DUMP_TEXT_FRAGMENTS > 0
static inline void dumpTextBoxes(Vector<SVGInlineTextBox*>& boxes)
{
    unsigned boxCount = boxes.size();
    fprintf(stderr, "Dumping all text fragments in text sub tree, %i boxes\n", boxCount);

    for (unsigned boxPosition = 0; boxPosition < boxCount; ++boxPosition) {
        SVGInlineTextBox* textBox = boxes.at(boxPosition);
        Vector<SVGTextFragment>& fragments = textBox->textFragments();
        fprintf(stderr, "-> Box %i: Dumping text fragments for SVGInlineTextBox, textBox=%p, textRenderer=%p\n", boxPosition, textBox, textBox->textRenderer());
        fprintf(stderr, "        textBox properties, start=%i, len=%i, box direction=%i\n", textBox->start(), textBox->len(), textBox->direction());
        fprintf(stderr, "   textRenderer properties, textLength=%i\n", textBox->textRenderer()->textLength());

        const UChar* characters = textBox->textRenderer()->characters();

        unsigned fragmentCount = fragments.size();
        for (unsigned i = 0; i < fragmentCount; ++i) {
            SVGTextFragment& fragment = fragments.at(i);
            String fragmentString(characters + fragment.characterOffset, fragment.length);
            fprintf(stderr, "    -> Fragment %i, x=%lf, y=%lf, width=%lf, height=%lf, characterOffset=%i, length=%i, characters='%s'\n"
                          , i, fragment.x, fragment.y, fragment.width, fragment.height, fragment.characterOffset, fragment.length, fragmentString.utf8().data());
        }
    }
}
#endif

void SVGTextLayoutEngine::finalizeTransformMatrices(Vector<SVGInlineTextBox*>& boxes)
{
    unsigned boxCount = boxes.size();
    if (!boxCount)
        return;

    AffineTransform textBoxTransformation;
    for (unsigned boxPosition = 0; boxPosition < boxCount; ++boxPosition) {
        SVGInlineTextBox* textBox = boxes.at(boxPosition);
        Vector<SVGTextFragment>& fragments = textBox->textFragments();

        unsigned fragmentCount = fragments.size();
        for (unsigned i = 0; i < fragmentCount; ++i) {
            m_chunkLayoutBuilder.transformationForTextBox(textBox, textBoxTransformation);
            if (textBoxTransformation.isIdentity())
                continue;
            ASSERT(fragments[i].lengthAdjustTransform.isIdentity());
            fragments[i].lengthAdjustTransform = textBoxTransformation;
        }
    }

    boxes.clear();
}

void SVGTextLayoutEngine::finishLayout()
{
    // After all text fragments are stored in their correpsonding SVGInlineTextBoxes, we can layout individual text chunks.
    // Chunk layouting is only performed for line layout boxes, not for path layout, where it has already been done.
    m_chunkLayoutBuilder.layoutTextChunks(m_lineLayoutBoxes);

    // Finalize transform matrices, after the chunk layout corrections have been applied, and all fragment x/y positions are finalized.
    if (!m_lineLayoutBoxes.isEmpty()) {
#if DUMP_TEXT_FRAGMENTS > 0
        fprintf(stderr, "Line layout: ");
        dumpTextBoxes(m_lineLayoutBoxes);
#endif

        finalizeTransformMatrices(m_lineLayoutBoxes);
    }

    if (!m_pathLayoutBoxes.isEmpty()) {
#if DUMP_TEXT_FRAGMENTS > 0
        fprintf(stderr, "Path layout: ");
        dumpTextBoxes(m_pathLayoutBoxes);
#endif

        finalizeTransformMatrices(m_pathLayoutBoxes);
    }
}

bool SVGTextLayoutEngine::currentLogicalCharacterAttributes(SVGTextLayoutAttributes& logicalAttributes)
{
    if (m_layoutAttributes.isEmpty())
        return false;

    logicalAttributes = m_layoutAttributes.first();
    if (m_logicalCharacterOffset != logicalAttributes.xValues().size())
        return true;

    m_layoutAttributes.remove(0);
    if (m_layoutAttributes.isEmpty())
        return false;

    logicalAttributes = m_layoutAttributes.first();
    m_logicalMetricsListOffset = 0;
    m_logicalCharacterOffset = 0;
    return true;
}

bool SVGTextLayoutEngine::currentLogicalCharacterMetrics(SVGTextLayoutAttributes& logicalAttributes, SVGTextMetrics& logicalMetrics)
{
    logicalMetrics = SVGTextMetrics::emptyMetrics();

    Vector<SVGTextMetrics>& textMetricsValues = logicalAttributes.textMetricsValues();
    unsigned textMetricsSize = textMetricsValues.size();
    while (true) {
        if (m_logicalMetricsListOffset == textMetricsSize) {
            if (!currentLogicalCharacterAttributes(logicalAttributes))
                return false;

            textMetricsValues = logicalAttributes.textMetricsValues();
            textMetricsSize = textMetricsValues.size();
            continue;
        }

        ASSERT(textMetricsSize);
        ASSERT(m_logicalMetricsListOffset < textMetricsSize);
        logicalMetrics = textMetricsValues.at(m_logicalMetricsListOffset);
        if (logicalMetrics == SVGTextMetrics::emptyMetrics() || (!logicalMetrics.width() && !logicalMetrics.height())) {
            advanceToNextLogicalCharacter(logicalMetrics);
            continue;
        }

        // Stop if we found the next valid logical text metrics object.
        return true;
    }

    ASSERT_NOT_REACHED();
    return true;
}

bool SVGTextLayoutEngine::currentVisualCharacterMetrics(SVGInlineTextBox* textBox, RenderSVGInlineText* text, SVGTextMetrics& metrics)
{
    SVGTextLayoutAttributes& attributes = text->layoutAttributes();
    Vector<SVGTextMetrics>& textMetricsValues = attributes.textMetricsValues();
    ASSERT(!textMetricsValues.isEmpty());

    unsigned textMetricsSize = textMetricsValues.size();
    unsigned boxStart = textBox->start();
    unsigned boxLength = textBox->len();

    if (m_visualMetricsListOffset == textMetricsSize)
        return false;

    while (m_visualMetricsListOffset < textMetricsSize) {
        SVGTextMetrics& visualMetrics = textMetricsValues.at(m_visualMetricsListOffset);

        // Advance to text box start location.
        if (m_visualCharacterOffset < boxStart) {
            advanceToNextVisualCharacter(visualMetrics);
            continue;
        }

        // Stop if we've finished processing this text box.
        if (m_visualCharacterOffset >= boxStart + boxLength)
            return false;

        metrics = visualMetrics;
        return true;
    }

    return false;
}

void SVGTextLayoutEngine::advanceToNextLogicalCharacter(const SVGTextMetrics& logicalMetrics)
{
    ++m_logicalMetricsListOffset;
    m_logicalCharacterOffset += logicalMetrics.length();
}

void SVGTextLayoutEngine::advanceToNextVisualCharacter(const SVGTextMetrics& visualMetrics)
{
    ++m_visualMetricsListOffset;
    m_visualCharacterOffset += visualMetrics.length();
}

void SVGTextLayoutEngine::layoutTextOnLineOrPath(SVGInlineTextBox* textBox, RenderSVGInlineText* text, const RenderStyle* style)
{
    SVGElement* lengthContext = static_cast<SVGElement*>(text->parent()->node());
    
    RenderObject* textParent = text->parent();
    bool definesTextLength = textParent ? parentDefinesTextLength(textParent) : false;

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    m_visualMetricsListOffset = 0;
    m_visualCharacterOffset = 0;

    Vector<SVGTextMetrics>& textMetricsValues = text->layoutAttributes().textMetricsValues();
    const UChar* characters = text->characters();

    const Font& font = style->font();
    SVGTextLayoutEngineSpacing spacingLayout(font);
    SVGTextLayoutEngineBaseline baselineLayout(font);

    bool didStartTextFragment = false;
    bool applySpacingToNextCharacter = false;

    float lastAngle = 0;
    float baselineShift = baselineLayout.calculateBaselineShift(svgStyle, lengthContext);
    baselineShift -= baselineLayout.calculateAlignmentBaselineShift(m_isVerticalText, text);

    // Main layout algorithm.
    while (true) {
        // Find the start of the current text box in this list, respecting ligatures.
        SVGTextMetrics visualMetrics = SVGTextMetrics::emptyMetrics();
        if (!currentVisualCharacterMetrics(textBox, text, visualMetrics))
            break;

        if (visualMetrics == SVGTextMetrics::emptyMetrics()) {
            advanceToNextVisualCharacter(visualMetrics);
            continue;
        }

        SVGTextLayoutAttributes logicalAttributes;
        if (!currentLogicalCharacterAttributes(logicalAttributes))
            break;

        SVGTextMetrics logicalMetrics = SVGTextMetrics::emptyMetrics();
        if (!currentLogicalCharacterMetrics(logicalAttributes, logicalMetrics))
            break;

        Vector<float>& xValues = logicalAttributes.xValues();
        Vector<float>& yValues = logicalAttributes.yValues();
        Vector<float>& dxValues = logicalAttributes.dxValues();
        Vector<float>& dyValues = logicalAttributes.dyValues();
        Vector<float>& rotateValues = logicalAttributes.rotateValues();

        float x = xValues.at(m_logicalCharacterOffset);
        float y = yValues.at(m_logicalCharacterOffset);

        // When we've advanced to the box start offset, determine using the original x/y values,
        // whether this character starts a new text chunk, before doing any further processing.
        if (m_visualCharacterOffset == textBox->start())
            textBox->setStartsNewTextChunk(logicalAttributes.context()->characterStartsNewTextChunk(m_logicalCharacterOffset));

        float angle = 0;
        if (!rotateValues.isEmpty()) {
            float newAngle = rotateValues.at(m_logicalCharacterOffset);
            if (newAngle != SVGTextLayoutAttributes::emptyValue())
                angle = newAngle;
        }

        // Calculate glyph orientation angle.
        const UChar* currentCharacter = characters + m_visualCharacterOffset;
        float orientationAngle = baselineLayout.calculateGlyphOrientationAngle(m_isVerticalText, svgStyle, *currentCharacter);

        // Calculate glyph advance & x/y orientation shifts.
        float xOrientationShift = 0;
        float yOrientationShift = 0;
        float glyphAdvance = baselineLayout.calculateGlyphAdvanceAndOrientation(m_isVerticalText, visualMetrics, orientationAngle, xOrientationShift, yOrientationShift);

        // Assign current text position to x/y values, if needed.
        updateCharacerPositionIfNeeded(x, y);

        // Apply dx/dy value adjustments to current text position, if needed.
        updateRelativePositionAdjustmentsIfNeeded(dxValues, dyValues);

        // Calculate SVG Fonts kerning, if needed.
        float kerning = spacingLayout.calculateSVGKerning(m_isVerticalText, visualMetrics.glyph());

        // Calculate CSS 'kerning', 'letter-spacing' and 'word-spacing' for next character, if needed.
        float spacing = spacingLayout.calculateCSSKerningAndSpacing(svgStyle, lengthContext, currentCharacter);

        float textPathOffset = 0;
        if (m_inPathLayout) {
            float scaledGlyphAdvance = glyphAdvance * m_textPathScaling;
            if (m_isVerticalText) {
                // If there's an absolute y position available, it marks the beginning of a new position along the path.
                if (y != SVGTextLayoutAttributes::emptyValue())
                    m_textPathCurrentOffset = y + m_textPathStartOffset;

                m_textPathCurrentOffset += m_dy - kerning;
                m_dy = 0;

                // Apply dx/dy correction and setup translations that move to the glyph midpoint.
                xOrientationShift += m_dx + baselineShift;
                yOrientationShift -= scaledGlyphAdvance / 2;
            } else {
                // If there's an absolute x position available, it marks the beginning of a new position along the path.
                if (x != SVGTextLayoutAttributes::emptyValue())
                    m_textPathCurrentOffset = x + m_textPathStartOffset;

                m_textPathCurrentOffset += m_dx - kerning;
                m_dx = 0;

                // Apply dx/dy correction and setup translations that move to the glyph midpoint.
                xOrientationShift -= scaledGlyphAdvance / 2;
                yOrientationShift += m_dy - baselineShift;
            }

            // Calculate current offset along path.
            textPathOffset = m_textPathCurrentOffset + scaledGlyphAdvance / 2;

            // Move to next character.
            m_textPathCurrentOffset += scaledGlyphAdvance + m_textPathSpacing + spacing * m_textPathScaling;

            // Skip character, if we're before the path.
            if (textPathOffset < 0) {
                advanceToNextLogicalCharacter(logicalMetrics);
                advanceToNextVisualCharacter(visualMetrics);
                continue;
            }

            // Stop processing, if the next character lies behind the path.
            if (textPathOffset > m_textPathLength)
                break;

            bool ok = false;
            FloatPoint point = m_textPath.pointAtLength(textPathOffset, ok);
            ASSERT(ok);

            x = point.x();
            y = point.y();
            angle = m_textPath.normalAngleAtLength(textPathOffset, ok);
            ASSERT(ok);

            // For vertical text on path, the actual angle has to be rotated 90 degrees anti-clockwise, not the orientation angle!
            if (m_isVerticalText)
                angle -= 90;
        } else {
            // Apply all previously calculated shift values.
            if (m_isVerticalText) {
                x += baselineShift;
                y -= kerning;
            } else {
                x -= kerning;
                y -= baselineShift;
            }

            x += m_dx;
            y += m_dy;
        }

        // Determine wheter we have to start a new fragment.
        bool shouldStartNewFragment = false;

        if (m_dx || m_dy)
            shouldStartNewFragment = true;

        if (!shouldStartNewFragment && (m_isVerticalText || m_inPathLayout))
            shouldStartNewFragment = true;

        if (!shouldStartNewFragment && (angle || angle != lastAngle || orientationAngle))
            shouldStartNewFragment = true;

        if (!shouldStartNewFragment && (kerning || applySpacingToNextCharacter || definesTextLength))
            shouldStartNewFragment = true;

        // If we already started a fragment, close it now.
        if (didStartTextFragment && shouldStartNewFragment) {
            applySpacingToNextCharacter = false;
            recordTextFragment(textBox, textMetricsValues);
        }

        // Eventually start a new fragment, if not yet done.
        if (!didStartTextFragment || shouldStartNewFragment) {
            ASSERT(!m_currentTextFragment.characterOffset);
            ASSERT(!m_currentTextFragment.length);

            didStartTextFragment = true;
            m_currentTextFragment.characterOffset = m_visualCharacterOffset;
            m_currentTextFragment.metricsListOffset = m_visualMetricsListOffset;
            m_currentTextFragment.x = x;
            m_currentTextFragment.y = y;

            // Build fragment transformation.
            if (angle)
                m_currentTextFragment.transform.rotate(angle);

            if (xOrientationShift || yOrientationShift)
                m_currentTextFragment.transform.translate(xOrientationShift, yOrientationShift);

            if (orientationAngle)
                m_currentTextFragment.transform.rotate(orientationAngle);

            m_currentTextFragment.isTextOnPath = m_inPathLayout && m_textPathScaling != 1;
            if (m_currentTextFragment.isTextOnPath) {
                if (m_isVerticalText)
                    m_currentTextFragment.lengthAdjustTransform.scaleNonUniform(1, m_textPathScaling);
                else
                    m_currentTextFragment.lengthAdjustTransform.scaleNonUniform(m_textPathScaling, 1);
            }
        }

        // Update current text position, after processing of the current character finished.
        if (m_inPathLayout)
            updateCurrentTextPosition(x, y, glyphAdvance);
        else {
            // Apply CSS 'kerning', 'letter-spacing' and 'word-spacing' to next character, if needed.
            if (spacing)
                applySpacingToNextCharacter = true;

            float xNew = x - m_dx;
            float yNew = y - m_dy;

            if (m_isVerticalText)
                xNew -= baselineShift;
            else
                yNew += baselineShift;

            updateCurrentTextPosition(xNew, yNew, glyphAdvance + spacing);
        }

        advanceToNextLogicalCharacter(logicalMetrics);
        advanceToNextVisualCharacter(visualMetrics);
        lastAngle = angle;
    }

    if (!didStartTextFragment)
        return;

    // Close last open fragment, if needed.
    recordTextFragment(textBox, textMetricsValues);
}

}

#endif // ENABLE(SVG)
