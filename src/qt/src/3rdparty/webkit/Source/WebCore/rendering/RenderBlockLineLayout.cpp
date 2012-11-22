/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"

#include "BidiResolver.h"
#include "Hyphenation.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "Logging.h"
#include "RenderArena.h"
#include "RenderCombineText.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListMarker.h"
#include "RenderRubyRun.h"
#include "RenderView.h"
#include "Settings.h"
#include "TextBreakIterator.h"
#include "TextRun.h"
#include "TrailingFloatsRootInlineBox.h"
#include "VerticalPositionCache.h"
#include "break_lines.h"
#include <wtf/AlwaysInline.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"
#include "SVGRootInlineBox.h"
#endif

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

// We don't let our line box tree for a single line get any deeper than this.
const unsigned cMaxLineDepth = 200;

class LineInfo {
public:
    LineInfo()
        : m_isFirstLine(true)
        , m_isLastLine(false)
        , m_isEmpty(true)
        , m_previousLineBrokeCleanly(true)
    { }

    bool isFirstLine() const { return m_isFirstLine; }
    bool isLastLine() const { return m_isLastLine; }
    bool isEmpty() const { return m_isEmpty; }
    bool previousLineBrokeCleanly() const { return m_previousLineBrokeCleanly; }

    void setFirstLine(bool firstLine) { m_isFirstLine = firstLine; }
    void setLastLine(bool lastLine) { m_isLastLine = lastLine; }
    void setEmpty(bool empty) { m_isEmpty = empty; }
    void setPreviousLineBrokeCleanly(bool previousLineBrokeCleanly) { m_previousLineBrokeCleanly = previousLineBrokeCleanly; }

private:
    bool m_isFirstLine;
    bool m_isLastLine;
    bool m_isEmpty;
    bool m_previousLineBrokeCleanly;
};

static inline int borderPaddingMarginStart(RenderInline* child)
{
    return child->marginStart() + child->paddingStart() + child->borderStart();
}

static inline int borderPaddingMarginEnd(RenderInline* child)
{
    return child->marginEnd() + child->paddingEnd() + child->borderEnd();
}

static int inlineLogicalWidth(RenderObject* child, bool start = true, bool end = true)
{
    unsigned lineDepth = 1;
    int extraWidth = 0;
    RenderObject* parent = child->parent();
    while (parent->isRenderInline() && lineDepth++ < cMaxLineDepth) {
        RenderInline* parentAsRenderInline = toRenderInline(parent);
        if (start && !child->previousSibling())
            extraWidth += borderPaddingMarginStart(parentAsRenderInline);
        if (end && !child->nextSibling())
            extraWidth += borderPaddingMarginEnd(parentAsRenderInline);
        child = parent;
        parent = child->parent();
    }
    return extraWidth;
}

static void checkMidpoints(LineMidpointState& lineMidpointState, InlineIterator& lBreak)
{
    // Check to see if our last midpoint is a start point beyond the line break.  If so,
    // shave it off the list, and shave off a trailing space if the previous end point doesn't
    // preserve whitespace.
    if (lBreak.m_obj && lineMidpointState.numMidpoints && !(lineMidpointState.numMidpoints % 2)) {
        InlineIterator* midpoints = lineMidpointState.midpoints.data();
        InlineIterator& endpoint = midpoints[lineMidpointState.numMidpoints - 2];
        const InlineIterator& startpoint = midpoints[lineMidpointState.numMidpoints - 1];
        InlineIterator currpoint = endpoint;
        while (!currpoint.atEnd() && currpoint != startpoint && currpoint != lBreak)
            currpoint.increment();
        if (currpoint == lBreak) {
            // We hit the line break before the start point.  Shave off the start point.
            lineMidpointState.numMidpoints--;
            if (endpoint.m_obj->style()->collapseWhiteSpace())
                endpoint.m_pos--;
        }
    }
}

static void addMidpoint(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    if (lineMidpointState.midpoints.size() <= lineMidpointState.numMidpoints)
        lineMidpointState.midpoints.grow(lineMidpointState.numMidpoints + 10);

    InlineIterator* midpoints = lineMidpointState.midpoints.data();
    midpoints[lineMidpointState.numMidpoints++] = midpoint;
}

static inline BidiRun* createRun(int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    return new (obj->renderArena()) BidiRun(start, end, obj, resolver.context(), resolver.dir());
}

void RenderBlock::appendRunsForObject(BidiRunList<BidiRun>& runs, int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    if (start > end || obj->isFloating() ||
        (obj->isPositioned() && !obj->style()->isOriginalDisplayInlineType() && !obj->container()->isRenderInline()))
        return;

    LineMidpointState& lineMidpointState = resolver.midpointState();
    bool haveNextMidpoint = (lineMidpointState.currentMidpoint < lineMidpointState.numMidpoints);
    InlineIterator nextMidpoint;
    if (haveNextMidpoint)
        nextMidpoint = lineMidpointState.midpoints[lineMidpointState.currentMidpoint];
    if (lineMidpointState.betweenMidpoints) {
        if (!(haveNextMidpoint && nextMidpoint.m_obj == obj))
            return;
        // This is a new start point. Stop ignoring objects and
        // adjust our start.
        lineMidpointState.betweenMidpoints = false;
        start = nextMidpoint.m_pos;
        lineMidpointState.currentMidpoint++;
        if (start < end)
            return appendRunsForObject(runs, start, end, obj, resolver);
    } else {
        if (!haveNextMidpoint || (obj != nextMidpoint.m_obj)) {
            runs.addRun(createRun(start, end, obj, resolver));
            return;
        }

        // An end midpoint has been encountered within our object.  We
        // need to go ahead and append a run with our endpoint.
        if (static_cast<int>(nextMidpoint.m_pos + 1) <= end) {
            lineMidpointState.betweenMidpoints = true;
            lineMidpointState.currentMidpoint++;
            if (nextMidpoint.m_pos != UINT_MAX) { // UINT_MAX means stop at the object and don't include any of it.
                if (static_cast<int>(nextMidpoint.m_pos + 1) > start)
                    runs.addRun(createRun(start, nextMidpoint.m_pos + 1, obj, resolver));
                return appendRunsForObject(runs, nextMidpoint.m_pos + 1, end, obj, resolver);
            }
        } else
           runs.addRun(createRun(start, end, obj, resolver));
    }
}

static inline InlineBox* createInlineBoxForRenderer(RenderObject* obj, bool isRootLineBox, bool isOnlyRun = false)
{
    if (isRootLineBox)
        return toRenderBlock(obj)->createAndAppendRootInlineBox();

    if (obj->isText()) {
        InlineTextBox* textBox = toRenderText(obj)->createInlineTextBox();
        // We only treat a box as text for a <br> if we are on a line by ourself or in strict mode
        // (Note the use of strict mode.  In "almost strict" mode, we don't treat the box for <br> as text.)
        if (obj->isBR())
            textBox->setIsText(isOnlyRun || obj->document()->inNoQuirksMode());
        return textBox;
    }

    if (obj->isBox())
        return toRenderBox(obj)->createInlineBox();

    return toRenderInline(obj)->createAndAppendInlineFlowBox();
}

static inline void dirtyLineBoxesForRenderer(RenderObject* o, bool fullLayout)
{
    if (o->isText()) {
        if (o->preferredLogicalWidthsDirty() && (o->isCounter() || o->isQuote()))
            toRenderText(o)->computePreferredLogicalWidths(0); // FIXME: Counters depend on this hack. No clue why. Should be investigated and removed.
        toRenderText(o)->dirtyLineBoxes(fullLayout);
    } else
        toRenderInline(o)->dirtyLineBoxes(fullLayout);
}

static bool parentIsConstructedOrHaveNext(InlineFlowBox* parentBox)
{
    do {
        if (parentBox->isConstructed() || parentBox->nextOnLine())
            return true;
        parentBox = parentBox->parent();
    } while (parentBox);
    return false;
}

InlineFlowBox* RenderBlock::createLineBoxes(RenderObject* obj, const LineInfo& lineInfo, InlineBox* childBox)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    unsigned lineDepth = 1;
    InlineFlowBox* parentBox = 0;
    InlineFlowBox* result = 0;
    bool hasDefaultLineBoxContain = style()->lineBoxContain() == RenderStyle::initialLineBoxContain();
    do {
        ASSERT(obj->isRenderInline() || obj == this);

        RenderInline* inlineFlow = (obj != this) ? toRenderInline(obj) : 0;

        // Get the last box we made for this render object.
        parentBox = inlineFlow ? inlineFlow->lastLineBox() : toRenderBlock(obj)->lastLineBox();

        // If this box or its ancestor is constructed then it is from a previous line, and we need
        // to make a new box for our line.  If this box or its ancestor is unconstructed but it has
        // something following it on the line, then we know we have to make a new box
        // as well.  In this situation our inline has actually been split in two on
        // the same line (this can happen with very fancy language mixtures).
        bool constructedNewBox = false;
        bool allowedToConstructNewBox = !hasDefaultLineBoxContain || !inlineFlow || inlineFlow->alwaysCreateLineBoxes();
        bool canUseExistingParentBox = parentBox && !parentIsConstructedOrHaveNext(parentBox);
        if (allowedToConstructNewBox && !canUseExistingParentBox) {
            // We need to make a new box for this render object.  Once
            // made, we need to place it at the end of the current line.
            InlineBox* newBox = createInlineBoxForRenderer(obj, obj == this);
            ASSERT(newBox->isInlineFlowBox());
            parentBox = static_cast<InlineFlowBox*>(newBox);
            parentBox->setFirstLineStyleBit(lineInfo.isFirstLine());
            parentBox->setIsHorizontal(isHorizontalWritingMode());
            if (!hasDefaultLineBoxContain)
                parentBox->clearDescendantsHaveSameLineHeightAndBaseline();
            constructedNewBox = true;
        }

        if (constructedNewBox || canUseExistingParentBox) {
            if (!result)
                result = parentBox;

            // If we have hit the block itself, then |box| represents the root
            // inline box for the line, and it doesn't have to be appended to any parent
            // inline.
            if (childBox)
                parentBox->addToLine(childBox);

            if (!constructedNewBox || obj == this)
                break;

            childBox = parentBox;
        }

        // If we've exceeded our line depth, then jump straight to the root and skip all the remaining
        // intermediate inline flows.
        obj = (++lineDepth >= cMaxLineDepth) ? this : obj->parent();

    } while (true);

    return result;
}

static bool reachedEndOfTextRenderer(const BidiRunList<BidiRun>& bidiRuns)
{
    BidiRun* run = bidiRuns.logicallyLastRun();
    if (!run)
        return true;
    unsigned int pos = run->stop();
    RenderObject* r = run->m_object;
    if (!r->isText() || r->isBR())
        return false;
    RenderText* renderText = toRenderText(r);
    if (pos >= renderText->textLength())
        return true;

    while (isASCIISpace(renderText->characters()[pos])) {
        pos++;
        if (pos >= renderText->textLength())
            return true;
    }
    return false;
}

RootInlineBox* RenderBlock::constructLine(BidiRunList<BidiRun>& bidiRuns, const LineInfo& lineInfo)
{
    ASSERT(bidiRuns.firstRun());

    bool rootHasSelectedChildren = false;
    InlineFlowBox* parentBox = 0;
    for (BidiRun* r = bidiRuns.firstRun(); r; r = r->next()) {
        // Create a box for our object.
        bool isOnlyRun = (bidiRuns.runCount() == 1);
        if (bidiRuns.runCount() == 2 && !r->m_object->isListMarker())
            isOnlyRun = (!style()->isLeftToRightDirection() ? bidiRuns.lastRun() : bidiRuns.firstRun())->m_object->isListMarker();

        InlineBox* box = createInlineBoxForRenderer(r->m_object, false, isOnlyRun);
        r->m_box = box;

        ASSERT(box);
        if (!box)
            continue;

        if (!rootHasSelectedChildren && box->renderer()->selectionState() != RenderObject::SelectionNone)
            rootHasSelectedChildren = true;

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box.
        if (!parentBox || parentBox->renderer() != r->m_object->parent())
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->m_object->parent(), lineInfo, box);
        else {
            // Append the inline box to this line.
            parentBox->addToLine(box);
        }

        bool visuallyOrdered = r->m_object->style()->visuallyOrdered();
        box->setBidiLevel(r->level());

        if (box->isInlineTextBox()) {
            InlineTextBox* text = static_cast<InlineTextBox*>(box);
            text->setStart(r->m_start);
            text->setLen(r->m_stop - r->m_start);
            text->m_dirOverride = r->dirOverride(visuallyOrdered);
            if (r->m_hasHyphen)
                text->setHasHyphen(true);
        }
    }

    // We should have a root inline box.  It should be unconstructed and
    // be the last continuation of our line list.
    ASSERT(lastLineBox() && !lastLineBox()->isConstructed());

    // Set the m_selectedChildren flag on the root inline box if one of the leaf inline box
    // from the bidi runs walk above has a selection state.
    if (rootHasSelectedChildren)
        lastLineBox()->root()->setHasSelectedChildren(true);

    // Set bits on our inline flow boxes that indicate which sides should
    // paint borders/margins/padding.  This knowledge will ultimately be used when
    // we determine the horizontal positions and widths of all the inline boxes on
    // the line.
    bool isLogicallyLastRunWrapped = bidiRuns.logicallyLastRun()->m_object && bidiRuns.logicallyLastRun()->m_object->isText() ? !reachedEndOfTextRenderer(bidiRuns) : true;
    lastLineBox()->determineSpacingForFlowBoxes(lineInfo.isLastLine(), isLogicallyLastRunWrapped, bidiRuns.logicallyLastRun()->m_object);

    // Now mark the line boxes as being constructed.
    lastLineBox()->setConstructed();

    // Return the last line.
    return lastRootBox();
}

