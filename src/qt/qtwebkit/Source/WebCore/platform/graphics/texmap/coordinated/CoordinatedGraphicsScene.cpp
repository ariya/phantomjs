/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2012 Company 100, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if USE(COORDINATED_GRAPHICS)

#include "CoordinatedGraphicsScene.h"

#include "CoordinatedBackingStore.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"
#include <OpenGLShims.h>
#include <wtf/Atomics.h>
#include <wtf/MainThread.h>

#if ENABLE(CSS_SHADERS)
#include "CoordinatedCustomFilterOperation.h"
#include "CoordinatedCustomFilterProgram.h"
#include "CustomFilterProgram.h"
#include "CustomFilterProgramInfo.h"
#endif

namespace WebCore {

void CoordinatedGraphicsScene::dispatchOnMainThread(const Function<void()>& function)
{
    if (isMainThread())
        function();
    else
        callOnMainThread(function);
}

static bool layerShouldHaveBackingStore(TextureMapperLayer* layer)
{
    return layer->drawsContent() && layer->contentsAreVisible() && !layer->size().isEmpty();
}

CoordinatedGraphicsScene::CoordinatedGraphicsScene(CoordinatedGraphicsSceneClient* client)
    : m_client(client)
    , m_isActive(false)
    , m_rootLayerID(InvalidCoordinatedLayerID)
    , m_backgroundColor(Color::white)
    , m_setDrawsBackground(false)
{
    ASSERT(isMainThread());
}

CoordinatedGraphicsScene::~CoordinatedGraphicsScene()
{
}

void CoordinatedGraphicsScene::paintToCurrentGLContext(const TransformationMatrix& matrix, float opacity, const FloatRect& clipRect, TextureMapper::PaintFlags PaintFlags)
{
    if (!m_textureMapper) {
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
        static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
    }

    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);
    syncRemoteContent();

    adjustPositionForFixedLayers();
    TextureMapperLayer* currentRootLayer = rootLayer();
    if (!currentRootLayer)
        return;

    TextureMapperLayer* layer = currentRootLayer;

    if (!layer)
        return;

    layer->setTextureMapper(m_textureMapper.get());
    layer->applyAnimationsRecursively();
    m_textureMapper->beginPainting(PaintFlags);
    m_textureMapper->beginClip(TransformationMatrix(), clipRect);

    if (m_setDrawsBackground) {
        RGBA32 rgba = makeRGBA32FromFloats(m_backgroundColor.red(),
            m_backgroundColor.green(), m_backgroundColor.blue(),
            m_backgroundColor.alpha() * opacity);
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), Color(rgba));
    }

    if (currentRootLayer->opacity() != opacity || currentRootLayer->transform() != matrix) {
        currentRootLayer->setOpacity(opacity);
        currentRootLayer->setTransform(matrix);
    }

    layer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location(), matrix);
    m_textureMapper->endClip();
    m_textureMapper->endPainting();

    if (layer->descendantsOrSelfHaveRunningAnimations())
        dispatchOnMainThread(bind(&CoordinatedGraphicsScene::updateViewport, this));
}

void CoordinatedGraphicsScene::paintToGraphicsContext(PlatformGraphicsContext* platformContext)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create();
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::SoftwareMode);
    syncRemoteContent();
    TextureMapperLayer* layer = rootLayer();

    if (!layer)
        return;

    GraphicsContext graphicsContext(platformContext);
    m_textureMapper->setGraphicsContext(&graphicsContext);
    m_textureMapper->beginPainting();

    IntRect clipRect = graphicsContext.clipBounds();
    if (m_setDrawsBackground)
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);

    layer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location());
    m_textureMapper->endPainting();
    m_textureMapper->setGraphicsContext(0);
}

void CoordinatedGraphicsScene::setScrollPosition(const FloatPoint& scrollPosition)
{
    m_scrollPosition = scrollPosition;
}

void CoordinatedGraphicsScene::updateViewport()
{
    ASSERT(isMainThread());
    if (m_client)
        m_client->updateViewport();
}

