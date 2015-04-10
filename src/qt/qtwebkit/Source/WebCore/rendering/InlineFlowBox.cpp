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

#include "CSSPropertyNames.h"
#include "Document.h"
#include "EllipsisBox.h"
#include "Font.h"
#include "GraphicsContext.h"
#include "InlineTextBox.h"
#include "HitTestResult.h"
#include "RenderBlock.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListMarker.h"
#include "RenderRubyBase.h"
#include "RenderRubyRun.h"
#include "RenderRubyText.h"
#include "RenderTableCell.h"
#include "RenderView.h"
#include "RootInlineBox.h"
#include "Text.h"

#include <math.h>

using namespace std;

namespace WebCore {

struct SameSizeAsInlineFlowBox : public InlineBox {
    void* pointers[5];
    uint32_t bitfields : 23;
};

COMPILE_ASSERT(sizeof(InlineFlowBox) == sizeof(SameSizeAsInlineFlowBox), InlineFlowBox_should_stay_small);

#ifndef NDEBUG

InlineFlowBox::~InlineFlowBox()
{
    if (!m_hasBadChildList)
        for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
            child->setHasBadParent();
}

#endif

LayoutUnit InlineFlowBox::getFlowSpacingLogicalWidth()
{
    LayoutUnit totWidth = marginBorderPaddingLogicalLeft() + marginBorderPaddingLogicalRight();
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->isInlineFlowBox())
            totWidth += toInlineFlowBox(curr)->getFlowSpacingLogicalWidth();
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

static void setHasTextDescendantsOnAncestors(InlineFlowBox* box)
{
    while (box && !box->hasTextDescendants()) {
        box->setHasTextDescendants();
        box = box->parent();
    }
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
    child->setFirstLineStyleBit(isFirstLineStyle());
    child->setIsHorizontal(isHorizontal());
    if (child->isText()) {
        if (child->renderer()->parent() == renderer())
            m_hasTextChildren = true;
        setHasTextDescendantsOnAncestors(this);
    } else if (child->isInlineFlowBox()) {
        if (toInlineFlowBox(child)->hasTextDescendants())
            setHasTextDescendantsOnAncestors(this);
    }

    if (descendantsHaveSameLineHeightAndBaseline() && !child->renderer()->isOutOfFlowPositioned()) {
        RenderStyle* parentStyle = renderer()->style(isFirstLineStyle());
        RenderStyle* childStyle = child->renderer()->style(isFirstLineStyle());
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
                InlineFlowBox* childFlowBox = toInlineFlowBox(child);
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

    if (!child->renderer()->isOutOfFlowPositioned()) {
        if (child->isText()) {
            RenderStyle* childStyle = child->renderer()->style(isFirstLineStyle());
            if (childStyle->letterSpacing() < 0 || childStyle->textShadow() || childStyle->textEmphasisMark() != TextEmphasisMarkNone || childStyle->textStrokeWidth())
                child->clearKnownToHaveNoOverflow();
        } else if (child->renderer()->isReplaced()) {
            RenderBox* box = toRenderBox(child->renderer());
            if (box->hasRenderOverflow() || box->hasSelfPaintingLayer())
                child->clearKnownToHaveNoOverflow();
        } else if (!child->renderer()->isBR() && (child->renderer()->style(isFirstLineStyle())->boxShadow() || child->boxModelObject()->hasSelfPaintingLayer()
                   || (child->renderer()->isListMarker() && !toRenderListMarker(child->renderer())->isInside())
                   || child->renderer()->style(isFirstLineStyle())->hasBorderImageOutsets()))
            child->clearKnownToHaveNoOverflow();
        
        if (knownToHaveNoOverflow() && child->isInlineFlowBox() && !toInlineFlowBox(child)->knownToHaveNoOverflow())
            clearKnownToHaveNoOverflow();
    }

    checkConsistency();
}

void InlineFlowBox::removeChild(InlineBox* child)
{
    checkConsistency();

    if (!isDirty())
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
    if (!extracted())
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
    if (extracted())
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
#if ENABLE(CSS_BOX_DECORATION_BREAK)
            if (renderer()->style()->boxDecorationBreak() == DCLONE)
                includeLeftEdge = includeRightEdge = true;
            else
#endif
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
#if ENABLE(CSS_BOX_DECORATION_BREAK)
            // (4) The decoration break is set to clone therefore there will be borders on every sides.
            if (renderer()->style()->boxDecorationBreak() == DCLONE)
                includeLeftEdge = includeRightEdge = true;
            else
#endif
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
            InlineFlowBox* currFlow = toInlineFlowBox(currChild);
            currFlow->determineSpacingForFlowBoxes(lastLine, isLogicallyLastRunWrapped, logicallyLastRunRenderer);
        }
    }
}

float InlineFlowBox::placeBoxesInInlineDirection(float logicalLeft, bool& needsWordSpacing, GlyphOverflowAndFallbackFontsMap& textBoxDataMap)
{
    // Set our x position.
    beginPlacingBoxRangesInInlineDirection(logicalLeft);

    float startLogicalLeft = logicalLeft;
    logicalLeft += borderLogicalLeft() + paddingLogicalLeft();

    float minLogicalLeft = startLogicalLeft;
    float maxLogicalRight = logicalLeft;

    placeBoxRangeInInlineDirection(firstChild(), 0, logicalLeft, minLogicalLeft, maxLogicalRight, needsWordSpacing, textBoxDataMap);

    logicalLeft += borderLogicalRight() + paddingLogicalRight();
    endPlacingBoxRangesInInlineDirection(startLogicalLeft, logicalLeft, minLogicalLeft, maxLogicalRight);
    return logicalLeft;
}

float InlineFlowBox::placeBoxRangeInInlineDirection(InlineBox* firstChild, InlineBox* lastChild, float& logicalLeft, float& minLogicalLeft, float& maxLogicalRight, bool& needsWordSpacing, GlyphOverflowAndFallbackFontsMap& textBoxDataMap)
{
    for (InlineBox* curr = firstChild; curr && curr != lastChild; curr = curr->nextOnLine()) {
        if (curr->renderer()->isText()) {
            InlineTextBox* text = toInlineTextBox(curr);
            RenderText* rt = toRenderText(text->renderer());
            if (rt->textLength()) {
                if (needsWordSpacing && isSpaceOrNewline(rt->characterAt(text->start())))
                    logicalLeft += rt->style(isFirstLineStyle())->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characterAt(text->end()));
            }
            text->setLogicalLeft(logicalLeft);
            if (knownToHaveNoOverflow())
                minLogicalLeft = min(logicalLeft, minLogicalLeft);
            logicalLeft += text->logicalWidth();
            if (knownToHaveNoOverflow())
                maxLogicalRight = max(logicalLeft, maxLogicalRight);
        } else {
            if (curr->renderer()->isOutOfFlowPositioned()) {
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
                InlineFlowBox* flow = toInlineFlowBox(curr);
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
                LayoutUnit logicalLeftMargin = isHorizontal() ? curr->boxModelObject()->marginLeft() : curr->boxModelObject()->marginTop();
                LayoutUnit logicalRightMargin = isHorizontal() ? curr->boxModelObject()->marginRight() : curr->boxModelObject()->marginBottom();
                
                logicalLeft += logicalLeftMargin;
                curr->setLogicalLeft(logicalLeft);
                if (knownToHaveNoOverflow())
                    minLogicalLeft = min(logicalLeft, minLogicalLeft);
                logicalLeft += curr->logicalWidth();
                if (knownToHaveNoOverflow())
                    maxLogicalRight = max(logicalLeft, maxLogicalRight);
                logicalLeft += logicalRightMargin;
                // If we encounter any space after this inline block then ensure it is treated as the space between two words.
                needsWordSpacing = true;
            }
        }
    }
    return logicalLeft;
}

