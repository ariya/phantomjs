/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Company 100, Inc.
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

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedLayerTreeHost.h"

#include "CoordinatedGraphicsArgumentCoders.h"
#include "CoordinatedLayerTreeHostProxyMessages.h"
#include "DrawingAreaImpl.h"
#include "GraphicsContext.h"
#include "WebCoordinatedSurface.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/Settings.h>
#include <wtf/CurrentTime.h>

#if ENABLE(CSS_SHADERS)
#include "CustomFilterValidatedProgram.h"
#include "ValidatedCustomFilterOperation.h"
#endif

using namespace WebCore;

namespace WebKit {

PassRefPtr<CoordinatedLayerTreeHost> CoordinatedLayerTreeHost::create(WebPage* webPage)
{
    return adoptRef(new CoordinatedLayerTreeHost(webPage));
}

CoordinatedLayerTreeHost::~CoordinatedLayerTreeHost()
{
#if ENABLE(CSS_SHADERS)
    disconnectCustomFilterPrograms();
#endif
}

CoordinatedLayerTreeHost::CoordinatedLayerTreeHost(WebPage* webPage)
    : LayerTreeHost(webPage)
    , m_notifyAfterScheduledLayerFlush(false)
    , m_isValid(true)
    , m_isSuspended(false)
    , m_isWaitingForRenderer(true)
    , m_layerFlushTimer(this, &CoordinatedLayerTreeHost::layerFlushTimerFired)
    , m_layerFlushSchedulingEnabled(true)
    , m_forceRepaintAsyncCallbackID(0)
{
    m_coordinator = CompositingCoordinator::create(webPage->corePage(), this);

    m_coordinator->createRootLayer(webPage->size());
    m_layerTreeContext.coordinatedLayerID = toCoordinatedGraphicsLayer(m_coordinator->rootLayer())->id();

    CoordinatedSurface::setFactory(createCoordinatedSurface);

    if (webPage->hasPageOverlay())
        createPageOverlayLayer();

    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    if (m_layerFlushSchedulingEnabled == layerFlushingEnabled)
        return;

    m_layerFlushSchedulingEnabled = layerFlushingEnabled;

    if (m_layerFlushSchedulingEnabled) {
        scheduleLayerFlush();
        return;
    }

    cancelPendingLayerFlush();
}

void CoordinatedLayerTreeHost::scheduleLayerFlush()
{
    if (!m_layerFlushSchedulingEnabled)
        return;

    if (!m_layerFlushTimer.isActive() || m_layerFlushTimer.nextFireInterval() > 0)
        m_layerFlushTimer.startOneShot(0);
}

void CoordinatedLayerTreeHost::cancelPendingLayerFlush()
{
    m_layerFlushTimer.stop();
}

void CoordinatedLayerTreeHost::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_notifyAfterScheduledLayerFlush = notifyAfterScheduledLayerFlush;
}

void CoordinatedLayerTreeHost::setRootCompositingLayer(WebCore::GraphicsLayer* graphicsLayer)
{
    m_coordinator->setRootCompositingLayer(graphicsLayer);
}

void CoordinatedLayerTreeHost::invalidate()
{
    cancelPendingLayerFlush();

    ASSERT(m_isValid);
    m_coordinator->clearRootLayer();
    m_isValid = false;
}

void CoordinatedLayerTreeHost::forceRepaint()
{
    // This is necessary for running layout tests. Since in this case we are not waiting for a UIProcess to reply nicely.
    // Instead we are just triggering forceRepaint. But we still want to have the scripted animation callbacks being executed.
    m_coordinator->syncDisplayState();

    // We need to schedule another flush, otherwise the forced paint might cancel a later expected flush.
    // This is aligned with LayerTreeHostCA.
    scheduleLayerFlush();

    if (m_isWaitingForRenderer)
        return;

    m_coordinator->flushPendingLayerChanges();
}

bool CoordinatedLayerTreeHost::forceRepaintAsync(uint64_t callbackID)
{
    // We expect the UI process to not require a new repaint until the previous one has finished.
    ASSERT(!m_forceRepaintAsyncCallbackID);
    m_forceRepaintAsyncCallbackID = callbackID;
    scheduleLayerFlush();
    return true;
}

