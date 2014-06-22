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

#ifndef GraphicsLayerBlackBerry_h
#define GraphicsLayerBlackBerry_h

#if USE(ACCELERATED_COMPOSITING)

#include "FilterOperations.h"
#include "GraphicsLayer.h"
#include <wtf/Vector.h>

namespace WebCore {

class LayerAnimation;
class LayerWebKitThread;

class GraphicsLayerBlackBerry : public GraphicsLayer {
public:
    GraphicsLayerBlackBerry(GraphicsLayerClient*);
    virtual ~GraphicsLayerBlackBerry();

    virtual void setName(const String&);

    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer*, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer*, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);

    virtual void removeFromParent();

    virtual void setPosition(const FloatPoint&);
    virtual void setAnchorPoint(const FloatPoint3D&);
    virtual void setSize(const FloatSize&);

    virtual void setTransform(const TransformationMatrix&);

    virtual void setChildrenTransform(const TransformationMatrix&);

    virtual void setPreserves3D(bool);
    virtual void setMasksToBounds(bool);
    virtual void setDrawsContent(bool);
    virtual void setContentsVisible(bool);
    virtual void setMaskLayer(GraphicsLayer*);
    virtual void setReplicatedByLayer(GraphicsLayer*);
    virtual void setHasFixedContainer(bool);
    virtual void setHasFixedAncestorInDOMTree(bool);

#if ENABLE(CSS_FILTERS)
    // Returns true if filter can be rendered by the compositor
    virtual bool setFilters(const FilterOperations &);
    const FilterOperations& filters() const { return m_filters; }
#endif

    virtual void setBackgroundColor(const Color&);

    virtual void setContentsOpaque(bool);
    virtual void setBackfaceVisibility(bool);

    virtual void setOpacity(float);

    virtual void setNeedsDisplay();
    virtual void setNeedsDisplayInRect(const FloatRect&);

    virtual void setContentsNeedsDisplay();

    virtual void setContentsRect(const IntRect&);

    virtual bool addAnimation(const KeyframeValueList&, const IntSize& boxSize, const Animation*, const String& animationName, double timeOffset);
    virtual void pauseAnimation(const String& animationName, double timeOffset);
    virtual void removeAnimation(const String& animationName);

    virtual void suspendAnimations(double time);
    virtual void resumeAnimations();

    virtual void setContentsToImage(Image*);
    virtual void setContentsToMedia(PlatformLayer*);
    virtual void setContentsToCanvas(PlatformLayer*);

    virtual PlatformLayer* platformLayer() const;

    virtual void setDebugBackgroundColor(const Color&);
    virtual void setDebugBorder(const Color&, float borderWidth);

    void notifyFlushRequired()
    {
        if (m_client)
            m_client->notifyFlushRequired(this);
    }

    void notifyAnimationStarted(double time)
    {
        if (m_client)
            m_client->notifyAnimationStarted(this, time);
    }

private:
    virtual void willBeDestroyed();

    void updateOpacityOnLayer();

    LayerWebKitThread* primaryLayer() const  { return m_transformLayer.get() ? m_transformLayer.get() : m_layer.get(); }
    LayerWebKitThread* hostLayerForSublayers() const;
    LayerWebKitThread* layerForSuperlayer() const;

    void updateSublayerList();
    void updateLayerPosition();
    void updateLayerSize();
    void updateAnchorPoint();
    void updateTransform();
    void updateChildrenTransform();
    void updateMasksToBounds();
    void updateContentsOpaque();
    void updateBackfaceVisibility();
    void updateLayerPreserves3D();
    void updateLayerIsDrawable();
    void updateHasFixedContainer();
    void updateHasFixedAncestorInDOMTree();
    void updateLayerBackgroundColor();
#if ENABLE(CSS_FILTERS)
    void updateFilters();
#endif
    void updateAnimations();

    void updateContentsImage(Image*);
    void updateContentsVideo();
    void updateContentsRect();

    void setupContentsLayer(LayerWebKitThread*);
    LayerWebKitThread* contentsLayer() const { return m_contentsLayer.get(); }

    RefPtr<LayerWebKitThread> m_layer;
    RefPtr<LayerWebKitThread> m_transformLayer;
    RefPtr<LayerWebKitThread> m_contentsLayer;

#if ENABLE(CSS_FILTERS)
    FilterOperations m_filters;
#endif

    Vector<RefPtr<LayerAnimation> > m_runningAnimations;
    Vector<RefPtr<LayerAnimation> > m_suspendedAnimations;
    double m_suspendTime;

    enum ContentsLayerPurpose {
        NoContentsLayer = 0,
        ContentsLayerForImage,
        ContentsLayerForVideo,
        ContentsLayerForCanvas,
    };

    ContentsLayerPurpose m_contentsLayerPurpose;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif
