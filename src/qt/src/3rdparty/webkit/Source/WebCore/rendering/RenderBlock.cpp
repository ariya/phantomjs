/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "ColumnInfo.h"
#include "Document.h"
#include "Element.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "PaintInfo.h"
#include "RenderCombineText.h"
#include "RenderFlexibleBox.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderMarquee.h"
#include "RenderReplica.h"
#include "RenderTableCell.h"
#include "RenderTextFragment.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "SelectionController.h"
#include "Settings.h"
#include "TextRun.h"
#include "TransformState.h"
#include <wtf/StdLibExtras.h>

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

using namespace HTMLNames;

typedef WTF::HashMap<const RenderBox*, ColumnInfo*> ColumnInfoMap;
static ColumnInfoMap* gColumnInfoMap = 0;

typedef WTF::HashMap<const RenderBlock*, HashSet<RenderBox*>*> PercentHeightDescendantsMap;
static PercentHeightDescendantsMap* gPercentHeightDescendantsMap = 0;

typedef WTF::HashMap<const RenderBox*, HashSet<RenderBlock*>*> PercentHeightContainerMap;
static PercentHeightContainerMap* gPercentHeightContainerMap = 0;
    
typedef WTF::HashMap<RenderBlock*, ListHashSet<RenderInline*>*> ContinuationOutlineTableMap;

typedef WTF::HashSet<RenderBlock*> DelayedUpdateScrollInfoSet;
static int gDelayUpdateScrollInfo = 0;
static DelayedUpdateScrollInfoSet* gDelayedUpdateScrollInfoSet = 0;

bool RenderBlock::s_canPropagateFloatIntoSibling = false;

// Our MarginInfo state used when laying out block children.
RenderBlock::MarginInfo::MarginInfo(RenderBlock* block, int beforeBorderPadding, int afterBorderPadding)
    : m_atBeforeSideOfBlock(true)
    , m_atAfterSideOfBlock(false)
    , m_marginBeforeQuirk(false)
    , m_marginAfterQuirk(false)
    , m_determinedMarginBeforeQuirk(false)
{
    // Whether or not we can collapse our own margins with our children.  We don't do this
    // if we had any border/padding (obviously), if we're the root or HTML elements, or if
    // we're positioned, floating, a table cell.
    m_canCollapseWithChildren = !block->isRenderView() && !block->isRoot() && !block->isPositioned()
        && !block->isFloating() && !block->isTableCell() && !block->hasOverflowClip() && !block->isInlineBlockOrInlineTable()
        && !block->isWritingModeRoot();

    m_canCollapseMarginBeforeWithChildren = m_canCollapseWithChildren && (beforeBorderPadding == 0) && block->style()->marginBeforeCollapse() != MSEPARATE;

    // If any height other than auto is specified in CSS, then we don't collapse our bottom
    // margins with our children's margins.  To do otherwise would be to risk odd visual
    // effects when the children overflow out of the parent block and yet still collapse
    // with it.  We also don't collapse if we have any bottom border/padding.
    m_canCollapseMarginAfterWithChildren = m_canCollapseWithChildren && (afterBorderPadding == 0) &&
        (block->style()->logicalHeight().isAuto() && block->style()->logicalHeight().value() == 0) && block->style()->marginAfterCollapse() != MSEPARATE;
    
    m_quirkContainer = block->isTableCell() || block->isBody() || block->style()->marginBeforeCollapse() == MDISCARD || 
        block->style()->marginAfterCollapse() == MDISCARD;

    m_positiveMargin = m_canCollapseMarginBeforeWithChildren ? block->maxPositiveMarginBefore() : 0;
    m_negativeMargin = m_canCollapseMarginBeforeWithChildren ? block->maxNegativeMarginBefore() : 0;
}

// -------------------------------------------------------------------------------------------------------

RenderBlock::RenderBlock(Node* node)
      : RenderBox(node)
      , m_lineHeight(-1)
      , m_beingDestroyed(false)
{
    setChildrenInline(true);
}

RenderBlock::~RenderBlock()
{
    if (m_floatingObjects)
        deleteAllValues(m_floatingObjects->set());
    
    if (hasColumns())
        delete gColumnInfoMap->take(this);

    if (gPercentHeightDescendantsMap) {
        if (HashSet<RenderBox*>* descendantSet = gPercentHeightDescendantsMap->take(this)) {
            HashSet<RenderBox*>::iterator end = descendantSet->end();
            for (HashSet<RenderBox*>::iterator descendant = descendantSet->begin(); descendant != end; ++descendant) {
                HashSet<RenderBlock*>* containerSet = gPercentHeightContainerMap->get(*descendant);
                ASSERT(containerSet);
                if (!containerSet)
                    continue;
                ASSERT(containerSet->contains(this));
                containerSet->remove(this);
                if (containerSet->isEmpty()) {
                    gPercentHeightContainerMap->remove(*descendant);
                    delete containerSet;
                }
            }
            delete descendantSet;
        }
    }
}

void RenderBlock::destroy()
{
    // Mark as being destroyed to avoid trouble with merges in removeChild().
    m_beingDestroyed = true;

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
            // FIXME: The SelectionController should be responsible for this when it
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

    RenderBox::destroy();
}

void RenderBlock::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    s_canPropagateFloatIntoSibling = style() ? !isFloatingOrPositioned() && !avoidsFloats() : false;

    setReplaced(newStyle->isDisplayInlineType());
    
    if (style() && parent() && diff == StyleDifferenceLayout && style()->position() != newStyle->position()) {
        if (newStyle->position() == StaticPosition)
            // Clear our positioned objects list. Our absolutely positioned descendants will be
            // inserted into our containing block's positioned objects list during layout.
            removePositionedObjects(0);
        else if (style()->position() == StaticPosition) {
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
                toRenderBlock(cb)->removePositionedObjects(this);
        }

        if (containsFloats() && !isFloating() && !isPositioned() && (newStyle->position() == AbsolutePosition || newStyle->position() == FixedPosition))
            markAllDescendantsWithFloatsForLayout();
    }

    RenderBox::styleWillChange(diff, newStyle);
}

void RenderBlock::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);

    if (!isAnonymousBlock()) {
        // Ensure that all of our continuation blocks pick up the new style.
        for (RenderBlock* currCont = blockElementContinuation(); currCont; currCont = currCont->blockElementContinuation()) {
            RenderBoxModelObject* nextCont = currCont->continuation();
            currCont->setContinuation(0);
            currCont->setStyle(style());
            currCont->setContinuation(nextCont);
        }
    }

    propagateStyleToAnonymousChildren(true);    
    m_lineHeight = -1;

    // Update pseudos for :before and :after now.
    if (!isAnonymous() && document()->usesBeforeAfterRules() && canHaveChildren()) {
        updateBeforeAfterContent(BEFORE);
        updateBeforeAfterContent(AFTER);
    }

    // After our style changed, if we lose our ability to propagate floats into next sibling
    // blocks, then we need to find the top most parent containing that overhanging float and
    // then mark its descendants with floats for layout and clear all floats from its next
    // sibling blocks that exist in our floating objects list. See bug 56299 and 62875.
    bool canPropagateFloatIntoSibling = !isFloatingOrPositioned() && !avoidsFloats();
    if (diff == StyleDifferenceLayout && s_canPropagateFloatIntoSibling && !canPropagateFloatIntoSibling && hasOverhangingFloats()) {
        RenderBlock* parentBlock = this;
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
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
}

void RenderBlock::updateBeforeAfterContent(PseudoId pseudoId)
{
    // If this is an anonymous wrapper, then the parent applies its own pseudo-element style to it.
    if (parent() && parent()->createsAnonymousWrapper())
        return;
    return children()->updateBeforeAfterContent(this, pseudoId);
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

    if (newChild->isFloatingOrPositioned())
        return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);

    // A continuation always consists of two potential candidates: a block or an anonymous
    // column span box holding column span children.
    bool childIsNormal = newChild->isInline() || !newChild->style()->columnSpan();
    bool bcpIsNormal = beforeChildParent->isInline() || !beforeChildParent->style()->columnSpan();
    bool flowIsNormal = flow->isInline() || !flow->style()->columnSpan();

    if (flow == beforeChildParent)
        return flow->addChildIgnoringContinuation(newChild, beforeChild);
    
    // The goal here is to match up if we can, so that we can coalesce and create the
    // minimal # of continuations needed for the inline.
    if (childIsNormal == bcpIsNormal)
        return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
    if (flowIsNormal == childIsNormal)
        return flow->addChildIgnoringContinuation(newChild, 0); // Just treat like an append.
    return beforeChildParent->addChildIgnoringContinuation(newChild, beforeChild);
}


void RenderBlock::addChildToAnonymousColumnBlocks(RenderObject* newChild, RenderObject* beforeChild)
{
    ASSERT(!continuation()); // We don't yet support column spans that aren't immediate children of the multi-column block.
        
    // The goal is to locate a suitable box in which to place our child.
    RenderBlock* beforeChildParent = toRenderBlock(beforeChild && beforeChild->parent()->isRenderBlock() ? beforeChild->parent() : lastChild());
    
    // If the new child is floating or positioned it can just go in that block.
    if (newChild->isFloatingOrPositioned())
        return beforeChildParent->addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);

    // See if the child can be placed in the box.
    bool newChildHasColumnSpan = newChild->style()->columnSpan() && !newChild->isInline();
    bool beforeChildParentHoldsColumnSpans = beforeChildParent->isAnonymousColumnSpanBlock();

    if (newChildHasColumnSpan == beforeChildParentHoldsColumnSpans)
        return beforeChildParent->addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);

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
    if (isPreviousBlockViable && immediateChild->previousSibling())
        return toRenderBlock(immediateChild->previousSibling())->addChildIgnoringAnonymousColumnBlocks(newChild, 0); // Treat like an append.
        
    // Split our anonymous blocks.
    RenderObject* newBeforeChild = splitAnonymousBlocksAroundChild(beforeChild);
    
    // Create a new anonymous box of the appropriate type.
    RenderBlock* newBox = newChildHasColumnSpan ? createAnonymousColumnSpanBlock() : createAnonymousColumnsBlock();
    children()->insertChildNode(this, newBox, newBeforeChild);
    newBox->addChildIgnoringAnonymousColumnBlocks(newChild, 0);
    return;
}

RenderBlock* RenderBlock::containingColumnsBlock(bool allowAnonymousColumnBlock)
{
    for (RenderObject* curr = this; curr; curr = curr->parent()) {
        if (!curr->isRenderBlock() || curr->isFloatingOrPositioned() || curr->isTableCell() || curr->isRoot() || curr->isRenderView() || curr->hasOverflowClip()
            || curr->isInlineBlockOrInlineTable())
            return 0;
        
        RenderBlock* currBlock = toRenderBlock(curr);
        if (currBlock->style()->specifiesColumns() && (allowAnonymousColumnBlock || !currBlock->isAnonymousColumnsBlock()))
            return currBlock;
            
        if (currBlock->isAnonymousColumnSpanBlock())
            return 0;
    }
    return 0;
}

RenderBlock* RenderBlock::clone() const
{
    RenderBlock* cloneBlock;
    if (isAnonymousBlock())
        cloneBlock = createAnonymousBlock();
    else {
        cloneBlock = new (renderArena()) RenderBlock(node());
        cloneBlock->setStyle(style());
    }
    cloneBlock->setChildrenInline(childrenInline());
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

    // Now take all of the children from beforeChild to the end and remove
    // them from |this| and place them in the clone.
    if (!beforeChild && isAfterContent(lastChild()))
        beforeChild = lastChild();
    moveChildrenTo(cloneBlock, beforeChild, 0);
    
    // Hook |clone| up as the continuation of the middle block.
    if (!cloneBlock->isAnonymousBlock())
        middleBlock->setContinuation(cloneBlock);

    // We have been reparented and are now under the fromBlock.  We need
    // to walk up our block parent chain until we hit the containing anonymous columns block.
    // Once we hit the anonymous columns block we're done.
    RenderBoxModelObject* curr = toRenderBoxModelObject(parent());
    RenderBoxModelObject* currChild = this;
    
    while (curr && curr != fromBlock) {
        ASSERT(curr->isRenderBlock());
        
        RenderBlock* blockCurr = toRenderBlock(curr);
        
        // Create a new clone.
        RenderBlock* cloneChild = cloneBlock;
        cloneBlock = blockCurr->clone();

        // Insert our child clone as the first child.
        cloneBlock->children()->appendChildNode(cloneBlock, cloneChild);

        // Hook the clone up as a continuation of |curr|.  Note we do encounter
        // anonymous blocks possibly as we walk up the block chain.  When we split an
        // anonymous block, there's no need to do any continuation hookup, since we haven't
        // actually split a real element.
        if (!blockCurr->isAnonymousBlock()) {
            oldCont = blockCurr->continuation();
            blockCurr->setContinuation(cloneBlock);
            cloneBlock->setContinuation(oldCont);
        }

        // Someone may have indirectly caused a <q> to split.  When this happens, the :after content
        // has to move into the inline continuation.  Call updateBeforeAfterContent to ensure that the inline's :after
        // content gets properly destroyed.
        if (document()->usesBeforeAfterRules())
            blockCurr->children()->updateBeforeAfterContent(blockCurr, AFTER);

        // Now we need to take all of the children starting from the first child
        // *after* currChild and append them all to the clone.
        RenderObject* afterContent = isAfterContent(cloneBlock->lastChild()) ? cloneBlock->lastChild() : 0;
        blockCurr->moveChildrenTo(cloneBlock, currChild->nextSibling(), 0, afterContent);

        // Keep walking up the chain.
        currChild = curr;
        curr = toRenderBoxModelObject(curr->parent());
    }

    // Now we are at the columns block level. We need to put the clone into the toBlock.
    toBlock->children()->appendChildNode(toBlock, cloneBlock);

    // Now take all the children after currChild and remove them from the fromBlock
    // and put them in the toBlock.
    fromBlock->moveChildrenTo(toBlock, currChild->nextSibling(), 0);
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
        block->moveChildrenTo(pre, boxFirst, 0);

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

RenderObject* RenderBlock::splitAnonymousBlocksAroundChild(RenderObject* beforeChild)
{
    while (beforeChild->parent() != this) {
        RenderBlock* blockToSplit = toRenderBlock(beforeChild->parent());
        if (blockToSplit->firstChild() != beforeChild) {
            // We have to split the parentBlock into two blocks.
            RenderBlock* post = createAnonymousBlockWithSameTypeAs(blockToSplit);
            post->setChildrenInline(blockToSplit->childrenInline());
            RenderBlock* parentBlock = toRenderBlock(blockToSplit->parent());
            parentBlock->children()->insertChildNode(parentBlock, post, blockToSplit->nextSibling());
            blockToSplit->moveChildrenTo(post, beforeChild, 0, blockToSplit->hasLayer());
            post->setNeedsLayoutAndPrefWidthsRecalc();
            blockToSplit->setNeedsLayoutAndPrefWidthsRecalc();
            beforeChild = post;
        } else
            beforeChild = blockToSplit;
    }
    return beforeChild;
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
        beforeChild = splitAnonymousBlocksAroundChild(beforeChild);

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
    if (!newChild->isText() && newChild->style()->columnSpan() && !newChild->isFloatingOrPositioned()
        && !newChild->isInline() && !isAnonymousColumnSpanBlock()) {
        if (style()->specifiesColumns())
            columnsBlockAncestor = this;
        else if (!isInline() && parent() && parent()->isRenderBlock()) {
            columnsBlockAncestor = toRenderBlock(parent())->containingColumnsBlock(false);
            
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
    }
    return columnsBlockAncestor;
}

void RenderBlock::addChildIgnoringAnonymousColumnBlocks(RenderObject* newChild, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if (!beforeChild) {
        RenderObject* lastRenderer = lastChild();
        while (lastRenderer && lastRenderer->isAnonymous() && !isAfterContent(lastRenderer))
            lastRenderer = lastRenderer->lastChild();
        if (lastRenderer && isAfterContent(lastRenderer))
            beforeChild = lastRenderer;
    }

    // If the requested beforeChild is not one of our children, then this is because
    // there is an anonymous container within this object that contains the beforeChild.
    if (beforeChild && beforeChild->parent() != this) {
        RenderObject* anonymousChild = beforeChild->parent();
        ASSERT(anonymousChild);

        while (anonymousChild->parent() != this)
            anonymousChild = anonymousChild->parent();

        ASSERT(anonymousChild->isAnonymous());

        if (anonymousChild->isAnonymousBlock()) {
            // Insert the child into the anonymous block box instead of here.
            if (newChild->isInline() || beforeChild->parent()->firstChild() != beforeChild)
                beforeChild->parent()->addChild(newChild, beforeChild);
            else
                addChild(newChild, beforeChild->parent());
            return;
        }

        ASSERT(anonymousChild->isTable());
        if ((newChild->isTableCol() && newChild->style()->display() == TABLE_COLUMN_GROUP)
                || (newChild->isRenderBlock() && newChild->style()->display() == TABLE_CAPTION)
                || newChild->isTableSection()
                || newChild->isTableRow()
                || newChild->isTableCell()) {
            // Insert into the anonymous table.
            anonymousChild->addChild(newChild, beforeChild);
            return;
        }

        // Go on to insert before the anonymous table.
        beforeChild = anonymousChild;
    }

    // Check for a spanning element in columns.
    RenderBlock* columnsBlockAncestor = columnsBlockForSpanningElement(newChild);
    if (columnsBlockAncestor) {
        // We are placing a column-span element inside a block. 
        RenderBlock* newBox = createAnonymousColumnSpanBlock();
        
        if (columnsBlockAncestor != this) {
            // We are nested inside a multi-column element and are being split by the span.  We have to break up
            // our block into continuations.
            RenderBoxModelObject* oldContinuation = continuation();
            setContinuation(newBox);

            // Someone may have put a <p> inside a <q>, causing a split.  When this happens, the :after content
            // has to move into the inline continuation.  Call updateBeforeAfterContent to ensure that our :after
            // content gets properly destroyed.
            bool isLastChild = (beforeChild == lastChild());
            if (document()->usesBeforeAfterRules())
                children()->updateBeforeAfterContent(this, AFTER);
            if (isLastChild && beforeChild != lastChild())
                beforeChild = 0; // We destroyed the last child, so now we need to update our insertion
                                 // point to be 0.  It's just a straight append now.

            splitFlow(beforeChild, newBox, newChild, oldContinuation);
            return;
        }

        // We have to perform a split of this block's children.  This involves creating an anonymous block box to hold
        // the column-spanning |newChild|.  We take all of the children from before |newChild| and put them into
        // one anonymous columns block, and all of the children after |newChild| go into another anonymous block.
        makeChildrenAnonymousColumnBlocks(beforeChild, newBox, newChild);
        return;
    }

    bool madeBoxesNonInline = false;

    // A block has to either have all of its children inline, or all of its children as blocks.
    // So, if our children are currently inline and a block child has to be inserted, we move all our
    // inline children into anonymous block boxes.
    if (childrenInline() && !newChild->isInline() && !newChild->isFloatingOrPositioned()) {
        // This is a block with inline content. Wrap the inline content in anonymous blocks.
        makeChildrenNonInline(beforeChild);
        madeBoxesNonInline = true;

        if (beforeChild && beforeChild->parent() != this) {
            beforeChild = beforeChild->parent();
            ASSERT(beforeChild->isAnonymousBlock());
            ASSERT(beforeChild->parent() == this);
        }
    } else if (!childrenInline() && (newChild->isFloatingOrPositioned() || newChild->isInline())) {
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

    if (madeBoxesNonInline && parent() && isAnonymousBlock() && parent()->isRenderBlock())
        toRenderBlock(parent())->removeLeftoverAnonymousBlock(this);
    // this object may be dead here
}

void RenderBlock::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (continuation() && !isAnonymousBlock())
        return addChildToContinuation(newChild, beforeChild);
    return addChildIgnoringContinuation(newChild, beforeChild);
}

void RenderBlock::addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    if (!isAnonymousBlock() && firstChild() && (firstChild()->isAnonymousColumnsBlock() || firstChild()->isAnonymousColumnSpanBlock()))
        return addChildToAnonymousColumnBlocks(newChild, beforeChild);
    return addChildIgnoringAnonymousColumnBlocks(newChild, beforeChild);
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
        while (curr && !(curr->isInline() || curr->isFloatingOrPositioned()))
            curr = curr->nextSibling();
        
        inlineRunStart = inlineRunEnd = curr;
        
        if (!curr)
            return; // No more inline children to be found.
        
        sawInline = curr->isInline();
        
        curr = curr->nextSibling();
        while (curr && (curr->isInline() || curr->isFloatingOrPositioned()) && (curr != boundary)) {
            inlineRunEnd = curr;
            if (curr->isInline())
                sawInline = true;
            curr = curr->nextSibling();
        }
    } while (!sawInline);
}

void RenderBlock::deleteLineBoxTree()
{
    m_lineBoxes.deleteLineBoxTree(renderArena());
}

RootInlineBox* RenderBlock::createRootInlineBox() 
{
    return new (renderArena()) RootInlineBox(this);
}

RootInlineBox* RenderBlock::createAndAppendRootInlineBox()
{
    RootInlineBox* rootBox = createRootInlineBox();
    m_lineBoxes.appendLineBox(rootBox);
    return rootBox;
}

void RenderBlock::moveChildTo(RenderBlock* to, RenderObject* child, RenderObject* beforeChild, bool fullRemoveInsert)
{
    ASSERT(this == child->parent());
    ASSERT(!beforeChild || to == beforeChild->parent());
    to->children()->insertChildNode(to, children()->removeChildNode(this, child, fullRemoveInsert), beforeChild, fullRemoveInsert);
}

void RenderBlock::moveChildrenTo(RenderBlock* to, RenderObject* startChild, RenderObject* endChild, RenderObject* beforeChild, bool fullRemoveInsert)
{
    ASSERT(!beforeChild || to == beforeChild->parent());
    RenderObject* nextChild = startChild;
    while (nextChild && nextChild != endChild) {
        RenderObject* child = nextChild;
        nextChild = child->nextSibling();
        to->children()->insertChildNode(to, children()->removeChildNode(this, child, fullRemoveInsert), beforeChild, fullRemoveInsert);
        if (child == endChild)
            return;
    }
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
    child->setParent(0);
    child->setPreviousSibling(0);
    child->setNextSibling(0);
    
    child->children()->setFirstChild(0);
    child->m_next = 0;

    child->destroy();
}

static bool canMergeContiguousAnonymousBlocks(RenderObject* oldChild, RenderObject* prev, RenderObject* next)
{
    if (oldChild->documentBeingDestroyed() || oldChild->isInline() || oldChild->virtualContinuation())
        return false;

#if ENABLE(DETAILS)
    if (oldChild->parent() && oldChild->parent()->isDetails())
        return false;
#endif

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

void RenderBlock::removeChild(RenderObject* oldChild)
{
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
            RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());
            children()->removeChildNode(this, inlineChildrenBlock, inlineChildrenBlock->hasLayer());
            inlineChildrenBlock->setStyle(newStyle);
            
            // Now just put the inlineChildrenBlock inside the blockChildrenBlock.
            blockChildrenBlock->children()->insertChildNode(blockChildrenBlock, inlineChildrenBlock, prev == inlineChildrenBlock ? blockChildrenBlock->firstChild() : 0,
                                                            inlineChildrenBlock->hasLayer() || blockChildrenBlock->hasLayer());
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
            nextBlock->moveAllChildrenTo(prevBlock, nextBlock->hasLayer() || prevBlock->hasLayer());        
            
            // Delete the now-empty block's lines and nuke it.
            nextBlock->deleteLineBoxTree();
            nextBlock->destroy();
            next = 0;
        }
    }

    RenderBox::removeChild(oldChild);

    RenderObject* child = prev ? prev : next;
    if (canMergeAnonymousBlocks && child && !child->previousSibling() && !child->nextSibling() && !isFlexibleBox()) {
        // The removal has knocked us down to containing only a single anonymous
        // box.  We can go ahead and pull the content right back up into our
        // box.
        setNeedsLayoutAndPrefWidthsRecalc();
        setChildrenInline(child->childrenInline());
        RenderBlock* anonBlock = toRenderBlock(children()->removeChildNode(this, child, child->hasLayer()));
        anonBlock->moveAllChildrenTo(this, child->hasLayer());
        // Delete the now-empty block's lines and nuke it.
        anonBlock->deleteLineBoxTree();
        anonBlock->destroy();
    }

    if (!firstChild() && !documentBeingDestroyed()) {
        // If this was our last child be sure to clear out our line boxes.
        if (childrenInline())
            lineBoxes()->deleteLineBoxes(renderArena());
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
            if (child->isFloatingOrPositioned())
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
            }
        }
    }
}

void RenderBlock::updateScrollInfoAfterLayout()
{
    if (hasOverflowClip()) {
        if (gDelayUpdateScrollInfo)
            gDelayedUpdateScrollInfoSet->add(this);
        else
            layer()->updateScrollInfoAfterLayout();
    }
}

