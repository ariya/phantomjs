/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
#include "RenderBlock.h"

#include "AXObjectCache.h"
#include "ColumnInfo.h"
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HitTestLocation.h"
#include "HitTestResult.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "LayoutRepainter.h"
#include "LogicalSelectionOffsetCaches.h"
#include "OverflowEvent.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderBoxRegionInfo.h"
#include "RenderCombineText.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderFlexibleBox.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderMarquee.h"
#include "RenderNamedFlowThread.h"
#include "RenderRegion.h"
#include "RenderTableCell.h"
#include "RenderTextFragment.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "SVGTextRunRenderingContext.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "TransformState.h"
#include <wtf/StackStats.h>
#include <wtf/TemporaryChange.h>

#if ENABLE(CSS_SHAPES)
#include "ShapeInsideInfo.h"
#include "ShapeOutsideInfo.h"
#endif

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

using namespace HTMLNames;

struct SameSizeAsRenderBlock : public RenderBox {
    void* pointers[2];
    RenderObjectChildList children;
    RenderLineBoxList lineBoxes;
    uint32_t bitfields;
};

COMPILE_ASSERT(sizeof(RenderBlock) == sizeof(SameSizeAsRenderBlock), RenderBlock_should_stay_small);

struct SameSizeAsFloatingObject {
    void* pointers[2];
    LayoutRect rect;
    int paginationStrut;
    uint32_t bitfields : 8;
};

COMPILE_ASSERT(sizeof(RenderBlock::MarginValues) == sizeof(LayoutUnit[4]), MarginValues_should_stay_small);

struct SameSizeAsMarginInfo {
    uint32_t bitfields : 16;
    LayoutUnit margins[2];
};

typedef WTF::HashMap<const RenderBox*, OwnPtr<ColumnInfo> > ColumnInfoMap;
static ColumnInfoMap* gColumnInfoMap = 0;

static TrackedDescendantsMap* gPositionedDescendantsMap = 0;
static TrackedDescendantsMap* gPercentHeightDescendantsMap = 0;

static TrackedContainerMap* gPositionedContainerMap = 0;
static TrackedContainerMap* gPercentHeightContainerMap = 0;
    
typedef WTF::HashMap<RenderBlock*, OwnPtr<ListHashSet<RenderInline*> > > ContinuationOutlineTableMap;

typedef WTF::HashSet<RenderBlock*> DelayedUpdateScrollInfoSet;
static int gDelayUpdateScrollInfo = 0;
static DelayedUpdateScrollInfoSet* gDelayedUpdateScrollInfoSet = 0;

static bool gColumnFlowSplitEnabled = true;

bool RenderBlock::s_canPropagateFloatIntoSibling = false;

// This class helps dispatching the 'overflow' event on layout change. overflow can be set on RenderBoxes, yet the existing code
// only works on RenderBlocks. If this change, this class should be shared with other RenderBoxes.
class OverflowEventDispatcher {
    WTF_MAKE_NONCOPYABLE(OverflowEventDispatcher);
public:
    OverflowEventDispatcher(const RenderBlock* block)
        : m_block(block)
        , m_hadHorizontalLayoutOverflow(false)
        , m_hadVerticalLayoutOverflow(false)
    {
        m_shouldDispatchEvent = !m_block->isAnonymous() && m_block->hasOverflowClip() && m_block->document()->hasListenerType(Document::OVERFLOWCHANGED_LISTENER);
        if (m_shouldDispatchEvent) {
            m_hadHorizontalLayoutOverflow = m_block->hasHorizontalLayoutOverflow();
            m_hadVerticalLayoutOverflow = m_block->hasVerticalLayoutOverflow();
        }
    }

    ~OverflowEventDispatcher()
    {
        if (!m_shouldDispatchEvent)
            return;

        bool hasHorizontalLayoutOverflow = m_block->hasHorizontalLayoutOverflow();
        bool hasVerticalLayoutOverflow = m_block->hasVerticalLayoutOverflow();

        bool horizontalLayoutOverflowChanged = hasHorizontalLayoutOverflow != m_hadHorizontalLayoutOverflow;
        bool verticalLayoutOverflowChanged = hasVerticalLayoutOverflow != m_hadVerticalLayoutOverflow;
        if (horizontalLayoutOverflowChanged || verticalLayoutOverflowChanged) {
            if (FrameView* frameView = m_block->document()->view())
                frameView->scheduleEvent(OverflowEvent::create(horizontalLayoutOverflowChanged, hasHorizontalLayoutOverflow, verticalLayoutOverflowChanged, hasVerticalLayoutOverflow), m_block->node());
        }
    }

private:
    const RenderBlock* m_block;
    bool m_shouldDispatchEvent;
    bool m_hadHorizontalLayoutOverflow;
    bool m_hadVerticalLayoutOverflow;
};

// Our MarginInfo state used when laying out block children.
RenderBlock::MarginInfo::MarginInfo(RenderBlock* block, LayoutUnit beforeBorderPadding, LayoutUnit afterBorderPadding)
    : m_atBeforeSideOfBlock(true)
    , m_atAfterSideOfBlock(false)
    , m_hasMarginBeforeQuirk(false)
    , m_hasMarginAfterQuirk(false)
    , m_determinedMarginBeforeQuirk(false)
    , m_discardMargin(false)
{
    RenderStyle* blockStyle = block->style();
    ASSERT(block->isRenderView() || block->parent());
    m_canCollapseWithChildren = !block->isRenderView() && !block->isRoot() && !block->isOutOfFlowPositioned()
        && !block->isFloating() && !block->isTableCell() && !block->hasOverflowClip() && !block->isInlineBlockOrInlineTable()
        && !block->isRenderFlowThread() && !block->isWritingModeRoot() && !block->parent()->isFlexibleBox()
        && blockStyle->hasAutoColumnCount() && blockStyle->hasAutoColumnWidth() && !blockStyle->columnSpan();

    m_canCollapseMarginBeforeWithChildren = m_canCollapseWithChildren && !beforeBorderPadding && blockStyle->marginBeforeCollapse() != MSEPARATE;

    // If any height other than auto is specified in CSS, then we don't collapse our bottom
    // margins with our children's margins.  To do otherwise would be to risk odd visual
    // effects when the children overflow out of the parent block and yet still collapse
    // with it.  We also don't collapse if we have any bottom border/padding.
    m_canCollapseMarginAfterWithChildren = m_canCollapseWithChildren && (afterBorderPadding == 0) &&
        (blockStyle->logicalHeight().isAuto() && !blockStyle->logicalHeight().value()) && blockStyle->marginAfterCollapse() != MSEPARATE;
    
    m_quirkContainer = block->isTableCell() || block->isBody();

    m_discardMargin = m_canCollapseMarginBeforeWithChildren && block->mustDiscardMarginBefore();

    m_positiveMargin = (m_canCollapseMarginBeforeWithChildren && !block->mustDiscardMarginBefore()) ? block->maxPositiveMarginBefore() : LayoutUnit();
    m_negativeMargin = (m_canCollapseMarginBeforeWithChildren && !block->mustDiscardMarginBefore()) ? block->maxNegativeMarginBefore() : LayoutUnit();
}

// -------------------------------------------------------------------------------------------------------

RenderBlock::RenderBlock(ContainerNode* node)
    : RenderBox(node)
    , m_lineHeight(-1)
    , m_hasMarginBeforeQuirk(false)
    , m_hasMarginAfterQuirk(false)
    , m_beingDestroyed(false)
    , m_hasMarkupTruncation(false)
    , m_hasBorderOrPaddingLogicalWidthChanged(false)
{
    setChildrenInline(true);
    COMPILE_ASSERT(sizeof(RenderBlock::FloatingObject) == sizeof(SameSizeAsFloatingObject), FloatingObject_should_stay_small);
    COMPILE_ASSERT(sizeof(RenderBlock::MarginInfo) == sizeof(SameSizeAsMarginInfo), MarginInfo_should_stay_small);
}

static void removeBlockFromDescendantAndContainerMaps(RenderBlock* block, TrackedDescendantsMap*& descendantMap, TrackedContainerMap*& containerMap)
{
    if (OwnPtr<TrackedRendererListHashSet> descendantSet = descendantMap->take(block)) {
        TrackedRendererListHashSet::iterator end = descendantSet->end();
        for (TrackedRendererListHashSet::iterator descendant = descendantSet->begin(); descendant != end; ++descendant) {
            TrackedContainerMap::iterator it = containerMap->find(*descendant);
            ASSERT(it != containerMap->end());
            if (it == containerMap->end())
                continue;
            HashSet<RenderBlock*>* containerSet = it->value.get();
            ASSERT(containerSet->contains(block));
            containerSet->remove(block);
            if (containerSet->isEmpty())
                containerMap->remove(it);
        }
    }
}

RenderBlock::~RenderBlock()
{
    if (m_floatingObjects)
        deleteAllValues(m_floatingObjects->set());
    
    if (hasColumns())
        gColumnInfoMap->take(this);

    if (gPercentHeightDescendantsMap)
        removeBlockFromDescendantAndContainerMaps(this, gPercentHeightDescendantsMap, gPercentHeightContainerMap);
    if (gPositionedDescendantsMap)
        removeBlockFromDescendantAndContainerMaps(this, gPositionedDescendantsMap, gPositionedContainerMap);
}

RenderBlock* RenderBlock::createAnonymous(Document* document)
{
    RenderBlock* renderer = new (document->renderArena()) RenderBlock(0);
    renderer->setDocumentForAnonymous(document);
    return renderer;
}

void RenderBlock::willBeDestroyed()
{
    // Mark as being destroyed to avoid trouble with merges in removeChild().
    m_beingDestroyed = true;

    if (!documentBeingDestroyed()) {
        if (firstChild() && firstChild()->isRunIn())
            moveRunInToOriginalPosition(firstChild());
    }

    // Make sure to destroy anonymous children first while they are still connected to the rest of the tree, so that they will
    // properly dirty line boxes that they are removed from. Effects that do :before/:after only on hover could crash otherwise.
    children()->destroyLeftoverChildren();

    // Destroy our continuation before anything other than anonymous children.
    // The reason we don't destroy it before anonymous children is that they may
    // have continuations of their own that are anonymous children of our continuation.
    RenderBoxModelObject* continuation = this->continuation();
    if (continuation) {
        continuation->destroy();
        setContinuation(0);
    }
    
    if (!documentBeingDestroyed()) {
        if (firstLineBox()) {
            // We can't wait for RenderBox::destroy to clear the selection,
            // because by then we will have nuked the line boxes.
            // FIXME: The FrameSelection should be responsible for this when it
            // is notified of DOM mutations.
            if (isSelectionBorder())
                view()->clearSelection();

            // If we are an anonymous block, then our line boxes might have children
            // that will outlast this block. In the non-anonymous block case those
            // children will be destroyed by the time we return from this function.
            if (isAnonymousBlock()) {
                for (InlineFlowBox* box = firstLineBox(); box; box = box->nextLineBox()) {
                    while (InlineBox* childBox = box->firstChild())
                        childBox->remove();
                }
            }
        } else if (parent())
            parent()->dirtyLinesFromChangedChild(this);
    }

    m_lineBoxes.deleteLineBoxes(renderArena());

    if (lineGridBox())
        lineGridBox()->destroy(renderArena());

    if (UNLIKELY(gDelayedUpdateScrollInfoSet != 0))
        gDelayedUpdateScrollInfoSet->remove(this);

    RenderBox::willBeDestroyed();
}

void RenderBlock::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    RenderStyle* oldStyle = style();
    s_canPropagateFloatIntoSibling = oldStyle ? !isFloatingOrOutOfFlowPositioned() && !avoidsFloats() : false;

    setReplaced(newStyle->isDisplayInlineType());
    
    if (oldStyle && parent() && diff == StyleDifferenceLayout && oldStyle->position() != newStyle->position()) {
        if (newStyle->position() == StaticPosition)
            // Clear our positioned objects list. Our absolutely positioned descendants will be
            // inserted into our containing block's positioned objects list during layout.
            removePositionedObjects(0, NewContainingBlock);
        else if (oldStyle->position() == StaticPosition) {
            // Remove our absolutely positioned descendants from their current containing block.
            // They will be inserted into our positioned objects list during layout.
            RenderObject* cb = parent();
            while (cb && (cb->style()->position() == StaticPosition || (cb->isInline() && !cb->isReplaced())) && !cb->isRenderView()) {
                if (cb->style()->position() == RelativePosition && cb->isInline() && !cb->isReplaced()) {
                    cb = cb->containingBlock();
                    break;
                }
                cb = cb->parent();
            }
            
            if (cb->isRenderBlock())
                toRenderBlock(cb)->removePositionedObjects(this, NewContainingBlock);
        }

        if (containsFloats() && !isFloating() && !isOutOfFlowPositioned() && newStyle->hasOutOfFlowPosition())
            markAllDescendantsWithFloatsForLayout();
    }

    RenderBox::styleWillChange(diff, newStyle);
}

static bool borderOrPaddingLogicalWidthChanged(const RenderStyle* oldStyle, const RenderStyle* newStyle)
{
    if (newStyle->isHorizontalWritingMode())
        return oldStyle->borderLeftWidth() != newStyle->borderLeftWidth()
            || oldStyle->borderRightWidth() != newStyle->borderRightWidth()
            || oldStyle->paddingLeft() != newStyle->paddingLeft()
            || oldStyle->paddingRight() != newStyle->paddingRight();

    return oldStyle->borderTopWidth() != newStyle->borderTopWidth()
        || oldStyle->borderBottomWidth() != newStyle->borderBottomWidth()
        || oldStyle->paddingTop() != newStyle->paddingTop()
        || oldStyle->paddingBottom() != newStyle->paddingBottom();
}

void RenderBlock::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);
    
    RenderStyle* newStyle = style();
    
#if ENABLE(CSS_SHAPES)
    updateShapeInsideInfoAfterStyleChange(newStyle->resolvedShapeInside(), oldStyle ? oldStyle->resolvedShapeInside() : 0);
#endif

    if (!isAnonymousBlock()) {
        // Ensure that all of our continuation blocks pick up the new style.
        for (RenderBlock* currCont = blockElementContinuation(); currCont; currCont = currCont->blockElementContinuation()) {
            RenderBoxModelObject* nextCont = currCont->continuation();
            currCont->setContinuation(0);
            currCont->setStyle(newStyle);
            currCont->setContinuation(nextCont);
        }
    }

    propagateStyleToAnonymousChildren(true);    
    m_lineHeight = -1;

    // After our style changed, if we lose our ability to propagate floats into next sibling
    // blocks, then we need to find the top most parent containing that overhanging float and
    // then mark its descendants with floats for layout and clear all floats from its next
    // sibling blocks that exist in our floating objects list. See bug 56299 and 62875.
    bool canPropagateFloatIntoSibling = !isFloatingOrOutOfFlowPositioned() && !avoidsFloats();
    if (diff == StyleDifferenceLayout && s_canPropagateFloatIntoSibling && !canPropagateFloatIntoSibling && hasOverhangingFloats()) {
        RenderBlock* parentBlock = this;
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = floatingObjectSet.end();

        for (RenderObject* curr = parent(); curr && !curr->isRenderView(); curr = curr->parent()) {
            if (curr->isRenderBlock()) {
                RenderBlock* currBlock = toRenderBlock(curr);

                if (currBlock->hasOverhangingFloats()) {
                    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                        RenderBox* renderer = (*it)->renderer();
                        if (currBlock->hasOverhangingFloat(renderer)) {
                            parentBlock = currBlock;
                            break;
                        }
                    }
                }
            }
        }
              
        parentBlock->markAllDescendantsWithFloatsForLayout();
        parentBlock->markSiblingsWithFloatsForLayout();
    }
    
    // It's possible for our border/padding to change, but for the overall logical width of the block to
    // end up being the same. We keep track of this change so in layoutBlock, we can know to set relayoutChildren=true.
    m_hasBorderOrPaddingLogicalWidthChanged = oldStyle && diff == StyleDifferenceLayout && needsLayout() && borderOrPaddingLogicalWidthChanged(oldStyle, newStyle);
}

RenderBlock* RenderBlock::continuationBefore(RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() == this)
        return this;

    RenderBlock* curr = toRenderBlock(continuation());
    RenderBlock* nextToLast = this;
    RenderBlock* last = this;
    while (curr) {
        if (beforeChild && beforeChild->parent() == curr) {
            if (curr->firstChild() == beforeChild)
                return last;
            return curr;
        }

        nextToLast = last;
        last = curr;
        curr = toRenderBlock(curr->continuation());
    }

    if (!beforeChild && !last->firstChild())
        return nextToLast;
    return last;
}

void RenderBlock::addChildToContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    RenderBlock* flow = continuationBefore(beforeChild);
    ASSERT(!beforeChild || beforeChild->parent()->isAnonymousColumnSpanBlock() || beforeChild->parent()->isRenderBlock());
    RenderBoxModelObject* beforeChildParent = 0;
    if (beforeChild)
        beforeChildParent = toRenderBoxModelObject(beforeChild->parent());
    else {
        RenderBoxModelObject* cont = flow->continuation();
        if (cont)
            beforeChildParent = cont;
        else
            beforeChildParent = flow;
    }

    if (newChild->isFloatingOrOutOfFlowPositioned()) {
        beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
        return;
    }

    // A continuation always consists of two potential candidates: a block or an anonymous
    // column span box holding column span children.
    bool childIsNormal = newChild->isInline() || !newChild->style()->columnSpan();
    bool bcpIsNormal = beforeChildParent->isInline() || !beforeChildParent->style()->columnSpan();
    bool flowIsNormal = flow->isInline() || !flow->style()->columnSpan();

    if (flow == beforeChildParent) {
        flow->addChildIgnoringContinuation(newChild, beforeChild);
        return;
    }
    
    // The goal here is to match up if we can, so that we can coalesce and create the
    // minimal # of continuations needed for the inline.
    if (childIsNormal == bcpIsNormal) {
        beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
        return;
    }
    if (flowIsNormal == childIsNormal) {
        flow->addChildIgnoringContinuation(newChild, 0); // Just treat like an append.
        return;
    }
    beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
}


void RenderBlock::addChildToAnonymousColumnBlocks(RenderObject* newChild, RenderObject* beforeChild)
{
    ASSERT(!continuation()); // We don't yet support column spans that aren't immediate children of the multi-column block.
        
    // The goal is to locate a suitable box in which to place our child.
    RenderBlock* beforeChildParent = 0;
    if (beforeChild) {
        RenderObject* curr = beforeChild;
        while (curr && curr->parent() != this)
            curr = curr->parent();
        beforeChildParent = toRenderBlock(curr);
        ASSERT(beforeChildParent);
        ASSERT(beforeChildParent->isAnonymousColumnsBlock() || beforeChildParent->isAnonymousColumnSpanBlock());
    } else
        beforeChildParent = toRenderBlock(lastChild());

    // If the new child is floating or positioned it can just go in that block.
    if (newChild->isFloatingOrOutOfFlowPositioned()) {
        beforeChildParent->addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);
        return;
    }

    // See if the child can be placed in the box.
    bool newChildHasColumnSpan = newChild->style()->columnSpan() && !newChild->isInline();
    bool beforeChildParentHoldsColumnSpans = beforeChildParent->isAnonymousColumnSpanBlock();

    if (newChildHasColumnSpan == beforeChildParentHoldsColumnSpans) {
        beforeChildParent->addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);
        return;
    }

    if (!beforeChild) {
        // Create a new block of the correct type.
        RenderBlock* newBox = newChildHasColumnSpan ? createAnonymousColumnSpanBlock() : createAnonymousColumnsBlock();
        children()->appendChildNode(this, newBox);
        newBox->addChildIgnoringAnonymousColumnBlocks(newChild, 0);
        return;
    }

    RenderObject* immediateChild = beforeChild;
    bool isPreviousBlockViable = true;
    while (immediateChild->parent() != this) {
        if (isPreviousBlockViable)
            isPreviousBlockViable = !immediateChild->previousSibling();
        immediateChild = immediateChild->parent();
    }
    if (isPreviousBlockViable && immediateChild->previousSibling()) {
        toRenderBlock(immediateChild->previousSibling())->addChildIgnoringAnonymousColumnBlocks(newChild, 0); // Treat like an append.
        return;
    }
        
    // Split our anonymous blocks.
    RenderObject* newBeforeChild = splitAnonymousBoxesAroundChild(beforeChild);

    
    // Create a new anonymous box of the appropriate type.
    RenderBlock* newBox = newChildHasColumnSpan ? createAnonymousColumnSpanBlock() : createAnonymousColumnsBlock();
    children()->insertChildNode(this, newBox, newBeforeChild);
    newBox->addChildIgnoringAnonymousColumnBlocks(newChild, 0);
    return;
}

RenderBlock* RenderBlock::containingColumnsBlock(bool allowAnonymousColumnBlock)
{
    RenderBlock* firstChildIgnoringAnonymousWrappers = 0;
    for (RenderObject* curr = this; curr; curr = curr->parent()) {
        if (!curr->isRenderBlock() || curr->isFloatingOrOutOfFlowPositioned() || curr->isTableCell() || curr->isRoot() || curr->isRenderView() || curr->hasOverflowClip()
            || curr->isInlineBlockOrInlineTable())
            return 0;

        // FIXME: Tables, RenderButtons, and RenderListItems all do special management
        // of their children that breaks when the flow is split through them. Disabling
        // multi-column for them to avoid this problem.
        if (curr->isTable() || curr->isRenderButton() || curr->isListItem())
            return 0;
        
        RenderBlock* currBlock = toRenderBlock(curr);
        if (!currBlock->createsAnonymousWrapper())
            firstChildIgnoringAnonymousWrappers = currBlock;

        if (currBlock->style()->specifiesColumns() && (allowAnonymousColumnBlock || !currBlock->isAnonymousColumnsBlock()))
            return firstChildIgnoringAnonymousWrappers;
            
        if (currBlock->isAnonymousColumnSpanBlock())
            return 0;
    }
    return 0;
}

RenderBlock* RenderBlock::clone() const
{
    RenderBlock* cloneBlock;
    if (isAnonymousBlock()) {
        cloneBlock = createAnonymousBlock();
        cloneBlock->setChildrenInline(childrenInline());
    }
    else {
        RenderObject* cloneRenderer = toElement(node())->createRenderer(renderArena(), style());
        cloneBlock = toRenderBlock(cloneRenderer);
        cloneBlock->setStyle(style());

        // This takes care of setting the right value of childrenInline in case
        // generated content is added to cloneBlock and 'this' does not have
        // generated content added yet.
        cloneBlock->setChildrenInline(cloneBlock->firstChild() ? cloneBlock->firstChild()->isInline() : childrenInline());
    }
    cloneBlock->setFlowThreadState(flowThreadState());
    return cloneBlock;
}

void RenderBlock::splitBlocks(RenderBlock* fromBlock, RenderBlock* toBlock,
                              RenderBlock* middleBlock,
                              RenderObject* beforeChild, RenderBoxModelObject* oldCont)
{
    // Create a clone of this inline.
    RenderBlock* cloneBlock = clone();
    if (!isAnonymousBlock())
        cloneBlock->setContinuation(oldCont);

    if (!beforeChild && isAfterContent(lastChild()))
        beforeChild = lastChild();

    // If we are moving inline children from |this| to cloneBlock, then we need
    // to clear our line box tree.
    if (beforeChild && childrenInline())
        deleteLineBoxTree();

    // Now take all of the children from beforeChild to the end and remove
    // them from |this| and place them in the clone.
    moveChildrenTo(cloneBlock, beforeChild, 0, true);
    
    // Hook |clone| up as the continuation of the middle block.
    if (!cloneBlock->isAnonymousBlock())
        middleBlock->setContinuation(cloneBlock);

    // We have been reparented and are now under the fromBlock.  We need
    // to walk up our block parent chain until we hit the containing anonymous columns block.
    // Once we hit the anonymous columns block we're done.
    RenderBoxModelObject* curr = toRenderBoxModelObject(parent());
    RenderBoxModelObject* currChild = this;
    RenderObject* currChildNextSibling = currChild->nextSibling();

    while (curr && curr != fromBlock) {
        ASSERT_WITH_SECURITY_IMPLICATION(curr->isRenderBlock());
        
        RenderBlock* blockCurr = toRenderBlock(curr);
        
        // Create a new clone.
        RenderBlock* cloneChild = cloneBlock;
        cloneBlock = blockCurr->clone();

        // Insert our child clone as the first child.
        cloneBlock->addChildIgnoringContinuation(cloneChild, 0);

        // Hook the clone up as a continuation of |curr|.  Note we do encounter
        // anonymous blocks possibly as we walk up the block chain.  When we split an
        // anonymous block, there's no need to do any continuation hookup, since we haven't
        // actually split a real element.
        if (!blockCurr->isAnonymousBlock()) {
            oldCont = blockCurr->continuation();
            blockCurr->setContinuation(cloneBlock);
            cloneBlock->setContinuation(oldCont);
        }

        // Now we need to take all of the children starting from the first child
        // *after* currChild and append them all to the clone.
        blockCurr->moveChildrenTo(cloneBlock, currChildNextSibling, 0, true);

        // Keep walking up the chain.
        currChild = curr;
        currChildNextSibling = currChild->nextSibling();
        curr = toRenderBoxModelObject(curr->parent());
    }

    // Now we are at the columns block level. We need to put the clone into the toBlock.
    toBlock->children()->appendChildNode(toBlock, cloneBlock);

    // Now take all the children after currChild and remove them from the fromBlock
    // and put them in the toBlock.
    fromBlock->moveChildrenTo(toBlock, currChildNextSibling, 0, true);
}

void RenderBlock::splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                            RenderObject* newChild, RenderBoxModelObject* oldCont)
{
    RenderBlock* pre = 0;
    RenderBlock* block = containingColumnsBlock();
    
    // Delete our line boxes before we do the inline split into continuations.
    block->deleteLineBoxTree();
    
    bool madeNewBeforeBlock = false;
    if (block->isAnonymousColumnsBlock()) {
        // We can reuse this block and make it the preBlock of the next continuation.
        pre = block;
        pre->removePositionedObjects(0);
        pre->removeFloatingObjects();
        block = toRenderBlock(block->parent());
    } else {
        // No anonymous block available for use.  Make one.
        pre = block->createAnonymousColumnsBlock();
        pre->setChildrenInline(false);
        madeNewBeforeBlock = true;
    }

    RenderBlock* post = block->createAnonymousColumnsBlock();
    post->setChildrenInline(false);

    RenderObject* boxFirst = madeNewBeforeBlock ? block->firstChild() : pre->nextSibling();
    if (madeNewBeforeBlock)
        block->children()->insertChildNode(block, pre, boxFirst);
    block->children()->insertChildNode(block, newBlockBox, boxFirst);
    block->children()->insertChildNode(block, post, boxFirst);
    block->setChildrenInline(false);
    
    if (madeNewBeforeBlock)
        block->moveChildrenTo(pre, boxFirst, 0, true);

    splitBlocks(pre, post, newBlockBox, beforeChild, oldCont);

    // We already know the newBlockBox isn't going to contain inline kids, so avoid wasting
    // time in makeChildrenNonInline by just setting this explicitly up front.
    newBlockBox->setChildrenInline(false);

    // We delayed adding the newChild until now so that the |newBlockBox| would be fully
    // connected, thus allowing newChild access to a renderArena should it need
    // to wrap itself in additional boxes (e.g., table construction).
    newBlockBox->addChild(newChild);

    // Always just do a full layout in order to ensure that line boxes (especially wrappers for images)
    // get deleted properly.  Because objects moves from the pre block into the post block, we want to
    // make new line boxes instead of leaving the old line boxes around.
    pre->setNeedsLayoutAndPrefWidthsRecalc();
    block->setNeedsLayoutAndPrefWidthsRecalc();
    post->setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderBlock::makeChildrenAnonymousColumnBlocks(RenderObject* beforeChild, RenderBlock* newBlockBox, RenderObject* newChild)
{
    RenderBlock* pre = 0;
    RenderBlock* post = 0;
    RenderBlock* block = this; // Eventually block will not just be |this|, but will also be a block nested inside |this|.  Assign to a variable
                               // so that we don't have to patch all of the rest of the code later on.
    
    // Delete the block's line boxes before we do the split.
    block->deleteLineBoxTree();

    if (beforeChild && beforeChild->parent() != this)
        beforeChild = splitAnonymousBoxesAroundChild(beforeChild);

    if (beforeChild != firstChild()) {
        pre = block->createAnonymousColumnsBlock();
        pre->setChildrenInline(block->childrenInline());
    }

    if (beforeChild) {
        post = block->createAnonymousColumnsBlock();
        post->setChildrenInline(block->childrenInline());
    }

    RenderObject* boxFirst = block->firstChild();
    if (pre)
        block->children()->insertChildNode(block, pre, boxFirst);
    block->children()->insertChildNode(block, newBlockBox, boxFirst);
    if (post)
        block->children()->insertChildNode(block, post, boxFirst);
    block->setChildrenInline(false);
    
    // The pre/post blocks always have layers, so we know to always do a full insert/remove (so we pass true as the last argument).
    block->moveChildrenTo(pre, boxFirst, beforeChild, true);
    block->moveChildrenTo(post, beforeChild, 0, true);

    // We already know the newBlockBox isn't going to contain inline kids, so avoid wasting
    // time in makeChildrenNonInline by just setting this explicitly up front.
    newBlockBox->setChildrenInline(false);

    // We delayed adding the newChild until now so that the |newBlockBox| would be fully
    // connected, thus allowing newChild access to a renderArena should it need
    // to wrap itself in additional boxes (e.g., table construction).
    newBlockBox->addChild(newChild);

    // Always just do a full layout in order to ensure that line boxes (especially wrappers for images)
    // get deleted properly.  Because objects moved from the pre block into the post block, we want to
    // make new line boxes instead of leaving the old line boxes around.
    if (pre)
        pre->setNeedsLayoutAndPrefWidthsRecalc();
    block->setNeedsLayoutAndPrefWidthsRecalc();
    if (post)
        post->setNeedsLayoutAndPrefWidthsRecalc();
}

RenderBlock* RenderBlock::columnsBlockForSpanningElement(RenderObject* newChild)
{
    // FIXME: This function is the gateway for the addition of column-span support.  It will
    // be added to in three stages:
    // (1) Immediate children of a multi-column block can span.
    // (2) Nested block-level children with only block-level ancestors between them and the multi-column block can span.
    // (3) Nested children with block or inline ancestors between them and the multi-column block can span (this is when we
    // cross the streams and have to cope with both types of continuations mixed together).
    // This function currently supports (1) and (2).
    RenderBlock* columnsBlockAncestor = 0;
    if (!newChild->isText() && newChild->style()->columnSpan() && !newChild->isBeforeOrAfterContent()
        && !newChild->isFloatingOrOutOfFlowPositioned() && !newChild->isInline() && !isAnonymousColumnSpanBlock()) {
        columnsBlockAncestor = containingColumnsBlock(false);
        if (columnsBlockAncestor) {
            // Make sure that none of the parent ancestors have a continuation.
            // If yes, we do not want split the block into continuations.
            RenderObject* curr = this;
            while (curr && curr != columnsBlockAncestor) {
                if (curr->isRenderBlock() && toRenderBlock(curr)->continuation()) {
                    columnsBlockAncestor = 0;
                    break;
                }
                curr = curr->parent();
            }
        }
    }
    return columnsBlockAncestor;
}

void RenderBlock::addChildIgnoringAnonymousColumnBlocks(RenderObject* newChild, RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() != this) {
        RenderObject* beforeChildContainer = beforeChild->parent();
        while (beforeChildContainer->parent() != this)
            beforeChildContainer = beforeChildContainer->parent();
        ASSERT(beforeChildContainer);

        if (beforeChildContainer->isAnonymous()) {
            // If the requested beforeChild is not one of our children, then this is because
            // there is an anonymous container within this object that contains the beforeChild.
            RenderObject* beforeChildAnonymousContainer = beforeChildContainer;
            if (beforeChildAnonymousContainer->isAnonymousBlock()
#if ENABLE(FULLSCREEN_API)
                // Full screen renderers and full screen placeholders act as anonymous blocks, not tables:
                || beforeChildAnonymousContainer->isRenderFullScreen()
                || beforeChildAnonymousContainer->isRenderFullScreenPlaceholder()
#endif
                ) {
                // Insert the child into the anonymous block box instead of here.
                if (newChild->isInline() || beforeChild->parent()->firstChild() != beforeChild)
                    beforeChild->parent()->addChild(newChild, beforeChild);
                else
                    addChild(newChild, beforeChild->parent());
                return;
            }

            ASSERT(beforeChildAnonymousContainer->isTable());
            if (newChild->isTablePart()) {
                // Insert into the anonymous table.
                beforeChildAnonymousContainer->addChild(newChild, beforeChild);
                return;
            }

            beforeChild = splitAnonymousBoxesAroundChild(beforeChild);

            ASSERT(beforeChild->parent() == this);
            if (beforeChild->parent() != this) {
                // We should never reach here. If we do, we need to use the
                // safe fallback to use the topmost beforeChild container.
                beforeChild = beforeChildContainer;
            }
        } else {
            // We will reach here when beforeChild is a run-in element.
            // If run-in element precedes a block-level element, it becomes the
            // the first inline child of that block level element. The insertion
            // point will be before that block-level element.
            ASSERT(beforeChild->isRunIn());
            beforeChild = beforeChildContainer;
        }
    }

    // Nothing goes before the intruded run-in.
    if (beforeChild && beforeChild->isRunIn() && runInIsPlacedIntoSiblingBlock(beforeChild))
        beforeChild = beforeChild->nextSibling();

    // Check for a spanning element in columns.
    if (gColumnFlowSplitEnabled) {
        RenderBlock* columnsBlockAncestor = columnsBlockForSpanningElement(newChild);
        if (columnsBlockAncestor) {
            TemporaryChange<bool> columnFlowSplitEnabled(gColumnFlowSplitEnabled, false);
            // We are placing a column-span element inside a block.
            RenderBlock* newBox = createAnonymousColumnSpanBlock();
        
            if (columnsBlockAncestor != this && !isRenderFlowThread()) {
                // We are nested inside a multi-column element and are being split by the span. We have to break up
                // our block into continuations.
                RenderBoxModelObject* oldContinuation = continuation();

                // When we split an anonymous block, there's no need to do any continuation hookup,
                // since we haven't actually split a real element.
                if (!isAnonymousBlock())
                    setContinuation(newBox);

                splitFlow(beforeChild, newBox, newChild, oldContinuation);
                return;
            }

            // We have to perform a split of this block's children. This involves creating an anonymous block box to hold
            // the column-spanning |newChild|. We take all of the children from before |newChild| and put them into
            // one anonymous columns block, and all of the children after |newChild| go into another anonymous block.
            makeChildrenAnonymousColumnBlocks(beforeChild, newBox, newChild);
            return;
        }
    }

    bool madeBoxesNonInline = false;

    // A block has to either have all of its children inline, or all of its children as blocks.
    // So, if our children are currently inline and a block child has to be inserted, we move all our
    // inline children into anonymous block boxes.
    if (childrenInline() && !newChild->isInline() && !newChild->isFloatingOrOutOfFlowPositioned()) {
        // This is a block with inline content. Wrap the inline content in anonymous blocks.
        makeChildrenNonInline(beforeChild);
        madeBoxesNonInline = true;

        if (beforeChild && beforeChild->parent() != this) {
            beforeChild = beforeChild->parent();
            ASSERT(beforeChild->isAnonymousBlock());
            ASSERT(beforeChild->parent() == this);
        }
    } else if (!childrenInline() && (newChild->isFloatingOrOutOfFlowPositioned() || newChild->isInline())) {
        // If we're inserting an inline child but all of our children are blocks, then we have to make sure
        // it is put into an anomyous block box. We try to use an existing anonymous box if possible, otherwise
        // a new one is created and inserted into our list of children in the appropriate position.
        RenderObject* afterChild = beforeChild ? beforeChild->previousSibling() : lastChild();

        if (afterChild && afterChild->isAnonymousBlock()) {
            afterChild->addChild(newChild);
            return;
        }

        if (newChild->isInline()) {
            // No suitable existing anonymous box - create a new one.
            RenderBlock* newBox = createAnonymousBlock();
            RenderBox::addChild(newBox, beforeChild);
            newBox->addChild(newChild);
            return;
        }
    }

    RenderBox::addChild(newChild, beforeChild);
 
    // Handle placement of run-ins.
    placeRunInIfNeeded(newChild);

    if (madeBoxesNonInline && parent() && isAnonymousBlock() && parent()->isRenderBlock())
        toRenderBlock(parent())->removeLeftoverAnonymousBlock(this);
    // this object may be dead here
}

void RenderBlock::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (continuation() && !isAnonymousBlock())
        addChildToContinuation(newChild, beforeChild);
    else
        addChildIgnoringContinuation(newChild, beforeChild);
}

void RenderBlock::addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    if (!isAnonymousBlock() && firstChild() && (firstChild()->isAnonymousColumnsBlock() || firstChild()->isAnonymousColumnSpanBlock()))
        addChildToAnonymousColumnBlocks(newChild, beforeChild);
    else
        addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);
}

static void getInlineRun(RenderObject* start, RenderObject* boundary,
                         RenderObject*& inlineRunStart,
                         RenderObject*& inlineRunEnd)
{
    // Beginning at |start| we find the largest contiguous run of inlines that
    // we can.  We denote the run with start and end points, |inlineRunStart|
    // and |inlineRunEnd|.  Note that these two values may be the same if
    // we encounter only one inline.
    //
    // We skip any non-inlines we encounter as long as we haven't found any
    // inlines yet.
    //
    // |boundary| indicates a non-inclusive boundary point.  Regardless of whether |boundary|
    // is inline or not, we will not include it in a run with inlines before it.  It's as though we encountered
    // a non-inline.
    
    // Start by skipping as many non-inlines as we can.
    RenderObject * curr = start;
    bool sawInline;
    do {
        while (curr && !(curr->isInline() || curr->isFloatingOrOutOfFlowPositioned()))
            curr = curr->nextSibling();
        
        inlineRunStart = inlineRunEnd = curr;
        
        if (!curr)
            return; // No more inline children to be found.
        
        sawInline = curr->isInline();
        
        curr = curr->nextSibling();
        while (curr && (curr->isInline() || curr->isFloatingOrOutOfFlowPositioned()) && (curr != boundary)) {
            inlineRunEnd = curr;
            if (curr->isInline())
                sawInline = true;
            curr = curr->nextSibling();
        }
    } while (!sawInline);
}

void RenderBlock::deleteLineBoxTree()
{
    if (containsFloats()) {
        // Clear references to originating lines, since the lines are being deleted
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
            ASSERT(!((*it)->m_originatingLine) || (*it)->m_originatingLine->renderer() == this);
            (*it)->m_originatingLine = 0;
        }
    }
    m_lineBoxes.deleteLineBoxTree(renderArena());

    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->recomputeIsIgnored(this);
}

RootInlineBox* RenderBlock::createRootInlineBox()
{
    return new (renderArena()) RootInlineBox(this);
}

RootInlineBox* RenderBlock::createAndAppendRootInlineBox()
{
    RootInlineBox* rootBox = createRootInlineBox();
    m_lineBoxes.appendLineBox(rootBox);

    if (UNLIKELY(AXObjectCache::accessibilityEnabled()) && m_lineBoxes.firstLineBox() == rootBox) {
        if (AXObjectCache* cache = document()->existingAXObjectCache())
            cache->recomputeIsIgnored(this);
    }

    return rootBox;
}

void RenderBlock::makeChildrenNonInline(RenderObject *insertionPoint)
{    
    // makeChildrenNonInline takes a block whose children are *all* inline and it
    // makes sure that inline children are coalesced under anonymous
    // blocks.  If |insertionPoint| is defined, then it represents the insertion point for
    // the new block child that is causing us to have to wrap all the inlines.  This
    // means that we cannot coalesce inlines before |insertionPoint| with inlines following
    // |insertionPoint|, because the new child is going to be inserted in between the inlines,
    // splitting them.
    ASSERT(isInlineBlockOrInlineTable() || !isInline());
    ASSERT(!insertionPoint || insertionPoint->parent() == this);

    setChildrenInline(false);

    RenderObject *child = firstChild();
    if (!child)
        return;

    deleteLineBoxTree();

    // Since we are going to have block children, we have to move
    // back the run-in to its original place.
    if (child->isRunIn()) {
        moveRunInToOriginalPosition(child);
        child = firstChild();
    }

    while (child) {
        RenderObject *inlineRunStart, *inlineRunEnd;
        getInlineRun(child, insertionPoint, inlineRunStart, inlineRunEnd);

        if (!inlineRunStart)
            break;

        child = inlineRunEnd->nextSibling();

        RenderBlock* block = createAnonymousBlock();
        children()->insertChildNode(this, block, inlineRunStart);
        moveChildrenTo(block, inlineRunStart, child);
    }

#ifndef NDEBUG
    for (RenderObject *c = firstChild(); c; c = c->nextSibling())
        ASSERT(!c->isInline());
#endif

    repaint();
}

void RenderBlock::removeLeftoverAnonymousBlock(RenderBlock* child)
{
    ASSERT(child->isAnonymousBlock());
    ASSERT(!child->childrenInline());
    
    if (child->continuation() || (child->firstChild() && (child->isAnonymousColumnSpanBlock() || child->isAnonymousColumnsBlock())))
        return;
    
    RenderObject* firstAnChild = child->m_children.firstChild();
    RenderObject* lastAnChild = child->m_children.lastChild();
    if (firstAnChild) {
        RenderObject* o = firstAnChild;
        while (o) {
            o->setParent(this);
            o = o->nextSibling();
        }
        firstAnChild->setPreviousSibling(child->previousSibling());
        lastAnChild->setNextSibling(child->nextSibling());
        if (child->previousSibling())
            child->previousSibling()->setNextSibling(firstAnChild);
        if (child->nextSibling())
            child->nextSibling()->setPreviousSibling(lastAnChild);
            
        if (child == m_children.firstChild())
            m_children.setFirstChild(firstAnChild);
        if (child == m_children.lastChild())
            m_children.setLastChild(lastAnChild);
    } else {
        if (child == m_children.firstChild())
            m_children.setFirstChild(child->nextSibling());
        if (child == m_children.lastChild())
            m_children.setLastChild(child->previousSibling());

        if (child->previousSibling())
            child->previousSibling()->setNextSibling(child->nextSibling());
        if (child->nextSibling())
            child->nextSibling()->setPreviousSibling(child->previousSibling());
    }

    child->children()->setFirstChild(0);
    child->m_next = 0;

    // Remove all the information in the flow thread associated with the leftover anonymous block.
    child->removeFromRenderFlowThread();

    child->setParent(0);
    child->setPreviousSibling(0);
    child->setNextSibling(0);

    child->destroy();
}

static bool canMergeContiguousAnonymousBlocks(RenderObject* oldChild, RenderObject* prev, RenderObject* next)
{
    if (oldChild->documentBeingDestroyed() || oldChild->isInline() || oldChild->virtualContinuation())
        return false;

    if ((prev && (!prev->isAnonymousBlock() || toRenderBlock(prev)->continuation() || toRenderBlock(prev)->beingDestroyed()))
        || (next && (!next->isAnonymousBlock() || toRenderBlock(next)->continuation() || toRenderBlock(next)->beingDestroyed())))
        return false;

    // FIXME: This check isn't required when inline run-ins can't be split into continuations.
    if (prev && prev->firstChild() && prev->firstChild()->isInline() && prev->firstChild()->isRunIn())
        return false;

    if ((prev && (prev->isRubyRun() || prev->isRubyBase()))
        || (next && (next->isRubyRun() || next->isRubyBase())))
        return false;

    if (!prev || !next)
        return true;

    // Make sure the types of the anonymous blocks match up.
    return prev->isAnonymousColumnsBlock() == next->isAnonymousColumnsBlock()
           && prev->isAnonymousColumnSpanBlock() == next->isAnonymousColumnSpanBlock();
}

void RenderBlock::collapseAnonymousBoxChild(RenderBlock* parent, RenderObject* child)
{
    parent->setNeedsLayoutAndPrefWidthsRecalc();
    parent->setChildrenInline(child->childrenInline());
    RenderObject* nextSibling = child->nextSibling();

    RenderFlowThread* childFlowThread = child->flowThreadContainingBlock();
    CurrentRenderFlowThreadMaintainer flowThreadMaintainer(childFlowThread);
    
    RenderBlock* anonBlock = toRenderBlock(parent->children()->removeChildNode(parent, child, child->hasLayer()));
    anonBlock->moveAllChildrenTo(parent, nextSibling, child->hasLayer());
    // Delete the now-empty block's lines and nuke it.
    anonBlock->deleteLineBoxTree();
    if (childFlowThread && childFlowThread->isRenderNamedFlowThread())
        toRenderNamedFlowThread(childFlowThread)->removeFlowChildInfo(anonBlock);
    anonBlock->destroy();
}

void RenderBlock::moveAllChildrenIncludingFloatsTo(RenderBlock* toBlock, bool fullRemoveInsert)
{
    moveAllChildrenTo(toBlock, fullRemoveInsert);

    // When a portion of the render tree is being detached, anonymous blocks
    // will be combined as their children are deleted. In this process, the
    // anonymous block later in the tree is merged into the one preceeding it.
    // It can happen that the later block (this) contains floats that the
    // previous block (toBlock) did not contain, and thus are not in the
    // floating objects list for toBlock. This can result in toBlock containing
    // floats that are not in it's floating objects list, but are in the
    // floating objects lists of siblings and parents. This can cause problems
    // when the float itself is deleted, since the deletion code assumes that
    // if a float is not in it's containing block's floating objects list, it
    // isn't in any floating objects list. In order to preserve this condition
    // (removing it has serious performance implications), we need to copy the
    // floating objects from the old block (this) to the new block (toBlock).
    // The float's metrics will likely all be wrong, but since toBlock is
    // already marked for layout, this will get fixed before anything gets
    // displayed.
    // See bug https://bugs.webkit.org/show_bug.cgi?id=115566
    if (m_floatingObjects) {
        if (!toBlock->m_floatingObjects)
            toBlock->createFloatingObjects();

        const FloatingObjectSet& fromFloatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = fromFloatingObjectSet.end();

        for (FloatingObjectSetIterator it = fromFloatingObjectSet.begin(); it != end; ++it) {
            FloatingObject* floatingObject = *it;

            // Don't insert the object again if it's already in the list
            if (toBlock->containsFloat(floatingObject->renderer()))
                continue;

            toBlock->m_floatingObjects->add(floatingObject->clone());
        }
    }
}

void RenderBlock::removeChild(RenderObject* oldChild)
{
    // No need to waste time in merging or removing empty anonymous blocks.
    // We can just bail out if our document is getting destroyed.
    if (documentBeingDestroyed()) {
        RenderBox::removeChild(oldChild);
        return;
    }

    // This protects against column split flows when anonymous blocks are getting merged.
    TemporaryChange<bool> columnFlowSplitEnabled(gColumnFlowSplitEnabled, false);

    // If this child is a block, and if our previous and next siblings are
    // both anonymous blocks with inline content, then we can go ahead and
    // fold the inline content back together.
    RenderObject* prev = oldChild->previousSibling();
    RenderObject* next = oldChild->nextSibling();
    bool canMergeAnonymousBlocks = canMergeContiguousAnonymousBlocks(oldChild, prev, next);
    if (canMergeAnonymousBlocks && prev && next) {
        prev->setNeedsLayoutAndPrefWidthsRecalc();
        RenderBlock* nextBlock = toRenderBlock(next);
        RenderBlock* prevBlock = toRenderBlock(prev);
       
        if (prev->childrenInline() != next->childrenInline()) {
            RenderBlock* inlineChildrenBlock = prev->childrenInline() ? prevBlock : nextBlock;
            RenderBlock* blockChildrenBlock = prev->childrenInline() ? nextBlock : prevBlock;
            
            // Place the inline children block inside of the block children block instead of deleting it.
            // In order to reuse it, we have to reset it to just be a generic anonymous block.  Make sure
            // to clear out inherited column properties by just making a new style, and to also clear the
            // column span flag if it is set.
            ASSERT(!inlineChildrenBlock->continuation());
            RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(style(), BLOCK);
            // Cache this value as it might get changed in setStyle() call.
            bool inlineChildrenBlockHasLayer = inlineChildrenBlock->hasLayer();
            inlineChildrenBlock->setStyle(newStyle);
            children()->removeChildNode(this, inlineChildrenBlock, inlineChildrenBlockHasLayer);
            
            // Now just put the inlineChildrenBlock inside the blockChildrenBlock.
            blockChildrenBlock->children()->insertChildNode(blockChildrenBlock, inlineChildrenBlock, prev == inlineChildrenBlock ? blockChildrenBlock->firstChild() : 0,
                                                            inlineChildrenBlockHasLayer || blockChildrenBlock->hasLayer());
            next->setNeedsLayoutAndPrefWidthsRecalc();
            
            // inlineChildrenBlock got reparented to blockChildrenBlock, so it is no longer a child
            // of "this". we null out prev or next so that is not used later in the function.
            if (inlineChildrenBlock == prevBlock)
                prev = 0;
            else
                next = 0;
        } else {
            // Take all the children out of the |next| block and put them in
            // the |prev| block.
            nextBlock->moveAllChildrenIncludingFloatsTo(prevBlock, nextBlock->hasLayer() || prevBlock->hasLayer());
            
            // Delete the now-empty block's lines and nuke it.
            nextBlock->deleteLineBoxTree();
            nextBlock->destroy();
            next = 0;
        }
    }

    RenderBox::removeChild(oldChild);

    RenderObject* child = prev ? prev : next;
    if (canMergeAnonymousBlocks && child && !child->previousSibling() && !child->nextSibling() && canCollapseAnonymousBlockChild()) {
        // The removal has knocked us down to containing only a single anonymous
        // box.  We can go ahead and pull the content right back up into our
        // box.
        collapseAnonymousBoxChild(this, child);
    } else if (((prev && prev->isAnonymousBlock()) || (next && next->isAnonymousBlock())) && canCollapseAnonymousBlockChild()) {
        // It's possible that the removal has knocked us down to a single anonymous
        // block with pseudo-style element siblings (e.g. first-letter). If these
        // are floating, then we need to pull the content up also.
        RenderBlock* anonBlock = toRenderBlock((prev && prev->isAnonymousBlock()) ? prev : next);
        if ((anonBlock->previousSibling() || anonBlock->nextSibling())
            && (!anonBlock->previousSibling() || (anonBlock->previousSibling()->style()->styleType() != NOPSEUDO && anonBlock->previousSibling()->isFloating() && !anonBlock->previousSibling()->previousSibling()))
            && (!anonBlock->nextSibling() || (anonBlock->nextSibling()->style()->styleType() != NOPSEUDO && anonBlock->nextSibling()->isFloating() && !anonBlock->nextSibling()->nextSibling()))) {
            collapseAnonymousBoxChild(this, anonBlock);
        }
    }

    if (!firstChild()) {
        // If this was our last child be sure to clear out our line boxes.
        if (childrenInline())
            deleteLineBoxTree();

        // If we are an empty anonymous block in the continuation chain,
        // we need to remove ourself and fix the continuation chain.
        if (!beingDestroyed() && isAnonymousBlockContinuation() && !oldChild->isListMarker()) {
            RenderObject* containingBlockIgnoringAnonymous = containingBlock();
            while (containingBlockIgnoringAnonymous && containingBlockIgnoringAnonymous->isAnonymousBlock())
                containingBlockIgnoringAnonymous = containingBlockIgnoringAnonymous->containingBlock();
            for (RenderObject* curr = this; curr; curr = curr->previousInPreOrder(containingBlockIgnoringAnonymous)) {
                if (curr->virtualContinuation() != this)
                    continue;

                // Found our previous continuation. We just need to point it to
                // |this|'s next continuation.
                RenderBoxModelObject* nextContinuation = continuation();
                if (curr->isRenderInline())
                    toRenderInline(curr)->setContinuation(nextContinuation);
                else if (curr->isRenderBlock())
                    toRenderBlock(curr)->setContinuation(nextContinuation);
                else
                    ASSERT_NOT_REACHED();

                break;
            }
            setContinuation(0);
            destroy();
        }
    }
}

bool RenderBlock::isSelfCollapsingBlock() const
{
    // We are not self-collapsing if we
    // (a) have a non-zero height according to layout (an optimization to avoid wasting time)
    // (b) are a table,
    // (c) have border/padding,
    // (d) have a min-height
    // (e) have specified that one of our margins can't collapse using a CSS extension
    if (logicalHeight() > 0
        || isTable() || borderAndPaddingLogicalHeight()
        || style()->logicalMinHeight().isPositive()
        || style()->marginBeforeCollapse() == MSEPARATE || style()->marginAfterCollapse() == MSEPARATE)
        return false;

    Length logicalHeightLength = style()->logicalHeight();
    bool hasAutoHeight = logicalHeightLength.isAuto();
    if (logicalHeightLength.isPercent() && !document()->inQuirksMode()) {
        hasAutoHeight = true;
        for (RenderBlock* cb = containingBlock(); !cb->isRenderView(); cb = cb->containingBlock()) {
            if (cb->style()->logicalHeight().isFixed() || cb->isTableCell())
                hasAutoHeight = false;
        }
    }

    // If the height is 0 or auto, then whether or not we are a self-collapsing block depends
    // on whether we have content that is all self-collapsing or not.
    if (hasAutoHeight || ((logicalHeightLength.isFixed() || logicalHeightLength.isPercent()) && logicalHeightLength.isZero())) {
        // If the block has inline children, see if we generated any line boxes.  If we have any
        // line boxes, then we can't be self-collapsing, since we have content.
        if (childrenInline())
            return !firstLineBox();
        
        // Whether or not we collapse is dependent on whether all our normal flow children
        // are also self-collapsing.
        for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
            if (child->isFloatingOrOutOfFlowPositioned())
                continue;
            if (!child->isSelfCollapsingBlock())
                return false;
        }
        return true;
    }
    return false;
}

void RenderBlock::startDelayUpdateScrollInfo()
{
    if (gDelayUpdateScrollInfo == 0) {
        ASSERT(!gDelayedUpdateScrollInfoSet);
        gDelayedUpdateScrollInfoSet = new DelayedUpdateScrollInfoSet;
    }
    ASSERT(gDelayedUpdateScrollInfoSet);
    ++gDelayUpdateScrollInfo;
}

void RenderBlock::finishDelayUpdateScrollInfo()
{
    --gDelayUpdateScrollInfo;
    ASSERT(gDelayUpdateScrollInfo >= 0);
    if (gDelayUpdateScrollInfo == 0) {
        ASSERT(gDelayedUpdateScrollInfoSet);

        OwnPtr<DelayedUpdateScrollInfoSet> infoSet(adoptPtr(gDelayedUpdateScrollInfoSet));
        gDelayedUpdateScrollInfoSet = 0;

        for (DelayedUpdateScrollInfoSet::iterator it = infoSet->begin(); it != infoSet->end(); ++it) {
            RenderBlock* block = *it;
            if (block->hasOverflowClip()) {
                block->layer()->updateScrollInfoAfterLayout();
                block->clearLayoutOverflow();
            }
        }
    }
}

void RenderBlock::updateScrollInfoAfterLayout()
{
    if (hasOverflowClip()) {
        if (style()->isFlippedBlocksWritingMode()) {
            // FIXME: https://bugs.webkit.org/show_bug.cgi?id=97937
            // Workaround for now. We cannot delay the scroll info for overflow
            // for items with opposite writing directions, as the contents needs
            // to overflow in that direction
            layer()->updateScrollInfoAfterLayout();
            return;
        }

        if (gDelayUpdateScrollInfo)
            gDelayedUpdateScrollInfoSet->add(this);
        else
            layer()->updateScrollInfoAfterLayout();
    }
}

void RenderBlock::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    OverflowEventDispatcher dispatcher(this);

    // Update our first letter info now.
    updateFirstLetter();

    // Table cells call layoutBlock directly, so don't add any logic here.  Put code into
    // layoutBlock().
    layoutBlock(false);
    
    // It's safe to check for control clip here, since controls can never be table cells.
    // If we have a lightweight clip, there can never be any overflow from children.
    if (hasControlClip() && m_overflow && !gDelayUpdateScrollInfo)
        clearLayoutOverflow();

    invalidateBackgroundObscurationStatus();
}

#if ENABLE(CSS_SHAPES)
void RenderBlock::updateShapeInsideInfoAfterStyleChange(const ShapeValue* shapeInside, const ShapeValue* oldShapeInside)
{
    // FIXME: A future optimization would do a deep comparison for equality.
    if (shapeInside == oldShapeInside)
        return;

    if (shapeInside) {
        ShapeInsideInfo* shapeInsideInfo = ensureShapeInsideInfo();
        shapeInsideInfo->dirtyShapeSize();
    } else {
        setShapeInsideInfo(nullptr);
        markShapeInsideDescendantsForLayout();
    }
}

void RenderBlock::markShapeInsideDescendantsForLayout()
{
    if (!everHadLayout())
        return;
    if (childrenInline()) {
        setNeedsLayout(true);
        return;
    }
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isRenderBlock())
            continue;
        RenderBlock* childBlock = toRenderBlock(child);
        childBlock->markShapeInsideDescendantsForLayout();
    }
}
#endif

static inline bool shapeInfoRequiresRelayout(const RenderBlock* block)
{
#if !ENABLE(CSS_SHAPES)
    return false;
#else
    ShapeInsideInfo* info = block->shapeInsideInfo();
    if (info)
        info->setNeedsLayout(info->shapeSizeDirty());
    else
        info = block->layoutShapeInsideInfo();
    return info && info->needsLayout();
#endif
}

bool RenderBlock::updateRegionsAndShapesBeforeChildLayout(RenderFlowThread* flowThread)
{
#if ENABLE(CSS_SHAPES)
    if (!flowThread && !shapeInsideInfo())
#else
    if (!flowThread)
#endif
        return shapeInfoRequiresRelayout(this);

    LayoutUnit oldHeight = logicalHeight();
    LayoutUnit oldTop = logicalTop();

    // Compute the maximum logical height content may cause this block to expand to
    // FIXME: These should eventually use the const computeLogicalHeight rather than updateLogicalHeight
    setLogicalHeight(RenderFlowThread::maxLogicalHeight());
    updateLogicalHeight();

#if ENABLE(CSS_SHAPES)
    computeShapeSize();
#endif

    // Set our start and end regions. No regions above or below us will be considered by our children. They are
    // effectively clamped to our region range.
    computeRegionRangeForBlock(flowThread);

    setLogicalHeight(oldHeight);
    setLogicalTop(oldTop);
    
    return shapeInfoRequiresRelayout(this);
}

#if ENABLE(CSS_SHAPES)
void RenderBlock::computeShapeSize()
{
    ShapeInsideInfo* shapeInsideInfo = this->shapeInsideInfo();
    if (shapeInsideInfo) {
        bool percentageLogicalHeightResolvable = percentageLogicalHeightIsResolvableFromBlock(this, false);
        shapeInsideInfo->setShapeSize(logicalWidth(), percentageLogicalHeightResolvable ? logicalHeight() : LayoutUnit());
    }
}
#endif

void RenderBlock::updateRegionsAndShapesAfterChildLayout(RenderFlowThread* flowThread, bool heightChanged)
{
#if ENABLE(CSS_SHAPES)
    // A previous sibling has changed dimension, so we need to relayout the shape with the content
    ShapeInsideInfo* shapeInsideInfo = layoutShapeInsideInfo();
    if (heightChanged && shapeInsideInfo)
        shapeInsideInfo->dirtyShapeSize();
#else
    UNUSED_PARAM(heightChanged);
#endif
    computeRegionRangeForBlock(flowThread);
}

void RenderBlock::computeRegionRangeForBlock(RenderFlowThread* flowThread)
{
    if (flowThread)
        flowThread->setRegionRangeForBox(this, offsetFromLogicalTopOfFirstPage());
}

bool RenderBlock::updateLogicalWidthAndColumnWidth()
{
    LayoutUnit oldWidth = logicalWidth();
    LayoutUnit oldColumnWidth = desiredColumnWidth();

    updateLogicalWidth();
    calcColumnWidth();

    bool hasBorderOrPaddingLogicalWidthChanged = m_hasBorderOrPaddingLogicalWidthChanged;
    m_hasBorderOrPaddingLogicalWidthChanged = false;

    return oldWidth != logicalWidth() || oldColumnWidth != desiredColumnWidth() || hasBorderOrPaddingLogicalWidthChanged;
}

void RenderBlock::checkForPaginationLogicalHeightChange(LayoutUnit& pageLogicalHeight, bool& pageLogicalHeightChanged, bool& hasSpecifiedPageLogicalHeight)
{
    ColumnInfo* colInfo = columnInfo();
    if (hasColumns()) {
        if (!pageLogicalHeight) {
            // We need to go ahead and set our explicit page height if one exists, so that we can
            // avoid doing two layout passes.
            updateLogicalHeight();
            LayoutUnit columnHeight = isRenderView() ? view()->pageOrViewLogicalHeight() : contentLogicalHeight();
            if (columnHeight > 0) {
                pageLogicalHeight = columnHeight;
                hasSpecifiedPageLogicalHeight = true;
            }
            setLogicalHeight(0);
        }

        if (colInfo->columnHeight() != pageLogicalHeight && everHadLayout())
            pageLogicalHeightChanged = true;

        colInfo->setColumnHeight(pageLogicalHeight);
        
        if (!hasSpecifiedPageLogicalHeight && !pageLogicalHeight)
            colInfo->clearForcedBreaks();

        colInfo->setPaginationUnit(paginationUnit());
    } else if (isRenderFlowThread()) {
        pageLogicalHeight = 1; // This is just a hack to always make sure we have a page logical height.
        pageLogicalHeightChanged = toRenderFlowThread(this)->pageLogicalSizeChanged();
    }
}

void RenderBlock::layoutBlock(bool relayoutChildren, LayoutUnit pageLogicalHeight)
{
    ASSERT(needsLayout());

    if (isInline() && !isInlineBlockOrInlineTable()) // Inline <form>s inside various table elements can
        return;                                      // cause us to come in here.  Just bail.

    if (!relayoutChildren && simplifiedLayout())
        return;

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());

    if (updateLogicalWidthAndColumnWidth())
        relayoutChildren = true;

    clearFloats();

    LayoutUnit previousHeight = logicalHeight();
    // FIXME: should this start out as borderAndPaddingLogicalHeight() + scrollbarLogicalHeight(),
    // for consistency with other render classes?
    setLogicalHeight(0);

    bool pageLogicalHeightChanged = false;
    bool hasSpecifiedPageLogicalHeight = false;
    checkForPaginationLogicalHeightChange(pageLogicalHeight, pageLogicalHeightChanged, hasSpecifiedPageLogicalHeight);

    RenderView* renderView = view();
    RenderStyle* styleToUse = style();
    LayoutStateMaintainer statePusher(renderView, this, locationOffset(), hasColumns() || hasTransform() || hasReflection() || styleToUse->isFlippedBlocksWritingMode(), pageLogicalHeight, pageLogicalHeightChanged, columnInfo());

    // Regions changing widths can force us to relayout our children.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (logicalWidthChangedInRegions(flowThread))
        relayoutChildren = true;
    if (updateRegionsAndShapesBeforeChildLayout(flowThread))
        relayoutChildren = true;

    // We use four values, maxTopPos, maxTopNeg, maxBottomPos, and maxBottomNeg, to track
    // our current maximal positive and negative margins.  These values are used when we
    // are collapsed with adjacent blocks, so for example, if you have block A and B
    // collapsing together, then you'd take the maximal positive margin from both A and B
    // and subtract it from the maximal negative margin from both A and B to get the
    // true collapsed margin.  This algorithm is recursive, so when we finish layout()
    // our block knows its current maximal positive/negative values.
    //
    // Start out by setting our margin values to our current margins.  Table cells have
    // no margins, so we don't fill in the values for table cells.
    bool isCell = isTableCell();
    if (!isCell) {
        initMaxMarginValues();
        
        setHasMarginBeforeQuirk(styleToUse->hasMarginBeforeQuirk());
        setHasMarginAfterQuirk(styleToUse->hasMarginAfterQuirk());
        setPaginationStrut(0);
    }

    LayoutUnit repaintLogicalTop = 0;
    LayoutUnit repaintLogicalBottom = 0;
    LayoutUnit maxFloatLogicalBottom = 0;
    if (!firstChild() && !isAnonymousBlock())
        setChildrenInline(true);
    if (childrenInline())
        layoutInlineChildren(relayoutChildren, repaintLogicalTop, repaintLogicalBottom);
    else
        layoutBlockChildren(relayoutChildren, maxFloatLogicalBottom);

    // Expand our intrinsic height to encompass floats.
    LayoutUnit toAdd = borderAndPaddingAfter() + scrollbarLogicalHeight();
    if (lowestFloatLogicalBottom() > (logicalHeight() - toAdd) && expandsToEncloseOverhangingFloats())
        setLogicalHeight(lowestFloatLogicalBottom() + toAdd);
    
    if (relayoutForPagination(hasSpecifiedPageLogicalHeight, pageLogicalHeight, statePusher))
        return;

    // Calculate our new height.
    LayoutUnit oldHeight = logicalHeight();
    LayoutUnit oldClientAfterEdge = clientLogicalBottom();

    // Before updating the final size of the flow thread make sure a forced break is applied after the content.
    // This ensures the size information is correctly computed for the last auto-height region receiving content.
    if (isRenderFlowThread())
        toRenderFlowThread(this)->applyBreakAfterContent(oldClientAfterEdge);

    updateLogicalHeight();
    LayoutUnit newHeight = logicalHeight();
    if (oldHeight != newHeight) {
        if (oldHeight > newHeight && maxFloatLogicalBottom > newHeight && !childrenInline()) {
            // One of our children's floats may have become an overhanging float for us. We need to look for it.
            for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
                if (child->isBlockFlow() && !child->isFloatingOrOutOfFlowPositioned()) {
                    RenderBlock* block = toRenderBlock(child);
                    if (block->lowestFloatLogicalBottom() + block->logicalTop() > newHeight)
                        addOverhangingFloats(block, false);
                }
            }
        }
    }

    bool heightChanged = (previousHeight != newHeight);
    if (heightChanged)
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    updateRegionsAndShapesAfterChildLayout(flowThread, heightChanged);

    // Add overflow from children (unless we're multi-column, since in that case all our child overflow is clipped anyway).
    computeOverflow(oldClientAfterEdge);
    
    statePusher.pop();

    fitBorderToLinesIfNeeded();

    if (renderView->layoutState()->m_pageLogicalHeight)
        setPageLogicalOffset(renderView->layoutState()->pageLogicalOffset(this, logicalTop()));

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    // FIXME: This repaint logic should be moved into a separate helper function!
    // Repaint with our new bounds if they are different from our old bounds.
    bool didFullRepaint = repainter.repaintAfterLayout();
    if (!didFullRepaint && repaintLogicalTop != repaintLogicalBottom && (styleToUse->visibility() == VISIBLE || enclosingLayer()->hasVisibleContent())) {
        // FIXME: We could tighten up the left and right invalidation points if we let layoutInlineChildren fill them in based off the particular lines
        // it had to lay out.  We wouldn't need the hasOverflowClip() hack in that case either.
        LayoutUnit repaintLogicalLeft = logicalLeftVisualOverflow();
        LayoutUnit repaintLogicalRight = logicalRightVisualOverflow();
        if (hasOverflowClip()) {
            // If we have clipped overflow, we should use layout overflow as well, since visual overflow from lines didn't propagate to our block's overflow.
            // Note the old code did this as well but even for overflow:visible.  The addition of hasOverflowClip() at least tightens up the hack a bit.
            // layoutInlineChildren should be patched to compute the entire repaint rect.
            repaintLogicalLeft = min(repaintLogicalLeft, logicalLeftLayoutOverflow());
            repaintLogicalRight = max(repaintLogicalRight, logicalRightLayoutOverflow());
        }
        
        LayoutRect repaintRect;
        if (isHorizontalWritingMode())
            repaintRect = LayoutRect(repaintLogicalLeft, repaintLogicalTop, repaintLogicalRight - repaintLogicalLeft, repaintLogicalBottom - repaintLogicalTop);
        else
            repaintRect = LayoutRect(repaintLogicalTop, repaintLogicalLeft, repaintLogicalBottom - repaintLogicalTop, repaintLogicalRight - repaintLogicalLeft);

        // The repaint rect may be split across columns, in which case adjustRectForColumns() will return the union.
        adjustRectForColumns(repaintRect);

        repaintRect.inflate(maximalOutlineSize(PaintPhaseOutline));
        
        if (hasOverflowClip()) {
            // Adjust repaint rect for scroll offset
            repaintRect.move(-scrolledContentOffset());

            // Don't allow this rect to spill out of our overflow box.
            repaintRect.intersect(LayoutRect(LayoutPoint(), size()));
        }

        // Make sure the rect is still non-empty after intersecting for overflow above
        if (!repaintRect.isEmpty()) {
            repaintRectangle(repaintRect); // We need to do a partial repaint of our content.
            if (hasReflection())
                repaintRectangle(reflectedRect(repaintRect));
        }
    }

    setNeedsLayout(false);

    if (document()->printing()) {
        // PHANTOMJS CUSTOM: reset pagination counter for printing
        StyledElement* elem = dynamic_cast<StyledElement*>(generatingNode());
        if (elem && elem->hasClass() && elem->classNames().contains("phantomjs_reset_pagination")) {
            frame()->addResetPage(y() / view()->layoutState()->m_pageLogicalHeight);
        }
    }

}

void RenderBlock::addOverflowFromChildren()
{
    if (!hasColumns()) {
        if (childrenInline())
            addOverflowFromInlineChildren();
        else
            addOverflowFromBlockChildren();
    } else {
        ColumnInfo* colInfo = columnInfo();
        if (columnCount(colInfo)) {
            LayoutRect lastRect = columnRectAt(colInfo, columnCount(colInfo) - 1);
            addLayoutOverflow(lastRect);
            if (!hasOverflowClip())
                addVisualOverflow(lastRect);
        }
    }
}

void RenderBlock::computeOverflow(LayoutUnit oldClientAfterEdge, bool recomputeFloats)
{
    m_overflow.clear();

    // Add overflow from children.
    addOverflowFromChildren();

    if (!hasColumns() && (recomputeFloats || isRoot() || expandsToEncloseOverhangingFloats() || hasSelfPaintingLayer()))
        addOverflowFromFloats();

    // Add in the overflow from positioned objects.
    addOverflowFromPositionedObjects();

    if (hasOverflowClip()) {
        // When we have overflow clip, propagate the original spillout since it will include collapsed bottom margins
        // and bottom padding.  Set the axis we don't care about to be 1, since we want this overflow to always
        // be considered reachable.
        LayoutRect clientRect(clientBoxRect());
        LayoutRect rectToApply;
        if (isHorizontalWritingMode())
            rectToApply = LayoutRect(clientRect.x(), clientRect.y(), 1, max<LayoutUnit>(0, oldClientAfterEdge - clientRect.y()));
        else
            rectToApply = LayoutRect(clientRect.x(), clientRect.y(), max<LayoutUnit>(0, oldClientAfterEdge - clientRect.x()), 1);
        addLayoutOverflow(rectToApply);
        if (hasRenderOverflow())
            m_overflow->setLayoutClientAfterEdge(oldClientAfterEdge);
    }
        
    // Allow our overflow to catch cases where the caret in an empty editable element with negative text indent needs to get painted.
    LayoutUnit textIndent = textIndentOffset();
    if (textIndent < 0) {
        LayoutRect clientRect(clientBoxRect());
        LayoutRect rectToApply = LayoutRect(clientRect.x() + min<LayoutUnit>(0, textIndent), clientRect.y(), clientRect.width() - min<LayoutUnit>(0, textIndent), clientRect.height());
        addVisualOverflow(rectToApply);
    }

    // Add visual overflow from box-shadow and border-image-outset.
    addVisualEffectOverflow();

    // Add visual overflow from theme.
    addVisualOverflowFromTheme();

    if (isRenderNamedFlowThread())
        toRenderNamedFlowThread(this)->computeOversetStateForRegions(oldClientAfterEdge);
}

void RenderBlock::clearLayoutOverflow()
{
    if (!m_overflow)
        return;
    
    if (visualOverflowRect() == borderBoxRect()) {
        m_overflow.clear();
        return;
    }
    
    m_overflow->setLayoutOverflow(borderBoxRect());
}

void RenderBlock::addOverflowFromBlockChildren()
{
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (!child->isFloatingOrOutOfFlowPositioned())
            addOverflowFromChild(child);
    }
}

void RenderBlock::addOverflowFromFloats()
{
    if (!m_floatingObjects)
        return;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        if (r->isDescendant())
            addOverflowFromChild(r->m_renderer, IntSize(xPositionForFloatIncludingMargin(r), yPositionForFloatIncludingMargin(r)));
    }
}

void RenderBlock::addOverflowFromPositionedObjects()
{
    TrackedRendererListHashSet* positionedDescendants = positionedObjects();
    if (!positionedDescendants)
        return;

    RenderBox* positionedObject;
    TrackedRendererListHashSet::iterator end = positionedDescendants->end();
    for (TrackedRendererListHashSet::iterator it = positionedDescendants->begin(); it != end; ++it) {
        positionedObject = *it;
        
        // Fixed positioned elements don't contribute to layout overflow, since they don't scroll with the content.
        if (positionedObject->style()->position() != FixedPosition) {
            LayoutUnit x = positionedObject->x();
            if (style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
                x -= verticalScrollbarWidth();
            addOverflowFromChild(positionedObject, LayoutSize(x, positionedObject->y()));
        }
    }
}

void RenderBlock::addVisualOverflowFromTheme()
{
    if (!style()->hasAppearance())
        return;

    IntRect inflatedRect = pixelSnappedBorderBoxRect();
    theme()->adjustRepaintRect(this, inflatedRect);
    addVisualOverflow(inflatedRect);
}

bool RenderBlock::expandsToEncloseOverhangingFloats() const
{
    return isInlineBlockOrInlineTable() || isFloatingOrOutOfFlowPositioned() || hasOverflowClip() || (parent() && parent()->isFlexibleBoxIncludingDeprecated())
           || hasColumns() || isTableCell() || isTableCaption() || isFieldset() || isWritingModeRoot() || isRoot();
}

void RenderBlock::adjustPositionedBlock(RenderBox* child, const MarginInfo& marginInfo)
{
    bool isHorizontal = isHorizontalWritingMode();
    bool hasStaticBlockPosition = child->style()->hasStaticBlockPosition(isHorizontal);
    
    LayoutUnit logicalTop = logicalHeight();
    updateStaticInlinePositionForChild(child, logicalTop);

    if (!marginInfo.canCollapseWithMarginBefore()) {
        // Positioned blocks don't collapse margins, so add the margin provided by
        // the container now. The child's own margin is added later when calculating its logical top.
        LayoutUnit collapsedBeforePos = marginInfo.positiveMargin();
        LayoutUnit collapsedBeforeNeg = marginInfo.negativeMargin();
        logicalTop += collapsedBeforePos - collapsedBeforeNeg;
    }
    
    RenderLayer* childLayer = child->layer();
    if (childLayer->staticBlockPosition() != logicalTop) {
        childLayer->setStaticBlockPosition(logicalTop);
        if (hasStaticBlockPosition)
            child->setChildNeedsLayout(true, MarkOnlyThis);
    }
}

void RenderBlock::adjustFloatingBlock(const MarginInfo& marginInfo)
{
    // The float should be positioned taking into account the bottom margin
    // of the previous flow.  We add that margin into the height, get the
    // float positioned properly, and then subtract the margin out of the
    // height again.  In the case of self-collapsing blocks, we always just
    // use the top margins, since the self-collapsing block collapsed its
    // own bottom margin into its top margin.
    //
    // Note also that the previous flow may collapse its margin into the top of
    // our block.  If this is the case, then we do not add the margin in to our
    // height when computing the position of the float.   This condition can be tested
    // for by simply calling canCollapseWithMarginBefore.  See
    // http://www.hixie.ch/tests/adhoc/css/box/block/margin-collapse/046.html for
    // an example of this scenario.
    LayoutUnit marginOffset = marginInfo.canCollapseWithMarginBefore() ? LayoutUnit() : marginInfo.margin();
    setLogicalHeight(logicalHeight() + marginOffset);
    positionNewFloats();
    setLogicalHeight(logicalHeight() - marginOffset);
}

static void destroyRunIn(RenderBoxModelObject* runIn)
{
    ASSERT(runIn->isRunIn());
    ASSERT(!runIn->firstChild());

    // Delete our line box tree. This is needed as our children got moved
    // and our line box tree is no longer valid.
    if (runIn->isRenderBlock())
        toRenderBlock(runIn)->deleteLineBoxTree();
    else if (runIn->isRenderInline())
        toRenderInline(runIn)->deleteLineBoxTree();
    else
        ASSERT_NOT_REACHED();

    runIn->destroy();
}

void RenderBlock::placeRunInIfNeeded(RenderObject* newChild)
{
    if (newChild->isRunIn())
        moveRunInUnderSiblingBlockIfNeeded(newChild);
    else if (RenderObject* prevSibling = newChild->previousSibling()) {
        if (prevSibling->isRunIn())
            moveRunInUnderSiblingBlockIfNeeded(prevSibling);
    }
}

RenderBoxModelObject* RenderBlock::createReplacementRunIn(RenderBoxModelObject* runIn)
{
    ASSERT(runIn->isRunIn());
    ASSERT(runIn->node());

    RenderBoxModelObject* newRunIn = 0;
    if (!runIn->isRenderBlock())
        newRunIn = new (renderArena()) RenderBlock(runIn->node());
    else
        newRunIn = new (renderArena()) RenderInline(toElement(runIn->node()));

    runIn->node()->setRenderer(newRunIn);
    newRunIn->setStyle(runIn->style());

    runIn->moveAllChildrenTo(newRunIn, true);

    return newRunIn;
}

void RenderBlock::moveRunInUnderSiblingBlockIfNeeded(RenderObject* runIn)
{
    ASSERT(runIn->isRunIn());

    // See if we have inline children. If the children aren't inline,
    // then just treat the run-in as a normal block.
    if (!runIn->childrenInline())
        return;

    // FIXME: We don't handle non-block elements with run-in for now.
    if (!runIn->isRenderBlock())
        return;

    // FIXME: We don't support run-ins with or as part of a continuation
    // as it makes the back-and-forth placing complex.
    if (runIn->isElementContinuation() || runIn->virtualContinuation())
        return;

    // Check if this node is allowed to run-in. E.g. <select> expects its renderer to
    // be a RenderListBox or RenderMenuList, and hence cannot be a RenderInline run-in.
    if (!runIn->canBeReplacedWithInlineRunIn())
        return;

    RenderObject* curr = runIn->nextSibling();
    if (!curr || !curr->isRenderBlock() || !curr->childrenInline())
        return;

    if (toRenderBlock(curr)->beingDestroyed())
        return;

    // Per CSS3, "A run-in cannot run in to a block that already starts with a
    // run-in or that itself is a run-in".
    if (curr->isRunIn() || (curr->firstChild() && curr->firstChild()->isRunIn()))
        return;

    if (curr->isAnonymous() || curr->isFloatingOrOutOfFlowPositioned())
        return;

    RenderBoxModelObject* oldRunIn = toRenderBoxModelObject(runIn);
    RenderBoxModelObject* newRunIn = createReplacementRunIn(oldRunIn);
    destroyRunIn(oldRunIn);

    // Now insert the new child under |curr| block. Use addChild instead of insertChildNode
    // since it handles correct placement of the children, especially where we cannot insert
    // anything before the first child. e.g. details tag. See https://bugs.webkit.org/show_bug.cgi?id=58228.
    curr->addChild(newRunIn, curr->firstChild());

    // Make sure that |this| get a layout since its run-in child moved.
    curr->setNeedsLayoutAndPrefWidthsRecalc();
}

bool RenderBlock::runInIsPlacedIntoSiblingBlock(RenderObject* runIn)
{
    ASSERT(runIn->isRunIn());

    // If we don't have a parent, we can't be moved into our sibling block.
    if (!parent())
        return false;

    // An intruded run-in needs to be an inline.
    if (!runIn->isRenderInline())
        return false;

    return true;
}

void RenderBlock::moveRunInToOriginalPosition(RenderObject* runIn)
{
    ASSERT(runIn->isRunIn());

    if (!runInIsPlacedIntoSiblingBlock(runIn))
        return;

    // FIXME: Run-in that are now placed in sibling block can break up into continuation
    // chains when new children are added to it. We cannot easily send them back to their
    // original place since that requires writing integration logic with RenderInline::addChild
    // and all other places that might cause continuations to be created (without blowing away
    // |this|). Disabling this feature for now to prevent crashes.
    if (runIn->isElementContinuation() || runIn->virtualContinuation())
        return;

    RenderBoxModelObject* oldRunIn = toRenderBoxModelObject(runIn);
    RenderBoxModelObject* newRunIn = createReplacementRunIn(oldRunIn);
    destroyRunIn(oldRunIn);

    // Add the run-in block as our previous sibling.
    parent()->addChild(newRunIn, this);

    // Make sure that the parent holding the new run-in gets layout.
    parent()->setNeedsLayoutAndPrefWidthsRecalc();
}

LayoutUnit RenderBlock::collapseMargins(RenderBox* child, MarginInfo& marginInfo)
{
    bool childDiscardMarginBefore = mustDiscardMarginBeforeForChild(child);
    bool childDiscardMarginAfter = mustDiscardMarginAfterForChild(child);
    bool childIsSelfCollapsing = child->isSelfCollapsingBlock();

    // The child discards the before margin when the the after margin has discard in the case of a self collapsing block.
    childDiscardMarginBefore = childDiscardMarginBefore || (childDiscardMarginAfter && childIsSelfCollapsing);

    // Get the four margin values for the child and cache them.
    const MarginValues childMargins = marginValuesForChild(child);

    // Get our max pos and neg top margins.
    LayoutUnit posTop = childMargins.positiveMarginBefore();
    LayoutUnit negTop = childMargins.negativeMarginBefore();

    // For self-collapsing blocks, collapse our bottom margins into our
    // top to get new posTop and negTop values.
    if (childIsSelfCollapsing) {
        posTop = max(posTop, childMargins.positiveMarginAfter());
        negTop = max(negTop, childMargins.negativeMarginAfter());
    }
    
    // See if the top margin is quirky. We only care if this child has
    // margins that will collapse with us.
    bool topQuirk = hasMarginBeforeQuirk(child);

    if (marginInfo.canCollapseWithMarginBefore()) {
        if (!childDiscardMarginBefore && !marginInfo.discardMargin()) {
            // This child is collapsing with the top of the
            // block. If it has larger margin values, then we need to update
            // our own maximal values.
            if (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !topQuirk)
                setMaxMarginBeforeValues(max(posTop, maxPositiveMarginBefore()), max(negTop, maxNegativeMarginBefore()));

            // The minute any of the margins involved isn't a quirk, don't
            // collapse it away, even if the margin is smaller (www.webreference.com
            // has an example of this, a <dt> with 0.8em author-specified inside
            // a <dl> inside a <td>.
            if (!marginInfo.determinedMarginBeforeQuirk() && !topQuirk && (posTop - negTop)) {
                setHasMarginBeforeQuirk(false);
                marginInfo.setDeterminedMarginBeforeQuirk(true);
            }

            if (!marginInfo.determinedMarginBeforeQuirk() && topQuirk && !marginBefore())
                // We have no top margin and our top child has a quirky margin.
                // We will pick up this quirky margin and pass it through.
                // This deals with the <td><div><p> case.
                // Don't do this for a block that split two inlines though. You do
                // still apply margins in this case.
                setHasMarginBeforeQuirk(true);
        } else
            // The before margin of the container will also discard all the margins it is collapsing with.
            setMustDiscardMarginBefore();
    }

    // Once we find a child with discardMarginBefore all the margins collapsing with us must also discard. 
    if (childDiscardMarginBefore) {
        marginInfo.setDiscardMargin(true);
        marginInfo.clearMargin();
    }

    if (marginInfo.quirkContainer() && marginInfo.atBeforeSideOfBlock() && (posTop - negTop))
        marginInfo.setHasMarginBeforeQuirk(topQuirk);

    LayoutUnit beforeCollapseLogicalTop = logicalHeight();
    LayoutUnit logicalTop = beforeCollapseLogicalTop;
    if (childIsSelfCollapsing) {
        // For a self collapsing block both the before and after margins get discarded. The block doesn't contribute anything to the height of the block.
        // Also, the child's top position equals the logical height of the container.
        if (!childDiscardMarginBefore && !marginInfo.discardMargin()) {
            // This child has no height. We need to compute our
            // position before we collapse the child's margins together,
            // so that we can get an accurate position for the zero-height block.
            LayoutUnit collapsedBeforePos = max(marginInfo.positiveMargin(), childMargins.positiveMarginBefore());
            LayoutUnit collapsedBeforeNeg = max(marginInfo.negativeMargin(), childMargins.negativeMarginBefore());
            marginInfo.setMargin(collapsedBeforePos, collapsedBeforeNeg);
            
            // Now collapse the child's margins together, which means examining our
            // bottom margin values as well. 
            marginInfo.setPositiveMarginIfLarger(childMargins.positiveMarginAfter());
            marginInfo.setNegativeMarginIfLarger(childMargins.negativeMarginAfter());

            if (!marginInfo.canCollapseWithMarginBefore())
                // We need to make sure that the position of the self-collapsing block
                // is correct, since it could have overflowing content
                // that needs to be positioned correctly (e.g., a block that
                // had a specified height of 0 but that actually had subcontent).
                logicalTop = logicalHeight() + collapsedBeforePos - collapsedBeforeNeg;
        }
    } else {
        if (mustSeparateMarginBeforeForChild(child)) {
            ASSERT(!marginInfo.discardMargin() || (marginInfo.discardMargin() && !marginInfo.margin()));
            // If we are at the before side of the block and we collapse, ignore the computed margin
            // and just add the child margin to the container height. This will correctly position
            // the child inside the container.
            LayoutUnit separateMargin = !marginInfo.canCollapseWithMarginBefore() ? marginInfo.margin() : LayoutUnit(0);
            setLogicalHeight(logicalHeight() + separateMargin + marginBeforeForChild(child));
            logicalTop = logicalHeight();
        } else if (!marginInfo.discardMargin() && (!marginInfo.atBeforeSideOfBlock()
            || (!marginInfo.canCollapseMarginBeforeWithChildren()
            && (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !marginInfo.hasMarginBeforeQuirk())))) {
            // We're collapsing with a previous sibling's margins and not
            // with the top of the block.
            setLogicalHeight(logicalHeight() + max(marginInfo.positiveMargin(), posTop) - max(marginInfo.negativeMargin(), negTop));
            logicalTop = logicalHeight();
        }

        marginInfo.setDiscardMargin(childDiscardMarginAfter);
        
        if (!marginInfo.discardMargin()) {
            marginInfo.setPositiveMargin(childMargins.positiveMarginAfter());
            marginInfo.setNegativeMargin(childMargins.negativeMarginAfter());
        } else
            marginInfo.clearMargin();

        if (marginInfo.margin())
            marginInfo.setHasMarginAfterQuirk(hasMarginAfterQuirk(child));
    }
    
    // If margins would pull us past the top of the next page, then we need to pull back and pretend like the margins
    // collapsed into the page edge.
    LayoutState* layoutState = view()->layoutState();
    if (layoutState->isPaginated() && layoutState->pageLogicalHeight() && logicalTop > beforeCollapseLogicalTop
        && hasNextPage(beforeCollapseLogicalTop)) {
        LayoutUnit oldLogicalTop = logicalTop;
        logicalTop = min(logicalTop, nextPageLogicalTop(beforeCollapseLogicalTop));
        setLogicalHeight(logicalHeight() + (logicalTop - oldLogicalTop));
    }

    // If we have collapsed into a previous sibling and so reduced the height of the parent, ensure any floats that now
    // overhang from the previous sibling are added to our parent. If the child's previous sibling itself is a float the child will avoid
    // or clear it anyway, so don't worry about any floating children it may contain.
    LayoutUnit oldLogicalHeight = logicalHeight();
    setLogicalHeight(logicalTop);
    RenderObject* prev = child->previousSibling();
    if (prev && prev->isBlockFlow() && !prev->isFloatingOrOutOfFlowPositioned()) {
        RenderBlock* block = toRenderBlock(prev);
        if (block->containsFloats() && !block->avoidsFloats() && (block->logicalTop() + block->lowestFloatLogicalBottom()) > logicalTop) 
            addOverhangingFloats(block, false);
    }
    setLogicalHeight(oldLogicalHeight);

    return logicalTop;
}

LayoutUnit RenderBlock::clearFloatsIfNeeded(RenderBox* child, MarginInfo& marginInfo, LayoutUnit oldTopPosMargin, LayoutUnit oldTopNegMargin, LayoutUnit yPos)
{
    LayoutUnit heightIncrease = getClearDelta(child, yPos);
    if (!heightIncrease)
        return yPos;

    if (child->isSelfCollapsingBlock()) {
        bool childDiscardMargin = mustDiscardMarginBeforeForChild(child) || mustDiscardMarginAfterForChild(child);

        // For self-collapsing blocks that clear, they can still collapse their
        // margins with following siblings.  Reset the current margins to represent
        // the self-collapsing block's margins only.
        // If DISCARD is specified for -webkit-margin-collapse, reset the margin values.
        if (!childDiscardMargin) {
            MarginValues childMargins = marginValuesForChild(child);
            marginInfo.setPositiveMargin(max(childMargins.positiveMarginBefore(), childMargins.positiveMarginAfter()));
            marginInfo.setNegativeMargin(max(childMargins.negativeMarginBefore(), childMargins.negativeMarginAfter()));
        } else
            marginInfo.clearMargin();
        marginInfo.setDiscardMargin(childDiscardMargin);

        // CSS2.1 states:
        // "If the top and bottom margins of an element with clearance are adjoining, its margins collapse with 
        // the adjoining margins of following siblings but that resulting margin does not collapse with the bottom margin of the parent block."
        // So the parent's bottom margin cannot collapse through this block or any subsequent self-collapsing blocks. Check subsequent siblings
        // for a block with height - if none is found then don't allow the margins to collapse with the parent.
        bool wouldCollapseMarginsWithParent = marginInfo.canCollapseMarginAfterWithChildren();
        for (RenderBox* curr = child->nextSiblingBox(); curr && wouldCollapseMarginsWithParent; curr = curr->nextSiblingBox()) {
            if (!curr->isFloatingOrOutOfFlowPositioned() && !curr->isSelfCollapsingBlock())
                wouldCollapseMarginsWithParent = false;
        }
        if (wouldCollapseMarginsWithParent)
            marginInfo.setCanCollapseMarginAfterWithChildren(false);

        // CSS2.1: "the amount of clearance is set so that clearance + margin-top = [height of float], i.e., clearance = [height of float] - margin-top"
        // Move the top of the child box to the bottom of the float ignoring the child's top margin.
        LayoutUnit collapsedMargin = collapsedMarginBeforeForChild(child);
        setLogicalHeight(child->logicalTop() - collapsedMargin);
        // A negative collapsed margin-top value cancels itself out as it has already been factored into |yPos| above.
        heightIncrease -= max(LayoutUnit(), collapsedMargin);
    } else
        // Increase our height by the amount we had to clear.
        setLogicalHeight(logicalHeight() + heightIncrease);
    
    if (marginInfo.canCollapseWithMarginBefore()) {
        // We can no longer collapse with the top of the block since a clear
        // occurred.  The empty blocks collapse into the cleared block.
        // FIXME: This isn't quite correct.  Need clarification for what to do
        // if the height the cleared block is offset by is smaller than the
        // margins involved.
        setMaxMarginBeforeValues(oldTopPosMargin, oldTopNegMargin);
        marginInfo.setAtBeforeSideOfBlock(false);

        // In case the child discarded the before margin of the block we need to reset the mustDiscardMarginBefore flag to the initial value.
        setMustDiscardMarginBefore(style()->marginBeforeCollapse() == MDISCARD);
    }

    LayoutUnit logicalTop = yPos + heightIncrease;
    // After margin collapsing, one of our floats may now intrude into the child. If the child doesn't contain floats of its own it
    // won't get picked up for relayout even though the logical top estimate was wrong - so add the newly intruding float now.
    if (containsFloats() && child->isRenderBlock() && !toRenderBlock(child)->containsFloats() && !child->avoidsFloats() && lowestFloatLogicalBottom() > logicalTop)
        toRenderBlock(child)->addIntrudingFloats(this, logicalLeftOffsetForContent(), logicalTop);

    return logicalTop;
}

void RenderBlock::marginBeforeEstimateForChild(RenderBox* child, LayoutUnit& positiveMarginBefore, LayoutUnit& negativeMarginBefore, bool& discardMarginBefore) const
{
    // Give up if in quirks mode and we're a body/table cell and the top margin of the child box is quirky.
    // Give up if the child specified -webkit-margin-collapse: separate that prevents collapsing.
    // FIXME: Use writing mode independent accessor for marginBeforeCollapse.
    if ((document()->inQuirksMode() && hasMarginAfterQuirk(child) && (isTableCell() || isBody())) || child->style()->marginBeforeCollapse() == MSEPARATE)
        return;

    // The margins are discarded by a child that specified -webkit-margin-collapse: discard.
    // FIXME: Use writing mode independent accessor for marginBeforeCollapse.
    if (child->style()->marginBeforeCollapse() == MDISCARD) {
        positiveMarginBefore = 0;
        negativeMarginBefore = 0;
        discardMarginBefore = true;
        return;
    }

    LayoutUnit beforeChildMargin = marginBeforeForChild(child);
    positiveMarginBefore = max(positiveMarginBefore, beforeChildMargin);
    negativeMarginBefore = max(negativeMarginBefore, -beforeChildMargin);

    if (!child->isRenderBlock())
        return;
    
    RenderBlock* childBlock = toRenderBlock(child);
    if (childBlock->childrenInline() || childBlock->isWritingModeRoot())
        return;

    MarginInfo childMarginInfo(childBlock, childBlock->borderAndPaddingBefore(), childBlock->borderAndPaddingAfter());
    if (!childMarginInfo.canCollapseMarginBeforeWithChildren())
        return;

    RenderBox* grandchildBox = childBlock->firstChildBox();
    for ( ; grandchildBox; grandchildBox = grandchildBox->nextSiblingBox()) {
        if (!grandchildBox->isFloatingOrOutOfFlowPositioned())
            break;
    }
    
    // Give up if there is clearance on the box, since it probably won't collapse into us.
    if (!grandchildBox || grandchildBox->style()->clear() != CNONE)
        return;

    // Make sure to update the block margins now for the grandchild box so that we're looking at current values.
    if (grandchildBox->needsLayout()) {
        grandchildBox->computeAndSetBlockDirectionMargins(this);
        if (grandchildBox->isRenderBlock()) {
            RenderBlock* grandchildBlock = toRenderBlock(grandchildBox);
            grandchildBlock->setHasMarginBeforeQuirk(grandchildBox->style()->hasMarginBeforeQuirk());
            grandchildBlock->setHasMarginAfterQuirk(grandchildBox->style()->hasMarginAfterQuirk());
        }
    }

    // Collapse the margin of the grandchild box with our own to produce an estimate.
    childBlock->marginBeforeEstimateForChild(grandchildBox, positiveMarginBefore, negativeMarginBefore, discardMarginBefore);
}

LayoutUnit RenderBlock::estimateLogicalTopPosition(RenderBox* child, const MarginInfo& marginInfo, LayoutUnit& estimateWithoutPagination)
{
    // FIXME: We need to eliminate the estimation of vertical position, because when it's wrong we sometimes trigger a pathological
    // relayout if there are intruding floats.
    LayoutUnit logicalTopEstimate = logicalHeight();
    if (!marginInfo.canCollapseWithMarginBefore()) {
        LayoutUnit positiveMarginBefore = 0;
        LayoutUnit negativeMarginBefore = 0;
        bool discardMarginBefore = false;
        if (child->selfNeedsLayout()) {
            // Try to do a basic estimation of how the collapse is going to go.
            marginBeforeEstimateForChild(child, positiveMarginBefore, negativeMarginBefore, discardMarginBefore);
        } else {
            // Use the cached collapsed margin values from a previous layout. Most of the time they
            // will be right.
            MarginValues marginValues = marginValuesForChild(child);
            positiveMarginBefore = max(positiveMarginBefore, marginValues.positiveMarginBefore());
            negativeMarginBefore = max(negativeMarginBefore, marginValues.negativeMarginBefore());
            discardMarginBefore = mustDiscardMarginBeforeForChild(child);
        }

        // Collapse the result with our current margins.
        if (!discardMarginBefore)
            logicalTopEstimate += max(marginInfo.positiveMargin(), positiveMarginBefore) - max(marginInfo.negativeMargin(), negativeMarginBefore);
    }

    // Adjust logicalTopEstimate down to the next page if the margins are so large that we don't fit on the current
    // page.
    LayoutState* layoutState = view()->layoutState();
    if (layoutState->isPaginated() && layoutState->pageLogicalHeight() && logicalTopEstimate > logicalHeight()
        && hasNextPage(logicalHeight()))
        logicalTopEstimate = min(logicalTopEstimate, nextPageLogicalTop(logicalHeight()));

    logicalTopEstimate += getClearDelta(child, logicalTopEstimate);
    
    estimateWithoutPagination = logicalTopEstimate;

    if (layoutState->isPaginated()) {
        // If the object has a page or column break value of "before", then we should shift to the top of the next page.
        logicalTopEstimate = applyBeforeBreak(child, logicalTopEstimate);
    
        // For replaced elements and scrolled elements, we want to shift them to the next page if they don't fit on the current one.
        logicalTopEstimate = adjustForUnsplittableChild(child, logicalTopEstimate);
        
        if (!child->selfNeedsLayout() && child->isRenderBlock())
            logicalTopEstimate += toRenderBlock(child)->paginationStrut();
    }

    return logicalTopEstimate;
}

LayoutUnit RenderBlock::computeStartPositionDeltaForChildAvoidingFloats(const RenderBox* child, LayoutUnit childMarginStart, RenderRegion* region)
{
    LayoutUnit startPosition = startOffsetForContent(region);

    // Add in our start margin.
    LayoutUnit oldPosition = startPosition + childMarginStart;
    LayoutUnit newPosition = oldPosition;

    LayoutUnit blockOffset = logicalTopForChild(child);
    if (region)
        blockOffset = max(blockOffset, blockOffset + (region->logicalTopForFlowThreadContent() - offsetFromLogicalTopOfFirstPage()));

    LayoutUnit startOff = startOffsetForLine(blockOffset, false, region, logicalHeightForChild(child));

    if (style()->textAlign() != WEBKIT_CENTER && !child->style()->marginStartUsing(style()).isAuto()) {
        if (childMarginStart < 0)
            startOff += childMarginStart;
        newPosition = max(newPosition, startOff); // Let the float sit in the child's margin if it can fit.
    } else if (startOff != startPosition)
        newPosition = startOff + childMarginStart;

    return newPosition - oldPosition;
}

void RenderBlock::determineLogicalLeftPositionForChild(RenderBox* child, ApplyLayoutDeltaMode applyDelta)
{
    LayoutUnit startPosition = borderStart() + paddingStart();
    if (style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
        startPosition -= verticalScrollbarWidth();
    LayoutUnit totalAvailableLogicalWidth = borderAndPaddingLogicalWidth() + availableLogicalWidth();

    // Add in our start margin.
    LayoutUnit childMarginStart = marginStartForChild(child);
    LayoutUnit newPosition = startPosition + childMarginStart;
        
    // Some objects (e.g., tables, horizontal rules, overflow:auto blocks) avoid floats.  They need
    // to shift over as necessary to dodge any floats that might get in the way.
    if (child->avoidsFloats() && containsFloats() && !flowThreadContainingBlock())
        newPosition += computeStartPositionDeltaForChildAvoidingFloats(child, marginStartForChild(child));

    setLogicalLeftForChild(child, style()->isLeftToRightDirection() ? newPosition : totalAvailableLogicalWidth - newPosition - logicalWidthForChild(child), applyDelta);
}

void RenderBlock::setCollapsedBottomMargin(const MarginInfo& marginInfo)
{
    if (marginInfo.canCollapseWithMarginAfter() && !marginInfo.canCollapseWithMarginBefore()) {
        // Update the after side margin of the container to discard if the after margin of the last child also discards and we collapse with it.
        // Don't update the max margin values because we won't need them anyway.
        if (marginInfo.discardMargin()) {
            setMustDiscardMarginAfter();
            return;
        }

        // Update our max pos/neg bottom margins, since we collapsed our bottom margins
        // with our children.
        setMaxMarginAfterValues(max(maxPositiveMarginAfter(), marginInfo.positiveMargin()), max(maxNegativeMarginAfter(), marginInfo.negativeMargin()));

        if (!marginInfo.hasMarginAfterQuirk())
            setHasMarginAfterQuirk(false);

        if (marginInfo.hasMarginAfterQuirk() && !marginAfter())
            // We have no bottom margin and our last child has a quirky margin.
            // We will pick up this quirky margin and pass it through.
            // This deals with the <td><div><p> case.
            setHasMarginAfterQuirk(true);
    }
}

void RenderBlock::handleAfterSideOfBlock(LayoutUnit beforeSide, LayoutUnit afterSide, MarginInfo& marginInfo)
{
    marginInfo.setAtAfterSideOfBlock(true);

    // If we can't collapse with children then go ahead and add in the bottom margin.
    if (!marginInfo.discardMargin() && (!marginInfo.canCollapseWithMarginAfter() && !marginInfo.canCollapseWithMarginBefore()
        && (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !marginInfo.hasMarginAfterQuirk())))
        setLogicalHeight(logicalHeight() + marginInfo.margin());
        
    // Now add in our bottom border/padding.
    setLogicalHeight(logicalHeight() + afterSide);

    // Negative margins can cause our height to shrink below our minimal height (border/padding).
    // If this happens, ensure that the computed height is increased to the minimal height.
    setLogicalHeight(max(logicalHeight(), beforeSide + afterSide));

    // Update our bottom collapsed margin info.
    setCollapsedBottomMargin(marginInfo);
}

void RenderBlock::setLogicalLeftForChild(RenderBox* child, LayoutUnit logicalLeft, ApplyLayoutDeltaMode applyDelta)
{
    if (isHorizontalWritingMode()) {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(LayoutSize(child->x() - logicalLeft, 0));
        child->setX(logicalLeft);
    } else {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(LayoutSize(0, child->y() - logicalLeft));
        child->setY(logicalLeft);
    }
}

void RenderBlock::setLogicalTopForChild(RenderBox* child, LayoutUnit logicalTop, ApplyLayoutDeltaMode applyDelta)
{
    if (isHorizontalWritingMode()) {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(LayoutSize(0, child->y() - logicalTop));
        child->setY(logicalTop);
    } else {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(LayoutSize(child->x() - logicalTop, 0));
        child->setX(logicalTop);
    }
}

void RenderBlock::updateBlockChildDirtyBitsBeforeLayout(bool relayoutChildren, RenderBox* child)
{
    // FIXME: Technically percentage height objects only need a relayout if their percentage isn't going to be turned into
    // an auto value. Add a method to determine this, so that we can avoid the relayout.
    if (relayoutChildren || (child->hasRelativeLogicalHeight() && !isRenderView()) || child->hasViewportPercentageLogicalHeight())
        child->setChildNeedsLayout(true, MarkOnlyThis);

    // If relayoutChildren is set and the child has percentage padding or an embedded content box, we also need to invalidate the childs pref widths.
    if (relayoutChildren && child->needsPreferredWidthsRecalculation())
        child->setPreferredLogicalWidthsDirty(true, MarkOnlyThis);
}

void RenderBlock::dirtyForLayoutFromPercentageHeightDescendants()
{
    if (!gPercentHeightDescendantsMap)
        return;

    TrackedRendererListHashSet* descendants = gPercentHeightDescendantsMap->get(this);
    if (!descendants)
        return;
    
    TrackedRendererListHashSet::iterator end = descendants->end();
    for (TrackedRendererListHashSet::iterator it = descendants->begin(); it != end; ++it) {
        RenderBox* box = *it;
        while (box != this) {
            if (box->normalChildNeedsLayout())
                break;
            box->setChildNeedsLayout(true, MarkOnlyThis);
            
            // If the width of an image is affected by the height of a child (e.g., an image with an aspect ratio),
            // then we have to dirty preferred widths, since even enclosing blocks can become dirty as a result.
            // (A horizontal flexbox that contains an inline image wrapped in an anonymous block for example.)
            if (box->hasAspectRatio()) 
                box->setPreferredLogicalWidthsDirty(true);
            
            box = box->containingBlock();
            ASSERT(box);
            if (!box)
                break;
        }
    }
}

void RenderBlock::layoutBlockChildren(bool relayoutChildren, LayoutUnit& maxFloatLogicalBottom)
{
    dirtyForLayoutFromPercentageHeightDescendants();

    LayoutUnit beforeEdge = borderAndPaddingBefore();
    LayoutUnit afterEdge = borderAndPaddingAfter() + scrollbarLogicalHeight();

    setLogicalHeight(beforeEdge);
    
    // Lay out our hypothetical grid line as though it occurs at the top of the block.
    if (view()->layoutState()->lineGrid() == this)
        layoutLineGridBox();

    // The margin struct caches all our current margin collapsing state.  The compact struct caches state when we encounter compacts,
    MarginInfo marginInfo(this, beforeEdge, afterEdge);

    // Fieldsets need to find their legend and position it inside the border of the object.
    // The legend then gets skipped during normal layout.  The same is true for ruby text.
    // It doesn't get included in the normal layout process but is instead skipped.
    RenderObject* childToExclude = layoutSpecialExcludedChild(relayoutChildren);

    LayoutUnit previousFloatLogicalBottom = 0;
    maxFloatLogicalBottom = 0;

    RenderBox* next = firstChildBox();

    while (next) {
        RenderBox* child = next;
        next = child->nextSiblingBox();

        if (childToExclude == child)
            continue; // Skip this child, since it will be positioned by the specialized subclass (fieldsets and ruby runs).

        updateBlockChildDirtyBitsBeforeLayout(relayoutChildren, child);

        if (child->isOutOfFlowPositioned()) {
            child->containingBlock()->insertPositionedObject(child);
            adjustPositionedBlock(child, marginInfo);
            continue;
        }
        if (child->isFloating()) {
            insertFloatingObject(child);
            adjustFloatingBlock(marginInfo);
            continue;
        }

        // Lay out the child.
        layoutBlockChild(child, marginInfo, previousFloatLogicalBottom, maxFloatLogicalBottom);
    }
    
    // Now do the handling of the bottom of the block, adding in our bottom border/padding and
    // determining the correct collapsed bottom margin information.
    handleAfterSideOfBlock(beforeEdge, afterEdge, marginInfo);
}

void RenderBlock::layoutBlockChild(RenderBox* child, MarginInfo& marginInfo, LayoutUnit& previousFloatLogicalBottom, LayoutUnit& maxFloatLogicalBottom)
{
    LayoutUnit oldPosMarginBefore = maxPositiveMarginBefore();
    LayoutUnit oldNegMarginBefore = maxNegativeMarginBefore();

    // The child is a normal flow object.  Compute the margins we will use for collapsing now.
    child->computeAndSetBlockDirectionMargins(this);

    // Try to guess our correct logical top position.  In most cases this guess will
    // be correct.  Only if we're wrong (when we compute the real logical top position)
    // will we have to potentially relayout.
    LayoutUnit estimateWithoutPagination;
    LayoutUnit logicalTopEstimate = estimateLogicalTopPosition(child, marginInfo, estimateWithoutPagination);

    // Cache our old rect so that we can dirty the proper repaint rects if the child moves.
    LayoutRect oldRect = child->frameRect();
    LayoutUnit oldLogicalTop = logicalTopForChild(child);

#if !ASSERT_DISABLED
    LayoutSize oldLayoutDelta = view()->layoutDelta();
#endif
    // Go ahead and position the child as though it didn't collapse with the top.
    setLogicalTopForChild(child, logicalTopEstimate, ApplyLayoutDelta);

    RenderBlock* childRenderBlock = child->isRenderBlock() ? toRenderBlock(child) : 0;
    bool markDescendantsWithFloats = false;
    if (logicalTopEstimate != oldLogicalTop && !child->avoidsFloats() && childRenderBlock && childRenderBlock->containsFloats())
        markDescendantsWithFloats = true;
#if ENABLE(SUBPIXEL_LAYOUT)
    else if (UNLIKELY(logicalTopEstimate.mightBeSaturated()))
        // logicalTopEstimate, returned by estimateLogicalTopPosition, might be saturated for
        // very large elements. If it does the comparison with oldLogicalTop might yield a
        // false negative as adding and removing margins, borders etc from a saturated number
        // might yield incorrect results. If this is the case always mark for layout.
        markDescendantsWithFloats = true;
#endif
    else if (!child->avoidsFloats() || child->shrinkToAvoidFloats()) {
        // If an element might be affected by the presence of floats, then always mark it for
        // layout.
        LayoutUnit fb = max(previousFloatLogicalBottom, lowestFloatLogicalBottom());
        if (fb > logicalTopEstimate)
            markDescendantsWithFloats = true;
    }

    if (childRenderBlock) {
        if (markDescendantsWithFloats)
            childRenderBlock->markAllDescendantsWithFloatsForLayout();
        if (!child->isWritingModeRoot())
            previousFloatLogicalBottom = max(previousFloatLogicalBottom, oldLogicalTop + childRenderBlock->lowestFloatLogicalBottom());
    }

    if (!child->needsLayout())
        child->markForPaginationRelayoutIfNeeded();

    bool childHadLayout = child->everHadLayout();
    bool childNeededLayout = child->needsLayout();
    if (childNeededLayout)
        child->layout();

    // Cache if we are at the top of the block right now.
    bool atBeforeSideOfBlock = marginInfo.atBeforeSideOfBlock();

    // Now determine the correct ypos based off examination of collapsing margin
    // values.
    LayoutUnit logicalTopBeforeClear = collapseMargins(child, marginInfo);

    // Now check for clear.
    LayoutUnit logicalTopAfterClear = clearFloatsIfNeeded(child, marginInfo, oldPosMarginBefore, oldNegMarginBefore, logicalTopBeforeClear);
    
    bool paginated = view()->layoutState()->isPaginated();
    if (paginated)
        logicalTopAfterClear = adjustBlockChildForPagination(logicalTopAfterClear, estimateWithoutPagination, child,
            atBeforeSideOfBlock && logicalTopBeforeClear == logicalTopAfterClear);

    setLogicalTopForChild(child, logicalTopAfterClear, ApplyLayoutDelta);

    // Now we have a final top position.  See if it really does end up being different from our estimate.
    // clearFloatsIfNeeded can also mark the child as needing a layout even though we didn't move. This happens
    // when collapseMargins dynamically adds overhanging floats because of a child with negative margins.
    if (logicalTopAfterClear != logicalTopEstimate || child->needsLayout() || (paginated && childRenderBlock && childRenderBlock->shouldBreakAtLineToAvoidWidow())) {
        if (child->shrinkToAvoidFloats()) {
            // The child's width depends on the line width.
            // When the child shifts to clear an item, its width can
            // change (because it has more available line width).
            // So go ahead and mark the item as dirty.
            child->setChildNeedsLayout(true, MarkOnlyThis);
        }
        
        if (childRenderBlock) {
            if (!child->avoidsFloats() && childRenderBlock->containsFloats())
                childRenderBlock->markAllDescendantsWithFloatsForLayout();
            if (!child->needsLayout())
                child->markForPaginationRelayoutIfNeeded();
        }

        // Our guess was wrong. Make the child lay itself out again.
        child->layoutIfNeeded();
    }

    // We are no longer at the top of the block if we encounter a non-empty child.  
    // This has to be done after checking for clear, so that margins can be reset if a clear occurred.
    if (marginInfo.atBeforeSideOfBlock() && !child->isSelfCollapsingBlock())
        marginInfo.setAtBeforeSideOfBlock(false);

    // Now place the child in the correct left position
    determineLogicalLeftPositionForChild(child, ApplyLayoutDelta);

    // Update our height now that the child has been placed in the correct position.
    setLogicalHeight(logicalHeight() + logicalHeightForChild(child));
    if (mustSeparateMarginAfterForChild(child)) {
        setLogicalHeight(logicalHeight() + marginAfterForChild(child));
        marginInfo.clearMargin();
    }
    // If the child has overhanging floats that intrude into following siblings (or possibly out
    // of this block), then the parent gets notified of the floats now.
    if (childRenderBlock && childRenderBlock->containsFloats())
        maxFloatLogicalBottom = max(maxFloatLogicalBottom, addOverhangingFloats(toRenderBlock(child), !childNeededLayout));

    LayoutSize childOffset = child->location() - oldRect.location();
    if (childOffset.width() || childOffset.height()) {
        view()->addLayoutDelta(childOffset);

        // If the child moved, we have to repaint it as well as any floating/positioned
        // descendants.  An exception is if we need a layout.  In this case, we know we're going to
        // repaint ourselves (and the child) anyway.
        if (childHadLayout && !selfNeedsLayout() && child->checkForRepaintDuringLayout())
            child->repaintDuringLayoutIfMoved(oldRect);
    }

    if (!childHadLayout && child->checkForRepaintDuringLayout()) {
        child->repaint();
        child->repaintOverhangingFloats(true);
    }

    if (paginated) {
        // Check for an after page/column break.
        LayoutUnit newHeight = applyAfterBreak(child, logicalHeight(), marginInfo);
        if (newHeight != height())
            setLogicalHeight(newHeight);
    }

    ASSERT(view()->layoutDeltaMatches(oldLayoutDelta));
}

void RenderBlock::simplifiedNormalFlowLayout()
{
    if (childrenInline()) {
        ListHashSet<RootInlineBox*> lineBoxes;
        for (InlineWalker walker(this); !walker.atEnd(); walker.advance()) {
            RenderObject* o = walker.current();
            if (!o->isOutOfFlowPositioned() && (o->isReplaced() || o->isFloating())) {
                o->layoutIfNeeded();
                if (toRenderBox(o)->inlineBoxWrapper()) {
                    RootInlineBox* box = toRenderBox(o)->inlineBoxWrapper()->root();
                    lineBoxes.add(box);
                }
            } else if (o->isText() || (o->isRenderInline() && !walker.atEndOfInline()))
                o->setNeedsLayout(false);
        }

        // FIXME: Glyph overflow will get lost in this case, but not really a big deal.
        GlyphOverflowAndFallbackFontsMap textBoxDataMap;                  
        for (ListHashSet<RootInlineBox*>::const_iterator it = lineBoxes.begin(); it != lineBoxes.end(); ++it) {
            RootInlineBox* box = *it;
            box->computeOverflow(box->lineTop(), box->lineBottom(), textBoxDataMap);
        }
    } else {
        for (RenderBox* box = firstChildBox(); box; box = box->nextSiblingBox()) {
            if (!box->isOutOfFlowPositioned())
                box->layoutIfNeeded();
        }
    }
}

bool RenderBlock::simplifiedLayout()
{
    if ((!posChildNeedsLayout() && !needsSimplifiedNormalFlowLayout()) || normalChildNeedsLayout() || selfNeedsLayout())
        return false;

    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasColumns() || hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());
    
    if (needsPositionedMovementLayout() && !tryLayoutDoingPositionedMovementOnly())
        return false;

    // Lay out positioned descendants or objects that just need to recompute overflow.
    if (needsSimplifiedNormalFlowLayout())
        simplifiedNormalFlowLayout();

    // Lay out our positioned objects if our positioned child bit is set.
    // Also, if an absolute position element inside a relative positioned container moves, and the absolute element has a fixed position
    // child, neither the fixed element nor its container learn of the movement since posChildNeedsLayout() is only marked as far as the 
    // relative positioned container. So if we can have fixed pos objects in our positioned objects list check if any of them
    // are statically positioned and thus need to move with their absolute ancestors.
    bool canContainFixedPosObjects = canContainFixedPositionObjects();
    if (posChildNeedsLayout() || canContainFixedPosObjects)
        layoutPositionedObjects(false, !posChildNeedsLayout() && canContainFixedPosObjects);

    // Recompute our overflow information.
    // FIXME: We could do better here by computing a temporary overflow object from layoutPositionedObjects and only
    // updating our overflow if we either used to have overflow or if the new temporary object has overflow.
    // For now just always recompute overflow.  This is no worse performance-wise than the old code that called rightmostPosition and
    // lowestPosition on every relayout so it's not a regression.
    // computeOverflow expects the bottom edge before we clamp our height. Since this information isn't available during
    // simplifiedLayout, we cache the value in m_overflow.
    LayoutUnit oldClientAfterEdge = hasRenderOverflow() ? m_overflow->layoutClientAfterEdge() : clientLogicalBottom();
    computeOverflow(oldClientAfterEdge, true);

    statePusher.pop();
    
    updateLayerTransform();

    updateScrollInfoAfterLayout();

    setNeedsLayout(false);
    return true;
}

void RenderBlock::markFixedPositionObjectForLayoutIfNeeded(RenderObject* child)
{
    if (child->style()->position() != FixedPosition)
        return;

    bool hasStaticBlockPosition = child->style()->hasStaticBlockPosition(isHorizontalWritingMode());
    bool hasStaticInlinePosition = child->style()->hasStaticInlinePosition(isHorizontalWritingMode());
    if (!hasStaticBlockPosition && !hasStaticInlinePosition)
        return;

    RenderObject* o = child->parent();
    while (o && !o->isRenderView() && o->style()->position() != AbsolutePosition)
        o = o->parent();
    if (o->style()->position() != AbsolutePosition)
        return;

    RenderBox* box = toRenderBox(child);
    if (hasStaticInlinePosition) {
        LogicalExtentComputedValues computedValues;
        box->computeLogicalWidthInRegion(computedValues);
        LayoutUnit newLeft = computedValues.m_position;
        if (newLeft != box->logicalLeft())
            child->setChildNeedsLayout(true, MarkOnlyThis);
    } else if (hasStaticBlockPosition) {
        LayoutUnit oldTop = box->logicalTop();
        box->updateLogicalHeight();
        if (box->logicalTop() != oldTop)
            child->setChildNeedsLayout(true, MarkOnlyThis);
    }
}

void RenderBlock::layoutPositionedObjects(bool relayoutChildren, bool fixedPositionObjectsOnly)
{
    TrackedRendererListHashSet* positionedDescendants = positionedObjects();
    if (!positionedDescendants)
        return;
        
    if (hasColumns())
        view()->layoutState()->clearPaginationInformation(); // Positioned objects are not part of the column flow, so they don't paginate with the columns.

    RenderBox* r;
    TrackedRendererListHashSet::iterator end = positionedDescendants->end();
    for (TrackedRendererListHashSet::iterator it = positionedDescendants->begin(); it != end; ++it) {
        r = *it;

        // A fixed position element with an absolute positioned ancestor has no way of knowing if the latter has changed position. So
        // if this is a fixed position element, mark it for layout if it has an abspos ancestor and needs to move with that ancestor, i.e. 
        // it has static position.
        markFixedPositionObjectForLayoutIfNeeded(r);
        if (fixedPositionObjectsOnly) {
            r->layoutIfNeeded();
            continue;
        }

        // When a non-positioned block element moves, it may have positioned children that are implicitly positioned relative to the
        // non-positioned block.  Rather than trying to detect all of these movement cases, we just always lay out positioned
        // objects that are positioned implicitly like this.  Such objects are rare, and so in typical DHTML menu usage (where everything is
        // positioned explicitly) this should not incur a performance penalty.
        if (relayoutChildren || (r->style()->hasStaticBlockPosition(isHorizontalWritingMode()) && r->parent() != this))
            r->setChildNeedsLayout(true, MarkOnlyThis);
            
        // If relayoutChildren is set and the child has percentage padding or an embedded content box, we also need to invalidate the childs pref widths.
        if (relayoutChildren && r->needsPreferredWidthsRecalculation())
            r->setPreferredLogicalWidthsDirty(true, MarkOnlyThis);
        
        if (!r->needsLayout())
            r->markForPaginationRelayoutIfNeeded();
        
        // We don't have to do a full layout.  We just have to update our position. Try that first. If we have shrink-to-fit width
        // and we hit the available width constraint, the layoutIfNeeded() will catch it and do a full layout.
        if (r->needsPositionedMovementLayoutOnly() && r->tryLayoutDoingPositionedMovementOnly())
            r->setNeedsLayout(false);
            
        // If we are paginated or in a line grid, go ahead and compute a vertical position for our object now.
        // If it's wrong we'll lay out again.
        LayoutUnit oldLogicalTop = 0;
        bool needsBlockDirectionLocationSetBeforeLayout = r->needsLayout() && view()->layoutState()->needsBlockDirectionLocationSetBeforeLayout(); 
        if (needsBlockDirectionLocationSetBeforeLayout) {
            if (isHorizontalWritingMode() == r->isHorizontalWritingMode())
                r->updateLogicalHeight();
            else
                r->updateLogicalWidth();
            oldLogicalTop = logicalTopForChild(r);
        }
        
        r->layoutIfNeeded();

        // Lay out again if our estimate was wrong.
        if (needsBlockDirectionLocationSetBeforeLayout && logicalTopForChild(r) != oldLogicalTop) {
            r->setChildNeedsLayout(true, MarkOnlyThis);
            r->layoutIfNeeded();
        }
    }
    
    if (hasColumns())
        view()->layoutState()->m_columnInfo = columnInfo(); // FIXME: Kind of gross. We just put this back into the layout state so that pop() will work.
}

void RenderBlock::markPositionedObjectsForLayout()
{
    TrackedRendererListHashSet* positionedDescendants = positionedObjects();
    if (positionedDescendants) {
        RenderBox* r;
        TrackedRendererListHashSet::iterator end = positionedDescendants->end();
        for (TrackedRendererListHashSet::iterator it = positionedDescendants->begin(); it != end; ++it) {
            r = *it;
            r->setChildNeedsLayout(true);
        }
    }
}

void RenderBlock::markForPaginationRelayoutIfNeeded()
{
    ASSERT(!needsLayout());
    if (needsLayout())
        return;

    if (view()->layoutState()->pageLogicalHeightChanged() || (view()->layoutState()->pageLogicalHeight() && view()->layoutState()->pageLogicalOffset(this, logicalTop()) != pageLogicalOffset()) || shouldBreakAtLineToAvoidWidow())
        setChildNeedsLayout(true, MarkOnlyThis);
}

void RenderBlock::repaintOverhangingFloats(bool paintAllDescendants)
{
    // Repaint any overhanging floats (if we know we're the one to paint them).
    // Otherwise, bail out.
    if (!hasOverhangingFloats())
        return;

    // FIXME: Avoid disabling LayoutState. At the very least, don't disable it for floats originating
    // in this block. Better yet would be to push extra state for the containers of other floats.
    LayoutStateDisabler layoutStateDisabler(view());
    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        // Only repaint the object if it is overhanging, is not in its own layer, and
        // is our responsibility to paint (m_shouldPaint is set). When paintAllDescendants is true, the latter
        // condition is replaced with being a descendant of us.
        if (logicalBottomForFloat(r) > logicalHeight() && ((paintAllDescendants && r->m_renderer->isDescendantOf(this)) || r->shouldPaint()) && !r->m_renderer->hasSelfPaintingLayer()) {
            r->m_renderer->repaint();
            r->m_renderer->repaintOverhangingFloats(false);
        }
    }
}
 
void RenderBlock::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutPoint adjustedPaintOffset = paintOffset + location();
    
    PaintPhase phase = paintInfo.phase;

    // Check if we need to do anything at all.
    // FIXME: Could eliminate the isRoot() check if we fix background painting so that the RenderView
    // paints the root's background.
    if (!isRoot()) {
        LayoutRect overflowBox = overflowRectForPaintRejection();
        flipForWritingMode(overflowBox);
        overflowBox.inflate(maximalOutlineSize(paintInfo.phase));
        overflowBox.moveBy(adjustedPaintOffset);
        if (!overflowBox.intersects(paintInfo.rect))
            return;
    }

    bool pushedClip = pushContentsClip(paintInfo, adjustedPaintOffset);
    paintObject(paintInfo, adjustedPaintOffset);
    if (pushedClip)
        popContentsClip(paintInfo, phase, adjustedPaintOffset);

    // Our scrollbar widgets paint exactly when we tell them to, so that they work properly with
    // z-index.  We paint after we painted the background/border, so that the scrollbars will
    // sit above the background/border.
    if (hasOverflowClip() && style()->visibility() == VISIBLE && (phase == PaintPhaseBlockBackground || phase == PaintPhaseChildBlockBackground) && paintInfo.shouldPaintWithinRoot(this) && !paintInfo.paintRootBackgroundOnly())
        layer()->paintOverflowControls(paintInfo.context, roundedIntPoint(adjustedPaintOffset), paintInfo.rect);
}

void RenderBlock::paintColumnRules(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.context->paintingDisabled())
        return;

    const Color& ruleColor = style()->visitedDependentColor(CSSPropertyWebkitColumnRuleColor);
    bool ruleTransparent = style()->columnRuleIsTransparent();
    EBorderStyle ruleStyle = style()->columnRuleStyle();
    LayoutUnit ruleThickness = style()->columnRuleWidth();
    LayoutUnit colGap = columnGap();
    bool renderRule = ruleStyle > BHIDDEN && !ruleTransparent;
    if (!renderRule)
        return;

    ColumnInfo* colInfo = columnInfo();
    unsigned colCount = columnCount(colInfo);

    bool antialias = shouldAntialiasLines(paintInfo.context);

    if (colInfo->progressionAxis() == ColumnInfo::InlineAxis) {
        bool leftToRight = style()->isLeftToRightDirection() ^ colInfo->progressionIsReversed();
        LayoutUnit currLogicalLeftOffset = leftToRight ? LayoutUnit() : contentLogicalWidth();
        LayoutUnit ruleAdd = logicalLeftOffsetForContent();
        LayoutUnit ruleLogicalLeft = leftToRight ? LayoutUnit() : contentLogicalWidth();
        LayoutUnit inlineDirectionSize = colInfo->desiredColumnWidth();
        BoxSide boxSide = isHorizontalWritingMode()
            ? leftToRight ? BSLeft : BSRight
            : leftToRight ? BSTop : BSBottom;

        for (unsigned i = 0; i < colCount; i++) {
            // Move to the next position.
            if (leftToRight) {
                ruleLogicalLeft += inlineDirectionSize + colGap / 2;
                currLogicalLeftOffset += inlineDirectionSize + colGap;
            } else {
                ruleLogicalLeft -= (inlineDirectionSize + colGap / 2);
                currLogicalLeftOffset -= (inlineDirectionSize + colGap);
            }
           
            // Now paint the column rule.
            if (i < colCount - 1) {
                LayoutUnit ruleLeft = isHorizontalWritingMode() ? paintOffset.x() + ruleLogicalLeft - ruleThickness / 2 + ruleAdd : paintOffset.x() + borderLeft() + paddingLeft();
                LayoutUnit ruleRight = isHorizontalWritingMode() ? ruleLeft + ruleThickness : ruleLeft + contentWidth();
                LayoutUnit ruleTop = isHorizontalWritingMode() ? paintOffset.y() + borderTop() + paddingTop() : paintOffset.y() + ruleLogicalLeft - ruleThickness / 2 + ruleAdd;
                LayoutUnit ruleBottom = isHorizontalWritingMode() ? ruleTop + contentHeight() : ruleTop + ruleThickness;
                IntRect pixelSnappedRuleRect = pixelSnappedIntRectFromEdges(ruleLeft, ruleTop, ruleRight, ruleBottom);
                drawLineForBoxSide(paintInfo.context, pixelSnappedRuleRect.x(), pixelSnappedRuleRect.y(), pixelSnappedRuleRect.maxX(), pixelSnappedRuleRect.maxY(), boxSide, ruleColor, ruleStyle, 0, 0, antialias);
            }
            
            ruleLogicalLeft = currLogicalLeftOffset;
        }
    } else {
        bool topToBottom = !style()->isFlippedBlocksWritingMode() ^ colInfo->progressionIsReversed();
        LayoutUnit ruleLeft = isHorizontalWritingMode()
            ? borderLeft() + paddingLeft()
            : colGap / 2 - colGap - ruleThickness / 2 + (!colInfo->progressionIsReversed() ? borderAndPaddingBefore() : borderAndPaddingAfter());
        LayoutUnit ruleWidth = isHorizontalWritingMode() ? contentWidth() : ruleThickness;
        LayoutUnit ruleTop = isHorizontalWritingMode()
            ? colGap / 2 - colGap - ruleThickness / 2 + (!colInfo->progressionIsReversed() ? borderAndPaddingBefore() : borderAndPaddingAfter())
            : borderStart() + paddingStart();
        LayoutUnit ruleHeight = isHorizontalWritingMode() ? ruleThickness : contentHeight();
        LayoutRect ruleRect(ruleLeft, ruleTop, ruleWidth, ruleHeight);

        if (!topToBottom) {
            if (isHorizontalWritingMode())
                ruleRect.setY(height() - ruleRect.maxY());
            else
                ruleRect.setX(width() - ruleRect.maxX());
        }

        ruleRect.moveBy(paintOffset);

        BoxSide boxSide = isHorizontalWritingMode()
            ? topToBottom ? BSTop : BSBottom
            : topToBottom ? BSLeft : BSRight;

        LayoutSize step(0, topToBottom ? colInfo->columnHeight() + colGap : -(colInfo->columnHeight() + colGap));
        if (!isHorizontalWritingMode())
            step = step.transposedSize();

        for (unsigned i = 1; i < colCount; i++) {
            ruleRect.move(step);
            IntRect pixelSnappedRuleRect = pixelSnappedIntRect(ruleRect);
            drawLineForBoxSide(paintInfo.context, pixelSnappedRuleRect.x(), pixelSnappedRuleRect.y(), pixelSnappedRuleRect.maxX(), pixelSnappedRuleRect.maxY(), boxSide, ruleColor, ruleStyle, 0, 0, antialias);
        }
    }
}

LayoutUnit RenderBlock::initialBlockOffsetForPainting() const
{
    ColumnInfo* colInfo = columnInfo();
    LayoutUnit result = 0;
    if (colInfo->progressionAxis() == ColumnInfo::BlockAxis && colInfo->progressionIsReversed()) {
        LayoutRect colRect = columnRectAt(colInfo, 0);
        result = isHorizontalWritingMode() ? colRect.y() : colRect.x();
        result -= borderAndPaddingBefore();
        if (style()->isFlippedBlocksWritingMode())
            result = -result;
    }
    return result;
}
    
LayoutUnit RenderBlock::blockDeltaForPaintingNextColumn() const
{
    ColumnInfo* colInfo = columnInfo();
    LayoutUnit blockDelta = -colInfo->columnHeight();
    LayoutUnit colGap = columnGap();
    if (colInfo->progressionAxis() == ColumnInfo::BlockAxis) {
        if (!colInfo->progressionIsReversed())
            blockDelta = colGap;
        else
            blockDelta -= (colInfo->columnHeight() + colGap);
    }
    if (style()->isFlippedBlocksWritingMode())
        blockDelta = -blockDelta;
    return blockDelta;
}

void RenderBlock::paintColumnContents(PaintInfo& paintInfo, const LayoutPoint& paintOffset, bool paintingFloats)
{
    // We need to do multiple passes, breaking up our child painting into strips.
    GraphicsContext* context = paintInfo.context;
    ColumnInfo* colInfo = columnInfo();
    unsigned colCount = columnCount(colInfo);
    if (!colCount)
        return;
    LayoutUnit colGap = columnGap();
    LayoutUnit currLogicalTopOffset = initialBlockOffsetForPainting();
    LayoutUnit blockDelta = blockDeltaForPaintingNextColumn();
    for (unsigned i = 0; i < colCount; i++) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        LayoutRect colRect = columnRectAt(colInfo, i);
        flipForWritingMode(colRect);
        
        LayoutUnit logicalLeftOffset = (isHorizontalWritingMode() ? colRect.x() : colRect.y()) - logicalLeftOffsetForContent();
        LayoutSize offset = isHorizontalWritingMode() ? LayoutSize(logicalLeftOffset, currLogicalTopOffset) : LayoutSize(currLogicalTopOffset, logicalLeftOffset);
        colRect.moveBy(paintOffset);
        PaintInfo info(paintInfo);
        info.rect.intersect(pixelSnappedIntRect(colRect));
        
        if (!info.rect.isEmpty()) {
            GraphicsContextStateSaver stateSaver(*context);
            LayoutRect clipRect(colRect);
            
            if (i < colCount - 1) {
                if (isHorizontalWritingMode())
                    clipRect.expand(colGap / 2, 0);
                else
                    clipRect.expand(0, colGap / 2);
            }
            // Each strip pushes a clip, since column boxes are specified as being
            // like overflow:hidden.
            // FIXME: Content and column rules that extend outside column boxes at the edges of the multi-column element
            // are clipped according to the 'overflow' property.
            context->clip(pixelSnappedIntRect(clipRect));

            // Adjust our x and y when painting.
            LayoutPoint adjustedPaintOffset = paintOffset + offset;
            if (paintingFloats)
                paintFloats(info, adjustedPaintOffset, paintInfo.phase == PaintPhaseSelection || paintInfo.phase == PaintPhaseTextClip);
            else
                paintContents(info, adjustedPaintOffset);
        }
        currLogicalTopOffset += blockDelta;
    }
}

void RenderBlock::paintContents(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    // Avoid painting descendants of the root element when stylesheets haven't loaded.  This eliminates FOUC.
    // It's ok not to draw, because later on, when all the stylesheets do load, styleResolverChanged() on the Document
    // will do a full repaint.
    if (document()->didLayoutWithPendingStylesheets() && !isRenderView())
        return;

    if (childrenInline())
        m_lineBoxes.paint(this, paintInfo, paintOffset);
    else {
        PaintPhase newPhase = (paintInfo.phase == PaintPhaseChildOutlines) ? PaintPhaseOutline : paintInfo.phase;
        newPhase = (newPhase == PaintPhaseChildBlockBackgrounds) ? PaintPhaseChildBlockBackground : newPhase;

        // We don't paint our own background, but we do let the kids paint their backgrounds.
        PaintInfo paintInfoForChild(paintInfo);
        paintInfoForChild.phase = newPhase;
        paintInfoForChild.updateSubtreePaintRootForChildren(this);

        // FIXME: Paint-time pagination is obsolete and is now only used by embedded WebViews inside AppKit
        // NSViews. Do not add any more code for this.
        bool usePrintRect = !view()->printRect().isEmpty();
        paintChildren(paintInfo, paintOffset, paintInfoForChild, usePrintRect);
    }
}

void RenderBlock::paintChildren(PaintInfo& paintInfo, const LayoutPoint& paintOffset, PaintInfo& paintInfoForChild, bool usePrintRect)
{
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (!paintChild(child, paintInfo, paintOffset, paintInfoForChild, usePrintRect))
            return;
    }
}

bool RenderBlock::paintChild(RenderBox* child, PaintInfo& paintInfo, const LayoutPoint& paintOffset, PaintInfo& paintInfoForChild, bool usePrintRect)
{
    // Check for page-break-before: always, and if it's set, break and bail.
    bool checkBeforeAlways = !childrenInline() && (usePrintRect && child->style()->pageBreakBefore() == PBALWAYS);
    LayoutUnit absoluteChildY = paintOffset.y() + child->y();
    if (checkBeforeAlways
        && absoluteChildY > paintInfo.rect.y()
        && absoluteChildY < paintInfo.rect.maxY()) {
        view()->setBestTruncatedAt(absoluteChildY, this, true);
        return false;
    }

    RenderView* renderView = view();
    if (!child->isFloating() && child->isReplaced() && usePrintRect && child->height() <= renderView->printRect().height()) {
        // Paginate block-level replaced elements.
        if (absoluteChildY + child->height() > renderView->printRect().maxY()) {
            if (absoluteChildY < renderView->truncatedAt())
                renderView->setBestTruncatedAt(absoluteChildY, child);
            // If we were able to truncate, don't paint.
            if (absoluteChildY >= renderView->truncatedAt())
                return false;
        }
    }

    LayoutPoint childPoint = flipForWritingModeForChild(child, paintOffset);
    if (!child->hasSelfPaintingLayer() && !child->isFloating())
        child->paint(paintInfoForChild, childPoint);

    // Check for page-break-after: always, and if it's set, break and bail.
    bool checkAfterAlways = !childrenInline() && (usePrintRect && child->style()->pageBreakAfter() == PBALWAYS);
    if (checkAfterAlways
        && (absoluteChildY + child->height()) > paintInfo.rect.y()
        && (absoluteChildY + child->height()) < paintInfo.rect.maxY()) {
        view()->setBestTruncatedAt(absoluteChildY + child->height() + max<LayoutUnit>(0, child->collapsedMarginAfter()), this, true);
        return false;
    }
    return true;
}


void RenderBlock::paintCaret(PaintInfo& paintInfo, const LayoutPoint& paintOffset, CaretType type)
{
    // Paint the caret if the FrameSelection says so or if caret browsing is enabled
    bool caretBrowsing = frame()->settings() && frame()->settings()->caretBrowsingEnabled();
    RenderObject* caretPainter;
    bool isContentEditable;
    if (type == CursorCaret) {
        caretPainter = frame()->selection()->caretRenderer();
        isContentEditable = frame()->selection()->rendererIsEditable();
    } else {
        caretPainter = frame()->page()->dragCaretController()->caretRenderer();
        isContentEditable = frame()->page()->dragCaretController()->isContentEditable();
    }

    if (caretPainter == this && (isContentEditable || caretBrowsing)) {
        if (type == CursorCaret)
            frame()->selection()->paintCaret(paintInfo.context, paintOffset, paintInfo.rect);
        else
            frame()->page()->dragCaretController()->paintDragCaret(frame(), paintInfo.context, paintOffset, paintInfo.rect);
    }
}

void RenderBlock::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    PaintPhase paintPhase = paintInfo.phase;

    // 1. paint background, borders etc
    if ((paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) && style()->visibility() == VISIBLE) {
        if (hasBoxDecorations())
            paintBoxDecorations(paintInfo, paintOffset);
        if (hasColumns() && !paintInfo.paintRootBackgroundOnly())
            paintColumnRules(paintInfo, paintOffset);
    }

    if (paintPhase == PaintPhaseMask && style()->visibility() == VISIBLE) {
        paintMask(paintInfo, paintOffset);
        return;
    }

    // We're done.  We don't bother painting any children.
    if (paintPhase == PaintPhaseBlockBackground || paintInfo.paintRootBackgroundOnly())
        return;

    // Adjust our painting position if we're inside a scrolled layer (e.g., an overflow:auto div).
    LayoutPoint scrolledOffset = paintOffset;
    if (hasOverflowClip())
        scrolledOffset.move(-scrolledContentOffset());

    // 2. paint contents
    if (paintPhase != PaintPhaseSelfOutline) {
        if (hasColumns())
            paintColumnContents(paintInfo, scrolledOffset);
        else
            paintContents(paintInfo, scrolledOffset);
    }

    // 3. paint selection
    // FIXME: Make this work with multi column layouts.  For now don't fill gaps.
    bool isPrinting = document()->printing();
    if (!isPrinting && !hasColumns())
        paintSelection(paintInfo, scrolledOffset); // Fill in gaps in selection on lines and between blocks.

    // 4. paint floats.
    if (paintPhase == PaintPhaseFloat || paintPhase == PaintPhaseSelection || paintPhase == PaintPhaseTextClip) {
        if (hasColumns())
            paintColumnContents(paintInfo, scrolledOffset, true);
        else
            paintFloats(paintInfo, scrolledOffset, paintPhase == PaintPhaseSelection || paintPhase == PaintPhaseTextClip);
    }

    // 5. paint outline.
    if ((paintPhase == PaintPhaseOutline || paintPhase == PaintPhaseSelfOutline) && hasOutline() && style()->visibility() == VISIBLE)
        paintOutline(paintInfo, LayoutRect(paintOffset, size()));

    // 6. paint continuation outlines.
    if ((paintPhase == PaintPhaseOutline || paintPhase == PaintPhaseChildOutlines)) {
        RenderInline* inlineCont = inlineElementContinuation();
        if (inlineCont && inlineCont->hasOutline() && inlineCont->style()->visibility() == VISIBLE) {
            RenderInline* inlineRenderer = toRenderInline(inlineCont->node()->renderer());
            RenderBlock* cb = containingBlock();

            bool inlineEnclosedInSelfPaintingLayer = false;
            for (RenderBoxModelObject* box = inlineRenderer; box != cb; box = box->parent()->enclosingBoxModelObject()) {
                if (box->hasSelfPaintingLayer()) {
                    inlineEnclosedInSelfPaintingLayer = true;
                    break;
                }
            }

            // Do not add continuations for outline painting by our containing block if we are a relative positioned
            // anonymous block (i.e. have our own layer), paint them straightaway instead. This is because a block depends on renderers in its continuation table being
            // in the same layer. 
            if (!inlineEnclosedInSelfPaintingLayer && !hasLayer())
                cb->addContinuationWithOutline(inlineRenderer);
            else if (!inlineRenderer->firstLineBox() || (!inlineEnclosedInSelfPaintingLayer && hasLayer()))
                inlineRenderer->paintOutline(paintInfo, paintOffset - locationOffset() + inlineRenderer->containingBlock()->location());
        }
        paintContinuationOutlines(paintInfo, paintOffset);
    }

    // 7. paint caret.
    // If the caret's node's render object's containing block is this block, and the paint action is PaintPhaseForeground,
    // then paint the caret.
    if (paintPhase == PaintPhaseForeground) {        
        paintCaret(paintInfo, paintOffset, CursorCaret);
        paintCaret(paintInfo, paintOffset, DragCaret);
    }
}

LayoutPoint RenderBlock::flipFloatForWritingModeForChild(const FloatingObject* child, const LayoutPoint& point) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return point;
    
    // This is similar to RenderBox::flipForWritingModeForChild. We have to subtract out our left/top offsets twice, since
    // it's going to get added back in. We hide this complication here so that the calling code looks normal for the unflipped
    // case.
    if (isHorizontalWritingMode())
        return LayoutPoint(point.x(), point.y() + height() - child->renderer()->height() - 2 * yPositionForFloatIncludingMargin(child));
    return LayoutPoint(point.x() + width() - child->renderer()->width() - 2 * xPositionForFloatIncludingMargin(child), point.y());
}

void RenderBlock::paintFloats(PaintInfo& paintInfo, const LayoutPoint& paintOffset, bool preservePhase)
{
    if (!m_floatingObjects)
        return;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        // Only paint the object if our m_shouldPaint flag is set.
        if (r->shouldPaint() && !r->m_renderer->hasSelfPaintingLayer()) {
            PaintInfo currentPaintInfo(paintInfo);
            currentPaintInfo.phase = preservePhase ? paintInfo.phase : PaintPhaseBlockBackground;
            LayoutPoint childPoint = flipFloatForWritingModeForChild(r, LayoutPoint(paintOffset.x() + xPositionForFloatIncludingMargin(r) - r->m_renderer->x(), paintOffset.y() + yPositionForFloatIncludingMargin(r) - r->m_renderer->y()));
            r->m_renderer->paint(currentPaintInfo, childPoint);
            if (!preservePhase) {
                currentPaintInfo.phase = PaintPhaseChildBlockBackgrounds;
                r->m_renderer->paint(currentPaintInfo, childPoint);
                currentPaintInfo.phase = PaintPhaseFloat;
                r->m_renderer->paint(currentPaintInfo, childPoint);
                currentPaintInfo.phase = PaintPhaseForeground;
                r->m_renderer->paint(currentPaintInfo, childPoint);
                currentPaintInfo.phase = PaintPhaseOutline;
                r->m_renderer->paint(currentPaintInfo, childPoint);
            }
        }
    }
}

RenderInline* RenderBlock::inlineElementContinuation() const
{ 
    RenderBoxModelObject* continuation = this->continuation();
    return continuation && continuation->isInline() ? toRenderInline(continuation) : 0;
}

RenderBlock* RenderBlock::blockElementContinuation() const
{
    RenderBoxModelObject* currentContinuation = continuation();
    if (!currentContinuation || currentContinuation->isInline())
        return 0;
    RenderBlock* nextContinuation = toRenderBlock(currentContinuation);
    if (nextContinuation->isAnonymousBlock())
        return nextContinuation->blockElementContinuation();
    return nextContinuation;
}
    
static ContinuationOutlineTableMap* continuationOutlineTable()
{
    DEFINE_STATIC_LOCAL(ContinuationOutlineTableMap, table, ());
    return &table;
}

void RenderBlock::addContinuationWithOutline(RenderInline* flow)
{
    // We can't make this work if the inline is in a layer.  We'll just rely on the broken
    // way of painting.
    ASSERT(!flow->layer() && !flow->isInlineElementContinuation());
    
    ContinuationOutlineTableMap* table = continuationOutlineTable();
    ListHashSet<RenderInline*>* continuations = table->get(this);
    if (!continuations) {
        continuations = new ListHashSet<RenderInline*>;
        table->set(this, adoptPtr(continuations));
    }
    
    continuations->add(flow);
}

bool RenderBlock::paintsContinuationOutline(RenderInline* flow)
{
    ContinuationOutlineTableMap* table = continuationOutlineTable();
    if (table->isEmpty())
        return false;
        
    ListHashSet<RenderInline*>* continuations = table->get(this);
    if (!continuations)
        return false;

    return continuations->contains(flow);
}

void RenderBlock::paintContinuationOutlines(PaintInfo& info, const LayoutPoint& paintOffset)
{
    ContinuationOutlineTableMap* table = continuationOutlineTable();
    if (table->isEmpty())
        return;
        
    OwnPtr<ListHashSet<RenderInline*> > continuations = table->take(this);
    if (!continuations)
        return;

    LayoutPoint accumulatedPaintOffset = paintOffset;
    // Paint each continuation outline.
    ListHashSet<RenderInline*>::iterator end = continuations->end();
    for (ListHashSet<RenderInline*>::iterator it = continuations->begin(); it != end; ++it) {
        // Need to add in the coordinates of the intervening blocks.
        RenderInline* flow = *it;
        RenderBlock* block = flow->containingBlock();
        for ( ; block && block != this; block = block->containingBlock())
            accumulatedPaintOffset.moveBy(block->location());
        ASSERT(block);   
        flow->paintOutline(info, accumulatedPaintOffset);
    }
}

bool RenderBlock::shouldPaintSelectionGaps() const
{
    return selectionState() != SelectionNone && style()->visibility() == VISIBLE && isSelectionRoot();
}

bool RenderBlock::isSelectionRoot() const
{
    if (isPseudoElement())
        return false;
    ASSERT(node() || isAnonymous());
        
    // FIXME: Eventually tables should have to learn how to fill gaps between cells, at least in simple non-spanning cases.
    if (isTable())
        return false;
        
    if (isBody() || isRoot() || hasOverflowClip()
        || isPositioned() || isFloating()
        || isTableCell() || isInlineBlockOrInlineTable()
        || hasTransform() || hasReflection() || hasMask() || isWritingModeRoot()
        || isRenderFlowThread())
        return true;
    
    if (view() && view()->selectionStart()) {
        Node* startElement = view()->selectionStart()->node();
        if (startElement && startElement->rootEditableElement() == node())
            return true;
    }
    
    return false;
}

GapRects RenderBlock::selectionGapRectsForRepaint(const RenderLayerModelObject* repaintContainer)
{
    ASSERT(!needsLayout());

    if (!shouldPaintSelectionGaps())
        return GapRects();

    TransformState transformState(TransformState::ApplyTransformDirection, FloatPoint());
    mapLocalToContainer(repaintContainer, transformState, ApplyContainerFlip | UseTransforms);
    LayoutPoint offsetFromRepaintContainer = roundedLayoutPoint(transformState.mappedPoint());

    if (hasOverflowClip())
        offsetFromRepaintContainer -= scrolledContentOffset();

    LogicalSelectionOffsetCaches cache(this);
    LayoutUnit lastTop = 0;
    LayoutUnit lastLeft = logicalLeftSelectionOffset(this, lastTop, cache);
    LayoutUnit lastRight = logicalRightSelectionOffset(this, lastTop, cache);
    
    return selectionGaps(this, offsetFromRepaintContainer, IntSize(), lastTop, lastLeft, lastRight, cache);
}

void RenderBlock::paintSelection(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (shouldPaintSelectionGaps() && paintInfo.phase == PaintPhaseForeground) {
        LogicalSelectionOffsetCaches cache(this);
        LayoutUnit lastTop = 0;
        LayoutUnit lastLeft = logicalLeftSelectionOffset(this, lastTop, cache);
        LayoutUnit lastRight = logicalRightSelectionOffset(this, lastTop, cache);
        GraphicsContextStateSaver stateSaver(*paintInfo.context);

        LayoutRect gapRectsBounds = selectionGaps(this, paintOffset, LayoutSize(), lastTop, lastLeft, lastRight, cache, &paintInfo);
        if (!gapRectsBounds.isEmpty()) {
            if (RenderLayer* layer = enclosingLayer()) {
                gapRectsBounds.moveBy(-paintOffset);
                if (!hasLayer()) {
                    LayoutRect localBounds(gapRectsBounds);
                    flipForWritingMode(localBounds);
                    gapRectsBounds = localToContainerQuad(FloatRect(localBounds), layer->renderer()).enclosingBoundingBox();
                    if (layer->renderer()->hasOverflowClip())
                        gapRectsBounds.move(layer->renderBox()->scrolledContentOffset());
                }
                layer->addBlockSelectionGapsBounds(gapRectsBounds);
            }
        }
    }
}

static void clipOutPositionedObjects(const PaintInfo* paintInfo, const LayoutPoint& offset, TrackedRendererListHashSet* positionedObjects)
{
    if (!positionedObjects)
        return;
    
    TrackedRendererListHashSet::const_iterator end = positionedObjects->end();
    for (TrackedRendererListHashSet::const_iterator it = positionedObjects->begin(); it != end; ++it) {
        RenderBox* r = *it;
        paintInfo->context->clipOut(IntRect(offset.x() + r->x(), offset.y() + r->y(), r->width(), r->height()));
    }
}

static LayoutUnit blockDirectionOffset(RenderBlock* rootBlock, const LayoutSize& offsetFromRootBlock)
{
    return rootBlock->isHorizontalWritingMode() ? offsetFromRootBlock.height() : offsetFromRootBlock.width();
}

static LayoutUnit inlineDirectionOffset(RenderBlock* rootBlock, const LayoutSize& offsetFromRootBlock)
{
    return rootBlock->isHorizontalWritingMode() ? offsetFromRootBlock.width() : offsetFromRootBlock.height();
}

LayoutRect RenderBlock::logicalRectToPhysicalRect(const LayoutPoint& rootBlockPhysicalPosition, const LayoutRect& logicalRect)
{
    LayoutRect result;
    if (isHorizontalWritingMode())
        result = logicalRect;
    else
        result = LayoutRect(logicalRect.y(), logicalRect.x(), logicalRect.height(), logicalRect.width());
    flipForWritingMode(result);
    result.moveBy(rootBlockPhysicalPosition);
    return result;
}

GapRects RenderBlock::selectionGaps(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    LayoutUnit& lastLogicalTop, LayoutUnit& lastLogicalLeft, LayoutUnit& lastLogicalRight, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    // IMPORTANT: Callers of this method that intend for painting to happen need to do a save/restore.
    // Clip out floating and positioned objects when painting selection gaps.
    if (paintInfo) {
        // Note that we don't clip out overflow for positioned objects.  We just stick to the border box.
        LayoutRect flippedBlockRect(offsetFromRootBlock.width(), offsetFromRootBlock.height(), width(), height());
        rootBlock->flipForWritingMode(flippedBlockRect);
        flippedBlockRect.moveBy(rootBlockPhysicalPosition);
        clipOutPositionedObjects(paintInfo, flippedBlockRect.location(), positionedObjects());
        if (isBody() || isRoot()) // The <body> must make sure to examine its containingBlock's positioned objects.
            for (RenderBlock* cb = containingBlock(); cb && !cb->isRenderView(); cb = cb->containingBlock())
                clipOutPositionedObjects(paintInfo, LayoutPoint(cb->x(), cb->y()), cb->positionedObjects()); // FIXME: Not right for flipped writing modes.
        if (m_floatingObjects) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* r = *it;
                LayoutRect floatBox(offsetFromRootBlock.width() + xPositionForFloatIncludingMargin(r),
                                    offsetFromRootBlock.height() + yPositionForFloatIncludingMargin(r),
                                    r->m_renderer->width(), r->m_renderer->height());
                rootBlock->flipForWritingMode(floatBox);
                floatBox.move(rootBlockPhysicalPosition.x(), rootBlockPhysicalPosition.y());
                paintInfo->context->clipOut(pixelSnappedIntRect(floatBox));
            }
        }
    }

    // FIXME: overflow: auto/scroll regions need more math here, since painting in the border box is different from painting in the padding box (one is scrolled, the other is
    // fixed).
    GapRects result;
    if (!isBlockFlow()) // FIXME: Make multi-column selection gap filling work someday.
        return result;

    if (hasColumns() || hasTransform() || style()->columnSpan()) {
        // FIXME: We should learn how to gap fill multiple columns and transforms eventually.
        lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalHeight();
        lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, logicalHeight(), cache);
        lastLogicalRight = logicalRightSelectionOffset(rootBlock, logicalHeight(), cache);
        return result;
    }

    if (childrenInline())
        result = inlineSelectionGaps(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, cache, paintInfo);
    else
        result = blockSelectionGaps(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, cache, paintInfo);

    // Go ahead and fill the vertical gap all the way to the bottom of our block if the selection extends past our block.
    if (rootBlock == this && (selectionState() != SelectionBoth && selectionState() != SelectionEnd)) {
        result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock,
            lastLogicalTop, lastLogicalLeft, lastLogicalRight, logicalHeight(), cache, paintInfo));
    }

    return result;
}

GapRects RenderBlock::inlineSelectionGaps(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    LayoutUnit& lastLogicalTop, LayoutUnit& lastLogicalLeft, LayoutUnit& lastLogicalRight, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    GapRects result;

    bool containsStart = selectionState() == SelectionStart || selectionState() == SelectionBoth;

    if (!firstLineBox()) {
        if (containsStart) {
            // Go ahead and update our lastLogicalTop to be the bottom of the block.  <hr>s or empty blocks with height can trip this
            // case.
            lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalHeight();
            lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, logicalHeight(), cache);
            lastLogicalRight = logicalRightSelectionOffset(rootBlock, logicalHeight(), cache);
        }
        return result;
    }

    RootInlineBox* lastSelectedLine = 0;
    RootInlineBox* curr;
    for (curr = firstRootBox(); curr && !curr->hasSelectedChildren(); curr = curr->nextRootBox()) { }

    // Now paint the gaps for the lines.
    for (; curr && curr->hasSelectedChildren(); curr = curr->nextRootBox()) {
        LayoutUnit selTop =  curr->selectionTopAdjustedForPrecedingBlock();
        LayoutUnit selHeight = curr->selectionHeightAdjustedForPrecedingBlock();

        if (!containsStart && !lastSelectedLine &&
            selectionState() != SelectionStart && selectionState() != SelectionBoth)
            result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, selTop, cache, paintInfo));
        
        LayoutRect logicalRect(curr->logicalLeft(), selTop, curr->logicalWidth(), selTop + selHeight);
        logicalRect.move(isHorizontalWritingMode() ? offsetFromRootBlock : offsetFromRootBlock.transposedSize());
        LayoutRect physicalRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, logicalRect);
        if (!paintInfo || (isHorizontalWritingMode() && physicalRect.y() < paintInfo->rect.maxY() && physicalRect.maxY() > paintInfo->rect.y())
            || (!isHorizontalWritingMode() && physicalRect.x() < paintInfo->rect.maxX() && physicalRect.maxX() > paintInfo->rect.x()))
            result.unite(curr->lineSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, selTop, selHeight, cache, paintInfo));

        lastSelectedLine = curr;
    }

    if (containsStart && !lastSelectedLine)
        // VisibleSelection must start just after our last line.
        lastSelectedLine = lastRootBox();

    if (lastSelectedLine && selectionState() != SelectionEnd && selectionState() != SelectionBoth) {
        // Go ahead and update our lastY to be the bottom of the last selected line.
        lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + lastSelectedLine->selectionBottom();
        lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, lastSelectedLine->selectionBottom(), cache);
        lastLogicalRight = logicalRightSelectionOffset(rootBlock, lastSelectedLine->selectionBottom(), cache);
    }
    return result;
}

GapRects RenderBlock::blockSelectionGaps(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    LayoutUnit& lastLogicalTop, LayoutUnit& lastLogicalLeft, LayoutUnit& lastLogicalRight, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    GapRects result;

    // Go ahead and jump right to the first block child that contains some selected objects.
    RenderBox* curr;
    for (curr = firstChildBox(); curr && curr->selectionState() == SelectionNone; curr = curr->nextSiblingBox()) { }
    
    if (!curr)
        return result;

    LogicalSelectionOffsetCaches childCache(this, cache);

    for (bool sawSelectionEnd = false; curr && !sawSelectionEnd; curr = curr->nextSiblingBox()) {
        SelectionState childState = curr->selectionState();
        if (childState == SelectionBoth || childState == SelectionEnd)
            sawSelectionEnd = true;

        if (curr->isFloatingOrOutOfFlowPositioned())
            continue; // We must be a normal flow object in order to even be considered.

        if (curr->isInFlowPositioned() && curr->hasLayer()) {
            // If the relposition offset is anything other than 0, then treat this just like an absolute positioned element.
            // Just disregard it completely.
            LayoutSize relOffset = curr->layer()->offsetForInFlowPosition();
            if (relOffset.width() || relOffset.height())
                continue;
        }

        bool paintsOwnSelection = curr->shouldPaintSelectionGaps() || curr->isTable(); // FIXME: Eventually we won't special-case table like this.
        bool fillBlockGaps = paintsOwnSelection || (curr->canBeSelectionLeaf() && childState != SelectionNone);
        if (fillBlockGaps) {
            // We need to fill the vertical gap above this object.
            if (childState == SelectionEnd || childState == SelectionInside) {
                // Fill the gap above the object.
                result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock,
                    lastLogicalTop, lastLogicalLeft, lastLogicalRight, curr->logicalTop(), cache, paintInfo));
            }

            // Only fill side gaps for objects that paint their own selection if we know for sure the selection is going to extend all the way *past*
            // our object.  We know this if the selection did not end inside our object.
            if (paintsOwnSelection && (childState == SelectionStart || sawSelectionEnd))
                childState = SelectionNone;

            // Fill side gaps on this object based off its state.
            bool leftGap, rightGap;
            getSelectionGapInfo(childState, leftGap, rightGap);

            if (leftGap)
                result.uniteLeft(logicalLeftSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, this, curr->logicalLeft(), curr->logicalTop(), curr->logicalHeight(), cache, paintInfo));
            if (rightGap)
                result.uniteRight(logicalRightSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, this, curr->logicalRight(), curr->logicalTop(), curr->logicalHeight(), cache, paintInfo));

            // Update lastLogicalTop to be just underneath the object.  lastLogicalLeft and lastLogicalRight extend as far as
            // they can without bumping into floating or positioned objects.  Ideally they will go right up
            // to the border of the root selection block.
            lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + curr->logicalBottom();
            lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, curr->logicalBottom(), cache);
            lastLogicalRight = logicalRightSelectionOffset(rootBlock, curr->logicalBottom(), cache);
        } else if (childState != SelectionNone) {
            // We must be a block that has some selected object inside it.  Go ahead and recur.
            result.unite(toRenderBlock(curr)->selectionGaps(rootBlock, rootBlockPhysicalPosition, LayoutSize(offsetFromRootBlock.width() + curr->x(), offsetFromRootBlock.height() + curr->y()), 
                lastLogicalTop, lastLogicalLeft, lastLogicalRight, childCache, paintInfo));
        }
    }
    return result;
}

LayoutRect RenderBlock::blockSelectionGap(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    LayoutUnit lastLogicalTop, LayoutUnit lastLogicalLeft, LayoutUnit lastLogicalRight, LayoutUnit logicalBottom, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    LayoutUnit logicalTop = lastLogicalTop;
    LayoutUnit logicalHeight = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalBottom - logicalTop;
    if (logicalHeight <= 0)
        return LayoutRect();

    // Get the selection offsets for the bottom of the gap
    LayoutUnit logicalLeft = max(lastLogicalLeft, logicalLeftSelectionOffset(rootBlock, logicalBottom, cache));
    LayoutUnit logicalRight = min(lastLogicalRight, logicalRightSelectionOffset(rootBlock, logicalBottom, cache));
    LayoutUnit logicalWidth = logicalRight - logicalLeft;
    if (logicalWidth <= 0)
        return LayoutRect();

    LayoutRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, LayoutRect(logicalLeft, logicalTop, logicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(pixelSnappedIntRect(gapRect), selectionBackgroundColor(), style()->colorSpace());
    return gapRect;
}

LayoutRect RenderBlock::logicalLeftSelectionGap(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    RenderObject* selObj, LayoutUnit logicalLeft, LayoutUnit logicalTop, LayoutUnit logicalHeight, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    LayoutUnit rootBlockLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalTop;
    LayoutUnit rootBlockLogicalLeft = max(logicalLeftSelectionOffset(rootBlock, logicalTop, cache), logicalLeftSelectionOffset(rootBlock, logicalTop + logicalHeight, cache));
    LayoutUnit rootBlockLogicalRight = min(inlineDirectionOffset(rootBlock, offsetFromRootBlock) + floorToInt(logicalLeft),
        min(logicalRightSelectionOffset(rootBlock, logicalTop, cache), logicalRightSelectionOffset(rootBlock, logicalTop + logicalHeight, cache)));
    LayoutUnit rootBlockLogicalWidth = rootBlockLogicalRight - rootBlockLogicalLeft;
    if (rootBlockLogicalWidth <= 0)
        return LayoutRect();

    LayoutRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, LayoutRect(rootBlockLogicalLeft, rootBlockLogicalTop, rootBlockLogicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(pixelSnappedIntRect(gapRect), selObj->selectionBackgroundColor(), selObj->style()->colorSpace());
    return gapRect;
}

LayoutRect RenderBlock::logicalRightSelectionGap(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
    RenderObject* selObj, LayoutUnit logicalRight, LayoutUnit logicalTop, LayoutUnit logicalHeight, const LogicalSelectionOffsetCaches& cache, const PaintInfo* paintInfo)
{
    LayoutUnit rootBlockLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalTop;
    LayoutUnit rootBlockLogicalLeft = max(inlineDirectionOffset(rootBlock, offsetFromRootBlock) + floorToInt(logicalRight),
        max(logicalLeftSelectionOffset(rootBlock, logicalTop, cache), logicalLeftSelectionOffset(rootBlock, logicalTop + logicalHeight, cache)));
    LayoutUnit rootBlockLogicalRight = min(logicalRightSelectionOffset(rootBlock, logicalTop, cache), logicalRightSelectionOffset(rootBlock, logicalTop + logicalHeight, cache));
    LayoutUnit rootBlockLogicalWidth = rootBlockLogicalRight - rootBlockLogicalLeft;
    if (rootBlockLogicalWidth <= 0)
        return LayoutRect();

    LayoutRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, LayoutRect(rootBlockLogicalLeft, rootBlockLogicalTop, rootBlockLogicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(pixelSnappedIntRect(gapRect), selObj->selectionBackgroundColor(), selObj->style()->colorSpace());
    return gapRect;
}

void RenderBlock::getSelectionGapInfo(SelectionState state, bool& leftGap, bool& rightGap)
{
    bool ltr = style()->isLeftToRightDirection();
    leftGap = (state == RenderObject::SelectionInside) ||
              (state == RenderObject::SelectionEnd && ltr) ||
              (state == RenderObject::SelectionStart && !ltr);
    rightGap = (state == RenderObject::SelectionInside) ||
               (state == RenderObject::SelectionStart && ltr) ||
               (state == RenderObject::SelectionEnd && !ltr);
}

LayoutUnit RenderBlock::logicalLeftSelectionOffset(RenderBlock* rootBlock, LayoutUnit position, const LogicalSelectionOffsetCaches& cache)
{
    LayoutUnit logicalLeft = logicalLeftOffsetForLine(position, false);
    if (logicalLeft == logicalLeftOffsetForContent()) {
        if (rootBlock != this) // The border can potentially be further extended by our containingBlock().
            return cache.containingBlockInfo(this).logicalLeftSelectionOffset(rootBlock, position + logicalTop());
        return logicalLeft;
    } else {
        RenderBlock* cb = this;
        const LogicalSelectionOffsetCaches* currentCache = &cache;
        while (cb != rootBlock) {
            logicalLeft += cb->logicalLeft();

            ASSERT(currentCache);
            const LogicalSelectionOffsetCaches::ContainingBlockInfo& info = currentCache->containingBlockInfo(cb);
            cb = info.block();
            currentCache = info.cache();
        }
    }
    return logicalLeft;
}

LayoutUnit RenderBlock::logicalRightSelectionOffset(RenderBlock* rootBlock, LayoutUnit position, const LogicalSelectionOffsetCaches& cache)
{
    LayoutUnit logicalRight = logicalRightOffsetForLine(position, false);
    if (logicalRight == logicalRightOffsetForContent()) {
        if (rootBlock != this) // The border can potentially be further extended by our containingBlock().
            return cache.containingBlockInfo(this).logicalRightSelectionOffset(rootBlock, position + logicalTop());
        return logicalRight;
    } else {
        RenderBlock* cb = this;
        const LogicalSelectionOffsetCaches* currentCache = &cache;
        while (cb != rootBlock) {
            logicalRight += cb->logicalLeft();

            ASSERT(currentCache);
            const LogicalSelectionOffsetCaches::ContainingBlockInfo& info = currentCache->containingBlockInfo(cb);
            cb = info.block();
            currentCache = info.cache();
        }
    }
    return logicalRight;
}

RenderBlock* RenderBlock::blockBeforeWithinSelectionRoot(LayoutSize& offset) const
{
    if (isSelectionRoot())
        return 0;

    const RenderObject* object = this;
    RenderObject* sibling;
    do {
        sibling = object->previousSibling();
        while (sibling && (!sibling->isRenderBlock() || toRenderBlock(sibling)->isSelectionRoot()))
            sibling = sibling->previousSibling();

        offset -= LayoutSize(toRenderBlock(object)->logicalLeft(), toRenderBlock(object)->logicalTop());
        object = object->parent();
    } while (!sibling && object && object->isRenderBlock() && !toRenderBlock(object)->isSelectionRoot());

    if (!sibling)
        return 0;

    RenderBlock* beforeBlock = toRenderBlock(sibling);

    offset += LayoutSize(beforeBlock->logicalLeft(), beforeBlock->logicalTop());

    RenderObject* child = beforeBlock->lastChild();
    while (child && child->isRenderBlock()) {
        beforeBlock = toRenderBlock(child);
        offset += LayoutSize(beforeBlock->logicalLeft(), beforeBlock->logicalTop());
        child = beforeBlock->lastChild();
    }
    return beforeBlock;
}

void RenderBlock::insertIntoTrackedRendererMaps(RenderBox* descendant, TrackedDescendantsMap*& descendantsMap, TrackedContainerMap*& containerMap)
{
    if (!descendantsMap) {
        descendantsMap = new TrackedDescendantsMap;
        containerMap = new TrackedContainerMap;
    }
    
    TrackedRendererListHashSet* descendantSet = descendantsMap->get(this);
    if (!descendantSet) {
        descendantSet = new TrackedRendererListHashSet;
        descendantsMap->set(this, adoptPtr(descendantSet));
    }
    bool added = descendantSet->add(descendant).isNewEntry;
    if (!added) {
        ASSERT(containerMap->get(descendant));
        ASSERT(containerMap->get(descendant)->contains(this));
        return;
    }
    
    HashSet<RenderBlock*>* containerSet = containerMap->get(descendant);
    if (!containerSet) {
        containerSet = new HashSet<RenderBlock*>;
        containerMap->set(descendant, adoptPtr(containerSet));
    }
    ASSERT(!containerSet->contains(this));
    containerSet->add(this);
}

void RenderBlock::removeFromTrackedRendererMaps(RenderBox* descendant, TrackedDescendantsMap*& descendantsMap, TrackedContainerMap*& containerMap)
{
    if (!descendantsMap)
        return;
    
    OwnPtr<HashSet<RenderBlock*> > containerSet = containerMap->take(descendant);
    if (!containerSet)
        return;
    
    HashSet<RenderBlock*>::iterator end = containerSet->end();
    for (HashSet<RenderBlock*>::iterator it = containerSet->begin(); it != end; ++it) {
        RenderBlock* container = *it;

        // FIXME: Disabling this assert temporarily until we fix the layout
        // bugs associated with positioned objects not properly cleared from
        // their ancestor chain before being moved. See webkit bug 93766.
        // ASSERT(descendant->isDescendantOf(container));

        TrackedDescendantsMap::iterator descendantsMapIterator = descendantsMap->find(container);
        ASSERT(descendantsMapIterator != descendantsMap->end());
        if (descendantsMapIterator == descendantsMap->end())
            continue;
        TrackedRendererListHashSet* descendantSet = descendantsMapIterator->value.get();
        ASSERT(descendantSet->contains(descendant));
        descendantSet->remove(descendant);
        if (descendantSet->isEmpty())
            descendantsMap->remove(descendantsMapIterator);
    }
}

TrackedRendererListHashSet* RenderBlock::positionedObjects() const
{
    if (gPositionedDescendantsMap)
        return gPositionedDescendantsMap->get(this);
    return 0;
}

void RenderBlock::insertPositionedObject(RenderBox* o)
{
    ASSERT(!isAnonymousBlock());

    if (o->isRenderFlowThread())
        return;
    
    insertIntoTrackedRendererMaps(o, gPositionedDescendantsMap, gPositionedContainerMap);
}

void RenderBlock::removePositionedObject(RenderBox* o)
{
    removeFromTrackedRendererMaps(o, gPositionedDescendantsMap, gPositionedContainerMap);
}

void RenderBlock::removePositionedObjects(RenderBlock* o, ContainingBlockState containingBlockState)
{
    TrackedRendererListHashSet* positionedDescendants = positionedObjects();
    if (!positionedDescendants)
        return;
    
    RenderBox* r;
    
    TrackedRendererListHashSet::iterator end = positionedDescendants->end();
    
    Vector<RenderBox*, 16> deadObjects;

    for (TrackedRendererListHashSet::iterator it = positionedDescendants->begin(); it != end; ++it) {
        r = *it;
        if (!o || r->isDescendantOf(o)) {
            if (containingBlockState == NewContainingBlock)
                r->setChildNeedsLayout(true, MarkOnlyThis);
            
            // It is parent blocks job to add positioned child to positioned objects list of its containing block
            // Parent layout needs to be invalidated to ensure this happens.
            RenderObject* p = r->parent();
            while (p && !p->isRenderBlock())
                p = p->parent();
            if (p)
                p->setChildNeedsLayout(true);
            
            deadObjects.append(r);
        }
    }
    
    for (unsigned i = 0; i < deadObjects.size(); i++)
        removePositionedObject(deadObjects.at(i));
}

void RenderBlock::removeFloatingObjects()
{
    if (!m_floatingObjects)
        return;

    deleteAllValues(m_floatingObjects->set());
    m_floatingObjects->clear();
}

RenderBlock::FloatingObject* RenderBlock::insertFloatingObject(RenderBox* o)
{
    ASSERT(o->isFloating());

    // Create the list of special objects if we don't aleady have one
    if (!m_floatingObjects)
        createFloatingObjects();
    else {
        // Don't insert the object again if it's already in the list
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(o);
        if (it != floatingObjectSet.end())
            return *it;
    }

    // Create the special object entry & append it to the list

    FloatingObject* newObj = new FloatingObject(o->style()->floating());
    
    // Our location is irrelevant if we're unsplittable or no pagination is in effect.
    // Just go ahead and lay out the float.
    bool isChildRenderBlock = o->isRenderBlock();
    if (isChildRenderBlock && !o->needsLayout() && view()->layoutState()->pageLogicalHeightChanged())
        o->setChildNeedsLayout(true, MarkOnlyThis);
            
    bool needsBlockDirectionLocationSetBeforeLayout = isChildRenderBlock && view()->layoutState()->needsBlockDirectionLocationSetBeforeLayout();
    if (!needsBlockDirectionLocationSetBeforeLayout || isWritingModeRoot()) // We are unsplittable if we're a block flow root.
        o->layoutIfNeeded();
    else {
        o->updateLogicalWidth();
        o->computeAndSetBlockDirectionMargins(this);
    }

    setLogicalWidthForFloat(newObj, logicalWidthForChild(o) + marginStartForChild(o) + marginEndForChild(o));

#if ENABLE(CSS_SHAPES)
    if (ShapeOutsideInfo* shapeOutside = o->shapeOutsideInfo())
        shapeOutside->setShapeSize(logicalWidthForChild(o), logicalHeightForChild(o));
#endif

    newObj->setShouldPaint(!o->hasSelfPaintingLayer()); // If a layer exists, the float will paint itself. Otherwise someone else will.
    newObj->setIsDescendant(true);
    newObj->m_renderer = o;

    m_floatingObjects->add(newObj);
    
    return newObj;
}

void RenderBlock::removeFloatingObject(RenderBox* o)
{
    if (m_floatingObjects) {
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(o);
        if (it != floatingObjectSet.end()) {
            FloatingObject* r = *it;
            if (childrenInline()) {
                LayoutUnit logicalTop = logicalTopForFloat(r);
                LayoutUnit logicalBottom = logicalBottomForFloat(r);

                // Fix for https://bugs.webkit.org/show_bug.cgi?id=54995.
                if (logicalBottom < 0 || logicalBottom < logicalTop || logicalTop == LayoutUnit::max())
                    logicalBottom = LayoutUnit::max();
                else {
                    // Special-case zero- and less-than-zero-height floats: those don't touch
                    // the line that they're on, but it still needs to be dirtied. This is
                    // accomplished by pretending they have a height of 1.
                    logicalBottom = max(logicalBottom, logicalTop + 1);
                }
                if (r->m_originatingLine) {
                    if (!selfNeedsLayout()) {
                        ASSERT(r->m_originatingLine->renderer() == this);
                        r->m_originatingLine->markDirty();
                    }
#if !ASSERT_DISABLED
                    r->m_originatingLine = 0;
#endif
                }
                markLinesDirtyInBlockRange(0, logicalBottom);
            }
            m_floatingObjects->remove(r);
            ASSERT(!r->m_originatingLine);
            delete r;
        }
    }
}

void RenderBlock::removeFloatingObjectsBelow(FloatingObject* lastFloat, int logicalOffset)
{
    if (!containsFloats())
        return;
    
    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObject* curr = floatingObjectSet.last();
    while (curr != lastFloat && (!curr->isPlaced() || logicalTopForFloat(curr) >= logicalOffset)) {
        m_floatingObjects->remove(curr);
        ASSERT(!curr->m_originatingLine);
        delete curr;
        if (floatingObjectSet.isEmpty())
            break;
        curr = floatingObjectSet.last();
    }
}

LayoutPoint RenderBlock::computeLogicalLocationForFloat(const FloatingObject* floatingObject, LayoutUnit logicalTopOffset) const
{
    RenderBox* childBox = floatingObject->renderer();
    LayoutUnit logicalLeftOffset = logicalLeftOffsetForContent(logicalTopOffset); // Constant part of left offset.
    LayoutUnit logicalRightOffset; // Constant part of right offset.
#if ENABLE(CSS_SHAPES)
    // FIXME Bug 102948: This only works for shape outside directly set on this block.
    ShapeInsideInfo* shapeInsideInfo = this->shapeInsideInfo();
    // FIXME Bug 102846: Take into account the height of the content. The offset should be
    // equal to the maximum segment length.
    if (shapeInsideInfo && shapeInsideInfo->hasSegments() && shapeInsideInfo->segments().size() == 1) {
        // FIXME Bug 102949: Add support for shapes with multipe segments.

        // The segment offsets are relative to the content box.
        logicalRightOffset = logicalLeftOffset + shapeInsideInfo->segments()[0].logicalRight;
        logicalLeftOffset += shapeInsideInfo->segments()[0].logicalLeft;
    } else
#endif
        logicalRightOffset = logicalRightOffsetForContent(logicalTopOffset);

    LayoutUnit floatLogicalWidth = min(logicalWidthForFloat(floatingObject), logicalRightOffset - logicalLeftOffset); // The width we look for.

    LayoutUnit floatLogicalLeft;

    bool insideFlowThread = flowThreadContainingBlock();

    if (childBox->style()->floating() == LeftFloat) {
        LayoutUnit heightRemainingLeft = 1;
        LayoutUnit heightRemainingRight = 1;
        floatLogicalLeft = logicalLeftOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalLeftOffset, false, &heightRemainingLeft);
        while (logicalRightOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalRightOffset, false, &heightRemainingRight) - floatLogicalLeft < floatLogicalWidth) {
            logicalTopOffset += min(heightRemainingLeft, heightRemainingRight);
            floatLogicalLeft = logicalLeftOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalLeftOffset, false, &heightRemainingLeft);
            if (insideFlowThread) {
                // Have to re-evaluate all of our offsets, since they may have changed.
                logicalRightOffset = logicalRightOffsetForContent(logicalTopOffset); // Constant part of right offset.
                logicalLeftOffset = logicalLeftOffsetForContent(logicalTopOffset); // Constant part of left offset.
                floatLogicalWidth = min(logicalWidthForFloat(floatingObject), logicalRightOffset - logicalLeftOffset);
            }
        }
        floatLogicalLeft = max(logicalLeftOffset - borderAndPaddingLogicalLeft(), floatLogicalLeft);
    } else {
        LayoutUnit heightRemainingLeft = 1;
        LayoutUnit heightRemainingRight = 1;
        floatLogicalLeft = logicalRightOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalRightOffset, false, &heightRemainingRight);
        while (floatLogicalLeft - logicalLeftOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalLeftOffset, false, &heightRemainingLeft) < floatLogicalWidth) {
            logicalTopOffset += min(heightRemainingLeft, heightRemainingRight);
            floatLogicalLeft = logicalRightOffsetForLineIgnoringShapeOutside(logicalTopOffset, logicalRightOffset, false, &heightRemainingRight);
            if (insideFlowThread) {
                // Have to re-evaluate all of our offsets, since they may have changed.
                logicalRightOffset = logicalRightOffsetForContent(logicalTopOffset); // Constant part of right offset.
                logicalLeftOffset = logicalLeftOffsetForContent(logicalTopOffset); // Constant part of left offset.
                floatLogicalWidth = min(logicalWidthForFloat(floatingObject), logicalRightOffset - logicalLeftOffset);
            }
        }
        floatLogicalLeft -= logicalWidthForFloat(floatingObject); // Use the original width of the float here, since the local variable
                                                                  // |floatLogicalWidth| was capped to the available line width.
                                                                  // See fast/block/float/clamped-right-float.html.
    }
    
    return LayoutPoint(floatLogicalLeft, logicalTopOffset);
}

bool RenderBlock::positionNewFloats()
{
    if (!m_floatingObjects)
        return false;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    if (floatingObjectSet.isEmpty())
        return false;

    // If all floats have already been positioned, then we have no work to do.
    if (floatingObjectSet.last()->isPlaced())
        return false;

    // Move backwards through our floating object list until we find a float that has
    // already been positioned.  Then we'll be able to move forward, positioning all of
    // the new floats that need it.
    FloatingObjectSetIterator it = floatingObjectSet.end();
    --it; // Go to last item.
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    FloatingObject* lastPlacedFloatingObject = 0;
    while (it != begin) {
        --it;
        if ((*it)->isPlaced()) {
            lastPlacedFloatingObject = *it;
            ++it;
            break;
        }
    }

    LayoutUnit logicalTop = logicalHeight();
    
    // The float cannot start above the top position of the last positioned float.
    if (lastPlacedFloatingObject)
        logicalTop = max(logicalTopForFloat(lastPlacedFloatingObject), logicalTop);

    FloatingObjectSetIterator end = floatingObjectSet.end();
    // Now walk through the set of unpositioned floats and place them.
    for (; it != end; ++it) {
        FloatingObject* floatingObject = *it;
        // The containing block is responsible for positioning floats, so if we have floats in our
        // list that come from somewhere else, do not attempt to position them.
        if (floatingObject->renderer()->containingBlock() != this)
            continue;

        RenderBox* childBox = floatingObject->renderer();
        LayoutUnit childLogicalLeftMargin = style()->isLeftToRightDirection() ? marginStartForChild(childBox) : marginEndForChild(childBox);

        LayoutRect oldRect = childBox->frameRect();

        if (childBox->style()->clear() & CLEFT)
            logicalTop = max(lowestFloatLogicalBottom(FloatingObject::FloatLeft), logicalTop);
        if (childBox->style()->clear() & CRIGHT)
            logicalTop = max(lowestFloatLogicalBottom(FloatingObject::FloatRight), logicalTop);

        LayoutPoint floatLogicalLocation = computeLogicalLocationForFloat(floatingObject, logicalTop);

        setLogicalLeftForFloat(floatingObject, floatLogicalLocation.x());

        setLogicalLeftForChild(childBox, floatLogicalLocation.x() + childLogicalLeftMargin);
        setLogicalTopForChild(childBox, floatLogicalLocation.y() + marginBeforeForChild(childBox));

        LayoutState* layoutState = view()->layoutState();
        bool isPaginated = layoutState->isPaginated();
        if (isPaginated && !childBox->needsLayout())
            childBox->markForPaginationRelayoutIfNeeded();
        
        childBox->layoutIfNeeded();

        if (isPaginated) {
            // If we are unsplittable and don't fit, then we need to move down.
            // We include our margins as part of the unsplittable area.
            LayoutUnit newLogicalTop = adjustForUnsplittableChild(childBox, floatLogicalLocation.y(), true);
            
            // See if we have a pagination strut that is making us move down further.
            // Note that an unsplittable child can't also have a pagination strut, so this is
            // exclusive with the case above.
            RenderBlock* childBlock = childBox->isRenderBlock() ? toRenderBlock(childBox) : 0;
            if (childBlock && childBlock->paginationStrut()) {
                newLogicalTop += childBlock->paginationStrut();
                childBlock->setPaginationStrut(0);
            }
            
            if (newLogicalTop != floatLogicalLocation.y()) {
                floatingObject->m_paginationStrut = newLogicalTop - floatLogicalLocation.y();

                floatLogicalLocation = computeLogicalLocationForFloat(floatingObject, newLogicalTop);
                setLogicalLeftForFloat(floatingObject, floatLogicalLocation.x());

                setLogicalLeftForChild(childBox, floatLogicalLocation.x() + childLogicalLeftMargin);
                setLogicalTopForChild(childBox, floatLogicalLocation.y() + marginBeforeForChild(childBox));
        
                if (childBlock)
                    childBlock->setChildNeedsLayout(true, MarkOnlyThis);
                childBox->layoutIfNeeded();
            }
        }

        setLogicalTopForFloat(floatingObject, floatLogicalLocation.y());

        setLogicalHeightForFloat(floatingObject, logicalHeightForChild(childBox) + marginBeforeForChild(childBox) + marginAfterForChild(childBox));

        m_floatingObjects->addPlacedObject(floatingObject);

        // If the child moved, we have to repaint it.
        if (childBox->checkForRepaintDuringLayout())
            childBox->repaintDuringLayoutIfMoved(oldRect);
    }
    return true;
}

void RenderBlock::newLine(EClear clear)
{
    positionNewFloats();
    // set y position
    LayoutUnit newY = 0;
    switch (clear)
    {
        case CLEFT:
            newY = lowestFloatLogicalBottom(FloatingObject::FloatLeft);
            break;
        case CRIGHT:
            newY = lowestFloatLogicalBottom(FloatingObject::FloatRight);
            break;
        case CBOTH:
            newY = lowestFloatLogicalBottom();
        default:
            break;
    }
    if (height() < newY)
        setLogicalHeight(newY);
}

void RenderBlock::addPercentHeightDescendant(RenderBox* descendant)
{
    insertIntoTrackedRendererMaps(descendant, gPercentHeightDescendantsMap, gPercentHeightContainerMap);
}

void RenderBlock::removePercentHeightDescendant(RenderBox* descendant)
{
    removeFromTrackedRendererMaps(descendant, gPercentHeightDescendantsMap, gPercentHeightContainerMap);
}

TrackedRendererListHashSet* RenderBlock::percentHeightDescendants() const
{
    return gPercentHeightDescendantsMap ? gPercentHeightDescendantsMap->get(this) : 0;
}

bool RenderBlock::hasPercentHeightContainerMap()
{
    return gPercentHeightContainerMap;
}

bool RenderBlock::hasPercentHeightDescendant(RenderBox* descendant)
{
    // We don't null check gPercentHeightContainerMap since the caller
    // already ensures this and we need to call this function on every
    // descendant in clearPercentHeightDescendantsFrom().
    ASSERT(gPercentHeightContainerMap);
    return gPercentHeightContainerMap->contains(descendant);
}

void RenderBlock::removePercentHeightDescendantIfNeeded(RenderBox* descendant)
{
    // We query the map directly, rather than looking at style's
    // logicalHeight()/logicalMinHeight()/logicalMaxHeight() since those
    // can change with writing mode/directional changes.
    if (!hasPercentHeightContainerMap())
        return;

    if (!hasPercentHeightDescendant(descendant))
        return;

    removePercentHeightDescendant(descendant);
}

void RenderBlock::clearPercentHeightDescendantsFrom(RenderBox* parent)
{
    ASSERT(gPercentHeightContainerMap);
    for (RenderObject* curr = parent->firstChild(); curr; curr = curr->nextInPreOrder(parent)) {
        if (!curr->isBox())
            continue;
 
        RenderBox* box = toRenderBox(curr);
        if (!hasPercentHeightDescendant(box))
            continue;

        removePercentHeightDescendant(box);
    }
}

static bool rangesIntersect(int floatTop, int floatBottom, int objectTop, int objectBottom)
{
    if (objectTop >= floatBottom || objectBottom < floatTop)
        return false;

    // The top of the object overlaps the float
    if (objectTop >= floatTop)
        return true;

    // The object encloses the float
    if (objectTop < floatTop && objectBottom > floatBottom)
        return true;

    // The bottom of the object overlaps the float
    if (objectBottom > objectTop && objectBottom > floatTop && objectBottom <= floatBottom)
        return true;

    return false;
}

template <RenderBlock::FloatingObject::Type FloatTypeValue>
inline void RenderBlock::FloatIntervalSearchAdapter<FloatTypeValue>::collectIfNeeded(const IntervalType& interval) const
{
    const FloatingObject* r = interval.data();
    if (r->type() != FloatTypeValue || !rangesIntersect(interval.low(), interval.high(), m_lowValue, m_highValue))
        return;

    // All the objects returned from the tree should be already placed.
    ASSERT(r->isPlaced() && rangesIntersect(m_renderer->logicalTopForFloat(r), m_renderer->logicalBottomForFloat(r), m_lowValue, m_highValue));

    if (FloatTypeValue == FloatingObject::FloatLeft 
        && m_renderer->logicalRightForFloat(r) > m_offset) {
        m_offset = m_renderer->logicalRightForFloat(r);
        if (m_heightRemaining)
            *m_heightRemaining = m_renderer->logicalBottomForFloat(r) - m_lowValue;
    }

    if (FloatTypeValue == FloatingObject::FloatRight
        && m_renderer->logicalLeftForFloat(r) < m_offset) {
        m_offset = m_renderer->logicalLeftForFloat(r);
        if (m_heightRemaining)
            *m_heightRemaining = m_renderer->logicalBottomForFloat(r) - m_lowValue;
    }

#if ENABLE(CSS_SHAPES)
    m_last = r;
#endif
}

LayoutUnit RenderBlock::textIndentOffset() const
{
    LayoutUnit cw = 0;
    RenderView* renderView = 0;
    if (style()->textIndent().isPercent())
        cw = containingBlock()->availableLogicalWidth();
    else if (style()->textIndent().isViewportPercentage())
        renderView = view();
    return minimumValueForLength(style()->textIndent(), cw, renderView);
}

LayoutUnit RenderBlock::logicalLeftOffsetForContent(RenderRegion* region) const
{
    LayoutUnit logicalLeftOffset = style()->isHorizontalWritingMode() ? borderLeft() + paddingLeft() : borderTop() + paddingTop();
    if (!region)
        return logicalLeftOffset;
    LayoutRect boxRect = borderBoxRectInRegion(region);
    return logicalLeftOffset + (isHorizontalWritingMode() ? boxRect.x() : boxRect.y());
}

LayoutUnit RenderBlock::logicalRightOffsetForContent(RenderRegion* region) const
{
    LayoutUnit logicalRightOffset = style()->isHorizontalWritingMode() ? borderLeft() + paddingLeft() : borderTop() + paddingTop();
    logicalRightOffset += availableLogicalWidth();
    if (!region)
        return logicalRightOffset;
    LayoutRect boxRect = borderBoxRectInRegion(region);
    return logicalRightOffset - (logicalWidth() - (isHorizontalWritingMode() ? boxRect.maxX() : boxRect.maxY()));
}

LayoutUnit RenderBlock::logicalLeftFloatOffsetForLine(LayoutUnit logicalTop, LayoutUnit fixedOffset, LayoutUnit* heightRemaining, LayoutUnit logicalHeight, ShapeOutsideFloatOffsetMode offsetMode) const
{
#if !ENABLE(CSS_SHAPES)
    UNUSED_PARAM(offsetMode);
#endif
    LayoutUnit left = fixedOffset;
    if (m_floatingObjects && m_floatingObjects->hasLeftObjects()) {
        if (heightRemaining)
            *heightRemaining = 1;

        FloatIntervalSearchAdapter<FloatingObject::FloatLeft> adapter(this, roundToInt(logicalTop), roundToInt(logicalTop + logicalHeight), left, heightRemaining);
        m_floatingObjects->placedFloatsTree().allOverlapsWithAdapter(adapter);

#if ENABLE(CSS_SHAPES)
        const FloatingObject* lastFloat = adapter.lastFloat();
        if (offsetMode == ShapeOutsideFloatShapeOffset && lastFloat) {
            if (ShapeOutsideInfo* shapeOutside = lastFloat->renderer()->shapeOutsideInfo()) {
                shapeOutside->computeSegmentsForContainingBlockLine(logicalTop, logicalTopForFloat(lastFloat), logicalHeight);
                left += shapeOutside->rightSegmentMarginBoxDelta();
            }
        }
#endif
    }

    return left;
}

LayoutUnit RenderBlock::adjustLogicalLeftOffsetForLine(LayoutUnit offsetFromFloats, bool applyTextIndent) const
{
    LayoutUnit left = offsetFromFloats;

    if (applyTextIndent && style()->isLeftToRightDirection())
        left += textIndentOffset();

    if (style()->lineAlign() == LineAlignNone)
        return left;
    
    // Push in our left offset so that it is aligned with the character grid.
    LayoutState* layoutState = view()->layoutState();
    if (!layoutState)
        return left;

    RenderBlock* lineGrid = layoutState->lineGrid();
    if (!lineGrid || lineGrid->style()->writingMode() != style()->writingMode())
        return left;

    // FIXME: Should letter-spacing apply? This is complicated since it doesn't apply at the edge?
    float maxCharWidth = lineGrid->style()->font().primaryFont()->maxCharWidth();
    if (!maxCharWidth)
        return left;

    LayoutUnit lineGridOffset = lineGrid->isHorizontalWritingMode() ? layoutState->lineGridOffset().width(): layoutState->lineGridOffset().height();
    LayoutUnit layoutOffset = lineGrid->isHorizontalWritingMode() ? layoutState->layoutOffset().width() : layoutState->layoutOffset().height();
    
    // Push in to the nearest character width (truncated so that we pixel snap left).
    // FIXME: Should be patched when subpixel layout lands, since this calculation doesn't have to pixel snap
    // any more (https://bugs.webkit.org/show_bug.cgi?id=79946).
    // FIXME: This is wrong for RTL (https://bugs.webkit.org/show_bug.cgi?id=79945).
    // FIXME: This doesn't work with columns or regions (https://bugs.webkit.org/show_bug.cgi?id=79942).
    // FIXME: This doesn't work when the inline position of the object isn't set ahead of time.
    // FIXME: Dynamic changes to the font or to the inline position need to result in a deep relayout.
    // (https://bugs.webkit.org/show_bug.cgi?id=79944)
    float remainder = fmodf(maxCharWidth - fmodf(left + layoutOffset - lineGridOffset, maxCharWidth), maxCharWidth);
    left += remainder;
    return left;
}

LayoutUnit RenderBlock::logicalRightFloatOffsetForLine(LayoutUnit logicalTop, LayoutUnit fixedOffset, LayoutUnit* heightRemaining, LayoutUnit logicalHeight, ShapeOutsideFloatOffsetMode offsetMode) const
{
#if !ENABLE(CSS_SHAPES)
    UNUSED_PARAM(offsetMode);
#endif
    LayoutUnit right = fixedOffset;
    if (m_floatingObjects && m_floatingObjects->hasRightObjects()) {
        if (heightRemaining)
            *heightRemaining = 1;

        LayoutUnit rightFloatOffset = fixedOffset;
        FloatIntervalSearchAdapter<FloatingObject::FloatRight> adapter(this, roundToInt(logicalTop), roundToInt(logicalTop + logicalHeight), rightFloatOffset, heightRemaining);
        m_floatingObjects->placedFloatsTree().allOverlapsWithAdapter(adapter);

#if ENABLE(CSS_SHAPES)
        const FloatingObject* lastFloat = adapter.lastFloat();
        if (offsetMode == ShapeOutsideFloatShapeOffset && lastFloat) {
            if (ShapeOutsideInfo* shapeOutside = lastFloat->renderer()->shapeOutsideInfo()) {
                shapeOutside->computeSegmentsForContainingBlockLine(logicalTop, logicalTopForFloat(lastFloat), logicalHeight);
                rightFloatOffset += shapeOutside->leftSegmentMarginBoxDelta();
            }
        }
#endif

        right = min(right, rightFloatOffset);
    }

    return right;
}

LayoutUnit RenderBlock::adjustLogicalRightOffsetForLine(LayoutUnit offsetFromFloats, bool applyTextIndent) const
{
    LayoutUnit right = offsetFromFloats;
    
    if (applyTextIndent && !style()->isLeftToRightDirection())
        right -= textIndentOffset();
    
    if (style()->lineAlign() == LineAlignNone)
        return right;
    
    // Push in our right offset so that it is aligned with the character grid.
    LayoutState* layoutState = view()->layoutState();
    if (!layoutState)
        return right;

    RenderBlock* lineGrid = layoutState->lineGrid();
    if (!lineGrid || lineGrid->style()->writingMode() != style()->writingMode())
        return right;

    // FIXME: Should letter-spacing apply? This is complicated since it doesn't apply at the edge?
    float maxCharWidth = lineGrid->style()->font().primaryFont()->maxCharWidth();
    if (!maxCharWidth)
        return right;

    LayoutUnit lineGridOffset = lineGrid->isHorizontalWritingMode() ? layoutState->lineGridOffset().width(): layoutState->lineGridOffset().height();
    LayoutUnit layoutOffset = lineGrid->isHorizontalWritingMode() ? layoutState->layoutOffset().width() : layoutState->layoutOffset().height();
    
    // Push in to the nearest character width (truncated so that we pixel snap right).
    // FIXME: Should be patched when subpixel layout lands, since this calculation doesn't have to pixel snap
    // any more (https://bugs.webkit.org/show_bug.cgi?id=79946).
    // FIXME: This is wrong for RTL (https://bugs.webkit.org/show_bug.cgi?id=79945).
    // FIXME: This doesn't work with columns or regions (https://bugs.webkit.org/show_bug.cgi?id=79942).
    // FIXME: This doesn't work when the inline position of the object isn't set ahead of time.
    // FIXME: Dynamic changes to the font or to the inline position need to result in a deep relayout.
    // (https://bugs.webkit.org/show_bug.cgi?id=79944)
    float remainder = fmodf(fmodf(right + layoutOffset - lineGridOffset, maxCharWidth), maxCharWidth);
    right -= ceilf(remainder);
    return right;
}

LayoutUnit RenderBlock::nextFloatLogicalBottomBelow(LayoutUnit logicalHeight) const
{
    if (!m_floatingObjects)
        return logicalHeight;

    LayoutUnit bottom = LayoutUnit::max();
    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        LayoutUnit floatBottom = logicalBottomForFloat(r);
        if (floatBottom > logicalHeight)
            bottom = min(floatBottom, bottom);
    }

    return bottom == LayoutUnit::max() ? LayoutUnit() : bottom;
}

LayoutUnit RenderBlock::lowestFloatLogicalBottom(FloatingObject::Type floatType) const
{
    if (!m_floatingObjects)
        return 0;
    LayoutUnit lowestFloatBottom = 0;
    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        if (r->isPlaced() && r->type() & floatType)
            lowestFloatBottom = max(lowestFloatBottom, logicalBottomForFloat(r));
    }
    return lowestFloatBottom;
}

void RenderBlock::markLinesDirtyInBlockRange(LayoutUnit logicalTop, LayoutUnit logicalBottom, RootInlineBox* highest)
{
    if (logicalTop >= logicalBottom)
        return;

    RootInlineBox* lowestDirtyLine = lastRootBox();
    RootInlineBox* afterLowest = lowestDirtyLine;
    while (lowestDirtyLine && lowestDirtyLine->lineBottomWithLeading() >= logicalBottom && logicalBottom < LayoutUnit::max()) {
        afterLowest = lowestDirtyLine;
        lowestDirtyLine = lowestDirtyLine->prevRootBox();
    }

    while (afterLowest && afterLowest != highest && (afterLowest->lineBottomWithLeading() >= logicalTop || afterLowest->lineBottomWithLeading() < 0)) {
        afterLowest->markDirty();
        afterLowest = afterLowest->prevRootBox();
    }
}

void RenderBlock::clearFloats()
{
    if (m_floatingObjects)
        m_floatingObjects->setHorizontalWritingMode(isHorizontalWritingMode());

    HashSet<RenderBox*> oldIntrudingFloatSet;
    if (!childrenInline() && m_floatingObjects) {
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
            FloatingObject* floatingObject = *it;
            if (!floatingObject->isDescendant())
                oldIntrudingFloatSet.add(floatingObject->m_renderer);
        }
    }

    // Inline blocks are covered by the isReplaced() check in the avoidFloats method.
    if (avoidsFloats() || isRoot() || isRenderView() || isFloatingOrOutOfFlowPositioned() || isTableCell()) {
        if (m_floatingObjects) {
            deleteAllValues(m_floatingObjects->set());
            m_floatingObjects->clear();
        }
        if (!oldIntrudingFloatSet.isEmpty())
            markAllDescendantsWithFloatsForLayout();
        return;
    }

    typedef HashMap<RenderObject*, FloatingObject*> RendererToFloatInfoMap;
    RendererToFloatInfoMap floatMap;

    if (m_floatingObjects) {
        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        if (childrenInline()) {
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* f = *it;
                floatMap.add(f->m_renderer, f);
            }
        } else
            deleteAllValues(floatingObjectSet);
        m_floatingObjects->clear();
    }

    // We should not process floats if the parent node is not a RenderBlock. Otherwise, we will add 
    // floats in an invalid context. This will cause a crash arising from a bad cast on the parent.
    // See <rdar://problem/8049753>, where float property is applied on a text node in a SVG.
    if (!parent() || !parent()->isRenderBlock())
        return;

    // Attempt to locate a previous sibling with overhanging floats.  We skip any elements that are
    // out of flow (like floating/positioned elements), and we also skip over any objects that may have shifted
    // to avoid floats.
    RenderBlock* parentBlock = toRenderBlock(parent());
    bool parentHasFloats = false;
    RenderObject* prev = previousSibling();
    while (prev && (prev->isFloatingOrOutOfFlowPositioned() || !prev->isBox() || !prev->isRenderBlock() || toRenderBlock(prev)->avoidsFloats())) {
        if (prev->isFloating())
            parentHasFloats = true;
        prev = prev->previousSibling();
    }

    // First add in floats from the parent.
    LayoutUnit logicalTopOffset = logicalTop();
    if (parentHasFloats)
        addIntrudingFloats(parentBlock, parentBlock->logicalLeftOffsetForContent(), logicalTopOffset);
    
    LayoutUnit logicalLeftOffset = 0;
    if (prev)
        logicalTopOffset -= toRenderBox(prev)->logicalTop();
    else {
        prev = parentBlock;
        logicalLeftOffset += parentBlock->logicalLeftOffsetForContent();
    }

    // Add overhanging floats from the previous RenderBlock, but only if it has a float that intrudes into our space.    
    RenderBlock* block = toRenderBlock(prev);
    if (block->m_floatingObjects && block->lowestFloatLogicalBottom() > logicalTopOffset)
        addIntrudingFloats(block, logicalLeftOffset, logicalTopOffset);

    if (childrenInline()) {
        LayoutUnit changeLogicalTop = LayoutUnit::max();
        LayoutUnit changeLogicalBottom = LayoutUnit::min();
        if (m_floatingObjects) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* f = *it;
                FloatingObject* oldFloatingObject = floatMap.get(f->m_renderer);
                LayoutUnit logicalBottom = logicalBottomForFloat(f);
                if (oldFloatingObject) {
                    LayoutUnit oldLogicalBottom = logicalBottomForFloat(oldFloatingObject);
                    if (logicalWidthForFloat(f) != logicalWidthForFloat(oldFloatingObject) || logicalLeftForFloat(f) != logicalLeftForFloat(oldFloatingObject)) {
                        changeLogicalTop = 0;
                        changeLogicalBottom = max(changeLogicalBottom, max(logicalBottom, oldLogicalBottom));
                    } else {
                        if (logicalBottom != oldLogicalBottom) {
                            changeLogicalTop = min(changeLogicalTop, min(logicalBottom, oldLogicalBottom));
                            changeLogicalBottom = max(changeLogicalBottom, max(logicalBottom, oldLogicalBottom));
                        }
                        LayoutUnit logicalTop = logicalTopForFloat(f);
                        LayoutUnit oldLogicalTop = logicalTopForFloat(oldFloatingObject);
                        if (logicalTop != oldLogicalTop) {
                            changeLogicalTop = min(changeLogicalTop, min(logicalTop, oldLogicalTop));
                            changeLogicalBottom = max(changeLogicalBottom, max(logicalTop, oldLogicalTop));
                        }
                    }

                    floatMap.remove(f->m_renderer);
                    if (oldFloatingObject->m_originatingLine && !selfNeedsLayout()) {
                        ASSERT(oldFloatingObject->m_originatingLine->renderer() == this);
                        oldFloatingObject->m_originatingLine->markDirty();
                    }
                    delete oldFloatingObject;
                } else {
                    changeLogicalTop = 0;
                    changeLogicalBottom = max(changeLogicalBottom, logicalBottom);
                }
            }
        }

        RendererToFloatInfoMap::iterator end = floatMap.end();
        for (RendererToFloatInfoMap::iterator it = floatMap.begin(); it != end; ++it) {
            FloatingObject* floatingObject = (*it).value;
            if (!floatingObject->isDescendant()) {
                changeLogicalTop = 0;
                changeLogicalBottom = max(changeLogicalBottom, logicalBottomForFloat(floatingObject));
            }
        }
        deleteAllValues(floatMap);

        markLinesDirtyInBlockRange(changeLogicalTop, changeLogicalBottom);
    } else if (!oldIntrudingFloatSet.isEmpty()) {
        // If there are previously intruding floats that no longer intrude, then children with floats
        // should also get layout because they might need their floating object lists cleared.
        if (m_floatingObjects->set().size() < oldIntrudingFloatSet.size())
            markAllDescendantsWithFloatsForLayout();
        else {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end && !oldIntrudingFloatSet.isEmpty(); ++it)
                oldIntrudingFloatSet.remove((*it)->m_renderer);
            if (!oldIntrudingFloatSet.isEmpty())
                markAllDescendantsWithFloatsForLayout();
        }
    }
}

LayoutUnit RenderBlock::addOverhangingFloats(RenderBlock* child, bool makeChildPaintOtherFloats)
{
    // Prevent floats from being added to the canvas by the root element, e.g., <html>.
    if (child->hasOverflowClip() || !child->containsFloats() || child->isRoot() || child->hasColumns() || child->isWritingModeRoot())
        return 0;

    LayoutUnit childLogicalTop = child->logicalTop();
    LayoutUnit childLogicalLeft = child->logicalLeft();
    LayoutUnit lowestFloatLogicalBottom = 0;

    // Floats that will remain the child's responsibility to paint should factor into its
    // overflow.
    FloatingObjectSetIterator childEnd = child->m_floatingObjects->set().end();
    for (FloatingObjectSetIterator childIt = child->m_floatingObjects->set().begin(); childIt != childEnd; ++childIt) {
        FloatingObject* r = *childIt;
        LayoutUnit logicalBottomForFloat = min(this->logicalBottomForFloat(r), LayoutUnit::max() - childLogicalTop);
        LayoutUnit logicalBottom = childLogicalTop + logicalBottomForFloat;
        lowestFloatLogicalBottom = max(lowestFloatLogicalBottom, logicalBottom);

        if (logicalBottom > logicalHeight()) {
            // If the object is not in the list, we add it now.
            if (!containsFloat(r->m_renderer)) {
                LayoutSize offset = isHorizontalWritingMode() ? LayoutSize(-childLogicalLeft, -childLogicalTop) : LayoutSize(-childLogicalTop, -childLogicalLeft);
                FloatingObject* floatingObj = new FloatingObject(r->type(), LayoutRect(r->frameRect().location() - offset, r->frameRect().size()));
                floatingObj->m_renderer = r->m_renderer;

                // The nearest enclosing layer always paints the float (so that zindex and stacking
                // behaves properly).  We always want to propagate the desire to paint the float as
                // far out as we can, to the outermost block that overlaps the float, stopping only
                // if we hit a self-painting layer boundary.
                if (r->m_renderer->enclosingFloatPaintingLayer() == enclosingFloatPaintingLayer())
                    r->setShouldPaint(false);
                else
                    floatingObj->setShouldPaint(false);

                floatingObj->setIsDescendant(true);

                // We create the floating object list lazily.
                if (!m_floatingObjects)
                    createFloatingObjects();
                m_floatingObjects->add(floatingObj);
            }
        } else {
            if (makeChildPaintOtherFloats && !r->shouldPaint() && !r->m_renderer->hasSelfPaintingLayer()
                && r->m_renderer->isDescendantOf(child) && r->m_renderer->enclosingFloatPaintingLayer() == child->enclosingFloatPaintingLayer()) {
                // The float is not overhanging from this block, so if it is a descendant of the child, the child should
                // paint it (the other case is that it is intruding into the child), unless it has its own layer or enclosing
                // layer.
                // If makeChildPaintOtherFloats is false, it means that the child must already know about all the floats
                // it should paint.
                r->setShouldPaint(true);
            }
            
            // Since the float doesn't overhang, it didn't get put into our list.  We need to go ahead and add its overflow in to the
            // child now.
            if (r->isDescendant())
                child->addOverflowFromChild(r->m_renderer, LayoutSize(xPositionForFloatIncludingMargin(r), yPositionForFloatIncludingMargin(r)));
        }
    }
    return lowestFloatLogicalBottom;
}

bool RenderBlock::hasOverhangingFloat(RenderBox* renderer)
{
    if (!m_floatingObjects || hasColumns() || !parent())
        return false;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(renderer);
    if (it == floatingObjectSet.end())
        return false;

    return logicalBottomForFloat(*it) > logicalHeight();
}

void RenderBlock::addIntrudingFloats(RenderBlock* prev, LayoutUnit logicalLeftOffset, LayoutUnit logicalTopOffset)
{
    ASSERT(!avoidsFloats());

    // If the parent or previous sibling doesn't have any floats to add, don't bother.
    if (!prev->m_floatingObjects)
        return;

    logicalLeftOffset += marginLogicalLeft();

    const FloatingObjectSet& prevSet = prev->m_floatingObjects->set();
    FloatingObjectSetIterator prevEnd = prevSet.end();
    for (FloatingObjectSetIterator prevIt = prevSet.begin(); prevIt != prevEnd; ++prevIt) {
        FloatingObject* r = *prevIt;
        if (logicalBottomForFloat(r) > logicalTopOffset) {
            if (!m_floatingObjects || !m_floatingObjects->set().contains(r)) {
                LayoutSize offset = isHorizontalWritingMode() ? LayoutSize(logicalLeftOffset, logicalTopOffset) : LayoutSize(logicalTopOffset, logicalLeftOffset);
                FloatingObject* floatingObj = new FloatingObject(r->type(), LayoutRect(r->frameRect().location() - offset, r->frameRect().size()));

                // Applying the child's margin makes no sense in the case where the child was passed in.
                // since this margin was added already through the modification of the |logicalLeftOffset| variable
                // above.  |logicalLeftOffset| will equal the margin in this case, so it's already been taken
                // into account.  Only apply this code if prev is the parent, since otherwise the left margin
                // will get applied twice.
                if (prev != parent()) {
                    if (isHorizontalWritingMode())
                        floatingObj->setX(floatingObj->x() + prev->marginLeft());
                    else
                        floatingObj->setY(floatingObj->y() + prev->marginTop());
                }
               
                floatingObj->setShouldPaint(false); // We are not in the direct inheritance chain for this float. We will never paint it.
                floatingObj->m_renderer = r->m_renderer;
                
                // We create the floating object list lazily.
                if (!m_floatingObjects)
                    createFloatingObjects();
                m_floatingObjects->add(floatingObj);
            }
        }
    }
}

bool RenderBlock::avoidsFloats() const
{
    // Floats can't intrude into our box if we have a non-auto column count or width.
    return RenderBox::avoidsFloats() || !style()->hasAutoColumnCount() || !style()->hasAutoColumnWidth();
}

bool RenderBlock::containsFloat(RenderBox* renderer) const
{
    return m_floatingObjects && m_floatingObjects->set().contains<RenderBox*, FloatingObjectHashTranslator>(renderer);
}

void RenderBlock::markAllDescendantsWithFloatsForLayout(RenderBox* floatToRemove, bool inLayout)
{
    if (!everHadLayout() && !containsFloats())
        return;

    MarkingBehavior markParents = inLayout ? MarkOnlyThis : MarkContainingBlockChain;
    setChildNeedsLayout(true, markParents);
 
    if (floatToRemove)
        removeFloatingObject(floatToRemove);

    // Iterate over our children and mark them as needed.
    if (!childrenInline()) {
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if ((!floatToRemove && child->isFloatingOrOutOfFlowPositioned()) || !child->isRenderBlock())
                continue;
            RenderBlock* childBlock = toRenderBlock(child);
            if ((floatToRemove ? childBlock->containsFloat(floatToRemove) : childBlock->containsFloats()) || childBlock->shrinkToAvoidFloats())
                childBlock->markAllDescendantsWithFloatsForLayout(floatToRemove, inLayout);
        }
    }
}

void RenderBlock::markSiblingsWithFloatsForLayout(RenderBox* floatToRemove)
{
    if (!m_floatingObjects)
        return;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();

    for (RenderObject* next = nextSibling(); next; next = next->nextSibling()) {
        if (!next->isRenderBlock() || next->isFloatingOrOutOfFlowPositioned() || toRenderBlock(next)->avoidsFloats())
            continue;

        RenderBlock* nextBlock = toRenderBlock(next);
        for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
            RenderBox* floatingBox = (*it)->renderer();
            if (floatToRemove && floatingBox != floatToRemove)
                continue;
            if (nextBlock->containsFloat(floatingBox))
                nextBlock->markAllDescendantsWithFloatsForLayout(floatingBox);
        }
    }
}

LayoutUnit RenderBlock::getClearDelta(RenderBox* child, LayoutUnit logicalTop)
{
    // There is no need to compute clearance if we have no floats.
    if (!containsFloats())
        return 0;
    
    // At least one float is present.  We need to perform the clearance computation.
    bool clearSet = child->style()->clear() != CNONE;
    LayoutUnit logicalBottom = 0;
    switch (child->style()->clear()) {
        case CNONE:
            break;
        case CLEFT:
            logicalBottom = lowestFloatLogicalBottom(FloatingObject::FloatLeft);
            break;
        case CRIGHT:
            logicalBottom = lowestFloatLogicalBottom(FloatingObject::FloatRight);
            break;
        case CBOTH:
            logicalBottom = lowestFloatLogicalBottom();
            break;
    }

    // We also clear floats if we are too big to sit on the same line as a float (and wish to avoid floats by default).
    LayoutUnit result = clearSet ? max<LayoutUnit>(0, logicalBottom - logicalTop) : LayoutUnit();
    if (!result && child->avoidsFloats()) {
        LayoutUnit newLogicalTop = logicalTop;
        while (true) {
            LayoutUnit availableLogicalWidthAtNewLogicalTopOffset = availableLogicalWidthForLine(newLogicalTop, false, logicalHeightForChild(child));
            if (availableLogicalWidthAtNewLogicalTopOffset == availableLogicalWidthForContent(newLogicalTop))
                return newLogicalTop - logicalTop;

            RenderRegion* region = regionAtBlockOffset(logicalTopForChild(child));
            LayoutRect borderBox = child->borderBoxRectInRegion(region, DoNotCacheRenderBoxRegionInfo);
            LayoutUnit childLogicalWidthAtOldLogicalTopOffset = isHorizontalWritingMode() ? borderBox.width() : borderBox.height();

            // FIXME: None of this is right for perpendicular writing-mode children.
            LayoutUnit childOldLogicalWidth = child->logicalWidth();
            LayoutUnit childOldMarginLeft = child->marginLeft();
            LayoutUnit childOldMarginRight = child->marginRight();
            LayoutUnit childOldLogicalTop = child->logicalTop();

            child->setLogicalTop(newLogicalTop);
            child->updateLogicalWidth();
            region = regionAtBlockOffset(logicalTopForChild(child));
            borderBox = child->borderBoxRectInRegion(region, DoNotCacheRenderBoxRegionInfo);
            LayoutUnit childLogicalWidthAtNewLogicalTopOffset = isHorizontalWritingMode() ? borderBox.width() : borderBox.height();

            child->setLogicalTop(childOldLogicalTop);
            child->setLogicalWidth(childOldLogicalWidth);
            child->setMarginLeft(childOldMarginLeft);
            child->setMarginRight(childOldMarginRight);
            
            if (childLogicalWidthAtNewLogicalTopOffset <= availableLogicalWidthAtNewLogicalTopOffset) {
                // Even though we may not be moving, if the logical width did shrink because of the presence of new floats, then
                // we need to force a relayout as though we shifted. This happens because of the dynamic addition of overhanging floats
                // from previous siblings when negative margins exist on a child (see the addOverhangingFloats call at the end of collapseMargins).
                if (childLogicalWidthAtOldLogicalTopOffset != childLogicalWidthAtNewLogicalTopOffset)
                    child->setChildNeedsLayout(true, MarkOnlyThis);
                return newLogicalTop - logicalTop;
            }

            newLogicalTop = nextFloatLogicalBottomBelow(newLogicalTop);
            ASSERT(newLogicalTop >= logicalTop);
            if (newLogicalTop < logicalTop)
                break;
        }
        ASSERT_NOT_REACHED();
    }
    return result;
}

bool RenderBlock::isPointInOverflowControl(HitTestResult& result, const LayoutPoint& locationInContainer, const LayoutPoint& accumulatedOffset)
{
    if (!scrollsOverflow())
        return false;

    return layer()->hitTestOverflowControls(result, roundedIntPoint(locationInContainer - toLayoutSize(accumulatedOffset)));
}

Node* RenderBlock::nodeForHitTest() const
{
    // If we are in the margins of block elements that are part of a
    // continuation we're actually still inside the enclosing element
    // that was split. Use the appropriate inner node.
    return isAnonymousBlockContinuation() ? continuation()->node() : node();
}

bool RenderBlock::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    LayoutPoint adjustedLocation(accumulatedOffset + location());
    LayoutSize localOffset = toLayoutSize(adjustedLocation);

    if (!isRenderView()) {
        // Check if we need to do anything at all.
        LayoutRect overflowBox = visualOverflowRect();
        flipForWritingMode(overflowBox);
        overflowBox.moveBy(adjustedLocation);
        if (!locationInContainer.intersects(overflowBox))
            return false;
    }

    if ((hitTestAction == HitTestBlockBackground || hitTestAction == HitTestChildBlockBackground) && isPointInOverflowControl(result, locationInContainer.point(), adjustedLocation)) {
        updateHitTestResult(result, locationInContainer.point() - localOffset);
        // FIXME: isPointInOverflowControl() doesn't handle rect-based tests yet.
        if (!result.addNodeToRectBasedTestResult(nodeForHitTest(), request, locationInContainer))
           return true;
    }

    // If we have clipping, then we can't have any spillout.
    bool useOverflowClip = hasOverflowClip() && !hasSelfPaintingLayer();
    bool useClip = (hasControlClip() || useOverflowClip);
    bool checkChildren = !useClip || (hasControlClip() ? locationInContainer.intersects(controlClipRect(adjustedLocation)) : locationInContainer.intersects(overflowClipRect(adjustedLocation, locationInContainer.region(), IncludeOverlayScrollbarSize)));
    if (checkChildren) {
        // Hit test descendants first.
        LayoutSize scrolledOffset(localOffset);
        if (hasOverflowClip())
            scrolledOffset -= scrolledContentOffset();

        // Hit test contents if we don't have columns.
        if (!hasColumns()) {
            if (hitTestContents(request, result, locationInContainer, toLayoutPoint(scrolledOffset), hitTestAction)) {
                updateHitTestResult(result, flipForWritingMode(locationInContainer.point() - localOffset));
                return true;
            }
            if (hitTestAction == HitTestFloat && hitTestFloats(request, result, locationInContainer, toLayoutPoint(scrolledOffset)))
                return true;
        } else if (hitTestColumns(request, result, locationInContainer, toLayoutPoint(scrolledOffset), hitTestAction)) {
            updateHitTestResult(result, flipForWritingMode(locationInContainer.point() - localOffset));
            return true;
        }
    }

    // Check if the point is outside radii.
    if (!isRenderView() && style()->hasBorderRadius()) {
        LayoutRect borderRect = borderBoxRect();
        borderRect.moveBy(adjustedLocation);
        RoundedRect border = style()->getRoundedBorderFor(borderRect, view());
        if (!locationInContainer.intersects(border))
            return false;
    }

    // Now hit test our background
    if (hitTestAction == HitTestBlockBackground || hitTestAction == HitTestChildBlockBackground) {
        LayoutRect boundsRect(adjustedLocation, size());
        if (visibleToHitTesting() && locationInContainer.intersects(boundsRect)) {
            updateHitTestResult(result, flipForWritingMode(locationInContainer.point() - localOffset));
            if (!result.addNodeToRectBasedTestResult(nodeForHitTest(), request, locationInContainer, boundsRect))
                return true;
        }
    }

    return false;
}

bool RenderBlock::hitTestFloats(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset)
{
    if (!m_floatingObjects)
        return false;

    LayoutPoint adjustedLocation = accumulatedOffset;
    if (isRenderView()) {
        adjustedLocation += toLayoutSize(toRenderView(this)->frameView()->scrollPosition());
    }

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    for (FloatingObjectSetIterator it = floatingObjectSet.end(); it != begin;) {
        --it;
        FloatingObject* floatingObject = *it;
        if (floatingObject->shouldPaint() && !floatingObject->m_renderer->hasSelfPaintingLayer()) {
            LayoutUnit xOffset = xPositionForFloatIncludingMargin(floatingObject) - floatingObject->m_renderer->x();
            LayoutUnit yOffset = yPositionForFloatIncludingMargin(floatingObject) - floatingObject->m_renderer->y();
            LayoutPoint childPoint = flipFloatForWritingModeForChild(floatingObject, adjustedLocation + LayoutSize(xOffset, yOffset));
            if (floatingObject->m_renderer->hitTest(request, result, locationInContainer, childPoint)) {
                updateHitTestResult(result, locationInContainer.point() - toLayoutSize(childPoint));
                return true;
            }
        }
    }

    return false;
}

class ColumnRectIterator {
    WTF_MAKE_NONCOPYABLE(ColumnRectIterator);
public:
    ColumnRectIterator(const RenderBlock& block)
        : m_block(block)
        , m_colInfo(block.columnInfo())
        , m_direction(m_block.style()->isFlippedBlocksWritingMode() ? 1 : -1)
        , m_isHorizontal(block.isHorizontalWritingMode())
        , m_logicalLeft(block.logicalLeftOffsetForContent())
    {
        int colCount = m_colInfo->columnCount();
        m_colIndex = colCount - 1;
        
        m_currLogicalTopOffset = m_block.initialBlockOffsetForPainting();
        m_currLogicalTopOffset = colCount * m_block.blockDeltaForPaintingNextColumn();
        
        update();
    }

    void advance()
    {
        ASSERT(hasMore());
        m_colIndex--;
        update();
    }

    LayoutRect columnRect() const { return m_colRect; }
    bool hasMore() const { return m_colIndex >= 0; }

    void adjust(LayoutSize& offset) const
    {
        LayoutUnit currLogicalLeftOffset = (m_isHorizontal ? m_colRect.x() : m_colRect.y()) - m_logicalLeft;
        offset += m_isHorizontal ? LayoutSize(currLogicalLeftOffset, m_currLogicalTopOffset) : LayoutSize(m_currLogicalTopOffset, currLogicalLeftOffset);
    }

private:
    void update()
    {
        if (m_colIndex < 0)
            return;
        m_colRect = m_block.columnRectAt(const_cast<ColumnInfo*>(m_colInfo), m_colIndex);
        m_block.flipForWritingMode(m_colRect);
        m_currLogicalTopOffset -= m_block.blockDeltaForPaintingNextColumn();
    }

    const RenderBlock& m_block;
    const ColumnInfo* const m_colInfo;
    const int m_direction;
    const bool m_isHorizontal;
    const LayoutUnit m_logicalLeft;
    int m_colIndex;
    LayoutUnit m_currLogicalTopOffset;
    LayoutRect m_colRect;
};

bool RenderBlock::hitTestColumns(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    // We need to do multiple passes, breaking up our hit testing into strips.
    if (!hasColumns())
        return false;

    for (ColumnRectIterator it(*this); it.hasMore(); it.advance()) {
        LayoutRect hitRect = locationInContainer.boundingBox();
        LayoutRect colRect = it.columnRect();
        colRect.moveBy(accumulatedOffset);
        if (locationInContainer.intersects(colRect)) {
            // The point is inside this column.
            // Adjust accumulatedOffset to change where we hit test.
            LayoutSize offset;
            it.adjust(offset);
            LayoutPoint finalLocation = accumulatedOffset + offset;
            if (!result.isRectBasedTest() || colRect.contains(hitRect))
                return hitTestContents(request, result, locationInContainer, finalLocation, hitTestAction) || (hitTestAction == HitTestFloat && hitTestFloats(request, result, locationInContainer, finalLocation));

            hitTestContents(request, result, locationInContainer, finalLocation, hitTestAction);
        }
    }

    return false;
}

void RenderBlock::adjustForColumnRect(LayoutSize& offset, const LayoutPoint& locationInContainer) const
{
    for (ColumnRectIterator it(*this); it.hasMore(); it.advance()) {
        LayoutRect colRect = it.columnRect();
        if (colRect.contains(locationInContainer)) {
            it.adjust(offset);
            return;
        }
    }
}

bool RenderBlock::hitTestContents(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (childrenInline() && !isTable()) {
        // We have to hit-test our line boxes.
        if (m_lineBoxes.hitTest(this, request, result, locationInContainer, accumulatedOffset, hitTestAction))
            return true;
    } else {
        // Hit test our children.
        HitTestAction childHitTest = hitTestAction;
        if (hitTestAction == HitTestChildBlockBackgrounds)
            childHitTest = HitTestChildBlockBackground;
        for (RenderBox* child = lastChildBox(); child; child = child->previousSiblingBox()) {
            LayoutPoint childPoint = flipForWritingModeForChild(child, accumulatedOffset);
            if (!child->hasSelfPaintingLayer() && !child->isFloating() && child->nodeAtPoint(request, result, locationInContainer, childPoint, childHitTest))
                return true;
        }
    }

    return false;
}

Position RenderBlock::positionForBox(InlineBox *box, bool start) const
{
    if (!box)
        return Position();

    if (!box->renderer()->nonPseudoNode())
        return createLegacyEditingPosition(nonPseudoNode(), start ? caretMinOffset() : caretMaxOffset());

    if (!box->isInlineTextBox())
        return createLegacyEditingPosition(box->renderer()->nonPseudoNode(), start ? box->renderer()->caretMinOffset() : box->renderer()->caretMaxOffset());

    InlineTextBox* textBox = toInlineTextBox(box);
    return createLegacyEditingPosition(box->renderer()->nonPseudoNode(), start ? textBox->start() : textBox->start() + textBox->len());
}

static inline bool isEditingBoundary(RenderObject* ancestor, RenderObject* child)
{
    ASSERT(!ancestor || ancestor->nonPseudoNode());
    ASSERT(child && child->nonPseudoNode());
    return !ancestor || !ancestor->parent() || (ancestor->hasLayer() && ancestor->parent()->isRenderView())
        || ancestor->nonPseudoNode()->rendererIsEditable() == child->nonPseudoNode()->rendererIsEditable();
}

// FIXME: This function should go on RenderObject as an instance method. Then
// all cases in which positionForPoint recurs could call this instead to
// prevent crossing editable boundaries. This would require many tests.
static VisiblePosition positionForPointRespectingEditingBoundaries(RenderBlock* parent, RenderBox* child, const LayoutPoint& pointInParentCoordinates)
{
    LayoutPoint childLocation = child->location();
    if (child->isInFlowPositioned())
        childLocation += child->offsetForInFlowPosition();

    // FIXME: This is wrong if the child's writing-mode is different from the parent's.
    LayoutPoint pointInChildCoordinates(toLayoutPoint(pointInParentCoordinates - childLocation));

    // If this is an anonymous renderer, we just recur normally
    Node* childNode = child->nonPseudoNode();
    if (!childNode)
        return child->positionForPoint(pointInChildCoordinates);

    // Otherwise, first make sure that the editability of the parent and child agree.
    // If they don't agree, then we return a visible position just before or after the child
    RenderObject* ancestor = parent;
    while (ancestor && !ancestor->nonPseudoNode())
        ancestor = ancestor->parent();

    // If we can't find an ancestor to check editability on, or editability is unchanged, we recur like normal
    if (isEditingBoundary(ancestor, child))
        return child->positionForPoint(pointInChildCoordinates);

    // Otherwise return before or after the child, depending on if the click was to the logical left or logical right of the child
    LayoutUnit childMiddle = parent->logicalWidthForChild(child) / 2;
    LayoutUnit logicalLeft = parent->isHorizontalWritingMode() ? pointInChildCoordinates.x() : pointInChildCoordinates.y();
    if (logicalLeft < childMiddle)
        return ancestor->createVisiblePosition(childNode->nodeIndex(), DOWNSTREAM);
    return ancestor->createVisiblePosition(childNode->nodeIndex() + 1, UPSTREAM);
}

VisiblePosition RenderBlock::positionForPointWithInlineChildren(const LayoutPoint& pointInLogicalContents)
{
    ASSERT(childrenInline());

    if (!firstRootBox())
        return createVisiblePosition(0, DOWNSTREAM);

    bool linesAreFlipped = style()->isFlippedLinesWritingMode();
    bool blocksAreFlipped = style()->isFlippedBlocksWritingMode();

    // look for the closest line box in the root box which is at the passed-in y coordinate
    InlineBox* closestBox = 0;
    RootInlineBox* firstRootBoxWithChildren = 0;
    RootInlineBox* lastRootBoxWithChildren = 0;
    for (RootInlineBox* root = firstRootBox(); root; root = root->nextRootBox()) {
        if (!root->firstLeafChild())
            continue;
        if (!firstRootBoxWithChildren)
            firstRootBoxWithChildren = root;

        if (!linesAreFlipped && root->isFirstAfterPageBreak() && (pointInLogicalContents.y() < root->lineTopWithLeading()
            || (blocksAreFlipped && pointInLogicalContents.y() == root->lineTopWithLeading())))
            break;

        lastRootBoxWithChildren = root;

        // check if this root line box is located at this y coordinate
        if (pointInLogicalContents.y() < root->selectionBottom() || (blocksAreFlipped && pointInLogicalContents.y() == root->selectionBottom())) {
            if (linesAreFlipped) {
                RootInlineBox* nextRootBoxWithChildren = root->nextRootBox();
                while (nextRootBoxWithChildren && !nextRootBoxWithChildren->firstLeafChild())
                    nextRootBoxWithChildren = nextRootBoxWithChildren->nextRootBox();

                if (nextRootBoxWithChildren && nextRootBoxWithChildren->isFirstAfterPageBreak() && (pointInLogicalContents.y() > nextRootBoxWithChildren->lineTopWithLeading()
                    || (!blocksAreFlipped && pointInLogicalContents.y() == nextRootBoxWithChildren->lineTopWithLeading())))
                    continue;
            }
            closestBox = root->closestLeafChildForLogicalLeftPosition(pointInLogicalContents.x());
            if (closestBox)
                break;
        }
    }

    bool moveCaretToBoundary = document()->frame()->editor().behavior().shouldMoveCaretToHorizontalBoundaryWhenPastTopOrBottom();

    if (!moveCaretToBoundary && !closestBox && lastRootBoxWithChildren) {
        // y coordinate is below last root line box, pretend we hit it
        closestBox = lastRootBoxWithChildren->closestLeafChildForLogicalLeftPosition(pointInLogicalContents.x());
    }

    if (closestBox) {
        if (moveCaretToBoundary) {
            LayoutUnit firstRootBoxWithChildrenTop = min<LayoutUnit>(firstRootBoxWithChildren->selectionTop(), firstRootBoxWithChildren->logicalTop());
            if (pointInLogicalContents.y() < firstRootBoxWithChildrenTop
                || (blocksAreFlipped && pointInLogicalContents.y() == firstRootBoxWithChildrenTop)) {
                InlineBox* box = firstRootBoxWithChildren->firstLeafChild();
                if (box->isLineBreak()) {
                    if (InlineBox* newBox = box->nextLeafChildIgnoringLineBreak())
                        box = newBox;
                }
                // y coordinate is above first root line box, so return the start of the first
                return VisiblePosition(positionForBox(box, true), DOWNSTREAM);
            }
        }

        // pass the box a top position that is inside it
        LayoutPoint point(pointInLogicalContents.x(), closestBox->root()->blockDirectionPointInLine());
        if (!isHorizontalWritingMode())
            point = point.transposedPoint();
        if (closestBox->renderer()->isReplaced())
            return positionForPointRespectingEditingBoundaries(this, toRenderBox(closestBox->renderer()), point);
        return closestBox->renderer()->positionForPoint(point);
    }

    if (lastRootBoxWithChildren) {
        // We hit this case for Mac behavior when the Y coordinate is below the last box.
        ASSERT(moveCaretToBoundary);
        InlineBox* logicallyLastBox;
        if (lastRootBoxWithChildren->getLogicalEndBoxWithNode(logicallyLastBox))
            return VisiblePosition(positionForBox(logicallyLastBox, false), DOWNSTREAM);
    }

    // Can't reach this. We have a root line box, but it has no kids.
    // FIXME: This should ASSERT_NOT_REACHED(), but clicking on placeholder text
    // seems to hit this code path.
    return createVisiblePosition(0, DOWNSTREAM);
}

static inline bool isChildHitTestCandidate(RenderBox* box)
{
    return box->height() && box->style()->visibility() == VISIBLE && !box->isFloatingOrOutOfFlowPositioned();
}

VisiblePosition RenderBlock::positionForPoint(const LayoutPoint& point)
{
    if (isTable())
        return RenderBox::positionForPoint(point);

    if (isReplaced()) {
        // FIXME: This seems wrong when the object's writing-mode doesn't match the line's writing-mode.
        LayoutUnit pointLogicalLeft = isHorizontalWritingMode() ? point.x() : point.y();
        LayoutUnit pointLogicalTop = isHorizontalWritingMode() ? point.y() : point.x();

        if (pointLogicalTop < 0 || (pointLogicalTop < logicalHeight() && pointLogicalLeft < 0))
            return createVisiblePosition(caretMinOffset(), DOWNSTREAM);
        if (pointLogicalTop >= logicalHeight() || (pointLogicalTop >= 0 && pointLogicalLeft >= logicalWidth()))
            return createVisiblePosition(caretMaxOffset(), DOWNSTREAM);
    } 

    LayoutPoint pointInContents = point;
    offsetForContents(pointInContents);
    LayoutPoint pointInLogicalContents(pointInContents);
    if (!isHorizontalWritingMode())
        pointInLogicalContents = pointInLogicalContents.transposedPoint();

    if (childrenInline())
        return positionForPointWithInlineChildren(pointInLogicalContents);

    RenderBox* lastCandidateBox = lastChildBox();
    while (lastCandidateBox && !isChildHitTestCandidate(lastCandidateBox))
        lastCandidateBox = lastCandidateBox->previousSiblingBox();

    bool blocksAreFlipped = style()->isFlippedBlocksWritingMode();
    if (lastCandidateBox) {
        if (pointInLogicalContents.y() > logicalTopForChild(lastCandidateBox)
            || (!blocksAreFlipped && pointInLogicalContents.y() == logicalTopForChild(lastCandidateBox)))
            return positionForPointRespectingEditingBoundaries(this, lastCandidateBox, pointInContents);

        for (RenderBox* childBox = firstChildBox(); childBox; childBox = childBox->nextSiblingBox()) {
            if (!isChildHitTestCandidate(childBox))
                continue;
            LayoutUnit childLogicalBottom = logicalTopForChild(childBox) + logicalHeightForChild(childBox);
            // We hit child if our click is above the bottom of its padding box (like IE6/7 and FF3).
            if (isChildHitTestCandidate(childBox) && (pointInLogicalContents.y() < childLogicalBottom
                || (blocksAreFlipped && pointInLogicalContents.y() == childLogicalBottom)))
                return positionForPointRespectingEditingBoundaries(this, childBox, pointInContents);
        }
    }

    // We only get here if there are no hit test candidate children below the click.
    return RenderBox::positionForPoint(point);
}

void RenderBlock::offsetForContents(LayoutPoint& offset) const
{
    offset = flipForWritingMode(offset);

    if (hasOverflowClip())
        offset += scrolledContentOffset();

    if (hasColumns())
        adjustPointToColumnContents(offset);

    offset = flipForWritingMode(offset);
}

LayoutUnit RenderBlock::availableLogicalWidth() const
{
    // If we have multiple columns, then the available logical width is reduced to our column width.
    if (hasColumns())
        return desiredColumnWidth();
    return RenderBox::availableLogicalWidth();
}

int RenderBlock::columnGap() const
{
    if (style()->hasNormalColumnGap())
        return style()->fontDescription().computedPixelSize(); // "1em" is recommended as the normal gap setting. Matches <p> margins.
    return static_cast<int>(style()->columnGap());
}

void RenderBlock::calcColumnWidth()
{   
    if (document()->regionBasedColumnsEnabled())
        return;

    // Calculate our column width and column count.
    // FIXME: Can overflow on fast/block/float/float-not-removed-from-next-sibling4.html, see https://bugs.webkit.org/show_bug.cgi?id=68744
    unsigned desiredColumnCount = 1;
    LayoutUnit desiredColumnWidth = contentLogicalWidth();
    
    // For now, we don't support multi-column layouts when printing, since we have to do a lot of work for proper pagination.
    if (document()->paginated() || (style()->hasAutoColumnCount() && style()->hasAutoColumnWidth()) || !style()->hasInlineColumnAxis()) {
        setDesiredColumnCountAndWidth(desiredColumnCount, desiredColumnWidth);
        return;
    }
        
    LayoutUnit availWidth = desiredColumnWidth;
    LayoutUnit colGap = columnGap();
    LayoutUnit colWidth = max<LayoutUnit>(1, LayoutUnit(style()->columnWidth()));
    int colCount = max<int>(1, style()->columnCount());

    if (style()->hasAutoColumnWidth() && !style()->hasAutoColumnCount()) {
        desiredColumnCount = colCount;
        desiredColumnWidth = max<LayoutUnit>(0, (availWidth - ((desiredColumnCount - 1) * colGap)) / desiredColumnCount);
    } else if (!style()->hasAutoColumnWidth() && style()->hasAutoColumnCount()) {
        desiredColumnCount = max<LayoutUnit>(1, (availWidth + colGap) / (colWidth + colGap));
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    } else {
        desiredColumnCount = max<LayoutUnit>(min<LayoutUnit>(colCount, (availWidth + colGap) / (colWidth + colGap)), 1);
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    }
    setDesiredColumnCountAndWidth(desiredColumnCount, desiredColumnWidth);
}

bool RenderBlock::requiresColumns(int desiredColumnCount) const
{
    // If overflow-y is set to paged-x or paged-y on the body or html element, we'll handle the paginating
    // in the RenderView instead.
    bool isPaginated = (style()->overflowY() == OPAGEDX || style()->overflowY() == OPAGEDY) && !(isRoot() || isBody());

    return firstChild()
        && (desiredColumnCount != 1 || !style()->hasAutoColumnWidth() || !style()->hasInlineColumnAxis() || isPaginated)
        && !firstChild()->isAnonymousColumnsBlock()
        && !firstChild()->isAnonymousColumnSpanBlock();
}

void RenderBlock::setDesiredColumnCountAndWidth(int count, LayoutUnit width)
{
    bool destroyColumns = !requiresColumns(count);
    if (destroyColumns) {
        if (hasColumns()) {
            gColumnInfoMap->take(this);
            setHasColumns(false);
        }
    } else {
        ColumnInfo* info;
        if (hasColumns())
            info = gColumnInfoMap->get(this);
        else {
            if (!gColumnInfoMap)
                gColumnInfoMap = new ColumnInfoMap;
            info = new ColumnInfo;
            gColumnInfoMap->add(this, adoptPtr(info));
            setHasColumns(true);
        }
        info->setDesiredColumnCount(count);
        info->setDesiredColumnWidth(width);
        info->setProgressionAxis(style()->hasInlineColumnAxis() ? ColumnInfo::InlineAxis : ColumnInfo::BlockAxis);
        info->setProgressionIsReversed(style()->columnProgression() == ReverseColumnProgression);
    }
}

void RenderBlock::updateColumnInfoFromStyle(RenderStyle* style)
{
    if (!hasColumns())
        return;

    ColumnInfo* info = gColumnInfoMap->get(this);

    bool needsLayout = false;
    ColumnInfo::Axis oldAxis = info->progressionAxis();
    ColumnInfo::Axis newAxis = style->hasInlineColumnAxis() ? ColumnInfo::InlineAxis : ColumnInfo::BlockAxis;
    if (oldAxis != newAxis) {
        info->setProgressionAxis(newAxis);
        needsLayout = true;
    }

    bool oldProgressionIsReversed = info->progressionIsReversed();
    bool newProgressionIsReversed = style->columnProgression() == ReverseColumnProgression;
    if (oldProgressionIsReversed != newProgressionIsReversed) {
        info->setProgressionIsReversed(newProgressionIsReversed);
        needsLayout = true;
    }

    if (needsLayout)
        setNeedsLayoutAndPrefWidthsRecalc();
}

LayoutUnit RenderBlock::desiredColumnWidth() const
{
    if (!hasColumns())
        return contentLogicalWidth();
    return gColumnInfoMap->get(this)->desiredColumnWidth();
}

unsigned RenderBlock::desiredColumnCount() const
{
    if (!hasColumns())
        return 1;
    return gColumnInfoMap->get(this)->desiredColumnCount();
}

ColumnInfo* RenderBlock::columnInfo() const
{
    if (!hasColumns())
        return 0;
    return gColumnInfoMap->get(this);    
}

unsigned RenderBlock::columnCount(ColumnInfo* colInfo) const
{
    ASSERT(hasColumns());
    ASSERT(gColumnInfoMap->get(this) == colInfo);
    return colInfo->columnCount();
}

LayoutRect RenderBlock::columnRectAt(ColumnInfo* colInfo, unsigned index) const
{
    ASSERT(hasColumns() && gColumnInfoMap->get(this) == colInfo);

    // Compute the appropriate rect based off our information.
    LayoutUnit colLogicalWidth = colInfo->desiredColumnWidth();
    LayoutUnit colLogicalHeight = colInfo->columnHeight();
    LayoutUnit colLogicalTop = borderAndPaddingBefore();
    LayoutUnit colLogicalLeft = logicalLeftOffsetForContent();
    LayoutUnit colGap = columnGap();
    if (colInfo->progressionAxis() == ColumnInfo::InlineAxis) {
        if (style()->isLeftToRightDirection() ^ colInfo->progressionIsReversed())
            colLogicalLeft += index * (colLogicalWidth + colGap);
        else
            colLogicalLeft += contentLogicalWidth() - colLogicalWidth - index * (colLogicalWidth + colGap);
    } else {
        if (!colInfo->progressionIsReversed())
            colLogicalTop += index * (colLogicalHeight + colGap);
        else
            colLogicalTop += contentLogicalHeight() - colLogicalHeight - index * (colLogicalHeight + colGap);
    }

    if (isHorizontalWritingMode())
        return LayoutRect(colLogicalLeft, colLogicalTop, colLogicalWidth, colLogicalHeight);
    return LayoutRect(colLogicalTop, colLogicalLeft, colLogicalHeight, colLogicalWidth);
}

bool RenderBlock::relayoutForPagination(bool hasSpecifiedPageLogicalHeight, LayoutUnit pageLogicalHeight, LayoutStateMaintainer& statePusher)
{
    if (!hasColumns())
        return false;

    OwnPtr<RenderOverflow> savedOverflow = m_overflow.release();
    if (childrenInline())
        addOverflowFromInlineChildren();
    else
        addOverflowFromBlockChildren();
    LayoutUnit layoutOverflowLogicalBottom = (isHorizontalWritingMode() ? layoutOverflowRect().maxY() : layoutOverflowRect().maxX()) - borderAndPaddingBefore();

    // FIXME: We don't balance properly at all in the presence of forced page breaks.  We need to understand what
    // the distance between forced page breaks is so that we can avoid making the minimum column height too tall.
    ColumnInfo* colInfo = columnInfo();
    if (!hasSpecifiedPageLogicalHeight) {
        LayoutUnit columnHeight = pageLogicalHeight;
        int minColumnCount = colInfo->forcedBreaks() + 1;
        int desiredColumnCount = colInfo->desiredColumnCount();
        if (minColumnCount >= desiredColumnCount) {
            // The forced page breaks are in control of the balancing.  Just set the column height to the
            // maximum page break distance.
            if (!pageLogicalHeight) {
                LayoutUnit distanceBetweenBreaks = max<LayoutUnit>(colInfo->maximumDistanceBetweenForcedBreaks(),
                    view()->layoutState()->pageLogicalOffset(this, borderAndPaddingBefore() + layoutOverflowLogicalBottom) - colInfo->forcedBreakOffset());
                columnHeight = max(colInfo->minimumColumnHeight(), distanceBetweenBreaks);
            }
        } else if (layoutOverflowLogicalBottom > boundedMultiply(pageLogicalHeight, desiredColumnCount)) {
            // Now that we know the intrinsic height of the columns, we have to rebalance them.
            columnHeight = max<LayoutUnit>(colInfo->minimumColumnHeight(), ceilf((float)layoutOverflowLogicalBottom / desiredColumnCount));
        }
        
        if (columnHeight && columnHeight != pageLogicalHeight) {
            statePusher.pop();
            setEverHadLayout(true);
            layoutBlock(false, columnHeight);
            return true;
        }
    } 

    if (pageLogicalHeight)
        colInfo->setColumnCountAndHeight(ceilf((float)layoutOverflowLogicalBottom / pageLogicalHeight), pageLogicalHeight);

    if (columnCount(colInfo)) {
        setLogicalHeight(borderAndPaddingBefore() + colInfo->columnHeight() + borderAndPaddingAfter() + scrollbarLogicalHeight());
        m_overflow.clear();
    } else
        m_overflow = savedOverflow.release();
    
    return false;
}

void RenderBlock::adjustPointToColumnContents(LayoutPoint& point) const
{
    // Just bail if we have no columns.
    if (!hasColumns())
        return;
    
    ColumnInfo* colInfo = columnInfo();
    if (!columnCount(colInfo))
        return;

    // Determine which columns we intersect.
    LayoutUnit colGap = columnGap();
    LayoutUnit halfColGap = colGap / 2;
    LayoutPoint columnPoint(columnRectAt(colInfo, 0).location());
    LayoutUnit logicalOffset = 0;
    for (unsigned i = 0; i < colInfo->columnCount(); i++) {
        // Add in half the column gap to the left and right of the rect.
        LayoutRect colRect = columnRectAt(colInfo, i);
        flipForWritingMode(colRect);
        if (isHorizontalWritingMode() == (colInfo->progressionAxis() == ColumnInfo::InlineAxis)) {
            LayoutRect gapAndColumnRect(colRect.x() - halfColGap, colRect.y(), colRect.width() + colGap, colRect.height());
            if (point.x() >= gapAndColumnRect.x() && point.x() < gapAndColumnRect.maxX()) {
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis) {
                    // FIXME: The clamping that follows is not completely right for right-to-left
                    // content.
                    // Clamp everything above the column to its top left.
                    if (point.y() < gapAndColumnRect.y())
                        point = gapAndColumnRect.location();
                    // Clamp everything below the column to the next column's top left. If there is
                    // no next column, this still maps to just after this column.
                    else if (point.y() >= gapAndColumnRect.maxY()) {
                        point = gapAndColumnRect.location();
                        point.move(0, gapAndColumnRect.height());
                    }
                } else {
                    if (point.x() < colRect.x())
                        point.setX(colRect.x());
                    else if (point.x() >= colRect.maxX())
                        point.setX(colRect.maxX() - 1);
                }

                // We're inside the column.  Translate the x and y into our column coordinate space.
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis)
                    point.move(columnPoint.x() - colRect.x(), (!style()->isFlippedBlocksWritingMode() ? logicalOffset : -logicalOffset));
                else
                    point.move((!style()->isFlippedBlocksWritingMode() ? logicalOffset : -logicalOffset) - colRect.x() + borderLeft() + paddingLeft(), 0);
                return;
            }

            // Move to the next position.
            logicalOffset += colInfo->progressionAxis() == ColumnInfo::InlineAxis ? colRect.height() : colRect.width();
        } else {
            LayoutRect gapAndColumnRect(colRect.x(), colRect.y() - halfColGap, colRect.width(), colRect.height() + colGap);
            if (point.y() >= gapAndColumnRect.y() && point.y() < gapAndColumnRect.maxY()) {
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis) {
                    // FIXME: The clamping that follows is not completely right for right-to-left
                    // content.
                    // Clamp everything above the column to its top left.
                    if (point.x() < gapAndColumnRect.x())
                        point = gapAndColumnRect.location();
                    // Clamp everything below the column to the next column's top left. If there is
                    // no next column, this still maps to just after this column.
                    else if (point.x() >= gapAndColumnRect.maxX()) {
                        point = gapAndColumnRect.location();
                        point.move(gapAndColumnRect.width(), 0);
                    }
                } else {
                    if (point.y() < colRect.y())
                        point.setY(colRect.y());
                    else if (point.y() >= colRect.maxY())
                        point.setY(colRect.maxY() - 1);
                }

                // We're inside the column.  Translate the x and y into our column coordinate space.
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis)
                    point.move((!style()->isFlippedBlocksWritingMode() ? logicalOffset : -logicalOffset), columnPoint.y() - colRect.y());
                else
                    point.move(0, (!style()->isFlippedBlocksWritingMode() ? logicalOffset : -logicalOffset) - colRect.y() + borderTop() + paddingTop());
                return;
            }

            // Move to the next position.
            logicalOffset += colInfo->progressionAxis() == ColumnInfo::InlineAxis ? colRect.width() : colRect.height();
        }
    }
}

void RenderBlock::adjustRectForColumns(LayoutRect& r) const
{
    // Just bail if we have no columns.
    if (!hasColumns())
        return;
    
    ColumnInfo* colInfo = columnInfo();
    
    // Determine which columns we intersect.
    unsigned colCount = columnCount(colInfo);
    if (!colCount)
        return;

    // Begin with a result rect that is empty.
    LayoutRect result;

    bool isHorizontal = isHorizontalWritingMode();
    LayoutUnit beforeBorderPadding = borderAndPaddingBefore();
    LayoutUnit colHeight = colInfo->columnHeight();
    if (!colHeight)
        return;

    LayoutUnit startOffset = max(isHorizontal ? r.y() : r.x(), beforeBorderPadding);
    LayoutUnit endOffset = max(min<LayoutUnit>(isHorizontal ? r.maxY() : r.maxX(), beforeBorderPadding + colCount * colHeight), beforeBorderPadding);

    // FIXME: Can overflow on fast/block/float/float-not-removed-from-next-sibling4.html, see https://bugs.webkit.org/show_bug.cgi?id=68744
    unsigned startColumn = (startOffset - beforeBorderPadding) / colHeight;
    unsigned endColumn = (endOffset - beforeBorderPadding) / colHeight;

    if (startColumn == endColumn) {
        // The rect is fully contained within one column. Adjust for our offsets
        // and repaint only that portion.
        LayoutUnit logicalLeftOffset = logicalLeftOffsetForContent();
        LayoutRect colRect = columnRectAt(colInfo, startColumn);
        LayoutRect repaintRect = r;

        if (colInfo->progressionAxis() == ColumnInfo::InlineAxis) {
            if (isHorizontal)
                repaintRect.move(colRect.x() - logicalLeftOffset, - static_cast<int>(startColumn) * colHeight);
            else
                repaintRect.move(- static_cast<int>(startColumn) * colHeight, colRect.y() - logicalLeftOffset);
        } else {
            if (isHorizontal)
                repaintRect.move(0, colRect.y() - startColumn * colHeight - beforeBorderPadding);
            else
                repaintRect.move(colRect.x() - startColumn * colHeight - beforeBorderPadding, 0);
        }
        repaintRect.intersect(colRect);
        result.unite(repaintRect);
    } else {
        // We span multiple columns. We can just unite the start and end column to get the final
        // repaint rect.
        result.unite(columnRectAt(colInfo, startColumn));
        result.unite(columnRectAt(colInfo, endColumn));
    }

    r = result;
}

LayoutPoint RenderBlock::flipForWritingModeIncludingColumns(const LayoutPoint& point) const
{
    ASSERT(hasColumns());
    if (!hasColumns() || !style()->isFlippedBlocksWritingMode())
        return point;
    ColumnInfo* colInfo = columnInfo();
    LayoutUnit columnLogicalHeight = colInfo->columnHeight();
    LayoutUnit expandedLogicalHeight = borderAndPaddingBefore() + columnCount(colInfo) * columnLogicalHeight + borderAndPaddingAfter() + scrollbarLogicalHeight();
    if (isHorizontalWritingMode())
        return LayoutPoint(point.x(), expandedLogicalHeight - point.y());
    return LayoutPoint(expandedLogicalHeight - point.x(), point.y());
}

void RenderBlock::adjustStartEdgeForWritingModeIncludingColumns(LayoutRect& rect) const
{
    ASSERT(hasColumns());
    if (!hasColumns() || !style()->isFlippedBlocksWritingMode())
        return;
    
    ColumnInfo* colInfo = columnInfo();
    LayoutUnit columnLogicalHeight = colInfo->columnHeight();
    LayoutUnit expandedLogicalHeight = borderAndPaddingBefore() + columnCount(colInfo) * columnLogicalHeight + borderAndPaddingAfter() + scrollbarLogicalHeight();
    
    if (isHorizontalWritingMode())
        rect.setY(expandedLogicalHeight - rect.maxY());
    else
        rect.setX(expandedLogicalHeight - rect.maxX());
}

void RenderBlock::adjustForColumns(LayoutSize& offset, const LayoutPoint& point) const
{
    if (!hasColumns())
        return;

    ColumnInfo* colInfo = columnInfo();

    LayoutUnit logicalLeft = logicalLeftOffsetForContent();
    unsigned colCount = columnCount(colInfo);
    LayoutUnit colLogicalWidth = colInfo->desiredColumnWidth();
    LayoutUnit colLogicalHeight = colInfo->columnHeight();

    for (unsigned i = 0; i < colCount; ++i) {
        // Compute the edges for a given column in the block progression direction.
        LayoutRect sliceRect = LayoutRect(logicalLeft, borderAndPaddingBefore() + i * colLogicalHeight, colLogicalWidth, colLogicalHeight);
        if (!isHorizontalWritingMode())
            sliceRect = sliceRect.transposedRect();
        
        LayoutUnit logicalOffset = i * colLogicalHeight;

        // Now we're in the same coordinate space as the point.  See if it is inside the rectangle.
        if (isHorizontalWritingMode()) {
            if (point.y() >= sliceRect.y() && point.y() < sliceRect.maxY()) {
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis)
                    offset.expand(columnRectAt(colInfo, i).x() - logicalLeft, -logicalOffset);
                else
                    offset.expand(0, columnRectAt(colInfo, i).y() - logicalOffset - borderAndPaddingBefore());
                return;
            }
        } else {
            if (point.x() >= sliceRect.x() && point.x() < sliceRect.maxX()) {
                if (colInfo->progressionAxis() == ColumnInfo::InlineAxis)
                    offset.expand(-logicalOffset, columnRectAt(colInfo, i).y() - logicalLeft);
                else
                    offset.expand(columnRectAt(colInfo, i).x() - logicalOffset - borderAndPaddingBefore(), 0);
                return;
            }
        }
    }
}

void RenderBlock::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    if (childrenInline()) {
        // FIXME: Remove this const_cast.
        const_cast<RenderBlock*>(this)->computeInlinePreferredLogicalWidths(minLogicalWidth, maxLogicalWidth);
    } else
        computeBlockPreferredLogicalWidths(minLogicalWidth, maxLogicalWidth);

    maxLogicalWidth = max(minLogicalWidth, maxLogicalWidth);

    if (!style()->autoWrap() && childrenInline()) {
        // A horizontal marquee with inline children has no minimum width.
        if (layer() && layer()->marquee() && layer()->marquee()->isHorizontal())
            minLogicalWidth = 0;
    }

    if (isTableCell()) {
        Length tableCellWidth = toRenderTableCell(this)->styleOrColLogicalWidth();
        if (tableCellWidth.isFixed() && tableCellWidth.value() > 0)
            maxLogicalWidth = max(minLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(tableCellWidth.value()));
    }

    int scrollbarWidth = instrinsicScrollbarLogicalWidth();
    maxLogicalWidth += scrollbarWidth;
    minLogicalWidth += scrollbarWidth;
}

void RenderBlock::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    updateFirstLetter();

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    RenderStyle* styleToUse = style();
    if (!isTableCell() && styleToUse->logicalWidth().isFixed() && styleToUse->logicalWidth().value() >= 0 
        && !(isDeprecatedFlexItem() && !styleToUse->logicalWidth().intValue()))
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalWidth().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);
    
    if (styleToUse->logicalMinWidth().isFixed() && styleToUse->logicalMinWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
    }
    
    if (styleToUse->logicalMaxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
    }
    
    // Table layout uses integers, ceil the preferred widths to ensure that they can contain the contents.
    if (isTableCell()) {
        m_minPreferredLogicalWidth = m_minPreferredLogicalWidth.ceil();
        m_maxPreferredLogicalWidth = m_maxPreferredLogicalWidth.ceil();
    }

    LayoutUnit borderAndPadding = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPadding;
    m_maxPreferredLogicalWidth += borderAndPadding;

    setPreferredLogicalWidthsDirty(false);
}

struct InlineMinMaxIterator {
/* InlineMinMaxIterator is a class that will iterate over all render objects that contribute to
   inline min/max width calculations.  Note the following about the way it walks:
   (1) Positioned content is skipped (since it does not contribute to min/max width of a block)
   (2) We do not drill into the children of floats or replaced elements, since you can't break
       in the middle of such an element.
   (3) Inline flows (e.g., <a>, <span>, <i>) are walked twice, since each side can have
       distinct borders/margin/padding that contribute to the min/max width.
*/
    RenderObject* parent;
    RenderObject* current;
    bool endOfInline;

    InlineMinMaxIterator(RenderObject* p, bool end = false)
        :parent(p), current(p), endOfInline(end) {}

    RenderObject* next();
};

RenderObject* InlineMinMaxIterator::next()
{
    RenderObject* result = 0;
    bool oldEndOfInline = endOfInline;
    endOfInline = false;
    while (current || current == parent) {
        if (!oldEndOfInline &&
            (current == parent ||
             (!current->isFloating() && !current->isReplaced() && !current->isOutOfFlowPositioned())))
            result = current->firstChild();
        if (!result) {
            // We hit the end of our inline. (It was empty, e.g., <span></span>.)
            if (!oldEndOfInline && current->isRenderInline()) {
                result = current;
                endOfInline = true;
                break;
            }

            while (current && current != parent) {
                result = current->nextSibling();
                if (result) break;
                current = current->parent();
                if (current && current != parent && current->isRenderInline()) {
                    result = current;
                    endOfInline = true;
                    break;
                }
            }
        }

        if (!result)
            break;

        if (!result->isOutOfFlowPositioned() && (result->isText() || result->isFloating() || result->isReplaced() || result->isRenderInline()))
             break;
        
        current = result;
        result = 0;
    }

    // Update our position.
    current = result;
    return current;
}

static LayoutUnit getBPMWidth(LayoutUnit childValue, Length cssUnit)
{
    if (cssUnit.type() != Auto)
        return (cssUnit.isFixed() ? static_cast<LayoutUnit>(cssUnit.value()) : childValue);
    return 0;
}

static LayoutUnit getBorderPaddingMargin(const RenderBoxModelObject* child, bool endOfInline)
{
    RenderStyle* childStyle = child->style();
    if (endOfInline)
        return getBPMWidth(child->marginEnd(), childStyle->marginEnd()) +
               getBPMWidth(child->paddingEnd(), childStyle->paddingEnd()) +
               child->borderEnd();
    return getBPMWidth(child->marginStart(), childStyle->marginStart()) +
               getBPMWidth(child->paddingStart(), childStyle->paddingStart()) +
               child->borderStart();
}

static inline void stripTrailingSpace(float& inlineMax, float& inlineMin,
                                      RenderObject* trailingSpaceChild)
{
    if (trailingSpaceChild && trailingSpaceChild->isText()) {
        // Collapse away the trailing space at the end of a block.
        RenderText* t = toRenderText(trailingSpaceChild);
        const UChar space = ' ';
        const Font& font = t->style()->font(); // FIXME: This ignores first-line.
        float spaceWidth = font.width(RenderBlock::constructTextRun(t, font, &space, 1, t->style()));
        inlineMax -= spaceWidth + font.wordSpacing();
        if (inlineMin > inlineMax)
            inlineMin = inlineMax;
    }
}

static inline void updatePreferredWidth(LayoutUnit& preferredWidth, float& result)
{
    LayoutUnit snappedResult = ceiledLayoutUnit(result);
    preferredWidth = max(snappedResult, preferredWidth);
}

// With sub-pixel enabled: When converting between floating point and LayoutUnits
// we risk losing precision with each conversion. When this occurs while
// accumulating our preferred widths, we can wind up with a line width that's
// larger than our maxPreferredWidth due to pure float accumulation.
//
// With sub-pixel disabled: values from Lengths or the render tree aren't subject
// to the same loss of precision, as they're always truncated and stored as
// integers. We mirror that behavior here to prevent over-allocating our preferred
// width.
static inline LayoutUnit adjustFloatForSubPixelLayout(float value)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return ceiledLayoutUnit(value);
#else
    return static_cast<int>(value);
#endif
}


void RenderBlock::computeInlinePreferredLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth)
{
    float inlineMax = 0;
    float inlineMin = 0;

    RenderStyle* styleToUse = style();
    RenderBlock* containingBlock = this->containingBlock();
    LayoutUnit cw = containingBlock ? containingBlock->contentLogicalWidth() : LayoutUnit();

    // If we are at the start of a line, we want to ignore all white-space.
    // Also strip spaces if we previously had text that ended in a trailing space.
    bool stripFrontSpaces = true;
    RenderObject* trailingSpaceChild = 0;

    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific cirucumstances (in order to match common WinIE renderings). 
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.) 
    bool allowImagesToBreak = !document()->inQuirksMode() || !isTableCell() || !styleToUse->logicalWidth().isIntrinsicOrAuto();

    bool autoWrap, oldAutoWrap;
    autoWrap = oldAutoWrap = styleToUse->autoWrap();

    InlineMinMaxIterator childIterator(this);

    // Only gets added to the max preffered width once.
    bool addedTextIndent = false;
    // Signals the text indent was more negative than the min preferred width
    bool hasRemainingNegativeTextIndent = false;

    LayoutUnit textIndent = minimumValueForLength(styleToUse->textIndent(), cw, view());
    RenderObject* prevFloat = 0;
    bool isPrevChildInlineFlow = false;
    bool shouldBreakLineAfterText = false;
    while (RenderObject* child = childIterator.next()) {
        autoWrap = child->isReplaced() ? child->parent()->style()->autoWrap() : 
            child->style()->autoWrap();

        if (!child->isBR()) {
            // Step One: determine whether or not we need to go ahead and
            // terminate our current line.  Each discrete chunk can become
            // the new min-width, if it is the widest chunk seen so far, and
            // it can also become the max-width.

            // Children fall into three categories:
            // (1) An inline flow object.  These objects always have a min/max of 0,
            // and are included in the iteration solely so that their margins can
            // be added in.
            //
            // (2) An inline non-text non-flow object, e.g., an inline replaced element.
            // These objects can always be on a line by themselves, so in this situation
            // we need to go ahead and break the current line, and then add in our own
            // margins and min/max width on its own line, and then terminate the line.
            //
            // (3) A text object.  Text runs can have breakable characters at the start,
            // the middle or the end.  They may also lose whitespace off the front if
            // we're already ignoring whitespace.  In order to compute accurate min-width
            // information, we need three pieces of information.
            // (a) the min-width of the first non-breakable run.  Should be 0 if the text string
            // starts with whitespace.
            // (b) the min-width of the last non-breakable run. Should be 0 if the text string
            // ends with whitespace.
            // (c) the min/max width of the string (trimmed for whitespace).
            //
            // If the text string starts with whitespace, then we need to go ahead and
            // terminate our current line (unless we're already in a whitespace stripping
            // mode.
            //
            // If the text string has a breakable character in the middle, but didn't start
            // with whitespace, then we add the width of the first non-breakable run and
            // then end the current line.  We then need to use the intermediate min/max width
            // values (if any of them are larger than our current min/max).  We then look at
            // the width of the last non-breakable run and use that to start a new line
            // (unless we end in whitespace).
            RenderStyle* childStyle = child->style();
            float childMin = 0;
            float childMax = 0;

            if (!child->isText()) {
                // Case (1) and (2).  Inline replaced and inline flow elements.
                if (child->isRenderInline()) {
                    // Add in padding/border/margin from the appropriate side of
                    // the element.
                    float bpm = getBorderPaddingMargin(toRenderInline(child), childIterator.endOfInline);
                    childMin += bpm;
                    childMax += bpm;

                    inlineMin += childMin;
                    inlineMax += childMax;
                    
                    child->setPreferredLogicalWidthsDirty(false);
                } else {
                    // Inline replaced elts add in their margins to their min/max values.
                    LayoutUnit margins = 0;
                    Length startMargin = childStyle->marginStart();
                    Length endMargin = childStyle->marginEnd();
                    if (startMargin.isFixed())
                        margins += adjustFloatForSubPixelLayout(startMargin.value());
                    if (endMargin.isFixed())
                        margins += adjustFloatForSubPixelLayout(endMargin.value());
                    childMin += margins.ceilToFloat();
                    childMax += margins.ceilToFloat();
                }
            }

            if (!child->isRenderInline() && !child->isText()) {
                // Case (2). Inline replaced elements and floats.
                // Go ahead and terminate the current line as far as
                // minwidth is concerned.
                childMin += child->minPreferredLogicalWidth().ceilToFloat();
                childMax += child->maxPreferredLogicalWidth().ceilToFloat();

                bool clearPreviousFloat;
                if (child->isFloating()) {
                    clearPreviousFloat = (prevFloat
                        && ((prevFloat->style()->floating() == LeftFloat && (childStyle->clear() & CLEFT))
                            || (prevFloat->style()->floating() == RightFloat && (childStyle->clear() & CRIGHT))));
                    prevFloat = child;
                } else
                    clearPreviousFloat = false;

                bool canBreakReplacedElement = !child->isImage() || allowImagesToBreak;
                if ((canBreakReplacedElement && (autoWrap || oldAutoWrap) && (!isPrevChildInlineFlow || shouldBreakLineAfterText)) || clearPreviousFloat) {
                    updatePreferredWidth(minLogicalWidth, inlineMin);
                    inlineMin = 0;
                }

                // If we're supposed to clear the previous float, then terminate maxwidth as well.
                if (clearPreviousFloat) {
                    updatePreferredWidth(maxLogicalWidth, inlineMax);
                    inlineMax = 0;
                }

                // Add in text-indent.  This is added in only once.
                if (!addedTextIndent && !child->isFloating()) {
                    LayoutUnit ceiledIndent = textIndent.ceilToFloat();
                    childMin += ceiledIndent;
                    childMax += ceiledIndent;

                    if (childMin < 0)
                        textIndent = adjustFloatForSubPixelLayout(childMin);
                    else
                        addedTextIndent = true;
                }

                // Add our width to the max.
                inlineMax += max<float>(0, childMax);

                if (!autoWrap || !canBreakReplacedElement || (isPrevChildInlineFlow && !shouldBreakLineAfterText)) {
                    if (child->isFloating())
                        updatePreferredWidth(minLogicalWidth, childMin);
                    else
                        inlineMin += childMin;
                } else {
                    // Now check our line.
                    updatePreferredWidth(minLogicalWidth, childMin);

                    // Now start a new line.
                    inlineMin = 0;
                }

                if (autoWrap && canBreakReplacedElement && isPrevChildInlineFlow) {
                    updatePreferredWidth(minLogicalWidth, inlineMin);
                    inlineMin = 0;
                }

                // We are no longer stripping whitespace at the start of
                // a line.
                if (!child->isFloating()) {
                    stripFrontSpaces = false;
                    trailingSpaceChild = 0;
                }
            } else if (child->isText()) {
                // Case (3). Text.
                RenderText* t = toRenderText(child);

                if (t->isWordBreak()) {
                    updatePreferredWidth(minLogicalWidth, inlineMin);
                    inlineMin = 0;
                    continue;
                }

                if (t->style()->hasTextCombine() && t->isCombineText())
                    toRenderCombineText(t)->combineText();

                // Determine if we have a breakable character.  Pass in
                // whether or not we should ignore any spaces at the front
                // of the string.  If those are going to be stripped out,
                // then they shouldn't be considered in the breakable char
                // check.
                bool hasBreakableChar, hasBreak;
                float beginMin, endMin;
                bool beginWS, endWS;
                float beginMax, endMax;
                t->trimmedPrefWidths(inlineMax, beginMin, beginWS, endMin, endWS,
                                     hasBreakableChar, hasBreak, beginMax, endMax,
                                     childMin, childMax, stripFrontSpaces);

                // This text object will not be rendered, but it may still provide a breaking opportunity.
                if (!hasBreak && childMax == 0) {
                    if (autoWrap && (beginWS || endWS)) {
                        updatePreferredWidth(minLogicalWidth, inlineMin);
                        inlineMin = 0;
                    }
                    continue;
                }
                
                if (stripFrontSpaces)
                    trailingSpaceChild = child;
                else
                    trailingSpaceChild = 0;

                // Add in text-indent.  This is added in only once.
                float ti = 0;
                if (!addedTextIndent || hasRemainingNegativeTextIndent) {
                    ti = textIndent.ceilToFloat();
                    childMin += ti;
                    beginMin += ti;
                    
                    // It the text indent negative and larger than the child minimum, we re-use the remainder
                    // in future minimum calculations, but using the negative value again on the maximum
                    // will lead to under-counting the max pref width.
                    if (!addedTextIndent) {
                        childMax += ti;
                        beginMax += ti;
                        addedTextIndent = true;
                    }
                    
                    if (childMin < 0) {
                        textIndent = childMin;
                        hasRemainingNegativeTextIndent = true;
                    }
                }
                
                // If we have no breakable characters at all,
                // then this is the easy case. We add ourselves to the current
                // min and max and continue.
                if (!hasBreakableChar) {
                    inlineMin += childMin;
                } else {
                    // We have a breakable character.  Now we need to know if
                    // we start and end with whitespace.
                    if (beginWS)
                        // Go ahead and end the current line.
                        updatePreferredWidth(minLogicalWidth, inlineMin);
                    else {
                        inlineMin += beginMin;
                        updatePreferredWidth(minLogicalWidth, inlineMin);
                        childMin -= ti;
                    }

                    inlineMin = childMin;

                    if (endWS) {
                        // We end in whitespace, which means we can go ahead
                        // and end our current line.
                        updatePreferredWidth(minLogicalWidth, inlineMin);
                        inlineMin = 0;
                        shouldBreakLineAfterText = false;
                    } else {
                        updatePreferredWidth(minLogicalWidth, inlineMin);
                        inlineMin = endMin;
                        shouldBreakLineAfterText = true;
                    }
                }

                if (hasBreak) {
                    inlineMax += beginMax;
                    updatePreferredWidth(maxLogicalWidth, inlineMax);
                    updatePreferredWidth(maxLogicalWidth, childMax);
                    inlineMax = endMax;
                    addedTextIndent = true;
                } else
                    inlineMax += max<float>(0, childMax);
            }

            // Ignore spaces after a list marker.
            if (child->isListMarker())
                stripFrontSpaces = true;
        } else {
            updatePreferredWidth(minLogicalWidth, inlineMin);
            updatePreferredWidth(maxLogicalWidth, inlineMax);
            inlineMin = inlineMax = 0;
            stripFrontSpaces = true;
            trailingSpaceChild = 0;
            addedTextIndent = true;
        }

        if (!child->isText() && child->isRenderInline())
            isPrevChildInlineFlow = true;
        else
            isPrevChildInlineFlow = false;

        oldAutoWrap = autoWrap;
    }

    if (styleToUse->collapseWhiteSpace())
        stripTrailingSpace(inlineMax, inlineMin, trailingSpaceChild);

    updatePreferredWidth(minLogicalWidth, inlineMin);
    updatePreferredWidth(maxLogicalWidth, inlineMax);
}

void RenderBlock::computeBlockPreferredLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    RenderStyle* styleToUse = style();
    bool nowrap = styleToUse->whiteSpace() == NOWRAP;

    RenderObject* child = firstChild();
    RenderBlock* containingBlock = this->containingBlock();
    LayoutUnit floatLeftWidth = 0, floatRightWidth = 0;
    while (child) {
        // Positioned children don't affect the min/max width
        if (child->isOutOfFlowPositioned()) {
            child = child->nextSibling();
            continue;
        }

        RenderStyle* childStyle = child->style();
        if (child->isFloating() || (child->isBox() && toRenderBox(child)->avoidsFloats())) {
            LayoutUnit floatTotalWidth = floatLeftWidth + floatRightWidth;
            if (childStyle->clear() & CLEFT) {
                maxLogicalWidth = max(floatTotalWidth, maxLogicalWidth);
                floatLeftWidth = 0;
            }
            if (childStyle->clear() & CRIGHT) {
                maxLogicalWidth = max(floatTotalWidth, maxLogicalWidth);
                floatRightWidth = 0;
            }
        }

        // A margin basically has three types: fixed, percentage, and auto (variable).
        // Auto and percentage margins simply become 0 when computing min/max width.
        // Fixed margins can be added in as is.
        Length startMarginLength = childStyle->marginStartUsing(styleToUse);
        Length endMarginLength = childStyle->marginEndUsing(styleToUse);
        LayoutUnit margin = 0;
        LayoutUnit marginStart = 0;
        LayoutUnit marginEnd = 0;
        if (startMarginLength.isFixed())
            marginStart += startMarginLength.value();
        if (endMarginLength.isFixed())
            marginEnd += endMarginLength.value();
        margin = marginStart + marginEnd;

        LayoutUnit childMinPreferredLogicalWidth, childMaxPreferredLogicalWidth;
        if (child->isBox() && child->isHorizontalWritingMode() != isHorizontalWritingMode()) {
            RenderBox* childBox = toRenderBox(child);
            LogicalExtentComputedValues computedValues;
            childBox->computeLogicalHeight(childBox->borderAndPaddingLogicalHeight(), 0, computedValues);
            childMinPreferredLogicalWidth = childMaxPreferredLogicalWidth = computedValues.m_extent;
        } else {
            childMinPreferredLogicalWidth = child->minPreferredLogicalWidth();
            childMaxPreferredLogicalWidth = child->maxPreferredLogicalWidth();
        }

        LayoutUnit w = childMinPreferredLogicalWidth + margin;
        minLogicalWidth = max(w, minLogicalWidth);
        
        // IE ignores tables for calculation of nowrap. Makes some sense.
        if (nowrap && !child->isTable())
            maxLogicalWidth = max(w, maxLogicalWidth);

        w = childMaxPreferredLogicalWidth + margin;

        if (!child->isFloating()) {
            if (child->isBox() && toRenderBox(child)->avoidsFloats()) {
                // Determine a left and right max value based off whether or not the floats can fit in the
                // margins of the object.  For negative margins, we will attempt to overlap the float if the negative margin
                // is smaller than the float width.
                bool ltr = containingBlock ? containingBlock->style()->isLeftToRightDirection() : styleToUse->isLeftToRightDirection();
                LayoutUnit marginLogicalLeft = ltr ? marginStart : marginEnd;
                LayoutUnit marginLogicalRight = ltr ? marginEnd : marginStart;
                LayoutUnit maxLeft = marginLogicalLeft > 0 ? max(floatLeftWidth, marginLogicalLeft) : floatLeftWidth + marginLogicalLeft;
                LayoutUnit maxRight = marginLogicalRight > 0 ? max(floatRightWidth, marginLogicalRight) : floatRightWidth + marginLogicalRight;
                w = childMaxPreferredLogicalWidth + maxLeft + maxRight;
                w = max(w, floatLeftWidth + floatRightWidth);
            }
            else
                maxLogicalWidth = max(floatLeftWidth + floatRightWidth, maxLogicalWidth);
            floatLeftWidth = floatRightWidth = 0;
        }
        
        if (child->isFloating()) {
            if (childStyle->floating() == LeftFloat)
                floatLeftWidth += w;
            else
                floatRightWidth += w;
        } else
            maxLogicalWidth = max(w, maxLogicalWidth);
        
        child = child->nextSibling();
    }

    // Always make sure these values are non-negative.
    minLogicalWidth = max<LayoutUnit>(0, minLogicalWidth);
    maxLogicalWidth = max<LayoutUnit>(0, maxLogicalWidth);

    maxLogicalWidth = max(floatLeftWidth + floatRightWidth, maxLogicalWidth);
}

bool RenderBlock::hasLineIfEmpty() const
{
    if (!node())
        return false;
    
    if (node()->isRootEditableElement())
        return true;
    
    if (node()->isShadowRoot() && isHTMLInputElement(toShadowRoot(node())->host()))
        return true;
    
    return false;
}

LayoutUnit RenderBlock::lineHeight(bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    // Inline blocks are replaced elements. Otherwise, just pass off to
    // the base class.  If we're being queried as though we're the root line
    // box, then the fact that we're an inline-block is irrelevant, and we behave
    // just like a block.
    if (isReplaced() && linePositionMode == PositionOnContainingLine)
        return RenderBox::lineHeight(firstLine, direction, linePositionMode);

    if (firstLine && document()->styleSheetCollection()->usesFirstLineRules()) {
        RenderStyle* s = style(firstLine);
        if (s != style())
            return s->computedLineHeight(view());
    }
    
    if (m_lineHeight == -1)
        m_lineHeight = style()->computedLineHeight(view());

    return m_lineHeight;
}

int RenderBlock::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    // Inline blocks are replaced elements. Otherwise, just pass off to
    // the base class.  If we're being queried as though we're the root line
    // box, then the fact that we're an inline-block is irrelevant, and we behave
    // just like a block.
    if (isReplaced() && linePositionMode == PositionOnContainingLine) {
        // For "leaf" theme objects, let the theme decide what the baseline position is.
        // FIXME: Might be better to have a custom CSS property instead, so that if the theme
        // is turned off, checkboxes/radios will still have decent baselines.
        // FIXME: Need to patch form controls to deal with vertical lines.
        if (style()->hasAppearance() && !theme()->isControlContainer(style()->appearance()))
            return theme()->baselinePosition(this);
            
        // CSS2.1 states that the baseline of an inline block is the baseline of the last line box in
        // the normal flow.  We make an exception for marquees, since their baselines are meaningless
        // (the content inside them moves).  This matches WinIE as well, which just bottom-aligns them.
        // We also give up on finding a baseline if we have a vertical scrollbar, or if we are scrolled
        // vertically (e.g., an overflow:hidden block that has had scrollTop moved) or if the baseline is outside
        // of our content box.
        bool ignoreBaseline = (layer() && (layer()->marquee() || (direction == HorizontalLine ? (layer()->verticalScrollbar() || layer()->scrollYOffset() != 0)
            : (layer()->horizontalScrollbar() || layer()->scrollXOffset() != 0)))) || (isWritingModeRoot() && !isRubyRun());
        
        int baselinePos = ignoreBaseline ? -1 : inlineBlockBaseline(direction);
        
        LayoutUnit bottomOfContent = direction == HorizontalLine ? borderTop() + paddingTop() + contentHeight() : borderRight() + paddingRight() + contentWidth();
        if (baselinePos != -1 && baselinePos <= bottomOfContent)
            return direction == HorizontalLine ? marginTop() + baselinePos : marginRight() + baselinePos;
            
        return RenderBox::baselinePosition(baselineType, firstLine, direction, linePositionMode);
    }

    const FontMetrics& fontMetrics = style(firstLine)->fontMetrics();
    return fontMetrics.ascent(baselineType) + (lineHeight(firstLine, direction, linePositionMode) - fontMetrics.height()) / 2;
}

int RenderBlock::firstLineBoxBaseline() const
{
    if (!isBlockFlow() || (isWritingModeRoot() && !isRubyRun()))
        return -1;

    if (childrenInline()) {
        if (firstLineBox())
            return firstLineBox()->logicalTop() + style(true)->fontMetrics().ascent(firstRootBox()->baselineType());
        else
            return -1;
    }
    else {
        for (RenderBox* curr = firstChildBox(); curr; curr = curr->nextSiblingBox()) {
            if (!curr->isFloatingOrOutOfFlowPositioned()) {
                int result = curr->firstLineBoxBaseline();
                if (result != -1)
                    return curr->logicalTop() + result; // Translate to our coordinate space.
            }
        }
    }

    return -1;
}

int RenderBlock::inlineBlockBaseline(LineDirectionMode direction) const
{
    return lastLineBoxBaseline(direction);
}

int RenderBlock::lastLineBoxBaseline(LineDirectionMode lineDirection) const
{
    if (!isBlockFlow() || (isWritingModeRoot() && !isRubyRun()))
        return -1;

    if (childrenInline()) {
        if (!firstLineBox() && hasLineIfEmpty()) {
            const FontMetrics& fontMetrics = firstLineStyle()->fontMetrics();
            return fontMetrics.ascent()
                 + (lineHeight(true, lineDirection, PositionOfInteriorLineBoxes) - fontMetrics.height()) / 2
                 + (lineDirection == HorizontalLine ? borderTop() + paddingTop() : borderRight() + paddingRight());
        }
        if (lastLineBox())
            return lastLineBox()->logicalTop() + style(lastLineBox() == firstLineBox())->fontMetrics().ascent(lastRootBox()->baselineType());
        return -1;
    } else {
        bool haveNormalFlowChild = false;
        for (RenderBox* curr = lastChildBox(); curr; curr = curr->previousSiblingBox()) {
            if (!curr->isFloatingOrOutOfFlowPositioned()) {
                haveNormalFlowChild = true;
                int result = curr->inlineBlockBaseline(lineDirection);
                if (result != -1)
                    return curr->logicalTop() + result; // Translate to our coordinate space.
            }
        }
        if (!haveNormalFlowChild && hasLineIfEmpty()) {
            const FontMetrics& fontMetrics = firstLineStyle()->fontMetrics();
            return fontMetrics.ascent()
                 + (lineHeight(true, lineDirection, PositionOfInteriorLineBoxes) - fontMetrics.height()) / 2
                 + (lineDirection == HorizontalLine ? borderTop() + paddingTop() : borderRight() + paddingRight());
        }
    }

    return -1;
}

bool RenderBlock::containsNonZeroBidiLevel() const
{
    for (RootInlineBox* root = firstRootBox(); root; root = root->nextRootBox()) {
        for (InlineBox* box = root->firstLeafChild(); box; box = box->nextLeafChild()) {
            if (box->bidiLevel())
                return true;
        }
    }
    return false;
}

RenderBlock* RenderBlock::firstLineBlock() const
{
    RenderBlock* firstLineBlock = const_cast<RenderBlock*>(this);
    bool hasPseudo = false;
    while (true) {
        hasPseudo = firstLineBlock->style()->hasPseudoStyle(FIRST_LINE);
        if (hasPseudo)
            break;
        RenderObject* parentBlock = firstLineBlock->parent();
        // We include isRenderButton in this check because buttons are
        // implemented using flex box but should still support first-line. The
        // flex box spec requires that flex box does not support first-line,
        // though.
        // FIXME: Remove when buttons are implemented with align-items instead
        // of flexbox.
        if (firstLineBlock->isReplaced() || firstLineBlock->isFloating()
            || !parentBlock || parentBlock->firstChild() != firstLineBlock || !parentBlock->isBlockFlow()
            || (parentBlock->isFlexibleBox() && !parentBlock->isRenderButton()))
            break;
        ASSERT_WITH_SECURITY_IMPLICATION(parentBlock->isRenderBlock());
        firstLineBlock = toRenderBlock(parentBlock);
    } 
    
    if (!hasPseudo)
        return 0;
    
    return firstLineBlock;
}

static RenderStyle* styleForFirstLetter(RenderObject* firstLetterBlock, RenderObject* firstLetterContainer)
{
    RenderStyle* pseudoStyle = firstLetterBlock->getCachedPseudoStyle(FIRST_LETTER, firstLetterContainer->firstLineStyle());
    // Force inline display (except for floating first-letters).
    pseudoStyle->setDisplay(pseudoStyle->isFloating() ? BLOCK : INLINE);
    // CSS2 says first-letter can't be positioned.
    pseudoStyle->setPosition(StaticPosition);
    return pseudoStyle;
}

// CSS 2.1 http://www.w3.org/TR/CSS21/selector.html#first-letter
// "Punctuation (i.e, characters defined in Unicode [UNICODE] in the "open" (Ps), "close" (Pe),
// "initial" (Pi). "final" (Pf) and "other" (Po) punctuation classes), that precedes or follows the first letter should be included"
static inline bool isPunctuationForFirstLetter(UChar c)
{
    CharCategory charCategory = category(c);
    return charCategory == Punctuation_Open
        || charCategory == Punctuation_Close
        || charCategory == Punctuation_InitialQuote
        || charCategory == Punctuation_FinalQuote
        || charCategory == Punctuation_Other;
}

static inline bool shouldSkipForFirstLetter(UChar c)
{
    return isSpaceOrNewline(c) || c == noBreakSpace || isPunctuationForFirstLetter(c);
}

static inline RenderObject* findFirstLetterBlock(RenderBlock* start)
{
    RenderObject* firstLetterBlock = start;
    while (true) {
        // We include isRenderButton in these two checks because buttons are
        // implemented using flex box but should still support first-letter.
        // The flex box spec requires that flex box does not support
        // first-letter, though.
        // FIXME: Remove when buttons are implemented with align-items instead
        // of flexbox.
        bool canHaveFirstLetterRenderer = firstLetterBlock->style()->hasPseudoStyle(FIRST_LETTER)
            && firstLetterBlock->canHaveGeneratedChildren()
            && (!firstLetterBlock->isFlexibleBox() || firstLetterBlock->isRenderButton());
        if (canHaveFirstLetterRenderer)
            return firstLetterBlock;

        RenderObject* parentBlock = firstLetterBlock->parent();
        if (firstLetterBlock->isReplaced() || !parentBlock || parentBlock->firstChild() != firstLetterBlock || 
            !parentBlock->isBlockFlow() || (parentBlock->isFlexibleBox() && !parentBlock->isRenderButton()))
            return 0;
        firstLetterBlock = parentBlock;
    } 

    return 0;
}

void RenderBlock::updateFirstLetterStyle(RenderObject* firstLetterBlock, RenderObject* currentChild)
{
    RenderObject* firstLetter = currentChild->parent();
    RenderObject* firstLetterContainer = firstLetter->parent();
    RenderStyle* pseudoStyle = styleForFirstLetter(firstLetterBlock, firstLetterContainer);
    ASSERT(firstLetter->isFloating() || firstLetter->isInline());

    if (Node::diff(firstLetter->style(), pseudoStyle, document()) == Node::Detach) {
        // The first-letter renderer needs to be replaced. Create a new renderer of the right type.
        RenderObject* newFirstLetter;
        if (pseudoStyle->display() == INLINE)
            newFirstLetter = RenderInline::createAnonymous(document());
        else
            newFirstLetter = RenderBlock::createAnonymous(document());
        newFirstLetter->setStyle(pseudoStyle);

        // Move the first letter into the new renderer.
        LayoutStateDisabler layoutStateDisabler(view());
        while (RenderObject* child = firstLetter->firstChild()) {
            if (child->isText())
                toRenderText(child)->removeAndDestroyTextBoxes();
            firstLetter->removeChild(child);
            newFirstLetter->addChild(child, 0);
        }

        RenderTextFragment* remainingText = 0;
        RenderObject* nextSibling = firstLetter->nextSibling();
        RenderObject* remainingTextObject = toRenderBoxModelObject(firstLetter)->firstLetterRemainingText();
        if (remainingTextObject && remainingTextObject->isText() && toRenderText(remainingTextObject)->isTextFragment())
            remainingText = toRenderTextFragment(remainingTextObject);
        if (remainingText) {
            ASSERT(remainingText->isAnonymous() || remainingText->node()->renderer() == remainingText);
            // Replace the old renderer with the new one.
            remainingText->setFirstLetter(newFirstLetter);
            toRenderBoxModelObject(newFirstLetter)->setFirstLetterRemainingText(remainingText);
        }
        // To prevent removal of single anonymous block in RenderBlock::removeChild and causing
        // |nextSibling| to go stale, we remove the old first letter using removeChildNode first.
        firstLetterContainer->virtualChildren()->removeChildNode(firstLetterContainer, firstLetter);
        firstLetter->destroy();
        firstLetter = newFirstLetter;
        firstLetterContainer->addChild(firstLetter, nextSibling);
    } else
        firstLetter->setStyle(pseudoStyle);

    for (RenderObject* genChild = firstLetter->firstChild(); genChild; genChild = genChild->nextSibling()) {
        if (genChild->isText())
            genChild->setStyle(pseudoStyle);
    }
}

void RenderBlock::createFirstLetterRenderer(RenderObject* firstLetterBlock, RenderObject* currentChild)
{
    RenderObject* firstLetterContainer = currentChild->parent();
    RenderStyle* pseudoStyle = styleForFirstLetter(firstLetterBlock, firstLetterContainer);
    RenderObject* firstLetter = 0;
    if (pseudoStyle->display() == INLINE)
        firstLetter = RenderInline::createAnonymous(document());
    else
        firstLetter = RenderBlock::createAnonymous(document());
    firstLetter->setStyle(pseudoStyle);
    firstLetterContainer->addChild(firstLetter, currentChild);

    RenderText* textObj = toRenderText(currentChild);

    // The original string is going to be either a generated content string or a DOM node's
    // string.  We want the original string before it got transformed in case first-letter has
    // no text-transform or a different text-transform applied to it.
    RefPtr<StringImpl> oldText = textObj->originalText();
    ASSERT(oldText);

    if (oldText && oldText->length() > 0) {
        unsigned length = 0;

        // Account for leading spaces and punctuation.
        while (length < oldText->length() && shouldSkipForFirstLetter((*oldText)[length]))
            length++;

        // Account for first letter.
        length++;
        
        // Keep looking for whitespace and allowed punctuation, but avoid
        // accumulating just whitespace into the :first-letter.
        for (unsigned scanLength = length; scanLength < oldText->length(); ++scanLength) {
            UChar c = (*oldText)[scanLength]; 
            
            if (!shouldSkipForFirstLetter(c))
                break;

            if (isPunctuationForFirstLetter(c))
                length = scanLength + 1;
         }
         
        // Construct a text fragment for the text after the first letter.
        // This text fragment might be empty.
        RenderTextFragment* remainingText = 
            new (renderArena()) RenderTextFragment(textObj->node() ? textObj->node() : textObj->document(), oldText.get(), length, oldText->length() - length);
        remainingText->setStyle(textObj->style());
        if (remainingText->node())
            remainingText->node()->setRenderer(remainingText);

        firstLetterContainer->addChild(remainingText, textObj);
        firstLetterContainer->removeChild(textObj);
        remainingText->setFirstLetter(firstLetter);
        toRenderBoxModelObject(firstLetter)->setFirstLetterRemainingText(remainingText);
        
        // construct text fragment for the first letter
        RenderTextFragment* letter = 
            new (renderArena()) RenderTextFragment(remainingText->node() ? remainingText->node() : remainingText->document(), oldText.get(), 0, length);
        letter->setStyle(pseudoStyle);
        firstLetter->addChild(letter);

        textObj->destroy();
    }
}

void RenderBlock::updateFirstLetter()
{
    if (!document()->styleSheetCollection()->usesFirstLetterRules())
        return;
    // Don't recur
    if (style()->styleType() == FIRST_LETTER)
        return;

    // FIXME: We need to destroy the first-letter object if it is no longer the first child. Need to find
    // an efficient way to check for that situation though before implementing anything.
    RenderObject* firstLetterBlock = findFirstLetterBlock(this);
    if (!firstLetterBlock)
        return;

    // Drill into inlines looking for our first text child.
    RenderObject* currChild = firstLetterBlock->firstChild();
    while (currChild) {
        if (currChild->isText())
            break;
        if (currChild->isListMarker())
            currChild = currChild->nextSibling();
        else if (currChild->isFloatingOrOutOfFlowPositioned()) {
            if (currChild->style()->styleType() == FIRST_LETTER) {
                currChild = currChild->firstChild();
                break;
            }
            currChild = currChild->nextSibling();
        } else if (currChild->isReplaced() || currChild->isRenderButton() || currChild->isMenuList())
            break;
        else if (currChild->style()->hasPseudoStyle(FIRST_LETTER) && currChild->canHaveGeneratedChildren())  {
            // We found a lower-level node with first-letter, which supersedes the higher-level style
            firstLetterBlock = currChild;
            currChild = currChild->firstChild();
        } else
            currChild = currChild->firstChild();
    }

    if (!currChild)
        return;

    // If the child already has style, then it has already been created, so we just want
    // to update it.
    if (currChild->parent()->style()->styleType() == FIRST_LETTER) {
        updateFirstLetterStyle(firstLetterBlock, currChild);
        return;
    }

    if (!currChild->isText() || currChild->isBR())
        return;

    // Our layout state is not valid for the repaints we are going to trigger by
    // adding and removing children of firstLetterContainer.
    LayoutStateDisabler layoutStateDisabler(view());

    createFirstLetterRenderer(firstLetterBlock, currChild);
}

// Helper methods for obtaining the last line, computing line counts and heights for line counts
// (crawling into blocks).
static bool shouldCheckLines(RenderObject* obj)
{
    return !obj->isFloatingOrOutOfFlowPositioned() && !obj->isRunIn()
            && obj->isBlockFlow() && obj->style()->height().isAuto()
            && (!obj->isDeprecatedFlexibleBox() || obj->style()->boxOrient() == VERTICAL);
}

static int getHeightForLineCount(RenderBlock* block, int l, bool includeBottom, int& count)
{
    if (block->style()->visibility() == VISIBLE) {
        if (block->childrenInline()) {
            for (RootInlineBox* box = block->firstRootBox(); box; box = box->nextRootBox()) {
                if (++count == l)
                    return box->lineBottom() + (includeBottom ? (block->borderBottom() + block->paddingBottom()) : LayoutUnit());
            }
        }
        else {
            RenderBox* normalFlowChildWithoutLines = 0;
            for (RenderBox* obj = block->firstChildBox(); obj; obj = obj->nextSiblingBox()) {
                if (shouldCheckLines(obj)) {
                    int result = getHeightForLineCount(toRenderBlock(obj), l, false, count);
                    if (result != -1)
                        return result + obj->y() + (includeBottom ? (block->borderBottom() + block->paddingBottom()) : LayoutUnit());
                } else if (!obj->isFloatingOrOutOfFlowPositioned() && !obj->isRunIn())
                    normalFlowChildWithoutLines = obj;
            }
            if (normalFlowChildWithoutLines && l == 0)
                return normalFlowChildWithoutLines->y() + normalFlowChildWithoutLines->height();
        }
    }
    
    return -1;
}

RootInlineBox* RenderBlock::lineAtIndex(int i) const
{
    ASSERT(i >= 0);

    if (style()->visibility() != VISIBLE)
        return 0;

    if (childrenInline()) {
        for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox())
            if (!i--)
                return box;
    } else {
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if (!shouldCheckLines(child))
                continue;
            if (RootInlineBox* box = toRenderBlock(child)->lineAtIndex(i))
                return box;
        }
    }

    return 0;
}

int RenderBlock::lineCount(const RootInlineBox* stopRootInlineBox, bool* found) const
{
    int count = 0;

    if (style()->visibility() == VISIBLE) {
        if (childrenInline())
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox()) {
                count++;
                if (box == stopRootInlineBox) {
                    if (found)
                        *found = true;
                    break;
                }
            }
        else
            for (RenderObject* obj = firstChild(); obj; obj = obj->nextSibling())
                if (shouldCheckLines(obj)) {
                    bool recursiveFound = false;
                    count += toRenderBlock(obj)->lineCount(stopRootInlineBox, &recursiveFound);
                    if (recursiveFound) {
                        if (found)
                            *found = true;
                        break;
                    }
                }
    }
    return count;
}

int RenderBlock::heightForLineCount(int l)
{
    int count = 0;
    return getHeightForLineCount(this, l, true, count);
}

void RenderBlock::adjustForBorderFit(LayoutUnit x, LayoutUnit& left, LayoutUnit& right) const
{
    // We don't deal with relative positioning.  Our assumption is that you shrink to fit the lines without accounting
    // for either overflow or translations via relative positioning.
    if (style()->visibility() == VISIBLE) {
        if (childrenInline()) {
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox()) {
                if (box->firstChild())
                    left = min(left, x + static_cast<LayoutUnit>(box->firstChild()->x()));
                if (box->lastChild())
                    right = max(right, x + static_cast<LayoutUnit>(ceilf(box->lastChild()->logicalRight())));
            }
        }
        else {
            for (RenderBox* obj = firstChildBox(); obj; obj = obj->nextSiblingBox()) {
                if (!obj->isFloatingOrOutOfFlowPositioned()) {
                    if (obj->isBlockFlow() && !obj->hasOverflowClip())
                        toRenderBlock(obj)->adjustForBorderFit(x + obj->x(), left, right);
                    else if (obj->style()->visibility() == VISIBLE) {
                        // We are a replaced element or some kind of non-block-flow object.
                        left = min(left, x + obj->x());
                        right = max(right, x + obj->x() + obj->width());
                    }
                }
            }
        }
        
        if (m_floatingObjects) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* r = *it;
                // Only examine the object if our m_shouldPaint flag is set.
                if (r->shouldPaint()) {
                    LayoutUnit floatLeft = xPositionForFloatIncludingMargin(r) - r->m_renderer->x();
                    LayoutUnit floatRight = floatLeft + r->m_renderer->width();
                    left = min(left, floatLeft);
                    right = max(right, floatRight);
                }
            }
        }
    }
}

void RenderBlock::fitBorderToLinesIfNeeded()
{
    if (style()->borderFit() == BorderFitBorder || hasOverrideWidth())
        return;

    // Walk any normal flow lines to snugly fit.
    LayoutUnit left = LayoutUnit::max();
    LayoutUnit right = LayoutUnit::min();
    LayoutUnit oldWidth = contentWidth();
    adjustForBorderFit(0, left, right);
    
    // Clamp to our existing edges. We can never grow. We only shrink.
    LayoutUnit leftEdge = borderLeft() + paddingLeft();
    LayoutUnit rightEdge = leftEdge + oldWidth;
    left = min(rightEdge, max(leftEdge, left));
    right = max(leftEdge, min(rightEdge, right));
    
    LayoutUnit newContentWidth = right - left;
    if (newContentWidth == oldWidth)
        return;
    
    setOverrideLogicalContentWidth(newContentWidth);
    layoutBlock(false);
    clearOverrideLogicalContentWidth();
}

void RenderBlock::clearTruncation()
{
    if (style()->visibility() == VISIBLE) {
        if (childrenInline() && hasMarkupTruncation()) {
            setHasMarkupTruncation(false);
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox())
                box->clearTruncation();
        } else {
            for (RenderObject* obj = firstChild(); obj; obj = obj->nextSibling()) {
                if (shouldCheckLines(obj))
                    toRenderBlock(obj)->clearTruncation();
            }
        }
    }
}

void RenderBlock::setMaxMarginBeforeValues(LayoutUnit pos, LayoutUnit neg)
{
    if (!m_rareData) {
        if (pos == RenderBlockRareData::positiveMarginBeforeDefault(this) && neg == RenderBlockRareData::negativeMarginBeforeDefault(this))
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_margins.setPositiveMarginBefore(pos);
    m_rareData->m_margins.setNegativeMarginBefore(neg);
}

void RenderBlock::setMaxMarginAfterValues(LayoutUnit pos, LayoutUnit neg)
{
    if (!m_rareData) {
        if (pos == RenderBlockRareData::positiveMarginAfterDefault(this) && neg == RenderBlockRareData::negativeMarginAfterDefault(this))
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_margins.setPositiveMarginAfter(pos);
    m_rareData->m_margins.setNegativeMarginAfter(neg);
}

void RenderBlock::setMustDiscardMarginBefore(bool value)
{
    if (style()->marginBeforeCollapse() == MDISCARD) {
        ASSERT(value);
        return;
    }
    
    if (!m_rareData && !value)
        return;

    if (!m_rareData)
        m_rareData = adoptPtr(new RenderBlockRareData(this));

    m_rareData->m_discardMarginBefore = value;
}

void RenderBlock::setMustDiscardMarginAfter(bool value)
{
    if (style()->marginAfterCollapse() == MDISCARD) {
        ASSERT(value);
        return;
    }

    if (!m_rareData && !value)
        return;

    if (!m_rareData)
        m_rareData = adoptPtr(new RenderBlockRareData(this));

    m_rareData->m_discardMarginAfter = value;
}

bool RenderBlock::mustDiscardMarginBefore() const
{
    return style()->marginBeforeCollapse() == MDISCARD || (m_rareData && m_rareData->m_discardMarginBefore);
}

bool RenderBlock::mustDiscardMarginAfter() const
{
    return style()->marginAfterCollapse() == MDISCARD || (m_rareData && m_rareData->m_discardMarginAfter);
}

bool RenderBlock::mustDiscardMarginBeforeForChild(const RenderBox* child) const
{
    ASSERT(!child->selfNeedsLayout());
    if (!child->isWritingModeRoot())
        return child->isRenderBlock() ? toRenderBlock(child)->mustDiscardMarginBefore() : (child->style()->marginBeforeCollapse() == MDISCARD);
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->isRenderBlock() ? toRenderBlock(child)->mustDiscardMarginAfter() : (child->style()->marginAfterCollapse() == MDISCARD);

    // FIXME: We return false here because the implementation is not geometrically complete. We have values only for before/after, not start/end.
    // In case the boxes are perpendicular we assume the property is not specified.
    return false;
}

bool RenderBlock::mustDiscardMarginAfterForChild(const RenderBox* child) const
{
    ASSERT(!child->selfNeedsLayout());
    if (!child->isWritingModeRoot())
        return child->isRenderBlock() ? toRenderBlock(child)->mustDiscardMarginAfter() : (child->style()->marginAfterCollapse() == MDISCARD);
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->isRenderBlock() ? toRenderBlock(child)->mustDiscardMarginBefore() : (child->style()->marginBeforeCollapse() == MDISCARD);

    // FIXME: See |mustDiscardMarginBeforeForChild| above.
    return false;
}

bool RenderBlock::mustSeparateMarginBeforeForChild(const RenderBox* child) const
{
    ASSERT(!child->selfNeedsLayout());
    const RenderStyle* childStyle = child->style();
    if (!child->isWritingModeRoot())
        return childStyle->marginBeforeCollapse() == MSEPARATE;
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return childStyle->marginAfterCollapse() == MSEPARATE;

    // FIXME: See |mustDiscardMarginBeforeForChild| above.
    return false;
}

bool RenderBlock::mustSeparateMarginAfterForChild(const RenderBox* child) const
{
    ASSERT(!child->selfNeedsLayout());
    const RenderStyle* childStyle = child->style();
    if (!child->isWritingModeRoot())
        return childStyle->marginAfterCollapse() == MSEPARATE;
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return childStyle->marginBeforeCollapse() == MSEPARATE;

    // FIXME: See |mustDiscardMarginBeforeForChild| above.
    return false;
}

void RenderBlock::setPaginationStrut(LayoutUnit strut)
{
    if (!m_rareData) {
        if (!strut)
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_paginationStrut = strut;
}

void RenderBlock::setPageLogicalOffset(LayoutUnit logicalOffset)
{
    if (!m_rareData) {
        if (!logicalOffset)
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_pageLogicalOffset = logicalOffset;
}

void RenderBlock::setBreakAtLineToAvoidWidow(RootInlineBox* lineToBreak)
{
    ASSERT(lineToBreak);
    if (!m_rareData)
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    m_rareData->m_shouldBreakAtLineToAvoidWidow = true;
    m_rareData->m_lineBreakToAvoidWidow = lineToBreak;
}

void RenderBlock::clearShouldBreakAtLineToAvoidWidow() const
{
    if (!m_rareData)
        return;
    m_rareData->m_shouldBreakAtLineToAvoidWidow = false;
    m_rareData->m_lineBreakToAvoidWidow = 0;
}

void RenderBlock::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (isAnonymousBlockContinuation()) {
        // FIXME: This is wrong for block-flows that are horizontal.
        // https://bugs.webkit.org/show_bug.cgi?id=46781
        rects.append(pixelSnappedIntRect(accumulatedOffset.x(), accumulatedOffset.y() - collapsedMarginBefore(),
                                width(), height() + collapsedMarginBefore() + collapsedMarginAfter()));
        continuation()->absoluteRects(rects, accumulatedOffset - toLayoutSize(location() +
                inlineElementContinuation()->containingBlock()->location()));
    } else
        rects.append(pixelSnappedIntRect(accumulatedOffset, size()));
}

void RenderBlock::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (isAnonymousBlockContinuation()) {
        // FIXME: This is wrong for block-flows that are horizontal.
        // https://bugs.webkit.org/show_bug.cgi?id=46781
        FloatRect localRect(0, -collapsedMarginBefore(),
                            width(), height() + collapsedMarginBefore() + collapsedMarginAfter());
        quads.append(localToAbsoluteQuad(localRect, 0 /* mode */, wasFixed));
        continuation()->absoluteQuads(quads, wasFixed);
    } else
        quads.append(RenderBox::localToAbsoluteQuad(FloatRect(0, 0, width(), height()), 0 /* mode */, wasFixed));
}

LayoutRect RenderBlock::rectWithOutlineForRepaint(const RenderLayerModelObject* repaintContainer, LayoutUnit outlineWidth) const
{
    LayoutRect r(RenderBox::rectWithOutlineForRepaint(repaintContainer, outlineWidth));
    if (isAnonymousBlockContinuation())
        r.inflateY(collapsedMarginBefore()); // FIXME: This is wrong for block-flows that are horizontal.
    return r;
}

RenderObject* RenderBlock::hoverAncestor() const
{
    return isAnonymousBlockContinuation() ? continuation() : RenderBox::hoverAncestor();
}

void RenderBlock::updateDragState(bool dragOn)
{
    RenderBox::updateDragState(dragOn);
    if (continuation())
        continuation()->updateDragState(dragOn);
}

RenderStyle* RenderBlock::outlineStyleForRepaint() const
{
    return isAnonymousBlockContinuation() ? continuation()->style() : style();
}

void RenderBlock::childBecameNonInline(RenderObject*)
{
    makeChildrenNonInline();
    if (isAnonymousBlock() && parent() && parent()->isRenderBlock())
        toRenderBlock(parent())->removeLeftoverAnonymousBlock(this);
    // |this| may be dead here
}

void RenderBlock::updateHitTestResult(HitTestResult& result, const LayoutPoint& point)
{
    if (result.innerNode())
        return;

    if (Node* n = nodeForHitTest()) {
        result.setInnerNode(n);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(n);
        result.setLocalPoint(point);
    }
}

LayoutRect RenderBlock::localCaretRect(InlineBox* inlineBox, int caretOffset, LayoutUnit* extraWidthToEndOfLine)
{
    // Do the normal calculation in most cases.
    if (firstChild())
        return RenderBox::localCaretRect(inlineBox, caretOffset, extraWidthToEndOfLine);

    LayoutRect caretRect = localCaretRectForEmptyElement(width(), textIndentOffset());

    // FIXME: Does this need to adjust for vertical orientation?
    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = width() - caretRect.maxX();

    return caretRect;
}

void RenderBlock::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (inlineElementContinuation()) {
        // FIXME: This check really isn't accurate. 
        bool nextInlineHasLineBox = inlineElementContinuation()->firstLineBox();
        // FIXME: This is wrong. The principal renderer may not be the continuation preceding this block.
        // FIXME: This is wrong for block-flows that are horizontal.
        // https://bugs.webkit.org/show_bug.cgi?id=46781
        bool prevInlineHasLineBox = toRenderInline(inlineElementContinuation()->node()->renderer())->firstLineBox(); 
        float topMargin = prevInlineHasLineBox ? collapsedMarginBefore() : LayoutUnit();
        float bottomMargin = nextInlineHasLineBox ? collapsedMarginAfter() : LayoutUnit();
        LayoutRect rect(additionalOffset.x(), additionalOffset.y() - topMargin, width(), height() + topMargin + bottomMargin);
        if (!rect.isEmpty())
            rects.append(pixelSnappedIntRect(rect));
    } else if (width() && height())
        rects.append(pixelSnappedIntRect(additionalOffset, size()));

    if (!hasOverflowClip() && !hasControlClip()) {
        for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
            LayoutUnit top = max<LayoutUnit>(curr->lineTop(), curr->top());
            LayoutUnit bottom = min<LayoutUnit>(curr->lineBottom(), curr->top() + curr->height());
            LayoutRect rect(additionalOffset.x() + curr->x(), additionalOffset.y() + top, curr->width(), bottom - top);
            if (!rect.isEmpty())
                rects.append(pixelSnappedIntRect(rect));
        }

        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (!curr->isText() && !curr->isListMarker() && curr->isBox()) {
                RenderBox* box = toRenderBox(curr);
                FloatPoint pos;
                // FIXME: This doesn't work correctly with transforms.
                if (box->layer()) 
                    pos = curr->localToContainerPoint(FloatPoint(), paintContainer);
                else
                    pos = FloatPoint(additionalOffset.x() + box->x(), additionalOffset.y() + box->y());
                box->addFocusRingRects(rects, flooredLayoutPoint(pos), paintContainer);
            }
        }
    }

    if (inlineElementContinuation())
        inlineElementContinuation()->addFocusRingRects(rects, flooredLayoutPoint(additionalOffset + inlineElementContinuation()->containingBlock()->location() - location()), paintContainer);
}

RenderBox* RenderBlock::createAnonymousBoxWithSameTypeAs(const RenderObject* parent) const
{
    if (isAnonymousColumnsBlock())
        return createAnonymousColumnsWithParentRenderer(parent);
    if (isAnonymousColumnSpanBlock())
        return createAnonymousColumnSpanWithParentRenderer(parent);
    return createAnonymousWithParentRendererAndDisplay(parent, style()->display());
}

bool RenderBlock::hasNextPage(LayoutUnit logicalOffset, PageBoundaryRule pageBoundaryRule) const
{
    ASSERT(view()->layoutState() && view()->layoutState()->isPaginated());

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread)
        return true; // Printing and multi-column both make new pages to accommodate content.

    // See if we're in the last region.
    LayoutUnit pageOffset = offsetFromLogicalTopOfFirstPage() + logicalOffset;
    RenderRegion* region = flowThread->regionAtBlockOffset(pageOffset, this);
    if (!region)
        return false;
    if (region->isLastRegion())
        return region->isRenderRegionSet() || region->style()->regionFragment() == BreakRegionFragment
            || (pageBoundaryRule == IncludePageBoundary && pageOffset == region->logicalTopForFlowThreadContent());
    return true;
}

LayoutUnit RenderBlock::nextPageLogicalTop(LayoutUnit logicalOffset, PageBoundaryRule pageBoundaryRule) const
{
    LayoutUnit pageLogicalHeight = pageLogicalHeightForOffset(logicalOffset);
    if (!pageLogicalHeight)
        return logicalOffset;
    
    // The logicalOffset is in our coordinate space.  We can add in our pushed offset.
    LayoutUnit remainingLogicalHeight = pageRemainingLogicalHeightForOffset(logicalOffset);
    if (pageBoundaryRule == ExcludePageBoundary)
        return logicalOffset + (remainingLogicalHeight ? remainingLogicalHeight : pageLogicalHeight);
    return logicalOffset + remainingLogicalHeight;
}

static bool inNormalFlow(RenderBox* child)
{
    RenderBlock* curr = child->containingBlock();
    RenderView* renderView = child->view();
    while (curr && curr != renderView) {
        if (curr->hasColumns() || curr->isRenderFlowThread())
            return true;
        if (curr->isFloatingOrOutOfFlowPositioned())
            return false;
        curr = curr->containingBlock();
    }
    return true;
}

ColumnInfo::PaginationUnit RenderBlock::paginationUnit() const
{
    return ColumnInfo::Column;
}

LayoutUnit RenderBlock::applyBeforeBreak(RenderBox* child, LayoutUnit logicalOffset)
{
    // FIXME: Add page break checking here when we support printing.
    bool checkColumnBreaks = view()->layoutState()->isPaginatingColumns();
    bool checkPageBreaks = !checkColumnBreaks && view()->layoutState()->m_pageLogicalHeight; // FIXME: Once columns can print we have to check this.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool checkRegionBreaks = flowThread && flowThread->isRenderNamedFlowThread();
    bool checkBeforeAlways = (checkColumnBreaks && child->style()->columnBreakBefore() == PBALWAYS) || (checkPageBreaks && child->style()->pageBreakBefore() == PBALWAYS)
                             || (checkRegionBreaks && child->style()->regionBreakBefore() == PBALWAYS);
    if (checkBeforeAlways && inNormalFlow(child) && hasNextPage(logicalOffset, IncludePageBoundary)) {
        if (checkColumnBreaks)
            view()->layoutState()->addForcedColumnBreak(child, logicalOffset);
        if (checkRegionBreaks) {
            LayoutUnit offsetBreakAdjustment = 0;
            if (flowThread->addForcedRegionBreak(offsetFromLogicalTopOfFirstPage() + logicalOffset, child, true, &offsetBreakAdjustment))
                return logicalOffset + offsetBreakAdjustment;
        }
        return nextPageLogicalTop(logicalOffset, IncludePageBoundary);
    }
    return logicalOffset;
}

LayoutUnit RenderBlock::applyAfterBreak(RenderBox* child, LayoutUnit logicalOffset, MarginInfo& marginInfo)
{
    // FIXME: Add page break checking here when we support printing.
    bool checkColumnBreaks = view()->layoutState()->isPaginatingColumns();
    bool checkPageBreaks = !checkColumnBreaks && view()->layoutState()->m_pageLogicalHeight; // FIXME: Once columns can print we have to check this.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool checkRegionBreaks = flowThread && flowThread->isRenderNamedFlowThread();
    bool checkAfterAlways = (checkColumnBreaks && child->style()->columnBreakAfter() == PBALWAYS) || (checkPageBreaks && child->style()->pageBreakAfter() == PBALWAYS)
                            || (checkRegionBreaks && child->style()->regionBreakAfter() == PBALWAYS);
    if (checkAfterAlways && inNormalFlow(child) && hasNextPage(logicalOffset, IncludePageBoundary)) {
        LayoutUnit marginOffset = marginInfo.canCollapseWithMarginBefore() ? LayoutUnit() : marginInfo.margin();

        // So our margin doesn't participate in the next collapsing steps.
        marginInfo.clearMargin();

        if (checkColumnBreaks)
            view()->layoutState()->addForcedColumnBreak(child, logicalOffset);
        if (checkRegionBreaks) {
            LayoutUnit offsetBreakAdjustment = 0;
            if (flowThread->addForcedRegionBreak(offsetFromLogicalTopOfFirstPage() + logicalOffset + marginOffset, child, false, &offsetBreakAdjustment))
                return logicalOffset + marginOffset + offsetBreakAdjustment;
        }
        return nextPageLogicalTop(logicalOffset, IncludePageBoundary);
    }
    return logicalOffset;
}

LayoutUnit RenderBlock::pageLogicalTopForOffset(LayoutUnit offset) const
{
    RenderView* renderView = view();
    LayoutUnit firstPageLogicalTop = isHorizontalWritingMode() ? renderView->layoutState()->m_pageOffset.height() : renderView->layoutState()->m_pageOffset.width();
    LayoutUnit blockLogicalTop = isHorizontalWritingMode() ? renderView->layoutState()->m_layoutOffset.height() : renderView->layoutState()->m_layoutOffset.width();

    LayoutUnit cumulativeOffset = offset + blockLogicalTop;
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread) {
        LayoutUnit pageLogicalHeight = renderView->layoutState()->pageLogicalHeight();
        if (!pageLogicalHeight)
            return 0;
        return cumulativeOffset - roundToInt(cumulativeOffset - firstPageLogicalTop) % roundToInt(pageLogicalHeight);
    }
    return flowThread->pageLogicalTopForOffset(cumulativeOffset);
}

LayoutUnit RenderBlock::pageLogicalHeightForOffset(LayoutUnit offset) const
{
    RenderView* renderView = view();
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread)
        return renderView->layoutState()->m_pageLogicalHeight;
    return flowThread->pageLogicalHeightForOffset(offset + offsetFromLogicalTopOfFirstPage());
}

LayoutUnit RenderBlock::pageRemainingLogicalHeightForOffset(LayoutUnit offset, PageBoundaryRule pageBoundaryRule) const
{
    RenderView* renderView = view();
    offset += offsetFromLogicalTopOfFirstPage();
    
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread) {
        LayoutUnit pageLogicalHeight = renderView->layoutState()->m_pageLogicalHeight;
        LayoutUnit remainingHeight = pageLogicalHeight - intMod(offset, pageLogicalHeight);
        if (pageBoundaryRule == IncludePageBoundary) {
            // If includeBoundaryPoint is true the line exactly on the top edge of a
            // column will act as being part of the previous column.
            remainingHeight = intMod(remainingHeight, pageLogicalHeight);
        }
        return remainingHeight;
    }
    
    return flowThread->pageRemainingLogicalHeightForOffset(offset, pageBoundaryRule);
}

LayoutUnit RenderBlock::adjustForUnsplittableChild(RenderBox* child, LayoutUnit logicalOffset, bool includeMargins)
{
    bool checkColumnBreaks = view()->layoutState()->isPaginatingColumns();
    bool checkPageBreaks = !checkColumnBreaks && view()->layoutState()->m_pageLogicalHeight;
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool checkRegionBreaks = flowThread && flowThread->isRenderNamedFlowThread();
    bool isUnsplittable = child->isUnsplittableForPagination() || (checkColumnBreaks && child->style()->columnBreakInside() == PBAVOID)
        || (checkPageBreaks && child->style()->pageBreakInside() == PBAVOID)
        || (checkRegionBreaks && child->style()->regionBreakInside() == PBAVOID);
    if (!isUnsplittable)
        return logicalOffset;
    LayoutUnit childLogicalHeight = logicalHeightForChild(child) + (includeMargins ? marginBeforeForChild(child) + marginAfterForChild(child) : LayoutUnit());
    LayoutUnit pageLogicalHeight = pageLogicalHeightForOffset(logicalOffset);
    bool hasUniformPageLogicalHeight = !flowThread || flowThread->regionsHaveUniformLogicalHeight();
    updateMinimumPageHeight(logicalOffset, childLogicalHeight);
    if (!pageLogicalHeight || (hasUniformPageLogicalHeight && childLogicalHeight > pageLogicalHeight)
        || !hasNextPage(logicalOffset))
        return logicalOffset;
    LayoutUnit remainingLogicalHeight = pageRemainingLogicalHeightForOffset(logicalOffset, ExcludePageBoundary);
    if (remainingLogicalHeight < childLogicalHeight) {
        if (!hasUniformPageLogicalHeight && !pushToNextPageWithMinimumLogicalHeight(remainingLogicalHeight, logicalOffset, childLogicalHeight))
            return logicalOffset;
        return logicalOffset + remainingLogicalHeight;
    }
    return logicalOffset;
}

bool RenderBlock::pushToNextPageWithMinimumLogicalHeight(LayoutUnit& adjustment, LayoutUnit logicalOffset, LayoutUnit minimumLogicalHeight) const
{
    bool checkRegion = false;
    for (LayoutUnit pageLogicalHeight = pageLogicalHeightForOffset(logicalOffset + adjustment); pageLogicalHeight;
        pageLogicalHeight = pageLogicalHeightForOffset(logicalOffset + adjustment)) {
        if (minimumLogicalHeight <= pageLogicalHeight)
            return true;
        if (!hasNextPage(logicalOffset + adjustment))
            return false;
        adjustment += pageLogicalHeight;
        checkRegion = true;
    }
    return !checkRegion;
}

void RenderBlock::setPageBreak(LayoutUnit offset, LayoutUnit spaceShortage)
{
    if (RenderFlowThread* flowThread = flowThreadContainingBlock())
        flowThread->setPageBreak(offsetFromLogicalTopOfFirstPage() + offset, spaceShortage);
}

void RenderBlock::updateMinimumPageHeight(LayoutUnit offset, LayoutUnit minHeight)
{
    if (RenderFlowThread* flowThread = flowThreadContainingBlock())
        flowThread->updateMinimumPageHeight(offsetFromLogicalTopOfFirstPage() + offset, minHeight);
    else if (ColumnInfo* colInfo = view()->layoutState()->m_columnInfo)
        colInfo->updateMinimumColumnHeight(minHeight);
}

static inline LayoutUnit calculateMinimumPageHeight(RenderStyle* renderStyle, RootInlineBox* lastLine, LayoutUnit lineTop, LayoutUnit lineBottom)
{
    // We may require a certain minimum number of lines per page in order to satisfy
    // orphans and widows, and that may affect the minimum page height.
    unsigned lineCount = max<unsigned>(renderStyle->hasAutoOrphans() ? 1 : renderStyle->orphans(), renderStyle->hasAutoWidows() ? 1 : renderStyle->widows());
    if (lineCount > 1) {
        RootInlineBox* line = lastLine;
        for (unsigned i = 1; i < lineCount && line->prevRootBox(); i++)
            line = line->prevRootBox();

        // FIXME: Paginating using line overflow isn't all fine. See FIXME in
        // adjustLinePositionForPagination() for more details.
        LayoutRect overflow = line->logicalVisualOverflowRect(line->lineTop(), line->lineBottom());
        lineTop = min(line->lineTopWithLeading(), overflow.y());
    }
    return lineBottom - lineTop;
}

void RenderBlock::adjustLinePositionForPagination(RootInlineBox* lineBox, LayoutUnit& delta, RenderFlowThread* flowThread)
{
    // FIXME: For now we paginate using line overflow.  This ensures that lines don't overlap at all when we
    // put a strut between them for pagination purposes.  However, this really isn't the desired rendering, since
    // the line on the top of the next page will appear too far down relative to the same kind of line at the top
    // of the first column.
    //
    // The rendering we would like to see is one where the lineTopWithLeading is at the top of the column, and any line overflow
    // simply spills out above the top of the column.  This effect would match what happens at the top of the first column.
    // We can't achieve this rendering, however, until we stop columns from clipping to the column bounds (thus allowing
    // for overflow to occur), and then cache visible overflow for each column rect.
    //
    // Furthermore, the paint we have to do when a column has overflow has to be special.  We need to exclude
    // content that paints in a previous column (and content that paints in the following column).
    //
    // For now we'll at least honor the lineTopWithLeading when paginating if it is above the logical top overflow. This will
    // at least make positive leading work in typical cases.
    //
    // FIXME: Another problem with simply moving lines is that the available line width may change (because of floats).
    // Technically if the location we move the line to has a different line width than our old position, then we need to dirty the
    // line and all following lines.
    LayoutRect logicalVisualOverflow = lineBox->logicalVisualOverflowRect(lineBox->lineTop(), lineBox->lineBottom());
    LayoutUnit logicalOffset = min(lineBox->lineTopWithLeading(), logicalVisualOverflow.y());
    LayoutUnit logicalBottom = max(lineBox->lineBottomWithLeading(), logicalVisualOverflow.maxY());
    LayoutUnit lineHeight = logicalBottom - logicalOffset;
    updateMinimumPageHeight(logicalOffset, calculateMinimumPageHeight(style(), lineBox, logicalOffset, logicalBottom));
    logicalOffset += delta;
    lineBox->setPaginationStrut(0);
    lineBox->setIsFirstAfterPageBreak(false);
    LayoutUnit pageLogicalHeight = pageLogicalHeightForOffset(logicalOffset);
    bool hasUniformPageLogicalHeight = !flowThread || flowThread->regionsHaveUniformLogicalHeight();
    // If lineHeight is greater than pageLogicalHeight, but logicalVisualOverflow.height() still fits, we are
    // still going to add a strut, so that the visible overflow fits on a single page.
    if (!pageLogicalHeight || (hasUniformPageLogicalHeight && logicalVisualOverflow.height() > pageLogicalHeight)
        || !hasNextPage(logicalOffset))
        return;
    LayoutUnit remainingLogicalHeight = pageRemainingLogicalHeightForOffset(logicalOffset, ExcludePageBoundary);

    if (remainingLogicalHeight < lineHeight || (shouldBreakAtLineToAvoidWidow() && lineBreakToAvoidWidow() == lineBox)) {
        if (shouldBreakAtLineToAvoidWidow() && lineBreakToAvoidWidow() == lineBox)
            clearShouldBreakAtLineToAvoidWidow();
        // If we have a non-uniform page height, then we have to shift further possibly.
        if (!hasUniformPageLogicalHeight && !pushToNextPageWithMinimumLogicalHeight(remainingLogicalHeight, logicalOffset, lineHeight))
            return;
        if (lineHeight > pageLogicalHeight) {
            // Split the top margin in order to avoid splitting the visible part of the line.
            remainingLogicalHeight -= min(lineHeight - pageLogicalHeight, max<LayoutUnit>(0, logicalVisualOverflow.y() - lineBox->lineTopWithLeading()));
        }
        LayoutUnit totalLogicalHeight = lineHeight + max<LayoutUnit>(0, logicalOffset);
        LayoutUnit pageLogicalHeightAtNewOffset = hasUniformPageLogicalHeight ? pageLogicalHeight : pageLogicalHeightForOffset(logicalOffset + remainingLogicalHeight);
        setPageBreak(logicalOffset, lineHeight - remainingLogicalHeight);
        if (((lineBox == firstRootBox() && totalLogicalHeight < pageLogicalHeightAtNewOffset) || (!style()->hasAutoOrphans() && style()->orphans() >= lineCount(lineBox)))
            && !isOutOfFlowPositioned() && !isTableCell())
            setPaginationStrut(remainingLogicalHeight + max<LayoutUnit>(0, logicalOffset));
        else {
            delta += remainingLogicalHeight;
            lineBox->setPaginationStrut(remainingLogicalHeight);
            lineBox->setIsFirstAfterPageBreak(true);
        }
    } else if (remainingLogicalHeight == pageLogicalHeight && lineBox != firstRootBox())
        lineBox->setIsFirstAfterPageBreak(true);
}

LayoutUnit RenderBlock::adjustBlockChildForPagination(LayoutUnit logicalTopAfterClear, LayoutUnit estimateWithoutPagination, RenderBox* child, bool atBeforeSideOfBlock)
{
    RenderBlock* childRenderBlock = child->isRenderBlock() ? toRenderBlock(child) : 0;

    if (estimateWithoutPagination != logicalTopAfterClear) {
        // Our guess prior to pagination movement was wrong. Before we attempt to paginate, let's try again at the new
        // position.
        setLogicalHeight(logicalTopAfterClear);
        setLogicalTopForChild(child, logicalTopAfterClear, ApplyLayoutDelta);

        if (child->shrinkToAvoidFloats()) {
            // The child's width depends on the line width.
            // When the child shifts to clear an item, its width can
            // change (because it has more available line width).
            // So go ahead and mark the item as dirty.
            child->setChildNeedsLayout(true, MarkOnlyThis);
        }
        
        if (childRenderBlock) {
            if (!child->avoidsFloats() && childRenderBlock->containsFloats())
                childRenderBlock->markAllDescendantsWithFloatsForLayout();
            if (!child->needsLayout())
                child->markForPaginationRelayoutIfNeeded();
        }

        // Our guess was wrong. Make the child lay itself out again.
        child->layoutIfNeeded();
    }

    LayoutUnit oldTop = logicalTopAfterClear;

    // If the object has a page or column break value of "before", then we should shift to the top of the next page.
    LayoutUnit result = applyBeforeBreak(child, logicalTopAfterClear);

    if (pageLogicalHeightForOffset(result)) {
        LayoutUnit remainingLogicalHeight = pageRemainingLogicalHeightForOffset(result, ExcludePageBoundary);
        LayoutUnit spaceShortage = child->logicalHeight() - remainingLogicalHeight;
        if (spaceShortage > 0) {
            // If the child crosses a column boundary, report a break, in case nothing inside it has already
            // done so. The column balancer needs to know how much it has to stretch the columns to make more
            // content fit. If no breaks are reported (but do occur), the balancer will have no clue. FIXME:
            // This should be improved, though, because here we just pretend that the child is
            // unsplittable. A splittable child, on the other hand, has break opportunities at every position
            // where there's no child content, border or padding. In other words, we risk stretching more
            // than necessary.
            setPageBreak(result, spaceShortage);
        }
    }

    // For replaced elements and scrolled elements, we want to shift them to the next page if they don't fit on the current one.
    LayoutUnit logicalTopBeforeUnsplittableAdjustment = result;
    LayoutUnit logicalTopAfterUnsplittableAdjustment = adjustForUnsplittableChild(child, result);
    
    LayoutUnit paginationStrut = 0;
    LayoutUnit unsplittableAdjustmentDelta = logicalTopAfterUnsplittableAdjustment - logicalTopBeforeUnsplittableAdjustment;
    if (unsplittableAdjustmentDelta)
        paginationStrut = unsplittableAdjustmentDelta;
    else if (childRenderBlock && childRenderBlock->paginationStrut())
        paginationStrut = childRenderBlock->paginationStrut();

    if (paginationStrut) {
        // We are willing to propagate out to our parent block as long as we were at the top of the block prior
        // to collapsing our margins, and as long as we didn't clear or move as a result of other pagination.
        if (atBeforeSideOfBlock && oldTop == result && !isOutOfFlowPositioned() && !isTableCell()) {
            // FIXME: Should really check if we're exceeding the page height before propagating the strut, but we don't
            // have all the information to do so (the strut only has the remaining amount to push). Gecko gets this wrong too
            // and pushes to the next page anyway, so not too concerned about it.
            setPaginationStrut(result + paginationStrut);
            if (childRenderBlock)
                childRenderBlock->setPaginationStrut(0);
        } else
            result += paginationStrut;
    }

    // Similar to how we apply clearance. Go ahead and boost height() to be the place where we're going to position the child.
    setLogicalHeight(logicalHeight() + (result - oldTop));
    
    // Return the final adjusted logical top.
    return result;
}

bool RenderBlock::lineWidthForPaginatedLineChanged(RootInlineBox* rootBox, LayoutUnit lineDelta, RenderFlowThread* flowThread) const
{
    if (!flowThread)
        return false;

    RenderRegion* currentRegion = regionAtBlockOffset(rootBox->lineTopWithLeading() + lineDelta);
    // Just bail if the region didn't change.
    if (rootBox->containingRegion() == currentRegion)
        return false;
    return rootBox->paginatedLineWidth() != availableLogicalWidthForContent(currentRegion);
}

LayoutUnit RenderBlock::offsetFromLogicalTopOfFirstPage() const
{
    LayoutState* layoutState = view()->layoutState();
    if (layoutState && !layoutState->isPaginated())
        return 0;

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (flowThread)
        return flowThread->offsetFromLogicalTopOfFirstRegion(this);

    if (layoutState) {
        ASSERT(layoutState->m_renderer == this);

        LayoutSize offsetDelta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
        return isHorizontalWritingMode() ? offsetDelta.height() : offsetDelta.width();
    }
    
    ASSERT_NOT_REACHED();
    return 0;
}

RenderRegion* RenderBlock::regionAtBlockOffset(LayoutUnit blockOffset) const
{
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (!flowThread || !flowThread->hasValidRegionInfo())
        return 0;

    return flowThread->regionAtBlockOffset(offsetFromLogicalTopOfFirstPage() + blockOffset, true);
}

void RenderBlock::updateStaticInlinePositionForChild(RenderBox* child, LayoutUnit logicalTop)
{
    if (child->style()->isOriginalDisplayInlineType())
        setStaticInlinePositionForChild(child, logicalTop, startAlignedOffsetForLine(logicalTop, false));
    else
        setStaticInlinePositionForChild(child, logicalTop, startOffsetForContent(logicalTop));
}

void RenderBlock::setStaticInlinePositionForChild(RenderBox* child, LayoutUnit blockOffset, LayoutUnit inlinePosition)
{
    if (flowThreadContainingBlock()) {
        // Shift the inline position to exclude the region offset.
        inlinePosition += startOffsetForContent() - startOffsetForContent(blockOffset);
    }
    child->layer()->setStaticInlinePosition(inlinePosition);
}

bool RenderBlock::logicalWidthChangedInRegions(RenderFlowThread* flowThread) const
{
    if (!flowThread || !flowThread->hasValidRegionInfo())
        return false;
    
    return flowThread->logicalWidthChangedInRegionsForBlock(this);
}

RenderRegion* RenderBlock::clampToStartAndEndRegions(RenderRegion* region) const
{
    RenderFlowThread* flowThread = flowThreadContainingBlock();

    ASSERT(isRenderView() || (region && flowThread));
    if (isRenderView())
        return region;

    // We need to clamp to the block, since we want any lines or blocks that overflow out of the
    // logical top or logical bottom of the block to size as though the border box in the first and
    // last regions extended infinitely. Otherwise the lines are going to size according to the regions
    // they overflow into, which makes no sense when this block doesn't exist in |region| at all.
    RenderRegion* startRegion;
    RenderRegion* endRegion;
    flowThread->getRegionRangeForBox(this, startRegion, endRegion);
    
    if (startRegion && region->logicalTopForFlowThreadContent() < startRegion->logicalTopForFlowThreadContent())
        return startRegion;
    if (endRegion && region->logicalTopForFlowThreadContent() > endRegion->logicalTopForFlowThreadContent())
        return endRegion;
    
    return region;
}

LayoutUnit RenderBlock::collapsedMarginBeforeForChild(const RenderBox* child) const
{
    // If the child has the same directionality as we do, then we can just return its
    // collapsed margin.
    if (!child->isWritingModeRoot())
        return child->collapsedMarginBefore();
    
    // The child has a different directionality.  If the child is parallel, then it's just
    // flipped relative to us.  We can use the collapsed margin for the opposite edge.
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->collapsedMarginAfter();
    
    // The child is perpendicular to us, which means its margins don't collapse but are on the
    // "logical left/right" sides of the child box.  We can just return the raw margin in this case.  
    return marginBeforeForChild(child);
}

LayoutUnit RenderBlock::collapsedMarginAfterForChild(const  RenderBox* child) const
{
    // If the child has the same directionality as we do, then we can just return its
    // collapsed margin.
    if (!child->isWritingModeRoot())
        return child->collapsedMarginAfter();
    
    // The child has a different directionality.  If the child is parallel, then it's just
    // flipped relative to us.  We can use the collapsed margin for the opposite edge.
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->collapsedMarginBefore();
    
    // The child is perpendicular to us, which means its margins don't collapse but are on the
    // "logical left/right" side of the child box.  We can just return the raw margin in this case.  
    return marginAfterForChild(child);
}

bool RenderBlock::hasMarginBeforeQuirk(const RenderBox* child) const
{
    // If the child has the same directionality as we do, then we can just return its
    // margin quirk.
    if (!child->isWritingModeRoot())
        return child->isRenderBlock() ? toRenderBlock(child)->hasMarginBeforeQuirk() : child->style()->hasMarginBeforeQuirk();
    
    // The child has a different directionality. If the child is parallel, then it's just
    // flipped relative to us. We can use the opposite edge.
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->isRenderBlock() ? toRenderBlock(child)->hasMarginAfterQuirk() : child->style()->hasMarginAfterQuirk();
    
    // The child is perpendicular to us and box sides are never quirky in html.css, and we don't really care about
    // whether or not authors specified quirky ems, since they're an implementation detail.
    return false;
}

bool RenderBlock::hasMarginAfterQuirk(const RenderBox* child) const
{
    // If the child has the same directionality as we do, then we can just return its
    // margin quirk.
    if (!child->isWritingModeRoot())
        return child->isRenderBlock() ? toRenderBlock(child)->hasMarginAfterQuirk() : child->style()->hasMarginAfterQuirk();
    
    // The child has a different directionality. If the child is parallel, then it's just
    // flipped relative to us. We can use the opposite edge.
    if (child->isHorizontalWritingMode() == isHorizontalWritingMode())
        return child->isRenderBlock() ? toRenderBlock(child)->hasMarginBeforeQuirk() : child->style()->hasMarginBeforeQuirk();
    
    // The child is perpendicular to us and box sides are never quirky in html.css, and we don't really care about
    // whether or not authors specified quirky ems, since they're an implementation detail.
    return false;
}

RenderBlock::MarginValues RenderBlock::marginValuesForChild(RenderBox* child) const
{
    LayoutUnit childBeforePositive = 0;
    LayoutUnit childBeforeNegative = 0;
    LayoutUnit childAfterPositive = 0;
    LayoutUnit childAfterNegative = 0;

    LayoutUnit beforeMargin = 0;
    LayoutUnit afterMargin = 0;

    RenderBlock* childRenderBlock = child->isRenderBlock() ? toRenderBlock(child) : 0;
    
    // If the child has the same directionality as we do, then we can just return its
    // margins in the same direction.
    if (!child->isWritingModeRoot()) {
        if (childRenderBlock) {
            childBeforePositive = childRenderBlock->maxPositiveMarginBefore();
            childBeforeNegative = childRenderBlock->maxNegativeMarginBefore();
            childAfterPositive = childRenderBlock->maxPositiveMarginAfter();
            childAfterNegative = childRenderBlock->maxNegativeMarginAfter();
        } else {
            beforeMargin = child->marginBefore();
            afterMargin = child->marginAfter();
        }
    } else if (child->isHorizontalWritingMode() == isHorizontalWritingMode()) {
        // The child has a different directionality.  If the child is parallel, then it's just
        // flipped relative to us.  We can use the margins for the opposite edges.
        if (childRenderBlock) {
            childBeforePositive = childRenderBlock->maxPositiveMarginAfter();
            childBeforeNegative = childRenderBlock->maxNegativeMarginAfter();
            childAfterPositive = childRenderBlock->maxPositiveMarginBefore();
            childAfterNegative = childRenderBlock->maxNegativeMarginBefore();
        } else {
            beforeMargin = child->marginAfter();
            afterMargin = child->marginBefore();
        }
    } else {
        // The child is perpendicular to us, which means its margins don't collapse but are on the
        // "logical left/right" sides of the child box.  We can just return the raw margin in this case.
        beforeMargin = marginBeforeForChild(child);
        afterMargin = marginAfterForChild(child);
    }

    // Resolve uncollapsing margins into their positive/negative buckets.
    if (beforeMargin) {
        if (beforeMargin > 0)
            childBeforePositive = beforeMargin;
        else
            childBeforeNegative = -beforeMargin;
    }
    if (afterMargin) {
        if (afterMargin > 0)
            childAfterPositive = afterMargin;
        else
            childAfterNegative = -afterMargin;
    }

    return MarginValues(childBeforePositive, childBeforeNegative, childAfterPositive, childAfterNegative);
}

const char* RenderBlock::renderName() const
{
    if (isBody())
        return "RenderBody"; // FIXME: Temporary hack until we know that the regression tests pass.
    
    if (isFloating())
        return "RenderBlock (floating)";
    if (isOutOfFlowPositioned())
        return "RenderBlock (positioned)";
    if (style() && isAnonymousColumnsBlock())
        return "RenderBlock (anonymous multi-column)";
    if (style() && isAnonymousColumnSpanBlock())
        return "RenderBlock (anonymous multi-column span)";
    if (style() && isAnonymousBlock())
        return "RenderBlock (anonymous)";
    // FIXME: Temporary hack while the new generated content system is being implemented.
    if (isPseudoElement())
        return "RenderBlock (generated)";
    if (isAnonymous())
        return "RenderBlock (generated)";
    if (isRelPositioned())
        return "RenderBlock (relative positioned)";
    if (isStickyPositioned())
        return "RenderBlock (sticky positioned)";
    if (style() && isRunIn())
        return "RenderBlock (run-in)";
    return "RenderBlock";
}

inline RenderBlock::FloatingObjects::FloatingObjects(const RenderBlock* renderer, bool horizontalWritingMode)
    : m_placedFloatsTree(UninitializedTree)
    , m_leftObjectsCount(0)
    , m_rightObjectsCount(0)
    , m_horizontalWritingMode(horizontalWritingMode)
    , m_renderer(renderer)
{
}

void RenderBlock::createFloatingObjects()
{
    m_floatingObjects = adoptPtr(new FloatingObjects(this, isHorizontalWritingMode()));
}

inline void RenderBlock::FloatingObjects::clear()
{
    m_set.clear();
    m_placedFloatsTree.clear();
    m_leftObjectsCount = 0;
    m_rightObjectsCount = 0;
}

inline void RenderBlock::FloatingObjects::increaseObjectsCount(FloatingObject::Type type)
{    
    if (type == FloatingObject::FloatLeft)
        m_leftObjectsCount++;
    else 
        m_rightObjectsCount++;
}

inline void RenderBlock::FloatingObjects::decreaseObjectsCount(FloatingObject::Type type)
{
    if (type == FloatingObject::FloatLeft)
        m_leftObjectsCount--;
    else
        m_rightObjectsCount--;
}

inline RenderBlock::FloatingObjectInterval RenderBlock::FloatingObjects::intervalForFloatingObject(FloatingObject* floatingObject)
{
    if (m_horizontalWritingMode)
        return RenderBlock::FloatingObjectInterval(floatingObject->frameRect().y(), floatingObject->frameRect().maxY(), floatingObject);
    return RenderBlock::FloatingObjectInterval(floatingObject->frameRect().x(), floatingObject->frameRect().maxX(), floatingObject);
}

void RenderBlock::FloatingObjects::addPlacedObject(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->isInPlacedTree());

    floatingObject->setIsPlaced(true);
    if (m_placedFloatsTree.isInitialized())
        m_placedFloatsTree.add(intervalForFloatingObject(floatingObject));

#ifndef NDEBUG
    floatingObject->setIsInPlacedTree(true);      
#endif
}

void RenderBlock::FloatingObjects::removePlacedObject(FloatingObject* floatingObject)
{
    ASSERT(floatingObject->isPlaced() && floatingObject->isInPlacedTree());

    if (m_placedFloatsTree.isInitialized()) {
        bool removed = m_placedFloatsTree.remove(intervalForFloatingObject(floatingObject));
        ASSERT_UNUSED(removed, removed);
    }
    
    floatingObject->setIsPlaced(false);
#ifndef NDEBUG
    floatingObject->setIsInPlacedTree(false);
#endif
}

inline void RenderBlock::FloatingObjects::add(FloatingObject* floatingObject)
{
    increaseObjectsCount(floatingObject->type());
    m_set.add(floatingObject);
    if (floatingObject->isPlaced())
        addPlacedObject(floatingObject);
}

inline void RenderBlock::FloatingObjects::remove(FloatingObject* floatingObject)
{
    decreaseObjectsCount(floatingObject->type());
    m_set.remove(floatingObject);
    ASSERT(floatingObject->isPlaced() || !floatingObject->isInPlacedTree());
    if (floatingObject->isPlaced())
        removePlacedObject(floatingObject);
}

void RenderBlock::FloatingObjects::computePlacedFloatsTree()
{
    ASSERT(!m_placedFloatsTree.isInitialized());
    if (m_set.isEmpty())
        return;
    m_placedFloatsTree.initIfNeeded(m_renderer->view()->intervalArena());
    FloatingObjectSetIterator it = m_set.begin();
    FloatingObjectSetIterator end = m_set.end();
    for (; it != end; ++it) {
        FloatingObject* floatingObject = *it;
        if (floatingObject->isPlaced())
            m_placedFloatsTree.add(intervalForFloatingObject(floatingObject));
    }
}

template <typename CharacterType>
static inline TextRun constructTextRunInternal(RenderObject* context, const Font& font, const CharacterType* characters, int length, RenderStyle* style, TextRun::ExpansionBehavior expansion)
{
    ASSERT(style);

    TextDirection textDirection = LTR;
    bool directionalOverride = style->rtlOrdering() == VisualOrder;

    TextRun run(characters, length, 0, 0, expansion, textDirection, directionalOverride);
    if (textRunNeedsRenderingContext(font))
        run.setRenderingContext(SVGTextRunRenderingContext::create(context));

    return run;
}

template <typename CharacterType>
static inline TextRun constructTextRunInternal(RenderObject* context, const Font& font, const CharacterType* characters, int length, RenderStyle* style, TextRun::ExpansionBehavior expansion, TextRunFlags flags)
{
    ASSERT(style);

    TextDirection textDirection = LTR;
    bool directionalOverride = style->rtlOrdering() == VisualOrder;
    if (flags != DefaultTextRunFlags) {
        if (flags & RespectDirection)
            textDirection = style->direction();
        if (flags & RespectDirectionOverride)
            directionalOverride |= isOverride(style->unicodeBidi());
    }
    TextRun run(characters, length, 0, 0, expansion, textDirection, directionalOverride);
    if (textRunNeedsRenderingContext(font))
        run.setRenderingContext(SVGTextRunRenderingContext::create(context));

    return run;
}

#if ENABLE(8BIT_TEXTRUN)
TextRun RenderBlock::constructTextRun(RenderObject* context, const Font& font, const LChar* characters, int length, RenderStyle* style, TextRun::ExpansionBehavior expansion)
{
    return constructTextRunInternal(context, font, characters, length, style, expansion);
}
#endif

TextRun RenderBlock::constructTextRun(RenderObject* context, const Font& font, const UChar* characters, int length, RenderStyle* style, TextRun::ExpansionBehavior expansion)
{
    return constructTextRunInternal(context, font, characters, length, style, expansion);
}

TextRun RenderBlock::constructTextRun(RenderObject* context, const Font& font, const RenderText* text, RenderStyle* style, TextRun::ExpansionBehavior expansion)
{
#if ENABLE(8BIT_TEXTRUN)
    if (text->is8Bit())
        return constructTextRunInternal(context, font, text->characters8(), text->textLength(), style, expansion);
    return constructTextRunInternal(context, font, text->characters16(), text->textLength(), style, expansion);
#else
    return constructTextRunInternal(context, font, text->characters(), text->textLength(), style, expansion);
#endif
}

TextRun RenderBlock::constructTextRun(RenderObject* context, const Font& font, const RenderText* text, unsigned offset, unsigned length, RenderStyle* style, TextRun::ExpansionBehavior expansion)
{
    ASSERT(offset + length <= text->textLength());
#if ENABLE(8BIT_TEXTRUN)
    if (text->is8Bit())
        return constructTextRunInternal(context, font, text->characters8() + offset, length, style, expansion);
    return constructTextRunInternal(context, font, text->characters16() + offset, length, style, expansion);
#else
    return constructTextRunInternal(context, font, text->characters() + offset, length, style, expansion);
#endif
}

TextRun RenderBlock::constructTextRun(RenderObject* context, const Font& font, const String& string, RenderStyle* style, TextRun::ExpansionBehavior expansion, TextRunFlags flags)
{
    unsigned length = string.length();

#if ENABLE(8BIT_TEXTRUN)
    if (length && string.is8Bit())
        return constructTextRunInternal(context, font, string.characters8(), length, style, expansion, flags);
    return constructTextRunInternal(context, font, string.characters(), length, style, expansion, flags);
#else
    return constructTextRunInternal(context, font, string.characters(), length, style, expansion, flags);
#endif
}

RenderBlock* RenderBlock::createAnonymousWithParentRendererAndDisplay(const RenderObject* parent, EDisplay display)
{
    // FIXME: Do we need to convert all our inline displays to block-type in the anonymous logic ?
    EDisplay newDisplay;
    RenderBlock* newBox = 0;
    if (display == BOX || display == INLINE_BOX) {
        // FIXME: Remove this case once we have eliminated all internal users of old flexbox
        newBox = RenderDeprecatedFlexibleBox::createAnonymous(parent->document());
        newDisplay = BOX;
    } else if (display == FLEX || display == INLINE_FLEX) {
        newBox = RenderFlexibleBox::createAnonymous(parent->document());
        newDisplay = FLEX;
    } else {
        newBox = RenderBlock::createAnonymous(parent->document());
        newDisplay = BLOCK;
    }

    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), newDisplay);
    newBox->setStyle(newStyle.release());
    return newBox;
}

RenderBlock* RenderBlock::createAnonymousColumnsWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), BLOCK);
    newStyle->inheritColumnPropertiesFrom(parent->style());

    RenderBlock* newBox = RenderBlock::createAnonymous(parent->document());
    newBox->setStyle(newStyle.release());
    return newBox;
}

RenderBlock* RenderBlock::createAnonymousColumnSpanWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), BLOCK);
    newStyle->setColumnSpan(ColumnSpanAll);

    RenderBlock* newBox = RenderBlock::createAnonymous(parent->document());
    newBox->setStyle(newStyle.release());
    return newBox;
}

#ifndef NDEBUG
void RenderBlock::checkPositionedObjectsNeedLayout()
{
    if (!gPositionedDescendantsMap)
        return;

    if (TrackedRendererListHashSet* positionedDescendantSet = positionedObjects()) {
        TrackedRendererListHashSet::const_iterator end = positionedDescendantSet->end();
        for (TrackedRendererListHashSet::const_iterator it = positionedDescendantSet->begin(); it != end; ++it) {
            RenderBox* currBox = *it;
            ASSERT(!currBox->needsLayout());
        }
    }
}

void RenderBlock::showLineTreeAndMark(const InlineBox* markedBox1, const char* markedLabel1, const InlineBox* markedBox2, const char* markedLabel2, const RenderObject* obj) const
{
    showRenderObject();
    for (const RootInlineBox* root = firstRootBox(); root; root = root->nextRootBox())
        root->showLineTreeAndMark(markedBox1, markedLabel1, markedBox2, markedLabel2, obj, 1);
}

// These helpers are only used by the PODIntervalTree for debugging purposes.
String ValueToString<int>::string(const int value)
{
    return String::number(value);
}

String ValueToString<RenderBlock::FloatingObject*>::string(const RenderBlock::FloatingObject* floatingObject)
{
    return String::format("%p (%ix%i %ix%i)", floatingObject, floatingObject->frameRect().x().toInt(), floatingObject->frameRect().y().toInt(), floatingObject->frameRect().maxX().toInt(), floatingObject->frameRect().maxY().toInt());
}

#endif

} // namespace WebCore