void CoordinatedGraphicsScene::adjustPositionForFixedLayers()
{
    if (m_fixedLayers.isEmpty())
        return;

    // Fixed layer positions are updated by the web process when we update the visible contents rect / scroll position.
    // If we want those layers to follow accurately the viewport when we move between the web process updates, we have to offset
    // them by the delta between the current position and the position of the viewport used for the last layout.
    FloatSize delta = m_scrollPosition - m_renderedContentsScrollPosition;

    LayerRawPtrMap::iterator end = m_fixedLayers.end();
    for (LayerRawPtrMap::iterator it = m_fixedLayers.begin(); it != end; ++it)
        it->value->setScrollPositionDeltaIfNeeded(delta);
}

#if USE(GRAPHICS_SURFACE)
void CoordinatedGraphicsScene::createCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.canvasToken.isValid())
        return;

    RefPtr<TextureMapperSurfaceBackingStore> canvasBackingStore(TextureMapperSurfaceBackingStore::create());
    m_surfaceBackingStores.set(layer, canvasBackingStore);
    canvasBackingStore->setGraphicsSurface(GraphicsSurface::create(state.canvasSize, state.canvasSurfaceFlags, state.canvasToken));
    layer->setContentsLayer(canvasBackingStore.get());
}

void CoordinatedGraphicsScene::syncCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    ASSERT(m_textureMapper);

    if (state.canvasChanged) {
        destroyCanvasIfNeeded(layer, state);
        createCanvasIfNeeded(layer, state);
    }

    if (state.canvasShouldSwapBuffers) {
        ASSERT(m_surfaceBackingStores.contains(layer));
        SurfaceBackingStoreMap::iterator it = m_surfaceBackingStores.find(layer);
        RefPtr<TextureMapperSurfaceBackingStore> canvasBackingStore = it->value;
        canvasBackingStore->swapBuffersIfNeeded(state.canvasFrontBuffer);
    }
}

void CoordinatedGraphicsScene::destroyCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.canvasToken.isValid())
        return;

    m_surfaceBackingStores.remove(layer);
    layer->setContentsLayer(0);
}
#endif

void CoordinatedGraphicsScene::setLayerRepaintCountIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!layer->isShowingRepaintCounter() || !state.repaintCountChanged)
        return;

    layer->setRepaintCount(state.repaintCount);
}

void CoordinatedGraphicsScene::setLayerChildrenIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.childrenChanged)
        return;

    Vector<TextureMapperLayer*> children;

    for (size_t i = 0; i < state.children.size(); ++i) {
        CoordinatedLayerID childID = state.children[i];
        TextureMapperLayer* child = layerByID(childID);
        children.append(child);
    }
    layer->setChildren(children);
}

#if ENABLE(CSS_FILTERS)
void CoordinatedGraphicsScene::setLayerFiltersIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.filtersChanged)
        return;

#if ENABLE(CSS_SHADERS)
    injectCachedCustomFilterPrograms(state.filters);
#endif
    layer->setFilters(state.filters);
}
#endif

#if ENABLE(CSS_SHADERS)
void CoordinatedGraphicsScene::syncCustomFilterPrograms(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.customFiltersToCreate.size(); ++i)
        createCustomFilterProgram(state.customFiltersToCreate[i].first, state.customFiltersToCreate[i].second);

    for (size_t i = 0; i < state.customFiltersToRemove.size(); ++i)
        removeCustomFilterProgram(state.customFiltersToRemove[i]);
}

void CoordinatedGraphicsScene::injectCachedCustomFilterPrograms(const FilterOperations& filters) const
{
    for (size_t i = 0; i < filters.size(); ++i) {
        FilterOperation* operation = filters.operations().at(i).get();
        if (operation->getOperationType() != FilterOperation::CUSTOM)
            continue;

        CoordinatedCustomFilterOperation* customOperation = static_cast<CoordinatedCustomFilterOperation*>(operation);
        ASSERT(!customOperation->program());
        CustomFilterProgramMap::const_iterator iter = m_customFilterPrograms.find(customOperation->programID());
        ASSERT(iter != m_customFilterPrograms.end());
        customOperation->setProgram(iter->value.get());
    }
}

void CoordinatedGraphicsScene::createCustomFilterProgram(int id, const CustomFilterProgramInfo& programInfo)
{
    ASSERT(!m_customFilterPrograms.contains(id));
    m_customFilterPrograms.set(id, CoordinatedCustomFilterProgram::create(programInfo.vertexShaderString(), programInfo.fragmentShaderString(), programInfo.programType(), programInfo.mixSettings(), programInfo.meshType()));
}

