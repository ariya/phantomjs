/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2013 Company 100, Inc.

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

#ifndef CoordinatedGraphicsScene_h
#define CoordinatedGraphicsScene_h

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedGraphicsState.h"
#include "CoordinatedSurface.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsSurface.h"
#include "IntRect.h"
#include "IntSize.h"
#include "RunLoop.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperFPSCounter.h"
#include "TextureMapperLayer.h"
#include "Timer.h"
#include <wtf/Functional.h>
#include <wtf/HashSet.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>

#if USE(GRAPHICS_SURFACE)
#include "TextureMapperSurfaceBackingStore.h"
#endif

namespace WebCore {

class CoordinatedBackingStore;
class CustomFilterProgram;
class CustomFilterProgramInfo;

class CoordinatedGraphicsSceneClient {
public:
    virtual ~CoordinatedGraphicsSceneClient() { }
    virtual void purgeBackingStores() = 0;
    virtual void renderNextFrame() = 0;
    virtual void updateViewport() = 0;
    virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset) = 0;
};

class CoordinatedGraphicsScene : public ThreadSafeRefCounted<CoordinatedGraphicsScene>, public TextureMapperLayer::ScrollingClient {
public:
    explicit CoordinatedGraphicsScene(CoordinatedGraphicsSceneClient*);
    virtual ~CoordinatedGraphicsScene();
    void paintToCurrentGLContext(const TransformationMatrix&, float, const FloatRect&, TextureMapper::PaintFlags = 0);
    void paintToGraphicsContext(PlatformGraphicsContext*);
    void setScrollPosition(const FloatPoint&);
    void detach();
    void appendUpdate(const Function<void()>&);

    WebCore::TextureMapperLayer* findScrollableContentsLayerAt(const WebCore::FloatPoint&);

    virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset);

    // The painting thread must lock the main thread to use below two methods, because two methods access members that the main thread manages. See m_client.
    // Currently, QQuickWebPage::updatePaintNode() locks the main thread before calling both methods.
    void purgeGLResources();
    void setActive(bool);

    void commitSceneState(const CoordinatedGraphicsState&);

    void setBackgroundColor(const Color&);
    void setDrawsBackground(bool enable) { m_setDrawsBackground = enable; }

private:
    void setRootLayerID(CoordinatedLayerID);
    void createLayers(const Vector<CoordinatedLayerID>&);
    void deleteLayers(const Vector<CoordinatedLayerID>&);
    void setLayerState(CoordinatedLayerID, const CoordinatedGraphicsLayerState&);
    void setLayerChildrenIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void updateTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void createTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void removeTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#if ENABLE(CSS_FILTERS)
    void setLayerFiltersIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#endif
    void setLayerAnimationsIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#if USE(GRAPHICS_SURFACE)
    void createCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void syncCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void destroyCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#endif
    void setLayerRepaintCountIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);

    void syncUpdateAtlases(const CoordinatedGraphicsState&);
    void createUpdateAtlas(uint32_t atlasID, PassRefPtr<CoordinatedSurface>);
    void removeUpdateAtlas(uint32_t atlasID);

    void syncImageBackings(const CoordinatedGraphicsState&);
    void createImageBacking(CoordinatedImageBackingID);
    void updateImageBacking(CoordinatedImageBackingID, PassRefPtr<CoordinatedSurface>);
    void clearImageBackingContents(CoordinatedImageBackingID);
    void removeImageBacking(CoordinatedImageBackingID);

#if ENABLE(CSS_SHADERS)
    void syncCustomFilterPrograms(const CoordinatedGraphicsState&);
    void injectCachedCustomFilterPrograms(const FilterOperations& filters) const;
    void createCustomFilterProgram(int id, const CustomFilterProgramInfo&);
    void removeCustomFilterProgram(int id);
#endif

    TextureMapperLayer* layerByID(CoordinatedLayerID id)
    {
        ASSERT(m_layers.contains(id));
        ASSERT(id != InvalidCoordinatedLayerID);
        return m_layers.get(id);
    }
    TextureMapperLayer* getLayerByIDIfExists(CoordinatedLayerID);
    TextureMapperLayer* rootLayer() { return m_rootLayer.get(); }

    void syncRemoteContent();
    void adjustPositionForFixedLayers();

    void dispatchOnMainThread(const Function<void()>&);
    void updateViewport();
    void renderNextFrame();
    void purgeBackingStores();

    void createLayer(CoordinatedLayerID);
    void deleteLayer(CoordinatedLayerID);

    void assignImageBackingToLayer(TextureMapperLayer*, CoordinatedImageBackingID);
    void removeReleasedImageBackingsIfNeeded();
    void ensureRootLayer();
    void commitPendingBackingStoreOperations();

    void prepareContentBackingStore(TextureMapperLayer*);
    void createBackingStoreIfNeeded(TextureMapperLayer*);
    void removeBackingStoreIfNeeded(TextureMapperLayer*);
    void resetBackingStoreSizeToLayerSize(TextureMapperLayer*);

    void dispatchCommitScrollOffset(uint32_t layerID, const IntSize& offset);

    // Render queue can be accessed ony from main thread or updatePaintNode call stack!
    Vector<Function<void()> > m_renderQueue;
    Mutex m_renderQueueMutex;

    OwnPtr<TextureMapper> m_textureMapper;

    typedef HashMap<CoordinatedImageBackingID, RefPtr<CoordinatedBackingStore> > ImageBackingMap;
    ImageBackingMap m_imageBackings;
    Vector<RefPtr<CoordinatedBackingStore> > m_releasedImageBackings;

    typedef HashMap<TextureMapperLayer*, RefPtr<CoordinatedBackingStore> > BackingStoreMap;
    BackingStoreMap m_backingStores;

    HashSet<RefPtr<CoordinatedBackingStore> > m_backingStoresWithPendingBuffers;

#if USE(GRAPHICS_SURFACE)
    typedef HashMap<TextureMapperLayer*, RefPtr<TextureMapperSurfaceBackingStore> > SurfaceBackingStoreMap;
    SurfaceBackingStoreMap m_surfaceBackingStores;
#endif

    typedef HashMap<uint32_t /* atlasID */, RefPtr<CoordinatedSurface> > SurfaceMap;
    SurfaceMap m_surfaces;

    // Below two members are accessed by only the main thread. The painting thread must lock the main thread to access both members.
    CoordinatedGraphicsSceneClient* m_client;
    bool m_isActive;

    OwnPtr<TextureMapperLayer> m_rootLayer;

    typedef HashMap<CoordinatedLayerID, OwnPtr<TextureMapperLayer> > LayerMap;
    LayerMap m_layers;
    typedef HashMap<CoordinatedLayerID, TextureMapperLayer*> LayerRawPtrMap;
    LayerRawPtrMap m_fixedLayers;
    CoordinatedLayerID m_rootLayerID;
    FloatPoint m_scrollPosition;
    FloatPoint m_renderedContentsScrollPosition;
    Color m_backgroundColor;
    bool m_setDrawsBackground;

#if ENABLE(CSS_SHADERS)
    typedef HashMap<int, RefPtr<CustomFilterProgram> > CustomFilterProgramMap;
    CustomFilterProgramMap m_customFilterPrograms;
#endif

    TextureMapperFPSCounter m_fpsCounter;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // CoordinatedGraphicsScene_h


