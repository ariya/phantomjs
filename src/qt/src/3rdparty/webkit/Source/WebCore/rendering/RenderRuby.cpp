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

#include "RenderRuby.h"

#include "RenderRubyRun.h"
#include "RenderStyle.h"
#include <wtf/RefPtr.h>

namespace WebCore {

//=== generic helper functions to avoid excessive code duplication ===

static inline bool isAnonymousRubyInlineBlock(const RenderObject* object)
{
    ASSERT(!object
        || !object->parent()->isRuby()
        || object->isRubyRun()
        || (object->isInline() && (object->isBeforeContent() || object->isAfterContent()))
        || (object->isAnonymous() && object->isRenderBlock() && object->style()->display() == INLINE_BLOCK));

    return object
        && object->parent()->isRuby()
        && object->isRenderBlock()
        && !object->isRubyRun();
}

static inline bool isRubyBeforeBlock(const RenderObject* object)
{
    return isAnonymousRubyInlineBlock(object)
        && !object->previousSibling()
        && object->firstChild()
        && object->firstChild()->style()->styleType() == BEFORE;
}

static inline bool isRubyAfterBlock(const RenderObject* object)
{
    return isAnonymousRubyInlineBlock(object)
        && !object->nextSibling()
        && object->firstChild()
        && object->firstChild()->style()->styleType() == AFTER;
}

static inline RenderBlock* rubyBeforeBlock(const RenderObject* ruby)
{
    RenderObject* child = ruby->firstChild();
    return isRubyBeforeBlock(child) ? static_cast<RenderBlock*>(child) : 0;
}

static inline RenderBlock* rubyAfterBlock(const RenderObject* ruby)
{
    RenderObject* child = ruby->lastChild();
    return isRubyAfterBlock(child) ? static_cast<RenderBlock*>(child) : 0;
}

static RenderBlock* createAnonymousRubyInlineBlock(RenderObject* ruby)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyle(ruby->style());
    newStyle->setDisplay(INLINE_BLOCK);

    RenderBlock* newBlock = new (ruby->renderArena()) RenderBlock(ruby->document() /* anonymous box */);
    newBlock->setStyle(newStyle.release());
    return newBlock;
}

static RenderRubyRun* lastRubyRun(const RenderObject* ruby)
{
    RenderObject* child = ruby->lastChild();
    if (child && !child->isRubyRun())
        child = child->previousSibling();
    ASSERT(!child || child->isRubyRun() || child->isBeforeContent() || child == rubyBeforeBlock(ruby));
    return child && child->isRubyRun() ? static_cast<RenderRubyRun*>(child) : 0;
}

static inline RenderRubyRun* findRubyRunParent(RenderObject* child)
{
    while (child && !child->isRubyRun())
        child = child->parent();
    return static_cast<RenderRubyRun*>(child);
}

//=== ruby as inline object ===

RenderRubyAsInline::RenderRubyAsInline(Node* node)
    : RenderInline(node)
{
}

RenderRubyAsInline::~RenderRubyAsInline()
{
}

void RenderRubyAsInline::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderInline::styleDidChange(diff, oldStyle);
    propagateStyleToAnonymousChildren();
}

void RenderRubyAsInline::addChild(RenderObject* child, RenderObject* beforeChild)
{
    // Insert :before and :after content before/after the RenderRubyRun(s)
    if (child->isBeforeContent()) {
        if (child->isInline()) {
            // Add generated inline content normally
            RenderInline::addChild(child, firstChild());
        } else {
            // Wrap non-inline content with an anonymous inline-block.
            RenderBlock* beforeBlock = rubyBeforeBlock(this);
            if (!beforeBlock) {
                beforeBlock = createAnonymousRubyInlineBlock(this);
                RenderInline::addChild(beforeBlock, firstChild());
            }
            beforeBlock->addChild(child);
        }
        return;
    }
    if (child->isAfterContent()) {
        if (child->isInline()) {
            // Add generated inline content normally
            RenderInline::addChild(child);
        } else {
            // Wrap non-inline content with an anonymous inline-block.
            RenderBlock* afterBlock = rubyAfterBlock(this);
            if (!afterBlock) {
                afterBlock = createAnonymousRubyInlineBlock(this);
                RenderInline::addChild(afterBlock);
            }
            afterBlock->addChild(child);
        }
        return;
    }

    // If the child is a ruby run, just add it normally.
    if (child->isRubyRun()) {
        RenderInline::addChild(child, beforeChild);
        return;
    }

    if (beforeChild && !isAfterContent(beforeChild)) {
        // insert child into run
        ASSERT(!beforeChild->isRubyRun());
        RenderObject* run = beforeChild;
        while (run && !run->isRubyRun())
            run = run->parent();
        if (run) {
            run->addChild(child, beforeChild);
            return;
        }
        ASSERT_NOT_REACHED(); // beforeChild should always have a run as parent!
        // Emergency fallback: fall through and just append.
    }

    // If the new child would be appended, try to add the child to the previous run
    // if possible, or create a new run otherwise.
    // (The RenderRubyRun object will handle the details)
    RenderRubyRun* lastRun = lastRubyRun(this);
    if (!lastRun || lastRun->hasRubyText()) {
        lastRun = RenderRubyRun::staticCreateRubyRun(this);
        RenderInline::addChild(lastRun);
    }
    lastRun->addChild(child);
}

