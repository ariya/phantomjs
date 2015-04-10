/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef GraphicsLayerCA_h
#define GraphicsLayerCA_h

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayer.h"
#include "Image.h"
#include "PlatformCAAnimation.h"
#include "PlatformCALayerClient.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/StringHash.h>

// Enable this to add a light red wash over the visible portion of Tiled Layers, as computed
// by flushCompositingState().
// #define VISIBLE_TILE_WASH

namespace WebCore {

class PlatformCALayer;
class TransformState;

class GraphicsLayerCA : public GraphicsLayer, public PlatformCALayerClient {
public:
    // The width and height of a single tile in a tiled layer. Should be large enough to
    // avoid lots of small tiles (and therefore lots of drawing callbacks), but small enough
    // to keep the overall tile cost low.
    static const int kTiledLayerTileSize = 512;

    GraphicsLayerCA(GraphicsLayerClient*);
    virtual ~GraphicsLayerCA();

    virtual void setName(const String&);

    virtual PlatformLayer* platformLayer() const;
    virtual PlatformCALayer* platformCALayer() const { return primaryLayer(); }

    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);

    virtual void removeFromParent();

    virtual void setMaskLayer(GraphicsLayer*);
    virtual void setReplicatedLayer(GraphicsLayer*);

    virtual void setPosition(const FloatPoint&);
    virtual void setAnchorPoint(const FloatPoint3D&);
    virtual void setSize(const FloatSize&);
    virtual void setBoundsOrigin(const FloatPoint&);

    virtual void setTransform(const TransformationMatrix&);

    virtual void setChildrenTransform(const TransformationMatrix&);

    virtual void setPreserves3D(bool);
    virtual void setMasksToBounds(bool);
    virtual void setDrawsContent(bool);
    virtual void setContentsVisible(bool);
    virtual void setAcceleratesDrawing(bool);

    virtual void setBackgroundColor(const Color&);

    virtual void setContentsOpaque(bool);
    virtual void setBackfaceVisibility(bool);

    // return true if we started an animation
    virtual void setOpacity(float);

#if ENABLE(CSS_FILTERS)
    virtual bool setFilters(const FilterOperations&);
#endif

    virtual void setNeedsDisplay();
    virtual void setNeedsDisplayInRect(const FloatRect&);
    virtual void setContentsNeedsDisplay();
    
    virtual void setContentsRect(const IntRect&);
    
    virtual void suspendAnimations(double time);
    virtual void resumeAnimations();

    virtual bool addAnimation(const KeyframeValueList&, const IntSize& boxSize, const Animation*, const String& animationName, double timeOffset);
    virtual void pauseAnimation(const String& animationName, double timeOffset);
    virtual void removeAnimation(const String& animationName);

    virtual void setContentsToImage(Image*);
    virtual void setContentsToMedia(PlatformLayer*);
    virtual void setContentsToCanvas(PlatformLayer*);
    virtual void setContentsToSolidColor(const Color&);

    virtual bool hasContentsLayer() const { return m_contentsLayer; }
    
    virtual void setShowDebugBorder(bool) OVERRIDE;
    virtual void setShowRepaintCounter(bool) OVERRIDE;

    virtual void setDebugBackgroundColor(const Color&);
    virtual void setDebugBorder(const Color&, float borderWidth);

    virtual void layerDidDisplay(PlatformLayer*);

    virtual void setMaintainsPixelAlignment(bool);
    virtual void deviceOrPageScaleFactorChanged();

    struct CommitState {
        bool ancestorHasTransformAnimation;
        int treeDepth;
        CommitState()
            : ancestorHasTransformAnimation(false)
            , treeDepth(0)
        { }
    };
    void recursiveCommitChanges(const CommitState&, const TransformState&, float pageScaleFactor = 1, const FloatPoint& positionRelativeToBase = FloatPoint(), bool affectedByPageScale = false);

    virtual void flushCompositingState(const FloatRect&);
    virtual void flushCompositingStateForThisLayerOnly();

    virtual bool visibleRectChangeRequiresFlush(const FloatRect& visibleRect) const OVERRIDE;

    virtual TiledBacking* tiledBacking() const OVERRIDE;

