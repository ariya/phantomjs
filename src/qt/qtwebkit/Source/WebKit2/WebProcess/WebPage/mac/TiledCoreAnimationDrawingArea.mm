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
#import "TiledCoreAnimationDrawingArea.h"

#if ENABLE(THREADED_SCROLLING)

#import "ColorSpaceData.h"
#import "DrawingAreaProxyMessages.h"
#import "EventDispatcher.h"
#import "LayerHostingContext.h"
#import "LayerTreeContext.h"
#import "WebFrame.h"
#import "WebPage.h"
#import "WebPageCreationParameters.h"
#import "WebPageProxyMessages.h"
#import "WebProcess.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/GraphicsLayerCA.h>
#import <WebCore/Page.h>
#import <WebCore/RenderView.h>
#import <WebCore/ScrollingCoordinator.h>
#import <WebCore/ScrollingThread.h>
#import <WebCore/ScrollingTree.h>
#import <WebCore/Settings.h>
#import <WebCore/TiledBacking.h>
#import <wtf/MainThread.h>

@interface CATransaction (Details)
+ (void)synchronize;
@end

using namespace WebCore;

namespace WebKit {

PassOwnPtr<TiledCoreAnimationDrawingArea> TiledCoreAnimationDrawingArea::create(WebPage* webPage, const WebPageCreationParameters& parameters)
{
    return adoptPtr(new TiledCoreAnimationDrawingArea(webPage, parameters));
}

TiledCoreAnimationDrawingArea::TiledCoreAnimationDrawingArea(WebPage* webPage, const WebPageCreationParameters& parameters)
    : DrawingArea(DrawingAreaTypeTiledCoreAnimation, webPage)
    , m_layerTreeStateIsFrozen(false)
    , m_layerFlushScheduler(this)
    , m_isPaintingSuspended(!parameters.isVisible)
    , m_clipsToExposedRect(false)
    , m_updateIntrinsicContentSizeTimer(this, &TiledCoreAnimationDrawingArea::updateIntrinsicContentSizeTimerFired)
{
    Page* page = m_webPage->corePage();

    page->settings()->setScrollingCoordinatorEnabled(true);
    page->settings()->setForceCompositingMode(true);

    WebProcess::shared().eventDispatcher().addScrollingTreeForPage(webPage);

    m_rootLayer = [CALayer layer];

    CGRect rootLayerFrame = m_webPage->bounds();
    m_rootLayer.get().frame = rootLayerFrame;
    m_rootLayer.get().opaque = YES;
    m_rootLayer.get().geometryFlipped = YES;

    updateLayerHostingContext();
    setColorSpace(parameters.colorSpace);

    LayerTreeContext layerTreeContext;
    layerTreeContext.contextID = m_layerHostingContext->contextID();
    m_webPage->send(Messages::DrawingAreaProxy::EnterAcceleratedCompositingMode(0, layerTreeContext));
}

TiledCoreAnimationDrawingArea::~TiledCoreAnimationDrawingArea()
{
    WebProcess::shared().eventDispatcher().removeScrollingTreeForPage(m_webPage);

    m_layerFlushScheduler.invalidate();
}

void TiledCoreAnimationDrawingArea::setNeedsDisplay()
{
}

void TiledCoreAnimationDrawingArea::setNeedsDisplayInRect(const IntRect& rect)
{
}

void TiledCoreAnimationDrawingArea::scroll(const IntRect& scrollRect, const IntSize& scrollDelta)
{
    updateScrolledExposedRect();
}

void TiledCoreAnimationDrawingArea::invalidateAllPageOverlays()
{
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it)
        it->value->setNeedsDisplay();
}

void TiledCoreAnimationDrawingArea::didChangeScrollOffsetForAnyFrame()
{
    invalidateAllPageOverlays();
}

void TiledCoreAnimationDrawingArea::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    CALayer *rootCompositingLayer = graphicsLayer ? graphicsLayer->platformLayer() : nil;

    if (m_layerTreeStateIsFrozen) {
        m_pendingRootCompositingLayer = rootCompositingLayer;
        return;
    }

    setRootCompositingLayer(rootCompositingLayer);
}

void TiledCoreAnimationDrawingArea::forceRepaint()
{
    if (m_layerTreeStateIsFrozen)
        return;

    for (Frame* frame = m_webPage->corePage()->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* frameView = frame->view();
        if (!frameView || !frameView->tiledBacking())
            continue;

        frameView->tiledBacking()->forceRepaint();
    }

    flushLayers();
    [CATransaction flush];
    [CATransaction synchronize];
}

