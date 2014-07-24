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


#ifndef LayerCompositingThread_h
#define LayerCompositingThread_h

#if USE(ACCELERATED_COMPOSITING)

#include "FilterOperations.h"
#include "FloatQuad.h"
#include "LayerAnimation.h"
#include "LayerData.h"
#include "LayerFilterRenderer.h"
#include "LayerRendererSurface.h"
#include "LayerTiler.h"

#include <BlackBerryPlatformGuardedPointer.h>
#include <GuardedPointerDeleter.h>

namespace BlackBerry {
namespace Platform {
namespace Graphics {
class Buffer;
class GLES2Program;
}
}
}

namespace WebCore {

class LayerCompositingThreadClient;
class LayerRenderer;

class LayerOverride {
public:
    static PassOwnPtr<LayerOverride> create() { return adoptPtr(new LayerOverride()); }

    bool isPositionSet() const { return m_positionSet; }
    FloatPoint position() const { return m_position; }
    void setPosition(const FloatPoint& position) { m_position = position; m_positionSet = true; }

    bool isAnchorPointSet() const { return m_anchorPointSet; }
    FloatPoint anchorPoint() const { return m_anchorPoint; }
    void setAnchorPoint(const FloatPoint& anchorPoint) { m_anchorPoint = anchorPoint; m_anchorPointSet = true; }

    bool isBoundsSet() const { return m_boundsSet; }
    IntSize bounds() const { return m_bounds; }
    void setBounds(const IntSize& bounds) { m_bounds = bounds; m_boundsSet = true; }

    bool isTransformSet() const { return m_transformSet; }
    const TransformationMatrix& transform() const { return m_transform; }
    void setTransform(const TransformationMatrix& transform) { m_transform = transform; m_transformSet = true; }

    bool isOpacitySet() const { return m_opacitySet; }
    float opacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; m_opacitySet = true; }

    const Vector<RefPtr<LayerAnimation> >& animations() const { return m_animations; }
    void addAnimation(PassRefPtr<LayerAnimation> animation) { m_animations.append(animation); }
    void removeAnimation(const String& name);

private:
    LayerOverride()
        : m_opacity(1.0)
        , m_positionSet(false)
        , m_anchorPointSet(false)
        , m_boundsSet(false)
        , m_transformSet(false)
        , m_opacitySet(false)
    {
    }

    FloatPoint m_position;
    FloatPoint m_anchorPoint;
    IntSize m_bounds;
    TransformationMatrix m_transform;
    float m_opacity;

    Vector<RefPtr<LayerAnimation> > m_animations;

    unsigned m_positionSet : 1;
    unsigned m_anchorPointSet : 1;
    unsigned m_boundsSet : 1;
    unsigned m_transformSet : 1;
    unsigned m_opacitySet : 1;
};

class LayerFilterRendererAction;

class LayerCompositingThread : public ThreadSafeRefCounted<LayerCompositingThread>, public LayerData, public BlackBerry::Platform::GuardedPointerBase {
public:
    static PassRefPtr<LayerCompositingThread> create(LayerType, LayerCompositingThreadClient*);

    LayerCompositingThreadClient* client() const { return m_client; }
    void setClient(LayerCompositingThreadClient* client) { m_client = client; }

    // Thread safe
    void setPluginView(PluginView*);
#if ENABLE(VIDEO)
    void setMediaPlayer(MediaPlayer*);
#endif

    // Not thread safe

    // These will be overwritten on the next commit if this layer has a LayerWebKitThread counterpart.
    // Useful for stand-alone layers that are created and managed on the compositing thread.
    // These functions can also be used to update animated properties in LayerAnimation.
    void setPosition(const FloatPoint& position) { m_position = position; }
    void setAnchorPoint(const FloatPoint& anchorPoint) { m_anchorPoint = anchorPoint; }
    void setBounds(const IntSize& bounds) { m_bounds = bounds; }
    void setSizeIsScaleInvariant(bool invariant) { m_sizeIsScaleInvariant = invariant; }
    void setTransform(const TransformationMatrix& matrix) { m_transform = matrix; }
    void setOpacity(float opacity) { m_opacity = opacity; }
    void addSublayer(LayerCompositingThread*);
    void removeFromSuperlayer();
    void setNeedsTexture(bool needsTexture) { m_needsTexture = needsTexture; }

    void commitPendingTextureUploads();

    // Returns true if we have an animation
    bool updateAnimations(double currentTime);
    void updateTextureContentsIfNeeded();
    LayerTexture* contentsTexture();

    const LayerCompositingThread* rootLayer() const;
    void setSublayers(const Vector<RefPtr<LayerCompositingThread> >&);
    const Vector<RefPtr<LayerCompositingThread> >& sublayers() const { return m_sublayers; }
    void setSuperlayer(LayerCompositingThread* superlayer) { m_superlayer = superlayer; }
    LayerCompositingThread* superlayer() const { return m_superlayer; }

    // The layer renderer must be set if the layer has been rendered
    LayerRenderer* layerRenderer() const { return m_layerRenderer; }
    void setLayerRenderer(LayerRenderer*);

    // The draw transform expects the origin to be located at the center of the layer.
    FloatPoint origin() const { return FloatPoint(m_bounds.width() / 2.0f, m_bounds.height() / 2.0f); }

    void setDrawTransform(double scale, const TransformationMatrix& modelViewMatrix, const TransformationMatrix& projectionMatrix);
    const TransformationMatrix& drawTransform() const { return m_drawTransform; }

    void setDrawOpacity(float opacity) { m_drawOpacity = opacity; }
    float drawOpacity() const { return m_drawOpacity; }

    void createLayerRendererSurface();
    LayerRendererSurface* layerRendererSurface() const { return m_layerRendererSurface.get(); }
    void clearLayerRendererSurface() { m_layerRendererSurface.clear(); }