    bool allowTiledLayer() const { return m_allowTiledLayer; }
    virtual void setAllowTiledLayer(bool b);

protected:
    virtual void setOpacityInternal(float);

private:
    virtual void willBeDestroyed();

    // PlatformCALayerClient overrides
    virtual void platformCALayerLayoutSublayersOfLayer(PlatformCALayer*) { }
    virtual bool platformCALayerRespondsToLayoutChanges() const { return false; }

    virtual void platformCALayerAnimationStarted(CFTimeInterval beginTime);
    virtual CompositingCoordinatesOrientation platformCALayerContentsOrientation() const { return contentsOrientation(); }
    virtual void platformCALayerPaintContents(GraphicsContext&, const IntRect& clip);
    virtual bool platformCALayerShowDebugBorders() const { return isShowingDebugBorder(); }
    virtual bool platformCALayerShowRepaintCounter(PlatformCALayer*) const;
    virtual int platformCALayerIncrementRepaintCount() { return incrementRepaintCount(); }

    virtual bool platformCALayerContentsOpaque() const { return contentsOpaque(); }
    virtual bool platformCALayerDrawsContent() const { return drawsContent(); }
    virtual void platformCALayerLayerDidDisplay(PlatformLayer* layer) { return layerDidDisplay(layer); }
    virtual void platformCALayerDidCreateTiles(const Vector<FloatRect>& dirtyRects) OVERRIDE;
    virtual float platformCALayerDeviceScaleFactor() OVERRIDE;

    virtual double backingStoreMemoryEstimate() const;

    void updateOpacityOnLayer();
    
#if ENABLE(CSS_FILTERS)
    void updateFilters();
#endif

    PlatformCALayer* primaryLayer() const { return m_structuralLayer.get() ? m_structuralLayer.get() : m_layer.get(); }
    PlatformCALayer* hostLayerForSublayers() const;
    PlatformCALayer* layerForSuperlayer() const;
    PlatformCALayer* animatedLayer(AnimatedPropertyID) const;

    typedef String CloneID; // Identifier for a given clone, based on original/replica branching down the tree.
    static bool isReplicatedRootClone(const CloneID& cloneID) { return cloneID[0U] & 1; }

    typedef HashMap<CloneID, RefPtr<PlatformCALayer> > LayerMap;
    LayerMap* primaryLayerClones() const { return m_structuralLayer.get() ? m_structuralLayerClones.get() : m_layerClones.get(); }
    LayerMap* animatedLayerClones(AnimatedPropertyID) const;

    bool createAnimationFromKeyframes(const KeyframeValueList&, const Animation*, const String& animationName, double timeOffset);
    bool createTransformAnimationsFromKeyframes(const KeyframeValueList&, const Animation*, const String& animationName, double timeOffset, const IntSize& boxSize);
#if ENABLE(CSS_FILTERS)
    bool createFilterAnimationsFromKeyframes(const KeyframeValueList&, const Animation*, const String& animationName, double timeOffset);
#endif

    // Return autoreleased animation (use RetainPtr?)
    PassRefPtr<PlatformCAAnimation> createBasicAnimation(const Animation*, const String& keyPath, bool additive);
    PassRefPtr<PlatformCAAnimation> createKeyframeAnimation(const Animation*, const String&, bool additive);
    void setupAnimation(PlatformCAAnimation*, const Animation*, bool additive);
    
    const TimingFunction* timingFunctionForAnimationValue(const AnimationValue&, const Animation&);
    
    bool setAnimationEndpoints(const KeyframeValueList&, const Animation*, PlatformCAAnimation*);
    bool setAnimationKeyframes(const KeyframeValueList&, const Animation*, PlatformCAAnimation*);

    bool setTransformAnimationEndpoints(const KeyframeValueList&, const Animation*, PlatformCAAnimation*, int functionIndex, TransformOperation::OperationType, bool isMatrixAnimation, const IntSize& boxSize);
    bool setTransformAnimationKeyframes(const KeyframeValueList&, const Animation*, PlatformCAAnimation*, int functionIndex, TransformOperation::OperationType, bool isMatrixAnimation, const IntSize& boxSize);
    
#if ENABLE(CSS_FILTERS)
    bool setFilterAnimationEndpoints(const KeyframeValueList&, const Animation*, PlatformCAAnimation*, int functionIndex, int internalFilterPropertyIndex);
    bool setFilterAnimationKeyframes(const KeyframeValueList&, const Animation*, PlatformCAAnimation*, int functionIndex, int internalFilterPropertyIndex, FilterOperation::OperationType);
#endif