void RenderRubyAsInline::removeChild(RenderObject* child)
{
    // If the child's parent is *this (must be a ruby run or generated content or anonymous block),
    // just use the normal remove method.
    if (child->parent() == this) {
        ASSERT(child->isRubyRun() || child->isBeforeContent() || child->isAfterContent() || isAnonymousRubyInlineBlock(child));
        RenderInline::removeChild(child);
        return;
    }
    // If the child's parent is an anoymous block (must be generated :before/:after content)
    // just use the block's remove method.
    if (isAnonymousRubyInlineBlock(child->parent())) {
        ASSERT(child->isBeforeContent() || child->isAfterContent());
        child->parent()->removeChild(child);
        removeChild(child->parent());
        return;
    }

    // Otherwise find the containing run and remove it from there.
    RenderRubyRun* run = findRubyRunParent(child);
    ASSERT(run);
    run->removeChild(child);
}


//=== ruby as block object ===

RenderRubyAsBlock::RenderRubyAsBlock(Node* node)
    : RenderBlock(node)
{
}

RenderRubyAsBlock::~RenderRubyAsBlock()
{
}

void RenderRubyAsBlock::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    propagateStyleToAnonymousChildren();
}

void RenderRubyAsBlock::addChild(RenderObject* child, RenderObject* beforeChild)
{
    // Insert :before and :after content before/after the RenderRubyRun(s)
    if (child->isBeforeContent()) {
        if (child->isInline()) {
            // Add generated inline content normally
            RenderBlock::addChild(child, firstChild());
        } else {
            // Wrap non-inline content with an anonymous inline-block.
            RenderBlock* beforeBlock = rubyBeforeBlock(this);
            if (!beforeBlock) {
                beforeBlock = createAnonymousRubyInlineBlock(this);
                RenderBlock::addChild(beforeBlock, firstChild());
            }
            beforeBlock->addChild(child);
        }
        return;
    }
    if (child->isAfterContent()) {
        if (child->isInline()) {
            // Add generated inline content normally
            RenderBlock::addChild(child);
        } else {
            // Wrap non-inline content with an anonymous inline-block.
            RenderBlock* afterBlock = rubyAfterBlock(this);
            if (!afterBlock) {
                afterBlock = createAnonymousRubyInlineBlock(this);
                RenderBlock::addChild(afterBlock);
            }
            afterBlock->addChild(child);
        }
        return;
    }

    // If the child is a ruby run, just add it normally.
    if (child->isRubyRun()) {
        RenderBlock::addChild(child, beforeChild);
        return;
    }

    if (beforeChild && !isAfterContent(beforeChild)) {
        // insert child into run
        ASSERT(!beforeChild->isRubyRun());
        RenderObject* run = beforeChild;
        while (run && !run->isRubyRun())
            run = run->parent();
        if (run) {
            run->addChild(child, beforeChild);
            return;
        }
        ASSERT_NOT_REACHED(); // beforeChild should always have a run as parent!
        // Emergency fallback: fall through and just append.
    }

    // If the new child would be appended, try to add the child to the previous run
    // if possible, or create a new run otherwise.
    // (The RenderRubyRun object will handle the details)
    RenderRubyRun* lastRun = lastRubyRun(this);
    if (!lastRun || lastRun->hasRubyText()) {
        lastRun = RenderRubyRun::staticCreateRubyRun(this);
        RenderBlock::addChild(lastRun);
    }
    lastRun->addChild(child);
}

void RenderRubyAsBlock::removeChild(RenderObject* child)
{
    // If the child's parent is *this (must be a ruby run or generated content or anonymous block),
    // just use the normal remove method.
    if (child->parent() == this) {
        ASSERT(child->isRubyRun() || child->isBeforeContent() || child->isAfterContent() || isAnonymousRubyInlineBlock(child));
        RenderBlock::removeChild(child);
        return;
    }
    // If the child's parent is an anoymous block (must be generated :before/:after content)
    // just use the block's remove method.
    if (isAnonymousRubyInlineBlock(child->parent())) {
        ASSERT(child->isBeforeContent() || child->isAfterContent());
        child->parent()->removeChild(child);
        removeChild(child->parent());
        return;
    }

    // Otherwise find the containing run and remove it from there.
    RenderRubyRun* run = findRubyRunParent(child);
    ASSERT(run);
    run->removeChild(child);
}

} // namespace WebCore
