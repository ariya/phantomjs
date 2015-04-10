/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "LayerTreeHostMac.h"

#import "DrawingAreaImpl.h"
#import "LayerHostingContext.h"
#import "WebPage.h"
#import "WebProcess.h"
#import <QuartzCore/CATransaction.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsLayerCA.h>
#import <WebCore/PlatformCALayer.h>
#import <WebCore/Settings.h>

using namespace WebCore;

@interface CATransaction (Details)
+ (void)synchronize;
@end

namespace WebKit {

PassRefPtr<LayerTreeHostMac> LayerTreeHostMac::create(WebPage* webPage)
{
    RefPtr<LayerTreeHostMac> host = adoptRef(new LayerTreeHostMac(webPage));
    host->initialize();
    return host.release();
}

LayerTreeHostMac::LayerTreeHostMac(WebPage* webPage)
    : LayerTreeHost(webPage)
    , m_isValid(true)
    , m_notifyAfterScheduledLayerFlush(false)
    , m_layerFlushScheduler(this)
{
}

LayerTreeHostMac::~LayerTreeHostMac()
{
    ASSERT(!m_isValid);
    ASSERT(!m_rootLayer);
    ASSERT(!m_layerHostingContext);
}

const LayerTreeContext& LayerTreeHostMac::layerTreeContext()
{
    return m_layerTreeContext;
}

void LayerTreeHostMac::scheduleLayerFlush()
{
    m_layerFlushScheduler.schedule();
}

void LayerTreeHostMac::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    if (layerFlushingEnabled)
        m_layerFlushScheduler.resume();
    else
        m_layerFlushScheduler.suspend();
}

void LayerTreeHostMac::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_notifyAfterScheduledLayerFlush = notifyAfterScheduledLayerFlush;
}

void LayerTreeHostMac::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    m_nonCompositedContentLayer->removeAllChildren();

    // Add the accelerated layer tree hierarchy.
    if (graphicsLayer)
        m_nonCompositedContentLayer->addChild(graphicsLayer);
}

void LayerTreeHostMac::invalidate()
{
    ASSERT(m_isValid);

    m_isValid = false;

    m_rootLayer = nullptr;

    m_layerHostingContext->invalidate();
    m_layerHostingContext = nullptr;
    m_layerFlushScheduler.invalidate();
}

void LayerTreeHostMac::setNonCompositedContentsNeedDisplay()
{
    m_nonCompositedContentLayer->setNeedsDisplay();

    PageOverlayLayerMap::iterator end = m_pageOverlayLayers.end();
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(); it != end; ++it)
        it->value->setNeedsDisplay();

    scheduleLayerFlush();
}

void LayerTreeHostMac::setNonCompositedContentsNeedDisplayInRect(const IntRect& rect)
{
    m_nonCompositedContentLayer->setNeedsDisplayInRect(rect);

    PageOverlayLayerMap::iterator end = m_pageOverlayLayers.end();
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(); it != end; ++it)
        it->value->setNeedsDisplayInRect(rect);

    scheduleLayerFlush();
}

void LayerTreeHostMac::scrollNonCompositedContents(const IntRect& scrollRect)
{
    setNonCompositedContentsNeedDisplayInRect(scrollRect);
}

void LayerTreeHostMac::forceRepaint()
{
    scheduleLayerFlush();
    flushPendingLayerChanges();

    [CATransaction flush];
    [CATransaction synchronize];
}

void LayerTreeHostMac::sizeDidChange(const IntSize& newSize)
{
    m_rootLayer->setSize(newSize);

    // If the newSize exposes new areas of the non-composited content a setNeedsDisplay is needed
    // for those newly exposed areas.
    FloatSize oldSize = m_nonCompositedContentLayer->size();
    m_nonCompositedContentLayer->setSize(newSize);

    if (newSize.width() > oldSize.width()) {
        float height = std::min(static_cast<float>(newSize.height()), oldSize.height());
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(oldSize.width(), 0, newSize.width() - oldSize.width(), height));
    }

    if (newSize.height() > oldSize.height())
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(0, oldSize.height(), newSize.width(), newSize.height() - oldSize.height()));

    PageOverlayLayerMap::iterator end = m_pageOverlayLayers.end();
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(); it != end; ++it)
        it->value->setSize(newSize);

    scheduleLayerFlush();
    flushPendingLayerChanges();

    [CATransaction flush];
    [CATransaction synchronize];
}