ETextAlign RenderBlock::textAlignmentForLine(bool endsWithSoftBreak) const
{
    ETextAlign alignment = style()->textAlign();
    if (!endsWithSoftBreak && alignment == JUSTIFY)
        alignment = TAAUTO;

    return alignment;
}

static void updateLogicalWidthForLeftAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // The direction of the block should determine what happens with wide lines.
    // In particular with RTL blocks, wide lines should still spill out to the left.
    if (isLeftToRightDirection) {
        if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
            trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        return;
    }

    if (trailingSpaceRun)
        trailingSpaceRun->m_box->setLogicalWidth(0);
    else if (totalLogicalWidth > availableLogicalWidth)
        logicalLeft -= (totalLogicalWidth - availableLogicalWidth);
}

static void updateLogicalWidthForRightAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // Wide lines spill out of the block based off direction.
    // So even if text-align is right, if direction is LTR, wide lines should overflow out of the right
    // side of the block.
    if (isLeftToRightDirection) {
        if (trailingSpaceRun) {
            totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
            trailingSpaceRun->m_box->setLogicalWidth(0);
        }
        if (totalLogicalWidth < availableLogicalWidth)
            logicalLeft += availableLogicalWidth - totalLogicalWidth;
        return;
    }

    if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun) {
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
    } else
        logicalLeft += availableLogicalWidth - totalLogicalWidth;
}

static void updateLogicalWidthForCenterAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    float trailingSpaceWidth = 0;
    if (trailingSpaceRun) {
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
        trailingSpaceWidth = min(trailingSpaceRun->m_box->logicalWidth(), (availableLogicalWidth - totalLogicalWidth + 1) / 2);
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceWidth));
    }
    if (isLeftToRightDirection)
        logicalLeft += max<float>((availableLogicalWidth - totalLogicalWidth) / 2, 0);
    else
        logicalLeft += totalLogicalWidth > availableLogicalWidth ? (availableLogicalWidth - totalLogicalWidth) : (availableLogicalWidth - totalLogicalWidth) / 2 - trailingSpaceWidth;
}

void RenderBlock::setMarginsForRubyRun(BidiRun* run, RenderRubyRun* renderer, RenderObject* previousObject, const LineInfo& lineInfo)
{
    int startOverhang;
    int endOverhang;
    RenderObject* nextObject = 0;
    for (BidiRun* runWithNextObject = run->next(); runWithNextObject; runWithNextObject = runWithNextObject->next()) {
        if (!runWithNextObject->m_object->isPositioned() && !runWithNextObject->m_box->isLineBreak()) {
            nextObject = runWithNextObject->m_object;
            break;
        }
    }
    renderer->getOverhang(lineInfo.isFirstLine(), renderer->style()->isLeftToRightDirection() ? previousObject : nextObject, renderer->style()->isLeftToRightDirection() ? nextObject : previousObject, startOverhang, endOverhang);
    setMarginStartForChild(renderer, -startOverhang);
    setMarginEndForChild(renderer, -endOverhang);
}

static inline void setLogicalWidthForTextRun(RootInlineBox* lineBox, BidiRun* run, RenderText* renderer, float xPos, const LineInfo& lineInfo,
                                   GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache)
{
    HashSet<const SimpleFontData*> fallbackFonts;
    GlyphOverflow glyphOverflow;
    
    // Always compute glyph overflow if the block's line-box-contain value is "glyphs".
    if (lineBox->fitsToGlyphs()) {
        // If we don't stick out of the root line's font box, then don't bother computing our glyph overflow. This optimization
        // will keep us from computing glyph bounds in nearly all cases.
        bool includeRootLine = lineBox->includesRootLineBoxFontOrLeading();
        int baselineShift = lineBox->verticalPositionForBox(run->m_box, verticalPositionCache);
        int rootDescent = includeRootLine ? lineBox->renderer()->style(lineInfo.isFirstLine())->font().fontMetrics().descent() : 0;
        int rootAscent = includeRootLine ? lineBox->renderer()->style(lineInfo.isFirstLine())->font().fontMetrics().ascent() : 0;
        int boxAscent = renderer->style(lineInfo.isFirstLine())->font().fontMetrics().ascent() - baselineShift;
        int boxDescent = renderer->style(lineInfo.isFirstLine())->font().fontMetrics().descent() + baselineShift;
        if (boxAscent > rootDescent ||  boxDescent > rootAscent)
            glyphOverflow.computeBounds = true; 
    }
    
    int hyphenWidth = 0;
    if (static_cast<InlineTextBox*>(run->m_box)->hasHyphen()) {
        const AtomicString& hyphenString = renderer->style()->hyphenString();
        hyphenWidth = renderer->style(lineInfo.isFirstLine())->font().width(TextRun(hyphenString.characters(), hyphenString.length()));
    }
    run->m_box->setLogicalWidth(renderer->width(run->m_start, run->m_stop - run->m_start, xPos, lineInfo.isFirstLine(), &fallbackFonts, &glyphOverflow) + hyphenWidth);
    if (!fallbackFonts.isEmpty()) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(static_cast<InlineTextBox*>(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).first;
        ASSERT(it->second.first.isEmpty());
        copyToVector(fallbackFonts, it->second.first);
        run->m_box->parent()->clearDescendantsHaveSameLineHeightAndBaseline();
    }
    if ((glyphOverflow.top || glyphOverflow.bottom || glyphOverflow.left || glyphOverflow.right)) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(static_cast<InlineTextBox*>(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).first;
        it->second.second = glyphOverflow;
        run->m_box->clearKnownToHaveNoOverflow();
    }
}

static inline void computeExpansionForJustifiedText(BidiRun* firstRun, BidiRun* trailingSpaceRun, Vector<unsigned, 16>& expansionOpportunities, unsigned expansionOpportunityCount, float& totalLogicalWidth, float availableLogicalWidth)
{
    if (!expansionOpportunityCount || availableLogicalWidth <= totalLogicalWidth)
        return;

    size_t i = 0;
    for (BidiRun* r = firstRun; r; r = r->next()) {
        if (!r->m_box || r == trailingSpaceRun)
            continue;
        
        if (r->m_object->isText()) {
            unsigned opportunitiesInRun = expansionOpportunities[i++];
            
            ASSERT(opportunitiesInRun <= expansionOpportunityCount);
            
            // Only justify text if whitespace is collapsed.
            if (r->m_object->style()->collapseWhiteSpace()) {
                InlineTextBox* textBox = static_cast<InlineTextBox*>(r->m_box);
                int expansion = (availableLogicalWidth - totalLogicalWidth) * opportunitiesInRun / expansionOpportunityCount;
                textBox->setExpansion(expansion);
                totalLogicalWidth += expansion;
            }
            expansionOpportunityCount -= opportunitiesInRun;
            if (!expansionOpportunityCount)
                break;
        }
    }
}

void RenderBlock::computeInlineDirectionPositionsForLine(RootInlineBox* lineBox, const LineInfo& lineInfo, BidiRun* firstRun, BidiRun* trailingSpaceRun, bool reachedEnd,
                                                         GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache)
{
    ETextAlign textAlign = textAlignmentForLine(!reachedEnd && !lineBox->endsWithBreak());
    float logicalLeft = logicalLeftOffsetForLine(logicalHeight(), lineInfo.isFirstLine());
    float availableLogicalWidth = logicalRightOffsetForLine(logicalHeight(), lineInfo.isFirstLine()) - logicalLeft;

    bool needsWordSpacing = false;
    float totalLogicalWidth = lineBox->getFlowSpacingLogicalWidth();
    unsigned expansionOpportunityCount = 0;
    bool isAfterExpansion = true;
    Vector<unsigned, 16> expansionOpportunities;
    RenderObject* previousObject = 0;

    for (BidiRun* r = firstRun; r; r = r->next()) {
        if (!r->m_box || r->m_object->isPositioned() || r->m_box->isLineBreak())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
                      // Similarly, line break boxes have no effect on the width.
        if (r->m_object->isText()) {
            RenderText* rt = toRenderText(r->m_object);
            if (textAlign == JUSTIFY && r != trailingSpaceRun) {
                if (!isAfterExpansion)
                    static_cast<InlineTextBox*>(r->m_box)->setCanHaveLeadingExpansion(true);
                unsigned opportunitiesInRun = Font::expansionOpportunityCount(rt->characters() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                expansionOpportunities.append(opportunitiesInRun);
                expansionOpportunityCount += opportunitiesInRun;
            }

            if (int length = rt->textLength()) {
                if (!r->m_start && needsWordSpacing && isSpaceOrNewline(rt->characters()[r->m_start]))
                    totalLogicalWidth += rt->style(lineInfo.isFirstLine())->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characters()[r->m_stop - 1]) && r->m_stop == length;
            }

            setLogicalWidthForTextRun(lineBox, r, rt, totalLogicalWidth, lineInfo, textBoxDataMap, verticalPositionCache);
        } else {
            isAfterExpansion = false;
            if (!r->m_object->isRenderInline()) {
                RenderBox* renderBox = toRenderBox(r->m_object);
                if (renderBox->isRubyRun())
                    setMarginsForRubyRun(r, toRenderRubyRun(renderBox), previousObject, lineInfo);
                r->m_box->setLogicalWidth(logicalWidthForChild(renderBox));
                totalLogicalWidth += marginStartForChild(renderBox) + marginEndForChild(renderBox);
            }
        }

        totalLogicalWidth += r->m_box->logicalWidth();
        previousObject = r->m_object;
    }

    if (isAfterExpansion && !expansionOpportunities.isEmpty()) {
        expansionOpportunities.last()--;
        expansionOpportunityCount--;
    }

    // Armed with the total width of the line (without justification),
    // we now examine our text-align property in order to determine where to position the
    // objects horizontally.  The total width of the line can be increased if we end up
    // justifying text.
    switch (textAlign) {
        case LEFT:
        case WEBKIT_LEFT:
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case JUSTIFY:
            adjustInlineDirectionLineBounds(expansionOpportunityCount, logicalLeft, availableLogicalWidth);
            if (expansionOpportunityCount) {
                if (trailingSpaceRun) {
                    totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
                    trailingSpaceRun->m_box->setLogicalWidth(0);
                }
                break;
            }
            // fall through
        case TAAUTO:
            // for right to left fall through to right aligned
            if (style()->isLeftToRightDirection()) {
                if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
                    trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
                break;
            }
        case RIGHT:
        case WEBKIT_RIGHT:
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case CENTER:
        case WEBKIT_CENTER:
            updateLogicalWidthForCenterAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case TASTART:
            if (style()->isLeftToRightDirection())
                updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            else
                updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case TAEND:
            if (style()->isLeftToRightDirection())
                updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            else
                updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
    }

    computeExpansionForJustifiedText(firstRun, trailingSpaceRun, expansionOpportunities, expansionOpportunityCount, totalLogicalWidth, availableLogicalWidth);

    // The widths of all runs are now known.  We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    needsWordSpacing = false;
    lineBox->placeBoxesInInlineDirection(logicalLeft, needsWordSpacing, textBoxDataMap);
}

