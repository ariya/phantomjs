/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef LayerTreeHostMac_h
#define LayerTreeHostMac_h

#include "LayerTreeHost.h"
#include <WebCore/GraphicsLayerClient.h>
#include <WebCore/LayerFlushScheduler.h>
#include <WebCore/LayerFlushSchedulerClient.h>
#include <wtf/HashMap.h>

namespace WebKit {

class LayerHostingContext;
class PageOverlay;

class LayerTreeHostMac : public LayerTreeHost, private WebCore::GraphicsLayerClient, private WebCore::LayerFlushSchedulerClient {
public:
    static PassRefPtr<LayerTreeHostMac> create(WebPage*);
    virtual ~LayerTreeHostMac();

private:
    explicit LayerTreeHostMac(WebPage*);

    // LayerTreeHost.
    virtual const LayerTreeContext& layerTreeContext() OVERRIDE;
    virtual void scheduleLayerFlush() OVERRIDE;
    virtual void setLayerFlushSchedulingEnabled(bool) OVERRIDE;
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool) OVERRIDE;
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*) OVERRIDE;
    virtual void invalidate() OVERRIDE;
    virtual void setNonCompositedContentsNeedDisplay() OVERRIDE;
    virtual void setNonCompositedContentsNeedDisplayInRect(const WebCore::IntRect&) OVERRIDE;
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect) OVERRIDE;
    virtual void forceRepaint() OVERRIDE;
    virtual void sizeDidChange(const WebCore::IntSize& newSize) OVERRIDE;
    virtual void deviceOrPageScaleFactorChanged() OVERRIDE;
    virtual void pageBackgroundTransparencyChanged() OVERRIDE;

    virtual void didInstallPageOverlay(PageOverlay*) OVERRIDE;
    virtual void didUninstallPageOverlay(PageOverlay*) OVERRIDE;
    virtual void setPageOverlayNeedsDisplay(PageOverlay*, const WebCore::IntRect&) OVERRIDE;

    virtual void pauseRendering() OVERRIDE;
    virtual void resumeRendering() OVERRIDE;

    virtual void setLayerHostingMode(LayerHostingMode) OVERRIDE;

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time) OVERRIDE;
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*) OVERRIDE;
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect) OVERRIDE;
    virtual float deviceScaleFactor() const OVERRIDE;
    virtual void didCommitChangesForLayer(const WebCore::GraphicsLayer*) const OVERRIDE { }

    // LayerFlushSchedulerClient
    virtual bool flushLayers();

    void initialize();
    void performScheduledLayerFlush();
    bool flushPendingLayerChanges();

    void createPageOverlayLayer(PageOverlay*);
    void destroyPageOverlayLayer(PageOverlay*);

    bool m_isValid;
    bool m_notifyAfterScheduledLayerFlush;

    LayerTreeContext m_layerTreeContext;

    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;
    OwnPtr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;
    typedef HashMap<PageOverlay*, OwnPtr<WebCore::GraphicsLayer>> PageOverlayLayerMap;
    PageOverlayLayerMap m_pageOverlayLayers;

    OwnPtr<LayerHostingContext> m_layerHostingContext;
    WebCore::LayerFlushScheduler m_layerFlushScheduler;
};

} // namespace WebKit

#endif // LayerTreeHostMac_h