void LayerTreeHostMac::deviceOrPageScaleFactorChanged()
{
    // Other layers learn of the scale factor change via WebPage::setDeviceScaleFactor.
    m_nonCompositedContentLayer->deviceOrPageScaleFactorChanged();
}

void LayerTreeHostMac::pageBackgroundTransparencyChanged()
{
    m_nonCompositedContentLayer->setContentsOpaque(m_webPage->drawsBackground() && !m_webPage->drawsTransparentBackground());
}

void LayerTreeHostMac::didInstallPageOverlay(PageOverlay* pageOverlay)
{
    createPageOverlayLayer(pageOverlay);
    scheduleLayerFlush();
}

void LayerTreeHostMac::didUninstallPageOverlay(PageOverlay* pageOverlay)
{
    destroyPageOverlayLayer(pageOverlay);
    scheduleLayerFlush();
}

void LayerTreeHostMac::setPageOverlayNeedsDisplay(PageOverlay* pageOverlay, const IntRect& rect)
{
    GraphicsLayer* layer = m_pageOverlayLayers.get(pageOverlay);

    if (!layer)
        return;

    layer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void LayerTreeHostMac::pauseRendering()
{
    CALayer* root = m_rootLayer->platformLayer();
    [root setValue:(id)kCFBooleanTrue forKey:@"NSCAViewRenderPaused"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"NSCAViewRenderDidPauseNotification" object:nil userInfo:[NSDictionary dictionaryWithObject:root forKey:@"layer"]];
}

void LayerTreeHostMac::resumeRendering()
{
    CALayer* root = m_rootLayer->platformLayer();
    [root setValue:(id)kCFBooleanFalse forKey:@"NSCAViewRenderPaused"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"NSCAViewRenderDidResumeNotification" object:nil userInfo:[NSDictionary dictionaryWithObject:root forKey:@"layer"]];
}

void LayerTreeHostMac::setLayerHostingMode(LayerHostingMode layerHostingMode)
{
    if (layerHostingMode == m_layerHostingContext->layerHostingMode())
        return;

    // The mode has changed.

    // First, invalidate the old hosting context.
    m_layerHostingContext->invalidate();
    m_layerHostingContext = nullptr;

    // Create a new context and set it up.
    switch (layerHostingMode) {
    case LayerHostingModeDefault:
        m_layerHostingContext = LayerHostingContext::createForPort(WebProcess::shared().compositingRenderServerPort());
        break;
#if HAVE(LAYER_HOSTING_IN_WINDOW_SERVER)
    case LayerHostingModeInWindowServer:
        m_layerHostingContext = LayerHostingContext::createForWindowServer();        
        break;
#endif
    }

    m_layerHostingContext->setRootLayer(m_rootLayer->platformLayer());
    m_layerTreeContext.contextID = m_layerHostingContext->contextID();

    scheduleLayerFlush();
}

void LayerTreeHostMac::notifyAnimationStarted(const WebCore::GraphicsLayer*, double time)
{
}

void LayerTreeHostMac::notifyFlushRequired(const WebCore::GraphicsLayer*)
{
}

void LayerTreeHostMac::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, GraphicsLayerPaintingPhase, const IntRect& clipRect)
{
    if (graphicsLayer == m_nonCompositedContentLayer) {
        m_webPage->drawRect(graphicsContext, clipRect);
        return;
    }

    PageOverlayLayerMap::iterator end = m_pageOverlayLayers.end();
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(); it != end; ++it) {
        if (it->value == graphicsLayer) {
            m_webPage->drawPageOverlay(it->key, graphicsContext, clipRect);
            break;
        }
    }
}

float LayerTreeHostMac::deviceScaleFactor() const
{
    return m_webPage->corePage()->deviceScaleFactor();
}

bool LayerTreeHostMac::flushLayers()
{
    performScheduledLayerFlush();
    return true;
}