void RenderBlock::layout()
{
    // Update our first letter info now.
    updateFirstLetter();

    // Table cells call layoutBlock directly, so don't add any logic here.  Put code into
    // layoutBlock().
    layoutBlock(false);
    
    // It's safe to check for control clip here, since controls can never be table cells.
    // If we have a lightweight clip, there can never be any overflow from children.
    if (hasControlClip() && m_overflow)
        clearLayoutOverflow();
}

void RenderBlock::layoutBlock(bool relayoutChildren, int pageLogicalHeight)
{
    ASSERT(needsLayout());

    if (isInline() && !isInlineBlockOrInlineTable()) // Inline <form>s inside various table elements can
        return;                                      // cause us to come in here.  Just bail.

    if (!relayoutChildren && simplifiedLayout())
        return;

    LayoutRepainter repainter(*this, m_everHadLayout && checkForRepaintDuringLayout());

    int oldWidth = logicalWidth();
    int oldColumnWidth = desiredColumnWidth();

    computeLogicalWidth();
    calcColumnWidth();

    m_overflow.clear();

    if (oldWidth != logicalWidth() || oldColumnWidth != desiredColumnWidth())
        relayoutChildren = true;

    clearFloats();

    int previousHeight = logicalHeight();
    setLogicalHeight(0);
    bool hasSpecifiedPageLogicalHeight = false;
    bool pageLogicalHeightChanged = false;
    ColumnInfo* colInfo = columnInfo();
    if (hasColumns()) {
        if (!pageLogicalHeight) {
            // We need to go ahead and set our explicit page height if one exists, so that we can
            // avoid doing two layout passes.
            computeLogicalHeight();
            int columnHeight = contentLogicalHeight();
            if (columnHeight > 0) {
                pageLogicalHeight = columnHeight;
                hasSpecifiedPageLogicalHeight = true;
            }
            setLogicalHeight(0);
        }
        if (colInfo->columnHeight() != pageLogicalHeight && m_everHadLayout) {
            colInfo->setColumnHeight(pageLogicalHeight);
            pageLogicalHeightChanged = true;
        }
        
        if (!hasSpecifiedPageLogicalHeight && !pageLogicalHeight)
            colInfo->clearForcedBreaks();
    }

    LayoutStateMaintainer statePusher(view(), this, IntSize(x(), y()), hasColumns() || hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode(), pageLogicalHeight, pageLogicalHeightChanged, colInfo);

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
        
        setMarginBeforeQuirk(style()->marginBefore().quirk());
        setMarginAfterQuirk(style()->marginAfter().quirk());

        Node* n = node();
        if (n && n->hasTagName(formTag) && static_cast<HTMLFormElement*>(n)->isMalformed()) {
            // See if this form is malformed (i.e., unclosed). If so, don't give the form
            // a bottom margin.
            setMaxMarginAfterValues(0, 0);
        }
        
        setPaginationStrut(0);
    }

    // For overflow:scroll blocks, ensure we have both scrollbars in place always.
    if (scrollsOverflow()) {
        if (style()->overflowX() == OSCROLL)
            layer()->setHasHorizontalScrollbar(true);
        if (style()->overflowY() == OSCROLL)
            layer()->setHasVerticalScrollbar(true);
    }

    int repaintLogicalTop = 0;
    int repaintLogicalBottom = 0;
    int maxFloatLogicalBottom = 0;
    if (!firstChild() && !isAnonymousBlock())
        setChildrenInline(true);
    if (childrenInline())
        layoutInlineChildren(relayoutChildren, repaintLogicalTop, repaintLogicalBottom);
    else
        layoutBlockChildren(relayoutChildren, maxFloatLogicalBottom);

    // Expand our intrinsic height to encompass floats.
    int toAdd = borderAfter() + paddingAfter() + scrollbarLogicalHeight();
    if (lowestFloatLogicalBottom() > (logicalHeight() - toAdd) && expandsToEncloseOverhangingFloats())
        setLogicalHeight(lowestFloatLogicalBottom() + toAdd);
    
    if (layoutColumns(hasSpecifiedPageLogicalHeight, pageLogicalHeight, statePusher))
        return;
 
    // Calculate our new height.
    int oldHeight = logicalHeight();
    int oldClientAfterEdge = clientLogicalBottom();
    computeLogicalHeight();
    int newHeight = logicalHeight();
    if (oldHeight != newHeight) {
        if (oldHeight > newHeight && maxFloatLogicalBottom > newHeight && !childrenInline()) {
            // One of our children's floats may have become an overhanging float for us. We need to look for it.
            for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
                if (child->isBlockFlow() && !child->isFloatingOrPositioned()) {
                    RenderBlock* block = toRenderBlock(child);
                    if (block->lowestFloatLogicalBottom() + block->logicalTop() > newHeight)
                        addOverhangingFloats(block, -block->logicalLeft(), -block->logicalTop(), false);
                }
            }
        }
    }

    if (previousHeight != newHeight)
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    // Add overflow from children (unless we're multi-column, since in that case all our child overflow is clipped anyway).
    computeOverflow(oldClientAfterEdge);
    
    statePusher.pop();

    if (view()->layoutState()->m_pageLogicalHeight)
        setPageLogicalOffset(view()->layoutState()->pageLogicalOffset(logicalTop()));

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    // Repaint with our new bounds if they are different from our old bounds.
    bool didFullRepaint = repainter.repaintAfterLayout();
    if (!didFullRepaint && repaintLogicalTop != repaintLogicalBottom && (style()->visibility() == VISIBLE || enclosingLayer()->hasVisibleContent())) {
        // FIXME: We could tighten up the left and right invalidation points if we let layoutInlineChildren fill them in based off the particular lines
        // it had to lay out.  We wouldn't need the hasOverflowClip() hack in that case either.
        int repaintLogicalLeft = logicalLeftVisualOverflow();
        int repaintLogicalRight = logicalRightVisualOverflow();
        if (hasOverflowClip()) {
            // If we have clipped overflow, we should use layout overflow as well, since visual overflow from lines didn't propagate to our block's overflow.
            // Note the old code did this as well but even for overflow:visible.  The addition of hasOverflowClip() at least tightens up the hack a bit.
            // layoutInlineChildren should be patched to compute the entire repaint rect.
            repaintLogicalLeft = min(repaintLogicalLeft, logicalLeftLayoutOverflow());
            repaintLogicalRight = max(repaintLogicalRight, logicalRightLayoutOverflow());
        }
        
        IntRect repaintRect;
        if (isHorizontalWritingMode())
            repaintRect = IntRect(repaintLogicalLeft, repaintLogicalTop, repaintLogicalRight - repaintLogicalLeft, repaintLogicalBottom - repaintLogicalTop);
        else
            repaintRect = IntRect(repaintLogicalTop, repaintLogicalLeft, repaintLogicalBottom - repaintLogicalTop, repaintLogicalRight - repaintLogicalLeft);

        // The repaint rect may be split across columns, in which case adjustRectForColumns() will return the union.
        adjustRectForColumns(repaintRect);

        repaintRect.inflate(maximalOutlineSize(PaintPhaseOutline));
        
        if (hasOverflowClip()) {
            // Adjust repaint rect for scroll offset
            repaintRect.move(-layer()->scrolledContentOffset());

            // Don't allow this rect to spill out of our overflow box.
            repaintRect.intersect(IntRect(0, 0, width(), height()));
        }

        // Make sure the rect is still non-empty after intersecting for overflow above
        if (!repaintRect.isEmpty()) {
            repaintRectangle(repaintRect); // We need to do a partial repaint of our content.
            if (hasReflection())
                repaintRectangle(reflectedRect(repaintRect));
        }
    }
    setNeedsLayout(false);
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
            IntRect lastRect = columnRectAt(colInfo, columnCount(colInfo) - 1);
            if (isHorizontalWritingMode()) {
                int overflowLeft = !style()->isLeftToRightDirection() ? min(0, lastRect.x()) : 0;
                int overflowRight = style()->isLeftToRightDirection() ? max(width(), lastRect.maxX()) : 0;
                int overflowHeight = borderBefore() + paddingBefore() + colInfo->columnHeight();
                addLayoutOverflow(IntRect(overflowLeft, 0, overflowRight - overflowLeft, overflowHeight));
                if (!hasOverflowClip())
                    addVisualOverflow(IntRect(overflowLeft, 0, overflowRight - overflowLeft, overflowHeight));
            } else {
                IntRect lastRect = columnRectAt(colInfo, columnCount(colInfo) - 1);
                int overflowTop = !style()->isLeftToRightDirection() ? min(0, lastRect.y()) : 0;
                int overflowBottom = style()->isLeftToRightDirection() ? max(height(), lastRect.maxY()) : 0;
                int overflowWidth = borderBefore() + paddingBefore() + colInfo->columnHeight();
                addLayoutOverflow(IntRect(0, overflowTop, overflowWidth, overflowBottom - overflowTop));
                if (!hasOverflowClip())
                    addVisualOverflow(IntRect(0, overflowTop, overflowWidth, overflowBottom - overflowTop));
            }
        }
    }
}

void RenderBlock::computeOverflow(int oldClientAfterEdge, bool recomputeFloats)
{
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
        IntRect clientRect(clientBoxRect());
        IntRect rectToApply;
        if (isHorizontalWritingMode())
            rectToApply = IntRect(clientRect.x(), clientRect.y(), 1, max(0, oldClientAfterEdge - clientRect.y()));
        else
            rectToApply = IntRect(clientRect.x(), clientRect.y(), max(0, oldClientAfterEdge - clientRect.x()), 1);
        addLayoutOverflow(rectToApply);
    }
        
    // Add visual overflow from box-shadow and reflections.
    addShadowOverflow();
}

void RenderBlock::addOverflowFromBlockChildren()
{
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (!child->isFloatingOrPositioned())
            addOverflowFromChild(child);
    }
}

void RenderBlock::addOverflowFromFloats()
{
    if (!m_floatingObjects)
        return;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        if (r->m_isDescendant)
            addOverflowFromChild(r->m_renderer, IntSize(xPositionForFloatIncludingMargin(r), yPositionForFloatIncludingMargin(r)));
    }
    return;
}

void RenderBlock::addOverflowFromPositionedObjects()
{
    if (!m_positionedObjects)
        return;

    RenderBox* positionedObject;
    Iterator end = m_positionedObjects->end();
    for (Iterator it = m_positionedObjects->begin(); it != end; ++it) {
        positionedObject = *it;
        
        // Fixed positioned elements don't contribute to layout overflow, since they don't scroll with the content.
        if (positionedObject->style()->position() != FixedPosition)
            addOverflowFromChild(positionedObject);
    }
}

bool RenderBlock::expandsToEncloseOverhangingFloats() const
{
    return isInlineBlockOrInlineTable() || isFloatingOrPositioned() || hasOverflowClip() || (parent() && parent()->isFlexibleBox())
           || hasColumns() || isTableCell() || isFieldset() || isWritingModeRoot() || isRoot();
}

