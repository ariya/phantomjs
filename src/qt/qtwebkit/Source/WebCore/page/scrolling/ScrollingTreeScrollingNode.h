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

#ifndef ScrollingTreeScrollingNode_h
#define ScrollingTreeScrollingNode_h

#if ENABLE(THREADED_SCROLLING)

#include "IntRect.h"
#include "ScrollTypes.h"
#include "ScrollingCoordinator.h"
#include "ScrollingTreeNode.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class PlatformWheelEvent;
class ScrollingTree;
class ScrollingStateScrollingNode;

class ScrollingTreeScrollingNode : public ScrollingTreeNode {
public:
    static PassOwnPtr<ScrollingTreeScrollingNode> create(ScrollingTree*, ScrollingNodeID);
    virtual ~ScrollingTreeScrollingNode();

    virtual void updateBeforeChildren(ScrollingStateNode*) OVERRIDE;

    // FIXME: We should implement this when we support ScrollingTreeScrollingNodes as children.
    virtual void parentScrollPositionDidChange(const IntRect& /*viewportRect*/, const FloatSize& /*cumulativeDelta*/) OVERRIDE { }

    virtual void handleWheelEvent(const PlatformWheelEvent&) = 0;
    virtual void setScrollPosition(const IntPoint&) = 0;

    MainThreadScrollingReasons shouldUpdateScrollLayerPositionOnMainThread() const { return m_shouldUpdateScrollLayerPositionOnMainThread; }

protected:
    explicit ScrollingTreeScrollingNode(ScrollingTree*, ScrollingNodeID);

    const IntRect& viewportRect() const { return m_viewportRect; }
    const IntSize& totalContentsSize() const { return m_totalContentsSize; }

    float frameScaleFactor() const { return m_frameScaleFactor; }

    ScrollElasticity horizontalScrollElasticity() const { return m_horizontalScrollElasticity; }
    ScrollElasticity verticalScrollElasticity() const { return m_verticalScrollElasticity; }

    bool hasEnabledHorizontalScrollbar() const { return m_hasEnabledHorizontalScrollbar; }
    bool hasEnabledVerticalScrollbar() const { return m_hasEnabledVerticalScrollbar; }

    bool canHaveScrollbars() const { return m_horizontalScrollbarMode != ScrollbarAlwaysOff || m_verticalScrollbarMode != ScrollbarAlwaysOff; }

    const IntPoint& scrollOrigin() const { return m_scrollOrigin; }

    int headerHeight() const { return m_headerHeight; }
    int footerHeight() const { return m_footerHeight; }

private:
    IntRect m_viewportRect;
    IntSize m_totalContentsSize;
    IntPoint m_scrollOrigin;
    
    float m_frameScaleFactor;

    MainThreadScrollingReasons m_shouldUpdateScrollLayerPositionOnMainThread;

    ScrollElasticity m_horizontalScrollElasticity;
    ScrollElasticity m_verticalScrollElasticity;
    
    bool m_hasEnabledHorizontalScrollbar;
    bool m_hasEnabledVerticalScrollbar;

    ScrollbarMode m_horizontalScrollbarMode;
    ScrollbarMode m_verticalScrollbarMode;

    int m_headerHeight;
    int m_footerHeight;
};

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)

#endif // ScrollingTreeScrollingNode_h
