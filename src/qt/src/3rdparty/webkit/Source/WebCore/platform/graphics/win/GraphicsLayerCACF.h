/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef GraphicsLayerCACF_h
#define GraphicsLayerCACF_h

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayer.h"
#include "GraphicsContext.h"
#include <wtf/RetainPtr.h>

namespace WebCore {

class WKCACFLayer;

class GraphicsLayerCACF : public GraphicsLayer {
public:

    GraphicsLayerCACF(GraphicsLayerClient*);
    virtual ~GraphicsLayerCACF();

    virtual void setName(const String& inName);

    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer *layer);
    virtual void addChildAtIndex(GraphicsLayer *layer, int index);
    virtual void addChildAbove(GraphicsLayer *layer, GraphicsLayer *sibling);
    virtual void addChildBelow(GraphicsLayer *layer, GraphicsLayer *sibling);
    virtual bool replaceChild(GraphicsLayer *oldChild, GraphicsLayer *newChild);

    virtual void removeFromParent();

    virtual void setPosition(const FloatPoint& inPoint);
    virtual void setAnchorPoint(const FloatPoint3D& inPoint);
    virtual void setSize(const FloatSize& inSize);

    virtual void setTransform(const TransformationMatrix&);

    virtual void setChildrenTransform(const TransformationMatrix&);

    virtual void setPreserves3D(bool);
    virtual void setMasksToBounds(bool);
    virtual void setDrawsContent(bool);

    virtual void setBackgroundColor(const Color&);
    virtual void clearBackgroundColor();

    virtual void setContentsOpaque(bool);
    virtual void setBackfaceVisibility(bool);

    virtual void setOpacity(float opacity);

    virtual void setNeedsDisplay();
    virtual void setNeedsDisplayInRect(const FloatRect&);

    virtual void setContentsRect(const IntRect&);

    virtual void setContentsToImage(Image*);
    virtual void setContentsToMedia(PlatformLayer*);
    
    virtual PlatformLayer* platformLayer() const;

    virtual void setDebugBackgroundColor(const Color&);
    virtual void setDebugBorder(const Color&, float borderWidth);

private:
    void updateOpacityOnLayer();

    WKCACFLayer* primaryLayer() const  { return m_transformLayer.get() ? m_transformLayer.get() : m_layer.get(); }
    WKCACFLayer* hostLayerForSublayers() const;
    WKCACFLayer* layerForSuperlayer() const;

    bool requiresTiledLayer(const FloatSize&) const;
    void swapFromOrToTiledLayer(bool useTiledLayer);

    CompositingCoordinatesOrientation defaultContentsOrientation() const;
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
    void updateLayerDrawsContent();
    void updateLayerBackgroundColor();

    void updateContentsImage();
    void updateContentsMedia();
    void updateContentsRect();
    
    void setupContentsLayer(WKCACFLayer*);
    WKCACFLayer* contentsLayer() const { return m_contentsLayer.get(); }
    
    RefPtr<WKCACFLayer> m_layer;
    RefPtr<WKCACFLayer> m_transformLayer;
    RefPtr<WKCACFLayer> m_contentsLayer;

    enum ContentsLayerPurpose {
        NoContentsLayer = 0,
        ContentsLayerForImage,
        ContentsLayerForMedia
    };
    
    ContentsLayerPurpose m_contentsLayerPurpose;
    bool m_contentsLayerHasBackgroundColor : 1;
    RetainPtr<CGImageRef> m_pendingContentsImage;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // GraphicsLayerCACF_h