void CoordinatedGraphicsScene::removeCustomFilterProgram(int id)
{
    CustomFilterProgramMap::iterator iter = m_customFilterPrograms.find(id);
    ASSERT(iter != m_customFilterPrograms.end());
    if (m_textureMapper)
        m_textureMapper->removeCachedCustomFilterProgram(iter->value.get());
    m_customFilterPrograms.remove(iter);
}
#endif // ENABLE(CSS_SHADERS)

void CoordinatedGraphicsScene::setLayerState(CoordinatedLayerID id, const CoordinatedGraphicsLayerState& layerState)
{
    ASSERT(m_rootLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* layer = layerByID(id);

    if (layerState.positionChanged)
        layer->setPosition(layerState.pos);

    if (layerState.anchorPointChanged)
        layer->setAnchorPoint(layerState.anchorPoint);

    if (layerState.sizeChanged)
        layer->setSize(layerState.size);

    if (layerState.transformChanged)
        layer->setTransform(layerState.transform);

    if (layerState.childrenTransformChanged)
        layer->setChildrenTransform(layerState.childrenTransform);

    if (layerState.contentsRectChanged)
        layer->setContentsRect(layerState.contentsRect);

    if (layerState.contentsTilingChanged) {
        layer->setContentsTilePhase(layerState.contentsTilePhase);
        layer->setContentsTileSize(layerState.contentsTileSize);
    }

    if (layerState.opacityChanged)
        layer->setOpacity(layerState.opacity);

    if (layerState.solidColorChanged)
        layer->setSolidColor(layerState.solidColor);

    if (layerState.debugBorderColorChanged || layerState.debugBorderWidthChanged)
        layer->setDebugVisuals(layerState.showDebugBorders, layerState.debugBorderColor, layerState.debugBorderWidth, layerState.showRepaintCounter);

    if (layerState.replicaChanged)
        layer->setReplicaLayer(getLayerByIDIfExists(layerState.replica));

    if (layerState.maskChanged)
        layer->setMaskLayer(getLayerByIDIfExists(layerState.mask));

    if (layerState.imageChanged)
        assignImageBackingToLayer(layer, layerState.imageID);

    if (layerState.flagsChanged) {
        layer->setContentsOpaque(layerState.contentsOpaque);
        layer->setDrawsContent(layerState.drawsContent);
        layer->setContentsVisible(layerState.contentsVisible);
        layer->setBackfaceVisibility(layerState.backfaceVisible);

        // Never clip the root layer.
        layer->setMasksToBounds(id == m_rootLayerID ? false : layerState.masksToBounds);
        layer->setPreserves3D(layerState.preserves3D);

        bool fixedToViewportChanged = layer->fixedToViewport() != layerState.fixedToViewport;
        layer->setFixedToViewport(layerState.fixedToViewport);
        if (fixedToViewportChanged) {
            if (layerState.fixedToViewport)
                m_fixedLayers.add(id, layer);
            else
                m_fixedLayers.remove(id);
        }

        layer->setIsScrollable(layerState.isScrollable);
    }

    if (layerState.committedScrollOffsetChanged)
        layer->didCommitScrollOffset(layerState.committedScrollOffset);

    prepareContentBackingStore(layer);

    // Apply Operations.
    setLayerChildrenIfNeeded(layer, layerState);
    createTilesIfNeeded(layer, layerState);
    removeTilesIfNeeded(layer, layerState);
    updateTilesIfNeeded(layer, layerState);
#if ENABLE(CSS_FILTERS)
    setLayerFiltersIfNeeded(layer, layerState);
#endif
    setLayerAnimationsIfNeeded(layer, layerState);
#if USE(GRAPHICS_SURFACE)
    syncCanvasIfNeeded(layer, layerState);
#endif
    setLayerRepaintCountIfNeeded(layer, layerState);
}

TextureMapperLayer* CoordinatedGraphicsScene::getLayerByIDIfExists(CoordinatedLayerID id)
{
    return (id != InvalidCoordinatedLayerID) ? layerByID(id) : 0;
}

void CoordinatedGraphicsScene::createLayers(const Vector<CoordinatedLayerID>& ids)
{
    for (size_t index = 0; index < ids.size(); ++index)
        createLayer(ids[index]);
}

void CoordinatedGraphicsScene::createLayer(CoordinatedLayerID id)
{
    OwnPtr<TextureMapperLayer> newLayer = adoptPtr(new TextureMapperLayer);
    newLayer->setID(id);
    newLayer->setScrollClient(this);
    m_layers.add(id, newLayer.release());
}

void CoordinatedGraphicsScene::deleteLayers(const Vector<CoordinatedLayerID>& layerIDs)
{
    for (size_t index = 0; index < layerIDs.size(); ++index)
        deleteLayer(layerIDs[index]);
}

void CoordinatedGraphicsScene::deleteLayer(CoordinatedLayerID layerID)
{
    OwnPtr<TextureMapperLayer> layer = m_layers.take(layerID);
    ASSERT(layer);

    m_backingStores.remove(layer.get());
    m_fixedLayers.remove(layerID);
#if USE(GRAPHICS_SURFACE)
    m_surfaceBackingStores.remove(layer.get());
#endif
}

void CoordinatedGraphicsScene::setRootLayerID(CoordinatedLayerID layerID)
{
    ASSERT(layerID != InvalidCoordinatedLayerID);
    ASSERT(m_rootLayerID == InvalidCoordinatedLayerID);

    m_rootLayerID = layerID;

    TextureMapperLayer* layer = layerByID(layerID);
    ASSERT(m_rootLayer->children().isEmpty());
    m_rootLayer->addChild(layer);
}

void CoordinatedGraphicsScene::prepareContentBackingStore(TextureMapperLayer* layer)
{
    if (!layerShouldHaveBackingStore(layer)) {
        removeBackingStoreIfNeeded(layer);
        return;
    }

    createBackingStoreIfNeeded(layer);
    resetBackingStoreSizeToLayerSize(layer);
}

void CoordinatedGraphicsScene::createBackingStoreIfNeeded(TextureMapperLayer* layer)
{
    if (m_backingStores.contains(layer))
        return;

    RefPtr<CoordinatedBackingStore> backingStore(CoordinatedBackingStore::create());
    m_backingStores.add(layer, backingStore);
    layer->setBackingStore(backingStore);
}

void CoordinatedGraphicsScene::removeBackingStoreIfNeeded(TextureMapperLayer* layer)
{
    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.take(layer);
    if (!backingStore)
        return;

    layer->setBackingStore(0);
}

void CoordinatedGraphicsScene::resetBackingStoreSizeToLayerSize(TextureMapperLayer* layer)
{
    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);
    backingStore->setSize(layer->size());
    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::createTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToCreate.isEmpty())
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);

    for (size_t i = 0; i < state.tilesToCreate.size(); ++i)
        backingStore->createTile(state.tilesToCreate[i].tileID, state.tilesToCreate[i].scale);
}

