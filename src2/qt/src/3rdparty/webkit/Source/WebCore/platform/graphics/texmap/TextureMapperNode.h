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

#ifndef TextureMapperNode_h
#define TextureMapperNode_h

#include "FloatRect.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "Image.h"
#include "TextureMapper.h"
#include "TextureMapperPlatformLayer.h"
#include "Timer.h"
#include "TransformOperations.h"
#include "TranslateTransformOperation.h"
#include "UnitBezier.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class TextureMapperNode;
class TextureMapperCache;
class GraphicsLayerTextureMapper;

struct TexmapPaintOptions {
    BitmapTexture* surface;
    TextureMapper* textureMapper;
    TextureMapperNode* rootLayer;
    float opacity;
    IntRect scissorRect;
    IntRect visibleRect;
    bool isSurface;
    TextureMapperCache* cache;
};

class TextureMapperNode : public TextureMapperContentLayer {

public:
    // This set of flags help us defer which properties of the layer have been
    // modified by the compositor, so we can know what to look for in the next flush.
    enum ChangeMask {
        NoChanges =                 0,

        ParentChange =              (1L << 0),
        ChildrenChange =            (1L << 1),
        MaskLayerChange =           (1L << 2),
        PositionChange =            (1L << 3),

        AnchorPointChange =         (1L << 4),
        SizeChange  =               (1L << 5),
        TransformChange =           (1L << 6),
        ContentChange =             (1L << 7),

        ContentsOrientationChange = (1L << 9),
        OpacityChange =             (1L << 10),
        ContentsRectChange =        (1L << 11),

        Preserves3DChange =         (1L << 12),
        MasksToBoundsChange =       (1L << 13),
        DrawsContentChange =        (1L << 14),
        ContentsOpaqueChange =      (1L << 15),

        BackfaceVisibilityChange =  (1L << 16),
        ChildrenTransformChange =   (1L << 17),
        DisplayChange =             (1L << 18),
        BackgroundColorChange =     (1L << 19),

        ReplicaLayerChange =        (1L << 20)
    };
    // The compositor lets us special-case images and colors, so we try to do so.
    enum ContentType { HTMLContentType, DirectImageContentType, ColorContentType, MediaContentType, Canvas3DContentType};
    struct ContentData {
        IntRect needsDisplayRect;
        bool needsDisplay;
        Color backgroundColor;

        ContentType contentType;
        RefPtr<Image> image;
        TextureMapperMediaLayer* media;

        ContentData()
            : needsDisplay(false)
            , contentType(HTMLContentType)
            , image(0)
            , media(0)
        {
        }
    };


    TextureMapperNode();
    virtual ~TextureMapperNode();

    void syncCompositingState(GraphicsLayerTextureMapper*, bool recursive);

protected:
    // Reimps from TextureMapperContentLayer
    virtual IntSize size() const { return m_size; }
    virtual void setPlatformLayerClient(TextureMapperLayerClient*);
    virtual void paint(TextureMapper*, const TextureMapperContentLayer::PaintOptions&);

private:
    TextureMapperNode* rootLayer();
    void clearDirectImage();
    void computeTransformations();
    IntSize nearestSurfaceSize() const;
    void computeReplicaTransform();
    void computeLayerType();
    void computeLocalTransform();
    void flattenTo2DSpaceIfNecessary();
    void initializeTextureMapper(TextureMapper*);
    void invalidateTransform();
    void notifyChange(ChangeMask);
    void setNeedsDisplay();
    void setNeedsDisplayInRect(IntRect);
    void performPostSyncOperations();
    void syncCompositingStateInternal(GraphicsLayerTextureMapper*, bool recursive, TextureMapper*);
    void syncCompositingStateSelf(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper);
    TextureMapperCache* cache();

    void paintRecursive(TexmapPaintOptions options);
    bool paintReplica(const TexmapPaintOptions& options);
    void paintSurface(const TexmapPaintOptions& options);
    void paintSelf(const TexmapPaintOptions& options);
    void paintSelfAndChildren(const TexmapPaintOptions& options, TexmapPaintOptions& optionsForDescendants);
    void uploadTextureFromContent(TextureMapper* textureMapper, const IntRect& visibleRect, GraphicsLayer* layer);

    int countDescendantsWithContent() const;
    bool hasSurfaceDescendants() const;

    TextureMapper* textureMapper();


    static TextureMapperNode* toTextureMapperNode(GraphicsLayer*);
    static int compareGraphicsLayersZValue(const void* a, const void* b);
    static void sortByZOrder(Vector<TextureMapperNode* >& array, int first, int last);
    struct TransformData {
        TransformationMatrix base, target, replica, forDescendants, perspective, local;
        IntRect targetBoundingRect;
        float centerZ;
        bool dirty, localDirty, perspectiveDirty;
        IntRect boundingRectFromRoot;
        TransformData() : dirty(true), localDirty(true), perspectiveDirty(true) { }
    };

    TransformData m_transforms;

    enum LayerType {
        DefaultLayer,
        RootLayer,
        ScissorLayer,
        ClipLayer,
        TransparencyLayer
    };

    LayerType m_layerType;

    inline IntRect targetRect() const
    {
        return m_currentContent.contentType == HTMLContentType ? entireRect() : m_state.contentsRect;
    }

    inline IntRect entireRect() const
    {
        return IntRect(0, 0, m_size.width(), m_size.height());
    }

    inline IntRect replicaRect() const
    {
        return m_layerType == TransparencyLayer ? IntRect(0, 0, m_nearestSurfaceSize.width(), m_nearestSurfaceSize.height()) : entireRect();
    }

    RefPtr<BitmapTexture> m_texture;
    RefPtr<BitmapTexture> m_surface, m_replicaSurface;

    ContentData m_currentContent;

    Vector<TextureMapperNode*> m_children;
    TextureMapperNode* m_parent;
    TextureMapperNode* m_effectTarget;
    IntSize m_size, m_nearestSurfaceSize;
    String m_name;
    TextureMapperLayerClient* m_platformClient;

    struct State {
        FloatPoint pos;
        FloatPoint3D anchorPoint;
        FloatSize size;
        TransformationMatrix transform;
        TransformationMatrix childrenTransform;
        Color backgroundColor;
        Color currentColor;
        GraphicsLayer::CompositingCoordinatesOrientation geoOrientation;
        GraphicsLayer::CompositingCoordinatesOrientation contentsOrientation;
        float opacity;
        IntRect contentsRect;
        int descendantsWithContent;
        TextureMapperNode* maskLayer;
        TextureMapperNode* replicaLayer;
        bool preserves3D;
        bool masksToBounds;
        bool drawsContent;
        bool contentsOpaque;
        bool backfaceVisibility;
        bool visible;
        bool dirty;
        bool tiled;
        bool hasSurfaceDescendants;
        IntRect visibleRect;

        State()
            : opacity(1.f)
            , descendantsWithContent(0)
            , maskLayer(0)
            , replicaLayer(0)
            , preserves3D(false)
            , masksToBounds(false)
            , drawsContent(false)
            , contentsOpaque(false)
            , backfaceVisibility(false)
            , visible(true)
            , dirty(true)
            , tiled(false)
            , hasSurfaceDescendants(false)
        {
        }
    };

    State m_state;
    TextureMapperCache* m_cache;
};

}
#endif // TextureMapperNode_h
