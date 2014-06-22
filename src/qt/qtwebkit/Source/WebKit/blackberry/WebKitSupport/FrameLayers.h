/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef FrameLayers_h
#define FrameLayers_h

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {
class Frame;
class LayerWebKitThread;
}

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

// This class may only be used on the WebKit thread.
class FrameLayers {
public:
    FrameLayers(WebPagePrivate*);
    ~FrameLayers();

    bool containsLayerForFrame(WebCore::Frame*);
    void addLayer(WebCore::Frame*, WebCore::LayerWebKitThread*);
    void removeLayerByFrame(WebCore::Frame*);

    bool hasLayer() const { return m_frameLayers.size(); }

    void commitOnWebKitThread(double scale);
    void notifyAnimationsStarted(double animationStartTime);

    void calculateRootLayer();

    bool isRootLayerMainFrameLayer() const { return m_rootLayer && !m_rootGraphicsLayer; }

    // FIXME: This function should only be called on the WebKit thread.
    // But it's now also being called on the Compositing thread.
    WebCore::LayerWebKitThread* rootLayer() const { return m_rootLayer; }

    void releaseLayerResources();

private:
    WebPagePrivate* m_pagePrivate;
    OwnPtr<WebCore::GraphicsLayer> m_rootGraphicsLayer;
    WebCore::LayerWebKitThread* m_rootLayer;
    typedef HashMap<WebCore::Frame*, WebCore::LayerWebKitThread*> FrameLayerMap;
    FrameLayerMap m_frameLayers;
};

} // namespace BlackBerry
} // namespace WebKit

#endif // USE(ACCELERATED_COMPOSITING)

#endif // FrameLayers_h
