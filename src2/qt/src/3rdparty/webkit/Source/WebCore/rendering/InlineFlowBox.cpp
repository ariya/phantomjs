/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "InlineFlowBox.h"

#include "CachedImage.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "EllipsisBox.h"
#include "GraphicsContext.h"
#include "InlineTextBox.h"
#include "HitTestResult.h"
#include "RootInlineBox.h"
#include "RenderBlock.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListMarker.h"
#include "RenderRubyBase.h"
#include "RenderRubyRun.h"
#include "RenderRubyText.h"
#include "RenderTableCell.h"
#include "RootInlineBox.h"
#include "Text.h"

#include <math.h>

using namespace std;

namespace WebCore {

#ifndef NDEBUG

InlineFlowBox::~InlineFlowBox()
{
    if (!m_hasBadChildList)
        for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
            child->setHasBadParent();
}

#endif

int InlineFlowBox::getFlowSpacingLogicalWidth()
{
    int totWidth = marginBorderPaddingLogicalLeft() + marginBorderPaddingLogicalRight();
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->isInlineFlowBox())
            totWidth += static_cast<InlineFlowBox*>(curr)->getFlowSpacingLogicalWidth();
    }
    return totWidth;
}

IntRect InlineFlowBox::roundedFrameRect() const
{
    // Begin by snapping the x and y coordinates to the nearest pixel.
    int snappedX = lroundf(x());
    int snappedY = lroundf(y());
    
    int snappedMaxX = lroundf(x() + width());
    int snappedMaxY = lroundf(y() + height());
    
    return IntRect(snappedX, snappedY, snappedMaxX - snappedX, snappedMaxY - snappedY);
}

void InlineFlowBox::addToLine(InlineBox* child) 
{
    ASSERT(!child->parent());
    ASSERT(!child->nextOnLine());
    ASSERT(!child->prevOnLine());
    checkConsistency();

    child->setParent(this);
    if (!m_firstChild) {
        m_firstChild = child;
        m_lastChild = child;
    } else {
        m_lastChild->setNextOnLine(child);
        child->setPrevOnLine(m_lastChild);
        m_lastChild = child;
    }
    child->setFirstLineStyleBit(m_firstLine);
    child->setIsHorizontal(isHorizontal());
    if (child->isText()) {
        if (child->renderer()->parent() == renderer())
            m_hasTextChildren = true;
        m_hasTextDescendants = true;
    } else if (child->isInlineFlowBox()) {
        if (static_cast<InlineFlowBox*>(child)->hasTextDescendants())
            m_hasTextDescendants = true;
    }

    if (descendantsHaveSameLineHeightAndBaseline() && !child->renderer()->isPositioned()) {
        RenderStyle* parentStyle = renderer()->style(m_firstLine);
        RenderStyle* childStyle = child->renderer()->style(m_firstLine);
        bool shouldClearDescendantsHaveSameLineHeightAndBaseline = false;
        if (child->renderer()->isReplaced())
            shouldClearDescendantsHaveSameLineHeightAndBaseline = true;
        else if (child->isText()) {
            if (child->renderer()->isBR() || child->renderer()->parent() != renderer()) {
                if (!parentStyle->font().fontMetrics().hasIdenticalAscentDescentAndLineGap(childStyle->font().fontMetrics())
                    || parentStyle->lineHeight() != childStyle->lineHeight()
                    || (parentStyle->verticalAlign() != BASELINE && !isRootInlineBox()) || childStyle->verticalAlign() != BASELINE)
                    shouldClearDescendantsHaveSameLineHeightAndBaseline = true;
            }
            if (childStyle->hasTextCombine() || childStyle->textEmphasisMark() != TextEmphasisMarkNone)
                shouldClearDescendantsHaveSameLineHeightAndBaseline = true;
        } else {
            if (child->renderer()->isBR()) {
                // FIXME: This is dumb. We only turn off because current layout test results expect the <br> to be 0-height on the baseline.
                // Other than making a zillion tests have to regenerate results, there's no reason to ditch the optimization here.
                shouldClearDescendantsHaveSameLineHeightAndBaseline = true;
            } else {
                ASSERT(isInlineFlowBox());
                InlineFlowBox* childFlowBox = static_cast<InlineFlowBox*>(child);
                // Check the child's bit, and then also check for differences in font, line-height, vertical-align
                if (!childFlowBox->descendantsHaveSameLineHeightAndBaseline()
                    || !parentStyle->font().fontMetrics().hasIdenticalAscentDescentAndLineGap(childStyle->font().fontMetrics())
                    || parentStyle->lineHeight() != childStyle->lineHeight()
                    || (parentStyle->verticalAlign() != BASELINE && !isRootInlineBox()) || childStyle->verticalAlign() != BASELINE
                    || childStyle->hasBorder() || childStyle->hasPadding() || childStyle->hasTextCombine())
                    shouldClearDescendantsHaveSameLineHeightAndBaseline = true;
            }
        }

        if (shouldClearDescendantsHaveSameLineHeightAndBaseline)
            clearDescendantsHaveSameLineHeightAndBaseline();
    }

    if (!child->renderer()->isPositioned()) {
        if (child->isText()) {
            RenderStyle* childStyle = child->renderer()->style(m_firstLine);
            if (childStyle->letterSpacing() < 0 || childStyle->textShadow() || childStyle->textEmphasisMark() != TextEmphasisMarkNone || childStyle->textStrokeWidth())
                child->clearKnownToHaveNoOverflow();
        } else if (child->renderer()->isReplaced()) {
            RenderBox* box = toRenderBox(child->renderer());
            if (box->hasRenderOverflow() || box->hasSelfPaintingLayer())
                child->clearKnownToHaveNoOverflow();
        } else if (!child->renderer()->isBR() && (child->renderer()->style(m_firstLine)->boxShadow() || child->boxModelObject()->hasSelfPaintingLayer()
                   || (child->renderer()->isListMarker() && !toRenderListMarker(child->renderer())->isInside())))
            child->clearKnownToHaveNoOverflow();
        
        if (knownToHaveNoOverflow() && child->isInlineFlowBox() && !static_cast<InlineFlowBox*>(child)->knownToHaveNoOverflow())
            clearKnownToHaveNoOverflow();
    }

    checkConsistency();
}

void InlineFlowBox::removeChild(InlineBox* child)
{
    checkConsistency();

    if (!m_dirty)
        dirtyLineBoxes();

    root()->childRemoved(child);

    if (child == m_firstChild)
        m_firstChild = child->nextOnLine();
    if (child == m_lastChild)
        m_lastChild = child->prevOnLine();
    if (child->nextOnLine())
        child->nextOnLine()->setPrevOnLine(child->prevOnLine());
    if (child->prevOnLine())
        child->prevOnLine()->setNextOnLine(child->nextOnLine());
    
    child->setParent(0);

    checkConsistency();
}

void InlineFlowBox::deleteLine(RenderArena* arena)
{
    InlineBox* child = firstChild();
    InlineBox* next = 0;
    while (child) {
        ASSERT(this == child->parent());
        next = child->nextOnLine();
#ifndef NDEBUG
        child->setParent(0);
#endif
        child->deleteLine(arena);
        child = next;
    }
#ifndef NDEBUG
    m_firstChild = 0;
    m_lastChild = 0;
#endif

    removeLineBoxFromRenderObject();
    destroy(arena);
}

void InlineFlowBox::removeLineBoxFromRenderObject()
{
    toRenderInline(renderer())->lineBoxes()->removeLineBox(this);
}

void InlineFlowBox::extractLine()
{
    if (!m_extracted)
        extractLineBoxFromRenderObject();
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
        child->extractLine();
}

void InlineFlowBox::extractLineBoxFromRenderObject()
{
    toRenderInline(renderer())->lineBoxes()->extractLineBox(this);
}

void InlineFlowBox::attachLine()
{
    if (m_extracted)
        attachLineBoxToRenderObject();
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
        child->attachLine();
}

void InlineFlowBox::attachLineBoxToRenderObject()
{
    toRenderInline(renderer())->lineBoxes()->attachLineBox(this);
}

