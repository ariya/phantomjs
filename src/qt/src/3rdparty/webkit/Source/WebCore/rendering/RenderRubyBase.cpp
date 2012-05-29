/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "RenderRubyBase.h"
#include "RenderRubyRun.h"
#include "RenderRubyText.h"

using namespace std;

namespace WebCore {

RenderRubyBase::RenderRubyBase(Node* node)
    : RenderBlock(node)
{
    setInline(false);
}

RenderRubyBase::~RenderRubyBase()
{
}

bool RenderRubyBase::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->isInline();
}

bool RenderRubyBase::hasOnlyWrappedInlineChildren(RenderObject* beforeChild) const
{
    // Tests whether all children in the base before beforeChild are either floated/positioned,
    // or inline objects wrapped in anonymous blocks.
    // Note that beforeChild may be 0, in which case all children are looked at.
    for (RenderObject* child = firstChild(); child != beforeChild; child = child->nextSibling()) {
        if (!child->isFloatingOrPositioned() && !(child->isAnonymousBlock() && child->childrenInline()))
            return false;
    }
    return true;
}

void RenderRubyBase::moveChildren(RenderRubyBase* toBase, RenderObject* fromBeforeChild)
{
    // This function removes all children that are before (!) beforeChild
    // and appends them to toBase.
    ASSERT(toBase);
    
    // First make sure that beforeChild (if set) is indeed a direct child of this.
    // Inline children might be wrapped in an anonymous block if there's a continuation.
    // Theoretically, in ruby bases, this can happen with only the first such a child,
    // so it should be OK to just climb the tree.
    while (fromBeforeChild && fromBeforeChild->parent() != this)
        fromBeforeChild = fromBeforeChild->parent();

    if (childrenInline())
        moveInlineChildren(toBase, fromBeforeChild);
    else
        moveBlockChildren(toBase, fromBeforeChild);

    setNeedsLayoutAndPrefWidthsRecalc();
    toBase->setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderRubyBase::moveInlineChildren(RenderRubyBase* toBase, RenderObject* fromBeforeChild)
{
    RenderBlock* toBlock;

    if (toBase->childrenInline()) {
        // The standard and easy case: move the children into the target base
        toBlock = toBase;
    } else {
        // We need to wrap the inline objects into an anonymous block.
        // If toBase has a suitable block, we re-use it, otherwise create a new one.
        RenderObject* lastChild = toBase->lastChild();
        if (lastChild && lastChild->isAnonymousBlock() && lastChild->childrenInline())
            toBlock = toRenderBlock(lastChild);
        else {
            toBlock = toBase->createAnonymousBlock();
            toBase->children()->appendChildNode(toBase, toBlock);
        }
    }
    // Move our inline children into the target block we determined above.
    moveChildrenTo(toBlock, firstChild(), fromBeforeChild);
}

void RenderRubyBase::moveBlockChildren(RenderRubyBase* toBase, RenderObject* fromBeforeChild)
{
    if (toBase->childrenInline()) {
        // First check whether we move only wrapped inline objects.
        if (hasOnlyWrappedInlineChildren(fromBeforeChild)) {
            // The reason why the base is in block flow must be after beforeChild.
            // We therefore can extract the inline objects and move them to toBase.
            for (RenderObject* child = firstChild(); child != fromBeforeChild; child = firstChild()) {
                if (child->isAnonymousBlock()) {
                    RenderBlock* anonBlock = toRenderBlock(child);
                    ASSERT(anonBlock->childrenInline());
                    ASSERT(!anonBlock->inlineElementContinuation());
                    anonBlock->moveAllChildrenTo(toBase, toBase->children());
                    anonBlock->deleteLineBoxTree();
                    anonBlock->destroy();
                } else {
                    ASSERT(child->isFloatingOrPositioned());
                    moveChildTo(toBase, child);
                }
            }
        } else {
            // Moving block children -> have to set toBase as block flow
            toBase->makeChildrenNonInline();
            // Move children, potentially collapsing anonymous block wrappers.
            mergeBlockChildren(toBase, fromBeforeChild);

            // Now we need to check if the leftover children are all inline.
            // If so, make this base inline again.
            if (hasOnlyWrappedInlineChildren()) {
                RenderObject* next = 0;
                for (RenderObject* child = firstChild(); child; child = next) {
                    next = child->nextSibling();
                    if (child->isFloatingOrPositioned())
                        continue;
                    ASSERT(child->isAnonymousBlock());

                    RenderBlock* anonBlock = toRenderBlock(child);
                    ASSERT(anonBlock->childrenInline());
                    ASSERT(!anonBlock->inlineElementContinuation());
                    // Move inline children out of anonymous block.
                    anonBlock->moveAllChildrenTo(this, anonBlock);
                    anonBlock->deleteLineBoxTree();
                    anonBlock->destroy();
                }
                setChildrenInline(true);
            }
        }
    } else
        mergeBlockChildren(toBase, fromBeforeChild);
}

void RenderRubyBase::mergeBlockChildren(RenderRubyBase* toBase, RenderObject* fromBeforeChild)
{
    // This function removes all children that are before fromBeforeChild and appends them to toBase.
    ASSERT(!childrenInline());
    ASSERT(toBase);
    ASSERT(!toBase->childrenInline());

    // Quick check whether we have anything to do, to simplify the following code.
    if (fromBeforeChild != firstChild())
        return;

    // If an anonymous block would be put next to another such block, then merge those.
    RenderObject* firstChildHere = firstChild();
    RenderObject* lastChildThere = toBase->lastChild();
    if (firstChildHere && firstChildHere->isAnonymousBlock() && firstChildHere->childrenInline() 
            && lastChildThere && lastChildThere->isAnonymousBlock() && lastChildThere->childrenInline()) {            
        RenderBlock* anonBlockHere = toRenderBlock(firstChildHere);
        RenderBlock* anonBlockThere = toRenderBlock(lastChildThere);
        anonBlockHere->moveAllChildrenTo(anonBlockThere, anonBlockThere->children());
        anonBlockHere->deleteLineBoxTree();
        anonBlockHere->destroy();
    }
    // Move all remaining children normally.
    moveChildrenTo(toBase, firstChild(), fromBeforeChild);
}

RenderRubyRun* RenderRubyBase::rubyRun() const
{
    ASSERT(parent());
    ASSERT(parent()->isRubyRun());

    return static_cast<RenderRubyRun*>(parent());
}

ETextAlign RenderRubyBase::textAlignmentForLine(bool /* endsWithSoftBreak */) const
{
    return JUSTIFY;
}

void RenderRubyBase::adjustInlineDirectionLineBounds(int expansionOpportunityCount, float& logicalLeft, float& logicalWidth) const
{
    int maxPreferredLogicalWidth = this->maxPreferredLogicalWidth();
    if (maxPreferredLogicalWidth >= logicalWidth)
        return;

    // Inset the ruby base by half the inter-ideograph expansion amount.
    float inset = (logicalWidth - maxPreferredLogicalWidth) / (expansionOpportunityCount + 1);

    logicalLeft += inset / 2;
    logicalWidth -= inset;
}

} // namespace WebCore
