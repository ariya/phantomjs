/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef InRegionScroller_p_h
#define InRegionScroller_p_h

#include "IntPoint.h"
#include "IntSize.h"

#include <interaction/ScrollViewBase.h>
#include <vector>

namespace WebCore {
class Frame;
class LayerWebKitThread;
class Node;
class RenderBox;
class RenderObject;
class RenderLayer;
}

namespace BlackBerry {
namespace WebKit {

class InRegionScrollableArea;
class WebPagePrivate;

class InRegionScrollerPrivate {
public:
    InRegionScrollerPrivate(WebPagePrivate*);

    void reset();
    void resetSelectionScrollView();
    bool isActive() const;

    bool setScrollPositionCompositingThread(unsigned camouflagedLayer, const WebCore::IntPoint& scrollPosition);
    bool setScrollPositionWebKitThread(unsigned camouflagedLayer, const WebCore::IntPoint& scrollPosition,
        bool /*acceleratedScrolling*/, Platform::ScrollViewBase::ScrollTarget);

    void calculateInRegionScrollableAreasForPoint(const WebCore::IntPoint&);
    const std::vector<Platform::ScrollViewBase*>& activeInRegionScrollableAreas() const;
    void updateSelectionScrollView(const WebCore::Node*);

    void clearDocumentData(const WebCore::Document*);

    static bool canScrollRenderBox(WebCore::RenderBox*);

    WebPagePrivate* m_webPage;
    bool m_needsActiveScrollableAreaCalculation;

private:
    Platform::ScrollViewBase* firstScrollableInRegionForNode(const WebCore::Node*);
    bool setLayerScrollPosition(WebCore::RenderLayer*, const WebCore::IntPoint& scrollPosition);

    void calculateActiveAndShrinkCachedScrollableAreas(WebCore::RenderLayer*);

    void pushBackInRegionScrollable(InRegionScrollableArea*);

    void adjustScrollDelta(const WebCore::IntPoint& maxOffset, const WebCore::IntPoint& currentOffset, WebCore::IntSize& delta) const;
    Platform::ScrollViewBase* clipAndCreateInRegionScrollableArea(WebCore::RenderLayer*);

    bool isValidScrollableLayerWebKitThread(WebCore::LayerWebKitThread*) const;
    bool isValidScrollableNode(WebCore::Node*) const;
    WebCore::IntRect clipToRect(const WebCore::IntRect&, InRegionScrollableArea*);
    std::vector<Platform::ScrollViewBase*> m_activeInRegionScrollableAreas;
    Platform::ScrollViewBase* m_selectionScrollView;
};

}
}

#endif
