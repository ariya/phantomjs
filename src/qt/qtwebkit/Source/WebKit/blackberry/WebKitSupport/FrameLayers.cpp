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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#include "FrameLayers.h"

#include "Frame.h"
#include "FrameView.h"
#include "LayerWebKitThread.h"
#include "Page.h"
#include "RenderPart.h"
#include "WebPage_p.h"
#include <wtf/Assertions.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static FloatSize frameLayerAbsoluteOffset(Frame* frame)
{
    ASSERT(frame);

    if (!frame->view() || !frame->page() || !frame->page()->mainFrame() || !frame->page()->mainFrame()->view())
        return FloatSize();

    IntPoint offset = frame->page()->mainFrame()->view()->windowToContents(frame->view()->contentsToWindow(IntPoint::zero()));
    return FloatSize(offset.x(), offset.y());
}

FrameLayers::FrameLayers(WebPagePrivate* page)
    : m_pagePrivate(page)
    , m_rootGraphicsLayer()
    , m_rootLayer(0)
{
}

FrameLayers::~FrameLayers()
{
    m_frameLayers.clear();
}

bool FrameLayers::containsLayerForFrame(Frame* frame)
{
    ASSERT(frame);
    return m_frameLayers.contains(frame);
}

void FrameLayers::addLayer(Frame* frame, LayerWebKitThread* layer)
{
    // If we have main frame layer we don't accept other layers.
    // This should not happen actually.
    ASSERT(frame != m_pagePrivate->m_mainFrame || !isRootLayerMainFrameLayer());
    if (frame == m_pagePrivate->m_mainFrame && isRootLayerMainFrameLayer())
        return;

    ASSERT(frame && layer && !m_frameLayers.contains(frame));

    m_frameLayers.add(frame, layer);

    calculateRootLayer();
    if (!m_rootLayer) {
        ASSERT(!m_rootGraphicsLayer);
        m_rootGraphicsLayer = GraphicsLayer::create(0);
        m_rootLayer = m_rootGraphicsLayer->platformLayer();
    }

    if (m_rootLayer != layer)
        m_rootLayer->addSublayer(layer);
}

void FrameLayers::removeLayerByFrame(Frame* frame)
{
    ASSERT(frame);
    FrameLayerMap::iterator it = m_frameLayers.find(frame);
    if (it != m_frameLayers.end()) {
        LayerWebKitThread* layer = it->value;
        if (layer->superlayer())
            layer->removeFromSuperlayer();
        m_frameLayers.remove(it);
        calculateRootLayer();
    }
}

void FrameLayers::commitOnWebKitThread(double scale)
{
    ASSERT(m_rootLayer);
    if (!isRootLayerMainFrameLayer()) {
        for (FrameLayerMap::iterator it = m_frameLayers.begin(); it != m_frameLayers.end(); ++it)
            it->value->setAbsoluteOffset(frameLayerAbsoluteOffset(it->key));
    }
    m_rootLayer->commitOnWebKitThread(scale);
}

void FrameLayers::notifyAnimationsStarted(double animationStartTime)
{
    m_rootLayer->notifyAnimationsStarted(animationStartTime);
}

void FrameLayers::calculateRootLayer()
{
    if (m_frameLayers.size() == 1 && m_frameLayers.begin()->key == m_pagePrivate->m_mainFrame)
        m_rootLayer = m_frameLayers.begin()->value;
    else if (m_rootGraphicsLayer)
        m_rootLayer = m_rootGraphicsLayer->platformLayer();
    else
        m_rootLayer = 0;
}

void FrameLayers::releaseLayerResources()
{
    if (m_rootLayer)
        m_rootLayer->releaseLayerResources();
}

} // namespace BlackBerry
} // namespace WebKit

#endif // USE(ACCELERATED_COMPOSITING)