    void setMaskLayer(LayerCompositingThread* maskLayer) { m_maskLayer = maskLayer; }
    LayerCompositingThread* maskLayer() const { return m_maskLayer.get(); }

    void setReplicaLayer(LayerCompositingThread* layer) { m_replicaLayer = layer; }
    LayerCompositingThread* replicaLayer() const { return m_replicaLayer.get(); }

    // These use normalized device coordinates
    FloatRect boundingBox() const { return m_boundingBox; }
    // The bounds are processed according to http://www.w3.org/TR/css3-transforms paragraph 6.2, which can result in a polygon with more than 4 sides.
    const Vector<FloatPoint, 4>& transformedBounds() const { return m_transformedBounds; }
    const Vector<float, 4>& ws() const { return m_ws; }

    enum TextureCoordinateOrientation {
        RightSideUp = 0,
        UpsideDown
    };

    const Vector<FloatPoint>& textureCoordinates(TextureCoordinateOrientation = RightSideUp) const;
    FloatQuad transformedHolePunchRect() const;
    float centerW() const { return m_centerW; }

    void deleteTextures();

    void drawTextures(const BlackBerry::Platform::Graphics::GLES2Program&, double scale, const FloatRect& visibleRect, const FloatRect& clipRect);
    void drawSurface(const BlackBerry::Platform::Graphics::GLES2Program&, const TransformationMatrix&, LayerCompositingThread* mask);

    void releaseTextureResources();

    // Layer visibility is determined by the LayerRenderer when drawing.
    // So we don't have an accurate value for visibility until it's too late,
    // but the attribute still is useful.
    bool isVisible() const { return m_visible; }
    void setVisible(bool);

    // This will cause a commit of the whole layer tree on the WebKit thread,
    // sometime after rendering is finished. Used when rendering results in a
    // need for commit, for example when a dirty layer becomes visible.
    void setNeedsCommit();

    // Normally you would schedule a commit from the webkit thread, but
    // this allows you to do it from the compositing thread.
    void scheduleCommit();

    bool hasRunningAnimations() const { return !m_runningAnimations.isEmpty(); }

    bool hasVisibleHolePunchRect() const;

    void addAnimation(LayerAnimation* animation) { m_runningAnimations.append(animation); }
    void removeAnimation(const String& name);

    void setRunningAnimations(const Vector<RefPtr<LayerAnimation> >& animations) { m_runningAnimations = animations; }
    void setSuspendedAnimations(const Vector<RefPtr<LayerAnimation> >& animations) { m_suspendedAnimations = animations; }

    LayerOverride* override();
    void clearOverride();

#if ENABLE(CSS_FILTERS)
    bool filterOperationsChanged() const { return m_filterOperationsChanged; }
    void setFilterOperationsChanged(bool changed) { m_filterOperationsChanged = changed; }

    Vector<RefPtr<LayerFilterRendererAction> > filterActions() const { return m_filterActions; }
    void setFilterActions(const Vector<RefPtr<LayerFilterRendererAction> >& actions) { m_filterActions = actions; }
#endif

protected:
    virtual ~LayerCompositingThread();

private:
    LayerCompositingThread(LayerType, LayerCompositingThreadClient*);

    void updateTileContents(const IntRect& tile);

    // Returns the index of the sublayer or -1 if not found.
    int indexOfSublayer(const LayerCompositingThread*);

    // This should only be called from removeFromSuperlayer.
    void removeSublayer(LayerCompositingThread*);

    LayerRenderer* m_layerRenderer;

    typedef Vector<RefPtr<LayerCompositingThread> > LayerList;
    LayerList m_sublayers;
    LayerCompositingThread* m_superlayer;

    // Vertex data for the bounds of this layer
    Vector<FloatPoint, 4> m_transformedBounds;
    Vector<float, 4> m_ws;
    Vector<FloatPoint> m_textureCoordinates; // Only used when a 3D layer is clipped against z = 0
    float m_centerW;
    FloatRect m_boundingBox;

    OwnPtr<LayerRendererSurface> m_layerRendererSurface;

    RefPtr<LayerCompositingThread> m_maskLayer;
    RefPtr<LayerCompositingThread> m_replicaLayer;

    BlackBerry::Platform::Graphics::Buffer* m_pluginBuffer;

    // The global property values, after concatenation with parent values
    TransformationMatrix m_drawTransform;
    float m_drawOpacity;

    bool m_visible;
    bool m_commitScheduled;

    Vector<RefPtr<LayerAnimation> > m_runningAnimations;
    Vector<RefPtr<LayerAnimation> > m_suspendedAnimations;

    OwnPtr<LayerOverride> m_override;
    LayerCompositingThreadClient* m_client;

#if ENABLE(CSS_FILTERS)
    bool m_filterOperationsChanged;
    Vector<RefPtr<LayerFilterRendererAction> > m_filterActions;
#endif
};

} // namespace WebCore

namespace WTF {

// LayerCompositingThread objects must be destroyed on the compositing thread.
// But it's possible for the last reference to be held by the WebKit thread.
// So we create a custom specialization of ThreadSafeRefCounted which calls a
// function that ensures the destructor is called on the correct thread, rather
// than calling delete directly.
template<>
inline void ThreadSafeRefCounted<WebCore::LayerCompositingThread>::deref()
{
    if (derefBase()) {
        // Delete on the compositing thread.
        BlackBerry::Platform::GuardedPointerDeleter::deleteOnThread(
            BlackBerry::Platform::userInterfaceThreadMessageClient(),
            static_cast<WebCore::LayerCompositingThread*>(this));
    }
}

} // namespace WTF


#endif // USE(ACCELERATED_COMPOSITING)

#endif
