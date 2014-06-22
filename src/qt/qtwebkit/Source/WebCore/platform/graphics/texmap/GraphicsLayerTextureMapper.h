/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef GraphicsLayerTextureMapper_h
#define GraphicsLayerTextureMapper_h

#if USE(TEXTURE_MAPPER)

#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#include "Image.h"
#include "TextureMapperLayer.h"
#include "TextureMapperPlatformLayer.h"
#include "TextureMapperTiledBackingStore.h"
#include "Timer.h"

namespace WebCore {

class GraphicsLayerTextureMapper : public GraphicsLayer, public TextureMapperPlatformLayer::Client {
public:
    explicit GraphicsLayerTextureMapper(GraphicsLayerClient*);
    virtual ~GraphicsLayerTextureMapper();

    void setScrollClient(TextureMapperLayer::ScrollingClient* client) { m_layer->setScrollClient(client); }
    void setID(uint32_t id) { m_layer->setID(id); }

    // reimps from GraphicsLayer.h
    virtual void setNeedsDisplay();
    virtual void setContentsNeedsDisplay();
    virtual void setNeedsDisplayInRect(const FloatRect&);
    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);
    virtual void setMaskLayer(GraphicsLayer* layer);
    virtual void setPosition(const FloatPoint& p);
    virtual void setAnchorPoint(const FloatPoint3D& p);
    virtual void setSize(const FloatSize& size);
    virtual void setTransform(const TransformationMatrix& t);
    virtual void setChildrenTransform(const TransformationMatrix& t);
    virtual void setPreserves3D(bool b);
    virtual void setMasksToBounds(bool b);
    virtual void setDrawsContent(bool b);
    virtual void setContentsVisible(bool);
    virtual void setContentsOpaque(bool b);
    virtual void setBackfaceVisibility(bool b);
    virtual void setOpacity(float opacity);
    virtual void setContentsRect(const IntRect& r);
    virtual void setReplicatedByLayer(GraphicsLayer*);
    virtual void setContentsToImage(Image*);
    virtual void setContentsToSolidColor(const Color&);
    Color solidColor() const { return m_solidColor; }
    virtual void setContentsToMedia(PlatformLayer*);
    virtual void setContentsToCanvas(PlatformLayer* canvas) { setContentsToMedia(canvas); }
    virtual void setShowDebugBorder(bool) OVERRIDE;
    virtual void setDebugBorder(const Color&, float width) OVERRIDE;
    virtual void setShowRepaintCounter(bool) OVERRIDE;
    virtual void flushCompositingState(const FloatRect&);
    virtual void flushCompositingStateForThisLayerOnly();
    virtual void setName(const String& name);
    virtual bool hasContentsLayer() const { return m_contentsLayer; }
    virtual PlatformLayer* platformLayer() const { return m_contentsLayer; }

    inline int changeMask() const { return m_changeMask; }

    virtual bool addAnimation(const KeyframeValueList&, const IntSize&, const Animation*, const String&, double);
    virtual void pauseAnimation(const String&, double);
    virtual void removeAnimation(const String&);
    void setAnimations(const GraphicsLayerAnimations&);

    TextureMapperLayer* layer() const { return m_layer.get(); }

    void didCommitScrollOffset(const IntSize&);
    void setIsScrollable(bool);
    bool isScrollable() const { return m_isScrollable; }

#if ENABLE(CSS_FILTERS)
    virtual bool setFilters(const FilterOperations&);
#endif

    void setFixedToViewport(bool);
    bool fixedToViewport() const { return m_fixedToViewport; }

    Color debugBorderColor() const { return m_debugBorderColor; }
    float debugBorderWidth() const { return m_debugBorderWidth; }
    void setRepaintCount(int);

private:
    virtual void willBeDestroyed();

    void commitLayerChanges();
    void updateDebugBorderAndRepaintCount();
    void updateBackingStoreIfNeeded();
    void prepareBackingStoreIfNeeded();
    bool shouldHaveBackingStore() const;

    // TextureMapperPlatformLayer::Client methods:
    virtual void setPlatformLayerNeedsDisplay() OVERRIDE { setContentsNeedsDisplay(); }
    virtual void platformLayerWasDestroyed() OVERRIDE { m_contentsLayer = 0; }

    // This set of flags help us defer which properties of the layer have been
    // modified by the compositor, so we can know what to look for in the next flush.
    enum ChangeMask {
        NoChanges =                 0,

        ChildrenChange =            (1L << 1),
        MaskLayerChange =           (1L << 2),
        ReplicaLayerChange =        (1L << 3),

        ContentChange =             (1L << 4),
        ContentsRectChange =        (1L << 5),
        ContentsVisibleChange =     (1L << 6),
        ContentsOpaqueChange =      (1L << 7),

        PositionChange =            (1L << 8),
        AnchorPointChange =         (1L << 9),
        SizeChange =                (1L << 10),
        TransformChange =           (1L << 11),
        ChildrenTransformChange =   (1L << 12),
        Preserves3DChange =         (1L << 13),

        MasksToBoundsChange =       (1L << 14),
        DrawsContentChange =        (1L << 15),
        OpacityChange =             (1L << 16),
        BackfaceVisibilityChange =  (1L << 17),

        BackingStoreChange =        (1L << 18),
        DisplayChange =             (1L << 19),
        ContentsDisplayChange =     (1L << 20),
        BackgroundColorChange =     (1L << 21),

        AnimationChange =           (1L << 22),
        FilterChange =              (1L << 23),

        DebugVisualsChange =        (1L << 24),
        RepaintCountChange =        (1L << 25),

        FixedToViewporChange =      (1L << 26),
        AnimationStarted =          (1L << 27),

        CommittedScrollOffsetChange =     (1L << 28),
        IsScrollableChange =              (1L << 29)
    };
    void notifyChange(ChangeMask);

    OwnPtr<TextureMapperLayer> m_layer;
    RefPtr<TextureMapperTiledBackingStore> m_compositedImage;
    NativeImagePtr m_compositedNativeImagePtr;
    RefPtr<TextureMapperBackingStore> m_backingStore;

    int m_changeMask;
    bool m_needsDisplay;
    bool m_hasOwnBackingStore;
    bool m_fixedToViewport;
    Color m_solidColor;

    Color m_debugBorderColor;
    float m_debugBorderWidth;

    TextureMapperPlatformLayer* m_contentsLayer;
    FloatRect m_needsDisplayRect;
    GraphicsLayerAnimations m_animations;
    double m_animationStartTime;

    IntSize m_committedScrollOffset;
    bool m_isScrollable;
};

inline static GraphicsLayerTextureMapper* toGraphicsLayerTextureMapper(GraphicsLayer* layer)
{
    return static_cast<GraphicsLayerTextureMapper*>(layer);
}

TextureMapperLayer* toTextureMapperLayer(GraphicsLayer*);

}
#endif

#endif // GraphicsLayerTextureMapper_h