void CoordinatedGraphicsScene::removeTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToRemove.isEmpty())
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    if (!backingStore)
        return;

    for (size_t i = 0; i < state.tilesToRemove.size(); ++i)
        backingStore->removeTile(state.tilesToRemove[i]);

    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::updateTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToUpdate.isEmpty())
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);

    for (size_t i = 0; i < state.tilesToUpdate.size(); ++i) {
        const TileUpdateInfo& tileInfo = state.tilesToUpdate[i];
        const SurfaceUpdateInfo& surfaceUpdateInfo = tileInfo.updateInfo;

        SurfaceMap::iterator surfaceIt = m_surfaces.find(surfaceUpdateInfo.atlasID);
        ASSERT(surfaceIt != m_surfaces.end());

        backingStore->updateTile(tileInfo.tileID, surfaceUpdateInfo.updateRect, tileInfo.tileRect, surfaceIt->value, surfaceUpdateInfo.surfaceOffset);
        m_backingStoresWithPendingBuffers.add(backingStore);
    }
}

void CoordinatedGraphicsScene::syncUpdateAtlases(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.updateAtlasesToCreate.size(); ++i)
        createUpdateAtlas(state.updateAtlasesToCreate[i].first, state.updateAtlasesToCreate[i].second);

    for (size_t i = 0; i < state.updateAtlasesToRemove.size(); ++i)
        removeUpdateAtlas(state.updateAtlasesToRemove[i]);
}

void CoordinatedGraphicsScene::createUpdateAtlas(uint32_t atlasID, PassRefPtr<CoordinatedSurface> surface)
{
    ASSERT(!m_surfaces.contains(atlasID));
    m_surfaces.add(atlasID, surface);
}