void RenderBlock::computeBlockDirectionPositionsForLine(RootInlineBox* lineBox, BidiRun* firstRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
                                                        VerticalPositionCache& verticalPositionCache)
{
    setLogicalHeight(lineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache));
    lineBox->setBlockLogicalHeight(logicalHeight());

    // Now make sure we place replaced render objects correctly.
    for (BidiRun* r = firstRun; r; r = r->next()) {
        ASSERT(r->m_box);
        if (!r->m_box)
            continue; // Skip runs with no line boxes.

        // Align positioned boxes with the top of the line box.  This is
        // a reasonable approximation of an appropriate y position.
        if (r->m_object->isPositioned())
            r->m_box->setLogicalTop(logicalHeight());

        // Position is used to properly position both replaced elements and
        // to update the static normal flow x/y of positioned elements.
        if (r->m_object->isText())
            toRenderText(r->m_object)->positionLineBox(r->m_box);
        else if (r->m_object->isBox())
            toRenderBox(r->m_object)->positionLineBox(r->m_box);
    }
    // Positioned objects and zero-length text nodes destroy their boxes in
    // position(), which unnecessarily dirties the line.
    lineBox->markDirty(false);
}

static inline bool isCollapsibleSpace(UChar character, RenderText* renderer)
{
    if (character == ' ' || character == '\t' || character == softHyphen)
        return true;
    if (character == '\n')
        return !renderer->style()->preserveNewline();
    if (character == noBreakSpace)
        return renderer->style()->nbspMode() == SPACE;
    return false;
}


static void setStaticPositions(RenderBlock* block, RenderBox* child)
{
    // FIXME: The math here is actually not really right. It's a best-guess approximation that
    // will work for the common cases
    RenderObject* containerBlock = child->container();
    int blockHeight = block->logicalHeight();
    if (containerBlock->isRenderInline()) {
        // A relative positioned inline encloses us. In this case, we also have to determine our
        // position as though we were an inline. Set |staticInlinePosition| and |staticBlockPosition| on the relative positioned
        // inline so that we can obtain the value later.
        toRenderInline(containerBlock)->layer()->setStaticInlinePosition(block->startOffsetForLine(blockHeight, false));
        toRenderInline(containerBlock)->layer()->setStaticBlockPosition(blockHeight);
    }

    if (child->style()->isOriginalDisplayInlineType())
        child->layer()->setStaticInlinePosition(block->startOffsetForLine(blockHeight, false));
    else
        child->layer()->setStaticInlinePosition(block->borderAndPaddingStart());
    child->layer()->setStaticBlockPosition(blockHeight);
}

inline BidiRun* RenderBlock::handleTrailingSpaces(BidiRunList<BidiRun>& bidiRuns, BidiContext* currentContext)
{
    if (!bidiRuns.runCount()
        || !bidiRuns.logicallyLastRun()->m_object->style()->breakOnlyAfterWhiteSpace()
        || !bidiRuns.logicallyLastRun()->m_object->style()->autoWrap())
        return 0;

    BidiRun* trailingSpaceRun = bidiRuns.logicallyLastRun();
    RenderObject* lastObject = trailingSpaceRun->m_object;
    if (!lastObject->isText())
        return 0;

    RenderText* lastText = toRenderText(lastObject);
    const UChar* characters = lastText->characters();
    int firstSpace = trailingSpaceRun->stop();
    while (firstSpace > trailingSpaceRun->start()) {
        UChar current = characters[firstSpace - 1];
        if (!isCollapsibleSpace(current, lastText))
            break;
        firstSpace--;
    }
    if (firstSpace == trailingSpaceRun->stop())
        return 0;

    TextDirection direction = style()->direction();
    bool shouldReorder = trailingSpaceRun != (direction == LTR ? bidiRuns.lastRun() : bidiRuns.firstRun());
    if (firstSpace != trailingSpaceRun->start()) {
        BidiContext* baseContext = currentContext;
        while (BidiContext* parent = baseContext->parent())
            baseContext = parent;

        BidiRun* newTrailingRun = new (renderArena()) BidiRun(firstSpace, trailingSpaceRun->m_stop, trailingSpaceRun->m_object, baseContext, OtherNeutral);
        trailingSpaceRun->m_stop = firstSpace;
        if (direction == LTR)
            bidiRuns.addRun(newTrailingRun);
        else
            bidiRuns.prependRun(newTrailingRun);
        trailingSpaceRun = newTrailingRun;
        return trailingSpaceRun;
    }
    if (!shouldReorder)
        return trailingSpaceRun;

    if (direction == LTR) {
        bidiRuns.moveRunToEnd(trailingSpaceRun);
        trailingSpaceRun->m_level = 0;
    } else {
        bidiRuns.moveRunToBeginning(trailingSpaceRun);
        trailingSpaceRun->m_level = 1;
    }
    return trailingSpaceRun;
}

void RenderBlock::appendFloatingObjectToLastLine(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->m_originatingLine);
    floatingObject->m_originatingLine = lastRootBox();
    lastRootBox()->appendFloat(floatingObject->renderer());
}

// This function constructs line boxes for all of the text runs in the resolver and computes their position.
RootInlineBox* RenderBlock::createLineBoxesFromBidiRuns(BidiRunList<BidiRun>& bidiRuns, const InlineIterator& end, LineInfo& lineInfo, VerticalPositionCache& verticalPositionCache, BidiRun* trailingSpaceRun)
{
    if (!bidiRuns.runCount())
        return 0;

    // FIXME: Why is this only done when we had runs?
    lineInfo.setLastLine(!end.m_obj);

    RootInlineBox* lineBox = constructLine(bidiRuns, lineInfo);
    if (!lineBox)
        return 0;

    lineBox->setEndsWithBreak(lineInfo.previousLineBrokeCleanly());
    
#if ENABLE(SVG)
    bool isSVGRootInlineBox = lineBox->isSVGRootInlineBox();
#else
    bool isSVGRootInlineBox = false;
#endif
    
    GlyphOverflowAndFallbackFontsMap textBoxDataMap;
    
    // Now we position all of our text runs horizontally.
    if (!isSVGRootInlineBox)
        computeInlineDirectionPositionsForLine(lineBox, lineInfo, bidiRuns.firstRun(), trailingSpaceRun, end.atEnd(), textBoxDataMap, verticalPositionCache);
    
    // Now position our text runs vertically.
    computeBlockDirectionPositionsForLine(lineBox, bidiRuns.firstRun(), textBoxDataMap, verticalPositionCache);
    
#if ENABLE(SVG)
    // SVG text layout code computes vertical & horizontal positions on its own.
    // Note that we still need to execute computeVerticalPositionsForLine() as
    // it calls InlineTextBox::positionLineBox(), which tracks whether the box
    // contains reversed text or not. If we wouldn't do that editing and thus
    // text selection in RTL boxes would not work as expected.
    if (isSVGRootInlineBox) {
        ASSERT(isSVGText());
        static_cast<SVGRootInlineBox*>(lineBox)->computePerCharacterLayoutInformation();
    }
#endif
    
    // Compute our overflow now.
    lineBox->computeOverflow(lineBox->lineTop(), lineBox->lineBottom(), textBoxDataMap);
    
#if PLATFORM(MAC)
    // Highlight acts as an overflow inflation.
    if (style()->highlight() != nullAtom)
        lineBox->addHighlightOverflow();
#endif
    return lineBox;
}