bool InlineFlowBox::requiresIdeographicBaseline(const GlyphOverflowAndFallbackFontsMap& textBoxDataMap) const
{
    if (isHorizontal())
        return false;
    
    if (renderer()->style(isFirstLineStyle())->fontDescription().nonCJKGlyphOrientation() == NonCJKGlyphOrientationUpright
        || renderer()->style(isFirstLineStyle())->font().primaryFont()->hasVerticalGlyphs())
        return true;

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->isInlineFlowBox()) {
            if (toInlineFlowBox(curr)->requiresIdeographicBaseline(textBoxDataMap))
                return true;
        } else {
            if (curr->renderer()->style(isFirstLineStyle())->font().primaryFont()->hasVerticalGlyphs())
                return true;
            
            const Vector<const SimpleFontData*>* usedFonts = 0;
            if (curr->isInlineTextBox()) {
                GlyphOverflowAndFallbackFontsMap::const_iterator it = textBoxDataMap.find(toInlineTextBox(curr));
                usedFonts = it == textBoxDataMap.end() ? 0 : &it->value.first;
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

static bool verticalAlignApplies(RenderObject* curr)
{
    // http://www.w3.org/TR/CSS2/visudet.html#propdef-vertical-align - vertical-align
    // only applies to inline level and table-cell elements
    return !curr->isText() || curr->parent()->isInline() || curr->parent()->isTableCell();
}

void InlineFlowBox::adjustMaxAscentAndDescent(int& maxAscent, int& maxDescent, int maxPositionTop, int maxPositionBottom)
{
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        // The computed lineheight needs to be extended for the
        // positioned elements
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if ((curr->verticalAlign() == TOP || curr->verticalAlign() == BOTTOM) && verticalAlignApplies(curr->renderer())) {
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
            toInlineFlowBox(curr)->adjustMaxAscentAndDescent(maxAscent, maxDescent, maxPositionTop, maxPositionBottom);
    }
}

void InlineFlowBox::computeLogicalBoxHeights(RootInlineBox* rootBox, LayoutUnit& maxPositionTop, LayoutUnit& maxPositionBottom,
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
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        InlineFlowBox* inlineFlowBox = curr->isInlineFlowBox() ? toInlineFlowBox(curr) : 0;
        
        bool affectsAscent = false;
        bool affectsDescent = false;
        
        // The verticalPositionForBox function returns the distance between the child box's baseline
        // and the root box's baseline.  The value is negative if the child box's baseline is above the
        // root box's baseline, and it is positive if the child box's baseline is below the root box's baseline.
        curr->setLogicalTop(rootBox->verticalPositionForBox(curr, verticalPositionCache));
        
        int ascent = 0;
        int descent = 0;
        rootBox->ascentAndDescentForBox(curr, textBoxDataMap, ascent, descent, affectsAscent, affectsDescent);

        LayoutUnit boxHeight = ascent + descent;
        if (curr->verticalAlign() == TOP && verticalAlignApplies(curr->renderer())) {
            if (maxPositionTop < boxHeight)
                maxPositionTop = boxHeight;
        } else if (curr->verticalAlign() == BOTTOM && verticalAlignApplies(curr->renderer())) {
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

void InlineFlowBox::placeBoxesInBlockDirection(LayoutUnit top, LayoutUnit maxHeight, int maxAscent, bool strictMode, LayoutUnit& lineTop, LayoutUnit& lineBottom, bool& setLineTop,
                                               LayoutUnit& lineTopIncludingMargins, LayoutUnit& lineBottomIncludingMargins, bool& hasAnnotationsBefore, bool& hasAnnotationsAfter, FontBaseline baselineType)
{
    bool isRootBox = isRootInlineBox();
    if (isRootBox) {
        const FontMetrics& fontMetrics = renderer()->style(isFirstLineStyle())->fontMetrics();
        // RootInlineBoxes are always placed on at pixel boundaries in their logical y direction. Not doing
        // so results in incorrect rendering of text decorations, most notably underlines.
        setLogicalTop(roundToInt(top + maxAscent - fontMetrics.ascent(baselineType)));
    }

    LayoutUnit adjustmentForChildrenWithSameLineHeightAndBaseline = 0;
    if (descendantsHaveSameLineHeightAndBaseline()) {
        adjustmentForChildrenWithSameLineHeightAndBaseline = logicalTop();
        if (parent())
            adjustmentForChildrenWithSameLineHeightAndBaseline += (boxModelObject()->borderAndPaddingBefore());
    }

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (descendantsHaveSameLineHeightAndBaseline()) {
            curr->adjustBlockDirectionPosition(adjustmentForChildrenWithSameLineHeightAndBaseline);
            continue;
        }

        InlineFlowBox* inlineFlowBox = curr->isInlineFlowBox() ? toInlineFlowBox(curr) : 0;
        bool childAffectsTopBottomPos = true;

        if (curr->verticalAlign() == TOP && verticalAlignApplies(curr->renderer()))
            curr->setLogicalTop(top);
        else if (curr->verticalAlign() == BOTTOM && verticalAlignApplies(curr->renderer()))
            curr->setLogicalTop(top + maxHeight - curr->lineHeight());
        else {
            if (!strictMode && inlineFlowBox && !inlineFlowBox->hasTextChildren() && !curr->boxModelObject()->hasInlineDirectionBordersOrPadding()
                && !(inlineFlowBox->descendantsHaveSameLineHeightAndBaseline() && inlineFlowBox->hasTextDescendants()))
                childAffectsTopBottomPos = false;
            LayoutUnit posAdjust = maxAscent - curr->baselinePosition(baselineType);
            curr->setLogicalTop(curr->logicalTop() + top + posAdjust);
        }
        
        LayoutUnit newLogicalTop = curr->logicalTop();
        LayoutUnit newLogicalTopIncludingMargins = newLogicalTop;
        LayoutUnit boxHeight = curr->logicalHeight();
        LayoutUnit boxHeightIncludingMargins = boxHeight;
            
        if (curr->isText() || curr->isInlineFlowBox()) {
            const FontMetrics& fontMetrics = curr->renderer()->style(isFirstLineStyle())->fontMetrics();
            newLogicalTop += curr->baselinePosition(baselineType) - fontMetrics.ascent(baselineType);
            if (curr->isInlineFlowBox()) {
                RenderBoxModelObject* boxObject = toRenderBoxModelObject(curr->renderer());
                newLogicalTop -= boxObject->style(isFirstLineStyle())->isHorizontalWritingMode() ? boxObject->borderTop() + boxObject->paddingTop() : 
                                 boxObject->borderRight() + boxObject->paddingRight();
            }
            newLogicalTopIncludingMargins = newLogicalTop;
        } else if (!curr->renderer()->isBR()) {
            RenderBox* box = toRenderBox(curr->renderer());
            newLogicalTopIncludingMargins = newLogicalTop;
            LayoutUnit overSideMargin = curr->isHorizontal() ? box->marginTop() : box->marginRight();
            LayoutUnit underSideMargin = curr->isHorizontal() ? box->marginBottom() : box->marginLeft();
            newLogicalTop += overSideMargin;
            boxHeightIncludingMargins += overSideMargin + underSideMargin;
        }

        curr->setLogicalTop(newLogicalTop);

        if (childAffectsTopBottomPos) {
            if (curr->renderer()->isRubyRun()) {
                // Treat the leading on the first and last lines of ruby runs as not being part of the overall lineTop/lineBottom.
                // Really this is a workaround hack for the fact that ruby should have been done as line layout and not done using
                // inline-block.
                if (renderer()->style()->isFlippedLinesWritingMode() == (curr->renderer()->style()->rubyPosition() == RubyPositionAfter))
                    hasAnnotationsBefore = true;
                else
                    hasAnnotationsAfter = true;

                RenderRubyRun* rubyRun = toRenderRubyRun(curr->renderer());
                if (RenderRubyBase* rubyBase = rubyRun->rubyBase()) {
                    LayoutUnit bottomRubyBaseLeading = (curr->logicalHeight() - rubyBase->logicalBottom()) + rubyBase->logicalHeight() - (rubyBase->lastRootBox() ? rubyBase->lastRootBox()->lineBottom() : LayoutUnit());
                    LayoutUnit topRubyBaseLeading = rubyBase->logicalTop() + (rubyBase->firstRootBox() ? rubyBase->firstRootBox()->lineTop() : LayoutUnit());
                    newLogicalTop += !renderer()->style()->isFlippedLinesWritingMode() ? topRubyBaseLeading : bottomRubyBaseLeading;
                    boxHeight -= (topRubyBaseLeading + bottomRubyBaseLeading);
                }
            }
            if (curr->isInlineTextBox()) {
                TextEmphasisPosition emphasisMarkPosition;
                if (toInlineTextBox(curr)->getEmphasisMarkPosition(curr->renderer()->style(isFirstLineStyle()), emphasisMarkPosition)) {
                    bool emphasisMarkIsOver = emphasisMarkPosition == TextEmphasisPositionOver;
                    if (emphasisMarkIsOver != curr->renderer()->style(isFirstLineStyle())->isFlippedLinesWritingMode())
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
                lineTop = pixelSnappedLogicalTop();
                lineTopIncludingMargins = lineTop;
            } else {
                lineTop = min<LayoutUnit>(lineTop, pixelSnappedLogicalTop());
                lineTopIncludingMargins = min(lineTop, lineTopIncludingMargins);
            }
            lineBottom = max<LayoutUnit>(lineBottom, pixelSnappedLogicalBottom());
            lineBottomIncludingMargins = max(lineBottom, lineBottomIncludingMargins);
        }
        
        if (renderer()->style()->isFlippedLinesWritingMode())
            flipLinesInBlockDirection(lineTopIncludingMargins, lineBottomIncludingMargins);
    }
}

#if ENABLE(CSS3_TEXT)
void InlineFlowBox::computeMaxLogicalTop(float& maxLogicalTop) const
{
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (descendantsHaveSameLineHeightAndBaseline())
            continue;

        maxLogicalTop = max<float>(maxLogicalTop, curr->y());
        float localMaxLogicalTop = 0;
        if (curr->isInlineFlowBox())
            toInlineFlowBox(curr)->computeMaxLogicalTop(localMaxLogicalTop);
        maxLogicalTop = max<float>(maxLogicalTop, localMaxLogicalTop);
    }
}
#endif // CSS3_TEXT

void InlineFlowBox::flipLinesInBlockDirection(LayoutUnit lineTop, LayoutUnit lineBottom)
{
    // Flip the box on the line such that the top is now relative to the lineBottom instead of the lineTop.
    setLogicalTop(lineBottom - (logicalTop() - lineTop) - logicalHeight());
    
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders aren't affected here.
        
        if (curr->isInlineFlowBox())
            toInlineFlowBox(curr)->flipLinesInBlockDirection(lineTop, lineBottom);
        else
            curr->setLogicalTop(lineBottom - (curr->logicalTop() - lineTop) - curr->logicalHeight());
    }
}

inline void InlineFlowBox::addBoxShadowVisualOverflow(LayoutRect& logicalVisualOverflow)
{
    // box-shadow on root line boxes is applying to the block and not to the lines.
    if (!parent())
        return;

    RenderStyle* style = renderer()->style(isFirstLineStyle());
    if (!style->boxShadow())
        return;

    LayoutUnit boxShadowLogicalTop;
    LayoutUnit boxShadowLogicalBottom;
    style->getBoxShadowBlockDirectionExtent(boxShadowLogicalTop, boxShadowLogicalBottom);
    
    // Similar to how glyph overflow works, if our lines are flipped, then it's actually the opposite shadow that applies, since
    // the line is "upside down" in terms of block coordinates.
    LayoutUnit shadowLogicalTop = style->isFlippedLinesWritingMode() ? -boxShadowLogicalBottom : boxShadowLogicalTop;
    LayoutUnit shadowLogicalBottom = style->isFlippedLinesWritingMode() ? -boxShadowLogicalTop : boxShadowLogicalBottom;
    
    LayoutUnit logicalTopVisualOverflow = min(pixelSnappedLogicalTop() + shadowLogicalTop, logicalVisualOverflow.y());
    LayoutUnit logicalBottomVisualOverflow = max(pixelSnappedLogicalBottom() + shadowLogicalBottom, logicalVisualOverflow.maxY());
    
    LayoutUnit boxShadowLogicalLeft;
    LayoutUnit boxShadowLogicalRight;
    style->getBoxShadowInlineDirectionExtent(boxShadowLogicalLeft, boxShadowLogicalRight);

    LayoutUnit logicalLeftVisualOverflow = min(pixelSnappedLogicalLeft() + boxShadowLogicalLeft, logicalVisualOverflow.x());
    LayoutUnit logicalRightVisualOverflow = max(pixelSnappedLogicalRight() + boxShadowLogicalRight, logicalVisualOverflow.maxX());
    
    logicalVisualOverflow = LayoutRect(logicalLeftVisualOverflow, logicalTopVisualOverflow,
                                       logicalRightVisualOverflow - logicalLeftVisualOverflow, logicalBottomVisualOverflow - logicalTopVisualOverflow);
}

inline void InlineFlowBox::addBorderOutsetVisualOverflow(LayoutRect& logicalVisualOverflow)
{
    // border-image-outset on root line boxes is applying to the block and not to the lines.
    if (!parent())
        return;
    
    RenderStyle* style = renderer()->style(isFirstLineStyle());
    if (!style->hasBorderImageOutsets())
        return;

    LayoutBoxExtent borderOutsets = style->borderImageOutsets();

    LayoutUnit borderOutsetLogicalTop = borderOutsets.logicalTop(style->writingMode());
    LayoutUnit borderOutsetLogicalBottom = borderOutsets.logicalBottom(style->writingMode());
    LayoutUnit borderOutsetLogicalLeft = borderOutsets.logicalLeft(style->writingMode());
    LayoutUnit borderOutsetLogicalRight = borderOutsets.logicalRight(style->writingMode());

    // Similar to how glyph overflow works, if our lines are flipped, then it's actually the opposite border that applies, since
    // the line is "upside down" in terms of block coordinates. vertical-rl and horizontal-bt are the flipped line modes.
    LayoutUnit outsetLogicalTop = style->isFlippedLinesWritingMode() ? borderOutsetLogicalBottom : borderOutsetLogicalTop;
    LayoutUnit outsetLogicalBottom = style->isFlippedLinesWritingMode() ? borderOutsetLogicalTop : borderOutsetLogicalBottom;

    LayoutUnit logicalTopVisualOverflow = min(pixelSnappedLogicalTop() - outsetLogicalTop, logicalVisualOverflow.y());
    LayoutUnit logicalBottomVisualOverflow = max(pixelSnappedLogicalBottom() + outsetLogicalBottom, logicalVisualOverflow.maxY());

    LayoutUnit outsetLogicalLeft = includeLogicalLeftEdge() ? borderOutsetLogicalLeft : LayoutUnit();
    LayoutUnit outsetLogicalRight = includeLogicalRightEdge() ? borderOutsetLogicalRight : LayoutUnit();

    LayoutUnit logicalLeftVisualOverflow = min(pixelSnappedLogicalLeft() - outsetLogicalLeft, logicalVisualOverflow.x());
    LayoutUnit logicalRightVisualOverflow = max(pixelSnappedLogicalRight() + outsetLogicalRight, logicalVisualOverflow.maxX());
    
    logicalVisualOverflow = LayoutRect(logicalLeftVisualOverflow, logicalTopVisualOverflow,
                                       logicalRightVisualOverflow - logicalLeftVisualOverflow, logicalBottomVisualOverflow - logicalTopVisualOverflow);
}

inline void InlineFlowBox::addTextBoxVisualOverflow(InlineTextBox* textBox, GlyphOverflowAndFallbackFontsMap& textBoxDataMap, LayoutRect& logicalVisualOverflow)
{
    if (textBox->knownToHaveNoOverflow())
        return;

    RenderStyle* style = textBox->renderer()->style(isFirstLineStyle());
    
    GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.find(textBox);
    GlyphOverflow* glyphOverflow = it == textBoxDataMap.end() ? 0 : &it->value.second;
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

    LayoutUnit textShadowLogicalTop;
    LayoutUnit textShadowLogicalBottom;
    style->getTextShadowBlockDirectionExtent(textShadowLogicalTop, textShadowLogicalBottom);
    
    LayoutUnit childOverflowLogicalTop = min<LayoutUnit>(textShadowLogicalTop + topGlyphOverflow, topGlyphOverflow);
    LayoutUnit childOverflowLogicalBottom = max<LayoutUnit>(textShadowLogicalBottom + bottomGlyphOverflow, bottomGlyphOverflow);
   
    LayoutUnit textShadowLogicalLeft;
    LayoutUnit textShadowLogicalRight;
    style->getTextShadowInlineDirectionExtent(textShadowLogicalLeft, textShadowLogicalRight);
   
    LayoutUnit childOverflowLogicalLeft = min<LayoutUnit>(textShadowLogicalLeft + leftGlyphOverflow, leftGlyphOverflow);
    LayoutUnit childOverflowLogicalRight = max<LayoutUnit>(textShadowLogicalRight + rightGlyphOverflow, rightGlyphOverflow);

    LayoutUnit logicalTopVisualOverflow = min(textBox->pixelSnappedLogicalTop() + childOverflowLogicalTop, logicalVisualOverflow.y());
    LayoutUnit logicalBottomVisualOverflow = max(textBox->pixelSnappedLogicalBottom() + childOverflowLogicalBottom, logicalVisualOverflow.maxY());
    LayoutUnit logicalLeftVisualOverflow = min(textBox->pixelSnappedLogicalLeft() + childOverflowLogicalLeft, logicalVisualOverflow.x());
    LayoutUnit logicalRightVisualOverflow = max(textBox->pixelSnappedLogicalRight() + childOverflowLogicalRight, logicalVisualOverflow.maxX());
    
    logicalVisualOverflow = LayoutRect(logicalLeftVisualOverflow, logicalTopVisualOverflow,
                                       logicalRightVisualOverflow - logicalLeftVisualOverflow, logicalBottomVisualOverflow - logicalTopVisualOverflow);
                                    
    textBox->setLogicalOverflowRect(logicalVisualOverflow);
}

inline void InlineFlowBox::addReplacedChildOverflow(const InlineBox* inlineBox, LayoutRect& logicalLayoutOverflow, LayoutRect& logicalVisualOverflow)
{
    RenderBox* box = toRenderBox(inlineBox->renderer());
    
    // Visual overflow only propagates if the box doesn't have a self-painting layer.  This rectangle does not include
    // transforms or relative positioning (since those objects always have self-painting layers), but it does need to be adjusted
    // for writing-mode differences.
    if (!box->hasSelfPaintingLayer()) {
        LayoutRect childLogicalVisualOverflow = box->logicalVisualOverflowRectForPropagation(renderer()->style());
        childLogicalVisualOverflow.move(inlineBox->logicalLeft(), inlineBox->logicalTop());
        logicalVisualOverflow.unite(childLogicalVisualOverflow);
    }

    // Layout overflow internal to the child box only propagates if the child box doesn't have overflow clip set.
    // Otherwise the child border box propagates as layout overflow.  This rectangle must include transforms and relative positioning
    // and be adjusted for writing-mode differences.
    LayoutRect childLogicalLayoutOverflow = box->logicalLayoutOverflowRectForPropagation(renderer()->style());
    childLogicalLayoutOverflow.move(inlineBox->logicalLeft(), inlineBox->logicalTop());
    logicalLayoutOverflow.unite(childLogicalLayoutOverflow);
}

void InlineFlowBox::computeOverflow(LayoutUnit lineTop, LayoutUnit lineBottom, GlyphOverflowAndFallbackFontsMap& textBoxDataMap)
{
    // If we know we have no overflow, we can just bail.
    if (knownToHaveNoOverflow())
        return;

    // Visual overflow just includes overflow for stuff we need to repaint ourselves.  Self-painting layers are ignored.
    // Layout overflow is used to determine scrolling extent, so it still includes child layers and also factors in
    // transforms, relative positioning, etc.
    LayoutRect logicalLayoutOverflow(enclosingLayoutRect(logicalFrameRectIncludingLineHeight(lineTop, lineBottom)));
    LayoutRect logicalVisualOverflow(logicalLayoutOverflow);
  
    addBoxShadowVisualOverflow(logicalVisualOverflow);
    addBorderOutsetVisualOverflow(logicalVisualOverflow);

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->renderer()->isText()) {
            InlineTextBox* text = toInlineTextBox(curr);
            RenderText* rt = toRenderText(text->renderer());
            if (rt->isBR())
                continue;
            LayoutRect textBoxOverflow(enclosingLayoutRect(text->logicalFrameRect()));
            addTextBoxVisualOverflow(text, textBoxDataMap, textBoxOverflow);
            logicalVisualOverflow.unite(textBoxOverflow);
        } else  if (curr->renderer()->isRenderInline()) {
            InlineFlowBox* flow = toInlineFlowBox(curr);
            flow->computeOverflow(lineTop, lineBottom, textBoxDataMap);
            if (!flow->boxModelObject()->hasSelfPaintingLayer())
                logicalVisualOverflow.unite(flow->logicalVisualOverflowRect(lineTop, lineBottom));
            LayoutRect childLayoutOverflow = flow->logicalLayoutOverflowRect(lineTop, lineBottom);
            childLayoutOverflow.move(flow->boxModelObject()->relativePositionLogicalOffset());
            logicalLayoutOverflow.unite(childLayoutOverflow);
        } else
            addReplacedChildOverflow(curr, logicalLayoutOverflow, logicalVisualOverflow);
    }
    
    setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, lineTop, lineBottom);
}

