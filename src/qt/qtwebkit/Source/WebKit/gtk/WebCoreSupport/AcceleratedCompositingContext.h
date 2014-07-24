/*
 * Copyright (C) 2011 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef AcceleratedCompositingContext_h
#define AcceleratedCompositingContext_h

#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#include "IntRect.h"
#include "IntSize.h"
#include "Timer.h"
#include "webkitwebview.h"
#include <wtf/PassOwnPtr.h>

#if USE(TEXTURE_MAPPER)
#include "TextureMapperLayer.h"
#endif

#if USE(TEXTURE_MAPPER_GL)
#include "GLContext.h"
#include "RedirectedXCompositeWindow.h"
#include "TextureMapperFPSCounter.h"
#endif

#if USE(ACCELERATED_COMPOSITING)

namespace WebKit {

class AcceleratedCompositingContext : public WebCore::GraphicsLayerClient {
    WTF_MAKE_NONCOPYABLE(AcceleratedCompositingContext);
public:
    static PassOwnPtr<AcceleratedCompositingContext> create(WebKitWebView* webView)
    {
        return adoptPtr(new AcceleratedCompositingContext(webView));
    }

    virtual ~AcceleratedCompositingContext();
    void setRootCompositingLayer(WebCore::GraphicsLayer*);
    void setNonCompositedContentsNeedDisplay(const WebCore::IntRect&);
    void scheduleLayerFlush();
    void resizeRootLayer(const WebCore::IntSize&);
    bool renderLayersToWindow(cairo_t*, const WebCore::IntRect& clipRect);
    bool enabled();

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time);
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& rectToPaint);

    void initialize();

    enum CompositePurpose { ForResize, NotForResize };
    void compositeLayersToContext(CompositePurpose = NotForResize);

    void flushAndRenderLayers();
    bool flushPendingLayerChanges();
    void scrollNonCompositedContents(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

private:
    WebKitWebView* m_webView;
    unsigned int m_layerFlushTimerCallbackId;

#if USE(TEXTURE_MAPPER_GL)
    OwnPtr<WebCore::RedirectedXCompositeWindow> m_redirectedWindow;
    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;
    OwnPtr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;
    OwnPtr<WebCore::TextureMapper> m_textureMapper;
    double m_lastFlushTime;
    double m_redrawPendingTime;
    bool m_needsExtraFlush;
    WebCore::TextureMapperFPSCounter m_fpsCounter;

    void layerFlushTimerFired();
    void stopAnyPendingLayerFlush();
    static gboolean layerFlushTimerFiredCallback(AcceleratedCompositingContext*);
    WebCore::GLContext* prepareForRendering();
    void clearEverywhere();
#elif USE(TEXTURE_MAPPER)
    WebCore::TextureMapperLayer* m_rootTextureMapperLayer;
    OwnPtr<WebCore::GraphicsLayer> m_rootGraphicsLayer;
    OwnPtr<WebCore::TextureMapper> m_textureMapper;
#endif

    AcceleratedCompositingContext(WebKitWebView*);
};

} // namespace WebKit

#endif // USE(ACCELERATED_COMPOSITING)
#endif // AcceleratedCompositingContext_h
