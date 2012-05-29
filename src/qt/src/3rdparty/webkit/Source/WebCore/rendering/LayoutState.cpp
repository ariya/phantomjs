/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "LayoutState.h"

#include "ColumnInfo.h"
#include "RenderArena.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderView.h"

namespace WebCore {

LayoutState::LayoutState(LayoutState* prev, RenderBox* renderer, const IntSize& offset, int pageLogicalHeight, bool pageLogicalHeightChanged, ColumnInfo* columnInfo)
    : m_columnInfo(columnInfo)
    , m_next(prev)
#ifndef NDEBUG
    , m_renderer(renderer)
#endif
{
    ASSERT(m_next);

    bool fixed = renderer->isPositioned() && renderer->style()->position() == FixedPosition;
    if (fixed) {
        // FIXME: This doesn't work correctly with transforms.
        FloatPoint fixedOffset = renderer->view()->localToAbsolute(FloatPoint(), true);
        m_paintOffset = IntSize(fixedOffset.x(), fixedOffset.y()) + offset;
    } else
        m_paintOffset = prev->m_paintOffset + offset;

    if (renderer->isPositioned() && !fixed) {
        if (RenderObject* container = renderer->container()) {
            if (container->isRelPositioned() && container->isRenderInline())
                m_paintOffset += toRenderInline(container)->relativePositionedInlineOffset(renderer);
        }
    }

    m_layoutOffset = m_paintOffset;

    if (renderer->isRelPositioned() && renderer->hasLayer())
        m_paintOffset += renderer->layer()->relativePositionOffset();

    m_clipped = !fixed && prev->m_clipped;
    if (m_clipped)
        m_clipRect = prev->m_clipRect;

    if (renderer->hasOverflowClip()) {
        RenderLayer* layer = renderer->layer();
        IntRect clipRect(toPoint(m_paintOffset) + renderer->view()->layoutDelta(), layer->size());
        if (m_clipped)
            m_clipRect.intersect(clipRect);
        else {
            m_clipRect = clipRect;
            m_clipped = true;
        }

        m_paintOffset -= layer->scrolledContentOffset();
    }

    // If we establish a new page height, then cache the offset to the top of the first page.
    // We can compare this later on to figure out what part of the page we're actually on,
    if (pageLogicalHeight || m_columnInfo) {
        m_pageLogicalHeight = pageLogicalHeight;
        m_pageOffset = IntSize(m_layoutOffset.width() + renderer->borderLeft() + renderer->paddingLeft(),
                               m_layoutOffset.height() + renderer->borderTop() + renderer->paddingTop());
        m_pageLogicalHeightChanged = pageLogicalHeightChanged;
    } else {
        // If we don't establish a new page height, then propagate the old page height and offset down.
        m_pageLogicalHeight = m_next->m_pageLogicalHeight;
        m_pageLogicalHeightChanged = m_next->m_pageLogicalHeightChanged;
        m_pageOffset = m_next->m_pageOffset;
        
        // Disable pagination for objects we don't support.  For now this includes overflow:scroll/auto and inline blocks.
        if (renderer->isReplaced() || renderer->scrollsOverflow())
            m_pageLogicalHeight = 0;
    }
    
    if (!m_columnInfo)
        m_columnInfo = m_next->m_columnInfo;

    m_layoutDelta = m_next->m_layoutDelta;
    
    // FIXME: <http://bugs.webkit.org/show_bug.cgi?id=13443> Apply control clip if present.
}

LayoutState::LayoutState(RenderObject* root)
    : m_clipped(false)
    , m_pageLogicalHeight(0)
    , m_pageLogicalHeightChanged(false)
    , m_columnInfo(0)
    , m_next(0)
#ifndef NDEBUG
    , m_renderer(root)
#endif
{
    RenderObject* container = root->container();
    FloatPoint absContentPoint = container->localToAbsolute(FloatPoint(), false, true);
    m_paintOffset = IntSize(absContentPoint.x(), absContentPoint.y());

    if (container->hasOverflowClip()) {
        RenderLayer* layer = toRenderBoxModelObject(container)->layer();
        m_clipped = true;
        m_clipRect = IntRect(toPoint(m_paintOffset), layer->size());
        m_paintOffset -= layer->scrolledContentOffset();
    }
}

#ifndef NDEBUG
static bool inLayoutStateDestroy;
#endif

void LayoutState::destroy(RenderArena* renderArena)
{
#ifndef NDEBUG
    inLayoutStateDestroy = true;
#endif
    delete this;
#ifndef NDEBUG
    inLayoutStateDestroy = false;
#endif
    renderArena->free(*(size_t*)this, this);
}

void* LayoutState::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void LayoutState::operator delete(void* ptr, size_t sz)
{
    ASSERT(inLayoutStateDestroy);
    *(size_t*)ptr = sz;
}

void LayoutState::clearPaginationInformation()
{
    m_pageLogicalHeight = m_next->m_pageLogicalHeight;
    m_pageOffset = m_next->m_pageOffset;
    m_columnInfo = m_next->m_columnInfo;
}

int LayoutState::pageLogicalOffset(int childLogicalOffset) const
{
    return m_layoutOffset.height() + childLogicalOffset - m_pageOffset.height();
}

void LayoutState::addForcedColumnBreak(int childLogicalOffset)
{
    if (!m_columnInfo || m_columnInfo->columnHeight())
        return;
    m_columnInfo->addForcedBreak(pageLogicalOffset(childLogicalOffset));
}

} // namespace WebCore