void InlineFlowBox::setLayoutOverflow(const LayoutRect& rect, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutRect frameBox = enclosingLayoutRect(frameRectIncludingLineHeight(lineTop, lineBottom));
    if (frameBox.contains(rect) || rect.isEmpty())
        return;

    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(frameBox, frameBox));
    
    m_overflow->setLayoutOverflow(rect);
}

void InlineFlowBox::setVisualOverflow(const LayoutRect& rect, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutRect frameBox = enclosingLayoutRect(frameRectIncludingLineHeight(lineTop, lineBottom));
    if (frameBox.contains(rect) || rect.isEmpty())
        return;
        
    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(frameBox, frameBox));
    
    m_overflow->setVisualOverflow(rect);
}

void InlineFlowBox::setOverflowFromLogicalRects(const LayoutRect& logicalLayoutOverflow, const LayoutRect& logicalVisualOverflow, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutRect layoutOverflow(isHorizontal() ? logicalLayoutOverflow : logicalLayoutOverflow.transposedRect());
    setLayoutOverflow(layoutOverflow, lineTop, lineBottom);
    
    LayoutRect visualOverflow(isHorizontal() ? logicalVisualOverflow : logicalVisualOverflow.transposedRect());
    setVisualOverflow(visualOverflow, lineTop, lineBottom);
}

