/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2013 Company 100, Inc. All rights reserved.
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
#include "CompositingCoordinator.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InspectorController.h"
#include "Page.h"
#include "Settings.h"
#include <wtf/CurrentTime.h>
#include <wtf/TemporaryChange.h>

namespace WebCore {

PassOwnPtr<CompositingCoordinator> CompositingCoordinator::create(Page* page, CompositingCoordinator::Client* client)
{
    return adoptPtr(new CompositingCoordinator(page, client));
}

CompositingCoordinator::~CompositingCoordinator()
{
    purgeBackingStores();

    LayerMap::iterator end = m_registeredLayers.end();
    for (LayerMap::iterator it = m_registeredLayers.begin(); it != end; ++it)
        it->value->setCoordinator(0);
}

CompositingCoordinator::CompositingCoordinator(Page* page, CompositingCoordinator::Client* client)
    : m_page(page)
    , m_client(client)
    , m_rootCompositingLayer(0)
    , m_isPurging(false)
    , m_isFlushingLayerChanges(false)
    , m_shouldSyncFrame(false)
    , m_didInitializeRootCompositingLayer(false)
    , m_releaseInactiveAtlasesTimer(this, &CompositingCoordinator::releaseInactiveAtlasesTimerFired)
#if ENABLE(REQUEST_ANIMATION_FRAME)
    , m_lastAnimationServiceTime(0)
#endif
{
    m_page->settings()->setApplyDeviceScaleFactorInCompositor(true);

    // This is a temporary way to enable this only in the GL case, until TextureMapperImageBuffer is removed.
    // See https://bugs.webkit.org/show_bug.cgi?id=114869
    CoordinatedGraphicsLayer::setShouldSupportContentsTiling(true);
}

void CompositingCoordinator::setRootCompositingLayer(GraphicsLayer* layer)
{
    if (m_rootCompositingLayer)
        m_rootCompositingLayer->removeFromParent();

    m_rootCompositingLayer = layer;
    if (m_rootCompositingLayer)
        m_rootLayer->addChildAtIndex(m_rootCompositingLayer, 0);
}

void CompositingCoordinator::sizeDidChange(const IntSize& newSize)
{
    m_rootLayer->setSize(newSize);
    m_client->notifyFlushRequired();
}

bool CompositingCoordinator::flushPendingLayerChanges()
{
    TemporaryChange<bool> protector(m_isFlushingLayerChanges, true);

    initializeRootCompositingLayerIfNeeded();

    m_rootLayer->flushCompositingStateForThisLayerOnly();
    m_client->didFlushRootLayer();

    bool didSync = m_page->mainFrame()->view()->flushCompositingStateIncludingSubframes();

    toCoordinatedGraphicsLayer(m_rootLayer.get())->updateContentBuffersIncludingSubLayers();
    toCoordinatedGraphicsLayer(m_rootLayer.get())->syncPendingStateChangesIncludingSubLayers();

    flushPendingImageBackingChanges();

    if (m_shouldSyncFrame) {
        didSync = true;

        if (m_rootCompositingLayer) {
            m_state.contentsSize = roundedIntSize(m_rootCompositingLayer->size());
            if (CoordinatedGraphicsLayer* contentsLayer = mainContentsLayer())
                m_state.coveredRect = contentsLayer->coverRect();
        }
        m_state.scrollPosition = m_visibleContentsRect.location();

        m_client->commitSceneState(m_state);

        clearPendingStateChanges();
        m_shouldSyncFrame = false;
    }

    return didSync;
}

void CompositingCoordinator::syncDisplayState()
{
#if ENABLE(INSPECTOR)
    m_page->inspectorController()->didBeginFrame();
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER) && !USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    // Make sure that any previously registered animation callbacks are being executed before we flush the layers.
    m_lastAnimationServiceTime = WTF::monotonicallyIncreasingTime();
    m_page->mainFrame()->view()->serviceScriptedAnimations(m_lastAnimationServiceTime);
#endif
    m_page->mainFrame()->view()->updateLayoutAndStyleIfNeededRecursive();
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
double CompositingCoordinator::nextAnimationServiceTime() const
{
    // According to the requestAnimationFrame spec, rAF callbacks should not be faster than 60FPS.
    static const double MinimalTimeoutForAnimations = 1. / 60.;
    return std::max<double>(0., MinimalTimeoutForAnimations - WTF::monotonicallyIncreasingTime() + m_lastAnimationServiceTime);
}
#endif

void CompositingCoordinator::clearPendingStateChanges()
{
    m_state.layersToCreate.clear();
    m_state.layersToUpdate.clear();
    m_state.layersToRemove.clear();

    m_state.imagesToCreate.clear();
    m_state.imagesToRemove.clear();
    m_state.imagesToUpdate.clear();
    m_state.imagesToClear.clear();

    m_state.updateAtlasesToCreate.clear();
    m_state.updateAtlasesToRemove.clear();

#if ENABLE(CSS_SHADERS)
    m_state.customFiltersToCreate.clear();
    m_state.customFiltersToRemove.clear();
#endif
}

void CompositingCoordinator::initializeRootCompositingLayerIfNeeded()
{
    if (m_didInitializeRootCompositingLayer)
        return;

    m_state.rootCompositingLayer = toCoordinatedGraphicsLayer(m_rootLayer.get())->id();
    m_didInitializeRootCompositingLayer = true;
    m_shouldSyncFrame = true;
}

void CompositingCoordinator::createRootLayer(const IntSize& size)
{
    ASSERT(!m_rootLayer);
    // Create a root layer.
    m_rootLayer = GraphicsLayer::create(this, this);
#ifndef NDEBUG
    m_rootLayer->setName("CompositingCoordinator root layer");
#endif
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(size);
}

void CompositingCoordinator::syncLayerState(CoordinatedLayerID id, CoordinatedGraphicsLayerState& state)
{
    m_shouldSyncFrame = true;
    m_client->willSyncLayerState(state);
    m_state.layersToUpdate.append(std::make_pair(id, state));
}

PassRefPtr<CoordinatedImageBacking> CompositingCoordinator::createImageBackingIfNeeded(Image* image)
{
    CoordinatedImageBackingID imageID = CoordinatedImageBacking::getCoordinatedImageBackingID(image);
    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    RefPtr<CoordinatedImageBacking> imageBacking;
    if (it == m_imageBackings.end()) {
        imageBacking = CoordinatedImageBacking::create(this, image);
        m_imageBackings.add(imageID, imageBacking);
    } else
        imageBacking = it->value;

    return imageBacking;
}

void CompositingCoordinator::createImageBacking(CoordinatedImageBackingID imageID)
{
    m_state.imagesToCreate.append(imageID);
}

void CompositingCoordinator::updateImageBacking(CoordinatedImageBackingID imageID, PassRefPtr<CoordinatedSurface> coordinatedSurface)
{
    m_shouldSyncFrame = true;
    m_state.imagesToUpdate.append(std::make_pair(imageID, coordinatedSurface));
}

void CompositingCoordinator::clearImageBackingContents(CoordinatedImageBackingID imageID)
{
    m_shouldSyncFrame = true;
    m_state.imagesToClear.append(imageID);
}

void CompositingCoordinator::removeImageBacking(CoordinatedImageBackingID imageID)
{
    if (m_isPurging)
        return;

    ASSERT(m_imageBackings.contains(imageID));
    m_imageBackings.remove(imageID);

    m_state.imagesToRemove.append(imageID);

    size_t imageIDPosition = m_state.imagesToClear.find(imageID);
    if (imageIDPosition != notFound)
        m_state.imagesToClear.remove(imageIDPosition);
}

void CompositingCoordinator::flushPendingImageBackingChanges()
{
    ImageBackingMap::iterator end = m_imageBackings.end();
    for (ImageBackingMap::iterator iter = m_imageBackings.begin(); iter != end; ++iter)
        iter->value->update();
}

void CompositingCoordinator::notifyAnimationStarted(const GraphicsLayer*, double /* time */)
{
}

void CompositingCoordinator::notifyFlushRequired(const GraphicsLayer*)
{
    m_client->notifyFlushRequired();
}


void CompositingCoordinator::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& graphicsContext, GraphicsLayerPaintingPhase, const IntRect& clipRect)
{
    m_client->paintLayerContents(graphicsLayer, graphicsContext, clipRect);
}

