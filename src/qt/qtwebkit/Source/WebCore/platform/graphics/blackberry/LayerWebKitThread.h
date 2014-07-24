/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef LayerWebKitThread_h
#define LayerWebKitThread_h

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerBlackBerry.h"
#include "LayerAnimation.h"
#include "LayerData.h"
#include "LayerTiler.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class LayerCompositingThread;

class LayerWebKitThread : public RefCounted<LayerWebKitThread>, public LayerData {
public:
    static PassRefPtr<LayerWebKitThread> create(LayerType, GraphicsLayerBlackBerry* owner = 0);

    virtual ~LayerWebKitThread();

    void addSublayer(PassRefPtr<LayerWebKitThread>);
    void insertSublayer(PassRefPtr<LayerWebKitThread> layer, size_t index) { insert(m_sublayers, layer, index); }
    void replaceSublayer(LayerWebKitThread* reference, PassRefPtr<LayerWebKitThread> newLayer);
    void removeFromSuperlayer();

    void addOverlay(PassRefPtr<LayerWebKitThread>);

    void setAnchorPoint(const FloatPoint& anchorPoint) { m_anchorPoint = anchorPoint; setNeedsCommit(); }

    void setAnchorPointZ(float anchorPointZ) { m_anchorPointZ = anchorPointZ; setNeedsCommit(); }

    void setBackgroundColor(const Color& color) { m_backgroundColor = color; setNeedsCommit(); }

    void setBorderColor(const Color& color) { m_borderColor = color; setNeedsCommit(); }

    void setBorderWidth(float width) { m_borderWidth = width; setNeedsCommit(); }

    void setBounds(const IntSize&);

    void setSizeIsScaleInvariant(bool invariant) { m_sizeIsScaleInvariant = invariant; setNeedsCommit(); }

    void setDoubleSided(bool doubleSided) { m_doubleSided = doubleSided; setNeedsCommit(); }

    void setFrame(const FloatRect&);

    void setMasksToBounds(bool masksToBounds) { m_masksToBounds = masksToBounds; }

    void setMaskLayer(LayerWebKitThread* maskLayer) { m_maskLayer = maskLayer; }
    LayerWebKitThread* maskLayer() const { return m_maskLayer.get(); }

    bool isMask() const { return m_isMask; }
    void setIsMask(bool);

    void setReplicaLayer(LayerWebKitThread* layer) { m_replicaLayer = layer; }
    LayerWebKitThread* replicaLayer() { return m_replicaLayer.get(); }

    // FIXME: Move to a new subclass, ContentLayerWebKitThread. This is only used for layers that draw content
    void setNeedsDisplayInRect(const FloatRect& dirtyRect);

    virtual void setNeedsDisplay();

    void setNeedsDisplayOnBoundsChange(bool needsDisplay) { m_needsDisplayOnBoundsChange = needsDisplay; }

    void setOpacity(float opacity) { m_opacity = opacity; setNeedsCommit(); }

#if ENABLE(CSS_FILTERS)
    void setFilters(const FilterOperations& filters) { m_filters = filters; m_filtersChanged = true; setNeedsCommit(); }
    static bool filtersCanBeComposited(const FilterOperations& filters);
#endif

    void setOpaque(bool isOpaque) { m_isOpaque = isOpaque; setNeedsCommit(); }

    void setPosition(const FloatPoint& position) { m_position = position; setNeedsCommit(); }

    const LayerWebKitThread* rootLayer() const;

    void removeAllSublayers() { removeAll(m_sublayers); }

    void setSublayers(const Vector<RefPtr<LayerWebKitThread> >&);

    const Vector<RefPtr<LayerWebKitThread> >& sublayers() const { return m_sublayers; }

    void setSublayerTransform(const TransformationMatrix& transform) { m_sublayerTransform = transform; setNeedsCommit(); }

    LayerWebKitThread* superlayer() const { return m_superlayer; }

    void setTransform(const TransformationMatrix& transform) { m_transform = transform; setNeedsCommit(); }

    void setPreserves3D(bool preserves3D) { m_preserves3D = preserves3D; setNeedsCommit(); }

    void setFixedPosition(bool fixed)
    {
        if (m_isFixedPosition == fixed)
            return;
        m_isFixedPosition = fixed;
        setNeedsCommit();
    }

    void setHasFixedContainer(bool fixed) { m_hasFixedContainer = fixed; setNeedsCommit(); }
    void setHasFixedAncestorInDOMTree(bool fixed) { m_hasFixedAncestorInDOMTree = fixed; setNeedsCommit(); }

    void setIsContainerForFixedPositionLayers(bool fixed)
    {
        if (m_isContainerForFixedPositionLayers == fixed)
            return;
        m_isContainerForFixedPositionLayers = fixed;
        setNeedsCommit();
    }