bool InlineFlowBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutRect overflowRect(visualOverflowRect(lineTop, lineBottom));
    flipForWritingMode(overflowRect);
    overflowRect.moveBy(accumulatedOffset);
    if (!locationInContainer.intersects(overflowRect))
        return false;

    // Check children first.
    // We need to account for culled inline parents of the hit-tested nodes, so that they may also get included in area-based hit-tests.
    RenderObject* culledParent = 0;
    for (InlineBox* curr = lastChild(); curr; curr = curr->prevOnLine()) {
        if (curr->renderer()->isText() || !curr->boxModelObject()->hasSelfPaintingLayer()) {
            RenderObject* newParent = 0;
            // Culled parents are only relevant for area-based hit-tests, so ignore it in point-based ones.
            if (locationInContainer.isRectBasedTest()) {
                newParent = curr->renderer()->parent();
                if (newParent == renderer())
                    newParent = 0;
            }
            // Check the culled parent after all its children have been checked, to do this we wait until
            // we are about to test an element with a different parent.
            if (newParent != culledParent) {
                if (!newParent || !newParent->isDescendantOf(culledParent)) {
                    while (culledParent && culledParent != renderer() && culledParent != newParent) {
                        if (culledParent->isRenderInline() && toRenderInline(culledParent)->hitTestCulledInline(request, result, locationInContainer, accumulatedOffset))
                            return true;
                        culledParent = culledParent->parent();
                    }
                }
                culledParent = newParent;
            }
            if (curr->nodeAtPoint(request, result, locationInContainer, accumulatedOffset, lineTop, lineBottom)) {
                renderer()->updateHitTestResult(result, locationInContainer.point() - toLayoutSize(accumulatedOffset));
                return true;
            }
        }
    }
    // Check any culled ancestor of the final children tested.
    while (culledParent && culledParent != renderer()) {
        if (culledParent->isRenderInline() && toRenderInline(culledParent)->hitTestCulledInline(request, result, locationInContainer, accumulatedOffset))
            return true;
        culledParent = culledParent->parent();
    }

    // Now check ourselves. Pixel snap hit testing.
    if (!visibleToHitTesting())
        return false;

    // Do not hittest content beyond the ellipsis box.
    if (isRootInlineBox() && hasEllipsisBox()) {
        const EllipsisBox* ellipsisBox = root()->ellipsisBox();
        LayoutRect boundsRect(roundedFrameRect());

        if (isHorizontal())
            renderer()->style()->isLeftToRightDirection() ? boundsRect.shiftXEdgeTo(ellipsisBox->right()) : boundsRect.setWidth(ellipsisBox->left() - left());
        else
            boundsRect.shiftYEdgeTo(ellipsisBox->right());

        flipForWritingMode(boundsRect);
        boundsRect.moveBy(accumulatedOffset);
        // We are beyond the ellipsis box.
        if (locationInContainer.intersects(boundsRect))
            return false;
    }

    LayoutRect frameRect = roundedFrameRect();
    LayoutUnit minX = frameRect.x();
    LayoutUnit minY = frameRect.y();
    LayoutUnit width = frameRect.width();
    LayoutUnit height = frameRect.height();

    // Constrain our hit testing to the line top and bottom if necessary.
    bool noQuirksMode = renderer()->document()->inNoQuirksMode();
    if (!noQuirksMode && !hasTextChildren() && !(descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
        RootInlineBox* rootBox = root();
        LayoutUnit& top = isHorizontal() ? minY : minX;
        LayoutUnit& logicalHeight = isHorizontal() ? height : width;
        LayoutUnit bottom = min(rootBox->lineBottom(), top + logicalHeight);
        top = max(rootBox->lineTop(), top);
        logicalHeight = bottom - top;
    }

    // Move x/y to our coordinates.
    LayoutRect rect(minX, minY, width, height);
    flipForWritingMode(rect);
    rect.moveBy(accumulatedOffset);

    if (locationInContainer.intersects(rect)) {
        renderer()->updateHitTestResult(result, flipForWritingMode(locationInContainer.point() - toLayoutSize(accumulatedOffset))); // Don't add in m_x or m_y here, we want coords in the containing block's space.
        if (!result.addNodeToRectBasedTestResult(renderer()->node(), request, locationInContainer, rect))
            return true;
    }

    return false;
}