PassOwnPtr<GraphicsLayer> CompositingCoordinator::createGraphicsLayer(GraphicsLayerClient* client)
{
    CoordinatedGraphicsLayer* layer = new CoordinatedGraphicsLayer(client);
    layer->setCoordinator(this);
    m_registeredLayers.add(layer->id(), layer);
    m_state.layersToCreate.append(layer->id());
    layer->setNeedsVisibleRectAdjustment();
    m_client->notifyFlushRequired();
    return adoptPtr(layer);
}

float CompositingCoordinator::deviceScaleFactor() const
{
    return m_page->deviceScaleFactor();
}

float CompositingCoordinator::pageScaleFactor() const
{
    return m_page->pageScaleFactor();
}

void CompositingCoordinator::createUpdateAtlas(uint32_t atlasID, PassRefPtr<CoordinatedSurface> coordinatedSurface)
{
    m_state.updateAtlasesToCreate.append(std::make_pair(atlasID, coordinatedSurface));
}

void CompositingCoordinator::removeUpdateAtlas(uint32_t atlasID)
{
    if (m_isPurging)
        return;
    m_state.updateAtlasesToRemove.append(atlasID);
}

FloatRect CompositingCoordinator::visibleContentsRect() const
{
    return m_visibleContentsRect;
}

CoordinatedGraphicsLayer* CompositingCoordinator::mainContentsLayer()
{
    if (!m_rootCompositingLayer)
        return 0;

    return toCoordinatedGraphicsLayer(m_rootCompositingLayer)->findFirstDescendantWithContentsRecursively();
}