    bool isRunningTransformAnimation() const;

    bool animationIsRunning(const String& animationName) const
    {
        return m_runningAnimations.find(animationName) != m_runningAnimations.end();
    }

    void commitLayerChangesBeforeSublayers(CommitState&, float pageScaleFactor, const FloatPoint& positionRelativeToBase, const FloatRect& oldVisibleRect);
    void commitLayerChangesAfterSublayers(CommitState&);

    FloatPoint computePositionRelativeToBase(float& pageScale) const;

    bool requiresTiledLayer(float pageScaleFactor) const;
    void swapFromOrToTiledLayer(bool useTiledLayer);

    CompositingCoordinatesOrientation defaultContentsOrientation() const;
    
    void setupContentsLayer(PlatformCALayer*);
    PlatformCALayer* contentsLayer() const { return m_contentsLayer.get(); }

    virtual void setReplicatedByLayer(GraphicsLayer*);

    virtual void getDebugBorderInfo(Color&, float& width) const;
    virtual void dumpAdditionalProperties(TextStream&, int indent, LayerTreeAsTextBehavior) const;

    void computePixelAlignment(float pixelAlignmentScale, const FloatPoint& positionRelativeToBase,
        FloatPoint& position, FloatSize&, FloatPoint3D& anchorPoint, FloatSize& alignmentOffset) const;
    enum ComputeVisibleRectFlag { RespectAnimatingTransforms = 1 << 0 };
    typedef unsigned ComputeVisibleRectFlags;
    FloatRect computeVisibleRect(TransformState&, ComputeVisibleRectFlags = RespectAnimatingTransforms) const;
    const FloatRect& visibleRect() const { return m_visibleRect; }
    
    static FloatRect adjustTiledLayerVisibleRect(TiledBacking*, const FloatRect& oldVisibleRect, const FloatRect& newVisibleRect, const FloatSize& oldSize, const FloatSize& newSize);

    bool recursiveVisibleRectChangeRequiresFlush(const TransformState&) const;

    virtual bool canThrottleLayerFlush() const;

    // Used to track the path down the tree for replica layers.
    struct ReplicaState {
        static const size_t maxReplicaDepth = 16;
        enum ReplicaBranchType { ChildBranch = 0, ReplicaBranch = 1 };
        ReplicaState(ReplicaBranchType firstBranch)
            : m_replicaDepth(0)
        {
            push(firstBranch);
        }
        
        // Called as we walk down the tree to build replicas.
        void push(ReplicaBranchType branchType)
        {
            m_replicaBranches.append(branchType);
            if (branchType == ReplicaBranch)
                ++m_replicaDepth;
        }
        
        void setBranchType(ReplicaBranchType branchType)
        {
            ASSERT(!m_replicaBranches.isEmpty());

            if (m_replicaBranches.last() != branchType) {
                if (branchType == ReplicaBranch)
                    ++m_replicaDepth;
                else
                    --m_replicaDepth;
            }

            m_replicaBranches.last() = branchType;
        }

        void pop()
        {
            if (m_replicaBranches.last() == ReplicaBranch)
                --m_replicaDepth;
            m_replicaBranches.removeLast();
        }
        
        size_t depth() const { return m_replicaBranches.size(); }
        size_t replicaDepth() const { return m_replicaDepth; }

        CloneID cloneID() const;        

    private:
        Vector<ReplicaBranchType> m_replicaBranches;
        size_t m_replicaDepth;
    };
    PassRefPtr<PlatformCALayer>replicatedLayerRoot(ReplicaState&);

    enum CloneLevel { RootCloneLevel, IntermediateCloneLevel };
    PassRefPtr<PlatformCALayer> fetchCloneLayers(GraphicsLayer* replicaRoot, ReplicaState&, CloneLevel);
    