void RenderBlock::layoutRunsAndFloats(bool fullLayout, bool hasInlineChild, Vector<FloatWithRect>& floats, int& repaintLogicalTop, int& repaintLogicalBottom)
{
    // We want to skip ahead to the first dirty line
    InlineBidiResolver resolver;
    unsigned floatIndex;
    LineInfo lineInfo;
    bool useRepaintBounds = false;

    RootInlineBox* startLine = determineStartPosition(lineInfo, fullLayout, resolver, floats, floatIndex,
                                                      useRepaintBounds, repaintLogicalTop, repaintLogicalBottom);

    // FIXME: This would make more sense outside of this function, but since
    // determineStartPosition can change the fullLayout flag we have to do this here. Failure to call
    // determineStartPosition first will break fast/repaint/line-flow-with-floats-9.html.
    if (fullLayout && hasInlineChild && !selfNeedsLayout()) {
        setNeedsLayout(true, false);  // Mark ourselves as needing a full layout. This way we'll repaint like
        // we're supposed to.
        RenderView* v = view();
        if (v && !v->doingFullRepaint() && hasLayer()) {
            // Because we waited until we were already inside layout to discover
            // that the block really needed a full layout, we missed our chance to repaint the layer
            // before layout started.  Luckily the layer has cached the repaint rect for its original
            // position and size, and so we can use that to make a repaint happen now.
            repaintUsingContainer(containerForRepaint(), layer()->repaintRect());
        }
    }

    FloatingObject* lastFloat = (m_floatingObjects && !m_floatingObjects->set().isEmpty()) ? m_floatingObjects->set().last() : 0;

    LineMidpointState& lineMidpointState = resolver.midpointState();

    // We also find the first clean line and extract these lines.  We will add them back
    // if we determine that we're able to synchronize after handling all our dirty lines.
    InlineIterator cleanLineStart;
    BidiStatus cleanLineBidiStatus;
    int endLineLogicalTop = 0;
    RootInlineBox* endLine = (fullLayout || !startLine) ?
        0 : determineEndPosition(startLine, floats, floatIndex, cleanLineStart, cleanLineBidiStatus, endLineLogicalTop);

    if (startLine) {
        if (!useRepaintBounds) {
            useRepaintBounds = true;
            repaintLogicalTop = logicalHeight();
            repaintLogicalBottom = logicalHeight();
        }
        RenderArena* arena = renderArena();
        RootInlineBox* box = startLine;
        while (box) {
            repaintLogicalTop = min(repaintLogicalTop, box->logicalTopVisualOverflow());
            repaintLogicalBottom = max(repaintLogicalBottom, box->logicalBottomVisualOverflow());
            RootInlineBox* next = box->nextRootBox();
            box->deleteLine(arena);
            box = next;
        }
    }

    InlineIterator end = resolver.position();

    if (!fullLayout && lastRootBox() && lastRootBox()->endsWithBreak()) {
        // If the last line before the start line ends with a line break that clear floats,
        // adjust the height accordingly.
        // A line break can be either the first or the last object on a line, depending on its direction.
        if (InlineBox* lastLeafChild = lastRootBox()->lastLeafChild()) {
            RenderObject* lastObject = lastLeafChild->renderer();
            if (!lastObject->isBR())
                lastObject = lastRootBox()->firstLeafChild()->renderer();
            if (lastObject->isBR()) {
                EClear clear = lastObject->style()->clear();
                if (clear != CNONE)
                    newLine(clear);
            }
        }
    }

    bool endLineMatched = false;
    bool checkForEndLineMatch = endLine;
    bool checkForFloatsFromLastLine = false;

    bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();

    LineBreakIteratorInfo lineBreakIteratorInfo;
    VerticalPositionCache verticalPositionCache;

    LineBreaker lineBreaker(this);

    while (!end.atEnd()) {
        // FIXME: Is this check necessary before the first iteration or can it be moved to the end?
        if (checkForEndLineMatch && (endLineMatched = matchedEndLine(resolver, cleanLineStart, cleanLineBidiStatus, endLine, endLineLogicalTop, repaintLogicalBottom, repaintLogicalTop)))
            break;

        lineMidpointState.reset();

        lineInfo.setEmpty(true);

        InlineIterator oldEnd = end;
        FloatingObject* lastFloatFromPreviousLine = (m_floatingObjects && !m_floatingObjects->set().isEmpty()) ? m_floatingObjects->set().last() : 0;
        end = lineBreaker.nextLineBreak(resolver, lineInfo, lineBreakIteratorInfo, lastFloatFromPreviousLine);
        if (resolver.position().atEnd()) {
            // FIXME: We shouldn't be creating any runs in findNextLineBreak to begin with!
            // Once BidiRunList is separated from BidiResolver this will not be needed.
            resolver.runs().deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).
            checkForFloatsFromLastLine = true;
            break;
        }
        ASSERT(end != resolver.position());

        // This is a short-cut for empty lines.
        if (lineInfo.isEmpty()) {
            if (lastRootBox())
                lastRootBox()->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
        } else {
            VisualDirectionOverride override = (style()->visuallyOrdered() ? (style()->direction() == LTR ? VisualLeftToRightOverride : VisualRightToLeftOverride) : NoVisualOverride);
            // FIXME: This ownership is reversed. We should own the BidiRunList and pass it to createBidiRunsForLine.
            BidiRunList<BidiRun>& bidiRuns = resolver.runs();
            resolver.createBidiRunsForLine(end, override, lineInfo.previousLineBrokeCleanly());
            ASSERT(resolver.position() == end);

            BidiRun* trailingSpaceRun = !lineInfo.previousLineBrokeCleanly() ? handleTrailingSpaces(bidiRuns, resolver.context()) : 0;

            if (bidiRuns.runCount() && lineBreaker.lineWasHyphenated())
                bidiRuns.logicallyLastRun()->m_hasHyphen = true;

            // Now that the runs have been ordered, we create the line boxes.
            // At the same time we figure out where border/padding/margin should be applied for
            // inline flow boxes.

            int oldLogicalHeight = logicalHeight();
            RootInlineBox* lineBox = createLineBoxesFromBidiRuns(bidiRuns, end, lineInfo, verticalPositionCache, trailingSpaceRun);

            bidiRuns.deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).

            if (lineBox) {
                lineBox->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
                if (useRepaintBounds) {
                    repaintLogicalTop = min(repaintLogicalTop, lineBox->logicalTopVisualOverflow());
                    repaintLogicalBottom = max(repaintLogicalBottom, lineBox->logicalBottomVisualOverflow());
                }

                // table cell pagination is handled in RenderTableSection
                if (paginated && !isTableCell()) {
                    int adjustment = 0;
                    adjustLinePositionForPagination(lineBox, adjustment);
                    if (adjustment) {
                        int oldLineWidth = availableLogicalWidthForLine(oldLogicalHeight, lineInfo.isFirstLine());
                        lineBox->adjustBlockDirectionPosition(adjustment);
                        if (useRepaintBounds) // This can only be a positive adjustment, so no need to update repaintTop.
                            repaintLogicalBottom = max(repaintLogicalBottom, lineBox->logicalBottomVisualOverflow());

                        if (availableLogicalWidthForLine(oldLogicalHeight + adjustment, lineInfo.isFirstLine()) != oldLineWidth) {
                            // We have to delete this line, remove all floats that got added, and let line layout re-run.
                            lineBox->deleteLine(renderArena());
                            removeFloatingObjectsBelow(lastFloatFromPreviousLine, oldLogicalHeight);
                            setLogicalHeight(oldLogicalHeight + adjustment);
                            resolver.setPosition(oldEnd);
                            end = oldEnd;
                            continue;
                        }

                        setLogicalHeight(lineBox->blockLogicalHeight());
                    }
                }
            }

            for (size_t i = 0; i < lineBreaker.positionedObjects().size(); ++i)
                setStaticPositions(this, lineBreaker.positionedObjects()[i]);

            lineInfo.setFirstLine(false);
            newLine(lineBreaker.clear());
        }

        if (m_floatingObjects && lastRootBox()) {
            FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator it = floatingObjectSet.begin();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            if (lastFloat) {
                FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(lastFloat);
                ASSERT(lastFloatIterator != end);
                ++lastFloatIterator;
                it = lastFloatIterator;
            }
            for (; it != end; ++it) {
                FloatingObject* f = *it;
                appendFloatingObjectToLastLine(f);
                ASSERT(f->m_renderer == floats[floatIndex].object);
                // If a float's geometry has changed, give up on syncing with clean lines.
                if (floats[floatIndex].rect != f->frameRect())
                    checkForEndLineMatch = false;
                floatIndex++;
            }
            lastFloat = !floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0;
        }

        lineMidpointState.reset();
        resolver.setPosition(end);
    }

    if (endLine) {
        if (endLineMatched) {
            // Attach all the remaining lines, and then adjust their y-positions as needed.
            int delta = logicalHeight() - endLineLogicalTop;
            for (RootInlineBox* line = endLine; line; line = line->nextRootBox()) {
                line->attachLine();
                if (paginated) {
                    delta -= line->paginationStrut();
                    adjustLinePositionForPagination(line, delta);
                }
                if (delta) {
                    repaintLogicalTop = min(repaintLogicalTop, line->logicalTopVisualOverflow() + min(delta, 0));
                    repaintLogicalBottom = max(repaintLogicalBottom, line->logicalBottomVisualOverflow() + max(delta, 0));
                    line->adjustBlockDirectionPosition(delta);
                }
                if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                    for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                        FloatingObject* floatingObject = insertFloatingObject(*f);
                        ASSERT(!floatingObject->m_originatingLine);
                        floatingObject->m_originatingLine = line;
                        setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f) + delta);
                        positionNewFloats();
                    }
                }
            }
            setLogicalHeight(lastRootBox()->blockLogicalHeight());
        } else {
            // Delete all the remaining lines.
            RootInlineBox* line = endLine;
            RenderArena* arena = renderArena();
            while (line) {
                repaintLogicalTop = min(repaintLogicalTop, line->logicalTopVisualOverflow());
                repaintLogicalBottom = max(repaintLogicalBottom, line->logicalBottomVisualOverflow());
                RootInlineBox* next = line->nextRootBox();
                line->deleteLine(arena);
                line = next;
            }
        }
    }
    if (m_floatingObjects && (checkForFloatsFromLastLine || positionNewFloats()) && lastRootBox()) {
        // In case we have a float on the last line, it might not be positioned up to now.
        // This has to be done before adding in the bottom border/padding, or the float will
        // include the padding incorrectly. -dwh
        if (checkForFloatsFromLastLine) {
            int bottomVisualOverflow = lastRootBox()->logicalBottomVisualOverflow();
            int bottomLayoutOverflow = lastRootBox()->logicalBottomLayoutOverflow();
            TrailingFloatsRootInlineBox* trailingFloatsLineBox = new (renderArena()) TrailingFloatsRootInlineBox(this);
            m_lineBoxes.appendLineBox(trailingFloatsLineBox);
            trailingFloatsLineBox->setConstructed();
            GlyphOverflowAndFallbackFontsMap textBoxDataMap;
            VerticalPositionCache verticalPositionCache;
            trailingFloatsLineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache);
            int blockLogicalHeight = logicalHeight();
            IntRect logicalLayoutOverflow(0, blockLogicalHeight, 1, bottomLayoutOverflow - blockLogicalHeight);
            IntRect logicalVisualOverflow(0, blockLogicalHeight, 1, bottomVisualOverflow - blockLogicalHeight);
            trailingFloatsLineBox->setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, trailingFloatsLineBox->lineTop(), trailingFloatsLineBox->lineBottom());
            trailingFloatsLineBox->setBlockLogicalHeight(logicalHeight());
        }

        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.begin();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        if (lastFloat) {
            FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(lastFloat);
            ASSERT(lastFloatIterator != end);
            ++lastFloatIterator;
            it = lastFloatIterator;
        }
        for (; it != end; ++it)
            appendFloatingObjectToLastLine(*it);
        lastFloat = !floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0;
    }
    size_t floatCount = floats.size();
    // Floats that did not have layout did not repaint when we laid them out. They would have
    // painted by now if they had moved, but if they stayed at (0, 0), they still need to be
    // painted.
    for (size_t i = 0; i < floatCount; ++i) {
        if (!floats[i].everHadLayout) {
            RenderBox* f = floats[i].object;
            if (!f->x() && !f->y() && f->checkForRepaintDuringLayout())
                f->repaint();
        }
    }
}

void RenderBlock::layoutInlineChildren(bool relayoutChildren, int& repaintLogicalTop, int& repaintLogicalBottom)
{
    m_overflow.clear();

    setLogicalHeight(borderBefore() + paddingBefore());

    // Figure out if we should clear out our line boxes.
    // FIXME: Handle resize eventually!
    bool fullLayout = !firstLineBox() || selfNeedsLayout() || relayoutChildren;
    if (fullLayout)
        lineBoxes()->deleteLineBoxes(renderArena());

    // Text truncation only kicks in if your overflow isn't visible and your text-overflow-mode isn't
    // clip.
    // FIXME: CSS3 says that descendants that are clipped must also know how to truncate.  This is insanely
    // difficult to figure out (especially in the middle of doing layout), and is really an esoteric pile of nonsense
    // anyway, so we won't worry about following the draft here.
    bool hasTextOverflow = style()->textOverflow() && hasOverflowClip();

    // Walk all the lines and delete our ellipsis line boxes if they exist.
    if (hasTextOverflow)
         deleteEllipsisLineBoxes();

    if (firstChild()) {
        // layout replaced elements
        bool endOfInline = false;
        RenderObject* o = bidiFirstNotSkippingInlines(this);
        Vector<FloatWithRect> floats;
        bool hasInlineChild = false;
        while (o) {
            if (!hasInlineChild && o->isInline())
                hasInlineChild = true;

            if (o->isReplaced() || o->isFloating() || o->isPositioned()) {
                RenderBox* box = toRenderBox(o);

                if (relayoutChildren || o->style()->width().isPercent() || o->style()->height().isPercent())
                    o->setChildNeedsLayout(true, false);

                // If relayoutChildren is set and we have percentage padding, we also need to invalidate the child's pref widths.
                if (relayoutChildren && (o->style()->paddingStart().isPercent() || o->style()->paddingEnd().isPercent()))
                    o->setPreferredLogicalWidthsDirty(true, false);

                if (o->isPositioned())
                    o->containingBlock()->insertPositionedObject(box);
                else if (o->isFloating())
                    floats.append(FloatWithRect(box));
                else if (fullLayout || o->needsLayout()) {
                    // Replaced elements
                    toRenderBox(o)->dirtyLineBoxes(fullLayout);
                    o->layoutIfNeeded();
                }
            } else if (o->isText() || (o->isRenderInline() && !endOfInline)) {
                if (!o->isText())
                    toRenderInline(o)->updateAlwaysCreateLineBoxes();
                if (fullLayout || o->selfNeedsLayout())
                    dirtyLineBoxesForRenderer(o, fullLayout);
                o->setNeedsLayout(false);
            }
            o = bidiNext(this, o, 0, false, &endOfInline);
        }

        layoutRunsAndFloats(fullLayout, hasInlineChild, floats, repaintLogicalTop, repaintLogicalBottom);
    }

    // Expand the last line to accommodate Ruby and emphasis marks.
    int lastLineAnnotationsAdjustment = 0;
    if (lastRootBox()) {
        int lowestAllowedPosition = max(lastRootBox()->lineBottom(), logicalHeight() + paddingAfter());
        if (!style()->isFlippedLinesWritingMode())
            lastLineAnnotationsAdjustment = lastRootBox()->computeUnderAnnotationAdjustment(lowestAllowedPosition);
        else
            lastLineAnnotationsAdjustment = lastRootBox()->computeOverAnnotationAdjustment(lowestAllowedPosition);
    }

    // Now add in the bottom border/padding.
    setLogicalHeight(logicalHeight() + lastLineAnnotationsAdjustment + borderAfter() + paddingAfter() + scrollbarLogicalHeight());

    if (!firstLineBox() && hasLineIfEmpty())
        setLogicalHeight(logicalHeight() + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

    // See if we have any lines that spill out of our block.  If we do, then we will possibly need to
    // truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();
}

void RenderBlock::checkFloatsInCleanLine(RootInlineBox* line, Vector<FloatWithRect>& floats, size_t& floatIndex, bool& encounteredNewFloat, bool& dirtiedByFloat)
{
    Vector<RenderBox*>* cleanLineFloats = line->floatsPtr();
    if (!cleanLineFloats)
        return;

    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
    for (Vector<RenderBox*>::iterator it = cleanLineFloats->begin(); it != end; ++it) {
        RenderBox* floatingBox = *it;
        floatingBox->layoutIfNeeded();
        IntSize newSize(floatingBox->width() + floatingBox->marginLeft() + floatingBox->marginRight(), floatingBox->height() + floatingBox->marginTop() + floatingBox->marginBottom());
        ASSERT(floatIndex < floats.size());
        if (floats[floatIndex].object != floatingBox) {
            encounteredNewFloat = true;
            return;
        }
        if (floats[floatIndex].rect.size() != newSize) {
            int floatTop = isHorizontalWritingMode() ? floats[floatIndex].rect.y() : floats[floatIndex].rect.x();
            int floatHeight = isHorizontalWritingMode() ? max(floats[floatIndex].rect.height(), newSize.height())
                                                                 : max(floats[floatIndex].rect.width(), newSize.width());
            floatHeight = min(floatHeight, numeric_limits<int>::max() - floatTop);
            line->markDirty();
            markLinesDirtyInBlockRange(line->blockLogicalHeight(), floatTop + floatHeight, line);
            floats[floatIndex].rect.setSize(newSize);
            dirtiedByFloat = true;
        }
        floatIndex++;
    }
}

RootInlineBox* RenderBlock::determineStartPosition(LineInfo& lineInfo, bool& fullLayout, InlineBidiResolver& resolver, Vector<FloatWithRect>& floats,
                                                   unsigned& numCleanFloats, bool& useRepaintBounds, int& repaintLogicalTop, int& repaintLogicalBottom)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;

    bool dirtiedByFloat = false;
    if (!fullLayout) {
        // Paginate all of the clean lines.
        bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
        int paginationDelta = 0;
        size_t floatIndex = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) {
            if (paginated) {
                paginationDelta -= curr->paginationStrut();
                adjustLinePositionForPagination(curr, paginationDelta);
                if (paginationDelta) {
                    if (containsFloats() || !floats.isEmpty()) {
                        // FIXME: Do better eventually.  For now if we ever shift because of pagination and floats are present just go to a full layout.
                        fullLayout = true;
                        break;
                    }

                    if (!useRepaintBounds)
                        useRepaintBounds = true;

                    repaintLogicalTop = min(repaintLogicalTop, curr->logicalTopVisualOverflow() + min(paginationDelta, 0));
                    repaintLogicalBottom = max(repaintLogicalBottom, curr->logicalBottomVisualOverflow() + max(paginationDelta, 0));
                    curr->adjustBlockDirectionPosition(paginationDelta);
                }
            }

            // If a new float has been inserted before this line or before its last known float,just do a full layout.
            checkFloatsInCleanLine(curr, floats, floatIndex, fullLayout, dirtiedByFloat);
            if (dirtiedByFloat || fullLayout)
                break;
        }
        // Check if a new float has been inserted after the last known float.
        if (!curr && floatIndex < floats.size())
            fullLayout = true;
    }

    if (fullLayout) {
        // Nuke all our lines.
        if (firstRootBox()) {
            RenderArena* arena = renderArena();
            curr = firstRootBox();
            while (curr) {
                RootInlineBox* next = curr->nextRootBox();
                curr->deleteLine(arena);
                curr = next;
            }
            ASSERT(!firstLineBox() && !lastLineBox());
        }
    } else {
        if (curr) {
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!dirtiedByFloat && (!prevRootBox->endsWithBreak() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= toRenderText(prevRootBox->lineBreakObj())->textLength())))
                    // The previous line didn't break cleanly or broke at a newline
                    // that has been deleted, so treat it as dirty too.
                    curr = prevRootBox;
            }
        } else {
            // No dirty lines were found.
            // If the last line didn't break cleanly, treat it as dirty.
            if (lastRootBox() && !lastRootBox()->endsWithBreak())
                curr = lastRootBox();
        }

        // If we have no dirty lines, then last is just the last root box.
        last = curr ? curr->prevRootBox() : lastRootBox();
    }

    numCleanFloats = 0;
    if (!floats.isEmpty()) {
        int savedLogicalHeight = logicalHeight();
        // Restore floats from clean lines.
        RootInlineBox* line = firstRootBox();
        while (line != curr) {
            if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                    FloatingObject* floatingObject = insertFloatingObject(*f);
                    ASSERT(!floatingObject->m_originatingLine);
                    floatingObject->m_originatingLine = line;
                    setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f));
                    positionNewFloats();
                    ASSERT(floats[numCleanFloats].object == *f);
                    numCleanFloats++;
                }
            }
            line = line->nextRootBox();
        }
        setLogicalHeight(savedLogicalHeight);
    }

    lineInfo.setFirstLine(!last);
    lineInfo.setPreviousLineBrokeCleanly(!last || last->endsWithBreak());

    if (last) {
        setLogicalHeight(last->blockLogicalHeight());
        resolver.setPosition(InlineIterator(this, last->lineBreakObj(), last->lineBreakPos()));
        resolver.setStatus(last->lineBreakBidiStatus());
    } else {
        resolver.setStatus(BidiStatus(style()->direction(), style()->unicodeBidi() == Override));
        resolver.setPosition(InlineIterator(this, bidiFirstSkippingInlines(this, &resolver), 0));
    }
    return curr;
}