void RenderBlock::adjustPositionedBlock(RenderBox* child, const MarginInfo& marginInfo)
{
    bool isHorizontal = isHorizontalWritingMode();
    bool hasStaticBlockPosition = child->style()->hasStaticBlockPosition(isHorizontal);
    RenderLayer* childLayer = child->layer();
        
    childLayer->setStaticInlinePosition(borderAndPaddingStart());

    int logicalTop = logicalHeight();
    if (!marginInfo.canCollapseWithMarginBefore()) {
        child->computeBlockDirectionMargins(this);
        int marginBefore = marginBeforeForChild(child);
        int collapsedBeforePos = marginInfo.positiveMargin();
        int collapsedBeforeNeg = marginInfo.negativeMargin();
        if (marginBefore > 0) {
            if (marginBefore > collapsedBeforePos)
                collapsedBeforePos = marginBefore;
        } else {
            if (-marginBefore > collapsedBeforeNeg)
                collapsedBeforeNeg = -marginBefore;
        }
        logicalTop += (collapsedBeforePos - collapsedBeforeNeg) - marginBefore;
    }
    if (childLayer->staticBlockPosition() != logicalTop) {
        childLayer->setStaticBlockPosition(logicalTop);
        if (hasStaticBlockPosition)
            child->setChildNeedsLayout(true, false);
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
    int marginOffset = marginInfo.canCollapseWithMarginBefore() ? 0 : marginInfo.margin();
    setLogicalHeight(logicalHeight() + marginOffset);
    positionNewFloats();
    setLogicalHeight(logicalHeight() - marginOffset);
}

bool RenderBlock::handleSpecialChild(RenderBox* child, const MarginInfo& marginInfo)
{
    // Handle in the given order
    return handlePositionedChild(child, marginInfo)
        || handleFloatingChild(child, marginInfo)
        || handleRunInChild(child);
}


bool RenderBlock::handlePositionedChild(RenderBox* child, const MarginInfo& marginInfo)
{
    if (child->isPositioned()) {
        child->containingBlock()->insertPositionedObject(child);
        adjustPositionedBlock(child, marginInfo);
        return true;
    }
    return false;
}

bool RenderBlock::handleFloatingChild(RenderBox* child, const MarginInfo& marginInfo)
{
    if (child->isFloating()) {
        insertFloatingObject(child);
        adjustFloatingBlock(marginInfo);
        return true;
    }
    return false;
}

bool RenderBlock::handleRunInChild(RenderBox* child)
{
    // See if we have a run-in element with inline children.  If the
    // children aren't inline, then just treat the run-in as a normal
    // block.
    if (!child->isRunIn() || !child->childrenInline())
        return false;
    // FIXME: We don't handle non-block elements with run-in for now.
    if (!child->isRenderBlock())
        return false;

    // Get the next non-positioned/non-floating RenderBlock.
    RenderBlock* blockRunIn = toRenderBlock(child);
    RenderObject* curr = blockRunIn->nextSibling();
    while (curr && curr->isFloatingOrPositioned())
        curr = curr->nextSibling();

    if (!curr || !curr->isRenderBlock() || !curr->childrenInline() || curr->isRunIn() || curr->isAnonymous())
        return false;

    RenderBlock* currBlock = toRenderBlock(curr);

    // Remove the old child.
    children()->removeChildNode(this, blockRunIn);

    // Create an inline.
    Node* runInNode = blockRunIn->node();
    RenderInline* inlineRunIn = new (renderArena()) RenderInline(runInNode ? runInNode : document());
    inlineRunIn->setStyle(blockRunIn->style());

    bool runInIsGenerated = child->style()->styleType() == BEFORE || child->style()->styleType() == AFTER;

    // Move the nodes from the old child to the new child, but skip any :before/:after content.  It has already
    // been regenerated by the new inline.
    for (RenderObject* runInChild = blockRunIn->firstChild(); runInChild;) {
        RenderObject* nextSibling = runInChild->nextSibling();
        if (runInIsGenerated || (runInChild->style()->styleType() != BEFORE && runInChild->style()->styleType() != AFTER)) {
            blockRunIn->children()->removeChildNode(blockRunIn, runInChild, false);
            inlineRunIn->addChild(runInChild); // Use addChild instead of appendChildNode since it handles correct placement of the children relative to :after-generated content.
        }
        runInChild = nextSibling;
    }

    // Now insert the new child under |currBlock|. Use addChild instead of insertChildNode since it handles correct placement of the children, esp where we cannot insert
    // anything before the first child. e.g. details tag. See https://bugs.webkit.org/show_bug.cgi?id=58228.
    currBlock->addChild(inlineRunIn, currBlock->firstChild());
    
    // If the run-in had an element, we need to set the new renderer.
    if (runInNode)
        runInNode->setRenderer(inlineRunIn);

    // Destroy the block run-in, which includes deleting its line box tree.
    blockRunIn->deleteLineBoxTree();
    blockRunIn->destroy();

    // The block acts like an inline, so just null out its
    // position.
    
    return true;
}

int RenderBlock::collapseMargins(RenderBox* child, MarginInfo& marginInfo)
{
    // Get the four margin values for the child and cache them.
    const MarginValues childMargins = marginValuesForChild(child);

    // Get our max pos and neg top margins.
    int posTop = childMargins.positiveMarginBefore();
    int negTop = childMargins.negativeMarginBefore();

    // For self-collapsing blocks, collapse our bottom margins into our
    // top to get new posTop and negTop values.
    if (child->isSelfCollapsingBlock()) {
        posTop = max(posTop, childMargins.positiveMarginAfter());
        negTop = max(negTop, childMargins.negativeMarginAfter());
    }
    
    // See if the top margin is quirky. We only care if this child has
    // margins that will collapse with us.
    bool topQuirk = child->isMarginBeforeQuirk() || style()->marginBeforeCollapse() == MDISCARD;

    if (marginInfo.canCollapseWithMarginBefore()) {
        // This child is collapsing with the top of the
        // block.  If it has larger margin values, then we need to update
        // our own maximal values.
        if (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !topQuirk)
            setMaxMarginBeforeValues(max(posTop, maxPositiveMarginBefore()), max(negTop, maxNegativeMarginBefore()));

        // The minute any of the margins involved isn't a quirk, don't
        // collapse it away, even if the margin is smaller (www.webreference.com
        // has an example of this, a <dt> with 0.8em author-specified inside
        // a <dl> inside a <td>.
        if (!marginInfo.determinedMarginBeforeQuirk() && !topQuirk && (posTop - negTop)) {
            setMarginBeforeQuirk(false);
            marginInfo.setDeterminedMarginBeforeQuirk(true);
        }

        if (!marginInfo.determinedMarginBeforeQuirk() && topQuirk && !marginBefore())
            // We have no top margin and our top child has a quirky margin.
            // We will pick up this quirky margin and pass it through.
            // This deals with the <td><div><p> case.
            // Don't do this for a block that split two inlines though.  You do
            // still apply margins in this case.
            setMarginBeforeQuirk(true);
    }

    if (marginInfo.quirkContainer() && marginInfo.atBeforeSideOfBlock() && (posTop - negTop))
        marginInfo.setMarginBeforeQuirk(topQuirk);

    int beforeCollapseLogicalTop = logicalHeight();
    int logicalTop = beforeCollapseLogicalTop;
    if (child->isSelfCollapsingBlock()) {
        // This child has no height.  We need to compute our
        // position before we collapse the child's margins together,
        // so that we can get an accurate position for the zero-height block.
        int collapsedBeforePos = max(marginInfo.positiveMargin(), childMargins.positiveMarginBefore());
        int collapsedBeforeNeg = max(marginInfo.negativeMargin(), childMargins.negativeMarginBefore());
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
    else {
        if (child->style()->marginBeforeCollapse() == MSEPARATE) {
            setLogicalHeight(logicalHeight() + marginInfo.margin() + marginBeforeForChild(child));
            logicalTop = logicalHeight();
        }
        else if (!marginInfo.atBeforeSideOfBlock() ||
            (!marginInfo.canCollapseMarginBeforeWithChildren()
             && (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !marginInfo.marginBeforeQuirk()))) {
            // We're collapsing with a previous sibling's margins and not
            // with the top of the block.
            setLogicalHeight(logicalHeight() + max(marginInfo.positiveMargin(), posTop) - max(marginInfo.negativeMargin(), negTop));
            logicalTop = logicalHeight();
        }

        marginInfo.setPositiveMargin(childMargins.positiveMarginAfter());
        marginInfo.setNegativeMargin(childMargins.negativeMarginAfter());

        if (marginInfo.margin())
            marginInfo.setMarginAfterQuirk(child->isMarginAfterQuirk() || style()->marginAfterCollapse() == MDISCARD);
    }
    
    // If margins would pull us past the top of the next page, then we need to pull back and pretend like the margins
    // collapsed into the page edge.
    bool paginated = view()->layoutState()->isPaginated();
    if (paginated && logicalTop > beforeCollapseLogicalTop) {
        int oldLogicalTop = logicalTop;
        logicalTop = min(logicalTop, nextPageLogicalTop(beforeCollapseLogicalTop));
        setLogicalHeight(logicalHeight() + (logicalTop - oldLogicalTop));
    }
    return logicalTop;
}

int RenderBlock::clearFloatsIfNeeded(RenderBox* child, MarginInfo& marginInfo, int oldTopPosMargin, int oldTopNegMargin, int yPos)
{
    int heightIncrease = getClearDelta(child, yPos);
    if (!heightIncrease)
        return yPos;

    if (child->isSelfCollapsingBlock()) {
        // For self-collapsing blocks that clear, they can still collapse their
        // margins with following siblings.  Reset the current margins to represent
        // the self-collapsing block's margins only.
        // CSS2.1 states:
        // "An element that has had clearance applied to it never collapses its top margin with its parent block's bottom margin.
        // Therefore if we are at the bottom of the block, let's go ahead and reset margins to only include the
        // self-collapsing block's bottom margin.
        bool atBottomOfBlock = true;
        for (RenderBox* curr = child->nextSiblingBox(); curr && atBottomOfBlock; curr = curr->nextSiblingBox()) {
            if (!curr->isFloatingOrPositioned())
                atBottomOfBlock = false;
        }
        
        MarginValues childMargins = marginValuesForChild(child);
        if (atBottomOfBlock) {
            marginInfo.setPositiveMargin(childMargins.positiveMarginAfter());
            marginInfo.setNegativeMargin(childMargins.negativeMarginAfter());
        } else {
            marginInfo.setPositiveMargin(max(childMargins.positiveMarginBefore(), childMargins.positiveMarginAfter()));
            marginInfo.setNegativeMargin(max(childMargins.negativeMarginBefore(), childMargins.negativeMarginAfter()));
        }
        
        // Adjust our height such that we are ready to be collapsed with subsequent siblings (or the bottom
        // of the parent block).
        setLogicalHeight(child->y() - max(0, marginInfo.margin()));
    } else
        // Increase our height by the amount we had to clear.
        setLogicalHeight(height() + heightIncrease);
    
    if (marginInfo.canCollapseWithMarginBefore()) {
        // We can no longer collapse with the top of the block since a clear
        // occurred.  The empty blocks collapse into the cleared block.
        // FIXME: This isn't quite correct.  Need clarification for what to do
        // if the height the cleared block is offset by is smaller than the
        // margins involved.
        setMaxMarginBeforeValues(oldTopPosMargin, oldTopNegMargin);
        marginInfo.setAtBeforeSideOfBlock(false);
    }
    
    return yPos + heightIncrease;
}

int RenderBlock::estimateLogicalTopPosition(RenderBox* child, const MarginInfo& marginInfo)
{
    // FIXME: We need to eliminate the estimation of vertical position, because when it's wrong we sometimes trigger a pathological
    // relayout if there are intruding floats.
    int logicalTopEstimate = logicalHeight();
    if (!marginInfo.canCollapseWithMarginBefore()) {
        int childMarginBefore = child->selfNeedsLayout() ? marginBeforeForChild(child) : collapsedMarginBeforeForChild(child);
        logicalTopEstimate += max(marginInfo.margin(), childMarginBefore);
    }
    
    bool paginated = view()->layoutState()->isPaginated();

    // Adjust logicalTopEstimate down to the next page if the margins are so large that we don't fit on the current
    // page.
    if (paginated && logicalTopEstimate > logicalHeight())
        logicalTopEstimate = min(logicalTopEstimate, nextPageLogicalTop(logicalHeight()));

    logicalTopEstimate += getClearDelta(child, logicalTopEstimate);
    
    if (paginated) {
        // If the object has a page or column break value of "before", then we should shift to the top of the next page.
        logicalTopEstimate = applyBeforeBreak(child, logicalTopEstimate);
    
        // For replaced elements and scrolled elements, we want to shift them to the next page if they don't fit on the current one.
        logicalTopEstimate = adjustForUnsplittableChild(child, logicalTopEstimate);
        
        if (!child->selfNeedsLayout() && child->isRenderBlock())
            logicalTopEstimate += toRenderBlock(child)->paginationStrut();
    }

    return logicalTopEstimate;
}

void RenderBlock::determineLogicalLeftPositionForChild(RenderBox* child)
{
    int startPosition = borderStart() + paddingStart();
    int totalAvailableLogicalWidth = borderAndPaddingLogicalWidth() + availableLogicalWidth();

    // Add in our start margin.
    int childMarginStart = marginStartForChild(child);
    int newPosition = startPosition + childMarginStart;
        
    // Some objects (e.g., tables, horizontal rules, overflow:auto blocks) avoid floats.  They need
    // to shift over as necessary to dodge any floats that might get in the way.
    if (child->avoidsFloats()) {
        int startOff = style()->isLeftToRightDirection() ? logicalLeftOffsetForLine(logicalHeight(), false) : totalAvailableLogicalWidth - logicalRightOffsetForLine(logicalHeight(), false);
        if (style()->textAlign() != WEBKIT_CENTER && !child->style()->marginStartUsing(style()).isAuto()) {
            if (childMarginStart < 0)
                startOff += childMarginStart;
            newPosition = max(newPosition, startOff); // Let the float sit in the child's margin if it can fit.
        } else if (startOff != startPosition) {
            // The object is shifting to the "end" side of the block. The object might be centered, so we need to
            // recalculate our inline direction margins. Note that the containing block content
            // width computation will take into account the delta between |startOff| and |startPosition|
            // so that we can just pass the content width in directly to the |computeMarginsInContainingBlockInlineDirection|
            // function.
            child->computeInlineDirectionMargins(this, availableLogicalWidthForLine(logicalTopForChild(child), false), logicalWidthForChild(child));
            newPosition = startOff + marginStartForChild(child);
        }
    }

    setLogicalLeftForChild(child, style()->isLeftToRightDirection() ? newPosition : totalAvailableLogicalWidth - newPosition - logicalWidthForChild(child), ApplyLayoutDelta);
}

void RenderBlock::setCollapsedBottomMargin(const MarginInfo& marginInfo)
{
    if (marginInfo.canCollapseWithMarginAfter() && !marginInfo.canCollapseWithMarginBefore()) {
        // Update our max pos/neg bottom margins, since we collapsed our bottom margins
        // with our children.
        setMaxMarginAfterValues(max(maxPositiveMarginAfter(), marginInfo.positiveMargin()), max(maxNegativeMarginAfter(), marginInfo.negativeMargin()));

        if (!marginInfo.marginAfterQuirk())
            setMarginAfterQuirk(false);

        if (marginInfo.marginAfterQuirk() && marginAfter() == 0)
            // We have no bottom margin and our last child has a quirky margin.
            // We will pick up this quirky margin and pass it through.
            // This deals with the <td><div><p> case.
            setMarginAfterQuirk(true);
    }
}

void RenderBlock::handleAfterSideOfBlock(int beforeSide, int afterSide, MarginInfo& marginInfo)
{
    marginInfo.setAtAfterSideOfBlock(true);

    // If we can't collapse with children then go ahead and add in the bottom margin.
    if (!marginInfo.canCollapseWithMarginAfter() && !marginInfo.canCollapseWithMarginBefore()
        && (!document()->inQuirksMode() || !marginInfo.quirkContainer() || !marginInfo.marginAfterQuirk()))
        setLogicalHeight(logicalHeight() + marginInfo.margin());
        
    // Now add in our bottom border/padding.
    setLogicalHeight(logicalHeight() + afterSide);

    // Negative margins can cause our height to shrink below our minimal height (border/padding).
    // If this happens, ensure that the computed height is increased to the minimal height.
    setLogicalHeight(max(logicalHeight(), beforeSide + afterSide));

    // Update our bottom collapsed margin info.
    setCollapsedBottomMargin(marginInfo);
}

void RenderBlock::setLogicalLeftForChild(RenderBox* child, int logicalLeft, ApplyLayoutDeltaMode applyDelta)
{
    if (isHorizontalWritingMode()) {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(IntSize(child->x() - logicalLeft, 0));
        child->setX(logicalLeft);
    } else {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(IntSize(0, child->y() - logicalLeft));
        child->setY(logicalLeft);
    }
}

void RenderBlock::setLogicalTopForChild(RenderBox* child, int logicalTop, ApplyLayoutDeltaMode applyDelta)
{
    if (isHorizontalWritingMode()) {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(IntSize(0, child->y() - logicalTop));
        child->setY(logicalTop);
    } else {
        if (applyDelta == ApplyLayoutDelta)
            view()->addLayoutDelta(IntSize(child->x() - logicalTop, 0));
        child->setX(logicalTop);
    }
}

void RenderBlock::layoutBlockChildren(bool relayoutChildren, int& maxFloatLogicalBottom)
{
    if (gPercentHeightDescendantsMap) {
        if (HashSet<RenderBox*>* descendants = gPercentHeightDescendantsMap->get(this)) {
            HashSet<RenderBox*>::iterator end = descendants->end();
            for (HashSet<RenderBox*>::iterator it = descendants->begin(); it != end; ++it) {
                RenderBox* box = *it;
                while (box != this) {
                    if (box->normalChildNeedsLayout())
                        break;
                    box->setChildNeedsLayout(true, false);
                    box = box->containingBlock();
                    ASSERT(box);
                    if (!box)
                        break;
                }
            }
        }
    }

    int beforeEdge = borderBefore() + paddingBefore();
    int afterEdge = borderAfter() + paddingAfter() + scrollbarLogicalHeight();

    setLogicalHeight(beforeEdge);

    // The margin struct caches all our current margin collapsing state.  The compact struct caches state when we encounter compacts,
    MarginInfo marginInfo(this, beforeEdge, afterEdge);

    // Fieldsets need to find their legend and position it inside the border of the object.
    // The legend then gets skipped during normal layout.  The same is true for ruby text.
    // It doesn't get included in the normal layout process but is instead skipped.
    RenderObject* childToExclude = layoutSpecialExcludedChild(relayoutChildren);

    int previousFloatLogicalBottom = 0;
    maxFloatLogicalBottom = 0;

    RenderBox* next = firstChildBox();

    while (next) {
        RenderBox* child = next;
        next = child->nextSiblingBox();

        if (childToExclude == child)
            continue; // Skip this child, since it will be positioned by the specialized subclass (fieldsets and ruby runs).

        // Make sure we layout children if they need it.
        // FIXME: Technically percentage height objects only need a relayout if their percentage isn't going to be turned into
        // an auto value.  Add a method to determine this, so that we can avoid the relayout.
        if (relayoutChildren || ((child->style()->logicalHeight().isPercent() || child->style()->logicalMinHeight().isPercent() || child->style()->logicalMaxHeight().isPercent()) && !isRenderView()))
            child->setChildNeedsLayout(true, false);

        // If relayoutChildren is set and the child has percentage padding, we also need to invalidate the child's pref widths.
        if (relayoutChildren && (child->style()->paddingStart().isPercent() || child->style()->paddingEnd().isPercent()))
            child->setPreferredLogicalWidthsDirty(true, false);

        // Handle the four types of special elements first.  These include positioned content, floating content, compacts and
        // run-ins.  When we encounter these four types of objects, we don't actually lay them out as normal flow blocks.
        if (handleSpecialChild(child, marginInfo))
            continue;

        // Lay out the child.
        layoutBlockChild(child, marginInfo, previousFloatLogicalBottom, maxFloatLogicalBottom);
    }
    
    // Now do the handling of the bottom of the block, adding in our bottom border/padding and
    // determining the correct collapsed bottom margin information.
    handleAfterSideOfBlock(beforeEdge, afterEdge, marginInfo);
}

void RenderBlock::layoutBlockChild(RenderBox* child, MarginInfo& marginInfo, int& previousFloatLogicalBottom, int& maxFloatLogicalBottom)
{
    int oldPosMarginBefore = maxPositiveMarginBefore();
    int oldNegMarginBefore = maxNegativeMarginBefore();

    // The child is a normal flow object.  Compute the margins we will use for collapsing now.
    child->computeBlockDirectionMargins(this);

    // Do not allow a collapse if the margin-before-collapse style is set to SEPARATE.
    if (child->style()->marginBeforeCollapse() == MSEPARATE) {
        marginInfo.setAtBeforeSideOfBlock(false);
        marginInfo.clearMargin();
    }

    // Try to guess our correct logical top position.  In most cases this guess will
    // be correct.  Only if we're wrong (when we compute the real logical top position)
    // will we have to potentially relayout.
    int logicalTopEstimate = estimateLogicalTopPosition(child, marginInfo);

    // Cache our old rect so that we can dirty the proper repaint rects if the child moves.
    IntRect oldRect(child->x(), child->y() , child->width(), child->height());
    int oldLogicalTop = logicalTopForChild(child);

#ifndef NDEBUG
    IntSize oldLayoutDelta = view()->layoutDelta();
#endif
    // Go ahead and position the child as though it didn't collapse with the top.
    setLogicalTopForChild(child, logicalTopEstimate, ApplyLayoutDelta);

    RenderBlock* childRenderBlock = child->isRenderBlock() ? toRenderBlock(child) : 0;
    bool markDescendantsWithFloats = false;
    if (logicalTopEstimate != oldLogicalTop && !child->avoidsFloats() && childRenderBlock && childRenderBlock->containsFloats())
        markDescendantsWithFloats = true;
    else if (!child->avoidsFloats() || child->shrinkToAvoidFloats()) {
        // If an element might be affected by the presence of floats, then always mark it for
        // layout.
        int fb = max(previousFloatLogicalBottom, lowestFloatLogicalBottom());
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

    bool childHadLayout = child->m_everHadLayout;
    bool childNeededLayout = child->needsLayout();
    if (childNeededLayout)
        child->layout();

    // Cache if we are at the top of the block right now.
    bool atBeforeSideOfBlock = marginInfo.atBeforeSideOfBlock();

    // Now determine the correct ypos based off examination of collapsing margin
    // values.
    int logicalTopBeforeClear = collapseMargins(child, marginInfo);

    // Now check for clear.
    int logicalTopAfterClear = clearFloatsIfNeeded(child, marginInfo, oldPosMarginBefore, oldNegMarginBefore, logicalTopBeforeClear);
    
    bool paginated = view()->layoutState()->isPaginated();
    if (paginated) {
        int oldTop = logicalTopAfterClear;
        
        // If the object has a page or column break value of "before", then we should shift to the top of the next page.
        logicalTopAfterClear = applyBeforeBreak(child, logicalTopAfterClear);
    
        // For replaced elements and scrolled elements, we want to shift them to the next page if they don't fit on the current one.
        int logicalTopBeforeUnsplittableAdjustment = logicalTopAfterClear;
        int logicalTopAfterUnsplittableAdjustment = adjustForUnsplittableChild(child, logicalTopAfterClear);
        
        int paginationStrut = 0;
        int unsplittableAdjustmentDelta = logicalTopAfterUnsplittableAdjustment - logicalTopBeforeUnsplittableAdjustment;
        if (unsplittableAdjustmentDelta)
            paginationStrut = unsplittableAdjustmentDelta;
        else if (childRenderBlock && childRenderBlock->paginationStrut())
            paginationStrut = childRenderBlock->paginationStrut();

        if (paginationStrut) {
            // We are willing to propagate out to our parent block as long as we were at the top of the block prior
            // to collapsing our margins, and as long as we didn't clear or move as a result of other pagination.
            if (atBeforeSideOfBlock && oldTop == logicalTopBeforeClear && !isPositioned() && !isTableCell()) {
                // FIXME: Should really check if we're exceeding the page height before propagating the strut, but we don't
                // have all the information to do so (the strut only has the remaining amount to push).  Gecko gets this wrong too
                // and pushes to the next page anyway, so not too concerned about it.
                setPaginationStrut(logicalTopAfterClear + paginationStrut);
                if (childRenderBlock)
                    childRenderBlock->setPaginationStrut(0);
            } else
                logicalTopAfterClear += paginationStrut;
        }

        // Similar to how we apply clearance.  Go ahead and boost height() to be the place where we're going to position the child.
        setLogicalHeight(logicalHeight() + (logicalTopAfterClear - oldTop));
    }

    setLogicalTopForChild(child, logicalTopAfterClear, ApplyLayoutDelta);

    // Now we have a final top position.  See if it really does end up being different from our estimate.
    if (logicalTopAfterClear != logicalTopEstimate) {
        if (child->shrinkToAvoidFloats()) {
            // The child's width depends on the line width.
            // When the child shifts to clear an item, its width can
            // change (because it has more available line width).
            // So go ahead and mark the item as dirty.
            child->setChildNeedsLayout(true, false);
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
    determineLogicalLeftPositionForChild(child);

    // Update our height now that the child has been placed in the correct position.
    setLogicalHeight(logicalHeight() + logicalHeightForChild(child));
    if (child->style()->marginAfterCollapse() == MSEPARATE) {
        setLogicalHeight(logicalHeight() + marginAfterForChild(child));
        marginInfo.clearMargin();
    }
    // If the child has overhanging floats that intrude into following siblings (or possibly out
    // of this block), then the parent gets notified of the floats now.
    if (childRenderBlock && childRenderBlock->containsFloats())
        maxFloatLogicalBottom = max(maxFloatLogicalBottom, addOverhangingFloats(toRenderBlock(child), -child->logicalLeft(), -child->logicalTop(), !childNeededLayout));

    IntSize childOffset(child->x() - oldRect.x(), child->y() - oldRect.y());
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
        int newHeight = applyAfterBreak(child, logicalHeight(), marginInfo);
        if (newHeight != height())
            setLogicalHeight(newHeight);
    }

    ASSERT(oldLayoutDelta == view()->layoutDelta());
}

void RenderBlock::simplifiedNormalFlowLayout()
{
    if (childrenInline()) {
        ListHashSet<RootInlineBox*> lineBoxes;
        bool endOfInline = false;
        RenderObject* o = bidiFirstNotSkippingInlines(this);
        while (o) {
            if (!o->isPositioned() && (o->isReplaced() || o->isFloating())) {
                o->layoutIfNeeded();
                if (toRenderBox(o)->inlineBoxWrapper()) {
                    RootInlineBox* box = toRenderBox(o)->inlineBoxWrapper()->root();
                    lineBoxes.add(box);
                }
            } else if (o->isText() || (o->isRenderInline() && !endOfInline))
                o->setNeedsLayout(false);
            o = bidiNext(this, o, 0, false, &endOfInline);
        }

        // FIXME: Glyph overflow will get lost in this case, but not really a big deal.
        GlyphOverflowAndFallbackFontsMap textBoxDataMap;                  
        for (ListHashSet<RootInlineBox*>::const_iterator it = lineBoxes.begin(); it != lineBoxes.end(); ++it) {
            RootInlineBox* box = *it;
            box->computeOverflow(box->lineTop(), box->lineBottom(), textBoxDataMap);
        }
    } else {
        for (RenderBox* box = firstChildBox(); box; box = box->nextSiblingBox()) {
            if (!box->isPositioned())
                box->layoutIfNeeded();
        }
    }
}

bool RenderBlock::simplifiedLayout()
{
    if ((!posChildNeedsLayout() && !needsSimplifiedNormalFlowLayout()) || normalChildNeedsLayout() || selfNeedsLayout())
        return false;

    LayoutStateMaintainer statePusher(view(), this, IntSize(x(), y()), hasColumns() || hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());
    
    if (needsPositionedMovementLayout() && !tryLayoutDoingPositionedMovementOnly())
        return false;

    // Lay out positioned descendants or objects that just need to recompute overflow.
    if (needsSimplifiedNormalFlowLayout())
        simplifiedNormalFlowLayout();

    // Lay out our positioned objects if our positioned child bit is set.
    if (posChildNeedsLayout())
        layoutPositionedObjects(false);

    // Recompute our overflow information.
    // FIXME: We could do better here by computing a temporary overflow object from layoutPositionedObjects and only
    // updating our overflow if we either used to have overflow or if the new temporary object has overflow.
    // For now just always recompute overflow.  This is no worse performance-wise than the old code that called rightmostPosition and
    // lowestPosition on every relayout so it's not a regression.
    m_overflow.clear();
    computeOverflow(clientLogicalBottom(), true);

    statePusher.pop();
    
    updateLayerTransform();

    updateScrollInfoAfterLayout();

    setNeedsLayout(false);
    return true;
}

void RenderBlock::layoutPositionedObjects(bool relayoutChildren)
{
    if (!m_positionedObjects)
        return;
        
    if (hasColumns())
        view()->layoutState()->clearPaginationInformation(); // Positioned objects are not part of the column flow, so they don't paginate with the columns.

    RenderBox* r;
    Iterator end = m_positionedObjects->end();
    for (Iterator it = m_positionedObjects->begin(); it != end; ++it) {
        r = *it;
        // When a non-positioned block element moves, it may have positioned children that are implicitly positioned relative to the
        // non-positioned block.  Rather than trying to detect all of these movement cases, we just always lay out positioned
        // objects that are positioned implicitly like this.  Such objects are rare, and so in typical DHTML menu usage (where everything is
        // positioned explicitly) this should not incur a performance penalty.
        if (relayoutChildren || (r->style()->hasStaticBlockPosition(isHorizontalWritingMode()) && r->parent() != this && r->parent()->isBlockFlow()))
            r->setChildNeedsLayout(true, false);
            
        // If relayoutChildren is set and we have percentage padding, we also need to invalidate the child's pref widths.
        if (relayoutChildren && (r->style()->paddingStart().isPercent() || r->style()->paddingEnd().isPercent()))
            r->setPreferredLogicalWidthsDirty(true, false);
        
        if (!r->needsLayout())
            r->markForPaginationRelayoutIfNeeded();

        // We don't have to do a full layout.  We just have to update our position. Try that first. If we have shrink-to-fit width
        // and we hit the available width constraint, the layoutIfNeeded() will catch it and do a full layout.
        if (r->needsPositionedMovementLayoutOnly() && r->tryLayoutDoingPositionedMovementOnly())
            r->setNeedsLayout(false);
        r->layoutIfNeeded();
    }
    
    if (hasColumns())
        view()->layoutState()->m_columnInfo = columnInfo(); // FIXME: Kind of gross. We just put this back into the layout state so that pop() will work.
}

void RenderBlock::markPositionedObjectsForLayout()
{
    if (m_positionedObjects) {
        RenderBox* r;
        Iterator end = m_positionedObjects->end();
        for (Iterator it = m_positionedObjects->begin(); it != end; ++it) {
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

    if (view()->layoutState()->pageLogicalHeightChanged() || (view()->layoutState()->pageLogicalHeight() && view()->layoutState()->pageLogicalOffset(logicalTop()) != pageLogicalOffset()))
        setChildNeedsLayout(true, false);
}

void RenderBlock::repaintOverhangingFloats(bool paintAllDescendants)
{
    // Repaint any overhanging floats (if we know we're the one to paint them).
    // Otherwise, bail out.
    if (!hasOverhangingFloats())
        return;

    // FIXME: Avoid disabling LayoutState. At the very least, don't disable it for floats originating
    // in this block. Better yet would be to push extra state for the containers of other floats.
    view()->disableLayoutState();
    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        // Only repaint the object if it is overhanging, is not in its own layer, and
        // is our responsibility to paint (m_shouldPaint is set). When paintAllDescendants is true, the latter
        // condition is replaced with being a descendant of us.
        if (logicalBottomForFloat(r) > logicalHeight() && ((paintAllDescendants && r->m_renderer->isDescendantOf(this)) || r->m_shouldPaint) && !r->m_renderer->hasSelfPaintingLayer()) {
            r->m_renderer->repaint();
            r->m_renderer->repaintOverhangingFloats();
        }
    }
    view()->enableLayoutState();
}
 
void RenderBlock::paint(PaintInfo& paintInfo, int tx, int ty)
{
    tx += x();
    ty += y();
    
    PaintPhase phase = paintInfo.phase;

    // Check if we need to do anything at all.
    // FIXME: Could eliminate the isRoot() check if we fix background painting so that the RenderView
    // paints the root's background.
    if (!isRoot()) {
        IntRect overflowBox = visualOverflowRect();
        flipForWritingMode(overflowBox);
        overflowBox.inflate(maximalOutlineSize(paintInfo.phase));
        overflowBox.move(tx, ty);
        if (!overflowBox.intersects(paintInfo.rect))
            return;
    }

    bool pushedClip = pushContentsClip(paintInfo, tx, ty);
    paintObject(paintInfo, tx, ty);
    if (pushedClip)
        popContentsClip(paintInfo, phase, tx, ty);

    // Our scrollbar widgets paint exactly when we tell them to, so that they work properly with
    // z-index.  We paint after we painted the background/border, so that the scrollbars will
    // sit above the background/border.
    if (hasOverflowClip() && style()->visibility() == VISIBLE && (phase == PaintPhaseBlockBackground || phase == PaintPhaseChildBlockBackground) && paintInfo.shouldPaintWithinRoot(this))
        layer()->paintOverflowControls(paintInfo.context, tx, ty, paintInfo.rect);
}

void RenderBlock::paintColumnRules(PaintInfo& paintInfo, int tx, int ty)
{
    if (paintInfo.context->paintingDisabled())
        return;

    const Color& ruleColor = style()->visitedDependentColor(CSSPropertyWebkitColumnRuleColor);
    bool ruleTransparent = style()->columnRuleIsTransparent();
    EBorderStyle ruleStyle = style()->columnRuleStyle();
    int ruleWidth = style()->columnRuleWidth();
    int colGap = columnGap();
    bool renderRule = ruleStyle > BHIDDEN && !ruleTransparent && ruleWidth <= colGap;
    if (!renderRule)
        return;

    // We need to do multiple passes, breaking up our child painting into strips.
    ColumnInfo* colInfo = columnInfo();
    unsigned colCount = columnCount(colInfo);
    int currLogicalLeftOffset = style()->isLeftToRightDirection() ? 0 : contentLogicalWidth();
    int ruleAdd = logicalLeftOffsetForContent();
    int ruleLogicalLeft = style()->isLeftToRightDirection() ? 0 : contentLogicalWidth();

    const AffineTransform& currentCTM = paintInfo.context->getCTM();
    bool antialias = !currentCTM.isIdentityOrTranslationOrFlipped();

    for (unsigned i = 0; i < colCount; i++) {
        IntRect colRect = columnRectAt(colInfo, i);

        int inlineDirectionSize = isHorizontalWritingMode() ? colRect.width() : colRect.height();
        
        // Move to the next position.
        if (style()->isLeftToRightDirection()) {
            ruleLogicalLeft += inlineDirectionSize + colGap / 2;
            currLogicalLeftOffset += inlineDirectionSize + colGap;
        } else {
            ruleLogicalLeft -= (inlineDirectionSize + colGap / 2);
            currLogicalLeftOffset -= (inlineDirectionSize + colGap);
        }
       
        // Now paint the column rule.
        if (i < colCount - 1) {
            int ruleLeft = isHorizontalWritingMode() ? tx + ruleLogicalLeft - ruleWidth / 2 + ruleAdd : tx + borderBefore() + paddingBefore();
            int ruleRight = isHorizontalWritingMode() ? ruleLeft + ruleWidth : ruleLeft + contentWidth();
            int ruleTop = isHorizontalWritingMode() ? ty + borderTop() + paddingTop() : ty + ruleLogicalLeft - ruleWidth / 2 + ruleAdd;
            int ruleBottom = isHorizontalWritingMode() ? ruleTop + contentHeight() : ruleTop + ruleWidth;
            drawLineForBoxSide(paintInfo.context, ruleLeft, ruleTop, ruleRight, ruleBottom,
                               style()->isLeftToRightDirection() ? BSLeft : BSRight, ruleColor, ruleStyle, 0, 0, antialias);
        }
        
        ruleLogicalLeft = currLogicalLeftOffset;
    }
}

void RenderBlock::paintColumnContents(PaintInfo& paintInfo, int tx, int ty, bool paintingFloats)
{
    // We need to do multiple passes, breaking up our child painting into strips.
    GraphicsContext* context = paintInfo.context;
    ColumnInfo* colInfo = columnInfo();
    unsigned colCount = columnCount(colInfo);
    if (!colCount)
        return;
    int currLogicalTopOffset = 0;
    for (unsigned i = 0; i < colCount; i++) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        IntRect colRect = columnRectAt(colInfo, i);
        flipForWritingMode(colRect);
        int logicalLeftOffset = (isHorizontalWritingMode() ? colRect.x() : colRect.y()) - logicalLeftOffsetForContent();
        IntSize offset = isHorizontalWritingMode() ? IntSize(logicalLeftOffset, currLogicalTopOffset) : IntSize(currLogicalTopOffset, logicalLeftOffset);
        colRect.move(tx, ty);
        PaintInfo info(paintInfo);
        info.rect.intersect(colRect);
        
        if (!info.rect.isEmpty()) {
            GraphicsContextStateSaver stateSaver(*context);
            
            // Each strip pushes a clip, since column boxes are specified as being
            // like overflow:hidden.
            context->clip(colRect);

            // Adjust our x and y when painting.
            int finalX = tx + offset.width();
            int finalY = ty + offset.height();
            if (paintingFloats)
                paintFloats(info, finalX, finalY, paintInfo.phase == PaintPhaseSelection || paintInfo.phase == PaintPhaseTextClip);
            else
                paintContents(info, finalX, finalY);
        }

        int blockDelta = (isHorizontalWritingMode() ? colRect.height() : colRect.width());
        if (style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset += blockDelta;
        else
            currLogicalTopOffset -= blockDelta;
    }
}

void RenderBlock::paintContents(PaintInfo& paintInfo, int tx, int ty)
{
    // Avoid painting descendants of the root element when stylesheets haven't loaded.  This eliminates FOUC.
    // It's ok not to draw, because later on, when all the stylesheets do load, updateStyleSelector on the Document
    // will do a full repaint().
    if (document()->didLayoutWithPendingStylesheets() && !isRenderView())
        return;

    if (childrenInline())
        m_lineBoxes.paint(this, paintInfo, tx, ty);
    else
        paintChildren(paintInfo, tx, ty);
}

void RenderBlock::paintChildren(PaintInfo& paintInfo, int tx, int ty)
{
    PaintPhase newPhase = (paintInfo.phase == PaintPhaseChildOutlines) ? PaintPhaseOutline : paintInfo.phase;
    newPhase = (newPhase == PaintPhaseChildBlockBackgrounds) ? PaintPhaseChildBlockBackground : newPhase;
    
    // We don't paint our own background, but we do let the kids paint their backgrounds.
    PaintInfo info(paintInfo);
    info.phase = newPhase;
    info.updatePaintingRootForChildren(this);
    
    // FIXME: Paint-time pagination is obsolete and is now only used by embedded WebViews inside AppKit
    // NSViews.  Do not add any more code for this.
    RenderView* renderView = view();
    bool usePrintRect = !renderView->printRect().isEmpty();
    
    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {        
        // Check for page-break-before: always, and if it's set, break and bail.
        bool checkBeforeAlways = !childrenInline() && (usePrintRect && child->style()->pageBreakBefore() == PBALWAYS);
        if (checkBeforeAlways
            && (ty + child->y()) > paintInfo.rect.y()
            && (ty + child->y()) < paintInfo.rect.maxY()) {
            view()->setBestTruncatedAt(ty + child->y(), this, true);
            return;
        }

        if (!child->isFloating() && child->isReplaced() && usePrintRect && child->height() <= renderView->printRect().height()) {
            // Paginate block-level replaced elements.
            if (ty + child->y() + child->height() > renderView->printRect().maxY()) {
                if (ty + child->y() < renderView->truncatedAt())
                    renderView->setBestTruncatedAt(ty + child->y(), child);
                // If we were able to truncate, don't paint.
                if (ty + child->y() >= renderView->truncatedAt())
                    break;
            }
        }

        IntPoint childPoint = flipForWritingMode(child, IntPoint(tx, ty), ParentToChildFlippingAdjustment);
        if (!child->hasSelfPaintingLayer() && !child->isFloating())
            child->paint(info, childPoint.x(), childPoint.y());

        // Check for page-break-after: always, and if it's set, break and bail.
        bool checkAfterAlways = !childrenInline() && (usePrintRect && child->style()->pageBreakAfter() == PBALWAYS);
        if (checkAfterAlways
            && (ty + child->y() + child->height()) > paintInfo.rect.y()
            && (ty + child->y() + child->height()) < paintInfo.rect.maxY()) {
            view()->setBestTruncatedAt(ty + child->y() + child->height() + max(0, child->collapsedMarginAfter()), this, true);
            return;
        }
    }
}

void RenderBlock::paintCaret(PaintInfo& paintInfo, int tx, int ty, CaretType type)
{
    SelectionController* selection = type == CursorCaret ? frame()->selection() : frame()->page()->dragCaretController();

    // Paint the caret if the SelectionController says so or if caret browsing is enabled
    bool caretBrowsing = frame()->settings() && frame()->settings()->caretBrowsingEnabled();
    RenderObject* caretPainter = selection->caretRenderer();
    if (caretPainter == this && (selection->isContentEditable() || caretBrowsing)) {
        // Convert the painting offset into the local coordinate system of this renderer,
        // to match the localCaretRect computed by the SelectionController
        offsetForContents(tx, ty);

        if (type == CursorCaret)
            frame()->selection()->paintCaret(paintInfo.context, tx, ty, paintInfo.rect);
        else
            frame()->selection()->paintDragCaret(paintInfo.context, tx, ty, paintInfo.rect);
    }
}

void RenderBlock::paintObject(PaintInfo& paintInfo, int tx, int ty)
{
    PaintPhase paintPhase = paintInfo.phase;

    // 1. paint background, borders etc
    if ((paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) && style()->visibility() == VISIBLE) {
        if (hasBoxDecorations())
            paintBoxDecorations(paintInfo, tx, ty);
        if (hasColumns())
            paintColumnRules(paintInfo, tx, ty);
    }

    if (paintPhase == PaintPhaseMask && style()->visibility() == VISIBLE) {
        paintMask(paintInfo, tx, ty);
        return;
    }

    // We're done.  We don't bother painting any children.
    if (paintPhase == PaintPhaseBlockBackground)
        return;

    // Adjust our painting position if we're inside a scrolled layer (e.g., an overflow:auto div).
    int scrolledX = tx;
    int scrolledY = ty;
    if (hasOverflowClip()) {
        IntSize offset = layer()->scrolledContentOffset();
        scrolledX -= offset.width();
        scrolledY -= offset.height();
    }

    // 2. paint contents
    if (paintPhase != PaintPhaseSelfOutline) {
        if (hasColumns())
            paintColumnContents(paintInfo, scrolledX, scrolledY);
        else
            paintContents(paintInfo, scrolledX, scrolledY);
    }

    // 3. paint selection
    // FIXME: Make this work with multi column layouts.  For now don't fill gaps.
    bool isPrinting = document()->printing();
    if (!isPrinting && !hasColumns())
        paintSelection(paintInfo, scrolledX, scrolledY); // Fill in gaps in selection on lines and between blocks.

    // 4. paint floats.
    if (paintPhase == PaintPhaseFloat || paintPhase == PaintPhaseSelection || paintPhase == PaintPhaseTextClip) {
        if (hasColumns())
            paintColumnContents(paintInfo, scrolledX, scrolledY, true);
        else
            paintFloats(paintInfo, scrolledX, scrolledY, paintPhase == PaintPhaseSelection || paintPhase == PaintPhaseTextClip);
    }

    // 5. paint outline.
    if ((paintPhase == PaintPhaseOutline || paintPhase == PaintPhaseSelfOutline) && hasOutline() && style()->visibility() == VISIBLE)
        paintOutline(paintInfo.context, tx, ty, width(), height());

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

            if (!inlineEnclosedInSelfPaintingLayer)
                cb->addContinuationWithOutline(inlineRenderer);
            else if (!inlineRenderer->firstLineBox())
                inlineRenderer->paintOutline(paintInfo.context, tx - x() + inlineRenderer->containingBlock()->x(),
                                             ty - y() + inlineRenderer->containingBlock()->y());
        }
        paintContinuationOutlines(paintInfo, tx, ty);
    }

    // 7. paint caret.
    // If the caret's node's render object's containing block is this block, and the paint action is PaintPhaseForeground,
    // then paint the caret.
    if (paintPhase == PaintPhaseForeground) {        
        paintCaret(paintInfo, scrolledX, scrolledY, CursorCaret);
        paintCaret(paintInfo, scrolledX, scrolledY, DragCaret);
    }
}

IntPoint RenderBlock::flipFloatForWritingMode(const FloatingObject* child, const IntPoint& point) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return point;
    
    // This is similar to the ParentToChildFlippingAdjustment in RenderBox::flipForWritingMode.  We have to subtract out our left/top offsets twice, since
    // it's going to get added back in.  We hide this complication here so that the calling code looks normal for the unflipped
    // case.
    if (isHorizontalWritingMode())
        return IntPoint(point.x(), point.y() + height() - child->renderer()->height() - 2 * yPositionForFloatIncludingMargin(child));
    return IntPoint(point.x() + width() - child->width() - 2 * xPositionForFloatIncludingMargin(child), point.y());
}

void RenderBlock::paintFloats(PaintInfo& paintInfo, int tx, int ty, bool preservePhase)
{
    if (!m_floatingObjects)
        return;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        // Only paint the object if our m_shouldPaint flag is set.
        if (r->m_shouldPaint && !r->m_renderer->hasSelfPaintingLayer()) {
            PaintInfo currentPaintInfo(paintInfo);
            currentPaintInfo.phase = preservePhase ? paintInfo.phase : PaintPhaseBlockBackground;
            IntPoint childPoint = flipFloatForWritingMode(r, IntPoint(tx + xPositionForFloatIncludingMargin(r) - r->m_renderer->x(), ty + yPositionForFloatIncludingMargin(r) - r->m_renderer->y()));
            r->m_renderer->paint(currentPaintInfo, childPoint.x(), childPoint.y());
            if (!preservePhase) {
                currentPaintInfo.phase = PaintPhaseChildBlockBackgrounds;
                r->m_renderer->paint(currentPaintInfo, childPoint.x(), childPoint.y());
                currentPaintInfo.phase = PaintPhaseFloat;
                r->m_renderer->paint(currentPaintInfo, childPoint.x(), childPoint.y());
                currentPaintInfo.phase = PaintPhaseForeground;
                r->m_renderer->paint(currentPaintInfo, childPoint.x(), childPoint.y());
                currentPaintInfo.phase = PaintPhaseOutline;
                r->m_renderer->paint(currentPaintInfo, childPoint.x(), childPoint.y());
            }
        }
    }
}

void RenderBlock::paintEllipsisBoxes(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this) || !firstLineBox())
        return;

    if (style()->visibility() == VISIBLE && paintInfo.phase == PaintPhaseForeground) {
        // We can check the first box and last box and avoid painting if we don't
        // intersect.
        int yPos = ty + firstLineBox()->y();
        int h = lastLineBox()->y() + lastLineBox()->logicalHeight() - firstLineBox()->y();
        if (yPos >= paintInfo.rect.maxY() || yPos + h <= paintInfo.rect.y())
            return;

        // See if our boxes intersect with the dirty rect.  If so, then we paint
        // them.  Note that boxes can easily overlap, so we can't make any assumptions
        // based off positions of our first line box or our last line box.
        for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
            yPos = ty + curr->y();
            h = curr->logicalHeight();
            if (curr->ellipsisBox() && yPos < paintInfo.rect.maxY() && yPos + h > paintInfo.rect.y())
                curr->paintEllipsisBox(paintInfo, tx, ty, curr->lineTop(), curr->lineBottom());
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
        table->set(this, continuations);
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

void RenderBlock::paintContinuationOutlines(PaintInfo& info, int tx, int ty)
{
    ContinuationOutlineTableMap* table = continuationOutlineTable();
    if (table->isEmpty())
        return;
        
    ListHashSet<RenderInline*>* continuations = table->get(this);
    if (!continuations)
        return;
        
    // Paint each continuation outline.
    ListHashSet<RenderInline*>::iterator end = continuations->end();
    for (ListHashSet<RenderInline*>::iterator it = continuations->begin(); it != end; ++it) {
        // Need to add in the coordinates of the intervening blocks.
        RenderInline* flow = *it;
        RenderBlock* block = flow->containingBlock();
        for ( ; block && block != this; block = block->containingBlock()) {
            tx += block->x();
            ty += block->y();
        }
        ASSERT(block);   
        flow->paintOutline(info.context, tx, ty);
    }
    
    // Delete
    delete continuations;
    table->remove(this);
}

bool RenderBlock::shouldPaintSelectionGaps() const
{
    return selectionState() != SelectionNone && style()->visibility() == VISIBLE && isSelectionRoot();
}

bool RenderBlock::isSelectionRoot() const
{
    if (!node())
        return false;
        
    // FIXME: Eventually tables should have to learn how to fill gaps between cells, at least in simple non-spanning cases.
    if (isTable())
        return false;
        
    if (isBody() || isRoot() || hasOverflowClip() || isRelPositioned() ||
        isFloatingOrPositioned() || isTableCell() || isInlineBlockOrInlineTable() || hasTransform() ||
        hasReflection() || hasMask() || isWritingModeRoot())
        return true;
    
    if (view() && view()->selectionStart()) {
        Node* startElement = view()->selectionStart()->node();
        if (startElement && startElement->rootEditableElement() == node())
            return true;
    }
    
    return false;
}

GapRects RenderBlock::selectionGapRectsForRepaint(RenderBoxModelObject* repaintContainer)
{
    ASSERT(!needsLayout());

    if (!shouldPaintSelectionGaps())
        return GapRects();

    // FIXME: this is broken with transforms
    TransformState transformState(TransformState::ApplyTransformDirection, FloatPoint());
    mapLocalToContainer(repaintContainer, false, false, transformState);
    IntPoint offsetFromRepaintContainer = roundedIntPoint(transformState.mappedPoint());

    if (hasOverflowClip())
        offsetFromRepaintContainer -= layer()->scrolledContentOffset();

    int lastTop = 0;
    int lastLeft = logicalLeftSelectionOffset(this, lastTop);
    int lastRight = logicalRightSelectionOffset(this, lastTop);
    
    return selectionGaps(this, offsetFromRepaintContainer, IntSize(), lastTop, lastLeft, lastRight);
}

void RenderBlock::paintSelection(PaintInfo& paintInfo, int tx, int ty)
{
    if (shouldPaintSelectionGaps() && paintInfo.phase == PaintPhaseForeground) {
        int lastTop = 0;
        int lastLeft = logicalLeftSelectionOffset(this, lastTop);
        int lastRight = logicalRightSelectionOffset(this, lastTop);
        GraphicsContextStateSaver stateSaver(*paintInfo.context);

        IntRect gapRectsBounds = selectionGaps(this, IntPoint(tx, ty), IntSize(), lastTop, lastLeft, lastRight, &paintInfo);
        if (!gapRectsBounds.isEmpty()) {
            if (RenderLayer* layer = enclosingLayer()) {
                gapRectsBounds.move(IntSize(-tx, -ty));
                if (!hasLayer()) {
                    IntRect localBounds(gapRectsBounds);
                    flipForWritingMode(localBounds);
                    gapRectsBounds = localToContainerQuad(FloatRect(localBounds), layer->renderer()).enclosingBoundingBox();
                    gapRectsBounds.move(layer->scrolledContentOffset());
                }
                layer->addBlockSelectionGapsBounds(gapRectsBounds);
            }
        }
    }
}

static void clipOutPositionedObjects(const PaintInfo* paintInfo, const IntPoint& offset, RenderBlock::PositionedObjectsListHashSet* positionedObjects)
{
    if (!positionedObjects)
        return;
    
    RenderBlock::PositionedObjectsListHashSet::const_iterator end = positionedObjects->end();
    for (RenderBlock::PositionedObjectsListHashSet::const_iterator it = positionedObjects->begin(); it != end; ++it) {
        RenderBox* r = *it;
        paintInfo->context->clipOut(IntRect(offset.x() + r->x(), offset.y() + r->y(), r->width(), r->height()));
    }
}

static int blockDirectionOffset(RenderBlock* rootBlock, const IntSize& offsetFromRootBlock)
{
    return rootBlock->isHorizontalWritingMode() ? offsetFromRootBlock.height() : offsetFromRootBlock.width();
}

static int inlineDirectionOffset(RenderBlock* rootBlock, const IntSize& offsetFromRootBlock)
{
    return rootBlock->isHorizontalWritingMode() ? offsetFromRootBlock.width() : offsetFromRootBlock.height();
}

IntRect RenderBlock::logicalRectToPhysicalRect(const IntPoint& rootBlockPhysicalPosition, const IntRect& logicalRect)
{
    IntRect result;
    if (isHorizontalWritingMode())
        result = logicalRect;
    else
        result = IntRect(logicalRect.y(), logicalRect.x(), logicalRect.height(), logicalRect.width());
    flipForWritingMode(result);
    result.move(rootBlockPhysicalPosition.x(), rootBlockPhysicalPosition.y());
    return result;
}

GapRects RenderBlock::selectionGaps(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                    int& lastLogicalTop, int& lastLogicalLeft, int& lastLogicalRight, const PaintInfo* paintInfo)
{
    // IMPORTANT: Callers of this method that intend for painting to happen need to do a save/restore.
    // Clip out floating and positioned objects when painting selection gaps.
    if (paintInfo) {
        // Note that we don't clip out overflow for positioned objects.  We just stick to the border box.
        IntRect flippedBlockRect = IntRect(offsetFromRootBlock.width(), offsetFromRootBlock.height(), width(), height());
        rootBlock->flipForWritingMode(flippedBlockRect);
        flippedBlockRect.move(rootBlockPhysicalPosition.x(), rootBlockPhysicalPosition.y());
        clipOutPositionedObjects(paintInfo, flippedBlockRect.location(), m_positionedObjects.get());
        if (isBody() || isRoot()) // The <body> must make sure to examine its containingBlock's positioned objects.
            for (RenderBlock* cb = containingBlock(); cb && !cb->isRenderView(); cb = cb->containingBlock())
                clipOutPositionedObjects(paintInfo, IntPoint(cb->x(), cb->y()), cb->m_positionedObjects.get()); // FIXME: Not right for flipped writing modes.
        if (m_floatingObjects) {
            FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* r = *it;
                IntRect floatBox = IntRect(offsetFromRootBlock.width() + xPositionForFloatIncludingMargin(r),
                                           offsetFromRootBlock.height() + yPositionForFloatIncludingMargin(r),
                                           r->m_renderer->width(), r->m_renderer->height());
                rootBlock->flipForWritingMode(floatBox);
                floatBox.move(rootBlockPhysicalPosition.x(), rootBlockPhysicalPosition.y());
                paintInfo->context->clipOut(floatBox);
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
        lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, logicalHeight());
        lastLogicalRight = logicalRightSelectionOffset(rootBlock, logicalHeight());
        return result;
    }

    if (childrenInline())
        result = inlineSelectionGaps(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, paintInfo);
    else
        result = blockSelectionGaps(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, paintInfo);

    // Go ahead and fill the vertical gap all the way to the bottom of our block if the selection extends past our block.
    if (rootBlock == this && (selectionState() != SelectionBoth && selectionState() != SelectionEnd))
        result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, 
                                             logicalHeight(), paintInfo));
    return result;
}

