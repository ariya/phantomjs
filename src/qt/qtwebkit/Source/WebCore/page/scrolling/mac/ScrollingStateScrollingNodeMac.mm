/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "ScrollingStateScrollingNode.h"

#include "GraphicsLayer.h"
#include "ScrollingStateTree.h"

#if ENABLE(THREADED_SCROLLING)

namespace WebCore {

PlatformLayer* ScrollingStateScrollingNode::counterScrollingPlatformLayer() const
{
    return m_counterScrollingPlatformLayer.get();
}

void ScrollingStateScrollingNode::setCounterScrollingLayer(GraphicsLayer* graphicsLayer)
{
    PlatformLayer* platformScrollLayer = graphicsLayer ? graphicsLayer->platformLayer() : nil;
    if (m_counterScrollingPlatformLayer == platformScrollLayer)
        return;

    m_counterScrollingPlatformLayer = platformScrollLayer;
    m_counterScrollingLayer = graphicsLayer;

    setPropertyChanged(CounterScrollingLayer);
    if (m_scrollingStateTree)
        m_scrollingStateTree->setHasChangedProperties(true);
}

PlatformLayer* ScrollingStateScrollingNode::headerPlatformLayer() const
{
    return m_headerPlatformLayer.get();
}

void ScrollingStateScrollingNode::setHeaderLayer(GraphicsLayer* graphicsLayer)
{
    PlatformLayer* platformHeaderLayer = graphicsLayer ? graphicsLayer->platformLayer() : nil;
    if (m_headerPlatformLayer == platformHeaderLayer)
        return;

    m_headerPlatformLayer = platformHeaderLayer;
    m_headerLayer = graphicsLayer;

    setPropertyChanged(HeaderLayer);
    if (m_scrollingStateTree)
        m_scrollingStateTree->setHasChangedProperties(true);
}

PlatformLayer* ScrollingStateScrollingNode::footerPlatformLayer() const
{
    return m_footerPlatformLayer.get();
}

void ScrollingStateScrollingNode::setFooterLayer(GraphicsLayer* graphicsLayer)
{
    PlatformLayer* platformFooterLayer = graphicsLayer ? graphicsLayer->platformLayer() : nil;
    if (m_footerPlatformLayer == platformFooterLayer)
        return;

    m_footerPlatformLayer = platformFooterLayer;
    m_footerLayer = graphicsLayer;

    setPropertyChanged(FooterLayer);
    if (m_scrollingStateTree)
        m_scrollingStateTree->setHasChangedProperties(true);
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