RootInlineBox* RenderBlock::determineEndPosition(RootInlineBox* startLine, Vector<FloatWithRect>& floats, size_t floatIndex, InlineIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus, int& logicalTop)
{
    RootInlineBox* last = 0;
    for (RootInlineBox* curr = startLine->nextRootBox(); curr; curr = curr->nextRootBox()) {
        if (!curr->isDirty()) {
            bool encounteredNewFloat = false;
            bool dirtiedByFloat = false;
            checkFloatsInCleanLine(curr, floats, floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                return 0;
        }
        if (curr->isDirty())
            last = 0;
        else if (!last)
            last = curr;
    }

    if (!last)
        return 0;

    // At this point, |last| is the first line in a run of clean lines that ends with the last line
    // in the block.

    RootInlineBox* prev = last->prevRootBox();
    cleanLineStart = InlineIterator(this, prev->lineBreakObj(), prev->lineBreakPos());
    cleanLineBidiStatus = prev->lineBreakBidiStatus();
    logicalTop = prev->blockLogicalHeight();

    for (RootInlineBox* line = last; line; line = line->nextRootBox())
        line->extractLine(); // Disconnect all line boxes from their render objects while preserving
                             // their connections to one another.

    return last;
}

bool RenderBlock::matchedEndLine(const InlineBidiResolver& resolver, const InlineIterator& endLineStart, const BidiStatus& endLineStatus, RootInlineBox*& endLine,
                                 int& endLogicalTop, int& repaintLogicalBottom, int& repaintLogicalTop)
{
    if (resolver.position() == endLineStart) {
        if (resolver.status() != endLineStatus)
            return false;

        int delta = logicalHeight() - endLogicalTop;
        if (!delta || !m_floatingObjects)
            return true;

        // See if any floats end in the range along which we want to shift the lines vertically.
        int logicalTop = min(logicalHeight(), endLogicalTop);

        RootInlineBox* lastLine = endLine;
        while (RootInlineBox* nextLine = lastLine->nextRootBox())
            lastLine = nextLine;

        int logicalBottom = lastLine->blockLogicalHeight() + abs(delta);

        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
            FloatingObject* f = *it;
            if (logicalBottomForFloat(f) >= logicalTop && logicalBottomForFloat(f) < logicalBottom)
                return false;
        }

        return true;
    }

    // The first clean line doesn't match, but we can check a handful of following lines to try
    // to match back up.
    static int numLines = 8; // The # of lines we're willing to match against.
    RootInlineBox* line = endLine;
    for (int i = 0; i < numLines && line; i++, line = line->nextRootBox()) {
        if (line->lineBreakObj() == resolver.position().m_obj && line->lineBreakPos() == resolver.position().m_pos) {
            // We have a match.
            if (line->lineBreakBidiStatus() != resolver.status())
                return false; // ...but the bidi state doesn't match.
            RootInlineBox* result = line->nextRootBox();

            // Set our logical top to be the block height of endLine.
            if (result)
                endLogicalTop = line->blockLogicalHeight();

            int delta = logicalHeight() - endLogicalTop;
            if (delta && m_floatingObjects) {
                // See if any floats end in the range along which we want to shift the lines vertically.
                int logicalTop = min(logicalHeight(), endLogicalTop);

                RootInlineBox* lastLine = endLine;
                while (RootInlineBox* nextLine = lastLine->nextRootBox())
                    lastLine = nextLine;

                int logicalBottom = lastLine->blockLogicalHeight() + abs(delta);

                FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
                FloatingObjectSetIterator end = floatingObjectSet.end();
                for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                    FloatingObject* f = *it;
                    if (logicalBottomForFloat(f) >= logicalTop && logicalBottomForFloat(f) < logicalBottom)
                        return false;
                }
            }

            // Now delete the lines that we failed to sync.
            RootInlineBox* boxToDelete = endLine;
            RenderArena* arena = renderArena();
            while (boxToDelete && boxToDelete != result) {
                repaintLogicalTop = min(repaintLogicalTop, boxToDelete->logicalTopVisualOverflow());
                repaintLogicalBottom = max(repaintLogicalBottom, boxToDelete->logicalBottomVisualOverflow());
                RootInlineBox* next = boxToDelete->nextRootBox();
                boxToDelete->deleteLine(arena);
                boxToDelete = next;
            }

            endLine = result;
            return result;
        }
    }

    return false;
}

static inline bool skipNonBreakingSpace(const InlineIterator& it, const LineInfo& lineInfo)
{
    if (it.m_obj->style()->nbspMode() != SPACE || it.current() != noBreakSpace)
        return false;

    // FIXME: This is bad.  It makes nbsp inconsistent with space and won't work correctly
    // with m_minWidth/m_maxWidth.
    // Do not skip a non-breaking space if it is the first character
    // on a line after a clean line break (or on the first line, since previousLineBrokeCleanly starts off
    // |true|).
    if (lineInfo.isEmpty() && lineInfo.previousLineBrokeCleanly())
        return false;

    return true;
}

static inline bool shouldCollapseWhiteSpace(const RenderStyle* style, const LineInfo& lineInfo)
{
    return style->collapseWhiteSpace() || (style->whiteSpace() == PRE_WRAP && (!lineInfo.isEmpty() || !lineInfo.previousLineBrokeCleanly()));
}

static bool inlineFlowRequiresLineBox(RenderInline* flow)
{
    // FIXME: Right now, we only allow line boxes for inlines that are truly empty.
    // We need to fix this, though, because at the very least, inlines containing only
    // ignorable whitespace should should also have line boxes.
    return !flow->firstChild() && flow->hasInlineDirectionBordersPaddingOrMargin();
}

static bool requiresLineBox(const InlineIterator& it, const LineInfo& lineInfo = LineInfo())
{
    if (it.m_obj->isFloatingOrPositioned())
        return false;

    if (it.m_obj->isRenderInline() && !inlineFlowRequiresLineBox(toRenderInline(it.m_obj)))
        return false;

    if (!shouldCollapseWhiteSpace(it.m_obj->style(), lineInfo) || it.m_obj->isBR())
        return true;

    UChar current = it.current();
    return current != ' ' && current != '\t' && current != softHyphen && (current != '\n' || it.m_obj->preservesNewline())
    && !skipNonBreakingSpace(it, lineInfo);
}

bool RenderBlock::generatesLineBoxesForInlineChild(RenderObject* inlineObj)
{
    ASSERT(inlineObj->parent() == this);

    InlineIterator it(this, inlineObj, 0);
    while (!it.atEnd() && !requiresLineBox(it))
        it.increment();

    return !it.atEnd();
}

// FIXME: The entire concept of the skipTrailingWhitespace function is flawed, since we really need to be building
// line boxes even for containers that may ultimately collapse away. Otherwise we'll never get positioned
// elements quite right. In other words, we need to build this function's work into the normal line
// object iteration process.
// NB. this function will insert any floating elements that would otherwise
// be skipped but it will not position them.
void RenderBlock::LineBreaker::skipTrailingWhitespace(InlineIterator& iterator, const LineInfo& lineInfo)
{
    while (!iterator.atEnd() && !requiresLineBox(iterator, lineInfo)) {
        RenderObject* object = iterator.m_obj;
        if (object->isFloating()) {
            m_block->insertFloatingObject(toRenderBox(object));
        } else if (object->isPositioned())
            setStaticPositions(m_block, toRenderBox(object));
        iterator.increment();
    }
}

void RenderBlock::LineBreaker::skipLeadingWhitespace(InlineBidiResolver& resolver, const LineInfo& lineInfo,
                                                     FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    while (!resolver.position().atEnd() && !requiresLineBox(resolver.position(), lineInfo)) {
        RenderObject* object = resolver.position().m_obj;
        if (object->isFloating())
            m_block->positionNewFloatOnLine(m_block->insertFloatingObject(toRenderBox(object)), lastFloatFromPreviousLine, width);
        else if (object->isPositioned())
            setStaticPositions(m_block, toRenderBox(object));
        resolver.increment();
    }
    resolver.commitExplicitEmbedding();
}