    PassRefPtr<PlatformCALayer> cloneLayer(PlatformCALayer *, CloneLevel);
    PassRefPtr<PlatformCALayer> findOrMakeClone(CloneID, PlatformCALayer *, LayerMap*, CloneLevel);

    void ensureCloneLayers(CloneID cloneID, RefPtr<PlatformCALayer>& primaryLayer, RefPtr<PlatformCALayer>& structuralLayer, RefPtr<PlatformCALayer>& contentsLayer, CloneLevel cloneLevel);

    bool hasCloneLayers() const { return m_layerClones; }
    void removeCloneLayers();
    FloatPoint positionForCloneRootLayer() const;
    
    void propagateLayerChangeToReplicas();
    
    // All these "update" methods will be called inside a BEGIN_BLOCK_OBJC_EXCEPTIONS/END_BLOCK_OBJC_EXCEPTIONS block.
    void updateLayerNames();
    void updateSublayerList(bool maxLayerDepthReached = false);
    void updateGeometry(float pixelAlignmentScale, const FloatPoint& positionRelativeToBase);
    void updateTransform();
    void updateChildrenTransform();
    void updateMasksToBounds();
    void updateContentsVisibility();
    void updateContentsOpaque();
    void updateBackfaceVisibility();
    void updateStructuralLayer();
    void updateLayerDrawsContent();
    void updateBackgroundColor();

    void updateContentsImage();
    void updateContentsMediaLayer();
    void updateContentsCanvasLayer();
    void updateContentsColorLayer();
    void updateContentsRect();
    void updateMaskLayer();
    void updateReplicatedLayers();

    void updateAnimations();
    void updateContentsNeedsDisplay();
    void updateAcceleratesDrawing();
    void updateDebugBorder();
    void updateVisibleRect(const FloatRect& oldVisibleRect);
    void updateContentsScale(float pageScaleFactor);
    
    enum StructuralLayerPurpose {
        NoStructuralLayer = 0,
        StructuralLayerForPreserves3D,
        StructuralLayerForReplicaFlattening
    };
    void ensureStructuralLayer(StructuralLayerPurpose);
    StructuralLayerPurpose structuralLayerPurpose() const;

    void setAnimationOnLayer(PlatformCAAnimation*, AnimatedPropertyID, const String& animationName, int index, int subIndex, double timeOffset);
    bool removeCAAnimationFromLayer(AnimatedPropertyID, const String& animationName, int index, int subINdex);
    void pauseCAAnimationOnLayer(AnimatedPropertyID, const String& animationName, int index, int subIndex, double timeOffset);

    enum MoveOrCopy { Move, Copy };
    static void moveOrCopyLayerAnimation(MoveOrCopy, const String& animationIdentifier, PlatformCALayer *fromLayer, PlatformCALayer *toLayer);
    void moveOrCopyAnimations(MoveOrCopy, PlatformCALayer * fromLayer, PlatformCALayer * toLayer);
    
    bool appendToUncommittedAnimations(const KeyframeValueList&, const TransformOperations*, const Animation*, const String& animationName, const IntSize& boxSize, int animationIndex, double timeOffset, bool isMatrixAnimation);
#if ENABLE(CSS_FILTERS)
    bool appendToUncommittedAnimations(const KeyframeValueList&, const FilterOperation*, const Animation*, const String& animationName, int animationIndex, double timeOffset);
#endif
    