void CoordinatedGraphicsScene::removeUpdateAtlas(uint32_t atlasID)
{
    ASSERT(m_surfaces.contains(atlasID));
    m_surfaces.remove(atlasID);
}

void CoordinatedGraphicsScene::syncImageBackings(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.imagesToRemove.size(); ++i)
        removeImageBacking(state.imagesToRemove[i]);

    for (size_t i = 0; i < state.imagesToCreate.size(); ++i)
        createImageBacking(state.imagesToCreate[i]);

    for (size_t i = 0; i < state.imagesToUpdate.size(); ++i)
        updateImageBacking(state.imagesToUpdate[i].first, state.imagesToUpdate[i].second);

    for (size_t i = 0; i < state.imagesToClear.size(); ++i)
        clearImageBackingContents(state.imagesToClear[i]);
}

void CoordinatedGraphicsScene::createImageBacking(CoordinatedImageBackingID imageID)
{
    ASSERT(!m_imageBackings.contains(imageID));
    RefPtr<CoordinatedBackingStore> backingStore(CoordinatedBackingStore::create());
    m_imageBackings.add(imageID, backingStore.release());
}

void CoordinatedGraphicsScene::updateImageBacking(CoordinatedImageBackingID imageID, PassRefPtr<CoordinatedSurface> surface)
{
    ASSERT(m_imageBackings.contains(imageID));
    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    RefPtr<CoordinatedBackingStore> backingStore = it->value;

    // CoordinatedImageBacking is realized to CoordinatedBackingStore with only one tile in UI Process.
    backingStore->createTile(1 /* id */, 1 /* scale */);
    IntRect rect(IntPoint::zero(), surface->size());
    // See CoordinatedGraphicsLayer::shouldDirectlyCompositeImage()
    ASSERT(2000 >= std::max(rect.width(), rect.height()));
    backingStore->setSize(rect.size());
    backingStore->updateTile(1 /* id */, rect, rect, surface, rect.location());

    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::clearImageBackingContents(CoordinatedImageBackingID imageID)
{
    ASSERT(m_imageBackings.contains(imageID));
    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    RefPtr<CoordinatedBackingStore> backingStore = it->value;
    backingStore->removeAllTiles();
    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::removeImageBacking(CoordinatedImageBackingID imageID)
{
    ASSERT(m_imageBackings.contains(imageID));

    // We don't want TextureMapperLayer refers a dangling pointer.
    m_releasedImageBackings.append(m_imageBackings.take(imageID));
}

void CoordinatedGraphicsScene::assignImageBackingToLayer(TextureMapperLayer* layer, CoordinatedImageBackingID imageID)
{
#if USE(GRAPHICS_SURFACE)
    if (m_surfaceBackingStores.contains(layer))
        return;
#endif

    if (imageID == InvalidCoordinatedImageBackingID) {
        layer->setContentsLayer(0);
        return;
    }

    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    ASSERT(it != m_imageBackings.end());
    layer->setContentsLayer(it->value.get());
}

void CoordinatedGraphicsScene::removeReleasedImageBackingsIfNeeded()
{
    m_releasedImageBackings.clear();
}

void CoordinatedGraphicsScene::commitPendingBackingStoreOperations()
{
    HashSet<RefPtr<CoordinatedBackingStore> >::iterator end = m_backingStoresWithPendingBuffers.end();
    for (HashSet<RefPtr<CoordinatedBackingStore> >::iterator it = m_backingStoresWithPendingBuffers.begin(); it != end; ++it)
        (*it)->commitTileOperations(m_textureMapper.get());

    m_backingStoresWithPendingBuffers.clear();
}

void CoordinatedGraphicsScene::commitSceneState(const CoordinatedGraphicsState& state)
{
    m_renderedContentsScrollPosition = state.scrollPosition;

    createLayers(state.layersToCreate);
    deleteLayers(state.layersToRemove);

    if (state.rootCompositingLayer != m_rootLayerID)
        setRootLayerID(state.rootCompositingLayer);

    syncImageBackings(state);
    syncUpdateAtlases(state);
#if ENABLE(CSS_SHADERS)
    syncCustomFilterPrograms(state);
#endif

    for (size_t i = 0; i < state.layersToUpdate.size(); ++i)
        setLayerState(state.layersToUpdate[i].first, state.layersToUpdate[i].second);

    commitPendingBackingStoreOperations();
    removeReleasedImageBackingsIfNeeded();

    // The pending tiles state is on its way for the screen, tell the web process to render the next one.
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::renderNextFrame, this));
}