void InlineFlowBox::adjustPosition(float dx, float dy)
{
    InlineBox::adjustPosition(dx, dy);
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
        child->adjustPosition(dx, dy);
    if (m_overflow)
        m_overflow->move(dx, dy); // FIXME: Rounding error here since overflow was pixel snapped, but nobody other than list markers passes non-integral values here.
}

RenderLineBoxList* InlineFlowBox::rendererLineBoxes() const
{
    return toRenderInline(renderer())->lineBoxes();
}

static inline bool isLastChildForRenderer(RenderObject* ancestor, RenderObject* child)
{
    if (!child)
        return false;
    
    if (child == ancestor)
        return true;

    RenderObject* curr = child;
    RenderObject* parent = curr->parent();
    while (parent && (!parent->isRenderBlock() || parent->isInline())) {
        if (parent->lastChild() != curr)
            return false;
        if (parent == ancestor)
            return true;
            
        curr = parent;
        parent = curr->parent();
    }

    return true;
}

static bool isAnsectorAndWithinBlock(RenderObject* ancestor, RenderObject* child)
{
    RenderObject* object = child;
    while (object && (!object->isRenderBlock() || object->isInline())) {
        if (object == ancestor)
            return true;
        object = object->parent();
    }
    return false;
}

void InlineFlowBox::determineSpacingForFlowBoxes(bool lastLine, bool isLogicallyLastRunWrapped, RenderObject* logicallyLastRunRenderer)
{
    // All boxes start off open.  They will not apply any margins/border/padding on
    // any side.
    bool includeLeftEdge = false;
    bool includeRightEdge = false;

    // The root inline box never has borders/margins/padding.
    if (parent()) {
        bool ltr = renderer()->style()->isLeftToRightDirection();

        // Check to see if all initial lines are unconstructed.  If so, then
        // we know the inline began on this line (unless we are a continuation).
        RenderLineBoxList* lineBoxList = rendererLineBoxes();
        if (!lineBoxList->firstLineBox()->isConstructed() && !renderer()->isInlineElementContinuation()) {
            if (ltr && lineBoxList->firstLineBox() == this)
                includeLeftEdge = true;
            else if (!ltr && lineBoxList->lastLineBox() == this)
                includeRightEdge = true;
        }

        if (!lineBoxList->lastLineBox()->isConstructed()) {
            RenderInline* inlineFlow = toRenderInline(renderer());
            bool isLastObjectOnLine = !isAnsectorAndWithinBlock(renderer(), logicallyLastRunRenderer) || (isLastChildForRenderer(renderer(), logicallyLastRunRenderer) && !isLogicallyLastRunWrapped);

            // We include the border under these conditions:
            // (1) The next line was not created, or it is constructed. We check the previous line for rtl.
            // (2) The logicallyLastRun is not a descendant of this renderer.
            // (3) The logicallyLastRun is a descendant of this renderer, but it is the last child of this renderer and it does not wrap to the next line.
            
            if (ltr) {
                if (!nextLineBox()
                    && ((lastLine || isLastObjectOnLine) && !inlineFlow->continuation()))
                    includeRightEdge = true;
            } else {
                if ((!prevLineBox() || prevLineBox()->isConstructed())
                    && ((lastLine || isLastObjectOnLine) && !inlineFlow->continuation()))
                    includeLeftEdge = true;
            }
        }
    }

    setEdges(includeLeftEdge, includeRightEdge);

    // Recur into our children.
    for (InlineBox* currChild = firstChild(); currChild; currChild = currChild->nextOnLine()) {
        if (currChild->isInlineFlowBox()) {
            InlineFlowBox* currFlow = static_cast<InlineFlowBox*>(currChild);
            currFlow->determineSpacingForFlowBoxes(lastLine, isLogicallyLastRunWrapped, logicallyLastRunRenderer);
        }
    }
}

float InlineFlowBox::placeBoxesInInlineDirection(float logicalLeft, bool& needsWordSpacing, GlyphOverflowAndFallbackFontsMap& textBoxDataMap)
{
    // Set our x position.
    setLogicalLeft(logicalLeft);
  
    float startLogicalLeft = logicalLeft;
    logicalLeft += borderLogicalLeft() + paddingLogicalLeft();

    float minLogicalLeft = startLogicalLeft;
    float maxLogicalRight = logicalLeft;

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isText()) {
            InlineTextBox* text = static_cast<InlineTextBox*>(curr);
            RenderText* rt = toRenderText(text->renderer());
            if (rt->textLength()) {
                if (needsWordSpacing && isSpaceOrNewline(rt->characters()[text->start()]))
                    logicalLeft += rt->style(m_firstLine)->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characters()[text->end()]);
            }
            text->setLogicalLeft(logicalLeft);
            if (knownToHaveNoOverflow())
                minLogicalLeft = min(logicalLeft, minLogicalLeft);
            logicalLeft += text->logicalWidth();
            if (knownToHaveNoOverflow())
                maxLogicalRight = max(logicalLeft, maxLogicalRight);
        } else {
            if (curr->renderer()->isPositioned()) {
                if (curr->renderer()->parent()->style()->isLeftToRightDirection())
                    curr->setLogicalLeft(logicalLeft);
                else
                    // Our offset that we cache needs to be from the edge of the right border box and
                    // not the left border box.  We have to subtract |x| from the width of the block
                    // (which can be obtained from the root line box).
                    curr->setLogicalLeft(root()->block()->logicalWidth() - logicalLeft);
                continue; // The positioned object has no effect on the width.
            }
            if (curr->renderer()->isRenderInline()) {
                InlineFlowBox* flow = static_cast<InlineFlowBox*>(curr);
                logicalLeft += flow->marginLogicalLeft();
                if (knownToHaveNoOverflow())
                    minLogicalLeft = min(logicalLeft, minLogicalLeft);
                logicalLeft = flow->placeBoxesInInlineDirection(logicalLeft, needsWordSpacing, textBoxDataMap);
                if (knownToHaveNoOverflow())
                    maxLogicalRight = max(logicalLeft, maxLogicalRight);
                logicalLeft += flow->marginLogicalRight();
            } else if (!curr->renderer()->isListMarker() || toRenderListMarker(curr->renderer())->isInside()) {
                // The box can have a different writing-mode than the overall line, so this is a bit complicated.
                // Just get all the physical margin and overflow values by hand based off |isVertical|.
                int logicalLeftMargin = isHorizontal() ? curr->boxModelObject()->marginLeft() : curr->boxModelObject()->marginTop();
                int logicalRightMargin = isHorizontal() ? curr->boxModelObject()->marginRight() : curr->boxModelObject()->marginBottom();
                
                logicalLeft += logicalLeftMargin;
                curr->setLogicalLeft(logicalLeft);
                if (knownToHaveNoOverflow())
                    minLogicalLeft = min(logicalLeft, minLogicalLeft);
                logicalLeft += curr->logicalWidth();
                if (knownToHaveNoOverflow())
                    maxLogicalRight = max(logicalLeft, maxLogicalRight);
                logicalLeft += logicalRightMargin;
            }
        }
    }

    logicalLeft += borderLogicalRight() + paddingLogicalRight();
    setLogicalWidth(logicalLeft - startLogicalLeft);
    if (knownToHaveNoOverflow() && (minLogicalLeft < startLogicalLeft || maxLogicalRight > logicalLeft))
        clearKnownToHaveNoOverflow();
    return logicalLeft;
}

bool InlineFlowBox::requiresIdeographicBaseline(const GlyphOverflowAndFallbackFontsMap& textBoxDataMap) const
{
    if (isHorizontal())
        return false;
    
    if (renderer()->style(m_firstLine)->fontDescription().textOrientation() == TextOrientationUpright
        || renderer()->style(m_firstLine)->font().primaryFont()->hasVerticalGlyphs())
        return true;

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->isInlineFlowBox()) {
            if (static_cast<InlineFlowBox*>(curr)->requiresIdeographicBaseline(textBoxDataMap))
                return true;
        } else {
            if (curr->renderer()->style(m_firstLine)->font().primaryFont()->hasVerticalGlyphs())
                return true;
            
            const Vector<const SimpleFontData*>* usedFonts = 0;
            if (curr->isInlineTextBox()) {
                GlyphOverflowAndFallbackFontsMap::const_iterator it = textBoxDataMap.find(static_cast<InlineTextBox*>(curr));
                usedFonts = it == textBoxDataMap.end() ? 0 : &it->second.first;
            }

            if (usedFonts) {
                for (size_t i = 0; i < usedFonts->size(); ++i) {
                    if (usedFonts->at(i)->hasVerticalGlyphs())
                        return true;
                }
            }
        }
    }
    
    return false;
}