void CompositingCoordinator::setVisibleContentsRect(const FloatRect& rect, const FloatPoint& trajectoryVector)
{
    // A zero trajectoryVector indicates that tiles all around the viewport are requested.
    if (CoordinatedGraphicsLayer* contentsLayer = mainContentsLayer())
        contentsLayer->setVisibleContentRectTrajectoryVector(trajectoryVector);

    bool contentsRectDidChange = rect != m_visibleContentsRect;
    if (contentsRectDidChange) {
        m_visibleContentsRect = rect;

        LayerMap::iterator end = m_registeredLayers.end();
        for (LayerMap::iterator it = m_registeredLayers.begin(); it != end; ++it)
            it->value->setNeedsVisibleRectAdjustment();
    }

    FrameView* view = m_page->mainFrame()->view();
    if (view->useFixedLayout()) {
        // Round the rect instead of enclosing it to make sure that its size stays
        // the same while panning. This can have nasty effects on layout.
        view->setFixedVisibleContentRect(roundedIntRect(rect));
    }
}

void CompositingCoordinator::deviceOrPageScaleFactorChanged()
{
    m_rootLayer->deviceOrPageScaleFactorChanged();
}

void CompositingCoordinator::detachLayer(CoordinatedGraphicsLayer* layer)
{
    if (m_isPurging)
        return;

    m_registeredLayers.remove(layer->id());

    size_t index = m_state.layersToCreate.find(layer->id());
    if (index != notFound) {
        m_state.layersToCreate.remove(index);
        return;
    }

    m_state.layersToRemove.append(layer->id());
    m_client->notifyFlushRequired();
}

void CompositingCoordinator::commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset)
{
    LayerMap::iterator i = m_registeredLayers.find(layerID);
    if (i == m_registeredLayers.end())
        return;

    i->value->commitScrollOffset(offset);
}

void CompositingCoordinator::renderNextFrame()
{
    for (unsigned i = 0; i < m_updateAtlases.size(); ++i)
        m_updateAtlases[i]->didSwapBuffers();
}

void CompositingCoordinator::purgeBackingStores()
{
    TemporaryChange<bool> purgingToggle(m_isPurging, true);

    LayerMap::iterator end = m_registeredLayers.end();
    for (LayerMap::iterator it = m_registeredLayers.begin(); it != end; ++it)
        it->value->purgeBackingStores();

    m_imageBackings.clear();
    m_updateAtlases.clear();
}

bool CompositingCoordinator::paintToSurface(const IntSize& size, CoordinatedSurface::Flags flags, uint32_t& atlasID, IntPoint& offset, CoordinatedSurface::Client* client)
{
    for (unsigned i = 0; i < m_updateAtlases.size(); ++i) {
        UpdateAtlas* atlas = m_updateAtlases[i].get();
        if (atlas->supportsAlpha() == (flags & CoordinatedSurface::SupportsAlpha)) {
            // This will be false if there is no available buffer space.
            if (atlas->paintOnAvailableBuffer(size, atlasID, offset, client))
                return true;
        }
    }

    static const int ScratchBufferDimension = 1024; // Should be a power of two.
    m_updateAtlases.append(adoptPtr(new UpdateAtlas(this, ScratchBufferDimension, flags)));
    scheduleReleaseInactiveAtlases();
    return m_updateAtlases.last()->paintOnAvailableBuffer(size, atlasID, offset, client);
}

const double ReleaseInactiveAtlasesTimerInterval = 0.5;

void CompositingCoordinator::scheduleReleaseInactiveAtlases()
{
    if (!m_releaseInactiveAtlasesTimer.isActive())
        m_releaseInactiveAtlasesTimer.startRepeating(ReleaseInactiveAtlasesTimerInterval);
}

void CompositingCoordinator::releaseInactiveAtlasesTimerFired(Timer<CompositingCoordinator>*)
{
    // We always want to keep one atlas for root contents layer.
    OwnPtr<UpdateAtlas> atlasToKeepAnyway;
    bool foundActiveAtlasForRootContentsLayer = false;
    for (int i = m_updateAtlases.size() - 1;  i >= 0; --i) {
        UpdateAtlas* atlas = m_updateAtlases[i].get();
        if (!atlas->isInUse())
            atlas->addTimeInactive(ReleaseInactiveAtlasesTimerInterval);
        bool usableForRootContentsLayer = !atlas->supportsAlpha();
        if (atlas->isInactive()) {
            if (!foundActiveAtlasForRootContentsLayer && !atlasToKeepAnyway && usableForRootContentsLayer)
                atlasToKeepAnyway = m_updateAtlases[i].release();
            m_updateAtlases.remove(i);
        } else if (usableForRootContentsLayer)
            foundActiveAtlasForRootContentsLayer = true;
    }

    if (!foundActiveAtlasForRootContentsLayer && atlasToKeepAnyway)
        m_updateAtlases.append(atlasToKeepAnyway.release());

    if (m_updateAtlases.size() <= 1)
        m_releaseInactiveAtlasesTimer.stop();
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
