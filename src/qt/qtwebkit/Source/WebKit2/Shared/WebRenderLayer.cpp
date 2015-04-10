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
#include "WebRenderLayer.h"

#include "WebPage.h"
#include "WebString.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderClient.h>
#include <WebCore/RenderLayer.h>
#include <WebCore/RenderLayerBacking.h>
#include <WebCore/RenderView.h>
#include <WebCore/StyledElement.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebRenderLayer> WebRenderLayer::create(WebPage* page)
{
    Frame* mainFrame = page->mainFrame();
    if (!mainFrame)
        return 0;

    if (!mainFrame->loader()->client()->hasHTMLView())
        return 0;

    RenderView* contentRenderer = mainFrame->contentRenderer();
    if (!contentRenderer)
        return 0;

    RenderLayer* rootLayer = contentRenderer->layer();
    if (!rootLayer)
        return 0;

    return adoptRef(new WebRenderLayer(rootLayer));
}

PassRefPtr<MutableArray> WebRenderLayer::createArrayFromLayerList(Vector<RenderLayer*>* list)
{
    if (!list || !list->size())
        return 0;

    RefPtr<MutableArray> array = MutableArray::create();
    for (size_t i = 0; i < list->size(); ++i) {
        RefPtr<WebRenderLayer> layer = adoptRef(new WebRenderLayer(list->at(i)));
        array->append(layer.get());
    }

    return array.release();
}

WebRenderLayer::WebRenderLayer(RenderLayer* layer)
{
    m_renderer = WebRenderObject::create(layer->renderer());
    m_isReflection = layer->isReflection();

#if USE(ACCELERATED_COMPOSITING)
    if (layer->isComposited()) {
        RenderLayerBacking* backing = layer->backing();
        m_isClipping = backing->hasClippingLayer();
        m_isClipped = backing->hasAncestorClippingLayer();
        switch (backing->compositingLayerType()) {
        case NormalCompositingLayer:
            m_compositingLayerType = Normal;
            break;
        case TiledCompositingLayer:
            m_compositingLayerType = Tiled;
            break;
        case MediaCompositingLayer:
            m_compositingLayerType = Media;
            break;
        case ContainerCompositingLayer:
            m_compositingLayerType = Container;
            break;
        }
    } else {
#endif
        m_isClipping = false;
        m_isClipped = false;
        m_compositingLayerType = None;
#if USE(ACCELERATED_COMPOSITING)
    }
#endif

    m_absoluteBoundingBox = layer->absoluteBoundingBox();

    m_negativeZOrderList = createArrayFromLayerList(layer->negZOrderList());
    m_normalFlowList = createArrayFromLayerList(layer->normalFlowList());
    m_positiveZOrderList = createArrayFromLayerList(layer->posZOrderList());
}

} // namespace WebKit