GapRects RenderBlock::inlineSelectionGaps(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                          int& lastLogicalTop, int& lastLogicalLeft, int& lastLogicalRight, const PaintInfo* paintInfo)
{
    GapRects result;

    bool containsStart = selectionState() == SelectionStart || selectionState() == SelectionBoth;

    if (!firstLineBox()) {
        if (containsStart) {
            // Go ahead and update our lastLogicalTop to be the bottom of the block.  <hr>s or empty blocks with height can trip this
            // case.
            lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalHeight();
            lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, logicalHeight());
            lastLogicalRight = logicalRightSelectionOffset(rootBlock, logicalHeight());
        }
        return result;
    }

    RootInlineBox* lastSelectedLine = 0;
    RootInlineBox* curr;
    for (curr = firstRootBox(); curr && !curr->hasSelectedChildren(); curr = curr->nextRootBox()) { }

    // Now paint the gaps for the lines.
    for (; curr && curr->hasSelectedChildren(); curr = curr->nextRootBox()) {
        int selTop =  curr->selectionTop();
        int selHeight = curr->selectionHeight();

        if (!containsStart && !lastSelectedLine &&
            selectionState() != SelectionStart && selectionState() != SelectionBoth)
            result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, 
                                                 selTop, paintInfo));
        
        IntRect logicalRect(curr->logicalLeft(), selTop, curr->logicalWidth(), selTop + selHeight);
        logicalRect.move(isHorizontalWritingMode() ? offsetFromRootBlock : IntSize(offsetFromRootBlock.height(), offsetFromRootBlock.width()));
        IntRect physicalRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, logicalRect);
        if (!paintInfo || (isHorizontalWritingMode() && physicalRect.y() < paintInfo->rect.maxY() && physicalRect.maxY() > paintInfo->rect.y())
            || (!isHorizontalWritingMode() && physicalRect.x() < paintInfo->rect.maxX() && physicalRect.maxX() > paintInfo->rect.x()))
            result.unite(curr->lineSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, selTop, selHeight, paintInfo));

        lastSelectedLine = curr;
    }

    if (containsStart && !lastSelectedLine)
        // VisibleSelection must start just after our last line.
        lastSelectedLine = lastRootBox();

    if (lastSelectedLine && selectionState() != SelectionEnd && selectionState() != SelectionBoth) {
        // Go ahead and update our lastY to be the bottom of the last selected line.
        lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + lastSelectedLine->selectionBottom();
        lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, lastSelectedLine->selectionBottom());
        lastLogicalRight = logicalRightSelectionOffset(rootBlock, lastSelectedLine->selectionBottom());
    }
    return result;
}

GapRects RenderBlock::blockSelectionGaps(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                         int& lastLogicalTop, int& lastLogicalLeft, int& lastLogicalRight, const PaintInfo* paintInfo)
{
    GapRects result;

    // Go ahead and jump right to the first block child that contains some selected objects.
    RenderBox* curr;
    for (curr = firstChildBox(); curr && curr->selectionState() == SelectionNone; curr = curr->nextSiblingBox()) { }

    for (bool sawSelectionEnd = false; curr && !sawSelectionEnd; curr = curr->nextSiblingBox()) {
        SelectionState childState = curr->selectionState();
        if (childState == SelectionBoth || childState == SelectionEnd)
            sawSelectionEnd = true;

        if (curr->isFloatingOrPositioned())
            continue; // We must be a normal flow object in order to even be considered.

        if (curr->isRelPositioned() && curr->hasLayer()) {
            // If the relposition offset is anything other than 0, then treat this just like an absolute positioned element.
            // Just disregard it completely.
            IntSize relOffset = curr->layer()->relativePositionOffset();
            if (relOffset.width() || relOffset.height())
                continue;
        }

        bool paintsOwnSelection = curr->shouldPaintSelectionGaps() || curr->isTable(); // FIXME: Eventually we won't special-case table like this.
        bool fillBlockGaps = paintsOwnSelection || (curr->canBeSelectionLeaf() && childState != SelectionNone);
        if (fillBlockGaps) {
            // We need to fill the vertical gap above this object.
            if (childState == SelectionEnd || childState == SelectionInside)
                // Fill the gap above the object.
                result.uniteCenter(blockSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, lastLogicalTop, lastLogicalLeft, lastLogicalRight, 
                                                     curr->logicalTop(), paintInfo));

            // Only fill side gaps for objects that paint their own selection if we know for sure the selection is going to extend all the way *past*
            // our object.  We know this if the selection did not end inside our object.
            if (paintsOwnSelection && (childState == SelectionStart || sawSelectionEnd))
                childState = SelectionNone;

            // Fill side gaps on this object based off its state.
            bool leftGap, rightGap;
            getSelectionGapInfo(childState, leftGap, rightGap);

            if (leftGap)
                result.uniteLeft(logicalLeftSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, this, curr->logicalLeft(), curr->logicalTop(), curr->logicalHeight(), paintInfo));
            if (rightGap)
                result.uniteRight(logicalRightSelectionGap(rootBlock, rootBlockPhysicalPosition, offsetFromRootBlock, this, curr->logicalRight(), curr->logicalTop(), curr->logicalHeight(), paintInfo));

            // Update lastLogicalTop to be just underneath the object.  lastLogicalLeft and lastLogicalRight extend as far as
            // they can without bumping into floating or positioned objects.  Ideally they will go right up
            // to the border of the root selection block.
            lastLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + curr->logicalBottom();
            lastLogicalLeft = logicalLeftSelectionOffset(rootBlock, curr->logicalBottom());
            lastLogicalRight = logicalRightSelectionOffset(rootBlock, curr->logicalBottom());
        } else if (childState != SelectionNone)
            // We must be a block that has some selected object inside it.  Go ahead and recur.
            result.unite(toRenderBlock(curr)->selectionGaps(rootBlock, rootBlockPhysicalPosition, IntSize(offsetFromRootBlock.width() + curr->x(), offsetFromRootBlock.height() + curr->y()), 
                                                            lastLogicalTop, lastLogicalLeft, lastLogicalRight, paintInfo));
    }
    return result;
}