void InlineFlowBox::adjustMaxAscentAndDescent(int& maxAscent, int& maxDescent, int maxPositionTop, int maxPositionBottom)
{
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        // The computed lineheight needs to be extended for the
        // positioned elements
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        if (curr->verticalAlign() == TOP || curr->verticalAlign() == BOTTOM) {
            int lineHeight = curr->lineHeight();
            if (curr->verticalAlign() == TOP) {
                if (maxAscent + maxDescent < lineHeight)
                    maxDescent = lineHeight - maxAscent;
            }
            else {
                if (maxAscent + maxDescent < lineHeight)
                    maxAscent = lineHeight - maxDescent;
            }

            if (maxAscent + maxDescent >= max(maxPositionTop, maxPositionBottom))
                break;
        }

        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->adjustMaxAscentAndDescent(maxAscent, maxDescent, maxPositionTop, maxPositionBottom);
    }
}

void InlineFlowBox::computeLogicalBoxHeights(RootInlineBox* rootBox, int& maxPositionTop, int& maxPositionBottom,
                                             int& maxAscent, int& maxDescent, bool& setMaxAscent, bool& setMaxDescent,
                                             bool strictMode, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
                                             FontBaseline baselineType, VerticalPositionCache& verticalPositionCache)
{
    // The primary purpose of this function is to compute the maximal ascent and descent values for
    // a line. These values are computed based off the block's line-box-contain property, which indicates
    // what parts of descendant boxes have to fit within the line.
    //
    // The maxAscent value represents the distance of the highest point of any box (typically including line-height) from
    // the root box's baseline. The maxDescent value represents the distance of the lowest point of any box
    // (also typically including line-height) from the root box baseline. These values can be negative.
    //
    // A secondary purpose of this function is to store the offset of every box's baseline from the root box's
    // baseline. This information is cached in the logicalTop() of every box. We're effectively just using
    // the logicalTop() as scratch space.
    //
    // Because a box can be positioned such that it ends up fully above or fully below the
    // root line box, we only consider it to affect the maxAscent and maxDescent values if some
    // part of the box (EXCLUDING leading) is above (for ascent) or below (for descent) the root box's baseline.
    bool affectsAscent = false;
    bool affectsDescent = false;
    bool checkChildren = !descendantsHaveSameLineHeightAndBaseline();
    
    if (isRootInlineBox()) {
        // Examine our root box.
        int ascent = 0;
        int descent = 0;
        rootBox->ascentAndDescentForBox(rootBox, textBoxDataMap, ascent, descent, affectsAscent, affectsDescent);
        if (strictMode || hasTextChildren() || (!checkChildren && hasTextDescendants())) {
            if (maxAscent < ascent || !setMaxAscent) {
                maxAscent = ascent;
                setMaxAscent = true;
            }
            if (maxDescent < descent || !setMaxDescent) {
                maxDescent = descent;
                setMaxDescent = true;
            }
        }
    }

    if (!checkChildren)
        return;

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        InlineFlowBox* inlineFlowBox = curr->isInlineFlowBox() ? static_cast<InlineFlowBox*>(curr) : 0;
        
        bool affectsAscent = false;
        bool affectsDescent = false;
        
        // The verticalPositionForBox function returns the distance between the child box's baseline
        // and the root box's baseline.  The value is negative if the child box's baseline is above the
        // root box's baseline, and it is positive if the child box's baseline is below the root box's baseline.
        curr->setLogicalTop(rootBox->verticalPositionForBox(curr, verticalPositionCache));
        
        int ascent = 0;
        int descent = 0;
        rootBox->ascentAndDescentForBox(curr, textBoxDataMap, ascent, descent, affectsAscent, affectsDescent);

        int boxHeight = ascent + descent;
        if (curr->verticalAlign() == TOP) {
            if (maxPositionTop < boxHeight)
                maxPositionTop = boxHeight;
        } else if (curr->verticalAlign() == BOTTOM) {
            if (maxPositionBottom < boxHeight)
                maxPositionBottom = boxHeight;
        } else if (!inlineFlowBox || strictMode || inlineFlowBox->hasTextChildren() || (inlineFlowBox->descendantsHaveSameLineHeightAndBaseline() && inlineFlowBox->hasTextDescendants())
                   || inlineFlowBox->boxModelObject()->hasInlineDirectionBordersOrPadding()) {
            // Note that these values can be negative.  Even though we only affect the maxAscent and maxDescent values
            // if our box (excluding line-height) was above (for ascent) or below (for descent) the root baseline, once you factor in line-height
            // the final box can end up being fully above or fully below the root box's baseline!  This is ok, but what it
            // means is that ascent and descent (including leading), can end up being negative.  The setMaxAscent and
            // setMaxDescent booleans are used to ensure that we're willing to initially set maxAscent/Descent to negative
            // values.
            ascent -= curr->logicalTop();
            descent += curr->logicalTop();
            if (affectsAscent && (maxAscent < ascent || !setMaxAscent)) {
                maxAscent = ascent;
                setMaxAscent = true;
            }

            if (affectsDescent && (maxDescent < descent || !setMaxDescent)) {
                maxDescent = descent;
                setMaxDescent = true;
            }
        }

        if (inlineFlowBox)
            inlineFlowBox->computeLogicalBoxHeights(rootBox, maxPositionTop, maxPositionBottom, maxAscent, maxDescent,
                                                    setMaxAscent, setMaxDescent, strictMode, textBoxDataMap,
                                                    baselineType, verticalPositionCache);
    }
}