bool TiledCoreAnimationDrawingArea::forceRepaintAsync(uint64_t callbackID)
{
    if (m_layerTreeStateIsFrozen)
        return false;

    dispatchAfterEnsuringUpdatedScrollPosition(bind(^{
        m_webPage->drawingArea()->forceRepaint();
        m_webPage->send(Messages::WebPageProxy::VoidCallback(callbackID));
    }));
    return true;
}

void TiledCoreAnimationDrawingArea::setLayerTreeStateIsFrozen(bool layerTreeStateIsFrozen)
{
    if (m_layerTreeStateIsFrozen == layerTreeStateIsFrozen)
        return;

    m_layerTreeStateIsFrozen = layerTreeStateIsFrozen;
    if (m_layerTreeStateIsFrozen)
        m_layerFlushScheduler.suspend();
    else
        m_layerFlushScheduler.resume();
}

bool TiledCoreAnimationDrawingArea::layerTreeStateIsFrozen() const
{
    return m_layerTreeStateIsFrozen;
}

void TiledCoreAnimationDrawingArea::scheduleCompositingLayerFlush()
{
    m_layerFlushScheduler.schedule();
}

void TiledCoreAnimationDrawingArea::didInstallPageOverlay(PageOverlay* pageOverlay)
{
    m_webPage->corePage()->scrollingCoordinator()->setForceMainThreadScrollLayerPositionUpdates(true);

    createPageOverlayLayer(pageOverlay);
}

void TiledCoreAnimationDrawingArea::didUninstallPageOverlay(PageOverlay* pageOverlay)
{
    destroyPageOverlayLayer(pageOverlay);
    scheduleCompositingLayerFlush();

    if (m_pageOverlayLayers.size())
        return;

    if (Page* page = m_webPage->corePage())
        page->scrollingCoordinator()->setForceMainThreadScrollLayerPositionUpdates(false);
}

void TiledCoreAnimationDrawingArea::setPageOverlayNeedsDisplay(PageOverlay* pageOverlay, const IntRect& rect)
{
    GraphicsLayer* layer = m_pageOverlayLayers.get(pageOverlay);

    if (!layer)
        return;

    if (!layer->drawsContent()) {
        layer->setDrawsContent(true);
        layer->setSize(expandedIntSize(FloatSize(m_rootLayer.get().frame.size)));
    }

    layer->setNeedsDisplayInRect(rect);
    scheduleCompositingLayerFlush();
}

void TiledCoreAnimationDrawingArea::updatePreferences(const WebPreferencesStore&)
{
    Settings* settings = m_webPage->corePage()->settings();
    bool scrollingPerformanceLoggingEnabled = m_webPage->scrollingPerformanceLoggingEnabled();
    ScrollingThread::dispatch(bind(&ScrollingTree::setScrollingPerformanceLoggingEnabled, m_webPage->corePage()->scrollingCoordinator()->scrollingTree(), scrollingPerformanceLoggingEnabled));

    if (TiledBacking* tiledBacking = mainFrameTiledBacking())
        tiledBacking->setAggressivelyRetainsTiles(settings->aggressiveTileRetentionEnabled());

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it) {
        it->value->setAcceleratesDrawing(settings->acceleratedDrawingEnabled());
        it->value->setShowDebugBorder(settings->showDebugBorders());
        it->value->setShowRepaintCounter(settings->showRepaintCounter());
    }

    // Soon we want pages with fixed positioned elements to be able to be scrolled by the ScrollingCoordinator.
    // As a part of that work, we have to composite fixed position elements, and we have to allow those
    // elements to create a stacking context.
    settings->setAcceleratedCompositingForFixedPositionEnabled(true);
    settings->setFixedPositionCreatesStackingContext(true);

    bool showTiledScrollingIndicator = settings->showTiledScrollingIndicator();
    if (showTiledScrollingIndicator == !!m_debugInfoLayer)
        return;

    updateDebugInfoLayer(showTiledScrollingIndicator);
}

void TiledCoreAnimationDrawingArea::mainFrameContentSizeChanged(const IntSize&)
{
    if (!m_webPage->minimumLayoutSize().width())
        return;

    if (m_inUpdateGeometry)
        return;

    if (!m_updateIntrinsicContentSizeTimer.isActive())
        m_updateIntrinsicContentSizeTimer.startOneShot(0);
}

void TiledCoreAnimationDrawingArea::updateIntrinsicContentSizeTimerFired(Timer<TiledCoreAnimationDrawingArea>*)
{
    Frame* frame = m_webPage->corePage()->mainFrame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    IntSize contentSize = frameView->contentsSize();

    if (m_lastSentIntrinsicContentSize == contentSize)
        return;

    m_lastSentIntrinsicContentSize = contentSize;
    m_webPage->send(Messages::DrawingAreaProxy::IntrinsicContentSizeDidChange(contentSize));
}

