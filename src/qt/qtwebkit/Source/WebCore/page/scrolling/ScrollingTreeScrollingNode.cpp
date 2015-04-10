/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollingTreeScrollingNode.h"

#if ENABLE(THREADED_SCROLLING)

#include "ScrollingStateTree.h"

namespace WebCore {

ScrollingTreeScrollingNode::ScrollingTreeScrollingNode(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
    : ScrollingTreeNode(scrollingTree, nodeID)
    , m_frameScaleFactor(1)
    , m_shouldUpdateScrollLayerPositionOnMainThread(0)
    , m_horizontalScrollElasticity(ScrollElasticityNone)
    , m_verticalScrollElasticity(ScrollElasticityNone)
    , m_hasEnabledHorizontalScrollbar(false)
    , m_hasEnabledVerticalScrollbar(false)
    , m_horizontalScrollbarMode(ScrollbarAuto)
    , m_verticalScrollbarMode(ScrollbarAuto)
    , m_headerHeight(0)
    , m_footerHeight(0)
{
}

ScrollingTreeScrollingNode::~ScrollingTreeScrollingNode()
{
}

void ScrollingTreeScrollingNode::updateBeforeChildren(ScrollingStateNode* stateNode)
{
    ScrollingStateScrollingNode* state = toScrollingStateScrollingNode(stateNode);

    if (state->hasChangedProperty(ScrollingStateScrollingNode::ViewportRect))
        m_viewportRect = state->viewportRect();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::TotalContentsSize))
        m_totalContentsSize = state->totalContentsSize();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::FrameScaleFactor))
        m_frameScaleFactor = state->frameScaleFactor();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::ShouldUpdateScrollLayerPositionOnMainThread))
        m_shouldUpdateScrollLayerPositionOnMainThread = state->shouldUpdateScrollLayerPositionOnMainThread();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::HorizontalScrollElasticity))
        m_horizontalScrollElasticity = state->horizontalScrollElasticity();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::VerticalScrollElasticity))
        m_verticalScrollElasticity = state->verticalScrollElasticity();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::HasEnabledHorizontalScrollbar))
        m_hasEnabledHorizontalScrollbar = state->hasEnabledHorizontalScrollbar();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::HasEnabledVerticalScrollbar))
        m_hasEnabledVerticalScrollbar = state->hasEnabledVerticalScrollbar();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::HorizontalScrollbarMode))
        m_horizontalScrollbarMode = state->horizontalScrollbarMode();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::VerticalScrollbarMode))
        m_verticalScrollbarMode = state->verticalScrollbarMode();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::ScrollOrigin))
        m_scrollOrigin = state->scrollOrigin();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::HeaderHeight))
        m_headerHeight = state->headerHeight();

    if (state->hasChangedProperty(ScrollingStateScrollingNode::FooterHeight))
        m_footerHeight = state->footerHeight();
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