void InlineFlowBox::placeBoxesInBlockDirection(int top, int maxHeight, int maxAscent, bool strictMode, int& lineTop, int& lineBottom, bool& setLineTop,
                                               int& lineTopIncludingMargins, int& lineBottomIncludingMargins, bool& hasAnnotationsBefore, bool& hasAnnotationsAfter, FontBaseline baselineType)
{
    bool isRootBox = isRootInlineBox();
    if (isRootBox) {
        const FontMetrics& fontMetrics = renderer()->style(m_firstLine)->fontMetrics();
        setLogicalTop(top + maxAscent - fontMetrics.ascent(baselineType));
    }

    int adjustmentForChildrenWithSameLineHeightAndBaseline = 0;
    if (descendantsHaveSameLineHeightAndBaseline()) {
        adjustmentForChildrenWithSameLineHeightAndBaseline = logicalTop();
        if (parent())
            adjustmentForChildrenWithSameLineHeightAndBaseline += (boxModelObject()->borderBefore() + boxModelObject()->paddingBefore());
    }

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (descendantsHaveSameLineHeightAndBaseline()) {
            curr->adjustBlockDirectionPosition(adjustmentForChildrenWithSameLineHeightAndBaseline);
            continue;
        }

        InlineFlowBox* inlineFlowBox = curr->isInlineFlowBox() ? static_cast<InlineFlowBox*>(curr) : 0;
        bool childAffectsTopBottomPos = true;
        if (curr->verticalAlign() == TOP)
            curr->setLogicalTop(top);
        else if (curr->verticalAlign() == BOTTOM)
            curr->setLogicalTop(top + maxHeight - curr->lineHeight());
        else {
            if (!strictMode && inlineFlowBox && !inlineFlowBox->hasTextChildren() && !curr->boxModelObject()->hasInlineDirectionBordersOrPadding()
                && !(inlineFlowBox->descendantsHaveSameLineHeightAndBaseline() && inlineFlowBox->hasTextDescendants()))
                childAffectsTopBottomPos = false;
            int posAdjust = maxAscent - curr->baselinePosition(baselineType);
            curr->setLogicalTop(curr->logicalTop() + top + posAdjust);
        }
        
        int newLogicalTop = curr->logicalTop();
        int newLogicalTopIncludingMargins = newLogicalTop;
        int boxHeight = curr->logicalHeight();
        int boxHeightIncludingMargins = boxHeight;
            
        if (curr->isText() || curr->isInlineFlowBox()) {
            const FontMetrics& fontMetrics = curr->renderer()->style(m_firstLine)->fontMetrics();
            newLogicalTop += curr->baselinePosition(baselineType) - fontMetrics.ascent(baselineType);
            if (curr->isInlineFlowBox()) {
                RenderBoxModelObject* boxObject = toRenderBoxModelObject(curr->renderer());
                newLogicalTop -= boxObject->style(m_firstLine)->isHorizontalWritingMode() ? boxObject->borderTop() + boxObject->paddingTop() : 
                                 boxObject->borderRight() + boxObject->paddingRight();
            }
            newLogicalTopIncludingMargins = newLogicalTop;
        } else if (!curr->renderer()->isBR()) {
            RenderBox* box = toRenderBox(curr->renderer());
            newLogicalTopIncludingMargins = newLogicalTop;
            int overSideMargin = curr->isHorizontal() ? box->marginTop() : box->marginRight();
            int underSideMargin = curr->isHorizontal() ? box->marginBottom() : box->marginLeft();
            newLogicalTop += overSideMargin;
            boxHeightIncludingMargins += overSideMargin + underSideMargin;
        }

        curr->setLogicalTop(newLogicalTop);

        if (childAffectsTopBottomPos) {
            if (curr->renderer()->isRubyRun()) {
                // Treat the leading on the first and last lines of ruby runs as not being part of the overall lineTop/lineBottom.
                // Really this is a workaround hack for the fact that ruby should have been done as line layout and not done using
                // inline-block.
                if (!renderer()->style()->isFlippedLinesWritingMode())
                    hasAnnotationsBefore = true;
                else
                    hasAnnotationsAfter = true;

                RenderRubyRun* rubyRun = static_cast<RenderRubyRun*>(curr->renderer());
                if (RenderRubyBase* rubyBase = rubyRun->rubyBase()) {
                    int bottomRubyBaseLeading = (curr->logicalHeight() - rubyBase->logicalBottom()) + rubyBase->logicalHeight() - (rubyBase->lastRootBox() ? rubyBase->lastRootBox()->lineBottom() : 0);
                    int topRubyBaseLeading = rubyBase->logicalTop() + (rubyBase->firstRootBox() ? rubyBase->firstRootBox()->lineTop() : 0);
                    newLogicalTop += !renderer()->style()->isFlippedLinesWritingMode() ? topRubyBaseLeading : bottomRubyBaseLeading;
                    boxHeight -= (topRubyBaseLeading + bottomRubyBaseLeading);
                }
            }
            if (curr->isInlineTextBox()) {
                TextEmphasisPosition emphasisMarkPosition;
                if (static_cast<InlineTextBox*>(curr)->getEmphasisMarkPosition(curr->renderer()->style(m_firstLine), emphasisMarkPosition)) {
                    bool emphasisMarkIsOver = emphasisMarkPosition == TextEmphasisPositionOver;
                    if (emphasisMarkIsOver != curr->renderer()->style(m_firstLine)->isFlippedLinesWritingMode())
                        hasAnnotationsBefore = true;
                    else
                        hasAnnotationsAfter = true;
                }
            }

            if (!setLineTop) {
                setLineTop = true;
                lineTop = newLogicalTop;
                lineTopIncludingMargins = min(lineTop, newLogicalTopIncludingMargins);
            } else {
                lineTop = min(lineTop, newLogicalTop);
                lineTopIncludingMargins = min(lineTop, min(lineTopIncludingMargins, newLogicalTopIncludingMargins));
            }
            lineBottom = max(lineBottom, newLogicalTop + boxHeight);
            lineBottomIncludingMargins = max(lineBottom, max(lineBottomIncludingMargins, newLogicalTopIncludingMargins + boxHeightIncludingMargins));
        }

        // Adjust boxes to use their real box y/height and not the logical height (as dictated by
        // line-height).
        if (inlineFlowBox)
            inlineFlowBox->placeBoxesInBlockDirection(top, maxHeight, maxAscent, strictMode, lineTop, lineBottom, setLineTop,
                                                      lineTopIncludingMargins, lineBottomIncludingMargins, hasAnnotationsBefore, hasAnnotationsAfter, baselineType);
    }

    if (isRootBox) {
        if (strictMode || hasTextChildren() || (descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
            if (!setLineTop) {
                setLineTop = true;
                lineTop = logicalTop();
                lineTopIncludingMargins = lineTop;
            } else {
                lineTop = min(lineTop, logicalTop());
                lineTopIncludingMargins = min(lineTop, lineTopIncludingMargins);
            }
            lineBottom = max(lineBottom, logicalTop() + logicalHeight());
            lineBottomIncludingMargins = max(lineBottom, lineBottomIncludingMargins);
        }
        
        if (renderer()->style()->isFlippedLinesWritingMode())
            flipLinesInBlockDirection(lineTopIncludingMargins, lineBottomIncludingMargins);
    }
}

void InlineFlowBox::flipLinesInBlockDirection(int lineTop, int lineBottom)
{
    // Flip the box on the line such that the top is now relative to the lineBottom instead of the lineTop.
    setLogicalTop(lineBottom - (logicalTop() - lineTop) - logicalHeight());
    
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders aren't affected here.
        
        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->flipLinesInBlockDirection(lineTop, lineBottom);
        else
            curr->setLogicalTop(lineBottom - (curr->logicalTop() - lineTop) - curr->logicalHeight());
    }
}

inline void InlineFlowBox::addBoxShadowVisualOverflow(IntRect& logicalVisualOverflow)
{
    if (!parent())
        return; // Box-shadow doesn't apply to root line boxes.

    int boxShadowLogicalTop;
    int boxShadowLogicalBottom;
    renderer()->style(m_firstLine)->getBoxShadowBlockDirectionExtent(boxShadowLogicalTop, boxShadowLogicalBottom);
    
    int logicalTopVisualOverflow = min(logicalTop() + boxShadowLogicalTop, logicalVisualOverflow.y());
    int logicalBottomVisualOverflow = max(logicalBottom() + boxShadowLogicalBottom, logicalVisualOverflow.maxY());
    
    int boxShadowLogicalLeft;
    int boxShadowLogicalRight;
    renderer()->style(m_firstLine)->getBoxShadowInlineDirectionExtent(boxShadowLogicalLeft, boxShadowLogicalRight);

    int logicalLeftVisualOverflow = min(pixelSnappedLogicalLeft() + boxShadowLogicalLeft, logicalVisualOverflow.x());
    int logicalRightVisualOverflow = max(pixelSnappedLogicalRight() + boxShadowLogicalRight, logicalVisualOverflow.maxX());
    
    logicalVisualOverflow = IntRect(logicalLeftVisualOverflow, logicalTopVisualOverflow,
                                    logicalRightVisualOverflow - logicalLeftVisualOverflow, logicalBottomVisualOverflow - logicalTopVisualOverflow);
}