IntRect RenderBlock::blockSelectionGap(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                       int lastLogicalTop, int lastLogicalLeft, int lastLogicalRight, int logicalBottom, const PaintInfo* paintInfo)
{
    int logicalTop = lastLogicalTop;
    int logicalHeight = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalBottom - logicalTop;
    if (logicalHeight <= 0)
        return IntRect();

    // Get the selection offsets for the bottom of the gap
    int logicalLeft = max(lastLogicalLeft, logicalLeftSelectionOffset(rootBlock, logicalBottom));
    int logicalRight = min(lastLogicalRight, logicalRightSelectionOffset(rootBlock, logicalBottom));
    int logicalWidth = logicalRight - logicalLeft;
    if (logicalWidth <= 0)
        return IntRect();

    IntRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, IntRect(logicalLeft, logicalTop, logicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(gapRect, selectionBackgroundColor(), style()->colorSpace());
    return gapRect;
}

IntRect RenderBlock::logicalLeftSelectionGap(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                             RenderObject* selObj, int logicalLeft, int logicalTop, int logicalHeight, const PaintInfo* paintInfo)
{
    int rootBlockLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalTop;
    int rootBlockLogicalLeft = max(logicalLeftSelectionOffset(rootBlock, logicalTop), logicalLeftSelectionOffset(rootBlock, logicalTop + logicalHeight));
    int rootBlockLogicalRight = min(inlineDirectionOffset(rootBlock, offsetFromRootBlock) + logicalLeft, min(logicalRightSelectionOffset(rootBlock, logicalTop), logicalRightSelectionOffset(rootBlock, logicalTop + logicalHeight)));
    int rootBlockLogicalWidth = rootBlockLogicalRight - rootBlockLogicalLeft;
    if (rootBlockLogicalWidth <= 0)
        return IntRect();

    IntRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, IntRect(rootBlockLogicalLeft, rootBlockLogicalTop, rootBlockLogicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(gapRect, selObj->selectionBackgroundColor(), selObj->style()->colorSpace());
    return gapRect;
}

IntRect RenderBlock::logicalRightSelectionGap(RenderBlock* rootBlock, const IntPoint& rootBlockPhysicalPosition, const IntSize& offsetFromRootBlock,
                                              RenderObject* selObj, int logicalRight, int logicalTop, int logicalHeight, const PaintInfo* paintInfo)
{
    int rootBlockLogicalTop = blockDirectionOffset(rootBlock, offsetFromRootBlock) + logicalTop;
    int rootBlockLogicalLeft = max(inlineDirectionOffset(rootBlock, offsetFromRootBlock) + logicalRight, max(logicalLeftSelectionOffset(rootBlock, logicalTop), logicalLeftSelectionOffset(rootBlock, logicalTop + logicalHeight)));
    int rootBlockLogicalRight = min(logicalRightSelectionOffset(rootBlock, logicalTop), logicalRightSelectionOffset(rootBlock, logicalTop + logicalHeight));
    int rootBlockLogicalWidth = rootBlockLogicalRight - rootBlockLogicalLeft;
    if (rootBlockLogicalWidth <= 0)
        return IntRect();

    IntRect gapRect = rootBlock->logicalRectToPhysicalRect(rootBlockPhysicalPosition, IntRect(rootBlockLogicalLeft, rootBlockLogicalTop, rootBlockLogicalWidth, logicalHeight));
    if (paintInfo)
        paintInfo->context->fillRect(gapRect, selObj->selectionBackgroundColor(), selObj->style()->colorSpace());
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

int RenderBlock::logicalLeftSelectionOffset(RenderBlock* rootBlock, int position)
{
    int logicalLeft = logicalLeftOffsetForLine(position, false);
    if (logicalLeft == logicalLeftOffsetForContent()) {
        if (rootBlock != this)
            // The border can potentially be further extended by our containingBlock().
            return containingBlock()->logicalLeftSelectionOffset(rootBlock, position + logicalTop());
        return logicalLeft;
    } else {
        RenderBlock* cb = this;
        while (cb != rootBlock) {
            logicalLeft += cb->logicalLeft();
            cb = cb->containingBlock();
        }
    }
    return logicalLeft;
}

int RenderBlock::logicalRightSelectionOffset(RenderBlock* rootBlock, int position)
{
    int logicalRight = logicalRightOffsetForLine(position, false);
    if (logicalRight == logicalRightOffsetForContent()) {
        if (rootBlock != this)
            // The border can potentially be further extended by our containingBlock().
            return containingBlock()->logicalRightSelectionOffset(rootBlock, position + logicalTop());
        return logicalRight;
    } else {
        RenderBlock* cb = this;
        while (cb != rootBlock) {
            logicalRight += cb->logicalLeft();
            cb = cb->containingBlock();
        }
    }
    return logicalRight;
}

void RenderBlock::insertPositionedObject(RenderBox* o)
{
    // Create the list of special objects if we don't aleady have one
    if (!m_positionedObjects)
        m_positionedObjects = adoptPtr(new PositionedObjectsListHashSet);

    m_positionedObjects->add(o);
}

void RenderBlock::removePositionedObject(RenderBox* o)
{
    if (m_positionedObjects)
        m_positionedObjects->remove(o);
}

void RenderBlock::removePositionedObjects(RenderBlock* o)
{
    if (!m_positionedObjects)
        return;
    
    RenderBox* r;
    
    Iterator end = m_positionedObjects->end();
    
    Vector<RenderBox*, 16> deadObjects;

    for (Iterator it = m_positionedObjects->begin(); it != end; ++it) {
        r = *it;
        if (!o || r->isDescendantOf(o)) {
            if (o)
                r->setChildNeedsLayout(true, false);
            
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
        m_positionedObjects->remove(deadObjects.at(i));
}

RenderBlock::FloatingObject* RenderBlock::insertFloatingObject(RenderBox* o)
{
    ASSERT(o->isFloating());

    // Create the list of special objects if we don't aleady have one
    if (!m_floatingObjects)
        m_floatingObjects = adoptPtr(new FloatingObjects);
    else {
        // Don't insert the object again if it's already in the list
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(o);
        if (it != floatingObjectSet.end())
            return *it;
    }

    // Create the special object entry & append it to the list

    FloatingObject* newObj = new FloatingObject(o->style()->floating() == FLEFT ? FloatingObject::FloatLeft : FloatingObject::FloatRight);
    
    // Our location is irrelevant if we're unsplittable or no pagination is in effect.
    // Just go ahead and lay out the float.
    bool isChildRenderBlock = o->isRenderBlock();
    if (isChildRenderBlock && !o->needsLayout() && view()->layoutState()->pageLogicalHeightChanged())
        o->setChildNeedsLayout(true, false);
        
    bool affectedByPagination = isChildRenderBlock && view()->layoutState()->m_pageLogicalHeight;
    if (!affectedByPagination || isWritingModeRoot()) // We are unsplittable if we're a block flow root.
        o->layoutIfNeeded();
    else {
        o->computeLogicalWidth();
        o->computeBlockDirectionMargins(this);
    }
    setLogicalWidthForFloat(newObj, logicalWidthForChild(o) + marginStartForChild(o) + marginEndForChild(o));

    newObj->m_shouldPaint = !o->hasSelfPaintingLayer(); // If a layer exists, the float will paint itself.  Otherwise someone else will.
    newObj->m_isDescendant = true;
    newObj->m_renderer = o;

    m_floatingObjects->increaseObjectsCount(newObj->type());
    m_floatingObjects->set().add(newObj);
    
    return newObj;
}

void RenderBlock::removeFloatingObject(RenderBox* o)
{
    if (m_floatingObjects) {
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSet::iterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(o);
        if (it != floatingObjectSet.end()) {
            FloatingObject* r = *it;
            if (childrenInline()) {
                int logicalTop = logicalTopForFloat(r);
                int logicalBottom = logicalBottomForFloat(r);

                // Fix for https://bugs.webkit.org/show_bug.cgi?id=54995.
                if (logicalBottom < 0 || logicalBottom < logicalTop || logicalTop == numeric_limits<int>::max())
                    logicalBottom = numeric_limits<int>::max();
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
            m_floatingObjects->decreaseObjectsCount(r->type());
            floatingObjectSet.remove(it);
            ASSERT(!r->m_originatingLine);
            delete r;
        }
    }
}

void RenderBlock::removeFloatingObjectsBelow(FloatingObject* lastFloat, int logicalOffset)
{
    if (!m_floatingObjects)
        return;
    
    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObject* curr = floatingObjectSet.last();
    while (curr != lastFloat && (!curr->isPlaced() || logicalTopForFloat(curr) >= logicalOffset)) {
        m_floatingObjects->decreaseObjectsCount(curr->type());
        floatingObjectSet.removeLast();
        ASSERT(!curr->m_originatingLine);
        delete curr;
        curr = floatingObjectSet.last();
    }
}

bool RenderBlock::positionNewFloats()
{
    if (!m_floatingObjects)
        return false;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
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

    int logicalTop = logicalHeight();
    
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
        int childLogicalLeftMargin = style()->isLeftToRightDirection() ? marginStartForChild(childBox) : marginEndForChild(childBox);

        int rightOffset = logicalRightOffsetForContent(); // Constant part of right offset.
        int leftOffset = logicalLeftOffsetForContent(); // Constant part of left offset.
        int floatLogicalWidth = logicalWidthForFloat(floatingObject); // The width we look for.
        if (rightOffset - leftOffset < floatLogicalWidth)
            floatLogicalWidth = rightOffset - leftOffset; // Never look for more than what will be available.
        
        IntRect oldRect(childBox->x(), childBox->y() , childBox->width(), childBox->height());

        if (childBox->style()->clear() & CLEFT)
            logicalTop = max(lowestFloatLogicalBottom(FloatingObject::FloatLeft), logicalTop);
        if (childBox->style()->clear() & CRIGHT)
            logicalTop = max(lowestFloatLogicalBottom(FloatingObject::FloatRight), logicalTop);

        int floatLogicalLeft;
        if (childBox->style()->floating() == FLEFT) {
            int heightRemainingLeft = 1;
            int heightRemainingRight = 1;
            floatLogicalLeft = logicalLeftOffsetForLine(logicalTop, leftOffset, false, &heightRemainingLeft);
            while (logicalRightOffsetForLine(logicalTop, rightOffset, false, &heightRemainingRight) - floatLogicalLeft < floatLogicalWidth) {
                logicalTop += min(heightRemainingLeft, heightRemainingRight);
                floatLogicalLeft = logicalLeftOffsetForLine(logicalTop, leftOffset, false, &heightRemainingLeft);
            }
            floatLogicalLeft = max(0, floatLogicalLeft);
        } else {
            int heightRemainingLeft = 1;
            int heightRemainingRight = 1;
            floatLogicalLeft = logicalRightOffsetForLine(logicalTop, rightOffset, false, &heightRemainingRight);
            while (floatLogicalLeft - logicalLeftOffsetForLine(logicalTop, leftOffset, false, &heightRemainingLeft) < floatLogicalWidth) {
                logicalTop += min(heightRemainingLeft, heightRemainingRight);
                floatLogicalLeft = logicalRightOffsetForLine(logicalTop, rightOffset, false, &heightRemainingRight);
            }
            floatLogicalLeft -= logicalWidthForFloat(floatingObject); // Use the original width of the float here, since the local variable
                                                                      // |floatLogicalWidth| was capped to the available line width.
                                                                      // See fast/block/float/clamped-right-float.html.
        }
        
        setLogicalLeftForFloat(floatingObject, floatLogicalLeft);
        setLogicalLeftForChild(childBox, floatLogicalLeft + childLogicalLeftMargin);
        setLogicalTopForChild(childBox, logicalTop + marginBeforeForChild(childBox));

        if (view()->layoutState()->isPaginated()) {
            RenderBlock* childBlock = childBox->isRenderBlock() ? toRenderBlock(childBox) : 0;

            if (!childBox->needsLayout())
                childBox->markForPaginationRelayoutIfNeeded();;
            childBox->layoutIfNeeded();

            // If we are unsplittable and don't fit, then we need to move down.
            // We include our margins as part of the unsplittable area.
            int newLogicalTop = adjustForUnsplittableChild(childBox, logicalTop, true);
            
            // See if we have a pagination strut that is making us move down further.
            // Note that an unsplittable child can't also have a pagination strut, so this is
            // exclusive with the case above.
            if (childBlock && childBlock->paginationStrut()) {
                newLogicalTop += childBlock->paginationStrut();
                childBlock->setPaginationStrut(0);
            }
            
            if (newLogicalTop != logicalTop) {
                floatingObject->m_paginationStrut = newLogicalTop - logicalTop;
                logicalTop = newLogicalTop;
                setLogicalTopForChild(childBox, logicalTop + marginBeforeForChild(childBox));
                if (childBlock)
                    childBlock->setChildNeedsLayout(true, false);
                childBox->layoutIfNeeded();
            }
        }

        setLogicalTopForFloat(floatingObject, logicalTop);
        setLogicalHeightForFloat(floatingObject, logicalHeightForChild(childBox) + marginBeforeForChild(childBox) + marginAfterForChild(childBox));

        floatingObject->setIsPlaced();

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
    int newY = 0;
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
    if (!gPercentHeightDescendantsMap) {
        gPercentHeightDescendantsMap = new PercentHeightDescendantsMap;
        gPercentHeightContainerMap = new PercentHeightContainerMap;
    }

    HashSet<RenderBox*>* descendantSet = gPercentHeightDescendantsMap->get(this);
    if (!descendantSet) {
        descendantSet = new HashSet<RenderBox*>;
        gPercentHeightDescendantsMap->set(this, descendantSet);
    }
    bool added = descendantSet->add(descendant).second;
    if (!added) {
        ASSERT(gPercentHeightContainerMap->get(descendant));
        ASSERT(gPercentHeightContainerMap->get(descendant)->contains(this));
        return;
    }

    HashSet<RenderBlock*>* containerSet = gPercentHeightContainerMap->get(descendant);
    if (!containerSet) {
        containerSet = new HashSet<RenderBlock*>;
        gPercentHeightContainerMap->set(descendant, containerSet);
    }
    ASSERT(!containerSet->contains(this));
    containerSet->add(this);
}

void RenderBlock::removePercentHeightDescendant(RenderBox* descendant)
{
    if (!gPercentHeightContainerMap)
        return;

    HashSet<RenderBlock*>* containerSet = gPercentHeightContainerMap->take(descendant);
    if (!containerSet)
        return;

    HashSet<RenderBlock*>::iterator end = containerSet->end();
    for (HashSet<RenderBlock*>::iterator it = containerSet->begin(); it != end; ++it) {
        RenderBlock* container = *it;
        HashSet<RenderBox*>* descendantSet = gPercentHeightDescendantsMap->get(container);
        ASSERT(descendantSet);
        if (!descendantSet)
            continue;
        ASSERT(descendantSet->contains(descendant));
        descendantSet->remove(descendant);
        if (descendantSet->isEmpty()) {
            gPercentHeightDescendantsMap->remove(container);
            delete descendantSet;
        }
    }

    delete containerSet;
}

HashSet<RenderBox*>* RenderBlock::percentHeightDescendants() const
{
    return gPercentHeightDescendantsMap ? gPercentHeightDescendantsMap->get(this) : 0;
}

// FIXME: The logicalLeftOffsetForLine/logicalRightOffsetForLine functions are very slow if there are many floats
// present. We need to add a structure to floating objects to represent "lines" of floats.  Then instead of checking
// each float individually, we'd just walk backwards through the "lines" and stop when we hit a line that is fully above
// the vertical offset that we'd like to check.  Computing the "lines" would be rather complicated, but could replace the left
// objects and right objects count hack that is currently used here.
int RenderBlock::logicalLeftOffsetForLine(int logicalTop, int fixedOffset, bool applyTextIndent, int* heightRemaining) const
{
    int left = fixedOffset;
    if (m_floatingObjects && m_floatingObjects->hasLeftObjects()) {
        if (heightRemaining)
            *heightRemaining = 1;

        // We know the list is non-empty, since we have "left" objects to search for.
        // Therefore we can assume that begin != end, and that we can do at least one
        // decrement.
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator begin = floatingObjectSet.begin();
        FloatingObjectSetIterator it = floatingObjectSet.end();
        do {
            --it;
            FloatingObject* r = *it;
            if (r->isPlaced() && logicalTopForFloat(r) <= logicalTop && logicalBottomForFloat(r) > logicalTop
                && r->type() == FloatingObject::FloatLeft
                && logicalRightForFloat(r) > left) {
                left = max(left, logicalRightForFloat(r));
                if (heightRemaining)
                    *heightRemaining = logicalBottomForFloat(r) - logicalTop;
            }
        } while (it != begin);
    }

    if (applyTextIndent && style()->isLeftToRightDirection()) {
        int cw = 0;
        if (style()->textIndent().isPercent())
            cw = containingBlock()->availableLogicalWidth();
        left += style()->textIndent().calcMinValue(cw);
    }

    return left;
}

int RenderBlock::logicalRightOffsetForLine(int logicalTop, int fixedOffset, bool applyTextIndent, int* heightRemaining) const
{
    int right = fixedOffset;

    if (m_floatingObjects && m_floatingObjects->hasRightObjects()) {
        if (heightRemaining)
            *heightRemaining = 1;
            
        // We know the list is non-empty, since we have "right" objects to search for.
        // Therefore we can assume that begin != end, and that we can do at least one
        // decrement.
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator begin = floatingObjectSet.begin();
        FloatingObjectSetIterator it = floatingObjectSet.end();
        do {
            --it;
            FloatingObject* r = *it;
            if (r->isPlaced() && logicalTopForFloat(r) <= logicalTop && logicalBottomForFloat(r) > logicalTop
                && r->type() == FloatingObject::FloatRight
                && logicalLeftForFloat(r) < right) {
                right = min(right, logicalLeftForFloat(r));
                if (heightRemaining)
                    *heightRemaining = logicalBottomForFloat(r) - logicalTop;
            }
        } while (it != begin);
    }
    
    if (applyTextIndent && !style()->isLeftToRightDirection()) {
        int cw = 0;
        if (style()->textIndent().isPercent())
            cw = containingBlock()->availableLogicalWidth();
        right -= style()->textIndent().calcMinValue(cw);
    }
    
    return right;
}

int RenderBlock::availableLogicalWidthForLine(int position, bool firstLine) const
{
    int result = logicalRightOffsetForLine(position, firstLine) - logicalLeftOffsetForLine(position, firstLine);
    return (result < 0) ? 0 : result;
}

int RenderBlock::nextFloatLogicalBottomBelow(int logicalHeight) const
{
    if (!m_floatingObjects)
        return 0;

    int bottom = INT_MAX;
    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        int floatBottom = logicalBottomForFloat(r);
        if (floatBottom > logicalHeight)
            bottom = min(floatBottom, bottom);
    }

    return bottom == INT_MAX ? 0 : bottom;
}

int RenderBlock::lowestFloatLogicalBottom(FloatingObject::Type floatType) const
{
    if (!m_floatingObjects)
        return 0;
    int lowestFloatBottom = 0;
    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* r = *it;
        if (r->isPlaced() && r->type() & floatType)
            lowestFloatBottom = max(lowestFloatBottom, logicalBottomForFloat(r));
    }
    return lowestFloatBottom;
}

void RenderBlock::markLinesDirtyInBlockRange(int logicalTop, int logicalBottom, RootInlineBox* highest)
{
    if (logicalTop >= logicalBottom)
        return;

    RootInlineBox* lowestDirtyLine = lastRootBox();
    RootInlineBox* afterLowest = lowestDirtyLine;
    while (lowestDirtyLine && lowestDirtyLine->blockLogicalHeight() >= logicalBottom && logicalBottom < numeric_limits<int>::max()) {
        afterLowest = lowestDirtyLine;
        lowestDirtyLine = lowestDirtyLine->prevRootBox();
    }

    while (afterLowest && afterLowest != highest && (afterLowest->blockLogicalHeight() >= logicalTop || afterLowest->blockLogicalHeight() < 0)) {
        afterLowest->markDirty();
        afterLowest = afterLowest->prevRootBox();
    }
}

void RenderBlock::clearFloats()
{
    // Inline blocks are covered by the isReplaced() check in the avoidFloats method.
    if (avoidsFloats() || isRoot() || isRenderView() || isFloatingOrPositioned() || isTableCell()) {
        if (m_floatingObjects) {
            deleteAllValues(m_floatingObjects->set());
            m_floatingObjects->clear();
        }
        return;
    }

    typedef HashMap<RenderObject*, FloatingObject*> RendererToFloatInfoMap;
    RendererToFloatInfoMap floatMap;

    if (m_floatingObjects) {
        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        if (childrenInline()) {
            FloatingObjectSet::iterator end = floatingObjectSet.end();
            for (FloatingObjectSet::iterator it = floatingObjectSet.begin(); it != end; ++it) {
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
    bool parentHasFloats = false;
    RenderBlock* parentBlock = toRenderBlock(parent());
    RenderObject* prev = previousSibling();
    while (prev && (prev->isFloatingOrPositioned() || !prev->isBox() || !prev->isRenderBlock() || toRenderBlock(prev)->avoidsFloats())) {
        if (prev->isFloating())
            parentHasFloats = true;
         prev = prev->previousSibling();
    }

    // First add in floats from the parent.
    int logicalTopOffset = logicalTop();
    if (parentHasFloats)
        addIntrudingFloats(parentBlock, parentBlock->logicalLeftOffsetForContent(), logicalTopOffset);
    
    int logicalLeftOffset = 0;
    if (prev)
        logicalTopOffset -= toRenderBox(prev)->logicalTop();
    else {
        prev = parentBlock;
        logicalLeftOffset += parentBlock->logicalLeftOffsetForContent();
    }

    // Add overhanging floats from the previous RenderBlock, but only if it has a float that intrudes into our space.
    if (!prev || !prev->isRenderBlock())
        return;
    
    RenderBlock* block = toRenderBlock(prev);
    if (block->m_floatingObjects && block->lowestFloatLogicalBottom() > logicalTopOffset)
        addIntrudingFloats(block, logicalLeftOffset, logicalTopOffset);

    if (childrenInline()) {
        int changeLogicalTop = numeric_limits<int>::max();
        int changeLogicalBottom = numeric_limits<int>::min();
        if (m_floatingObjects) {
            FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* f = *it;
                FloatingObject* oldFloatingObject = floatMap.get(f->m_renderer);
                int logicalBottom = logicalBottomForFloat(f);
                if (oldFloatingObject) {
                    int oldLogicalBottom = logicalBottomForFloat(oldFloatingObject);
                    if (logicalWidthForFloat(f) != logicalWidthForFloat(oldFloatingObject) || logicalLeftForFloat(f) != logicalLeftForFloat(oldFloatingObject)) {
                        changeLogicalTop = 0;
                        changeLogicalBottom = max(changeLogicalBottom, max(logicalBottom, oldLogicalBottom));
                    } else if (logicalBottom != oldLogicalBottom) {
                        changeLogicalTop = min(changeLogicalTop, min(logicalBottom, oldLogicalBottom));
                        changeLogicalBottom = max(changeLogicalBottom, max(logicalBottom, oldLogicalBottom));
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
            FloatingObject* floatingObject = (*it).second;
            if (!floatingObject->m_isDescendant) {
                changeLogicalTop = 0;
                changeLogicalBottom = max(changeLogicalBottom, logicalBottomForFloat(floatingObject));
            }
        }
        deleteAllValues(floatMap);

        markLinesDirtyInBlockRange(changeLogicalTop, changeLogicalBottom);
    }
}

int RenderBlock::addOverhangingFloats(RenderBlock* child, int logicalLeftOffset, int logicalTopOffset, bool makeChildPaintOtherFloats)
{
    // Prevent floats from being added to the canvas by the root element, e.g., <html>.
    if (child->hasOverflowClip() || !child->containsFloats() || child->isRoot() || child->hasColumns() || child->isWritingModeRoot())
        return 0;

    int childLogicalTop = child->logicalTop();
    int lowestFloatLogicalBottom = 0;

    // Floats that will remain the child's responsibility to paint should factor into its
    // overflow.
    FloatingObjectSetIterator childEnd = child->m_floatingObjects->set().end();
    for (FloatingObjectSetIterator childIt = child->m_floatingObjects->set().begin(); childIt != childEnd; ++childIt) {
        FloatingObject* r = *childIt;
        int logicalBottomForFloat = min(this->logicalBottomForFloat(r), numeric_limits<int>::max() - childLogicalTop);
        int logicalBottom = childLogicalTop + logicalBottomForFloat;
        lowestFloatLogicalBottom = max(lowestFloatLogicalBottom, logicalBottom);

        if (logicalBottom > logicalHeight()) {
            // If the object is not in the list, we add it now.
            if (!containsFloat(r->m_renderer)) {
                int leftOffset = isHorizontalWritingMode() ? logicalLeftOffset : logicalTopOffset;
                int topOffset = isHorizontalWritingMode() ? logicalTopOffset : logicalLeftOffset;
                FloatingObject* floatingObj = new FloatingObject(r->type(), IntRect(r->x() - leftOffset, r->y() - topOffset, r->width(), r->height()));
                floatingObj->m_renderer = r->m_renderer;

                // The nearest enclosing layer always paints the float (so that zindex and stacking
                // behaves properly).  We always want to propagate the desire to paint the float as
                // far out as we can, to the outermost block that overlaps the float, stopping only
                // if we hit a self-painting layer boundary.
                if (r->m_renderer->enclosingFloatPaintingLayer() == enclosingFloatPaintingLayer())
                    r->m_shouldPaint = false;
                else
                    floatingObj->m_shouldPaint = false;
                
                floatingObj->m_isDescendant = true;

                // We create the floating object list lazily.
                if (!m_floatingObjects)
                    m_floatingObjects = adoptPtr(new FloatingObjects);

                m_floatingObjects->increaseObjectsCount(floatingObj->type());
                m_floatingObjects->set().add(floatingObj);
            }
        } else {
            if (makeChildPaintOtherFloats && !r->m_shouldPaint && !r->m_renderer->hasSelfPaintingLayer() &&
                r->m_renderer->isDescendantOf(child) && r->m_renderer->enclosingFloatPaintingLayer() == child->enclosingFloatPaintingLayer()) {
                // The float is not overhanging from this block, so if it is a descendant of the child, the child should
                // paint it (the other case is that it is intruding into the child), unless it has its own layer or enclosing
                // layer.
                // If makeChildPaintOtherFloats is false, it means that the child must already know about all the floats
                // it should paint.
                r->m_shouldPaint = true;
            }
            
            // Since the float doesn't overhang, it didn't get put into our list.  We need to go ahead and add its overflow in to the
            // child now.
            if (r->m_isDescendant)
                child->addOverflowFromChild(r->m_renderer, IntSize(xPositionForFloatIncludingMargin(r), yPositionForFloatIncludingMargin(r)));
        }
    }
    return lowestFloatLogicalBottom;
}

bool RenderBlock::hasOverhangingFloat(RenderBox* renderer)
{
    if (!m_floatingObjects || hasColumns() || !parent())
        return false;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator it = floatingObjectSet.find<RenderBox*, FloatingObjectHashTranslator>(renderer);
    if (it == floatingObjectSet.end())
        return false;

    return logicalBottomForFloat(*it) > logicalHeight();
}

void RenderBlock::addIntrudingFloats(RenderBlock* prev, int logicalLeftOffset, int logicalTopOffset)
{
    // If the parent or previous sibling doesn't have any floats to add, don't bother.
    if (!prev->m_floatingObjects)
        return;

    logicalLeftOffset += (isHorizontalWritingMode() ? marginLeft() : marginTop());

    FloatingObjectSet& prevSet = prev->m_floatingObjects->set();
    FloatingObjectSetIterator prevEnd = prevSet.end();
    for (FloatingObjectSetIterator prevIt = prevSet.begin(); prevIt != prevEnd; ++prevIt) {
        FloatingObject* r = *prevIt;
        if (logicalBottomForFloat(r) > logicalTopOffset) {
            if (!m_floatingObjects || !m_floatingObjects->set().contains(r)) {
                int leftOffset = isHorizontalWritingMode() ? logicalLeftOffset : logicalTopOffset;
                int topOffset = isHorizontalWritingMode() ? logicalTopOffset : logicalLeftOffset;
                
                FloatingObject* floatingObj = new FloatingObject(r->type(), IntRect(r->x() - leftOffset, r->y() - topOffset, r->width(), r->height()));

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
               
                floatingObj->m_shouldPaint = false;  // We are not in the direct inheritance chain for this float. We will never paint it.
                floatingObj->m_renderer = r->m_renderer;
                
                // We create the floating object list lazily.
                if (!m_floatingObjects)
                    m_floatingObjects = adoptPtr(new FloatingObjects);
                m_floatingObjects->increaseObjectsCount(floatingObj->type());
                m_floatingObjects->set().add(floatingObj);
            }
        }
    }
}

bool RenderBlock::avoidsFloats() const
{
    // Floats can't intrude into our box if we have a non-auto column count or width.
    return RenderBox::avoidsFloats() || !style()->hasAutoColumnCount() || !style()->hasAutoColumnWidth();
}

bool RenderBlock::containsFloat(RenderBox* renderer)
{
    return m_floatingObjects && m_floatingObjects->set().contains<RenderBox*, FloatingObjectHashTranslator>(renderer);
}

void RenderBlock::markAllDescendantsWithFloatsForLayout(RenderBox* floatToRemove, bool inLayout)
{
    if (!m_everHadLayout)
        return;

    setChildNeedsLayout(true, !inLayout);

    if (floatToRemove)
        removeFloatingObject(floatToRemove);

    // Iterate over our children and mark them as needed.
    if (!childrenInline()) {
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if ((!floatToRemove && child->isFloatingOrPositioned()) || !child->isRenderBlock())
                continue;
            RenderBlock* childBlock = toRenderBlock(child);
            if ((floatToRemove ? childBlock->containsFloat(floatToRemove) : childBlock->containsFloats()) || childBlock->shrinkToAvoidFloats())
                childBlock->markAllDescendantsWithFloatsForLayout(floatToRemove, inLayout);
        }
    }
}

void RenderBlock::markSiblingsWithFloatsForLayout()
{
    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        if (logicalBottomForFloat(*it) > logicalHeight()) {
            RenderBox* floatingBox = (*it)->renderer();

            RenderObject* next = nextSibling();
            while (next) {
                if (next->isRenderBlock() && !next->isFloatingOrPositioned() && !toRenderBlock(next)->avoidsFloats()) {
                    RenderBlock* nextBlock = toRenderBlock(next);
                    if (nextBlock->containsFloat(floatingBox))
                        nextBlock->markAllDescendantsWithFloatsForLayout(floatingBox);
                    else
                        break;
                }

                next = next->nextSibling();
            }
        }
    }
}

int RenderBlock::getClearDelta(RenderBox* child, int yPos)
{
    // There is no need to compute clearance if we have no floats.
    if (!containsFloats())
        return 0;
    
    // At least one float is present.  We need to perform the clearance computation.
    bool clearSet = child->style()->clear() != CNONE;
    int bottom = 0;
    switch (child->style()->clear()) {
        case CNONE:
            break;
        case CLEFT:
            bottom = lowestFloatLogicalBottom(FloatingObject::FloatLeft);
            break;
        case CRIGHT:
            bottom = lowestFloatLogicalBottom(FloatingObject::FloatRight);
            break;
        case CBOTH:
            bottom = lowestFloatLogicalBottom();
            break;
    }

    // We also clear floats if we are too big to sit on the same line as a float (and wish to avoid floats by default).
    int result = clearSet ? max(0, bottom - yPos) : 0;
    if (!result && child->avoidsFloats()) {
        int y = yPos;
        while (true) {
            int widthAtY = availableLogicalWidthForLine(y, false);
            if (widthAtY == availableLogicalWidth())
                return y - yPos;

            int oldChildY = child->y();
            int oldChildWidth = child->width();
            child->setY(y);
            child->computeLogicalWidth();
            int childWidthAtY = child->width();
            child->setY(oldChildY);
            child->setWidth(oldChildWidth);

            if (childWidthAtY <= widthAtY)
                return y - yPos;

            y = nextFloatLogicalBottomBelow(y);
            ASSERT(y >= yPos);
            if (y < yPos)
                break;
        }
        ASSERT_NOT_REACHED();
    }
    return result;
}

bool RenderBlock::isPointInOverflowControl(HitTestResult& result, int _x, int _y, int _tx, int _ty)
{
    if (!scrollsOverflow())
        return false;

    return layer()->hitTestOverflowControls(result, IntPoint(_x - _tx, _y - _ty));
}

bool RenderBlock::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction)
{
    int tx = _tx + x();
    int ty = _ty + y();

    if (!isRenderView()) {
        // Check if we need to do anything at all.
        IntRect overflowBox = visualOverflowRect();
        overflowBox.move(tx, ty);
        if (!overflowBox.intersects(result.rectForPoint(_x, _y)))
            return false;
    }

    if ((hitTestAction == HitTestBlockBackground || hitTestAction == HitTestChildBlockBackground) && isPointInOverflowControl(result, _x, _y, tx, ty)) {
        updateHitTestResult(result, IntPoint(_x - tx, _y - ty));
        // FIXME: isPointInOverflowControl() doesn't handle rect-based tests yet.
        if (!result.addNodeToRectBasedTestResult(node(), _x, _y))
           return true;
    }

    // If we have clipping, then we can't have any spillout.
    bool useOverflowClip = hasOverflowClip() && !hasSelfPaintingLayer();
    bool useClip = (hasControlClip() || useOverflowClip);
    IntRect hitTestArea(result.rectForPoint(_x, _y));
    bool checkChildren = !useClip || (hasControlClip() ? controlClipRect(tx, ty).intersects(hitTestArea) : overflowClipRect(tx, ty, IncludeOverlayScrollbarSize).intersects(hitTestArea));
    if (checkChildren) {
        // Hit test descendants first.
        int scrolledX = tx;
        int scrolledY = ty;
        if (hasOverflowClip()) {
            IntSize offset = layer()->scrolledContentOffset();
            scrolledX -= offset.width();
            scrolledY -= offset.height();
        }

        // Hit test contents if we don't have columns.
        if (!hasColumns()) {
            if (hitTestContents(request, result, _x, _y, scrolledX, scrolledY, hitTestAction)) {
                updateHitTestResult(result, IntPoint(_x - tx, _y - ty));
                return true;
            }
            if (hitTestAction == HitTestFloat && hitTestFloats(request, result, _x, _y, scrolledX, scrolledY))
                return true;
        } else if (hitTestColumns(request, result, _x, _y, scrolledX, scrolledY, hitTestAction)) {
            updateHitTestResult(result, IntPoint(_x - tx, _y - ty));
            return true;
        }
    }

    // Now hit test our background
    if (hitTestAction == HitTestBlockBackground || hitTestAction == HitTestChildBlockBackground) {
        IntRect boundsRect(tx, ty, width(), height());
        if (visibleToHitTesting() && boundsRect.intersects(result.rectForPoint(_x, _y))) {
            updateHitTestResult(result, flipForWritingMode(IntPoint(_x - tx, _y - ty)));
            if (!result.addNodeToRectBasedTestResult(node(), _x, _y, boundsRect))
                return true;
        }
    }

    return false;
}

bool RenderBlock::hitTestFloats(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty)
{
    if (!m_floatingObjects)
        return false;

    if (isRenderView()) {
        tx += toRenderView(this)->frameView()->scrollX();
        ty += toRenderView(this)->frameView()->scrollY();
    }

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    for (FloatingObjectSetIterator it = floatingObjectSet.end(); it != begin;) {
        --it;
        FloatingObject* floatingObject = *it;
        if (floatingObject->m_shouldPaint && !floatingObject->m_renderer->hasSelfPaintingLayer()) {
            int xOffset = xPositionForFloatIncludingMargin(floatingObject) - floatingObject->m_renderer->x();
            int yOffset = yPositionForFloatIncludingMargin(floatingObject) - floatingObject->m_renderer->y();
            IntPoint childPoint = flipFloatForWritingMode(floatingObject, IntPoint(tx + xOffset, ty + yOffset));
            if (floatingObject->m_renderer->hitTest(request, result, IntPoint(x, y), childPoint.x(), childPoint.y())) {
                updateHitTestResult(result, IntPoint(x - childPoint.x(), y - childPoint.y()));
                return true;
            }
        }
    }

    return false;
}

bool RenderBlock::hitTestColumns(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    // We need to do multiple passes, breaking up our hit testing into strips.
    ColumnInfo* colInfo = columnInfo();
    int colCount = columnCount(colInfo);
    if (!colCount)
        return false;
    int logicalLeft = logicalLeftOffsetForContent();
    int currLogicalTopOffset = 0;
    int i;
    bool isHorizontal = isHorizontalWritingMode();
    for (i = 0; i < colCount; i++) {
        IntRect colRect = columnRectAt(colInfo, i);
        int blockDelta =  (isHorizontal ? colRect.height() : colRect.width());
        if (style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset += blockDelta;
        else
            currLogicalTopOffset -= blockDelta;
    }
    for (i = colCount - 1; i >= 0; i--) {
        IntRect colRect = columnRectAt(colInfo, i);
        flipForWritingMode(colRect);
        int currLogicalLeftOffset = (isHorizontal ? colRect.x() : colRect.y()) - logicalLeft;
        int blockDelta =  (isHorizontal ? colRect.height() : colRect.width());
        if (style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset -= blockDelta;
        else
            currLogicalTopOffset += blockDelta;
        colRect.move(tx, ty);
        
        if (colRect.intersects(result.rectForPoint(x, y))) {
            // The point is inside this column.
            // Adjust tx and ty to change where we hit test.
        
            IntSize offset = isHorizontal ? IntSize(currLogicalLeftOffset, currLogicalTopOffset) : IntSize(currLogicalTopOffset, currLogicalLeftOffset);
            int finalX = tx + offset.width();
            int finalY = ty + offset.height();
            if (result.isRectBasedTest() && !colRect.contains(result.rectForPoint(x, y)))
                hitTestContents(request, result, x, y, finalX, finalY, hitTestAction);
            else
                return hitTestContents(request, result, x, y, finalX, finalY, hitTestAction) || (hitTestAction == HitTestFloat && hitTestFloats(request, result, x, y, finalX, finalY));
        }
    }

    return false;
}

bool RenderBlock::hitTestContents(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    if (childrenInline() && !isTable()) {
        // We have to hit-test our line boxes.
        if (m_lineBoxes.hitTest(this, request, result, x, y, tx, ty, hitTestAction))
            return true;
    } else {
        // Hit test our children.
        HitTestAction childHitTest = hitTestAction;
        if (hitTestAction == HitTestChildBlockBackgrounds)
            childHitTest = HitTestChildBlockBackground;
        for (RenderBox* child = lastChildBox(); child; child = child->previousSiblingBox()) {
            IntPoint childPoint = flipForWritingMode(child, IntPoint(tx, ty), ParentToChildFlippingAdjustment);
            if (!child->hasSelfPaintingLayer() && !child->isFloating() && child->nodeAtPoint(request, result, x, y, childPoint.x(), childPoint.y(), childHitTest))
                return true;
        }
    }
    
    return false;
}

Position RenderBlock::positionForBox(InlineBox *box, bool start) const
{
    if (!box)
        return Position();

    if (!box->renderer()->node())
        return Position(node(), start ? caretMinOffset() : caretMaxOffset());

    if (!box->isInlineTextBox())
        return Position(box->renderer()->node(), start ? box->renderer()->caretMinOffset() : box->renderer()->caretMaxOffset());

    InlineTextBox *textBox = static_cast<InlineTextBox *>(box);
    return Position(box->renderer()->node(), start ? textBox->start() : textBox->start() + textBox->len());
}

// FIXME: This function should go on RenderObject as an instance method. Then
// all cases in which positionForPoint recurs could call this instead to
// prevent crossing editable boundaries. This would require many tests.
static VisiblePosition positionForPointRespectingEditingBoundaries(RenderBlock* parent, RenderBox* child, const IntPoint& pointInParentCoordinates)
{
    // FIXME: This is wrong if the child's writing-mode is different from the parent's.
    IntPoint pointInChildCoordinates(pointInParentCoordinates - child->location());

    // If this is an anonymous renderer, we just recur normally
    Node* childNode = child->node();
    if (!childNode)
        return child->positionForPoint(pointInChildCoordinates);

    // Otherwise, first make sure that the editability of the parent and child agree.
    // If they don't agree, then we return a visible position just before or after the child
    RenderObject* ancestor = parent;
    while (ancestor && !ancestor->node())
        ancestor = ancestor->parent();

    // If we can't find an ancestor to check editability on, or editability is unchanged, we recur like normal
    if (!ancestor || ancestor->node()->rendererIsEditable() == childNode->rendererIsEditable())
        return child->positionForPoint(pointInChildCoordinates);

    // Otherwise return before or after the child, depending on if the click was to the logical left or logical right of the child
    int childMiddle = parent->logicalWidthForChild(child) / 2;
    int logicalLeft = parent->isHorizontalWritingMode() ? pointInChildCoordinates.x() : pointInChildCoordinates.y();
    if (logicalLeft < childMiddle)
        return ancestor->createVisiblePosition(childNode->nodeIndex(), DOWNSTREAM);
    return ancestor->createVisiblePosition(childNode->nodeIndex() + 1, UPSTREAM);
}

VisiblePosition RenderBlock::positionForPointWithInlineChildren(const IntPoint& pointInLogicalContents)
{
    ASSERT(childrenInline());

    if (!firstRootBox())
        return createVisiblePosition(0, DOWNSTREAM);

    // look for the closest line box in the root box which is at the passed-in y coordinate
    InlineBox* closestBox = 0;
    RootInlineBox* firstRootBoxWithChildren = 0;
    RootInlineBox* lastRootBoxWithChildren = 0;
    for (RootInlineBox* root = firstRootBox(); root; root = root->nextRootBox()) {
        if (!root->firstLeafChild())
            continue;
        if (!firstRootBoxWithChildren)
            firstRootBoxWithChildren = root;
        lastRootBoxWithChildren = root;

        // check if this root line box is located at this y coordinate
        if (pointInLogicalContents.y() < root->selectionBottom()) {
            closestBox = root->closestLeafChildForLogicalLeftPosition(pointInLogicalContents.x());
            if (closestBox)
                break;
        }
    }

    bool moveCaretToBoundary = document()->frame()->editor()->behavior().shouldMoveCaretToHorizontalBoundaryWhenPastTopOrBottom();

    if (!moveCaretToBoundary && !closestBox && lastRootBoxWithChildren) {
        // y coordinate is below last root line box, pretend we hit it
        closestBox = lastRootBoxWithChildren->closestLeafChildForLogicalLeftPosition(pointInLogicalContents.x());
    }

    if (closestBox) {
        if (moveCaretToBoundary && pointInLogicalContents.y() < firstRootBoxWithChildren->selectionTop()
            && pointInLogicalContents.y() < firstRootBoxWithChildren->logicalTop()) {
            // y coordinate is above first root line box, so return the start of the first
            return VisiblePosition(positionForBox(firstRootBoxWithChildren->firstLeafChild(), true), DOWNSTREAM);
        }

        // pass the box a top position that is inside it
        IntPoint point(pointInLogicalContents.x(), closestBox->logicalTop());
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
    return box->height() && box->style()->visibility() == VISIBLE && !box->isFloatingOrPositioned();
}

VisiblePosition RenderBlock::positionForPoint(const IntPoint& point)
{
    if (isTable())
        return RenderBox::positionForPoint(point);

    if (isReplaced()) {
        // FIXME: This seems wrong when the object's writing-mode doesn't match the line's writing-mode.
        int pointLogicalLeft = isHorizontalWritingMode() ? point.x() : point.y();
        int pointLogicalTop = isHorizontalWritingMode() ? point.y() : point.x();

        if (pointLogicalTop < 0 || (pointLogicalTop < logicalHeight() && pointLogicalLeft < 0))
            return createVisiblePosition(caretMinOffset(), DOWNSTREAM);
        if (pointLogicalTop >= logicalHeight() || (pointLogicalTop >= 0 && pointLogicalLeft >= logicalWidth()))
            return createVisiblePosition(caretMaxOffset(), DOWNSTREAM);
    } 

    int contentsX = point.x();
    int contentsY = point.y();
    offsetForContents(contentsX, contentsY);
    IntPoint pointInContents(contentsX, contentsY);
    IntPoint pointInLogicalContents(pointInContents);
    if (!isHorizontalWritingMode())
        pointInLogicalContents = pointInLogicalContents.transposedPoint();

    if (childrenInline())
        return positionForPointWithInlineChildren(pointInLogicalContents);

    if (lastChildBox() && pointInContents.y() > lastChildBox()->logicalTop()) {
        for (RenderBox* childBox = lastChildBox(); childBox; childBox = childBox->previousSiblingBox()) {
            if (isChildHitTestCandidate(childBox))
                return positionForPointRespectingEditingBoundaries(this, childBox, pointInContents);
        }
    } else {
        for (RenderBox* childBox = firstChildBox(); childBox; childBox = childBox->nextSiblingBox()) {
            // We hit child if our click is above the bottom of its padding box (like IE6/7 and FF3).
            if (isChildHitTestCandidate(childBox) && pointInContents.y() < childBox->logicalBottom())
                return positionForPointRespectingEditingBoundaries(this, childBox, pointInContents);
        }
    }

    // We only get here if there are no hit test candidate children below the click.
    return RenderBox::positionForPoint(point);
}

void RenderBlock::offsetForContents(int& tx, int& ty) const
{
    IntPoint contentsPoint(tx, ty);

    if (hasOverflowClip())
        contentsPoint += layer()->scrolledContentOffset();

    if (hasColumns())
        adjustPointToColumnContents(contentsPoint);

    tx = contentsPoint.x();
    ty = contentsPoint.y();
}

int RenderBlock::availableLogicalWidth() const
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
    // Calculate our column width and column count.
    unsigned desiredColumnCount = 1;
    int desiredColumnWidth = contentLogicalWidth();
    
    // For now, we don't support multi-column layouts when printing, since we have to do a lot of work for proper pagination.
    if (document()->paginated() || (style()->hasAutoColumnCount() && style()->hasAutoColumnWidth())) {
        setDesiredColumnCountAndWidth(desiredColumnCount, desiredColumnWidth);
        return;
    }
        
    int availWidth = desiredColumnWidth;
    int colGap = columnGap();
    int colWidth = max(1, static_cast<int>(style()->columnWidth()));
    int colCount = max(1, static_cast<int>(style()->columnCount()));

    if (style()->hasAutoColumnWidth() && !style()->hasAutoColumnCount()) {
        desiredColumnCount = colCount;
        desiredColumnWidth = max<int>(0, (availWidth - ((desiredColumnCount - 1) * colGap)) / desiredColumnCount);
    } else if (!style()->hasAutoColumnWidth() && style()->hasAutoColumnCount()) {
        desiredColumnCount = max<int>(1, (float)(availWidth + colGap) / (colWidth + colGap));
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    } else {
        desiredColumnCount = max(min<int>(colCount, (float)(availWidth + colGap) / (colWidth + colGap)), 1);
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    }
    setDesiredColumnCountAndWidth(desiredColumnCount, desiredColumnWidth);
}

void RenderBlock::setDesiredColumnCountAndWidth(int count, int width)
{
    bool destroyColumns = !firstChild()
                          || (count == 1 && style()->hasAutoColumnWidth())
                          || firstChild()->isAnonymousColumnsBlock()
                          || firstChild()->isAnonymousColumnSpanBlock();
    if (destroyColumns) {
        if (hasColumns()) {
            delete gColumnInfoMap->take(this);
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
            gColumnInfoMap->add(this, info);
            setHasColumns(true);
        }
        info->setDesiredColumnCount(count);
        info->setDesiredColumnWidth(width);   
    }
}

int RenderBlock::desiredColumnWidth() const
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
    ASSERT(hasColumns() && gColumnInfoMap->get(this) == colInfo);
    return colInfo->columnCount();
}

IntRect RenderBlock::columnRectAt(ColumnInfo* colInfo, unsigned index) const
{
    ASSERT(hasColumns() && gColumnInfoMap->get(this) == colInfo);

    // Compute the appropriate rect based off our information.
    int colLogicalWidth = colInfo->desiredColumnWidth();
    int colLogicalHeight = colInfo->columnHeight();
    int colLogicalTop = borderBefore() + paddingBefore();
    int colGap = columnGap();
    int colLogicalLeft = style()->isLeftToRightDirection() ? 
                          logicalLeftOffsetForContent() + (index * (colLogicalWidth + colGap))
                        : logicalLeftOffsetForContent() + contentLogicalWidth() - colLogicalWidth - (index * (colLogicalWidth + colGap));
    IntRect rect(colLogicalLeft, colLogicalTop, colLogicalWidth, colLogicalHeight);
    if (isHorizontalWritingMode())
        return IntRect(colLogicalLeft, colLogicalTop, colLogicalWidth, colLogicalHeight);
    return IntRect(colLogicalTop, colLogicalLeft, colLogicalHeight, colLogicalWidth);
}

bool RenderBlock::layoutColumns(bool hasSpecifiedPageLogicalHeight, int pageLogicalHeight, LayoutStateMaintainer& statePusher)
{
    if (!hasColumns())
        return false;

    // FIXME: We don't balance properly at all in the presence of forced page breaks.  We need to understand what
    // the distance between forced page breaks is so that we can avoid making the minimum column height too tall.
    ColumnInfo* colInfo = columnInfo();
    int desiredColumnCount = colInfo->desiredColumnCount();
    if (!hasSpecifiedPageLogicalHeight) {
        int columnHeight = pageLogicalHeight;
        int minColumnCount = colInfo->forcedBreaks() + 1;
        if (minColumnCount >= desiredColumnCount) {
            // The forced page breaks are in control of the balancing.  Just set the column height to the
            // maximum page break distance.
            if (!pageLogicalHeight) {
                int distanceBetweenBreaks = max(colInfo->maximumDistanceBetweenForcedBreaks(),
                                                view()->layoutState()->pageLogicalOffset(borderBefore() + paddingBefore() + contentLogicalHeight()) - colInfo->forcedBreakOffset());
                columnHeight = max(colInfo->minimumColumnHeight(), distanceBetweenBreaks);
            }
        } else if (contentLogicalHeight() > pageLogicalHeight * desiredColumnCount) {
            // Now that we know the intrinsic height of the columns, we have to rebalance them.
            columnHeight = max(colInfo->minimumColumnHeight(), (int)ceilf((float)contentLogicalHeight() / desiredColumnCount));
        }
        
        if (columnHeight && columnHeight != pageLogicalHeight) {
            statePusher.pop();
            m_everHadLayout = true;
            layoutBlock(false, columnHeight);
            return true;
        }
    } 
    
    if (pageLogicalHeight)
        colInfo->setColumnCountAndHeight(ceilf((float)contentLogicalHeight() / pageLogicalHeight), pageLogicalHeight);

    if (columnCount(colInfo)) {
        setLogicalHeight(borderBefore() + paddingBefore() + colInfo->columnHeight() + borderAfter() + paddingAfter() + scrollbarLogicalHeight());
        m_overflow.clear();
    }
    
    return false;
}

void RenderBlock::adjustPointToColumnContents(IntPoint& point) const
{
    // Just bail if we have no columns.
    if (!hasColumns())
        return;
    
    ColumnInfo* colInfo = columnInfo();
    if (!columnCount(colInfo))
        return;

    // Determine which columns we intersect.
    int colGap = columnGap();
    int halfColGap = colGap / 2;
    IntPoint columnPoint(columnRectAt(colInfo, 0).location());
    int logicalOffset = 0;
    for (unsigned i = 0; i < colInfo->columnCount(); i++) {
        // Add in half the column gap to the left and right of the rect.
        IntRect colRect = columnRectAt(colInfo, i);
        if (isHorizontalWritingMode()) {
            IntRect gapAndColumnRect(colRect.x() - halfColGap, colRect.y(), colRect.width() + colGap, colRect.height());
            if (point.x() >= gapAndColumnRect.x() && point.x() < gapAndColumnRect.maxX()) {
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

                // We're inside the column.  Translate the x and y into our column coordinate space.
                point.move(columnPoint.x() - colRect.x(), logicalOffset);
                return;
            }
            
            // Move to the next position.
            logicalOffset += colRect.height();
        } else {
            IntRect gapAndColumnRect(colRect.x(), colRect.y() - halfColGap, colRect.width(), colRect.height() + colGap);
            if (point.y() >= gapAndColumnRect.y() && point.y() < gapAndColumnRect.maxY()) {
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

                // We're inside the column.  Translate the x and y into our column coordinate space.
                point.move(logicalOffset, columnPoint.y() - colRect.y());
                return;
            }
            
            // Move to the next position.
            logicalOffset += colRect.width();
        }
    }
}

void RenderBlock::adjustRectForColumns(IntRect& r) const
{
    // Just bail if we have no columns.
    if (!hasColumns())
        return;
    
    ColumnInfo* colInfo = columnInfo();

    // Begin with a result rect that is empty.
    IntRect result;
    
    // Determine which columns we intersect.
    unsigned colCount = columnCount(colInfo);
    if (!colCount)
        return;
    
    int logicalLeft = logicalLeftOffsetForContent();
    int currLogicalOffset = 0;

    for (unsigned i = 0; i < colCount; i++) {
        IntRect colRect = columnRectAt(colInfo, i);
        IntRect repaintRect = r;
        if (isHorizontalWritingMode()) {
            int currXOffset = colRect.x() - logicalLeft;
            repaintRect.move(currXOffset, currLogicalOffset);
            currLogicalOffset -= colRect.height();
        } else {
            int currYOffset = colRect.y() - logicalLeft;
            repaintRect.move(currLogicalOffset, currYOffset);
            currLogicalOffset -= colRect.width();
        }
        repaintRect.intersect(colRect);
        result.unite(repaintRect);
    }

    r = result;
}

IntPoint RenderBlock::flipForWritingModeIncludingColumns(const IntPoint& point) const
{
    ASSERT(hasColumns());
    if (!hasColumns() || !style()->isFlippedBlocksWritingMode())
        return point;
    ColumnInfo* colInfo = columnInfo();
    int columnLogicalHeight = colInfo->columnHeight();
    int expandedLogicalHeight = borderBefore() + paddingBefore() + columnCount(colInfo) * columnLogicalHeight + borderAfter() + paddingAfter() + scrollbarLogicalHeight();
    if (isHorizontalWritingMode())
        return IntPoint(point.x(), expandedLogicalHeight - point.y());
    return IntPoint(expandedLogicalHeight - point.x(), point.y());
}

void RenderBlock::flipForWritingModeIncludingColumns(IntRect& rect) const
{
    ASSERT(hasColumns());
    if (!hasColumns() || !style()->isFlippedBlocksWritingMode())
        return;
    
    ColumnInfo* colInfo = columnInfo();
    int columnLogicalHeight = colInfo->columnHeight();
    int expandedLogicalHeight = borderBefore() + paddingBefore() + columnCount(colInfo) * columnLogicalHeight + borderAfter() + paddingAfter() + scrollbarLogicalHeight();
    if (isHorizontalWritingMode())
        rect.setY(expandedLogicalHeight - rect.maxY());
    else
        rect.setX(expandedLogicalHeight - rect.maxX());
}

void RenderBlock::adjustForColumns(IntSize& offset, const IntPoint& point) const
{
    if (!hasColumns())
        return;

    ColumnInfo* colInfo = columnInfo();

    int logicalLeft = logicalLeftOffsetForContent();
    size_t colCount = columnCount(colInfo);
    int colLogicalWidth = colInfo->desiredColumnWidth();
    int colLogicalHeight = colInfo->columnHeight();

    for (size_t i = 0; i < colCount; ++i) {
        // Compute the edges for a given column in the block progression direction.
        IntRect sliceRect = IntRect(logicalLeft, borderBefore() + paddingBefore() + i * colLogicalHeight, colLogicalWidth, colLogicalHeight);
        if (!isHorizontalWritingMode())
            sliceRect = sliceRect.transposedRect();
        
        // If we have a flipped blocks writing mode, then convert the column so that it's coming from the after edge (either top or left edge).
        flipForWritingModeIncludingColumns(sliceRect);
        
        int logicalOffset = style()->isFlippedBlocksWritingMode() ? (colCount - 1 - i) * colLogicalHeight : i * colLogicalHeight;

        // Now we're in the same coordinate space as the point.  See if it is inside the rectangle.
        if (isHorizontalWritingMode()) {
            if (point.y() >= sliceRect.y() && point.y() < sliceRect.maxY()) {
                offset.expand(columnRectAt(colInfo, i).x() - logicalLeft, -logicalOffset);
                return;
            }
        } else {
            if (point.x() >= sliceRect.x() && point.x() < sliceRect.maxX()) {
                offset.expand(-logicalOffset, columnRectAt(colInfo, i).y() - logicalLeft);
                return;
            }
        }
    }
}

void RenderBlock::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    updateFirstLetter();

    if (!isTableCell() && style()->logicalWidth().isFixed() && style()->logicalWidth().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = computeContentBoxLogicalWidth(style()->logicalWidth().value());
    else {
        m_minPreferredLogicalWidth = 0;
        m_maxPreferredLogicalWidth = 0;

        if (childrenInline())
            computeInlinePreferredLogicalWidths();
        else
            computeBlockPreferredLogicalWidths();

        m_maxPreferredLogicalWidth = max(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

        if (!style()->autoWrap() && childrenInline()) {
            m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth;
            
            // A horizontal marquee with inline children has no minimum width.
            if (layer() && layer()->marquee() && layer()->marquee()->isHorizontal())
                m_minPreferredLogicalWidth = 0;
        }

        int scrollbarWidth = 0;
        if (hasOverflowClip() && style()->overflowY() == OSCROLL) {
            layer()->setHasVerticalScrollbar(true);
            scrollbarWidth = verticalScrollbarWidth();
            m_maxPreferredLogicalWidth += scrollbarWidth;
        }

        if (isTableCell()) {
            Length w = toRenderTableCell(this)->styleOrColLogicalWidth();
            if (w.isFixed() && w.value() > 0) {
                m_maxPreferredLogicalWidth = max(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(w.value()));
                scrollbarWidth = 0;
            }
        }
        
        m_minPreferredLogicalWidth += scrollbarWidth;
    }
    
    if (style()->logicalMinWidth().isFixed() && style()->logicalMinWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->logicalMinWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->logicalMinWidth().value()));
    }
    
    if (style()->logicalMaxWidth().isFixed() && style()->logicalMaxWidth().value() != undefinedLength) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->logicalMaxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->logicalMaxWidth().value()));
    }

    int borderAndPadding = borderAndPaddingLogicalWidth();
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
             (!current->isFloating() && !current->isReplaced() && !current->isPositioned())))
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

        if (!result->isPositioned() && (result->isText() || result->isFloating() || result->isReplaced() || result->isRenderInline()))
             break;
        
        current = result;
        result = 0;
    }

    // Update our position.
    current = result;
    return current;
}

