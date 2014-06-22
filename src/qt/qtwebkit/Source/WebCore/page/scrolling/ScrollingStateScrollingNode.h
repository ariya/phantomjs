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

#ifndef ScrollingStateScrollingNode_h
#define ScrollingStateScrollingNode_h

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "IntRect.h"
#include "Region.h"
#include "ScrollTypes.h"
#include "ScrollingCoordinator.h"
#include "ScrollingStateNode.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class ScrollingStateScrollingNode : public ScrollingStateNode {
public:
    static PassOwnPtr<ScrollingStateScrollingNode> create(ScrollingStateTree*, ScrollingNodeID);

    virtual PassOwnPtr<ScrollingStateNode> clone();

    virtual ~ScrollingStateScrollingNode();

    enum ChangedProperty {
        ViewportRect = NumStateNodeBits,
        TotalContentsSize,
        FrameScaleFactor,
        NonFastScrollableRegion,
        WheelEventHandlerCount,
        ShouldUpdateScrollLayerPositionOnMainThread,
        HorizontalScrollElasticity,
        VerticalScrollElasticity,
        HasEnabledHorizontalScrollbar,
        HasEnabledVerticalScrollbar,
        HorizontalScrollbarMode,
        VerticalScrollbarMode,
        ScrollOrigin,
        RequestedScrollPosition,
        CounterScrollingLayer,
        HeaderHeight,
        FooterHeight,
        HeaderLayer,
        FooterLayer
    };

    virtual bool isScrollingNode() OVERRIDE { return true; }

    const IntRect& viewportRect() const { return m_viewportRect; }
    void setViewportRect(const IntRect&);

    const IntSize& totalContentsSize() const { return m_totalContentsSize; }
    void setTotalContentsSize(const IntSize&);

    float frameScaleFactor() const { return m_frameScaleFactor; }
    void setFrameScaleFactor(float);

    const Region& nonFastScrollableRegion() const { return m_nonFastScrollableRegion; }
    void setNonFastScrollableRegion(const Region&);

    unsigned wheelEventHandlerCount() const { return m_wheelEventHandlerCount; }
    void setWheelEventHandlerCount(unsigned);

    MainThreadScrollingReasons shouldUpdateScrollLayerPositionOnMainThread() const { return m_shouldUpdateScrollLayerPositionOnMainThread; }
    void setShouldUpdateScrollLayerPositionOnMainThread(MainThreadScrollingReasons);

    ScrollElasticity horizontalScrollElasticity() const { return m_horizontalScrollElasticity; }
    void setHorizontalScrollElasticity(ScrollElasticity);

    ScrollElasticity verticalScrollElasticity() const { return m_verticalScrollElasticity; }
    void setVerticalScrollElasticity(ScrollElasticity);

    bool hasEnabledHorizontalScrollbar() const { return m_hasEnabledHorizontalScrollbar; }
    void setHasEnabledHorizontalScrollbar(bool);

    bool hasEnabledVerticalScrollbar() const { return m_hasEnabledVerticalScrollbar; }
    void setHasEnabledVerticalScrollbar(bool);

    ScrollbarMode horizontalScrollbarMode() const { return m_horizontalScrollbarMode; }
    void setHorizontalScrollbarMode(ScrollbarMode);

    ScrollbarMode verticalScrollbarMode() const { return m_verticalScrollbarMode; }
    void setVerticalScrollbarMode(ScrollbarMode);

    const IntPoint& requestedScrollPosition() const { return m_requestedScrollPosition; }
    void setRequestedScrollPosition(const IntPoint&, bool representsProgrammaticScroll);

    const IntPoint& scrollOrigin() const { return m_scrollOrigin; }
    void setScrollOrigin(const IntPoint&);

    int headerHeight() const { return m_headerHeight; }
    void setHeaderHeight(int);

    int footerHeight() const { return m_footerHeight; }
    void setFooterHeight(int);

    // This is a layer moved in the opposite direction to scrolling, for example for background-attachment:fixed
    GraphicsLayer* counterScrollingLayer() const { return m_counterScrollingLayer; }
    void setCounterScrollingLayer(GraphicsLayer*);
    PlatformLayer* counterScrollingPlatformLayer() const;

    // The header and footer layers scroll vertically with the page, they should remain fixed when scrolling horizontally.
    GraphicsLayer* headerLayer() const { return m_headerLayer; }
    void setHeaderLayer(GraphicsLayer*);
    PlatformLayer* headerPlatformLayer() const;

    // The header and footer layers scroll vertically with the page, they should remain fixed when scrolling horizontally.
    GraphicsLayer* footerLayer() const { return m_footerLayer; }
    void setFooterLayer(GraphicsLayer*);
    PlatformLayer* footerPlatformLayer() const;

    bool requestedScrollPositionRepresentsProgrammaticScroll() const { return m_requestedScrollPositionRepresentsProgrammaticScroll; }

    virtual void dumpProperties(TextStream&, int indent) const OVERRIDE;

private:
    ScrollingStateScrollingNode(ScrollingStateTree*, ScrollingNodeID);
    ScrollingStateScrollingNode(const ScrollingStateScrollingNode&);

    GraphicsLayer* m_counterScrollingLayer;
    GraphicsLayer* m_headerLayer;
    GraphicsLayer* m_footerLayer;
#if PLATFORM(MAC)
    RetainPtr<PlatformLayer> m_counterScrollingPlatformLayer;
    RetainPtr<PlatformLayer> m_headerPlatformLayer;
    RetainPtr<PlatformLayer> m_footerPlatformLayer;
#endif
    
    IntRect m_viewportRect;
    IntSize m_totalContentsSize;
    
    float m_frameScaleFactor;

    Region m_nonFastScrollableRegion;

    unsigned m_wheelEventHandlerCount;

    MainThreadScrollingReasons m_shouldUpdateScrollLayerPositionOnMainThread;

    ScrollElasticity m_horizontalScrollElasticity;
    ScrollElasticity m_verticalScrollElasticity;

    bool m_hasEnabledHorizontalScrollbar;
    bool m_hasEnabledVerticalScrollbar;
    bool m_requestedScrollPositionRepresentsProgrammaticScroll;

    ScrollbarMode m_horizontalScrollbarMode;
    ScrollbarMode m_verticalScrollbarMode;

    IntPoint m_requestedScrollPosition;
    IntPoint m_scrollOrigin;

    int m_headerHeight;
    int m_footerHeight;
};

inline ScrollingStateScrollingNode* toScrollingStateScrollingNode(ScrollingStateNode* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isScrollingNode());
    return static_cast<ScrollingStateScrollingNode*>(node);
}
    
// This will catch anyone doing an unnecessary cast.
void toScrollingStateScrollingNode(const ScrollingStateScrollingNode*);

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#endif // ScrollingStateScrollingNode_h