inline void InlineFlowBox::addTextBoxVisualOverflow(InlineTextBox* textBox, GlyphOverflowAndFallbackFontsMap& textBoxDataMap, IntRect& logicalVisualOverflow)
{
    if (textBox->knownToHaveNoOverflow())
        return;

    RenderStyle* style = textBox->renderer()->style(m_firstLine);
    
    GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.find(textBox);
    GlyphOverflow* glyphOverflow = it == textBoxDataMap.end() ? 0 : &it->second.second;
    bool isFlippedLine = style->isFlippedLinesWritingMode();

    int topGlyphEdge = glyphOverflow ? (isFlippedLine ? glyphOverflow->bottom : glyphOverflow->top) : 0;
    int bottomGlyphEdge = glyphOverflow ? (isFlippedLine ? glyphOverflow->top : glyphOverflow->bottom) : 0;
    int leftGlyphEdge = glyphOverflow ? glyphOverflow->left : 0;
    int rightGlyphEdge = glyphOverflow ? glyphOverflow->right : 0;

    int strokeOverflow = static_cast<int>(ceilf(style->textStrokeWidth() / 2.0f));
    int topGlyphOverflow = -strokeOverflow - topGlyphEdge;
    int bottomGlyphOverflow = strokeOverflow + bottomGlyphEdge;
    int leftGlyphOverflow = -strokeOverflow - leftGlyphEdge;
    int rightGlyphOverflow = strokeOverflow + rightGlyphEdge;

    TextEmphasisPosition emphasisMarkPosition;
    if (style->textEmphasisMark() != TextEmphasisMarkNone && textBox->getEmphasisMarkPosition(style, emphasisMarkPosition)) {
        int emphasisMarkHeight = style->font().emphasisMarkHeight(style->textEmphasisMarkString());
        if ((emphasisMarkPosition == TextEmphasisPositionOver) == (!style->isFlippedLinesWritingMode()))
            topGlyphOverflow = min(topGlyphOverflow, -emphasisMarkHeight);
        else
            bottomGlyphOverflow = max(bottomGlyphOverflow, emphasisMarkHeight);
    }

    // If letter-spacing is negative, we should factor that into right layout overflow. (Even in RTL, letter-spacing is
    // applied to the right, so this is not an issue with left overflow.
    rightGlyphOverflow -= min(0, (int)style->font().letterSpacing());

    int textShadowLogicalTop;
    int textShadowLogicalBottom;
    style->getTextShadowBlockDirectionExtent(textShadowLogicalTop, textShadowLogicalBottom);
    
    int childOverflowLogicalTop = min(textShadowLogicalTop + topGlyphOverflow, topGlyphOverflow);
    int childOverflowLogicalBottom = max(textShadowLogicalBottom + bottomGlyphOverflow, bottomGlyphOverflow);
   
    int textShadowLogicalLeft;
    int textShadowLogicalRight;
    style->getTextShadowInlineDirectionExtent(textShadowLogicalLeft, textShadowLogicalRight);
   
    int childOverflowLogicalLeft = min(textShadowLogicalLeft + leftGlyphOverflow, leftGlyphOverflow);
    int childOverflowLogicalRight = max(textShadowLogicalRight + rightGlyphOverflow, rightGlyphOverflow);

    int logicalTopVisualOverflow = min(textBox->logicalTop() + childOverflowLogicalTop, logicalVisualOverflow.y());
    int logicalBottomVisualOverflow = max(textBox->logicalBottom() + childOverflowLogicalBottom, logicalVisualOverflow.maxY());
    int logicalLeftVisualOverflow = min(textBox->pixelSnappedLogicalLeft() + childOverflowLogicalLeft, logicalVisualOverflow.x());
    int logicalRightVisualOverflow = max(textBox->pixelSnappedLogicalRight() + childOverflowLogicalRight, logicalVisualOverflow.maxX());
    
    logicalVisualOverflow = IntRect(logicalLeftVisualOverflow, logicalTopVisualOverflow,
                                    logicalRightVisualOverflow - logicalLeftVisualOverflow, logicalBottomVisualOverflow - logicalTopVisualOverflow);
                                    
    textBox->setLogicalOverflowRect(logicalVisualOverflow);
}

inline void InlineFlowBox::addReplacedChildOverflow(const InlineBox* inlineBox, IntRect& logicalLayoutOverflow, IntRect& logicalVisualOverflow)
{
    RenderBox* box = toRenderBox(inlineBox->renderer());
    
    // Visual overflow only propagates if the box doesn't have a self-painting layer.  This rectangle does not include
    // transforms or relative positioning (since those objects always have self-painting layers), but it does need to be adjusted
    // for writing-mode differences.
    if (!box->hasSelfPaintingLayer()) {
        IntRect childLogicalVisualOverflow = box->logicalVisualOverflowRectForPropagation(renderer()->style());
        childLogicalVisualOverflow.move(inlineBox->logicalLeft(), inlineBox->logicalTop());
        logicalVisualOverflow.unite(childLogicalVisualOverflow);
    }

    // Layout overflow internal to the child box only propagates if the child box doesn't have overflow clip set.
    // Otherwise the child border box propagates as layout overflow.  This rectangle must include transforms and relative positioning
    // and be adjusted for writing-mode differences.
    IntRect childLogicalLayoutOverflow = box->logicalLayoutOverflowRectForPropagation(renderer()->style());
    childLogicalLayoutOverflow.move(inlineBox->logicalLeft(), inlineBox->logicalTop());
    logicalLayoutOverflow.unite(childLogicalLayoutOverflow);
}

void InlineFlowBox::computeOverflow(int lineTop, int lineBottom, GlyphOverflowAndFallbackFontsMap& textBoxDataMap)
{
    // If we know we have no overflow, we can just bail.
    if (knownToHaveNoOverflow())
        return;

    // Visual overflow just includes overflow for stuff we need to repaint ourselves.  Self-painting layers are ignored.
    // Layout overflow is used to determine scrolling extent, so it still includes child layers and also factors in
    // transforms, relative positioning, etc.
    IntRect logicalLayoutOverflow(enclosingIntRect(logicalFrameRectIncludingLineHeight(lineTop, lineBottom)));
    IntRect logicalVisualOverflow(logicalLayoutOverflow);
  
    // box-shadow on root line boxes is applying to the block and not to the lines.
    addBoxShadowVisualOverflow(logicalVisualOverflow);

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->renderer()->isText()) {
            InlineTextBox* text = static_cast<InlineTextBox*>(curr);
            RenderText* rt = toRenderText(text->renderer());
            if (rt->isBR())
                continue;
            IntRect textBoxOverflow(enclosingIntRect(text->logicalFrameRect()));
            addTextBoxVisualOverflow(text, textBoxDataMap, textBoxOverflow);
            logicalVisualOverflow.unite(textBoxOverflow);
        } else  if (curr->renderer()->isRenderInline()) {
            InlineFlowBox* flow = static_cast<InlineFlowBox*>(curr);
            flow->computeOverflow(lineTop, lineBottom, textBoxDataMap);
            if (!flow->boxModelObject()->hasSelfPaintingLayer())
                logicalVisualOverflow.unite(flow->logicalVisualOverflowRect(lineTop, lineBottom));
            IntRect childLayoutOverflow = flow->logicalLayoutOverflowRect(lineTop, lineBottom);
            childLayoutOverflow.move(flow->boxModelObject()->relativePositionLogicalOffset());
            logicalLayoutOverflow.unite(childLayoutOverflow);
        } else
            addReplacedChildOverflow(curr, logicalLayoutOverflow, logicalVisualOverflow);
    }
    
    setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, lineTop, lineBottom);
}

void InlineFlowBox::setLayoutOverflow(const IntRect& rect, int lineTop, int lineBottom)
{
    IntRect frameBox = enclosingIntRect(frameRectIncludingLineHeight(lineTop, lineBottom));
    if (frameBox.contains(rect) || rect.isEmpty())
        return;

    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(frameBox, frameBox));
    
    m_overflow->setLayoutOverflow(rect);
}

void InlineFlowBox::setVisualOverflow(const IntRect& rect, int lineTop, int lineBottom)
{
    IntRect frameBox = enclosingIntRect(frameRectIncludingLineHeight(lineTop, lineBottom));
    if (frameBox.contains(rect) || rect.isEmpty())
        return;
        
    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(frameBox, frameBox));
    
    m_overflow->setVisualOverflow(rect);
}

void InlineFlowBox::setOverflowFromLogicalRects(const IntRect& logicalLayoutOverflow, const IntRect& logicalVisualOverflow, int lineTop, int lineBottom)
{
    IntRect layoutOverflow(isHorizontal() ? logicalLayoutOverflow : logicalLayoutOverflow.transposedRect());
    setLayoutOverflow(layoutOverflow, lineTop, lineBottom);
    
    IntRect visualOverflow(isHorizontal() ? logicalVisualOverflow : logicalVisualOverflow.transposedRect());
    setVisualOverflow(visualOverflow, lineTop, lineBottom);
}

bool InlineFlowBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, int lineTop, int lineBottom)
{
    IntRect overflowRect(visualOverflowRect(lineTop, lineBottom));
    flipForWritingMode(overflowRect);
    overflowRect.move(tx, ty);
    if (!overflowRect.intersects(result.rectForPoint(x, y)))
        return false;

    // Check children first.
    for (InlineBox* curr = lastChild(); curr; curr = curr->prevOnLine()) {
        if ((curr->renderer()->isText() || !curr->boxModelObject()->hasSelfPaintingLayer()) && curr->nodeAtPoint(request, result, x, y, tx, ty, lineTop, lineBottom)) {
            renderer()->updateHitTestResult(result, IntPoint(x - tx, y - ty));
            return true;
        }
    }

    // Now check ourselves. Pixel snap hit testing.
    IntRect frameRect = roundedFrameRect();
    int minX = frameRect.x();
    int minY = frameRect.y();
    int width = frameRect.width();
    int height = frameRect.height();

    // Constrain our hit testing to the line top and bottom if necessary.
    bool noQuirksMode = renderer()->document()->inNoQuirksMode();
    if (!noQuirksMode && !hasTextChildren() && !(descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
        RootInlineBox* rootBox = root();
        int& top = isHorizontal() ? minY : minX;
        int& logicalHeight = isHorizontal() ? height : width;
        int bottom = min(rootBox->lineBottom(), top + logicalHeight);
        top = max(rootBox->lineTop(), top);
        logicalHeight = bottom - top;
    }
    
    // Move x/y to our coordinates.
    IntRect rect(minX, minY, width, height);
    flipForWritingMode(rect);
    rect.move(tx, ty);

    if (visibleToHitTesting() && rect.intersects(result.rectForPoint(x, y))) {
        renderer()->updateHitTestResult(result, flipForWritingMode(IntPoint(x - tx, y - ty))); // Don't add in m_x or m_y here, we want coords in the containing block's space.
        if (!result.addNodeToRectBasedTestResult(renderer()->node(), x, y, rect))
            return true;
    }
    
    return false;
}

void InlineFlowBox::paint(PaintInfo& paintInfo, int tx, int ty, int lineTop, int lineBottom)
{
    IntRect overflowRect(visualOverflowRect(lineTop, lineBottom));
    overflowRect.inflate(renderer()->maximalOutlineSize(paintInfo.phase));
    flipForWritingMode(overflowRect);
    overflowRect.move(tx, ty);
    
    if (!paintInfo.rect.intersects(overflowRect))
        return;

    if (paintInfo.phase != PaintPhaseChildOutlines) {
        if (paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) {
            // Add ourselves to the paint info struct's list of inlines that need to paint their
            // outlines.
            if (renderer()->style()->visibility() == VISIBLE && renderer()->hasOutline() && !isRootInlineBox()) {
                RenderInline* inlineFlow = toRenderInline(renderer());

                RenderBlock* cb = 0;
                bool containingBlockPaintsContinuationOutline = inlineFlow->continuation() || inlineFlow->isInlineElementContinuation();
                if (containingBlockPaintsContinuationOutline) {           
                    // FIXME: See https://bugs.webkit.org/show_bug.cgi?id=54690. We currently don't reconnect inline continuations
                    // after a child removal. As a result, those merged inlines do not get seperated and hence not get enclosed by
                    // anonymous blocks. In this case, it is better to bail out and paint it ourself.
                    RenderBlock* enclosingAnonymousBlock = renderer()->containingBlock();
                    if (!enclosingAnonymousBlock->isAnonymousBlock())
                        containingBlockPaintsContinuationOutline = false;
                    else {
                        cb = enclosingAnonymousBlock->containingBlock();
                        for (RenderBoxModelObject* box = boxModelObject(); box != cb; box = box->parent()->enclosingBoxModelObject()) {
                            if (box->hasSelfPaintingLayer()) {
                                containingBlockPaintsContinuationOutline = false;
                                break;
                            }
                        }
                    }
                }

                if (containingBlockPaintsContinuationOutline) {
                    // Add ourselves to the containing block of the entire continuation so that it can
                    // paint us atomically.
                    cb->addContinuationWithOutline(toRenderInline(renderer()->node()->renderer()));
                } else if (!inlineFlow->isInlineElementContinuation())
                    paintInfo.outlineObjects->add(inlineFlow);
            }
        } else if (paintInfo.phase == PaintPhaseMask) {
            paintMask(paintInfo, tx, ty);
            return;
        } else {
            // Paint our background, border and box-shadow.
            paintBoxDecorations(paintInfo, tx, ty);
        }
    }

    if (paintInfo.phase == PaintPhaseMask)
        return;

    PaintPhase paintPhase = paintInfo.phase == PaintPhaseChildOutlines ? PaintPhaseOutline : paintInfo.phase;
    PaintInfo childInfo(paintInfo);
    childInfo.phase = paintPhase;
    childInfo.updatePaintingRootForChildren(renderer());
    
    // Paint our children.
    if (paintPhase != PaintPhaseSelfOutline) {
        for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
            if (curr->renderer()->isText() || !curr->boxModelObject()->hasSelfPaintingLayer())
                curr->paint(childInfo, tx, ty, lineTop, lineBottom);
        }
    }
}

void InlineFlowBox::paintFillLayers(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, int _tx, int _ty, int w, int h, CompositeOperator op)
{
    if (!fillLayer)
        return;
    paintFillLayers(paintInfo, c, fillLayer->next(), _tx, _ty, w, h, op);
    paintFillLayer(paintInfo, c, fillLayer, _tx, _ty, w, h, op);
}

