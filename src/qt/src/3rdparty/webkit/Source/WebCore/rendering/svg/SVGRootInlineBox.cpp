/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Computer Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGRootInlineBox.h"

#if ENABLE(SVG)
#include "GraphicsContext.h"
#include "RenderSVGInlineText.h"
#include "RenderSVGText.h"
#include "SVGInlineFlowBox.h"
#include "SVGInlineTextBox.h"
#include "SVGNames.h"
#include "SVGRenderSupport.h"
#include "SVGTextPositioningElement.h"

namespace WebCore {

void SVGRootInlineBox::paint(PaintInfo& paintInfo, int, int, int, int)
{
    ASSERT(paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection);
    ASSERT(!paintInfo.context->paintingDisabled());

    RenderObject* boxRenderer = renderer();
    ASSERT(boxRenderer);

    bool isPrinting = renderer()->document()->printing();
    bool hasSelection = !isPrinting && selectionState() != RenderObject::SelectionNone;

    PaintInfo childPaintInfo(paintInfo);
    if (hasSelection) {
        for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
            if (child->isSVGInlineTextBox())
                static_cast<SVGInlineTextBox*>(child)->paintSelectionBackground(childPaintInfo);
            else if (child->isSVGInlineFlowBox())
                static_cast<SVGInlineFlowBox*>(child)->paintSelectionBackground(childPaintInfo);
        }
    }

    GraphicsContextStateSaver stateSaver(*childPaintInfo.context);

    if (SVGRenderSupport::prepareToRenderSVGContent(boxRenderer, childPaintInfo)) {
        for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
            if (child->isSVGInlineTextBox())
                SVGInlineFlowBox::computeTextMatchMarkerRectForRenderer(toRenderSVGInlineText(static_cast<SVGInlineTextBox*>(child)->textRenderer()));

            child->paint(childPaintInfo, 0, 0, 0, 0);
        }
    }

    SVGRenderSupport::finishRenderSVGContent(boxRenderer, childPaintInfo, paintInfo.context);
}

void SVGRootInlineBox::computePerCharacterLayoutInformation()
{
    RenderSVGText* parentBlock = toRenderSVGText(block());
    ASSERT(parentBlock);

    Vector<SVGTextLayoutAttributes>& attributes = parentBlock->layoutAttributes();
    if (parentBlock->needsReordering())
        reorderValueLists(attributes);

    // Perform SVG text layout phase two (see SVGTextLayoutEngine for details).
    SVGTextLayoutEngine characterLayout(attributes);
    layoutCharactersInTextBoxes(this, characterLayout);

    // Perform SVG text layout phase three (see SVGTextChunkBuilder for details).
    characterLayout.finishLayout();

    // Perform SVG text layout phase four
    // Position & resize all SVGInlineText/FlowBoxes in the inline box tree, resize the root box as well as the RenderSVGText parent block.
    layoutChildBoxes(this);
    layoutRootBox();
}

void SVGRootInlineBox::layoutCharactersInTextBoxes(InlineFlowBox* start, SVGTextLayoutEngine& characterLayout)
{
    for (InlineBox* child = start->firstChild(); child; child = child->nextOnLine()) {
        if (child->isSVGInlineTextBox()) {
            ASSERT(child->renderer());
            ASSERT(child->renderer()->isSVGInlineText());

            SVGInlineTextBox* textBox = static_cast<SVGInlineTextBox*>(child);
            characterLayout.layoutInlineTextBox(textBox);
        } else {
            // Skip generated content.
            Node* node = child->renderer()->node();
            if (!node)
                continue;

            ASSERT(child->isInlineFlowBox());

            SVGInlineFlowBox* flowBox = static_cast<SVGInlineFlowBox*>(child);
            bool isTextPath = node->hasTagName(SVGNames::textPathTag);
            if (isTextPath) {
                // Build text chunks for all <textPath> children, using the line layout algorithm.
                // This is needeed as text-anchor is just an additional startOffset for text paths.
                RenderSVGText* parentBlock = toRenderSVGText(block());
                ASSERT(parentBlock);

                SVGTextLayoutEngine lineLayout(parentBlock->layoutAttributes());
                layoutCharactersInTextBoxes(flowBox, lineLayout);

                characterLayout.beginTextPathLayout(child->renderer(), lineLayout);
            }

            layoutCharactersInTextBoxes(flowBox, characterLayout);

            if (isTextPath)
                characterLayout.endTextPathLayout();
        }
    }
}

