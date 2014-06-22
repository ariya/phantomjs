/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapperLayer_h
#define TextureMapperLayer_h

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

#include "FilterOperations.h"
#include "FloatRect.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsLayerTransform.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"

namespace WebCore {

class Region;
class TextureMapperPaintOptions;
class TextureMapperPlatformLayer;

class TextureMapperLayer : public GraphicsLayerAnimation::Client {
    WTF_MAKE_NONCOPYABLE(TextureMapperLayer);
    WTF_MAKE_FAST_ALLOCATED;
public:

    class ScrollingClient {
    public:
        virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset) = 0;
    };

    TextureMapperLayer()
        : m_parent(0)
        , m_effectTarget(0)
        , m_contentsLayer(0)
        , m_currentOpacity(1)
        , m_centerZ(0)
        , m_textureMapper(0)
        , m_fixedToViewport(false)
        , m_id(0)
        , m_scrollClient(0)
        , m_isScrollable(false)
        , m_patternTransformDirty(false)
    { }

    virtual ~TextureMapperLayer();

    void setID(uint32_t id) { m_id = id; }
    uint32_t id() { return m_id; }

    const Vector<TextureMapperLayer*>& children() const { return m_children; }
    TextureMapperLayer* findScrollableContentsLayerAt(const FloatPoint& pos);

    void setScrollClient(ScrollingClient* scrollClient) { m_scrollClient = scrollClient; }
    void scrollBy(const WebCore::FloatSize&);

    void didCommitScrollOffset(const IntSize&);
    void setIsScrollable(bool isScrollable) { m_isScrollable = isScrollable; }
    bool isScrollable() const { return m_isScrollable; }

    TextureMapper* textureMapper() const;
    void setTextureMapper(TextureMapper* texmap) { m_textureMapper = texmap; }

    void setChildren(const Vector<TextureMapperLayer*>&);
    void setMaskLayer(TextureMapperLayer*);
    void setReplicaLayer(TextureMapperLayer*);
    void setPosition(const FloatPoint&);
    void setSize(const FloatSize&);
    void setAnchorPoint(const FloatPoint3D&);
    void setPreserves3D(bool);
    void setTransform(const TransformationMatrix&);
    void setChildrenTransform(const TransformationMatrix&);
    void setContentsRect(const IntRect&);
    void setMasksToBounds(bool);
    void setDrawsContent(bool);
    bool drawsContent() const { return m_state.drawsContent; }
    bool contentsAreVisible() const { return m_state.contentsVisible; }
    FloatSize size() const { return m_state.size; }
    float opacity() const { return m_state.opacity; }
    TransformationMatrix transform() const { return m_state.transform; }
    void setContentsVisible(bool);
    void setContentsOpaque(bool);
    void setBackfaceVisibility(bool);
    void setOpacity(float);
    void setSolidColor(const Color&);
    void setContentsTileSize(const IntSize&);
    void setContentsTilePhase(const IntPoint&);
#if ENABLE(CSS_FILTERS)
    void setFilters(const FilterOperations&);
#endif

    bool hasFilters() const
    {
#if ENABLE(CSS_FILTERS)
        return !m_currentFilters.isEmpty();
#else
        return false;
#endif
    }

    void setDebugVisuals(bool showDebugBorders, const Color& debugBorderColor, float debugBorderWidth, bool showRepaintCounter);
    bool isShowingRepaintCounter() const { return m_state.showRepaintCounter; }
    void setRepaintCount(int);
    void setContentsLayer(TextureMapperPlatformLayer*);
    void setAnimations(const GraphicsLayerAnimations&);
    void setFixedToViewport(bool);
    bool fixedToViewport() const { return m_fixedToViewport; }
    void setBackingStore(PassRefPtr<TextureMapperBackingStore>);

    void syncAnimations();
    bool descendantsOrSelfHaveRunningAnimations() const;

    void paint();

    void setScrollPositionDeltaIfNeeded(const FloatSize&);

    void applyAnimationsRecursively();
    void addChild(TextureMapperLayer*);

private:
    const TextureMapperLayer* rootLayer() const;
    void computeTransformsRecursive();

    static int compareGraphicsLayersZValue(const void* a, const void* b);
    static void sortByZOrder(Vector<TextureMapperLayer* >& array);

    PassRefPtr<BitmapTexture> texture() { return m_backingStore ? m_backingStore->texture() : 0; }
    FloatPoint adjustedPosition() const { return m_state.pos + m_scrollPositionDelta - m_userScrollOffset; }
    bool isAncestorFixedToViewport() const;
    TransformationMatrix replicaTransform();
    void removeFromParent();
    void removeAllChildren();