void InlineFlowBox::paintFillLayer(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, int tx, int ty, int w, int h, CompositeOperator op)
{
    StyleImage* img = fillLayer->image();
    bool hasFillImage = img && img->canRender(renderer()->style()->effectiveZoom());
    if ((!hasFillImage && !renderer()->style()->hasBorderRadius()) || (!prevLineBox() && !nextLineBox()) || !parent())
        boxModelObject()->paintFillLayerExtended(paintInfo, c, fillLayer, tx, ty, w, h, BackgroundBleedNone, this, w, h, op);
    else {
        // We have a fill image that spans multiple lines.
        // We need to adjust tx and ty by the width of all previous lines.
        // Think of background painting on inlines as though you had one long line, a single continuous
        // strip.  Even though that strip has been broken up across multiple lines, you still paint it
        // as though you had one single line.  This means each line has to pick up the background where
        // the previous line left off.
        int logicalOffsetOnLine = 0;
        int totalLogicalWidth;
        if (renderer()->style()->direction() == LTR) {
            for (InlineFlowBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
                logicalOffsetOnLine += curr->logicalWidth();
            totalLogicalWidth = logicalOffsetOnLine;
            for (InlineFlowBox* curr = this; curr; curr = curr->nextLineBox())
                totalLogicalWidth += curr->logicalWidth();
        } else {
            for (InlineFlowBox* curr = nextLineBox(); curr; curr = curr->nextLineBox())
                logicalOffsetOnLine += curr->logicalWidth();
            totalLogicalWidth = logicalOffsetOnLine;
            for (InlineFlowBox* curr = this; curr; curr = curr->prevLineBox())
                totalLogicalWidth += curr->logicalWidth();
        }
        int stripX = tx - (isHorizontal() ? logicalOffsetOnLine : 0);
        int stripY = ty - (isHorizontal() ? 0 : logicalOffsetOnLine);
        int stripWidth = isHorizontal() ? totalLogicalWidth : width();
        int stripHeight = isHorizontal() ? height() : totalLogicalWidth;

        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        paintInfo.context->clip(IntRect(tx, ty, width(), height()));
        boxModelObject()->paintFillLayerExtended(paintInfo, c, fillLayer, stripX, stripY, stripWidth, stripHeight, BackgroundBleedNone, this, w, h, op);
    }
}

void InlineFlowBox::paintBoxShadow(GraphicsContext* context, RenderStyle* s, ShadowStyle shadowStyle, int tx, int ty, int w, int h)
{
    if ((!prevLineBox() && !nextLineBox()) || !parent())
        boxModelObject()->paintBoxShadow(context, tx, ty, w, h, s, shadowStyle);
    else {
        // FIXME: We can do better here in the multi-line case. We want to push a clip so that the shadow doesn't
        // protrude incorrectly at the edges, and we want to possibly include shadows cast from the previous/following lines
        boxModelObject()->paintBoxShadow(context, tx, ty, w, h, s, shadowStyle, includeLogicalLeftEdge(), includeLogicalRightEdge());
    }
}

void InlineFlowBox::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(renderer()) || renderer()->style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseForeground)
        return;

    // Pixel snap background/border painting.
    IntRect frameRect = roundedFrameRect();
    int x = frameRect.x();
    int y = frameRect.y();
    int w = frameRect.width();
    int h = frameRect.height();

    // Constrain our background/border painting to the line top and bottom if necessary.
    bool noQuirksMode = renderer()->document()->inNoQuirksMode();
    if (!noQuirksMode && !hasTextChildren() && !(descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
        RootInlineBox* rootBox = root();
        int& top = isHorizontal() ? y : x;
        int& logicalHeight = isHorizontal() ? h : w;
        int bottom = min(rootBox->lineBottom(), top + logicalHeight);
        top = max(rootBox->lineTop(), top);
        logicalHeight = bottom - top;
    }
    
    // Move x/y to our coordinates.
    IntRect localRect(x, y, w, h);
    flipForWritingMode(localRect);
    tx += localRect.x();
    ty += localRect.y();
    
    GraphicsContext* context = paintInfo.context;
    
    // You can use p::first-line to specify a background. If so, the root line boxes for
    // a line may actually have to paint a background.
    RenderStyle* styleToUse = renderer()->style(m_firstLine);
    if ((!parent() && m_firstLine && styleToUse != renderer()->style()) || (parent() && renderer()->hasBoxDecorations())) {
        // Shadow comes first and is behind the background and border.
        paintBoxShadow(context, styleToUse, Normal, tx, ty, w, h);

        Color c = styleToUse->visitedDependentColor(CSSPropertyBackgroundColor);
        paintFillLayers(paintInfo, c, styleToUse->backgroundLayers(), tx, ty, w, h);
        paintBoxShadow(context, styleToUse, Inset, tx, ty, w, h);

        // :first-line cannot be used to put borders on a line. Always paint borders with our
        // non-first-line style.
        if (parent() && renderer()->style()->hasBorder()) {
            StyleImage* borderImage = renderer()->style()->borderImage().image();
            bool hasBorderImage = borderImage && borderImage->canRender(styleToUse->effectiveZoom());
            if (hasBorderImage && !borderImage->isLoaded())
                return; // Don't paint anything while we wait for the image to load.

            // The simple case is where we either have no border image or we are the only box for this object.  In those
            // cases only a single call to draw is required.
            if (!hasBorderImage || (!prevLineBox() && !nextLineBox()))
                boxModelObject()->paintBorder(context, tx, ty, w, h, renderer()->style(), BackgroundBleedNone, includeLogicalLeftEdge(), includeLogicalRightEdge());
            else {
                // We have a border image that spans multiple lines.
                // We need to adjust tx and ty by the width of all previous lines.
                // Think of border image painting on inlines as though you had one long line, a single continuous
                // strip.  Even though that strip has been broken up across multiple lines, you still paint it
                // as though you had one single line.  This means each line has to pick up the image where
                // the previous line left off.
                // FIXME: What the heck do we do with RTL here? The math we're using is obviously not right,
                // but it isn't even clear how this should work at all.
                int logicalOffsetOnLine = 0;
                for (InlineFlowBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
                    logicalOffsetOnLine += curr->logicalWidth();
                int totalLogicalWidth = logicalOffsetOnLine;
                for (InlineFlowBox* curr = this; curr; curr = curr->nextLineBox())
                    totalLogicalWidth += curr->logicalWidth();
                int stripX = tx - (isHorizontal() ? logicalOffsetOnLine : 0);
                int stripY = ty - (isHorizontal() ? 0 : logicalOffsetOnLine);
                int stripWidth = isHorizontal() ? totalLogicalWidth : w;
                int stripHeight = isHorizontal() ? h : totalLogicalWidth;

                GraphicsContextStateSaver stateSaver(*context);
                context->clip(IntRect(tx, ty, w, h));
                boxModelObject()->paintBorder(context, stripX, stripY, stripWidth, stripHeight, renderer()->style());
            }
        }
    }
}

void InlineFlowBox::paintMask(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(renderer()) || renderer()->style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    // Pixel snap mask painting.
    IntRect frameRect = roundedFrameRect();
    int x = frameRect.x();
    int y = frameRect.y();
    int w = frameRect.width();
    int h = frameRect.height();

    // Constrain our background/border painting to the line top and bottom if necessary.
    bool noQuirksMode = renderer()->document()->inNoQuirksMode();
    if (!noQuirksMode && !hasTextChildren() && !(descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
        RootInlineBox* rootBox = root();
        int& top = isHorizontal() ? y : x;
        int& logicalHeight = isHorizontal() ? h : w;
        int bottom = min(rootBox->lineBottom(), top + logicalHeight);
        top = max(rootBox->lineTop(), top);
        logicalHeight = bottom - top;
    }
    
    // Move x/y to our coordinates.
    IntRect localRect(x, y, w, h);
    flipForWritingMode(localRect);
    tx += localRect.x();
    ty += localRect.y();

    const NinePieceImage& maskNinePieceImage = renderer()->style()->maskBoxImage();
    StyleImage* maskBoxImage = renderer()->style()->maskBoxImage().image();

    // Figure out if we need to push a transparency layer to render our mask.
    bool pushTransparencyLayer = false;
    bool compositedMask = renderer()->hasLayer() && boxModelObject()->layer()->hasCompositedMask();
    CompositeOperator compositeOp = CompositeSourceOver;
    if (!compositedMask) {
        if ((maskBoxImage && renderer()->style()->maskLayers()->hasImage()) || renderer()->style()->maskLayers()->next())
            pushTransparencyLayer = true;
        
        compositeOp = CompositeDestinationIn;
        if (pushTransparencyLayer) {
            paintInfo.context->setCompositeOperation(CompositeDestinationIn);
            paintInfo.context->beginTransparencyLayer(1.0f);
            compositeOp = CompositeSourceOver;
        }
    }

    paintFillLayers(paintInfo, Color(), renderer()->style()->maskLayers(), tx, ty, w, h, compositeOp);
    
    bool hasBoxImage = maskBoxImage && maskBoxImage->canRender(renderer()->style()->effectiveZoom());
    if (!hasBoxImage || !maskBoxImage->isLoaded())
        return; // Don't paint anything while we wait for the image to load.

    // The simple case is where we are the only box for this object.  In those
    // cases only a single call to draw is required.
    if (!prevLineBox() && !nextLineBox()) {
        boxModelObject()->paintNinePieceImage(paintInfo.context, tx, ty, w, h, renderer()->style(), maskNinePieceImage, compositeOp);
    } else {
        // We have a mask image that spans multiple lines.
        // We need to adjust _tx and _ty by the width of all previous lines.
        int logicalOffsetOnLine = 0;
        for (InlineFlowBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
            logicalOffsetOnLine += curr->logicalWidth();
        int totalLogicalWidth = logicalOffsetOnLine;
        for (InlineFlowBox* curr = this; curr; curr = curr->nextLineBox())
            totalLogicalWidth += curr->logicalWidth();
        int stripX = tx - (isHorizontal() ? logicalOffsetOnLine : 0);
        int stripY = ty - (isHorizontal() ? 0 : logicalOffsetOnLine);
        int stripWidth = isHorizontal() ? totalLogicalWidth : w;
        int stripHeight = isHorizontal() ? h : totalLogicalWidth;

        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        paintInfo.context->clip(IntRect(tx, ty, w, h));
        boxModelObject()->paintNinePieceImage(paintInfo.context, stripX, stripY, stripWidth, stripHeight, renderer()->style(), maskNinePieceImage, compositeOp);
    }
    
    if (pushTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

InlineBox* InlineFlowBox::firstLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* child = firstChild(); child && !leaf; child = child->nextOnLine())
        leaf = child->isLeaf() ? child : static_cast<InlineFlowBox*>(child)->firstLeafChild();
    return leaf;
}

InlineBox* InlineFlowBox::lastLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* child = lastChild(); child && !leaf; child = child->prevOnLine())
        leaf = child->isLeaf() ? child : static_cast<InlineFlowBox*>(child)->lastLeafChild();
    return leaf;
}

RenderObject::SelectionState InlineFlowBox::selectionState()
{
    return RenderObject::SelectionNone;
}

bool InlineFlowBox::canAccommodateEllipsis(bool ltr, int blockEdge, int ellipsisWidth)
{
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine()) {
        if (!box->canAccommodateEllipsis(ltr, blockEdge, ellipsisWidth))
            return false;
    }
    return true;
}