void InlineFlowBox::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    LayoutRect overflowRect(visualOverflowRect(lineTop, lineBottom));
    overflowRect.inflate(renderer()->maximalOutlineSize(paintInfo.phase));
    flipForWritingMode(overflowRect);
    overflowRect.moveBy(paintOffset);
    
    if (!paintInfo.rect.intersects(pixelSnappedIntRect(overflowRect)))
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
            paintMask(paintInfo, paintOffset);
            return;
        } else {
            // Paint our background, border and box-shadow.
            paintBoxDecorations(paintInfo, paintOffset);
        }
    }

    if (paintInfo.phase == PaintPhaseMask)
        return;

    PaintPhase paintPhase = paintInfo.phase == PaintPhaseChildOutlines ? PaintPhaseOutline : paintInfo.phase;
    PaintInfo childInfo(paintInfo);
    childInfo.phase = paintPhase;
    childInfo.updateSubtreePaintRootForChildren(renderer());
    
    // Paint our children.
    if (paintPhase != PaintPhaseSelfOutline) {
        for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
            if (curr->renderer()->isText() || !curr->boxModelObject()->hasSelfPaintingLayer())
                curr->paint(childInfo, paintOffset, lineTop, lineBottom);
        }
    }
}

void InlineFlowBox::paintFillLayers(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, const LayoutRect& rect, CompositeOperator op)
{
    if (!fillLayer)
        return;
    paintFillLayers(paintInfo, c, fillLayer->next(), rect, op);
    paintFillLayer(paintInfo, c, fillLayer, rect, op);
}