    enum LayerChange {
        NoChange = 0,
        NameChanged = 1 << 1,
        ChildrenChanged = 1 << 2, // also used for content layer, and preserves-3d, and size if tiling changes?
        GeometryChanged = 1 << 3,
        TransformChanged = 1 << 4,
        ChildrenTransformChanged = 1 << 5,
        Preserves3DChanged = 1 << 6,
        MasksToBoundsChanged = 1 << 7,
        DrawsContentChanged = 1 << 8,
        BackgroundColorChanged = 1 << 9,
        ContentsOpaqueChanged = 1 << 10,
        BackfaceVisibilityChanged = 1 << 11,
        OpacityChanged = 1 << 12,
        AnimationChanged = 1 << 13,
        DirtyRectsChanged = 1 << 14,
        ContentsImageChanged = 1 << 15,
        ContentsMediaLayerChanged = 1 << 16,
        ContentsCanvasLayerChanged = 1 << 17,
        ContentsColorLayerChanged = 1 << 18,
        ContentsRectChanged = 1 << 19,
        MaskLayerChanged = 1 << 20,
        ReplicatedLayerChanged = 1 << 21,
        ContentsNeedsDisplay = 1 << 22,
        AcceleratesDrawingChanged = 1 << 23,
        ContentsScaleChanged = 1 << 24,
        ContentsVisibilityChanged = 1 << 25,
        VisibleRectChanged = 1 << 26,
        FiltersChanged = 1 << 27,
        TilesAdded = 1 < 28,
        DebugIndicatorsChanged = 1 << 29
    };
    typedef unsigned LayerChangeFlags;
    void noteLayerPropertyChanged(LayerChangeFlags flags);
    void noteSublayersChanged();
    void noteChangesForScaleSensitiveProperties();

    void repaintLayerDirtyRects();

    RefPtr<PlatformCALayer> m_layer; // The main layer
    RefPtr<PlatformCALayer> m_structuralLayer; // A layer used for structural reasons, like preserves-3d or replica-flattening. Is the parent of m_layer.
    RefPtr<PlatformCALayer> m_contentsLayer; // A layer used for inner content, like image and video

    // References to clones of our layers, for replicated layers.
    OwnPtr<LayerMap> m_layerClones;
    OwnPtr<LayerMap> m_structuralLayerClones;
    OwnPtr<LayerMap> m_contentsLayerClones;

#ifdef VISIBLE_TILE_WASH
    RefPtr<PlatformCALayer> m_visibleTileWashLayer;
#endif
    FloatRect m_visibleRect;
    FloatSize m_sizeAtLastVisibleRectUpdate;
    
    enum ContentsLayerPurpose {
        NoContentsLayer = 0,
        ContentsLayerForImage,
        ContentsLayerForMedia,
        ContentsLayerForCanvas,
        ContentsLayerForBackgroundColor
    };
    
    ContentsLayerPurpose m_contentsLayerPurpose;
    bool m_allowTiledLayer : 1;
    bool m_isPageTiledBackingLayer : 1;
    
    Color m_contentsSolidColor;

    RetainPtr<CGImageRef> m_uncorrectedContentsImage;
    RetainPtr<CGImageRef> m_pendingContentsImage;
    
    // This represents the animation of a single property. There may be multiple transform animations for
    // a single transition or keyframe animation, so index is used to distinguish these.
    struct LayerPropertyAnimation {
        LayerPropertyAnimation(PassRefPtr<PlatformCAAnimation> caAnimation, const String& animationName, AnimatedPropertyID property, int index, int subIndex, double timeOffset)
        : m_animation(caAnimation)
        , m_name(animationName)
        , m_property(property)
        , m_index(index)
        , m_subIndex(subIndex)
        , m_timeOffset(timeOffset)
        { }

        RefPtr<PlatformCAAnimation> m_animation;
        String m_name;
        AnimatedPropertyID m_property;
        int m_index;
        int m_subIndex;
        double m_timeOffset;
    };
    
    // Uncommitted transitions and animations.
    Vector<LayerPropertyAnimation> m_uncomittedAnimations;
    
    enum Action { Remove, Pause };
    struct AnimationProcessingAction {
        AnimationProcessingAction(Action action = Remove, double timeOffset = 0)
            : action(action)
            , timeOffset(timeOffset)
        {
        }
        Action action;
        double timeOffset; // only used for pause
    };
    typedef HashMap<String, AnimationProcessingAction> AnimationsToProcessMap;
    AnimationsToProcessMap m_animationsToProcess;

    // Map of animation names to their associated lists of property animations, so we can remove/pause them.
    typedef HashMap<String, Vector<LayerPropertyAnimation> > AnimationsMap;
    AnimationsMap m_runningAnimations;

    Vector<FloatRect> m_dirtyRects;
    FloatSize m_pixelAlignmentOffset;
    
    LayerChangeFlags m_uncommittedChanges;
};

} // namespace WebCore


#endif // USE(ACCELERATED_COMPOSITING)

#endif // GraphicsLayerCA_h