    void setFixedToTop(bool fixedToTop)
    {
        if (m_isFixedToTop == fixedToTop)
            return;
        m_isFixedToTop = fixedToTop;
        setNeedsCommit();
    }

    void setFixedToLeft(bool fixedToLeft)
    {
        if (m_isFixedToLeft == fixedToLeft)
            return;
        m_isFixedToLeft = fixedToLeft;
        setNeedsCommit();
    }

    void setFrameVisibleRect(const IntRect& rect)
    {
        if (m_frameVisibleRect == rect)
            return;
        m_frameVisibleRect = rect;
        setNeedsCommit();
    }

    void setFrameContentsSize(const IntSize& size)
    {
        if (m_frameContentsSize == size)
            return;
        m_frameContentsSize = size;
        setNeedsCommit();
    }

    void setContents(Image*);
    Image* contents() const { return m_contents.get(); }

    void setOwner(GraphicsLayerBlackBerry* owner) { m_owner = owner; }
    // NOTE: Can be 0.
    GraphicsLayerBlackBerry* owner() const { return m_owner; }

    bool drawsContent() const { return m_owner && m_owner->drawsContent(); }
    void setDrawable(bool);

    // 1. Commit on WebKit thread
    void commitOnWebKitThread(double scale);

    // 2. Commit on Compositing thread
    void commitOnCompositingThread();
    bool startAnimations(double time);

    // 3. Notify when returning to WebKit thread
    void notifyAnimationsStarted(double time);

    LayerCompositingThread* layerCompositingThread() const { return m_layerCompositingThread.get(); }

    // Only used when this layer is the root layer of a frame.
    void setAbsoluteOffset(const FloatSize& offset) { m_absoluteOffset = offset; }

    void paintContents(BlackBerry::Platform::Graphics::Buffer*, const IntRect& transformedContentsRect, double scale);

    void setNeedsCommit();

    void setRunningAnimations(const Vector<RefPtr<LayerAnimation> >&);
    void setSuspendedAnimations(const Vector<RefPtr<LayerAnimation> >&);

    // Allows you to clear the LayerCompositingThread::overrides from the WK thread
    void clearOverride() { m_clearOverrideOnCommit = true; setNeedsCommit(); }

    void releaseLayerResources();

    static IntRect mapFromTransformed(const IntRect&, double scale);

protected:
    LayerWebKitThread(LayerType, GraphicsLayerBlackBerry* owner);

    void setNeedsTexture(bool needsTexture) { m_needsTexture = needsTexture; }
    void setLayerProgram(LayerData::LayerProgram layerProgram) { m_layerProgram = layerProgram; }
    bool isDrawable() const { return m_isDrawable; }

    void updateVisibility();
    void updateTextureContents(double scale);

    virtual void boundsChanged() { }
    virtual void updateTextureContentsIfNeeded();
    virtual void commitPendingTextureUploads();
    virtual void deleteTextures() { }

private:
    void updateLayerHierarchy();

    void setSuperlayer(LayerWebKitThread* superlayer) { m_superlayer = superlayer; }

    // This should only be called from removeFromSuperlayer.
    void removeSublayerOrOverlay(LayerWebKitThread*);
    void remove(Vector<RefPtr<LayerWebKitThread> >&, LayerWebKitThread*);
    void removeAll(Vector<RefPtr<LayerWebKitThread> >&);
    void insert(Vector<RefPtr<LayerWebKitThread> >&, PassRefPtr<LayerWebKitThread>, size_t index);

    GraphicsLayerBlackBerry* m_owner;

    Vector<RefPtr<LayerAnimation> > m_runningAnimations;
    Vector<RefPtr<LayerAnimation> > m_suspendedAnimations;

    Vector<RefPtr<LayerWebKitThread> > m_sublayers;
    Vector<RefPtr<LayerWebKitThread> > m_overlays;
    LayerWebKitThread* m_superlayer;
    RefPtr<LayerWebKitThread> m_maskLayer;
    RefPtr<LayerWebKitThread> m_replicaLayer;

    RefPtr<Image> m_contents;

    RefPtr<LayerCompositingThread> m_layerCompositingThread;
    RefPtr<LayerTiler> m_tiler;
    FloatSize m_absoluteOffset;
    unsigned m_isDrawable : 1;
    unsigned m_isMask : 1;
    unsigned m_animationsChanged : 1;
    unsigned m_clearOverrideOnCommit : 1;
#if ENABLE(CSS_FILTERS)
    unsigned m_filtersChanged : 1;
#endif
    unsigned m_didStartAnimations : 1;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif
