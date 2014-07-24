/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebPageCompositor_p_h
#define WebPageCompositor_p_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerCompositingThread.h"
#include "LayerRenderer.h"
#include "LayerRendererClient.h"
#include "WebPageCompositor.h"

#include <BlackBerryPlatformAnimationFrameRateController.h>
#include <BlackBerryPlatformGLES2Context.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class LayerWebKitThread;
};

namespace BlackBerry {
namespace WebKit {

class WebPageCompositorClient;
class WebPagePrivate;

// This class may only be used on the compositing thread. So it does not need to be threadsaferefcounted.
class WebPageCompositorPrivate
    : public RefCounted<WebPageCompositorPrivate>
    , public WebCore::LayerRendererClient
    , public Platform::AnimationFrameRateClient {
public:
    static PassRefPtr<WebPageCompositorPrivate> create(WebPagePrivate* page, WebPageCompositorClient* client)
    {
        return adoptRef(new WebPageCompositorPrivate(page, client));
    }

    ~WebPageCompositorPrivate();

    // Public API
    void setChildWindowPlacement(WebPageCompositor::ChildWindowPlacement placement) { m_childWindowPlacement = placement; }
    void prepareFrame(double animationTime);
    void render(const WebCore::IntRect& targetRect, const WebCore::IntRect& clipRect, const WebCore::TransformationMatrix&, const WebCore::FloatRect& documentSrcRect);

    Platform::Graphics::GLES2Context* context() const { return m_context; }
    void setContext(Platform::Graphics::GLES2Context*);

    WebCore::LayerCompositingThread* rootLayer() const { return m_rootLayer.get(); }
    void setRootLayer(WebCore::LayerCompositingThread*);

    WebCore::LayerCompositingThread* overlayLayer() const { return m_overlayLayer.get(); }
    void setOverlayLayer(WebCore::LayerCompositingThread*);

    WebCore::LayerCompositingThread* compositingThreadOverlayLayer() const { return m_compositingThreadOverlayLayer.get(); }

    bool drawsRootLayer() const;
    void setDrawsRootLayer(bool drawsRootLayer) { m_drawsRootLayer = drawsRootLayer; }

    // Render everything but the root layer, or everything if drawsRootLayer() is true.
    bool drawLayers(const WebCore::IntRect& dstRect, const WebCore::FloatRect& contents);

    WebCore::IntRect layoutRect() const { return m_layoutRect; }
    void setLayoutRect(const WebCore::IntRect& rect) { m_layoutRect = rect; }

    WebCore::IntRect documentRect() const { return m_documentRect; }
    void setDocumentRect(const WebCore::IntRect& rect) { m_documentRect = rect; }

    WebCore::LayerRenderingResults lastCompositingResults() const { return m_lastCompositingResults; }
    void setLastCompositingResults(const WebCore::LayerRenderingResults& results) { m_lastCompositingResults = results; }

    WebCore::Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const WebCore::Color&);

    void releaseLayerResources();

    WebPagePrivate* page() const { return m_webPage; }
    void setPage(WebPagePrivate*);
    void detach();
    WebPageCompositorClient* client() const { return m_client; }
    void compositorDestroyed();

    void addOverlay(WebCore::LayerCompositingThread*);
    void removeOverlay(WebCore::LayerCompositingThread*);

    void findFixedElementRect(WebCore::LayerCompositingThread*, WebCore::IntRect&);

protected:
    WebPageCompositorPrivate(WebPagePrivate*, WebPageCompositorClient*);

private:
    void compositeLayers(const WebCore::TransformationMatrix&);
    void attachOverlays() { attachOverlays(m_compositingThreadOverlayLayer.get(), m_webPage); }
    void detachOverlays() { attachOverlays(m_compositingThreadOverlayLayer.get(), 0); }
    static void attachOverlays(WebCore::LayerCompositingThread* overlayRoot, WebPagePrivate*);

    // LayerRendererClient
    virtual bool shouldChildWindowsUseDocumentCoordinates();

    // AnimationFrameRateClient
    virtual void animationFrameChanged();

    WebPageCompositorClient* m_client;
    WebPagePrivate* m_webPage;
    Platform::Graphics::GLES2Context* m_context;
    OwnPtr<WebCore::LayerRenderer> m_layerRenderer;
    RefPtr<WebCore::LayerCompositingThread> m_rootLayer;
    RefPtr<WebCore::LayerCompositingThread> m_overlayLayer;
    RefPtr<WebCore::LayerCompositingThread> m_compositingThreadOverlayLayer;
    WebCore::IntRect m_layoutRect;
    WebCore::IntRect m_documentRect;
    WebCore::LayerRenderingResults m_lastCompositingResults;
    WebCore::Color m_backgroundColor;
    bool m_drawsRootLayer;
    WebPageCompositor::ChildWindowPlacement m_childWindowPlacement;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)

#endif // WebPageCompositor_p_h