void LayerTreeHostMac::initialize()
{
    // Create a root layer.
    m_rootLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
    m_rootLayer->setName("LayerTreeHost root layer");
#endif
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(m_webPage->size());
    static_cast<GraphicsLayerCA*>(m_rootLayer.get())->platformCALayer()->setGeometryFlipped(true);

    m_nonCompositedContentLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
    static_cast<GraphicsLayerCA*>(m_nonCompositedContentLayer.get())->setAllowTiledLayer(false);
#ifndef NDEBUG
    m_nonCompositedContentLayer->setName("LayerTreeHost non-composited content");
#endif
    m_nonCompositedContentLayer->setDrawsContent(true);
    m_nonCompositedContentLayer->setContentsOpaque(m_webPage->drawsBackground() && !m_webPage->drawsTransparentBackground());
    m_nonCompositedContentLayer->setSize(m_webPage->size());
    if (m_webPage->corePage()->settings()->acceleratedDrawingEnabled())
        m_nonCompositedContentLayer->setAcceleratesDrawing(true);

    m_rootLayer->addChild(m_nonCompositedContentLayer.get());

    if (m_webPage->hasPageOverlay()) {
        PageOverlayList& pageOverlays = m_webPage->pageOverlays();
        PageOverlayList::iterator end = pageOverlays.end();
        for (PageOverlayList::iterator it = pageOverlays.begin(); it != end; ++it)
            createPageOverlayLayer(it->get());
    }

    switch (m_webPage->layerHostingMode()) {
        case LayerHostingModeDefault:
            m_layerHostingContext = LayerHostingContext::createForPort(WebProcess::shared().compositingRenderServerPort());
            break;
#if HAVE(LAYER_HOSTING_IN_WINDOW_SERVER)
        case LayerHostingModeInWindowServer:
            m_layerHostingContext = LayerHostingContext::createForWindowServer();
            break;
#endif
    }

    m_layerHostingContext->setRootLayer(m_rootLayer->platformLayer());
    m_layerTreeContext.contextID = m_layerHostingContext->contextID();

    setLayerFlushSchedulingEnabled(!m_webPage->drawingArea() || !m_webPage->drawingArea()->layerTreeStateIsFrozen());
    scheduleLayerFlush();
}

void LayerTreeHostMac::performScheduledLayerFlush()
{
    {
        RefPtr<LayerTreeHostMac> protect(this);
        m_webPage->layoutIfNeeded();

        if (!m_isValid)
            return;
    }

    if (!flushPendingLayerChanges())
        return;

    if (m_notifyAfterScheduledLayerFlush) {
        // Let the drawing area know that we've done a flush of the layer changes.
        static_cast<DrawingAreaImpl*>(m_webPage->drawingArea())->layerHostDidFlushLayers();
        m_notifyAfterScheduledLayerFlush = false;
    }
}

bool LayerTreeHostMac::flushPendingLayerChanges()
{
    if (m_layerFlushScheduler.isSuspended())
        return false;

    m_rootLayer->flushCompositingStateForThisLayerOnly();
    m_nonCompositedContentLayer->flushCompositingStateForThisLayerOnly();

    PageOverlayLayerMap::iterator end = m_pageOverlayLayers.end();
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(); it != end; ++it)
        it->value->flushCompositingStateForThisLayerOnly();

    return m_webPage->corePage()->mainFrame()->view()->flushCompositingStateIncludingSubframes();
}

void LayerTreeHostMac::createPageOverlayLayer(PageOverlay* pageOverlay)
{
    OwnPtr<GraphicsLayer> layer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
    layer->setName("LayerTreeHost page overlay content");
#endif

    layer->setAcceleratesDrawing(m_webPage->corePage()->settings()->acceleratedDrawingEnabled());
    layer->setDrawsContent(true);
    layer->setSize(m_webPage->size());
    layer->setShowDebugBorder(m_webPage->corePage()->settings()->showDebugBorders());
    layer->setShowRepaintCounter(m_webPage->corePage()->settings()->showRepaintCounter());

    m_rootLayer->addChild(layer.get());

    m_pageOverlayLayers.add(pageOverlay, layer.release());
}

void LayerTreeHostMac::destroyPageOverlayLayer(PageOverlay* pageOverlay)
{
    OwnPtr<GraphicsLayer> layer = m_pageOverlayLayers.take(pageOverlay);
    ASSERT(layer);

    layer->removeFromParent();
}

} // namespace WebKit