void CoordinatedGraphicsScene::renderNextFrame()
{
    if (m_client)
        m_client->renderNextFrame();
}

void CoordinatedGraphicsScene::ensureRootLayer()
{
    if (m_rootLayer)
        return;

    m_rootLayer = adoptPtr(new TextureMapperLayer);
    m_rootLayer->setMasksToBounds(false);
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setAnchorPoint(FloatPoint3D(0, 0, 0));

    // The root layer should not have zero size, or it would be optimized out.
    m_rootLayer->setSize(FloatSize(1.0, 1.0));

    ASSERT(m_textureMapper);
    m_rootLayer->setTextureMapper(m_textureMapper.get());
}

void CoordinatedGraphicsScene::syncRemoteContent()
{
    // We enqueue messages and execute them during paint, as they require an active GL context.
    ensureRootLayer();

    Vector<Function<void()> > renderQueue;
    bool calledOnMainThread = WTF::isMainThread();
    if (!calledOnMainThread)
        m_renderQueueMutex.lock();
    renderQueue.swap(m_renderQueue);
    if (!calledOnMainThread)
        m_renderQueueMutex.unlock();

    for (size_t i = 0; i < renderQueue.size(); ++i)
        renderQueue[i]();
}

void CoordinatedGraphicsScene::purgeGLResources()
{
    m_imageBackings.clear();
    m_releasedImageBackings.clear();
#if USE(GRAPHICS_SURFACE)
    m_surfaceBackingStores.clear();
#endif
    m_surfaces.clear();

    m_rootLayer.clear();
    m_rootLayerID = InvalidCoordinatedLayerID;
    m_layers.clear();
    m_fixedLayers.clear();
    m_textureMapper.clear();
    m_backingStores.clear();
    m_backingStoresWithPendingBuffers.clear();

    setActive(false);
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::purgeBackingStores, this));
}

void CoordinatedGraphicsScene::dispatchCommitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    m_client->commitScrollOffset(layerID, offset);
}

void CoordinatedGraphicsScene::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::dispatchCommitScrollOffset, this, layerID, offset));
}

void CoordinatedGraphicsScene::purgeBackingStores()
{
    if (m_client)
        m_client->purgeBackingStores();
}

void CoordinatedGraphicsScene::setLayerAnimationsIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.animationsChanged)
        return;

#if ENABLE(CSS_SHADERS)
    for (size_t i = 0; i < state.animations.animations().size(); ++i) {
        const KeyframeValueList& keyframes = state.animations.animations().at(i).keyframes();
        if (keyframes.property() != AnimatedPropertyWebkitFilter)
            continue;
        for (size_t j = 0; j < keyframes.size(); ++j) {
            const FilterAnimationValue& filterValue = static_cast<const FilterAnimationValue&>(keyframes.at(j));
            injectCachedCustomFilterPrograms(filterValue.value());
        }
    }
#endif
    layer->setAnimations(state.animations);
}

void CoordinatedGraphicsScene::detach()
{
    ASSERT(isMainThread());
    m_renderQueue.clear();
    m_client = 0;
}

void CoordinatedGraphicsScene::appendUpdate(const Function<void()>& function)
{
    if (!m_isActive)
        return;

    ASSERT(isMainThread());
    MutexLocker locker(m_renderQueueMutex);
    m_renderQueue.append(function);
}

void CoordinatedGraphicsScene::setActive(bool active)
{
    if (m_isActive == active)
        return;

    // Have to clear render queue in both cases.
    // If there are some updates in queue during activation then those updates are from previous instance of paint node
    // and cannot be applied to the newly created instance.
    m_renderQueue.clear();
    m_isActive = active;
    if (m_isActive)
        dispatchOnMainThread(bind(&CoordinatedGraphicsScene::renderNextFrame, this));
}

void CoordinatedGraphicsScene::setBackgroundColor(const Color& color)
{
    m_backgroundColor = color;
}

TextureMapperLayer* CoordinatedGraphicsScene::findScrollableContentsLayerAt(const FloatPoint& point)
{
    return rootLayer() ? rootLayer()->findScrollableContentsLayerAt(point) : 0;
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