void TiledCoreAnimationDrawingArea::dispatchAfterEnsuringUpdatedScrollPosition(const Function<void ()>& functionRef)
{
    m_webPage->ref();
    m_webPage->corePage()->scrollingCoordinator()->commitTreeStateIfNeeded();

    if (!m_layerTreeStateIsFrozen)
        m_layerFlushScheduler.suspend();

    Function<void ()> function = functionRef;

    // It is possible for the drawing area to be destroyed before the bound block
    // is invoked, so grab a reference to the web page here so we can access the drawing area through it.
    // (The web page is already kept alive by dispatchAfterEnsuringUpdatedScrollPosition).
    WebPage* webPage = m_webPage;

    ScrollingThread::dispatchBarrier(bind(^{
        DrawingArea* drawingArea = webPage->drawingArea();
        if (!drawingArea)
            return;

        function();

        if (!m_layerTreeStateIsFrozen)
            m_layerFlushScheduler.resume();

        webPage->deref();
    }));
}

void TiledCoreAnimationDrawingArea::notifyAnimationStarted(const GraphicsLayer*, double)
{
}

void TiledCoreAnimationDrawingArea::notifyFlushRequired(const GraphicsLayer*)
{
}

void TiledCoreAnimationDrawingArea::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, GraphicsLayerPaintingPhase, const IntRect& clipRect)
{
    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it) {
        if (it->value == graphicsLayer) {
            m_webPage->drawPageOverlay(it->key, graphicsContext, clipRect);
            break;
        }
    }
}

float TiledCoreAnimationDrawingArea::deviceScaleFactor() const
{
    return m_webPage->corePage()->deviceScaleFactor();
}

bool TiledCoreAnimationDrawingArea::flushLayers()
{
    ASSERT(!m_layerTreeStateIsFrozen);

    // This gets called outside of the normal event loop so wrap in an autorelease pool
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    m_webPage->layoutIfNeeded();

    if (m_pendingRootCompositingLayer) {
        setRootCompositingLayer(m_pendingRootCompositingLayer.get());
        m_pendingRootCompositingLayer = nullptr;
    }

    IntRect visibleRect = enclosingIntRect(m_rootLayer.get().frame);
    if (m_clipsToExposedRect)
        visibleRect.intersect(enclosingIntRect(m_scrolledExposedRect));

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it) {
        GraphicsLayer* layer = it->value.get();
        layer->flushCompositingState(visibleRect);
    }

    bool returnValue = m_webPage->corePage()->mainFrame()->view()->flushCompositingStateIncludingSubframes();

    [pool drain];
    return returnValue;
}

void TiledCoreAnimationDrawingArea::suspendPainting()
{
    ASSERT(!m_isPaintingSuspended);
    m_isPaintingSuspended = true;

    [m_rootLayer.get() setValue:(id)kCFBooleanTrue forKey:@"NSCAViewRenderPaused"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"NSCAViewRenderDidPauseNotification" object:nil userInfo:[NSDictionary dictionaryWithObject:m_rootLayer.get() forKey:@"layer"]];

    m_webPage->corePage()->suspendScriptedAnimations();
}

void TiledCoreAnimationDrawingArea::resumePainting()
{
    if (!m_isPaintingSuspended) {
        // FIXME: We can get a call to resumePainting when painting is not suspended.
        // This happens when sending a synchronous message to create a new page. See <rdar://problem/8976531>.
        return;
    }
    m_isPaintingSuspended = false;

    [m_rootLayer.get() setValue:(id)kCFBooleanFalse forKey:@"NSCAViewRenderPaused"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"NSCAViewRenderDidResumeNotification" object:nil userInfo:[NSDictionary dictionaryWithObject:m_rootLayer.get() forKey:@"layer"]];

    if (m_webPage->windowIsVisible()) {
        m_webPage->corePage()->resumeScriptedAnimations();

        Frame* frame = m_webPage->corePage()->mainFrame();
        if (!frame)
            return;

        FrameView* frameView = frame->view();
        if (!frameView)
            return;

        frameView->resumeAnimatingImages();
    }
}

void TiledCoreAnimationDrawingArea::setExposedRect(const FloatRect& exposedRect)
{
    m_exposedRect = exposedRect;
    updateScrolledExposedRect();
}

void TiledCoreAnimationDrawingArea::setClipsToExposedRect(bool clipsToExposedRect)
{
    m_clipsToExposedRect = clipsToExposedRect;
    updateScrolledExposedRect();
    updateMainFrameClipsToExposedRect();
}