// This is currently just used for list markers and inline flows that have line boxes. Neither should
// have an effect on whitespace at the start of the line.
static bool shouldSkipWhitespaceAfterStartObject(RenderBlock* block, RenderObject* o, LineMidpointState& lineMidpointState)
{
    RenderObject* next = bidiNext(block, o);
    if (next && !next->isBR() && next->isText() && toRenderText(next)->textLength() > 0) {
        RenderText* nextText = toRenderText(next);
        UChar nextChar = nextText->characters()[0];
        if (nextText->style()->isCollapsibleWhiteSpace(nextChar)) {
            addMidpoint(lineMidpointState, InlineIterator(0, o, 0));
            return true;
        }
    }

    return false;
}

static inline float textWidth(RenderText* text, unsigned from, unsigned len, const Font& font, float xPos, bool isFixedPitch, bool collapseWhiteSpace)
{
    if (isFixedPitch || (!from && len == text->textLength()) || text->style()->hasTextCombine())
        return text->width(from, len, font, xPos);
    return font.width(TextRun(text->characters() + from, len, !collapseWhiteSpace, xPos));
}

static void tryHyphenating(RenderText* text, const Font& font, const AtomicString& localeIdentifier, int minimumPrefixLength, int minimumSuffixLength, int lastSpace, int pos, float xPos, int availableWidth, bool isFixedPitch, bool collapseWhiteSpace, int lastSpaceWordSpacing, InlineIterator& lineBreak, int nextBreakable, bool& hyphenated)
{
    // Map 'hyphenate-limit-{before,after}: auto;' to 2.
    if (minimumPrefixLength < 0)
        minimumPrefixLength = 2;

    if (minimumSuffixLength < 0)
        minimumSuffixLength = 2;

    if (pos - lastSpace <= minimumSuffixLength)
        return;

    const AtomicString& hyphenString = text->style()->hyphenString();
    int hyphenWidth = font.width(TextRun(hyphenString.characters(), hyphenString.length()));

    float maxPrefixWidth = availableWidth - xPos - hyphenWidth - lastSpaceWordSpacing;
    // If the maximum width available for the prefix before the hyphen is small, then it is very unlikely
    // that an hyphenation opportunity exists, so do not bother to look for it.
    if (maxPrefixWidth <= font.pixelSize() * 5 / 4)
        return;

    unsigned prefixLength = font.offsetForPosition(TextRun(text->characters() + lastSpace, pos - lastSpace, !collapseWhiteSpace, xPos + lastSpaceWordSpacing), maxPrefixWidth, false);
    if (prefixLength < static_cast<unsigned>(minimumPrefixLength))
        return;

    prefixLength = lastHyphenLocation(text->characters() + lastSpace, pos - lastSpace, min(prefixLength, static_cast<unsigned>(pos - lastSpace - minimumSuffixLength)) + 1, localeIdentifier);
    // FIXME: The following assumes that the character at lastSpace is a space (and therefore should not factor
    // into hyphenate-limit-before) unless lastSpace is 0. This is wrong in the rare case of hyphenating
    // the first word in a text node which has leading whitespace.
    if (!prefixLength || prefixLength - (lastSpace ? 1 : 0) < static_cast<unsigned>(minimumPrefixLength))
        return;

    ASSERT(pos - lastSpace - prefixLength >= static_cast<unsigned>(minimumSuffixLength));

#if !ASSERT_DISABLED
    float prefixWidth = hyphenWidth + textWidth(text, lastSpace, prefixLength, font, xPos, isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
    ASSERT(xPos + prefixWidth <= availableWidth);
#else
    UNUSED_PARAM(isFixedPitch);
#endif

    lineBreak.moveTo(text, lastSpace + prefixLength, nextBreakable);
    hyphenated = true;
}

class LineWidth {
public:
    LineWidth(RenderBlock* block, bool isFirstLine)
        : m_block(block)
        , m_uncommittedWidth(0)
        , m_committedWidth(0)
        , m_overhangWidth(0)
        , m_left(0)
        , m_right(0)
        , m_availableWidth(0)
        , m_isFirstLine(isFirstLine)
    {
        ASSERT(block);
        updateAvailableWidth();
    }
    bool fitsOnLine() const { return currentWidth() <= m_availableWidth; }
    bool fitsOnLine(float extra) const { return currentWidth() + extra <= m_availableWidth; }
    float currentWidth() const { return m_committedWidth + m_uncommittedWidth; }

    // FIXME: We should eventually replace these three functions by ones that work on a higher abstraction.
    float uncommittedWidth() const { return m_uncommittedWidth; }
    float committedWidth() const { return m_committedWidth; }
    float availableWidth() const { return m_availableWidth; }

    void updateAvailableWidth();
    void shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject*);
    void addUncommittedWidth(float delta) { m_uncommittedWidth += delta; }
    void commit()
    {
        m_committedWidth += m_uncommittedWidth;
        m_uncommittedWidth = 0;
    }
    void applyOverhang(RenderRubyRun*, RenderObject* startRenderer, RenderObject* endRenderer);
    void fitBelowFloats();

private:
    void computeAvailableWidthFromLeftAndRight()
    {
        m_availableWidth = max(0, m_right - m_left) + m_overhangWidth;
    }

private:
    RenderBlock* m_block;
    float m_uncommittedWidth;
    float m_committedWidth;
    float m_overhangWidth; // The amount by which |m_availableWidth| has been inflated to account for possible contraction due to ruby overhang.
    int m_left;
    int m_right;
    float m_availableWidth;
    bool m_isFirstLine;
};

inline void LineWidth::updateAvailableWidth()
{
    int height = m_block->logicalHeight();
    m_left = m_block->logicalLeftOffsetForLine(height, m_isFirstLine);
    m_right = m_block->logicalRightOffsetForLine(height, m_isFirstLine);

    computeAvailableWidthFromLeftAndRight();
}

inline void LineWidth::shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject* newFloat)
{
    int height = m_block->logicalHeight();
    if (height < m_block->logicalTopForFloat(newFloat) || height >= m_block->logicalBottomForFloat(newFloat))
        return;

    if (newFloat->type() == RenderBlock::FloatingObject::FloatLeft)
        m_left = m_block->logicalRightForFloat(newFloat);
    else
        m_right = m_block->logicalLeftForFloat(newFloat);

    computeAvailableWidthFromLeftAndRight();
}

void LineWidth::applyOverhang(RenderRubyRun* rubyRun, RenderObject* startRenderer, RenderObject* endRenderer)
{
    int startOverhang;
    int endOverhang;
    rubyRun->getOverhang(m_isFirstLine, startRenderer, endRenderer, startOverhang, endOverhang);

    startOverhang = min<int>(startOverhang, m_committedWidth);
    m_availableWidth += startOverhang;

    endOverhang = max(min<int>(endOverhang, m_availableWidth - currentWidth()), 0);
    m_availableWidth += endOverhang;
    m_overhangWidth += startOverhang + endOverhang;
}

void LineWidth::fitBelowFloats()
{
    ASSERT(!m_committedWidth);
    ASSERT(!fitsOnLine());

    int floatLogicalBottom;
    int lastFloatLogicalBottom = m_block->logicalHeight();
    float newLineWidth = m_availableWidth;
    while (true) {
        floatLogicalBottom = m_block->nextFloatLogicalBottomBelow(lastFloatLogicalBottom);
        if (!floatLogicalBottom)
            break;

        newLineWidth = m_block->availableLogicalWidthForLine(floatLogicalBottom, m_isFirstLine);
        lastFloatLogicalBottom = floatLogicalBottom;
        if (newLineWidth >= m_uncommittedWidth)
            break;
    }

    if (newLineWidth > m_availableWidth) {
        m_block->setLogicalHeight(lastFloatLogicalBottom);
        m_availableWidth = newLineWidth + m_overhangWidth;
    }
}

class TrailingObjects {
public:
    TrailingObjects();
    void setTrailingWhitespace(RenderText*);
    void clear();
    void appendBoxIfNeeded(RenderBox*);

    enum CollapseFirstSpaceOrNot { DoNotCollapseFirstSpace, CollapseFirstSpace };

    void updateMidpointsForTrailingBoxes(LineMidpointState&, const InlineIterator& lBreak, CollapseFirstSpaceOrNot);

private:
    RenderText* m_whitespace;
    Vector<RenderBox*, 4> m_boxes;
};

TrailingObjects::TrailingObjects()
    : m_whitespace(0)
{
}

inline void TrailingObjects::setTrailingWhitespace(RenderText* whitespace)
{
    ASSERT(whitespace);
    m_whitespace = whitespace;
}

inline void TrailingObjects::clear()
{
    m_whitespace = 0;
    m_boxes.clear();
}

inline void TrailingObjects::appendBoxIfNeeded(RenderBox* box)
{
    if (m_whitespace)
        m_boxes.append(box);
}

void TrailingObjects::updateMidpointsForTrailingBoxes(LineMidpointState& lineMidpointState, const InlineIterator& lBreak, CollapseFirstSpaceOrNot collapseFirstSpace)
{
    if (!m_whitespace)
        return;

    // This object is either going to be part of the last midpoint, or it is going to be the actual endpoint.
    // In both cases we just decrease our pos by 1 level to exclude the space, allowing it to - in effect - collapse into the newline.
    if (lineMidpointState.numMidpoints % 2) {
        // Find the trailing space object's midpoint.
        int trailingSpaceMidpoint = lineMidpointState.numMidpoints - 1;
        for ( ; trailingSpaceMidpoint > 0 && lineMidpointState.midpoints[trailingSpaceMidpoint].m_obj != m_whitespace; --trailingSpaceMidpoint) { }
        ASSERT(trailingSpaceMidpoint >= 0);
        if (collapseFirstSpace == CollapseFirstSpace)
            lineMidpointState.midpoints[trailingSpaceMidpoint].m_pos--;

        // Now make sure every single trailingPositionedBox following the trailingSpaceMidpoint properly stops and starts
        // ignoring spaces.
        size_t currentMidpoint = trailingSpaceMidpoint + 1;
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            if (currentMidpoint >= lineMidpointState.numMidpoints) {
                // We don't have a midpoint for this box yet.
                InlineIterator ignoreStart(0, m_boxes[i], 0);
                addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring.
                addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
            } else {
                ASSERT(lineMidpointState.midpoints[currentMidpoint].m_obj == m_boxes[i]);
                ASSERT(lineMidpointState.midpoints[currentMidpoint + 1].m_obj == m_boxes[i]);
            }
            currentMidpoint += 2;
        }
    } else if (!lBreak.m_obj) {
        ASSERT(m_whitespace->isText());
        ASSERT(collapseFirstSpace == CollapseFirstSpace);
        // Add a new end midpoint that stops right at the very end.
        unsigned length = m_whitespace->textLength();
        unsigned pos = length >= 2 ? length - 2 : UINT_MAX;
        InlineIterator endMid(0, m_whitespace, pos);
        addMidpoint(lineMidpointState, endMid);
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            InlineIterator ignoreStart(0, m_boxes[i], 0);
            addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
            addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
        }
    }
}

void RenderBlock::LineBreaker::reset()
{
    m_positionedObjects.clear();
    m_hyphenated = false;
    m_clear = CNONE;
}