bool InlineFlowBox::boxShadowCanBeAppliedToBackground(const FillLayer& lastBackgroundLayer) const
{
    // The checks here match how paintFillLayer() decides whether to clip (if it does, the shadow
    // would be clipped out, so it has to be drawn separately).
    StyleImage* image = lastBackgroundLayer.image();
    bool hasFillImage = image && image->canRender(renderer(), renderer()->style()->effectiveZoom());
    return (!hasFillImage && !renderer()->style()->hasBorderRadius()) || (!prevLineBox() && !nextLineBox()) || !parent();
}

void InlineFlowBox::paintFillLayer(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, const LayoutRect& rect, CompositeOperator op)
{
    StyleImage* img = fillLayer->image();
    bool hasFillImage = img && img->canRender(renderer(), renderer()->style()->effectiveZoom());
    if ((!hasFillImage && !renderer()->style()->hasBorderRadius()) || (!prevLineBox() && !nextLineBox()) || !parent())
        boxModelObject()->paintFillLayerExtended(paintInfo, c, fillLayer, rect, BackgroundBleedNone, this, rect.size(), op);
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    else if (renderer()->style()->boxDecorationBreak() == DCLONE) {
        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        paintInfo.context->clip(LayoutRect(rect.x(), rect.y(), width(), height()));
        boxModelObject()->paintFillLayerExtended(paintInfo, c, fillLayer, rect, BackgroundBleedNone, this, rect.size(), op);
    }
#endif
    else {
        // We have a fill image that spans multiple lines.
        // We need to adjust tx and ty by the width of all previous lines.
        // Think of background painting on inlines as though you had one long line, a single continuous
        // strip.  Even though that strip has been broken up across multiple lines, you still paint it
        // as though you had one single line.  This means each line has to pick up the background where
        // the previous line left off.
        LayoutUnit logicalOffsetOnLine = 0;
        LayoutUnit totalLogicalWidth;
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
        LayoutUnit stripX = rect.x() - (isHorizontal() ? logicalOffsetOnLine : LayoutUnit());
        LayoutUnit stripY = rect.y() - (isHorizontal() ? LayoutUnit() : logicalOffsetOnLine);
        LayoutUnit stripWidth = isHorizontal() ? totalLogicalWidth : static_cast<LayoutUnit>(width());
        LayoutUnit stripHeight = isHorizontal() ? static_cast<LayoutUnit>(height()) : totalLogicalWidth;

        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        paintInfo.context->clip(LayoutRect(rect.x(), rect.y(), width(), height()));
        boxModelObject()->paintFillLayerExtended(paintInfo, c, fillLayer, LayoutRect(stripX, stripY, stripWidth, stripHeight), BackgroundBleedNone, this, rect.size(), op);
    }
}

void InlineFlowBox::paintBoxShadow(const PaintInfo& info, RenderStyle* s, ShadowStyle shadowStyle, const LayoutRect& paintRect)
{
    if ((!prevLineBox() && !nextLineBox()) || !parent())
        boxModelObject()->paintBoxShadow(info, paintRect, s, shadowStyle);
    else {
        // FIXME: We can do better here in the multi-line case. We want to push a clip so that the shadow doesn't
        // protrude incorrectly at the edges, and we want to possibly include shadows cast from the previous/following lines
        boxModelObject()->paintBoxShadow(info, paintRect, s, shadowStyle, includeLogicalLeftEdge(), includeLogicalRightEdge());
    }
}

void InlineFlowBox::constrainToLineTopAndBottomIfNeeded(LayoutRect& rect) const
{
    bool noQuirksMode = renderer()->document()->inNoQuirksMode();
    if (!noQuirksMode && !hasTextChildren() && !(descendantsHaveSameLineHeightAndBaseline() && hasTextDescendants())) {
        const RootInlineBox* rootBox = root();
        LayoutUnit logicalTop = isHorizontal() ? rect.y() : rect.x();
        LayoutUnit logicalHeight = isHorizontal() ? rect.height() : rect.width();
        LayoutUnit bottom = min(rootBox->lineBottom(), logicalTop + logicalHeight);
        logicalTop = max(rootBox->lineTop(), logicalTop);
        logicalHeight = bottom - logicalTop;
        if (isHorizontal()) {
            rect.setY(logicalTop);
            rect.setHeight(logicalHeight);
        } else {
            rect.setX(logicalTop);
            rect.setWidth(logicalHeight);
        }
    }
}

static LayoutRect clipRectForNinePieceImageStrip(InlineFlowBox* box, const NinePieceImage& image, const LayoutRect& paintRect)
{
    LayoutRect clipRect(paintRect);
    RenderStyle* style = box->renderer()->style();
    LayoutBoxExtent outsets = style->imageOutsets(image);
    if (box->isHorizontal()) {
        clipRect.setY(paintRect.y() - outsets.top());
        clipRect.setHeight(paintRect.height() + outsets.top() + outsets.bottom());
        if (box->includeLogicalLeftEdge()) {
            clipRect.setX(paintRect.x() - outsets.left());
            clipRect.setWidth(paintRect.width() + outsets.left());
        }
        if (box->includeLogicalRightEdge())
            clipRect.setWidth(clipRect.width() + outsets.right());
    } else {
        clipRect.setX(paintRect.x() - outsets.left());
        clipRect.setWidth(paintRect.width() + outsets.left() + outsets.right());
        if (box->includeLogicalLeftEdge()) {
            clipRect.setY(paintRect.y() - outsets.top());
            clipRect.setHeight(paintRect.height() + outsets.top());
        }
        if (box->includeLogicalRightEdge())
            clipRect.setHeight(clipRect.height() + outsets.bottom());
    }
    return clipRect;
}