    enum ResolveSelfOverlapMode {
        ResolveSelfOverlapAlways = 0,
        ResolveSelfOverlapIfNeeded
    };
    void computeOverlapRegions(Region& overlapRegion, Region& nonOverlapRegion, ResolveSelfOverlapMode);

    void paintRecursive(const TextureMapperPaintOptions&);
    void paintUsingOverlapRegions(const TextureMapperPaintOptions&);
    PassRefPtr<BitmapTexture> paintIntoSurface(const TextureMapperPaintOptions&, const IntSize&);
    void paintWithIntermediateSurface(const TextureMapperPaintOptions&, const IntRect&);
    void paintSelf(const TextureMapperPaintOptions&);
    void paintSelfAndChildren(const TextureMapperPaintOptions&);
    void paintSelfAndChildrenWithReplica(const TextureMapperPaintOptions&);
    void applyMask(const TextureMapperPaintOptions&);
    void computePatternTransformIfNeeded();

    // GraphicsLayerAnimation::Client
    virtual void setAnimatedTransform(const TransformationMatrix&) OVERRIDE;
    virtual void setAnimatedOpacity(float) OVERRIDE;
#if ENABLE(CSS_FILTERS)
    virtual void setAnimatedFilters(const FilterOperations&) OVERRIDE;
#endif

    bool isVisible() const;
    enum ContentsLayerCount {
        NoLayersWithContent,
        SingleLayerWithContents,
        MultipleLayersWithContents
    };

    bool shouldBlend() const;

    inline FloatRect layerRect() const
    {
        return FloatRect(FloatPoint::zero(), m_state.size);
    }

    Vector<TextureMapperLayer*> m_children;
    TextureMapperLayer* m_parent;
    TextureMapperLayer* m_effectTarget;
    RefPtr<TextureMapperBackingStore> m_backingStore;
    TextureMapperPlatformLayer* m_contentsLayer;
    GraphicsLayerTransform m_currentTransform;
    float m_currentOpacity;
#if ENABLE(CSS_FILTERS)
    FilterOperations m_currentFilters;
#endif
    float m_centerZ;

    template<class HitTestCondition> TextureMapperLayer* hitTest(const FloatPoint&, HitTestCondition);
    static bool scrollableLayerHitTestCondition(TextureMapperLayer*, const FloatPoint&);

    FloatSize mapScrollOffset(const FloatSize&);
    void commitScrollOffset(const FloatSize&);

    struct State {
        FloatPoint pos;
        FloatPoint3D anchorPoint;
        FloatSize size;
        TransformationMatrix transform;
        TransformationMatrix childrenTransform;
        float opacity;
        FloatRect contentsRect;
        IntSize contentsTileSize;
        IntPoint contentsTilePhase;
        TextureMapperLayer* maskLayer;
        TextureMapperLayer* replicaLayer;
        Color solidColor;
#if ENABLE(CSS_FILTERS)
        FilterOperations filters;
#endif
        Color debugBorderColor;
        float debugBorderWidth;
        int repaintCount;

        bool preserves3D : 1;
        bool masksToBounds : 1;
        bool drawsContent : 1;
        bool contentsVisible : 1;
        bool contentsOpaque : 1;
        bool backfaceVisibility : 1;
        bool visible : 1;
        bool showDebugBorders : 1;
        bool showRepaintCounter : 1;

        State()
            : opacity(1)
            , maskLayer(0)
            , replicaLayer(0)
            , debugBorderWidth(0)
            , repaintCount(0)
            , preserves3D(false)
            , masksToBounds(false)
            , drawsContent(false)
            , contentsVisible(true)
            , contentsOpaque(false)
            , backfaceVisibility(true)
            , visible(true)
            , showDebugBorders(false)
            , showRepaintCounter(false)
        {
        }
    };

    State m_state;
    TextureMapper* m_textureMapper;
    GraphicsLayerAnimations m_animations;
    FloatSize m_scrollPositionDelta;
    bool m_fixedToViewport;
    uint32_t m_id;
    ScrollingClient* m_scrollClient;
    bool m_isScrollable;
    FloatSize m_userScrollOffset;
    FloatSize m_accumulatedScrollOffsetFractionalPart;
    TransformationMatrix m_patternTransform;
    bool m_patternTransformDirty;
};

}
#endif

#endif // TextureMapperLayer_h