void TiledCoreAnimationDrawingArea::updateScrolledExposedRect()
{
    if (!m_clipsToExposedRect)
        return;

    Frame* frame = m_webPage->corePage()->mainFrame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    IntPoint scrollPositionWithOrigin = frameView->scrollPosition() + toIntSize(frameView->scrollOrigin());

    m_scrolledExposedRect = m_exposedRect;
    m_scrolledExposedRect.moveBy(scrollPositionWithOrigin);

    mainFrameTiledBacking()->setExposedRect(m_scrolledExposedRect);

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it) {
        if (TiledBacking* tiledBacking = it->value->tiledBacking())
            tiledBacking->setExposedRect(m_scrolledExposedRect);
    }
}

void TiledCoreAnimationDrawingArea::updateGeometry(const IntSize& viewSize, const IntSize& layerPosition)
{
    m_inUpdateGeometry = true;

    IntSize size = viewSize;
    IntSize contentSize = IntSize(-1, -1);

    if (!m_webPage->minimumLayoutSize().width())
        m_webPage->setSize(size);

    m_webPage->layoutIfNeeded();

    if (m_webPage->minimumLayoutSize().width()) {
        contentSize = m_webPage->mainWebFrame()->contentBounds().size();
        size = contentSize;
    }

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it) {
        GraphicsLayer* layer = it->value.get();
        if (layer->drawsContent())
            layer->setSize(viewSize);
    }

    if (!m_layerTreeStateIsFrozen)
        flushLayers();

    invalidateAllPageOverlays();

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    m_rootLayer.get().frame = CGRectMake(layerPosition.width(), layerPosition.height(), viewSize.width(), viewSize.height());

    [CATransaction commit];
    
    [CATransaction flush];
    [CATransaction synchronize];

    m_webPage->send(Messages::DrawingAreaProxy::DidUpdateGeometry());

    if (m_webPage->minimumLayoutSize().width() && !m_updateIntrinsicContentSizeTimer.isActive())
        m_updateIntrinsicContentSizeTimer.startOneShot(0);

    m_inUpdateGeometry = false;
}

void TiledCoreAnimationDrawingArea::setDeviceScaleFactor(float deviceScaleFactor)
{
    m_webPage->setDeviceScaleFactor(deviceScaleFactor);

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it)
        it->value->noteDeviceOrPageScaleFactorChangedIncludingDescendants();
}

void TiledCoreAnimationDrawingArea::setLayerHostingMode(uint32_t opaqueLayerHostingMode)
{
    LayerHostingMode layerHostingMode = static_cast<LayerHostingMode>(opaqueLayerHostingMode);
    if (layerHostingMode == m_webPage->layerHostingMode())
        return;

    m_webPage->setLayerHostingMode(layerHostingMode);

    updateLayerHostingContext();

    // Finally, inform the UIProcess that the context has changed.
    LayerTreeContext layerTreeContext;
    layerTreeContext.contextID = m_layerHostingContext->contextID();
    m_webPage->send(Messages::DrawingAreaProxy::UpdateAcceleratedCompositingMode(0, layerTreeContext));
}

void TiledCoreAnimationDrawingArea::setColorSpace(const ColorSpaceData& colorSpace)
{
    m_layerHostingContext->setColorSpace(colorSpace.cgColorSpace.get());
}

void TiledCoreAnimationDrawingArea::updateLayerHostingContext()
{
    RetainPtr<CGColorSpaceRef> colorSpace;

    // Invalidate the old context.
    if (m_layerHostingContext) {
        colorSpace = m_layerHostingContext->colorSpace();
        m_layerHostingContext->invalidate();
        m_layerHostingContext = nullptr;
    }

    // Create a new context and set it up.
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

    if (m_hasRootCompositingLayer)
        m_layerHostingContext->setRootLayer(m_rootLayer.get());

    if (colorSpace)
        m_layerHostingContext->setColorSpace(colorSpace.get());
}

void TiledCoreAnimationDrawingArea::updateMainFrameClipsToExposedRect()
{
    if (TiledBacking* tiledBacking = mainFrameTiledBacking())
        tiledBacking->setClipsToExposedRect(m_clipsToExposedRect);

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it)
        if (TiledBacking* tiledBacking = it->value->tiledBacking())
            tiledBacking->setClipsToExposedRect(m_clipsToExposedRect);

    Frame* frame = m_webPage->corePage()->mainFrame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    frameView->adjustTiledBackingCoverage();
}