static int getBPMWidth(int childValue, Length cssUnit)
{
    if (cssUnit.type() != Auto)
        return (cssUnit.isFixed() ? cssUnit.value() : childValue);
    return 0;
}

static int getBorderPaddingMargin(const RenderBoxModelObject* child, bool endOfInline)
{
    RenderStyle* cstyle = child->style();
    if (endOfInline)
        return getBPMWidth(child->marginEnd(), cstyle->marginEnd()) + 
               getBPMWidth(child->paddingEnd(), cstyle->paddingEnd()) +
               child->borderEnd();
    return getBPMWidth(child->marginStart(), cstyle->marginStart()) + 
               getBPMWidth(child->paddingStart(), cstyle->paddingStart()) +
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
        float spaceWidth = font.width(TextRun(&space, 1));
        inlineMax -= spaceWidth + font.wordSpacing();
        if (inlineMin > inlineMax)
            inlineMin = inlineMax;
    }
}

static inline void updatePreferredWidth(int& preferredWidth, float& result)
{
    int snappedResult = ceilf(result);
    preferredWidth = max(snappedResult, preferredWidth);
}

void RenderBlock::computeInlinePreferredLogicalWidths()
{
    float inlineMax = 0;
    float inlineMin = 0;

    int cw = containingBlock()->contentLogicalWidth();

    // If we are at the start of a line, we want to ignore all white-space.
    // Also strip spaces if we previously had text that ended in a trailing space.
    bool stripFrontSpaces = true;
    RenderObject* trailingSpaceChild = 0;

    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific cirucumstances (in order to match common WinIE renderings). 
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.) 
    bool allowImagesToBreak = !document()->inQuirksMode() || !isTableCell() || !style()->logicalWidth().isIntrinsicOrAuto();

    bool autoWrap, oldAutoWrap;
    autoWrap = oldAutoWrap = style()->autoWrap();

    InlineMinMaxIterator childIterator(this);
    bool addedTextIndent = false; // Only gets added in once.
    RenderObject* prevFloat = 0;
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
            RenderStyle* cstyle = child->style();
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
                    float margins = 0;
                    Length startMargin = cstyle->marginStart();
                    Length endMargin = cstyle->marginEnd();
                    if (startMargin.isFixed())
                        margins += startMargin.value();
                    if (endMargin.isFixed())
                        margins += endMargin.value();
                    childMin += margins;
                    childMax += margins;
                }
            }

            if (!child->isRenderInline() && !child->isText()) {
                // Case (2). Inline replaced elements and floats.
                // Go ahead and terminate the current line as far as
                // minwidth is concerned.
                childMin += child->minPreferredLogicalWidth();
                childMax += child->maxPreferredLogicalWidth();

                bool clearPreviousFloat;
                if (child->isFloating()) {
                    clearPreviousFloat = (prevFloat
                        && ((prevFloat->style()->floating() == FLEFT && (child->style()->clear() & CLEFT))
                            || (prevFloat->style()->floating() == FRIGHT && (child->style()->clear() & CRIGHT))));
                    prevFloat = child;
                } else
                    clearPreviousFloat = false;

                bool canBreakReplacedElement = !child->isImage() || allowImagesToBreak;
                if ((canBreakReplacedElement && (autoWrap || oldAutoWrap)) || clearPreviousFloat) {
                    updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                    inlineMin = 0;
                }

                // If we're supposed to clear the previous float, then terminate maxwidth as well.
                if (clearPreviousFloat) {
                    updatePreferredWidth(m_maxPreferredLogicalWidth, inlineMax);
                    inlineMax = 0;
                }

                // Add in text-indent.  This is added in only once.
                int ti = 0;
                if (!addedTextIndent) {
                    addedTextIndent = true;
                    ti = style()->textIndent().calcMinValue(cw);
                    childMin += ti;
                    childMax += ti;
                }

                // Add our width to the max.
                inlineMax += childMax;

                if (!autoWrap || !canBreakReplacedElement) {
                    if (child->isFloating())
                        updatePreferredWidth(m_minPreferredLogicalWidth, childMin);
                    else
                        inlineMin += childMin;
                } else {
                    // Now check our line.
                    updatePreferredWidth(m_minPreferredLogicalWidth, childMin);

                    // Now start a new line.
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
                    updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
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
                        updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                        inlineMin = 0;
                    }
                    continue;
                }
                
                if (stripFrontSpaces)
                    trailingSpaceChild = child;
                else
                    trailingSpaceChild = 0;

                // Add in text-indent.  This is added in only once.
                int ti = 0;
                if (!addedTextIndent) {
                    addedTextIndent = true;
                    ti = style()->textIndent().calcMinValue(cw);
                    childMin+=ti; beginMin += ti;
                    childMax+=ti; beginMax += ti;
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
                        updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                    else {
                        inlineMin += beginMin;
                        updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                        childMin -= ti;
                    }

                    inlineMin = childMin;

                    if (endWS) {
                        // We end in whitespace, which means we can go ahead
                        // and end our current line.
                        updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                        inlineMin = 0;
                    } else {
                        updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
                        inlineMin = endMin;
                    }
                }

                if (hasBreak) {
                    inlineMax += beginMax;
                    updatePreferredWidth(m_maxPreferredLogicalWidth, inlineMax);
                    updatePreferredWidth(m_maxPreferredLogicalWidth, childMax);
                    inlineMax = endMax;
                } else
                    inlineMax += childMax;
            }

            // Ignore spaces after a list marker.
            if (child->isListMarker())
                stripFrontSpaces = true;
        } else {
            updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
            updatePreferredWidth(m_maxPreferredLogicalWidth, inlineMax);
            inlineMin = inlineMax = 0;
            stripFrontSpaces = true;
            trailingSpaceChild = 0;
        }

        oldAutoWrap = autoWrap;
    }

    if (style()->collapseWhiteSpace())
        stripTrailingSpace(inlineMax, inlineMin, trailingSpaceChild);

    updatePreferredWidth(m_minPreferredLogicalWidth, inlineMin);
    updatePreferredWidth(m_maxPreferredLogicalWidth, inlineMax);
}