void InlineFlowBox::paintBoxDecorations(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(renderer()) || renderer()->style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseForeground)
        return;

    // Pixel snap background/border painting.
    LayoutRect frameRect = roundedFrameRect();

    constrainToLineTopAndBottomIfNeeded(frameRect);
    
    // Move x/y to our coordinates.
    LayoutRect localRect(frameRect);
    flipForWritingMode(localRect);
    LayoutPoint adjustedPaintoffset = paintOffset + localRect.location();
    
    GraphicsContext* context = paintInfo.context;
    
    // You can use p::first-line to specify a background. If so, the root line boxes for
    // a line may actually have to paint a background.
    RenderStyle* styleToUse = renderer()->style(isFirstLineStyle());
    if ((!parent() && isFirstLineStyle() && styleToUse != renderer()->style()) || (parent() && renderer()->hasBoxDecorations())) {
        LayoutRect paintRect = LayoutRect(adjustedPaintoffset, frameRect.size());
        // Shadow comes first and is behind the background and border.
        if (!boxModelObject()->boxShadowShouldBeAppliedToBackground(BackgroundBleedNone, this))
            paintBoxShadow(paintInfo, styleToUse, Normal, paintRect);

        Color c = styleToUse->visitedDependentColor(CSSPropertyBackgroundColor);
        paintFillLayers(paintInfo, c, styleToUse->backgroundLayers(), paintRect);
        paintBoxShadow(paintInfo, styleToUse, Inset, paintRect);

        // :first-line cannot be used to put borders on a line. Always paint borders with our
        // non-first-line style.
        if (parent() && renderer()->style()->hasBorder()) {
            const NinePieceImage& borderImage = renderer()->style()->borderImage();
            StyleImage* borderImageSource = borderImage.image();
            bool hasBorderImage = borderImageSource && borderImageSource->canRender(renderer(), styleToUse->effectiveZoom());
            if (hasBorderImage && !borderImageSource->isLoaded())
                return; // Don't paint anything while we wait for the image to load.

            // The simple case is where we either have no border image or we are the only box for this object.  In those
            // cases only a single call to draw is required.
            if (!hasBorderImage || (!prevLineBox() && !nextLineBox()))
                boxModelObject()->paintBorder(paintInfo, paintRect, renderer()->style(isFirstLineStyle()), BackgroundBleedNone, includeLogicalLeftEdge(), includeLogicalRightEdge());
            else {
                // We have a border image that spans multiple lines.
                // We need to adjust tx and ty by the width of all previous lines.
                // Think of border image painting on inlines as though you had one long line, a single continuous
                // strip.  Even though that strip has been broken up across multiple lines, you still paint it
                // as though you had one single line.  This means each line has to pick up the image where
                // the previous line left off.
                // FIXME: What the heck do we do with RTL here? The math we're using is obviously not right,
                // but it isn't even clear how this should work at all.
                LayoutUnit logicalOffsetOnLine = 0;
                for (InlineFlowBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
                    logicalOffsetOnLine += curr->logicalWidth();
                LayoutUnit totalLogicalWidth = logicalOffsetOnLine;
                for (InlineFlowBox* curr = this; curr; curr = curr->nextLineBox())
                    totalLogicalWidth += curr->logicalWidth();
                LayoutUnit stripX = adjustedPaintoffset.x() - (isHorizontal() ? logicalOffsetOnLine : LayoutUnit());
                LayoutUnit stripY = adjustedPaintoffset.y() - (isHorizontal() ? LayoutUnit() : logicalOffsetOnLine);
                LayoutUnit stripWidth = isHorizontal() ? totalLogicalWidth : frameRect.width();
                LayoutUnit stripHeight = isHorizontal() ? frameRect.height() : totalLogicalWidth;

                LayoutRect clipRect = clipRectForNinePieceImageStrip(this, borderImage, paintRect);
                GraphicsContextStateSaver stateSaver(*context);
                context->clip(clipRect);
                boxModelObject()->paintBorder(paintInfo, LayoutRect(stripX, stripY, stripWidth, stripHeight), renderer()->style(isFirstLineStyle()));
            }
        }
    }
}

void InlineFlowBox::paintMask(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(renderer()) || renderer()->style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    // Pixel snap mask painting.
    LayoutRect frameRect = roundedFrameRect();

    constrainToLineTopAndBottomIfNeeded(frameRect);
    
    // Move x/y to our coordinates.
    LayoutRect localRect(frameRect);
    flipForWritingMode(localRect);
    LayoutPoint adjustedPaintOffset = paintOffset + localRect.location();

    const NinePieceImage& maskNinePieceImage = renderer()->style()->maskBoxImage();
    StyleImage* maskBoxImage = renderer()->style()->maskBoxImage().image();

    // Figure out if we need to push a transparency layer to render our mask.
    bool pushTransparencyLayer = false;
    bool compositedMask = renderer()->hasLayer() && boxModelObject()->layer()->hasCompositedMask();
    bool flattenCompositingLayers = renderer()->view()->frameView() && renderer()->view()->frameView()->paintBehavior() & PaintBehaviorFlattenCompositingLayers;
    CompositeOperator compositeOp = CompositeSourceOver;
    if (!compositedMask || flattenCompositingLayers) {
        if ((maskBoxImage && renderer()->style()->maskLayers()->hasImage()) || renderer()->style()->maskLayers()->next())
            pushTransparencyLayer = true;
        
        compositeOp = CompositeDestinationIn;
        if (pushTransparencyLayer) {
            paintInfo.context->setCompositeOperation(CompositeDestinationIn);
            paintInfo.context->beginTransparencyLayer(1.0f);
            compositeOp = CompositeSourceOver;
        }
    }

    LayoutRect paintRect = LayoutRect(adjustedPaintOffset, frameRect.size());
    paintFillLayers(paintInfo, Color(), renderer()->style()->maskLayers(), paintRect, compositeOp);
    
    bool hasBoxImage = maskBoxImage && maskBoxImage->canRender(renderer(), renderer()->style()->effectiveZoom());
    if (!hasBoxImage || !maskBoxImage->isLoaded()) {
        if (pushTransparencyLayer)
            paintInfo.context->endTransparencyLayer();
        return; // Don't paint anything while we wait for the image to load.
    }

    // The simple case is where we are the only box for this object.  In those
    // cases only a single call to draw is required.
    if (!prevLineBox() && !nextLineBox()) {
        boxModelObject()->paintNinePieceImage(paintInfo.context, LayoutRect(adjustedPaintOffset, frameRect.size()), renderer()->style(), maskNinePieceImage, compositeOp);
    } else {
        // We have a mask image that spans multiple lines.
        // We need to adjust _tx and _ty by the width of all previous lines.
        LayoutUnit logicalOffsetOnLine = 0;
        for (InlineFlowBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
            logicalOffsetOnLine += curr->logicalWidth();
        LayoutUnit totalLogicalWidth = logicalOffsetOnLine;
        for (InlineFlowBox* curr = this; curr; curr = curr->nextLineBox())
            totalLogicalWidth += curr->logicalWidth();
        LayoutUnit stripX = adjustedPaintOffset.x() - (isHorizontal() ? logicalOffsetOnLine : LayoutUnit());
        LayoutUnit stripY = adjustedPaintOffset.y() - (isHorizontal() ? LayoutUnit() : logicalOffsetOnLine);
        LayoutUnit stripWidth = isHorizontal() ? totalLogicalWidth : frameRect.width();
        LayoutUnit stripHeight = isHorizontal() ? frameRect.height() : totalLogicalWidth;

        LayoutRect clipRect = clipRectForNinePieceImageStrip(this, maskNinePieceImage, paintRect);
        GraphicsContextStateSaver stateSaver(*paintInfo.context);
        paintInfo.context->clip(clipRect);
        boxModelObject()->paintNinePieceImage(paintInfo.context, LayoutRect(stripX, stripY, stripWidth, stripHeight), renderer()->style(), maskNinePieceImage, compositeOp);
    }
    
    if (pushTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

InlineBox* InlineFlowBox::firstLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* child = firstChild(); child && !leaf; child = child->nextOnLine())
        leaf = child->isLeaf() ? child : toInlineFlowBox(child)->firstLeafChild();
    return leaf;
}

InlineBox* InlineFlowBox::lastLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* child = lastChild(); child && !leaf; child = child->prevOnLine())
        leaf = child->isLeaf() ? child : toInlineFlowBox(child)->lastLeafChild();
    return leaf;
}