InlineIterator RenderBlock::LineBreaker::nextLineBreak(InlineBidiResolver& resolver, LineInfo& lineInfo,
    LineBreakIteratorInfo& lineBreakIteratorInfo, FloatingObject* lastFloatFromPreviousLine)
{
    reset();

    ASSERT(resolver.position().root() == m_block);

    bool appliedStartWidth = resolver.position().m_pos > 0;
    LineMidpointState& lineMidpointState = resolver.midpointState();

    LineWidth width(m_block, lineInfo.isFirstLine());

    skipLeadingWhitespace(resolver, lineInfo, lastFloatFromPreviousLine, width);

    if (resolver.position().atEnd())
        return resolver.position();

    // This variable is used only if whitespace isn't set to PRE, and it tells us whether
    // or not we are currently ignoring whitespace.
    bool ignoringSpaces = false;
    InlineIterator ignoreStart;

    // This variable tracks whether the very last character we saw was a space.  We use
    // this to detect when we encounter a second space so we know we have to terminate
    // a run.
    bool currentCharacterIsSpace = false;
    bool currentCharacterIsWS = false;
    TrailingObjects trailingObjects;

    InlineIterator lBreak = resolver.position();

    // FIXME: It is error-prone to split the position object out like this.
    // Teach this code to work with objects instead of this split tuple.
    InlineIterator current = resolver.position();
    RenderObject* last = current.m_obj;
    bool atStart = true;

    bool startingNewParagraph = lineInfo.previousLineBrokeCleanly();
    lineInfo.setPreviousLineBrokeCleanly(false);

    bool autoWrapWasEverTrueOnLine = false;
    bool floatsFitOnLine = true;

    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific circumstances (in order to match common WinIE renderings).
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.)
    bool allowImagesToBreak = !m_block->document()->inQuirksMode() || !m_block->isTableCell() || !m_block->style()->logicalWidth().isIntrinsicOrAuto();

    EWhiteSpace currWS = m_block->style()->whiteSpace();
    EWhiteSpace lastWS = currWS;
    while (current.m_obj) {
        RenderObject* next = bidiNext(m_block, current.m_obj);

        currWS = current.m_obj->isReplaced() ? current.m_obj->parent()->style()->whiteSpace() : current.m_obj->style()->whiteSpace();
        lastWS = last->isReplaced() ? last->parent()->style()->whiteSpace() : last->style()->whiteSpace();

        bool autoWrap = RenderStyle::autoWrap(currWS);
        autoWrapWasEverTrueOnLine = autoWrapWasEverTrueOnLine || autoWrap;

#if ENABLE(SVG)
        bool preserveNewline = current.m_obj->isSVGInlineText() ? false : RenderStyle::preserveNewline(currWS);
#else
        bool preserveNewline = RenderStyle::preserveNewline(currWS);
#endif

        bool collapseWhiteSpace = RenderStyle::collapseWhiteSpace(currWS);

        if (current.m_obj->isBR()) {
            if (width.fitsOnLine()) {
                lBreak.moveToStartOf(current.m_obj);
                lBreak.increment();

                // A <br> always breaks a line, so don't let the line be collapsed
                // away. Also, the space at the end of a line with a <br> does not
                // get collapsed away.  It only does this if the previous line broke
                // cleanly.  Otherwise the <br> has no effect on whether the line is
                // empty or not.
                if (startingNewParagraph)
                    lineInfo.setEmpty(false);
                trailingObjects.clear();
                lineInfo.setPreviousLineBrokeCleanly(true);

                if (!lineInfo.isEmpty())
                    m_clear = current.m_obj->style()->clear();
            }
            goto end;
        }

        if (current.m_obj->isFloating()) {
            RenderBox* floatBox = toRenderBox(current.m_obj);
            FloatingObject* f = m_block->insertFloatingObject(floatBox);
            // check if it fits in the current line.
            // If it does, position it now, otherwise, position
            // it after moving to next line (in newLine() func)
            if (floatsFitOnLine && width.fitsOnLine(m_block->logicalWidthForFloat(f))) {
                m_block->positionNewFloatOnLine(f, lastFloatFromPreviousLine, width);
                if (lBreak.m_obj == current.m_obj) {
                    ASSERT(!lBreak.m_pos);
                    lBreak.increment();
                }
            } else
                floatsFitOnLine = false;
        } else if (current.m_obj->isPositioned()) {
            // If our original display wasn't an inline type, then we can
            // go ahead and determine our static inline position now.
            RenderBox* box = toRenderBox(current.m_obj);
            bool isInlineType = box->style()->isOriginalDisplayInlineType();
            if (!isInlineType)
                box->layer()->setStaticInlinePosition(m_block->borderAndPaddingStart());
            else  {
                // If our original display was an INLINE type, then we can go ahead
                // and determine our static y position now.
                box->layer()->setStaticBlockPosition(m_block->logicalHeight());
            }

            // If we're ignoring spaces, we have to stop and include this object and
            // then start ignoring spaces again.
            if (isInlineType || current.m_obj->container()->isRenderInline()) {
                if (ignoringSpaces) {
                    ignoreStart.m_obj = current.m_obj;
                    ignoreStart.m_pos = 0;
                    addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
                    addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
                }
                trailingObjects.appendBoxIfNeeded(box);
            } else
                m_positionedObjects.append(box);
        } else if (current.m_obj->isRenderInline()) {
            // Right now, we should only encounter empty inlines here.
            ASSERT(!current.m_obj->firstChild());

            RenderInline* flowBox = toRenderInline(current.m_obj);

            // Now that some inline flows have line boxes, if we are already ignoring spaces, we need
            // to make sure that we stop to include this object and then start ignoring spaces again.
            // If this object is at the start of the line, we need to behave like list markers and
            // start ignoring spaces.
            if (inlineFlowRequiresLineBox(flowBox)) {
                lineInfo.setEmpty(false);
                if (ignoringSpaces) {
                    trailingObjects.clear();
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0)); // Stop ignoring spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0)); // Start ignoring again.
                } else if (m_block->style()->collapseWhiteSpace() && resolver.position().m_obj == current.m_obj
                    && shouldSkipWhitespaceAfterStartObject(m_block, current.m_obj, lineMidpointState)) {
                    // Like with list markers, we start ignoring spaces to make sure that any
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
            }

            width.addUncommittedWidth(borderPaddingMarginStart(flowBox) + borderPaddingMarginEnd(flowBox));
        } else if (current.m_obj->isReplaced()) {
            RenderBox* replacedBox = toRenderBox(current.m_obj);

            // Break on replaced elements if either has normal white-space.
            if ((autoWrap || RenderStyle::autoWrap(lastWS)) && (!current.m_obj->isImage() || allowImagesToBreak)) {
                width.commit();
                lBreak.moveToStartOf(current.m_obj);
            }

            if (ignoringSpaces)
                addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, 0));

            lineInfo.setEmpty(false);
            ignoringSpaces = false;
            currentCharacterIsSpace = false;
            currentCharacterIsWS = false;
            trailingObjects.clear();

            // Optimize for a common case. If we can't find whitespace after the list
            // item, then this is all moot.
            int replacedLogicalWidth = m_block->logicalWidthForChild(replacedBox) + m_block->marginStartForChild(replacedBox) + m_block->marginEndForChild(replacedBox) + inlineLogicalWidth(current.m_obj);
            if (current.m_obj->isListMarker()) {
                if (m_block->style()->collapseWhiteSpace() && shouldSkipWhitespaceAfterStartObject(m_block, current.m_obj, lineMidpointState)) {
                    // Like with inline flows, we start ignoring spaces to make sure that any
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
                if (toRenderListMarker(current.m_obj)->isInside())
                    width.addUncommittedWidth(replacedLogicalWidth);
            } else
                width.addUncommittedWidth(replacedLogicalWidth);
            if (current.m_obj->isRubyRun())
                width.applyOverhang(toRenderRubyRun(current.m_obj), last, next);
        } else if (current.m_obj->isText()) {
            if (!current.m_pos)
                appliedStartWidth = false;

            RenderText* t = toRenderText(current.m_obj);

#if ENABLE(SVG)
            bool isSVGText = t->isSVGInlineText();
#endif

            RenderStyle* style = t->style(lineInfo.isFirstLine());
            if (style->hasTextCombine() && current.m_obj->isCombineText())
                toRenderCombineText(current.m_obj)->combineText();

            const Font& f = style->font();
            bool isFixedPitch = f.isFixedPitch();
            bool canHyphenate = style->hyphens() == HyphensAuto && WebCore::canHyphenate(style->locale());

            int lastSpace = current.m_pos;
            float wordSpacing = current.m_obj->style()->wordSpacing();
            float lastSpaceWordSpacing = 0;

            // Non-zero only when kerning is enabled, in which case we measure words with their trailing
            // space, then subtract its width.
            float wordTrailingSpaceWidth = f.typesettingFeatures() & Kerning ? f.width(TextRun(&space, 1)) + wordSpacing : 0;

            float wrapW = width.uncommittedWidth() + inlineLogicalWidth(current.m_obj, !appliedStartWidth, true);
            float charWidth = 0;
            bool breakNBSP = autoWrap && current.m_obj->style()->nbspMode() == SPACE;
            // Auto-wrapping text should wrap in the middle of a word only if it could not wrap before the word,
            // which is only possible if the word is the first thing on the line, that is, if |w| is zero.
            bool breakWords = current.m_obj->style()->breakWords() && ((autoWrap && !width.committedWidth()) || currWS == PRE);
            bool midWordBreak = false;
            bool breakAll = current.m_obj->style()->wordBreak() == BreakAllWordBreak && autoWrap;
            float hyphenWidth = 0;

            if (t->isWordBreak()) {
                width.commit();
                lBreak.moveToStartOf(current.m_obj);
                ASSERT(current.m_pos == t->textLength());
            }

            for (; current.m_pos < t->textLength(); current.fastIncrementInTextNode()) {
                bool previousCharacterIsSpace = currentCharacterIsSpace;
                bool previousCharacterIsWS = currentCharacterIsWS;
                UChar c = current.current();
                currentCharacterIsSpace = c == ' ' || c == '\t' || (!preserveNewline && (c == '\n'));

                if (!collapseWhiteSpace || !currentCharacterIsSpace)
                    lineInfo.setEmpty(false);

                if (c == softHyphen && autoWrap && !hyphenWidth && style->hyphens() != HyphensNone) {
                    const AtomicString& hyphenString = style->hyphenString();
                    hyphenWidth = f.width(TextRun(hyphenString.characters(), hyphenString.length()));
                    width.addUncommittedWidth(hyphenWidth);
                }

                bool applyWordSpacing = false;

                currentCharacterIsWS = currentCharacterIsSpace || (breakNBSP && c == noBreakSpace);

                if ((breakAll || breakWords) && !midWordBreak) {
                    wrapW += charWidth;
                    charWidth = textWidth(t, current.m_pos, 1, f, width.committedWidth() + wrapW, isFixedPitch, collapseWhiteSpace);
                    midWordBreak = width.committedWidth() + wrapW + charWidth > width.availableWidth();
                }

                if (lineBreakIteratorInfo.first != t) {
                    lineBreakIteratorInfo.first = t;
                    lineBreakIteratorInfo.second.reset(t->characters(), t->textLength());
                }

                bool betweenWords = c == '\n' || (currWS != PRE && !atStart && isBreakable(lineBreakIteratorInfo.second, current.m_pos, current.m_nextBreakablePosition, breakNBSP)
                    && (style->hyphens() != HyphensNone || (current.previousInSameNode() != softHyphen)));

                if (betweenWords || midWordBreak) {
                    bool stoppedIgnoringSpaces = false;
                    if (ignoringSpaces) {
                        if (!currentCharacterIsSpace) {
                            // Stop ignoring spaces and begin at this
                            // new point.
                            ignoringSpaces = false;
                            lastSpaceWordSpacing = 0;
                            lastSpace = current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                            stoppedIgnoringSpaces = true;
                        } else {
                            // Just keep ignoring these spaces.
                            continue;
                        }
                    }

                    float additionalTmpW;
                    if (wordTrailingSpaceWidth && currentCharacterIsSpace)
                        additionalTmpW = textWidth(t, lastSpace, current.m_pos + 1 - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) - wordTrailingSpaceWidth + lastSpaceWordSpacing;
                    else
                        additionalTmpW = textWidth(t, lastSpace, current.m_pos - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
                    width.addUncommittedWidth(additionalTmpW);
                    if (!appliedStartWidth) {
                        width.addUncommittedWidth(inlineLogicalWidth(current.m_obj, true, false));
                        appliedStartWidth = true;
                    }

                    applyWordSpacing =  wordSpacing && currentCharacterIsSpace && !previousCharacterIsSpace;

                    if (!width.committedWidth() && autoWrap && !width.fitsOnLine())
                        width.fitBelowFloats();

                    if (autoWrap || breakWords) {
                        // If we break only after white-space, consider the current character
                        // as candidate width for this line.
                        bool lineWasTooWide = false;
                        if (width.fitsOnLine() && currentCharacterIsWS && current.m_obj->style()->breakOnlyAfterWhiteSpace() && !midWordBreak) {
                            float charWidth = textWidth(t, current.m_pos, 1, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + (applyWordSpacing ? wordSpacing : 0);
                            // Check if line is too big even without the extra space
                            // at the end of the line. If it is not, do nothing.
                            // If the line needs the extra whitespace to be too long,
                            // then move the line break to the space and skip all
                            // additional whitespace.
                            if (!width.fitsOnLine(charWidth)) {
                                lineWasTooWide = true;
                                lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                                skipTrailingWhitespace(lBreak, lineInfo);
                            }
                        }
                        if (lineWasTooWide || !width.fitsOnLine()) {
                            if (canHyphenate && !width.fitsOnLine()) {
                                tryHyphenating(t, f, style->locale(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, current.m_pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, current.m_nextBreakablePosition, m_hyphenated);
                                if (m_hyphenated)
                                    goto end;
                            }
                            if (lBreak.atTextParagraphSeparator()) {
                                if (!stoppedIgnoringSpaces && current.m_pos > 0) {
                                    // We need to stop right before the newline and then start up again.
                                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1)); // Stop
                                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos)); // Start
                                }
                                lBreak.increment();
                                lineInfo.setPreviousLineBrokeCleanly(true);
                            }
                            if (lBreak.m_obj && lBreak.m_pos && lBreak.m_obj->isText() && toRenderText(lBreak.m_obj)->textLength() && toRenderText(lBreak.m_obj)->characters()[lBreak.m_pos - 1] == softHyphen && style->hyphens() != HyphensNone)
                                m_hyphenated = true;
                            goto end; // Didn't fit. Jump to the end.
                        } else {
                            if (!betweenWords || (midWordBreak && !autoWrap))
                                width.addUncommittedWidth(-additionalTmpW);
                            if (hyphenWidth) {
                                // Subtract the width of the soft hyphen out since we fit on a line.
                                width.addUncommittedWidth(-hyphenWidth);
                                hyphenWidth = 0;
                            }
                        }
                    }

                    if (c == '\n' && preserveNewline) {
                        if (!stoppedIgnoringSpaces && current.m_pos > 0) {
                            // We need to stop right before the newline and then start up again.
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1)); // Stop
                            addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos)); // Start
                        }
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        lBreak.increment();
                        lineInfo.setPreviousLineBrokeCleanly(true);
                        return lBreak;
                    }

                    if (autoWrap && betweenWords) {
                        width.commit();
                        wrapW = 0;
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        // Auto-wrapping text should not wrap in the middle of a word once it has had an
                        // opportunity to break after a word.
                        breakWords = false;
                    }

                    if (midWordBreak) {
                        // Remember this as a breakable position in case
                        // adding the end width forces a break.
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                        midWordBreak &= (breakWords || breakAll);
                    }

                    if (betweenWords) {
                        lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                        lastSpace = current.m_pos;
                    }

                    if (!ignoringSpaces && current.m_obj->style()->collapseWhiteSpace()) {
                        // If we encounter a newline, or if we encounter a
                        // second space, we need to go ahead and break up this
                        // run and enter a mode where we start collapsing spaces.
                        if (currentCharacterIsSpace && previousCharacterIsSpace) {
                            ignoringSpaces = true;

                            // We just entered a mode where we are ignoring
                            // spaces. Create a midpoint to terminate the run
                            // before the second space.
                            addMidpoint(lineMidpointState, ignoreStart);
                            trailingObjects.updateMidpointsForTrailingBoxes(lineMidpointState, InlineIterator(), TrailingObjects::DoNotCollapseFirstSpace);
                        }
                    }
                } else if (ignoringSpaces) {
                    // Stop ignoring spaces and begin at this
                    // new point.
                    ignoringSpaces = false;
                    lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                    lastSpace = current.m_pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                }

