/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LayerTreeHostGtk_h
#define LayerTreeHostGtk_h

#if USE(TEXTURE_MAPPER_GL)

#include "LayerTreeContext.h"
#include "LayerTreeHost.h"
#include "TextureMapperLayer.h"
#include <WebCore/GLContext.h>
#include <WebCore/GraphicsLayerClient.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

class LayerTreeHostGtk : public LayerTreeHost, WebCore::GraphicsLayerClient {
public:
    static PassRefPtr<LayerTreeHostGtk> create(WebPage*);
    virtual ~LayerTreeHostGtk();

protected:
    explicit LayerTreeHostGtk(WebPage*);

    WebCore::GraphicsLayer* rootLayer() const { return m_rootLayer.get(); }

    void initialize();

    // LayerTreeHost.
    virtual void invalidate();
    virtual void sizeDidChange(const WebCore::IntSize& newSize);
    virtual void deviceOrPageScaleFactorChanged();
    virtual void forceRepaint();
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*);
    virtual void scheduleLayerFlush();
    virtual void setLayerFlushSchedulingEnabled(bool layerFlushingEnabled);
    virtual void pageBackgroundTransparencyChanged() OVERRIDE;

private:
    // LayerTreeHost.
    virtual const LayerTreeContext& layerTreeContext();
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool);

    virtual void setNonCompositedContentsNeedDisplay() OVERRIDE;
    virtual void setNonCompositedContentsNeedDisplayInRect(const WebCore::IntRect&) OVERRIDE;
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect);

    virtual void didInstallPageOverlay(PageOverlay*) OVERRIDE;
    virtual void didUninstallPageOverlay(PageOverlay*) OVERRIDE;
    virtual void setPageOverlayNeedsDisplay(PageOverlay*, const WebCore::IntRect&) OVERRIDE;

    virtual bool flushPendingLayerChanges();

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time);
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect);
    virtual void didCommitChangesForLayer(const WebCore::GraphicsLayer*) const { }

    void createPageOverlayLayer(PageOverlay*);
    void destroyPageOverlayLayer(PageOverlay*);

    enum CompositePurpose { ForResize, NotForResize };
    void compositeLayersToContext(CompositePurpose = NotForResize);

    void flushAndRenderLayers();
    void cancelPendingLayerFlush();

    void layerFlushTimerFired();
    static gboolean layerFlushTimerFiredCallback(LayerTreeHostGtk*);

    WebCore::GLContext* glContext();

    LayerTreeContext m_layerTreeContext;
    bool m_isValid;
    bool m_notifyAfterScheduledLayerFlush;
    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;
    OwnPtr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;
    typedef HashMap<PageOverlay*, OwnPtr<WebCore::GraphicsLayer> > PageOverlayLayerMap;
    PageOverlayLayerMap m_pageOverlayLayers;
    OwnPtr<WebCore::TextureMapper> m_textureMapper;
    OwnPtr<WebCore::GLContext> m_context;
    double m_lastFlushTime;
    bool m_layerFlushSchedulingEnabled;
    unsigned m_layerFlushTimerCallbackId;
};

} // namespace WebKit

#endif

#endif // LayerTreeHostGtk_h