void TiledCoreAnimationDrawingArea::setRootCompositingLayer(CALayer *layer)
{
    ASSERT(!m_layerTreeStateIsFrozen);

    bool hadRootCompositingLayer = m_hasRootCompositingLayer;
    m_hasRootCompositingLayer = !!layer;

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    m_rootLayer.get().sublayers = m_hasRootCompositingLayer ? [NSArray arrayWithObject:layer] : [NSArray array];

    if (hadRootCompositingLayer != m_hasRootCompositingLayer)
        m_layerHostingContext->setRootLayer(m_hasRootCompositingLayer ? m_rootLayer.get() : 0);

    for (PageOverlayLayerMap::iterator it = m_pageOverlayLayers.begin(), end = m_pageOverlayLayers.end(); it != end; ++it)
        [m_rootLayer.get() addSublayer:it->value->platformLayer()];

    if (TiledBacking* tiledBacking = mainFrameTiledBacking()) {
        tiledBacking->setAggressivelyRetainsTiles(m_webPage->corePage()->settings()->aggressiveTileRetentionEnabled());
        tiledBacking->setExposedRect(m_scrolledExposedRect);
    }

    updateMainFrameClipsToExposedRect();

    updateDebugInfoLayer(m_webPage->corePage()->settings()->showTiledScrollingIndicator());

    [CATransaction commit];
}

void TiledCoreAnimationDrawingArea::createPageOverlayLayer(PageOverlay* pageOverlay)
{
    OwnPtr<GraphicsLayer> layer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
    layer->setName("page overlay content");
#endif

    layer->setAcceleratesDrawing(m_webPage->corePage()->settings()->acceleratedDrawingEnabled());
    layer->setShowDebugBorder(m_webPage->corePage()->settings()->showDebugBorders());
    layer->setShowRepaintCounter(m_webPage->corePage()->settings()->showRepaintCounter());

    m_pageOverlayPlatformLayers.set(layer.get(), layer->platformLayer());

    if (TiledBacking* tiledBacking = layer->tiledBacking()) {
        tiledBacking->setExposedRect(m_scrolledExposedRect);
        tiledBacking->setClipsToExposedRect(m_clipsToExposedRect);
    }

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    [m_rootLayer.get() addSublayer:layer->platformLayer()];

    [CATransaction commit];

    m_pageOverlayLayers.add(pageOverlay, layer.release());
}

void TiledCoreAnimationDrawingArea::destroyPageOverlayLayer(PageOverlay* pageOverlay)
{
    OwnPtr<GraphicsLayer> layer = m_pageOverlayLayers.take(pageOverlay);
    ASSERT(layer);

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    [layer->platformLayer() removeFromSuperlayer];

    [CATransaction commit];

    m_pageOverlayPlatformLayers.remove(layer.get());
}

void TiledCoreAnimationDrawingArea::didCommitChangesForLayer(const GraphicsLayer* layer) const
{
    RetainPtr<CALayer> oldPlatformLayer = m_pageOverlayPlatformLayers.get(layer);

    if (!oldPlatformLayer)
        return;

    if (oldPlatformLayer.get() == layer->platformLayer())
        return;

    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    [m_rootLayer.get() insertSublayer:layer->platformLayer() above:oldPlatformLayer.get()];
    [oldPlatformLayer.get() removeFromSuperlayer];

    [CATransaction commit];

    if (TiledBacking* tiledBacking = layer->tiledBacking()) {
        tiledBacking->setExposedRect(m_scrolledExposedRect);
        tiledBacking->setClipsToExposedRect(m_clipsToExposedRect);
    }

    m_pageOverlayPlatformLayers.set(layer, layer->platformLayer());
}

TiledBacking* TiledCoreAnimationDrawingArea::mainFrameTiledBacking() const
{
    Frame* frame = m_webPage->corePage()->mainFrame();
    if (!frame)
        return 0;
    
    FrameView* frameView = frame->view();
    return frameView ? frameView->tiledBacking() : 0;
}

void TiledCoreAnimationDrawingArea::updateDebugInfoLayer(bool showLayer)
{
    if (showLayer) {
        if (TiledBacking* tiledBacking = mainFrameTiledBacking())
            m_debugInfoLayer = tiledBacking->tiledScrollingIndicatorLayer();
        
        if (m_debugInfoLayer) {
#ifndef NDEBUG
            [m_debugInfoLayer.get() setName:@"Debug Info"];
#endif
            [m_rootLayer.get() addSublayer:m_debugInfoLayer.get()];
        }
    } else if (m_debugInfoLayer) {
        [m_debugInfoLayer.get() removeFromSuperlayer];
        m_debugInfoLayer = nullptr;
    }
}

} // namespace WebKit

#endif // ENABLE(THREADED_SCROLLING)