void SVGRootInlineBox::layoutChildBoxes(InlineFlowBox* start)
{
    for (InlineBox* child = start->firstChild(); child; child = child->nextOnLine()) {
        if (child->isSVGInlineTextBox()) {
            ASSERT(child->renderer());
            ASSERT(child->renderer()->isSVGInlineText());

            SVGInlineTextBox* textBox = static_cast<SVGInlineTextBox*>(child);
            IntRect boxRect = textBox->calculateBoundaries();
            textBox->setX(boxRect.x());
            textBox->setY(boxRect.y());
            textBox->setLogicalWidth(boxRect.width());
            textBox->setLogicalHeight(boxRect.height());
        } else {
            // Skip generated content.
            if (!child->renderer()->node())
                continue;

            ASSERT(child->isInlineFlowBox());

            SVGInlineFlowBox* flowBox = static_cast<SVGInlineFlowBox*>(child);
            layoutChildBoxes(flowBox);

            IntRect boxRect = flowBox->calculateBoundaries();
            flowBox->setX(boxRect.x());
            flowBox->setY(boxRect.y());
            flowBox->setLogicalWidth(boxRect.width());
            flowBox->setLogicalHeight(boxRect.height());
        }
    }
}

void SVGRootInlineBox::layoutRootBox()
{
    RenderBlock* parentBlock = block();
    ASSERT(parentBlock);

    IntRect childRect;
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
        // Skip generated content.
        if (!child->renderer()->node())
            continue;
        childRect.unite(child->calculateBoundaries());
    }

    int xBlock = childRect.x();
    int yBlock = childRect.y();
    int widthBlock = childRect.width();
    int heightBlock = childRect.height();

    // Finally, assign the root block position, now that all content is laid out.
    parentBlock->setLocation(xBlock, yBlock);
    parentBlock->setWidth(widthBlock);
    parentBlock->setHeight(heightBlock);

    // Position all children relative to the parent block.
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
        // Skip generated content.
        if (!child->renderer()->node())
            continue;
        child->adjustPosition(-xBlock, -yBlock);
    }

    // Position ourselves.
    setX(0);
    setY(0);
    setLogicalWidth(widthBlock);
    setLogicalHeight(heightBlock);
    setBlockLogicalHeight(heightBlock);
    setLineTopBottomPositions(0, heightBlock);
}

InlineBox* SVGRootInlineBox::closestLeafChildForPosition(const IntPoint& point)
{
    InlineBox* firstLeaf = firstLeafChild();
    InlineBox* lastLeaf = lastLeafChild();
    if (firstLeaf == lastLeaf)
        return firstLeaf;

    // FIXME: Check for vertical text!
    InlineBox* closestLeaf = 0;
    for (InlineBox* leaf = firstLeaf; leaf; leaf = leaf->nextLeafChild()) {
        if (!leaf->isSVGInlineTextBox())
            continue;
        if (point.y() < leaf->m_y)
            continue;
        if (point.y() > leaf->m_y + leaf->virtualLogicalHeight())
            continue;

        closestLeaf = leaf;
        if (point.x() < leaf->m_x + leaf->m_logicalWidth)
            return leaf;
    }

    return closestLeaf ? closestLeaf : lastLeaf;
}
 
static inline void swapItemsInVector(Vector<float>& firstVector, Vector<float>& lastVector, unsigned first, unsigned last)
{
    float temp = firstVector.at(first);
    firstVector.at(first) = lastVector.at(last);
    lastVector.at(last) = temp;
}

