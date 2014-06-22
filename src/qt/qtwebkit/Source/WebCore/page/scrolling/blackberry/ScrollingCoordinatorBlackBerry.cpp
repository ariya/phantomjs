/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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
#include "ScrollingCoordinatorBlackBerry.h"

#include "Frame.h"
#include "FrameView.h"
#include "LayerWebKitThread.h"
#include "Page.h"
#include "PlatformLayer.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"

namespace WebCore {

ScrollingCoordinatorBlackBerry::ScrollingCoordinatorBlackBerry(Page* page)
    : ScrollingCoordinator(page)
{
}

void ScrollingCoordinatorBlackBerry::setLayerIsContainerForFixedPositionLayers(GraphicsLayer* layer, bool isContainer)
{
    // We don't want to set this for the main frame, the main frame would be treated like a subframe
    // by the LayerRenderer which would result in incorrect positioning.
    if (m_page->mainFrame() && scrollLayerForFrameView(m_page->mainFrame()->view()) == layer)
        isContainer = false;

    if (layer->platformLayer())
        layer->platformLayer()->setIsContainerForFixedPositionLayers(isContainer);
}

void ScrollingCoordinatorBlackBerry::setLayerIsFixedToContainerLayer(GraphicsLayer* layer, bool isFixed)
{
    if (layer->platformLayer())
        layer->platformLayer()->setFixedPosition(isFixed);
}

void ScrollingCoordinatorBlackBerry::setLayerFixedToContainerLayerEdge(GraphicsLayer* layer, bool fixedToTop, bool fixedToLeft)
{
    if (!layer->platformLayer())
        return;

    layer->platformLayer()->setFixedToTop(fixedToTop);
    layer->platformLayer()->setFixedToLeft(fixedToLeft);
}

void ScrollingCoordinatorBlackBerry::frameViewLayoutUpdated(FrameView* frameView)
{
    if (GraphicsLayer* scrollLayer = scrollLayerForFrameView(frameView)) {
        scrollLayer->platformLayer()->setFrameContentsSize(frameView->contentsSize());
        scrollLayer->platformLayer()->setFrameVisibleRect(frameView->visibleContentRect());
    }
}

}