// Use a very large value (in effect infinite).
#define BLOCK_MAX_WIDTH 15000

void RenderBlock::computeBlockPreferredLogicalWidths()
{
    bool nowrap = style()->whiteSpace() == NOWRAP;

    RenderObject *child = firstChild();
    int floatLeftWidth = 0, floatRightWidth = 0;
    while (child) {
        // Positioned children don't affect the min/max width
        if (child->isPositioned()) {
            child = child->nextSibling();
            continue;
        }

        if (child->isFloating() || (child->isBox() && toRenderBox(child)->avoidsFloats())) {
            int floatTotalWidth = floatLeftWidth + floatRightWidth;
            if (child->style()->clear() & CLEFT) {
                m_maxPreferredLogicalWidth = max(floatTotalWidth, m_maxPreferredLogicalWidth);
                floatLeftWidth = 0;
            }
            if (child->style()->clear() & CRIGHT) {
                m_maxPreferredLogicalWidth = max(floatTotalWidth, m_maxPreferredLogicalWidth);
                floatRightWidth = 0;
            }
        }

        // A margin basically has three types: fixed, percentage, and auto (variable).
        // Auto and percentage margins simply become 0 when computing min/max width.
        // Fixed margins can be added in as is.
        Length startMarginLength = child->style()->marginStart();
        Length endMarginLength = child->style()->marginEnd();
        int margin = 0;
        int marginStart = 0;
        int marginEnd = 0;
        if (startMarginLength.isFixed())
            marginStart += startMarginLength.value();
        if (endMarginLength.isFixed())
            marginEnd += endMarginLength.value();
        margin = marginStart + marginEnd;

        int w = child->minPreferredLogicalWidth() + margin;
        m_minPreferredLogicalWidth = max(w, m_minPreferredLogicalWidth);
        
        // IE ignores tables for calculation of nowrap. Makes some sense.
        if (nowrap && !child->isTable())
            m_maxPreferredLogicalWidth = max(w, m_maxPreferredLogicalWidth);

        w = child->maxPreferredLogicalWidth() + margin;

        if (!child->isFloating()) {
            if (child->isBox() && toRenderBox(child)->avoidsFloats()) {
                // Determine a left and right max value based off whether or not the floats can fit in the
                // margins of the object.  For negative margins, we will attempt to overlap the float if the negative margin
                // is smaller than the float width.
                bool ltr = containingBlock()->style()->isLeftToRightDirection();
                int marginLogicalLeft = ltr ? marginStart : marginEnd;
                int marginLogicalRight = ltr ? marginEnd : marginStart;
                int maxLeft = marginLogicalLeft > 0 ? max(floatLeftWidth, marginLogicalLeft) : floatLeftWidth + marginLogicalLeft;
                int maxRight = marginLogicalRight > 0 ? max(floatRightWidth, marginLogicalRight) : floatRightWidth + marginLogicalRight;
                w = child->maxPreferredLogicalWidth() + maxLeft + maxRight;
                w = max(w, floatLeftWidth + floatRightWidth);
            }
            else
                m_maxPreferredLogicalWidth = max(floatLeftWidth + floatRightWidth, m_maxPreferredLogicalWidth);
            floatLeftWidth = floatRightWidth = 0;
        }
        
        if (child->isFloating()) {
            if (style()->floating() == FLEFT)
                floatLeftWidth += w;
            else
                floatRightWidth += w;
        } else
            m_maxPreferredLogicalWidth = max(w, m_maxPreferredLogicalWidth);

        // A very specific WinIE quirk.
        // Example:
        /*
           <div style="position:absolute; width:100px; top:50px;">
              <div style="position:absolute;left:0px;top:50px;height:50px;background-color:green">
                <table style="width:100%"><tr><td></table>
              </div>
           </div>
        */
        // In the above example, the inner absolute positioned block should have a computed width
        // of 100px because of the table.
        // We can achieve this effect by making the maxwidth of blocks that contain tables
        // with percentage widths be infinite (as long as they are not inside a table cell).
        if (document()->inQuirksMode() && child->style()->logicalWidth().isPercent() &&
            !isTableCell() && child->isTable() && m_maxPreferredLogicalWidth < BLOCK_MAX_WIDTH) {
            RenderBlock* cb = containingBlock();
            while (!cb->isRenderView() && !cb->isTableCell())
                cb = cb->containingBlock();
            if (!cb->isTableCell())
                m_maxPreferredLogicalWidth = BLOCK_MAX_WIDTH;
        }
        
        child = child->nextSibling();
    }

    // Always make sure these values are non-negative.
    m_minPreferredLogicalWidth = max(0, m_minPreferredLogicalWidth);
    m_maxPreferredLogicalWidth = max(0, m_maxPreferredLogicalWidth);

    m_maxPreferredLogicalWidth = max(floatLeftWidth + floatRightWidth, m_maxPreferredLogicalWidth);
}

bool RenderBlock::hasLineIfEmpty() const
{
    if (!node())
        return false;
    
    if (node()->rendererIsEditable() && node()->rootEditableElement() == node())
        return true;
    
    if (node()->isShadowRoot() && (node()->shadowHost()->hasTagName(inputTag)))
        return true;
    
    return false;
}

int RenderBlock::lineHeight(bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    // Inline blocks are replaced elements. Otherwise, just pass off to
    // the base class.  If we're being queried as though we're the root line
    // box, then the fact that we're an inline-block is irrelevant, and we behave
    // just like a block.
    if (isReplaced() && linePositionMode == PositionOnContainingLine)
        return RenderBox::lineHeight(firstLine, direction, linePositionMode);

    if (firstLine && document()->usesFirstLineRules()) {
        RenderStyle* s = style(firstLine);
        if (s != style())
            return s->computedLineHeight();
    }
    
    if (m_lineHeight == -1)
        m_lineHeight = style()->computedLineHeight();

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
        
        int baselinePos = ignoreBaseline ? -1 : lastLineBoxBaseline();
        
        int bottomOfContent = direction == HorizontalLine ? borderTop() + paddingTop() + contentHeight() : borderRight() + paddingRight() + contentWidth();
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
            if (!curr->isFloatingOrPositioned()) {
                int result = curr->firstLineBoxBaseline();
                if (result != -1)
                    return curr->logicalTop() + result; // Translate to our coordinate space.
            }
        }
    }

    return -1;
}

int RenderBlock::lastLineBoxBaseline() const
{
    if (!isBlockFlow() || (isWritingModeRoot() && !isRubyRun()))
        return -1;

    LineDirectionMode lineDirection = isHorizontalWritingMode() ? HorizontalLine : VerticalLine;

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
            if (!curr->isFloatingOrPositioned()) {
                haveNormalFlowChild = true;
                int result = curr->lastLineBoxBaseline();
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
        if (firstLineBlock->isReplaced() || firstLineBlock->isFloating() || 
            !parentBlock || parentBlock->firstChild() != firstLineBlock || !parentBlock->isBlockFlow())
            break;
        ASSERT(parentBlock->isRenderBlock());
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

void RenderBlock::updateFirstLetter()
{
    if (!document()->usesFirstLetterRules())
        return;
    // Don't recur
    if (style()->styleType() == FIRST_LETTER)
        return;

    // FIXME: We need to destroy the first-letter object if it is no longer the first child.  Need to find
    // an efficient way to check for that situation though before implementing anything.
    RenderObject* firstLetterBlock = this;
    bool hasPseudoStyle = false;
    while (true) {
        // We only honor first-letter if the firstLetterBlock can have children in the DOM. This correctly 
        // prevents form controls from honoring first-letter.
        hasPseudoStyle = firstLetterBlock->style()->hasPseudoStyle(FIRST_LETTER) 
            && firstLetterBlock->canHaveChildren();
        if (hasPseudoStyle)
            break;
        RenderObject* parentBlock = firstLetterBlock->parent();
        if (firstLetterBlock->isReplaced() || !parentBlock || parentBlock->firstChild() != firstLetterBlock || 
            !parentBlock->isBlockFlow())
            break;
        firstLetterBlock = parentBlock;
    } 

    if (!hasPseudoStyle) 
        return;

    // Drill into inlines looking for our first text child.
    RenderObject* currChild = firstLetterBlock->firstChild();
    while (currChild && ((!currChild->isReplaced() && !currChild->isRenderButton() && !currChild->isMenuList()) || currChild->isFloatingOrPositioned()) && !currChild->isText()) {
        if (currChild->isFloatingOrPositioned()) {
            if (currChild->style()->styleType() == FIRST_LETTER) {
                currChild = currChild->firstChild();
                break;
            } 
            currChild = currChild->nextSibling();
        } else
            currChild = currChild->firstChild();
    }

    // Get list markers out of the way.
    while (currChild && currChild->isListMarker())
        currChild = currChild->nextSibling();

    if (!currChild)
        return;

    // If the child already has style, then it has already been created, so we just want
    // to update it.
    if (currChild->parent()->style()->styleType() == FIRST_LETTER) {
        RenderObject* firstLetter = currChild->parent();
        RenderObject* firstLetterContainer = firstLetter->parent();
        RenderStyle* pseudoStyle = styleForFirstLetter(firstLetterBlock, firstLetterContainer);

        if (Node::diff(firstLetter->style(), pseudoStyle) == Node::Detach) {
            // The first-letter renderer needs to be replaced. Create a new renderer of the right type.
            RenderObject* newFirstLetter;
            if (pseudoStyle->display() == INLINE)
                newFirstLetter = new (renderArena()) RenderInline(document());
            else
                newFirstLetter = new (renderArena()) RenderBlock(document());
            newFirstLetter->setStyle(pseudoStyle);

            // Move the first letter into the new renderer.
            view()->disableLayoutState();
            while (RenderObject* child = firstLetter->firstChild()) {
                if (child->isText())
                    toRenderText(child)->removeAndDestroyTextBoxes();
                firstLetter->removeChild(child);
                newFirstLetter->addChild(child, 0);
            }
            
            RenderTextFragment* remainingText = 0;
            RenderObject* nextSibling = firstLetter->nextSibling();
            RenderObject* next = nextSibling;
            while (next) {
                if (next->isText() && toRenderText(next)->isTextFragment()) {
                    remainingText = toRenderTextFragment(next);
                    break;
                }
                next = next->nextSibling();
            }
            if (remainingText) {
                ASSERT(remainingText->node()->renderer() == remainingText);
                // Replace the old renderer with the new one.
                remainingText->setFirstLetter(newFirstLetter);
            }
            firstLetter->destroy();
            firstLetter = newFirstLetter;
            firstLetterContainer->addChild(firstLetter, nextSibling);
            view()->enableLayoutState();
        } else
            firstLetter->setStyle(pseudoStyle);

        for (RenderObject* genChild = firstLetter->firstChild(); genChild; genChild = genChild->nextSibling()) {
            if (genChild->isText()) 
                genChild->setStyle(pseudoStyle);
        }

        return;
    }

    if (!currChild->isText() || currChild->isBR())
        return;

    // If the child does not already have style, we create it here.
    RenderObject* firstLetterContainer = currChild->parent();

    // Our layout state is not valid for the repaints we are going to trigger by
    // adding and removing children of firstLetterContainer.
    view()->disableLayoutState();

    RenderText* textObj = toRenderText(currChild);

    // Create our pseudo style now that we have our firstLetterContainer determined.
    RenderStyle* pseudoStyle = styleForFirstLetter(firstLetterBlock, firstLetterContainer);

    RenderObject* firstLetter = 0;
    if (pseudoStyle->display() == INLINE)
        firstLetter = new (renderArena()) RenderInline(document());
    else
        firstLetter = new (renderArena()) RenderBlock(document());
    firstLetter->setStyle(pseudoStyle);
    firstLetterContainer->addChild(firstLetter, currChild);

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
        
        // construct text fragment for the first letter
        RenderTextFragment* letter = 
            new (renderArena()) RenderTextFragment(remainingText->node() ? remainingText->node() : remainingText->document(), oldText.get(), 0, length);
        letter->setStyle(pseudoStyle);
        firstLetter->addChild(letter);

        textObj->destroy();
    }
    view()->enableLayoutState();
}

// Helper methods for obtaining the last line, computing line counts and heights for line counts
// (crawling into blocks).
static bool shouldCheckLines(RenderObject* obj)
{
    return !obj->isFloatingOrPositioned() && !obj->isRunIn() &&
            obj->isBlockFlow() && obj->style()->height().isAuto() &&
            (!obj->isFlexibleBox() || obj->style()->boxOrient() == VERTICAL);
}

static RootInlineBox* getLineAtIndex(RenderBlock* block, int i, int& count)
{
    if (block->style()->visibility() == VISIBLE) {
        if (block->childrenInline()) {
            for (RootInlineBox* box = block->firstRootBox(); box; box = box->nextRootBox()) {
                if (count++ == i)
                    return box;
            }
        }
        else {
            for (RenderObject* obj = block->firstChild(); obj; obj = obj->nextSibling()) {
                if (shouldCheckLines(obj)) {
                    RootInlineBox *box = getLineAtIndex(toRenderBlock(obj), i, count);
                    if (box)
                        return box;
                }
            }
        }
    }
    return 0;
}

static int getHeightForLineCount(RenderBlock* block, int l, bool includeBottom, int& count)
{
    if (block->style()->visibility() == VISIBLE) {
        if (block->childrenInline()) {
            for (RootInlineBox* box = block->firstRootBox(); box; box = box->nextRootBox()) {
                if (++count == l)
                    return box->lineBottom() + (includeBottom ? (block->borderBottom() + block->paddingBottom()) : 0);
            }
        }
        else {
            RenderBox* normalFlowChildWithoutLines = 0;
            for (RenderBox* obj = block->firstChildBox(); obj; obj = obj->nextSiblingBox()) {
                if (shouldCheckLines(obj)) {
                    int result = getHeightForLineCount(toRenderBlock(obj), l, false, count);
                    if (result != -1)
                        return result + obj->y() + (includeBottom ? (block->borderBottom() + block->paddingBottom()) : 0);
                }
                else if (!obj->isFloatingOrPositioned() && !obj->isRunIn())
                    normalFlowChildWithoutLines = obj;
            }
            if (normalFlowChildWithoutLines && l == 0)
                return normalFlowChildWithoutLines->y() + normalFlowChildWithoutLines->height();
        }
    }
    
    return -1;
}

RootInlineBox* RenderBlock::lineAtIndex(int i)
{
    int count = 0;
    return getLineAtIndex(this, i, count);
}

int RenderBlock::lineCount()
{
    int count = 0;
    if (style()->visibility() == VISIBLE) {
        if (childrenInline())
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox())
                count++;
        else
            for (RenderObject* obj = firstChild(); obj; obj = obj->nextSibling())
                if (shouldCheckLines(obj))
                    count += toRenderBlock(obj)->lineCount();
    }
    return count;
}