static inline void swapItemsInLayoutAttributes(SVGTextLayoutAttributes& firstAttributes, SVGTextLayoutAttributes& lastAttributes, unsigned firstPosition, unsigned lastPosition)
{
    swapItemsInVector(firstAttributes.xValues(), lastAttributes.xValues(), firstPosition, lastPosition);
    swapItemsInVector(firstAttributes.yValues(), lastAttributes.yValues(), firstPosition, lastPosition);
    swapItemsInVector(firstAttributes.dxValues(), lastAttributes.dxValues(), firstPosition, lastPosition);
    swapItemsInVector(firstAttributes.dyValues(), lastAttributes.dyValues(), firstPosition, lastPosition);
    swapItemsInVector(firstAttributes.rotateValues(), lastAttributes.rotateValues(), firstPosition, lastPosition);
}

static inline void findFirstAndLastAttributesInVector(Vector<SVGTextLayoutAttributes>& attributes, RenderSVGInlineText* firstContext, RenderSVGInlineText* lastContext,
                                                      SVGTextLayoutAttributes*& first, SVGTextLayoutAttributes*& last)
{
    first = 0;
    last = 0;

    unsigned attributesSize = attributes.size();
    for (unsigned i = 0; i < attributesSize; ++i) {
        SVGTextLayoutAttributes& current = attributes.at(i);
        if (!first && firstContext == current.context())
            first = &current;
        if (!last && lastContext == current.context())
            last = &current;
        if (first && last)
            break;
    }

    ASSERT(first);
    ASSERT(last);
}

static inline void reverseInlineBoxRangeAndValueListsIfNeeded(void* userData, Vector<InlineBox*>::iterator first, Vector<InlineBox*>::iterator last)
{
    ASSERT(userData);
    Vector<SVGTextLayoutAttributes>& attributes = *reinterpret_cast<Vector<SVGTextLayoutAttributes>*>(userData);

    // This is a copy of std::reverse(first, last). It additionally assure that the value lists within the InlineBoxes are reordered as well.
    while (true)  {
        if (first == last || first == --last)
            return;

        if (!(*last)->isSVGInlineTextBox() || !(*first)->isSVGInlineTextBox()) {
            InlineBox* temp = *first;
            *first = *last;
            *last = temp;
            ++first;
            continue;
        }

        SVGInlineTextBox* firstTextBox = static_cast<SVGInlineTextBox*>(*first);
        SVGInlineTextBox* lastTextBox = static_cast<SVGInlineTextBox*>(*last);

        // Reordering is only necessary for BiDi text that is _absolutely_ positioned.
        if (firstTextBox->len() == 1 && firstTextBox->len() == lastTextBox->len()) {
            RenderSVGInlineText* firstContext = toRenderSVGInlineText(firstTextBox->textRenderer());
            RenderSVGInlineText* lastContext = toRenderSVGInlineText(lastTextBox->textRenderer());

            SVGTextLayoutAttributes* firstAttributes = 0;
            SVGTextLayoutAttributes* lastAttributes = 0;
            findFirstAndLastAttributesInVector(attributes, firstContext, lastContext, firstAttributes, lastAttributes);

            unsigned firstBoxPosition = firstTextBox->start();
            unsigned firstBoxEnd = firstTextBox->end();

            unsigned lastBoxPosition = lastTextBox->start();
            unsigned lastBoxEnd = lastTextBox->end();
            for (; firstBoxPosition <= firstBoxEnd && lastBoxPosition <= lastBoxEnd; ++lastBoxPosition, ++firstBoxPosition)
                swapItemsInLayoutAttributes(*firstAttributes, *lastAttributes, firstBoxPosition, lastBoxPosition);
        }

        InlineBox* temp = *first;
        *first = *last;
        *last = temp;

        ++first;
    }
}

void SVGRootInlineBox::reorderValueLists(Vector<SVGTextLayoutAttributes>& attributes)
{
    Vector<InlineBox*> leafBoxesInLogicalOrder;
    collectLeafBoxesInLogicalOrder(leafBoxesInLogicalOrder, reverseInlineBoxRangeAndValueListsIfNeeded, &attributes);
}

} // namespace WebCore

#endif // ENABLE(SVG)