#if ENABLE(SVG)
                if (isSVGText && current.m_pos > 0) {
                    // Force creation of new InlineBoxes for each absolute positioned character (those that start new text chunks).
                    if (static_cast<RenderSVGInlineText*>(t)->characterStartsNewTextChunk(current.m_pos)) {
                        addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos - 1));
                        addMidpoint(lineMidpointState, InlineIterator(0, current.m_obj, current.m_pos));
                    }
                }
#endif

                if (currentCharacterIsSpace && !previousCharacterIsSpace) {
                    ignoreStart.m_obj = current.m_obj;
                    ignoreStart.m_pos = current.m_pos;
                }

                if (!currentCharacterIsWS && previousCharacterIsWS) {
                    if (autoWrap && current.m_obj->style()->breakOnlyAfterWhiteSpace())
                        lBreak.moveTo(current.m_obj, current.m_pos, current.m_nextBreakablePosition);
                }

                if (collapseWhiteSpace && currentCharacterIsSpace && !ignoringSpaces)
                    trailingObjects.setTrailingWhitespace(static_cast<RenderText*>(current.m_obj));
                else if (!current.m_obj->style()->collapseWhiteSpace() || !currentCharacterIsSpace)
                    trailingObjects.clear();

                atStart = false;
            }

            // IMPORTANT: current.m_pos is > length here!
            float additionalTmpW = ignoringSpaces ? 0 : textWidth(t, lastSpace, current.m_pos - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
            width.addUncommittedWidth(additionalTmpW + inlineLogicalWidth(current.m_obj, !appliedStartWidth, true));

            if (!width.fitsOnLine()) {
                if (canHyphenate)
                    tryHyphenating(t, f, style->locale(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, current.m_pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, current.m_nextBreakablePosition, m_hyphenated);

                if (!m_hyphenated && lBreak.previousInSameNode() == softHyphen && style->hyphens() != HyphensNone)
                    m_hyphenated = true;

                if (m_hyphenated)
                    goto end;
            }
        } else
            ASSERT_NOT_REACHED();

        bool checkForBreak = autoWrap;
        if (width.committedWidth() && !width.fitsOnLine() && lBreak.m_obj && currWS == NOWRAP)
            checkForBreak = true;
        else if (next && current.m_obj->isText() && next->isText() && !next->isBR() && (autoWrap || (next->style()->autoWrap()))) {
            if (currentCharacterIsSpace)
                checkForBreak = true;
            else {
                RenderText* nextText = toRenderText(next);
                if (nextText->textLength()) {
                    UChar c = nextText->characters()[0];
                    checkForBreak = (c == ' ' || c == '\t' || (c == '\n' && !next->preservesNewline()));
                    // If the next item on the line is text, and if we did not end with
                    // a space, then the next text run continues our word (and so it needs to
                    // keep adding to |tmpW|. Just update and continue.
                } else if (nextText->isWordBreak())
                    checkForBreak = true;

                if (!width.fitsOnLine() && !width.committedWidth())
                    width.fitBelowFloats();

                bool canPlaceOnLine = width.fitsOnLine() || !autoWrapWasEverTrueOnLine;
                if (canPlaceOnLine && checkForBreak) {
                    width.commit();
                    lBreak.moveToStartOf(next);
                }
            }
        }

        if (checkForBreak && !width.fitsOnLine()) {
            // if we have floats, try to get below them.
            if (currentCharacterIsSpace && !ignoringSpaces && current.m_obj->style()->collapseWhiteSpace())
                trailingObjects.clear();

            if (width.committedWidth())
                goto end;

            width.fitBelowFloats();

            // |width| may have been adjusted because we got shoved down past a float (thus
            // giving us more room), so we need to retest, and only jump to
            // the end label if we still don't fit on the line. -dwh
            if (!width.fitsOnLine())
                goto end;
        }

        if (!current.m_obj->isFloatingOrPositioned()) {
            last = current.m_obj;
            if (last->isReplaced() && autoWrap && (!last->isImage() || allowImagesToBreak) && (!last->isListMarker() || toRenderListMarker(last)->isInside())) {
                width.commit();
                lBreak.moveToStartOf(next);
            }
        }

        // Clear out our character space bool, since inline <pre>s don't collapse whitespace
        // with adjacent inline normal/nowrap spans.
        if (!collapseWhiteSpace)
            currentCharacterIsSpace = false;

        current.moveToStartOf(next);
        atStart = false;
    }

    if (width.fitsOnLine() || lastWS == NOWRAP)
        lBreak.clear();

 end:
    if (lBreak == resolver.position() && (!lBreak.m_obj || !lBreak.m_obj->isBR())) {
        // we just add as much as possible
        if (m_block->style()->whiteSpace() == PRE) {
            // FIXME: Don't really understand this case.
            if (current.m_pos) {
                // FIXME: This should call moveTo which would clear m_nextBreakablePosition
                // this code as-is is likely wrong.
                lBreak.m_obj = current.m_obj;
                lBreak.m_pos = current.m_pos - 1;
            } else
                lBreak.moveTo(last, last->isText() ? last->length() : 0);
        } else if (lBreak.m_obj) {
            // Don't ever break in the middle of a word if we can help it.
            // There's no room at all. We just have to be on this line,
            // even though we'll spill out.
            lBreak.moveTo(current.m_obj, current.m_pos);
        }
    }

    // make sure we consume at least one char/object.
    if (lBreak == resolver.position())
        lBreak.increment();

    // Sanity check our midpoints.
    checkMidpoints(lineMidpointState, lBreak);

    trailingObjects.updateMidpointsForTrailingBoxes(lineMidpointState, lBreak, TrailingObjects::CollapseFirstSpace);

    // We might have made lBreak an iterator that points past the end
    // of the object. Do this adjustment to make it point to the start
    // of the next object instead to avoid confusing the rest of the
    // code.
    if (lBreak.m_pos > 0) {
        lBreak.m_pos--;
        lBreak.increment();
    }

    return lBreak;
}

void RenderBlock::addOverflowFromInlineChildren()
{
    int endPadding = hasOverflowClip() ? paddingEnd() : 0;
    // FIXME: Need to find another way to do this, since scrollbars could show when we don't want them to.
    if (hasOverflowClip() && !endPadding && node() && node()->rendererIsEditable() && node() == node()->rootEditableElement() && style()->isLeftToRightDirection())
        endPadding = 1;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        addLayoutOverflow(curr->paddedLayoutOverflowRect(endPadding));
        if (!hasOverflowClip())
            addVisualOverflow(curr->visualOverflowRect(curr->lineTop(), curr->lineBottom()));
    }
}

void RenderBlock::deleteEllipsisLineBoxes()
{
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox())
        curr->clearTruncation();
}

void RenderBlock::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if horizontal ellipsis is "not renderable"
    TextRun ellipsisRun(&horizontalEllipsis, 1);
    DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
    const Font& firstLineFont = firstLineStyle()->font();
    const Font& font = style()->font();
    int firstLineEllipsisWidth = firstLineFont.width(ellipsisRun);
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.width(ellipsisRun);

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style()->isLeftToRightDirection();
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        int blockRightEdge = logicalRightOffsetForLine(curr->y(), curr == firstRootBox());
        int blockLeftEdge = logicalLeftOffsetForLine(curr->y(), curr == firstRootBox());
        int lineBoxEdge = ltr ? curr->x() + curr->logicalWidth() : curr->x();
        if ((ltr && lineBoxEdge > blockRightEdge) || (!ltr && lineBoxEdge < blockLeftEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.
            int width = curr == firstRootBox() ? firstLineEllipsisWidth : ellipsisWidth;
            int blockEdge = ltr ? blockRightEdge : blockLeftEdge;
            if (curr->lineCanAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width))
                curr->placeEllipsis(ellipsisStr, ltr, blockLeftEdge, blockRightEdge, width);
        }
    }
}

bool RenderBlock::positionNewFloatOnLine(FloatingObject* newFloat, FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    if (!positionNewFloats())
        return false;

    width.shrinkAvailableWidthForNewFloatIfNeeded(newFloat);

    if (!newFloat->m_paginationStrut)
        return true;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    ASSERT(floatingObjectSet.last() == newFloat);

    int floatLogicalTop = logicalTopForFloat(newFloat);
    int paginationStrut = newFloat->m_paginationStrut;

    if (floatLogicalTop - paginationStrut != logicalHeight())
        return true;

    FloatingObjectSetIterator it = floatingObjectSet.end();
    --it; // Last float is newFloat, skip that one.
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    while (it != begin) {
        --it;
        FloatingObject* f = *it;
        if (f == lastFloatFromPreviousLine)
            break;
        if (logicalTopForFloat(f) == logicalHeight()) {
            ASSERT(!f->m_paginationStrut);
            f->m_paginationStrut = paginationStrut;
            RenderBox* o = f->m_renderer;
            setLogicalTopForChild(o, logicalTopForChild(o) + marginBeforeForChild(o) + paginationStrut);
            if (o->isRenderBlock())
                toRenderBlock(o)->setChildNeedsLayout(true, false);
            o->layoutIfNeeded();
            setLogicalTopForFloat(f, logicalTopForFloat(f) + f->m_paginationStrut);
        }
    }

    setLogicalHeight(logicalHeight() + paginationStrut);
    width.updateAvailableWidth();

    return true;
}

}