int RenderBlock::heightForLineCount(int l)
{
    int count = 0;
    return getHeightForLineCount(this, l, true, count);
}

void RenderBlock::adjustForBorderFit(int x, int& left, int& right) const
{
    // We don't deal with relative positioning.  Our assumption is that you shrink to fit the lines without accounting
    // for either overflow or translations via relative positioning.
    if (style()->visibility() == VISIBLE) {
        if (childrenInline()) {
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox()) {
                if (box->firstChild())
                    left = min(left, x + static_cast<int>(box->firstChild()->x()));
                if (box->lastChild())
                    right = max(right, x + static_cast<int>(ceilf(box->lastChild()->logicalRight())));
            }
        }
        else {
            for (RenderBox* obj = firstChildBox(); obj; obj = obj->nextSiblingBox()) {
                if (!obj->isFloatingOrPositioned()) {
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
            FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                FloatingObject* r = *it;
                // Only examine the object if our m_shouldPaint flag is set.
                if (r->m_shouldPaint) {
                    int floatLeft = xPositionForFloatIncludingMargin(r) - r->m_renderer->x();
                    int floatRight = floatLeft + r->m_renderer->width();
                    left = min(left, floatLeft);
                    right = max(right, floatRight);
                }
            }
        }
    }
}

void RenderBlock::borderFitAdjust(int& x, int& w) const
{
    if (style()->borderFit() == BorderFitBorder)
        return;

    // Walk any normal flow lines to snugly fit.
    int left = INT_MAX;
    int right = INT_MIN;
    int oldWidth = w;
    adjustForBorderFit(0, left, right);
    if (left != INT_MAX) {
        left -= (borderLeft() + paddingLeft());
        if (left > 0) {
            x += left;
            w -= left;
        }
    }
    if (right != INT_MIN) {
        right += (borderRight() + paddingRight());
        if (right < oldWidth)
            w -= (oldWidth - right);
    }
}

void RenderBlock::clearTruncation()
{
    if (style()->visibility() == VISIBLE) {
        if (childrenInline() && hasMarkupTruncation()) {
            setHasMarkupTruncation(false);
            for (RootInlineBox* box = firstRootBox(); box; box = box->nextRootBox())
                box->clearTruncation();
        }
        else
            for (RenderObject* obj = firstChild(); obj; obj = obj->nextSibling())
                if (shouldCheckLines(obj))
                    toRenderBlock(obj)->clearTruncation();
    }
}

void RenderBlock::setMaxMarginBeforeValues(int pos, int neg)
{
    if (!m_rareData) {
        if (pos == RenderBlockRareData::positiveMarginBeforeDefault(this) && neg == RenderBlockRareData::negativeMarginBeforeDefault(this))
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_margins.setPositiveMarginBefore(pos);
    m_rareData->m_margins.setNegativeMarginBefore(neg);
}

void RenderBlock::setMaxMarginAfterValues(int pos, int neg)
{
    if (!m_rareData) {
        if (pos == RenderBlockRareData::positiveMarginAfterDefault(this) && neg == RenderBlockRareData::negativeMarginAfterDefault(this))
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_margins.setPositiveMarginAfter(pos);
    m_rareData->m_margins.setNegativeMarginAfter(neg);
}

void RenderBlock::setPaginationStrut(int strut)
{
    if (!m_rareData) {
        if (!strut)
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_paginationStrut = strut;
}

void RenderBlock::setPageLogicalOffset(int logicalOffset)
{
    if (!m_rareData) {
        if (!logicalOffset)
            return;
        m_rareData = adoptPtr(new RenderBlockRareData(this));
    }
    m_rareData->m_pageLogicalOffset = logicalOffset;
}

void RenderBlock::absoluteRects(Vector<IntRect>& rects, int tx, int ty)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (isAnonymousBlockContinuation()) {
        // FIXME: This is wrong for block-flows that are horizontal.
        // https://bugs.webkit.org/show_bug.cgi?id=46781
        rects.append(IntRect(tx, ty - collapsedMarginBefore(),
                             width(), height() + collapsedMarginBefore() + collapsedMarginAfter()));
        continuation()->absoluteRects(rects,
                                      tx - x() + inlineElementContinuation()->containingBlock()->x(),
                                      ty - y() + inlineElementContinuation()->containingBlock()->y());
    } else
        rects.append(IntRect(tx, ty, width(), height()));
}

void RenderBlock::absoluteQuads(Vector<FloatQuad>& quads)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (isAnonymousBlockContinuation()) {
        // FIXME: This is wrong for block-flows that are horizontal.
        // https://bugs.webkit.org/show_bug.cgi?id=46781
        FloatRect localRect(0, -collapsedMarginBefore(),
                            width(), height() + collapsedMarginBefore() + collapsedMarginAfter());
        quads.append(localToAbsoluteQuad(localRect));
        continuation()->absoluteQuads(quads);
    } else
        quads.append(RenderBox::localToAbsoluteQuad(FloatRect(0, 0, width(), height())));
}

IntRect RenderBlock::rectWithOutlineForRepaint(RenderBoxModelObject* repaintContainer, int outlineWidth)
{
    IntRect r(RenderBox::rectWithOutlineForRepaint(repaintContainer, outlineWidth));
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

void RenderBlock::updateHitTestResult(HitTestResult& result, const IntPoint& point)
{
    if (result.innerNode())
        return;

    Node* n = node();
    if (isAnonymousBlockContinuation())
        // We are in the margins of block elements that are part of a continuation.  In
        // this case we're actually still inside the enclosing element that was
        // split.  Go ahead and set our inner node accordingly.
        n = continuation()->node();

    if (n) {
        result.setInnerNode(n);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(n);
        result.setLocalPoint(point);
    }
}

IntRect RenderBlock::localCaretRect(InlineBox* inlineBox, int caretOffset, int* extraWidthToEndOfLine)
{
    // Do the normal calculation in most cases.
    if (firstChild())
        return RenderBox::localCaretRect(inlineBox, caretOffset, extraWidthToEndOfLine);

    // This is a special case:
    // The element is not an inline element, and it's empty. So we have to
    // calculate a fake position to indicate where objects are to be inserted.
    
    // FIXME: This does not take into account either :first-line or :first-letter
    // However, as soon as some content is entered, the line boxes will be
    // constructed and this kludge is not called any more. So only the caret size
    // of an empty :first-line'd block is wrong. I think we can live with that.
    RenderStyle* currentStyle = firstLineStyle();
    int height = lineHeight(true, currentStyle->isHorizontalWritingMode() ? HorizontalLine : VerticalLine);

    enum CaretAlignment { alignLeft, alignRight, alignCenter };

    CaretAlignment alignment = alignLeft;

    switch (currentStyle->textAlign()) {
        case TAAUTO:
        case JUSTIFY:
            if (!currentStyle->isLeftToRightDirection())
                alignment = alignRight;
            break;
        case LEFT:
        case WEBKIT_LEFT:
            break;
        case CENTER:
        case WEBKIT_CENTER:
            alignment = alignCenter;
            break;
        case RIGHT:
        case WEBKIT_RIGHT:
            alignment = alignRight;
            break;
        case TASTART:
            if (!currentStyle->isLeftToRightDirection())
                alignment = alignRight;
            break;
        case TAEND:
            if (currentStyle->isLeftToRightDirection())
                alignment = alignRight;
            break;
    }

    int x = borderLeft() + paddingLeft();
    int w = width();

    switch (alignment) {
        case alignLeft:
            break;
        case alignCenter:
            x = (x + w - (borderRight() + paddingRight())) / 2;
            break;
        case alignRight:
            x = w - (borderRight() + paddingRight()) - caretWidth;
            break;
    }

    if (extraWidthToEndOfLine) {
        if (isRenderBlock()) {
            *extraWidthToEndOfLine = w - (x + caretWidth);
        } else {
            // FIXME: This code looks wrong.
            // myRight and containerRight are set up, but then clobbered.
            // So *extraWidthToEndOfLine will always be 0 here.

            int myRight = x + caretWidth;
            // FIXME: why call localToAbsoluteForContent() twice here, too?
            FloatPoint absRightPoint = localToAbsolute(FloatPoint(myRight, 0));

            int containerRight = containingBlock()->x() + containingBlockLogicalWidthForContent();
            FloatPoint absContainerPoint = localToAbsolute(FloatPoint(containerRight, 0));

            *extraWidthToEndOfLine = absContainerPoint.x() - absRightPoint.x();
        }
    }

    int y = paddingTop() + borderTop();

    return IntRect(x, y, caretWidth, height);
}

void RenderBlock::addFocusRingRects(Vector<IntRect>& rects, int tx, int ty)
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
        int topMargin = prevInlineHasLineBox ? collapsedMarginBefore() : 0;
        int bottomMargin = nextInlineHasLineBox ? collapsedMarginAfter() : 0;
        IntRect rect(tx, ty - topMargin, width(), height() + topMargin + bottomMargin);
        if (!rect.isEmpty())
            rects.append(rect);
    } else if (width() && height())
        rects.append(IntRect(tx, ty, width(), height()));

    if (!hasOverflowClip() && !hasControlClip()) {
        for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
            int top = max(curr->lineTop(), curr->logicalTop());
            int bottom = min(curr->lineBottom(), curr->logicalTop() + curr->logicalHeight());
            IntRect rect(tx + curr->x(), ty + top, curr->logicalWidth(), bottom - top);
            if (!rect.isEmpty())
                rects.append(rect);
        }

        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (!curr->isText() && !curr->isListMarker() && curr->isBox()) {
                RenderBox* box = toRenderBox(curr);
                FloatPoint pos;
                // FIXME: This doesn't work correctly with transforms.
                if (box->layer()) 
                    pos = curr->localToAbsolute();
                else
                    pos = FloatPoint(tx + box->x(), ty + box->y());
                box->addFocusRingRects(rects, pos.x(), pos.y());
            }
        }
    }

    if (inlineElementContinuation())
        inlineElementContinuation()->addFocusRingRects(rects, 
                                                       tx - x() + inlineElementContinuation()->containingBlock()->x(),
                                                       ty - y() + inlineElementContinuation()->containingBlock()->y());
}

RenderBlock* RenderBlock::createAnonymousBlock(bool isFlexibleBox) const
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());

    RenderBlock* newBox = 0;
    if (isFlexibleBox) {
        newStyle->setDisplay(BOX);
        newBox = new (renderArena()) RenderFlexibleBox(document() /* anonymous box */);
    } else {
        newStyle->setDisplay(BLOCK);
        newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
    }

    newBox->setStyle(newStyle.release());
    return newBox;
}

RenderBlock* RenderBlock::createAnonymousBlockWithSameTypeAs(RenderBlock* otherAnonymousBlock) const
{
    if (otherAnonymousBlock->isAnonymousColumnsBlock())
        return createAnonymousColumnsBlock();
    if (otherAnonymousBlock->isAnonymousColumnSpanBlock())
        return createAnonymousColumnSpanBlock();
    return createAnonymousBlock(otherAnonymousBlock->style()->display() == BOX);
}

RenderBlock* RenderBlock::createAnonymousColumnsBlock() const
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());
    newStyle->inheritColumnPropertiesFrom(style());
    newStyle->setDisplay(BLOCK);

    RenderBlock* newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
    newBox->setStyle(newStyle.release());
    return newBox;
}

RenderBlock* RenderBlock::createAnonymousColumnSpanBlock() const
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(style());
    newStyle->setColumnSpan(true);
    newStyle->setDisplay(BLOCK);

    RenderBlock* newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
    newBox->setStyle(newStyle.release());
    return newBox;
}

int RenderBlock::nextPageLogicalTop(int logicalOffset) const
{
    LayoutState* layoutState = view()->layoutState();
    if (!layoutState->m_pageLogicalHeight)
        return logicalOffset;
    
    // The logicalOffset is in our coordinate space.  We can add in our pushed offset.
    int pageLogicalHeight = layoutState->m_pageLogicalHeight;
    IntSize delta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
    int offset = isHorizontalWritingMode() ? delta.height() : delta.width();
    int remainingLogicalHeight = (pageLogicalHeight - (offset + logicalOffset) % pageLogicalHeight) % pageLogicalHeight;
    return logicalOffset + remainingLogicalHeight;
}

static bool inNormalFlow(RenderBox* child)
{
    RenderBlock* curr = child->containingBlock();
    RenderBlock* initialBlock = child->view();
    while (curr && curr != initialBlock) {
        if (curr->hasColumns())
            return true;
        if (curr->isFloatingOrPositioned())
            return false;
        curr = curr->containingBlock();
    }
    return true;
}

int RenderBlock::applyBeforeBreak(RenderBox* child, int logicalOffset)
{
    // FIXME: Add page break checking here when we support printing.
    bool checkColumnBreaks = view()->layoutState()->isPaginatingColumns();
    bool checkPageBreaks = !checkColumnBreaks && view()->layoutState()->m_pageLogicalHeight; // FIXME: Once columns can print we have to check this.
    bool checkBeforeAlways = (checkColumnBreaks && child->style()->columnBreakBefore() == PBALWAYS) || (checkPageBreaks && child->style()->pageBreakBefore() == PBALWAYS);
    if (checkBeforeAlways && inNormalFlow(child)) {
        if (checkColumnBreaks)
            view()->layoutState()->addForcedColumnBreak(logicalOffset);
        return nextPageLogicalTop(logicalOffset);
    }
    return logicalOffset;
}

int RenderBlock::applyAfterBreak(RenderBox* child, int logicalOffset, MarginInfo& marginInfo)
{
    // FIXME: Add page break checking here when we support printing.
    bool checkColumnBreaks = view()->layoutState()->isPaginatingColumns();
    bool checkPageBreaks = !checkColumnBreaks && view()->layoutState()->m_pageLogicalHeight; // FIXME: Once columns can print we have to check this.
    bool checkAfterAlways = (checkColumnBreaks && child->style()->columnBreakAfter() == PBALWAYS) || (checkPageBreaks && child->style()->pageBreakAfter() == PBALWAYS);
    if (checkAfterAlways && inNormalFlow(child)) {
        marginInfo.setMarginAfterQuirk(true); // Cause margins to be discarded for any following content.
        if (checkColumnBreaks)
            view()->layoutState()->addForcedColumnBreak(logicalOffset);
        return nextPageLogicalTop(logicalOffset);
    }
    return logicalOffset;
}

int RenderBlock::adjustForUnsplittableChild(RenderBox* child, int logicalOffset, bool includeMargins)
{
    bool isUnsplittable = child->isReplaced() || child->scrollsOverflow();
    if (!isUnsplittable)
        return logicalOffset;
    int childLogicalHeight = logicalHeightForChild(child) + (includeMargins ? marginBeforeForChild(child) + marginAfterForChild(child) : 0);
    LayoutState* layoutState = view()->layoutState();
    if (layoutState->m_columnInfo)
        layoutState->m_columnInfo->updateMinimumColumnHeight(childLogicalHeight);
    int pageLogicalHeight = layoutState->m_pageLogicalHeight;
    if (!pageLogicalHeight || childLogicalHeight > pageLogicalHeight)
        return logicalOffset;
    IntSize delta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
    int offset = isHorizontalWritingMode() ? delta.height() : delta.width();
    int remainingLogicalHeight = (pageLogicalHeight - (offset + logicalOffset) % pageLogicalHeight) % pageLogicalHeight;
    if (remainingLogicalHeight < childLogicalHeight)
        return logicalOffset + remainingLogicalHeight;
    return logicalOffset;
}

void RenderBlock::adjustLinePositionForPagination(RootInlineBox* lineBox, int& delta)
{
    // FIXME: For now we paginate using line overflow.  This ensures that lines don't overlap at all when we
    // put a strut between them for pagination purposes.  However, this really isn't the desired rendering, since
    // the line on the top of the next page will appear too far down relative to the same kind of line at the top
    // of the first column.
    //
    // The rendering we would like to see is one where the lineTop is at the top of the column, and any line overflow
    // simply spills out above the top of the column.  This effect would match what happens at the top of the first column.
    // We can't achieve this rendering, however, until we stop columns from clipping to the column bounds (thus allowing
    // for overflow to occur), and then cache visible overflow for each column rect.
    //
    // Furthermore, the paint we have to do when a column has overflow has to be special.  We need to exclude
    // content that paints in a previous column (and content that paints in the following column).
    //
    // FIXME: Another problem with simply moving lines is that the available line width may change (because of floats).
    // Technically if the location we move the line to has a different line width than our old position, then we need to dirty the
    // line and all following lines.
    LayoutState* layoutState = view()->layoutState();
    int pageLogicalHeight = layoutState->m_pageLogicalHeight;
    IntRect logicalVisualOverflow = lineBox->logicalVisualOverflowRect(lineBox->lineTop(), lineBox->lineBottom());
    int logicalOffset = logicalVisualOverflow.y();
    int lineHeight = logicalVisualOverflow.maxY() - logicalOffset;
    if (layoutState->m_columnInfo)
        layoutState->m_columnInfo->updateMinimumColumnHeight(lineHeight);
    logicalOffset += delta;
    lineBox->setPaginationStrut(0);
    if (!pageLogicalHeight || lineHeight > pageLogicalHeight)
        return;
    IntSize offsetDelta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
    int offset = isHorizontalWritingMode() ? offsetDelta.height() : offsetDelta.width();
    int remainingLogicalHeight = pageLogicalHeight - (offset + logicalOffset) % pageLogicalHeight;
    if (remainingLogicalHeight < lineHeight) {
        int totalLogicalHeight = lineHeight + max(0, logicalOffset);
        if (lineBox == firstRootBox() && totalLogicalHeight < pageLogicalHeight && !isPositioned() && !isTableCell())
            setPaginationStrut(remainingLogicalHeight + max(0, logicalOffset));
        else {
            delta += remainingLogicalHeight;
            lineBox->setPaginationStrut(remainingLogicalHeight);
        }
    }  
}

int RenderBlock::collapsedMarginBeforeForChild(RenderBox* child) const
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

int RenderBlock::collapsedMarginAfterForChild(RenderBox* child) const
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

int RenderBlock::marginBeforeForChild(RenderBoxModelObject* child) const
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return child->marginTop();
    case BottomToTopWritingMode:
        return child->marginBottom();
    case LeftToRightWritingMode:
        return child->marginLeft();
    case RightToLeftWritingMode:
        return child->marginRight();
    }
    ASSERT_NOT_REACHED();
    return child->marginTop();
}

int RenderBlock::marginAfterForChild(RenderBoxModelObject* child) const
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return child->marginBottom();
    case BottomToTopWritingMode:
        return child->marginTop();
    case LeftToRightWritingMode:
        return child->marginRight();
    case RightToLeftWritingMode:
        return child->marginLeft();
    }
    ASSERT_NOT_REACHED();
    return child->marginBottom();
}
    
int RenderBlock::marginStartForChild(RenderBoxModelObject* child) const
{
    if (isHorizontalWritingMode())
        return style()->isLeftToRightDirection() ? child->marginLeft() : child->marginRight();
    return style()->isLeftToRightDirection() ? child->marginTop() : child->marginBottom();
}

int RenderBlock::marginEndForChild(RenderBoxModelObject* child) const
{
    if (isHorizontalWritingMode())
        return style()->isLeftToRightDirection() ? child->marginRight() : child->marginLeft();
    return style()->isLeftToRightDirection() ? child->marginBottom() : child->marginTop();
}

void RenderBlock::setMarginStartForChild(RenderBox* child, int margin)
{
    if (isHorizontalWritingMode()) {
        if (style()->isLeftToRightDirection())
            child->setMarginLeft(margin);
        else
            child->setMarginRight(margin);
    } else {
        if (style()->isLeftToRightDirection())
            child->setMarginTop(margin);
        else
            child->setMarginBottom(margin);
    }
}

void RenderBlock::setMarginEndForChild(RenderBox* child, int margin)
{
    if (isHorizontalWritingMode()) {
        if (style()->isLeftToRightDirection())
            child->setMarginRight(margin);
        else
            child->setMarginLeft(margin);
    } else {
        if (style()->isLeftToRightDirection())
            child->setMarginBottom(margin);
        else
            child->setMarginTop(margin);
    }
}

void RenderBlock::setMarginBeforeForChild(RenderBox* child, int margin)
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        child->setMarginTop(margin);
        break;
    case BottomToTopWritingMode:
        child->setMarginBottom(margin);
        break;
    case LeftToRightWritingMode:
        child->setMarginLeft(margin);
        break;
    case RightToLeftWritingMode:
        child->setMarginRight(margin);
        break;
    }
}

void RenderBlock::setMarginAfterForChild(RenderBox* child, int margin)
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        child->setMarginBottom(margin);
        break;
    case BottomToTopWritingMode:
        child->setMarginTop(margin);
        break;
    case LeftToRightWritingMode:
        child->setMarginRight(margin);
        break;
    case RightToLeftWritingMode:
        child->setMarginLeft(margin);
        break;
    }
}

RenderBlock::MarginValues RenderBlock::marginValuesForChild(RenderBox* child)
{
    int childBeforePositive = 0;
    int childBeforeNegative = 0;
    int childAfterPositive = 0;
    int childAfterNegative = 0;

    int beforeMargin = 0;
    int afterMargin = 0;

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
    if (isPositioned())
        return "RenderBlock (positioned)";
    if (isAnonymousColumnsBlock())
        return "RenderBlock (anonymous multi-column)";
    if (isAnonymousColumnSpanBlock())
        return "RenderBlock (anonymous multi-column span)";
    if (isAnonymousBlock())
        return "RenderBlock (anonymous)";
    else if (isAnonymous())
        return "RenderBlock (generated)";
    if (isRelPositioned())
        return "RenderBlock (relative positioned)";
    if (isRunIn())
        return "RenderBlock (run-in)";
    return "RenderBlock";
}

inline void RenderBlock::FloatingObjects::clear()
{
    m_set.clear();
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


#ifndef NDEBUG

void RenderBlock::showLineTreeAndMark(const InlineBox* markedBox1, const char* markedLabel1, const InlineBox* markedBox2, const char* markedLabel2, const RenderObject* obj) const
{
    showRenderObject();
    for (const RootInlineBox* root = firstRootBox(); root; root = root->nextRootBox())
        root->showLineTreeAndMark(markedBox1, markedLabel1, markedBox2, markedLabel2, obj, 1);
}

#endif

} // namespace WebCore