void CoordinatedLayerTreeHost::sizeDidChange(const WebCore::IntSize& newSize)
{
    m_coordinator->sizeDidChange(newSize);
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::didInstallPageOverlay(PageOverlay* pageOverlay)
{
    ASSERT(!m_pageOverlay);
    m_pageOverlay = pageOverlay;

    createPageOverlayLayer();
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::didUninstallPageOverlay(PageOverlay*)
{
    m_pageOverlay = 0;

    destroyPageOverlayLayer();
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::setPageOverlayNeedsDisplay(PageOverlay*, const WebCore::IntRect& rect)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::setPageOverlayOpacity(PageOverlay*, float value)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setOpacity(value);
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::setVisibleContentsRect(const FloatRect& rect, const FloatPoint& trajectoryVector)
{
    m_coordinator->setVisibleContentsRect(rect, trajectoryVector);
    scheduleLayerFlush();
}

void CoordinatedLayerTreeHost::renderNextFrame()
{
    m_isWaitingForRenderer = false;
    scheduleLayerFlush();
    m_coordinator->renderNextFrame();
}

void CoordinatedLayerTreeHost::purgeBackingStores()
{
    m_coordinator->purgeBackingStores();
}

void CoordinatedLayerTreeHost::willSyncLayerState(CoordinatedGraphicsLayerState& state)
{
#if ENABLE(CSS_SHADERS)
    if (state.animationsChanged)
        prepareCustomFilterProxiesForAnimations(state.animations);

    if (state.filtersChanged)
        checkCustomFilterProgramProxies(state.filters);
#else
    UNUSED_PARAM(state);
#endif
}

#if ENABLE(CSS_SHADERS)
void CoordinatedLayerTreeHost::prepareCustomFilterProxiesForAnimations(GraphicsLayerAnimations& activeAnimations)
{
    for (size_t i = 0; i < activeAnimations.animations().size(); ++i) {
        const KeyframeValueList& keyframes = activeAnimations.animations().at(i).keyframes();
        if (keyframes.property() != AnimatedPropertyWebkitFilter)
            continue;
        for (size_t j = 0; j < keyframes.size(); ++j) {
            const FilterAnimationValue& filterValue = static_cast<const FilterAnimationValue&>(keyframes.at(j));
            checkCustomFilterProgramProxies(filterValue.value());
        }
    }
}

void CoordinatedLayerTreeHost::checkCustomFilterProgramProxies(const FilterOperations& filters)
{
    // We need to create the WebCustomFilterProgramProxy objects before we get to serialize the
    // custom filters to the other process. That's because WebCustomFilterProgramProxy needs
    // to link back to the coordinator, so that we can send a message to the UI process when
    // the program is not needed anymore.
    // Note that the serialization will only happen at a later time in ArgumentCoder<WebCore::FilterOperations>::encode.
    // At that point the program will only be serialized once. All the other times it will only use the ID of the program.
    for (size_t i = 0; i < filters.size(); ++i) {
        const FilterOperation* operation = filters.at(i);
        if (operation->getOperationType() != FilterOperation::VALIDATED_CUSTOM)
            continue;
        const ValidatedCustomFilterOperation* customOperation = static_cast<const ValidatedCustomFilterOperation*>(operation);
        ASSERT(customOperation->validatedProgram()->isInitialized());
        TextureMapperPlatformCompiledProgram* program = customOperation->validatedProgram()->platformCompiledProgram();

        RefPtr<WebCustomFilterProgramProxy> customFilterProgramProxy;
        if (program->client())
            customFilterProgramProxy = static_cast<WebCustomFilterProgramProxy*>(program->client());
        else {
            customFilterProgramProxy = WebCustomFilterProgramProxy::create();
            program->setClient(customFilterProgramProxy);
        }

        if (!customFilterProgramProxy->client()) {
            customFilterProgramProxy->setClient(this);
            m_customFilterPrograms.add(customFilterProgramProxy.get());
            m_coordinator->state().customFiltersToCreate.append(std::make_pair(customFilterProgramProxy->id(), customOperation->validatedProgram()->validatedProgramInfo()));
        } else {
            // If the client was not disconnected then this coordinator must be the client for it.
            ASSERT(customFilterProgramProxy->client() == this);
        }
    }
}

void CoordinatedLayerTreeHost::removeCustomFilterProgramProxy(WebCustomFilterProgramProxy* customFilterProgramProxy)
{
    // At this time the shader is not needed anymore, so we remove it from our set and
    // send a message to the other process to delete it.
    m_customFilterPrograms.remove(customFilterProgramProxy);
    m_coordinator->state().customFiltersToRemove.append(customFilterProgramProxy->id());
}

void CoordinatedLayerTreeHost::disconnectCustomFilterPrograms()
{
    // Make sure that WebCore will not call into this coordinator anymore.
    HashSet<WebCustomFilterProgramProxy*>::iterator iter = m_customFilterPrograms.begin();
    for (; iter != m_customFilterPrograms.end(); ++iter)
        (*iter)->setClient(0);
}
#endif // ENABLE(CSS_SHADERS)

void CoordinatedLayerTreeHost::didFlushRootLayer()
{
    if (m_pageOverlayLayer)
        m_pageOverlayLayer->flushCompositingStateForThisLayerOnly();
}

void CoordinatedLayerTreeHost::performScheduledLayerFlush()
{
    if (m_isSuspended || m_isWaitingForRenderer)
        return;

    m_coordinator->syncDisplayState();

    if (!m_isValid)
        return;

    bool didSync = m_coordinator->flushPendingLayerChanges();

    if (m_forceRepaintAsyncCallbackID) {
        m_webPage->send(Messages::WebPageProxy::VoidCallback(m_forceRepaintAsyncCallbackID));
        m_forceRepaintAsyncCallbackID = 0;
    }

    if (m_notifyAfterScheduledLayerFlush && didSync) {
        static_cast<DrawingAreaImpl*>(m_webPage->drawingArea())->layerHostDidFlushLayers();
        m_notifyAfterScheduledLayerFlush = false;
    }
}

void CoordinatedLayerTreeHost::layerFlushTimerFired(Timer<CoordinatedLayerTreeHost>*)
{
    performScheduledLayerFlush();
}

void CoordinatedLayerTreeHost::createPageOverlayLayer()
{
    ASSERT(!m_pageOverlayLayer);

    m_pageOverlayLayer = GraphicsLayer::create(graphicsLayerFactory(), m_coordinator.get());
#ifndef NDEBUG
    m_pageOverlayLayer->setName("CompositingCoordinator page overlay content");
#endif

    m_pageOverlayLayer->setDrawsContent(true);
    m_pageOverlayLayer->setSize(m_coordinator->rootLayer()->size());

    m_coordinator->rootLayer()->addChild(m_pageOverlayLayer.get());
}

void CoordinatedLayerTreeHost::destroyPageOverlayLayer()
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->removeFromParent();
    m_pageOverlayLayer = nullptr;
}

void CoordinatedLayerTreeHost::paintLayerContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, const IntRect& clipRect)
{
    if (graphicsLayer == m_pageOverlayLayer) {
        // Overlays contain transparent contents and won't clear the context as part of their rendering, so we do it here.
        graphicsContext.clearRect(clipRect);
        m_webPage->drawPageOverlay(m_pageOverlay.get(), graphicsContext, clipRect);
        return;
    }
}

void CoordinatedLayerTreeHost::commitSceneState(const WebCore::CoordinatedGraphicsState& state)
{
    m_webPage->send(Messages::CoordinatedLayerTreeHostProxy::CommitCoordinatedGraphicsState(state));
    m_isWaitingForRenderer = true;
}

PassRefPtr<CoordinatedSurface> CoordinatedLayerTreeHost::createCoordinatedSurface(const IntSize& size, CoordinatedSurface::Flags flags)
{
    return WebCoordinatedSurface::create(size, flags);
}

bool LayerTreeHost::supportsAcceleratedCompositing()
{
    return true;
}

void CoordinatedLayerTreeHost::deviceOrPageScaleFactorChanged()
{
    m_coordinator->deviceOrPageScaleFactorChanged();
    if (m_pageOverlayLayer)
        m_pageOverlayLayer->deviceOrPageScaleFactorChanged();
}

void CoordinatedLayerTreeHost::pageBackgroundTransparencyChanged()
{
}

GraphicsLayerFactory* CoordinatedLayerTreeHost::graphicsLayerFactory()
{
    return m_coordinator.get();
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void CoordinatedLayerTreeHost::scheduleAnimation()
{
    if (m_isWaitingForRenderer)
        return;

    if (m_layerFlushTimer.isActive())
        return;

    m_layerFlushTimer.startOneShot(m_coordinator->nextAnimationServiceTime());
    scheduleLayerFlush();
}
#endif

void CoordinatedLayerTreeHost::setBackgroundColor(const WebCore::Color& color)
{
    m_webPage->send(Messages::CoordinatedLayerTreeHostProxy::SetBackgroundColor(color));
}

void CoordinatedLayerTreeHost::commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset)
{
    m_coordinator->commitScrollOffset(layerID, offset);
}

} // namespace WebKit
#endif // USE(COORDINATED_GRAPHICS)