float InlineFlowBox::placeEllipsisBox(bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, bool& foundBox)
{
    float result = -1;
    // We iterate over all children, the foundBox variable tells us when we've found the
    // box containing the ellipsis.  All boxes after that one in the flow are hidden.
    // If our flow is ltr then iterate over the boxes from left to right, otherwise iterate
    // from right to left. Varying the order allows us to correctly hide the boxes following the ellipsis.
    InlineBox* box = ltr ? firstChild() : lastChild();

    // NOTE: these will cross after foundBox = true.
    int visibleLeftEdge = blockLeftEdge;
    int visibleRightEdge = blockRightEdge;

    while (box) {
        int currResult = box->placeEllipsisBox(ltr, visibleLeftEdge, visibleRightEdge, ellipsisWidth, foundBox);
        if (currResult != -1 && result == -1)
            result = currResult;

        if (ltr) {
            visibleLeftEdge += box->logicalWidth();
            box = box->nextOnLine();
        }
        else {
            visibleRightEdge -= box->logicalWidth();
            box = box->prevOnLine();
        }
    }
    return result;
}

void InlineFlowBox::clearTruncation()
{
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine())
        box->clearTruncation();
}

int InlineFlowBox::computeOverAnnotationAdjustment(int allowedPosition) const
{
    int result = 0;
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->isInlineFlowBox())
            result = max(result, static_cast<InlineFlowBox*>(curr)->computeOverAnnotationAdjustment(allowedPosition));
        
        if (curr->renderer()->isReplaced() && curr->renderer()->isRubyRun()) {
            RenderRubyRun* rubyRun = static_cast<RenderRubyRun*>(curr->renderer());
            RenderRubyText* rubyText = rubyRun->rubyText();
            if (!rubyText)
                continue;
            
            if (!rubyRun->style()->isFlippedLinesWritingMode()) {
                int topOfFirstRubyTextLine = rubyText->logicalTop() + (rubyText->firstRootBox() ? rubyText->firstRootBox()->lineTop() : 0);
                if (topOfFirstRubyTextLine >= 0)
                    continue;
                topOfFirstRubyTextLine += curr->logicalTop();
                result = max(result, allowedPosition - topOfFirstRubyTextLine);
            } else {
                int bottomOfLastRubyTextLine = rubyText->logicalTop() + (rubyText->lastRootBox() ? rubyText->lastRootBox()->lineBottom() : rubyText->logicalHeight());
                if (bottomOfLastRubyTextLine <= curr->logicalHeight())
                    continue;
                bottomOfLastRubyTextLine += curr->logicalTop();
                result = max(result, bottomOfLastRubyTextLine - allowedPosition);
            }
        }

        if (curr->isInlineTextBox()) {
            RenderStyle* style = curr->renderer()->style(m_firstLine);
            TextEmphasisPosition emphasisMarkPosition;
            if (style->textEmphasisMark() != TextEmphasisMarkNone && static_cast<InlineTextBox*>(curr)->getEmphasisMarkPosition(style, emphasisMarkPosition) && emphasisMarkPosition == TextEmphasisPositionOver) {
                if (!style->isFlippedLinesWritingMode()) {
                    int topOfEmphasisMark = curr->logicalTop() - style->font().emphasisMarkHeight(style->textEmphasisMarkString());
                    result = max(result, allowedPosition - topOfEmphasisMark);
                } else {
                    int bottomOfEmphasisMark = curr->logicalBottom() + style->font().emphasisMarkHeight(style->textEmphasisMarkString());
                    result = max(result, bottomOfEmphasisMark - allowedPosition);
                }
            }
        }
    }
    return result;
}

int InlineFlowBox::computeUnderAnnotationAdjustment(int allowedPosition) const
{
    int result = 0;
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (curr->isInlineFlowBox())
            result = max(result, static_cast<InlineFlowBox*>(curr)->computeUnderAnnotationAdjustment(allowedPosition));

        if (curr->isInlineTextBox()) {
            RenderStyle* style = curr->renderer()->style(m_firstLine);
            if (style->textEmphasisMark() != TextEmphasisMarkNone && style->textEmphasisPosition() == TextEmphasisPositionUnder) {
                if (!style->isFlippedLinesWritingMode()) {
                    int bottomOfEmphasisMark = curr->logicalBottom() + style->font().emphasisMarkHeight(style->textEmphasisMarkString());
                    result = max(result, bottomOfEmphasisMark - allowedPosition);
                } else {
                    int topOfEmphasisMark = curr->logicalTop() - style->font().emphasisMarkHeight(style->textEmphasisMarkString());
                    result = max(result, allowedPosition - topOfEmphasisMark);
                }
            }
        }
    }
    return result;
}

void InlineFlowBox::collectLeafBoxesInLogicalOrder(Vector<InlineBox*>& leafBoxesInLogicalOrder, CustomInlineBoxRangeReverse customReverseImplementation, void* userData) const
{
    InlineBox* leaf = firstLeafChild();

    // FIXME: The reordering code is a copy of parts from BidiResolver::createBidiRunsForLine, operating directly on InlineBoxes, instead of BidiRuns.
    // Investigate on how this code could possibly be shared.
    unsigned char minLevel = 128;
    unsigned char maxLevel = 0;

    // First find highest and lowest levels, and initialize leafBoxesInLogicalOrder with the leaf boxes in visual order.
    for (; leaf; leaf = leaf->nextLeafChild()) {
        minLevel = min(minLevel, leaf->bidiLevel());
        maxLevel = max(maxLevel, leaf->bidiLevel());
        leafBoxesInLogicalOrder.append(leaf);
    }

    if (renderer()->style()->visuallyOrdered())
        return;

    // Reverse of reordering of the line (L2 according to Bidi spec):
    // L2. From the highest level found in the text to the lowest odd level on each line,
    // reverse any contiguous sequence of characters that are at that level or higher.

    // Reversing the reordering of the line is only done up to the lowest odd level.
    if (!(minLevel % 2))
        ++minLevel;

    Vector<InlineBox*>::iterator end = leafBoxesInLogicalOrder.end();
    while (minLevel <= maxLevel) {
        Vector<InlineBox*>::iterator it = leafBoxesInLogicalOrder.begin();
        while (it != end) {
            while (it != end) {
                if ((*it)->bidiLevel() >= minLevel)
                    break;
                ++it;
            }
            Vector<InlineBox*>::iterator first = it;
            while (it != end) {
                if ((*it)->bidiLevel() < minLevel)
                    break;
                ++it;
            }
            Vector<InlineBox*>::iterator last = it;
            if (customReverseImplementation) {
                ASSERT(userData);
                (*customReverseImplementation)(userData, first, last);
            } else
                std::reverse(first, last);
        }                
        ++minLevel;
    }
}

#ifndef NDEBUG

const char* InlineFlowBox::boxName() const
{
    return "InlineFlowBox";
}

void InlineFlowBox::showLineTreeAndMark(const InlineBox* markedBox1, const char* markedLabel1, const InlineBox* markedBox2, const char* markedLabel2, const RenderObject* obj, int depth) const
{
    InlineBox::showLineTreeAndMark(markedBox1, markedLabel1, markedBox2, markedLabel2, obj, depth);
    for (const InlineBox* box = firstChild(); box; box = box->nextOnLine())
        box->showLineTreeAndMark(markedBox1, markedLabel1, markedBox2, markedLabel2, obj, depth + 1);
}

void InlineFlowBox::checkConsistency() const
{
#ifdef CHECK_CONSISTENCY
    ASSERT(!m_hasBadChildList);
    const InlineBox* prev = 0;
    for (const InlineBox* child = m_firstChild; child; child = child->nextOnLine()) {
        ASSERT(child->parent() == this);
        ASSERT(child->prevOnLine() == prev);
        prev = child;
    }
    ASSERT(prev == m_lastChild);
#endif
}

#endif

} // namespace WebCore