RenderObject::SelectionState InlineFlowBox::selectionState()
{
    return RenderObject::SelectionNone;
}

bool InlineFlowBox::canAccommodateEllipsis(bool ltr, int blockEdge, int ellipsisWidth) const
{
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine()) {
        if (!box->canAccommodateEllipsis(ltr, blockEdge, ellipsisWidth))
            return false;
    }
    return true;
}

float InlineFlowBox::placeEllipsisBox(bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, float &truncatedWidth, bool& foundBox)
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
        int currResult = box->placeEllipsisBox(ltr, visibleLeftEdge, visibleRightEdge, ellipsisWidth, truncatedWidth, foundBox);
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

LayoutUnit InlineFlowBox::computeOverAnnotationAdjustment(LayoutUnit allowedPosition) const
{
    LayoutUnit result = 0;
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.
        
        if (curr->isInlineFlowBox())
            result = max(result, toInlineFlowBox(curr)->computeOverAnnotationAdjustment(allowedPosition));
        
        if (curr->renderer()->isReplaced() && curr->renderer()->isRubyRun() && curr->renderer()->style()->rubyPosition() == RubyPositionBefore) {
            RenderRubyRun* rubyRun = toRenderRubyRun(curr->renderer());
            RenderRubyText* rubyText = rubyRun->rubyText();
            if (!rubyText)
                continue;
            
            if (!rubyRun->style()->isFlippedLinesWritingMode()) {
                LayoutUnit topOfFirstRubyTextLine = rubyText->logicalTop() + (rubyText->firstRootBox() ? rubyText->firstRootBox()->lineTop() : LayoutUnit());
                if (topOfFirstRubyTextLine >= 0)
                    continue;
                topOfFirstRubyTextLine += curr->logicalTop();
                result = max(result, allowedPosition - topOfFirstRubyTextLine);
            } else {
                LayoutUnit bottomOfLastRubyTextLine = rubyText->logicalTop() + (rubyText->lastRootBox() ? rubyText->lastRootBox()->lineBottom() : rubyText->logicalHeight());
                if (bottomOfLastRubyTextLine <= curr->logicalHeight())
                    continue;
                bottomOfLastRubyTextLine += curr->logicalTop();
                result = max(result, bottomOfLastRubyTextLine - allowedPosition);
            }
        }

        if (curr->isInlineTextBox()) {
            RenderStyle* style = curr->renderer()->style(isFirstLineStyle());
            TextEmphasisPosition emphasisMarkPosition;
            if (style->textEmphasisMark() != TextEmphasisMarkNone && toInlineTextBox(curr)->getEmphasisMarkPosition(style, emphasisMarkPosition) && emphasisMarkPosition == TextEmphasisPositionOver) {
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

LayoutUnit InlineFlowBox::computeUnderAnnotationAdjustment(LayoutUnit allowedPosition) const
{
    LayoutUnit result = 0;
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->renderer()->isOutOfFlowPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (curr->isInlineFlowBox())
            result = max(result, toInlineFlowBox(curr)->computeUnderAnnotationAdjustment(allowedPosition));

        if (curr->renderer()->isReplaced() && curr->renderer()->isRubyRun() && curr->renderer()->style()->rubyPosition() == RubyPositionAfter) {
            RenderRubyRun* rubyRun = toRenderRubyRun(curr->renderer());
            RenderRubyText* rubyText = rubyRun->rubyText();
            if (!rubyText)
                continue;

            if (rubyRun->style()->isFlippedLinesWritingMode()) {
                LayoutUnit topOfFirstRubyTextLine = rubyText->logicalTop() + (rubyText->firstRootBox() ? rubyText->firstRootBox()->lineTop() : LayoutUnit());
                if (topOfFirstRubyTextLine >= 0)
                    continue;
                topOfFirstRubyTextLine += curr->logicalTop();
                result = max(result, allowedPosition - topOfFirstRubyTextLine);
            } else {
                LayoutUnit bottomOfLastRubyTextLine = rubyText->logicalTop() + (rubyText->lastRootBox() ? rubyText->lastRootBox()->lineBottom() : rubyText->logicalHeight());
                if (bottomOfLastRubyTextLine <= curr->logicalHeight())
                    continue;
                bottomOfLastRubyTextLine += curr->logicalTop();
                result = max(result, bottomOfLastRubyTextLine - allowedPosition);
            }
        }

        if (curr->isInlineTextBox()) {
            RenderStyle* style = curr->renderer()->style(isFirstLineStyle());
            if (style->textEmphasisMark() != TextEmphasisMarkNone && style->textEmphasisPosition() == TextEmphasisPositionUnder) {
                if (!style->isFlippedLinesWritingMode()) {
                    LayoutUnit bottomOfEmphasisMark = curr->logicalBottom() + style->font().emphasisMarkHeight(style->textEmphasisMarkString());
                    result = max(result, bottomOfEmphasisMark - allowedPosition);
                } else {
                    LayoutUnit topOfEmphasisMark = curr->logicalTop() - style->font().emphasisMarkHeight(style->textEmphasisMarkString());
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

    if (renderer()->style()->rtlOrdering() == VisualOrder)
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
