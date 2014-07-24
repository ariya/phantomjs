/*
 * Copyright (C) 2012 Igalia, S.L.
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

#include "config.h"
#include "AcceleratedCompositingContext.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)

#include "CairoUtilities.h"
#include "Chrome.h"
#include "ChromeClientGtk.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsLayerTextureMapper.h"
#include "PlatformContextCairo.h"
#include "Settings.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"
#include "webkitwebviewprivate.h"
#include <wtf/CurrentTime.h>

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

#include <cairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

const double gFramesPerSecond = 60;

// There seems to be a delicate balance between the main loop being flooded
// with motion events (that force flushes) and starving the main loop of events
// with flush callbacks. This delay is entirely empirical.
const double gScheduleDelay = (1.0 / (gFramesPerSecond / 3.0));

using namespace WebCore;

namespace WebKit {

AcceleratedCompositingContext::AcceleratedCompositingContext(WebKitWebView* webView)
    : m_webView(webView)
    , m_layerFlushTimerCallbackId(0)
    , m_lastFlushTime(0)
    , m_redrawPendingTime(0)
    , m_needsExtraFlush(false)
{
}

static IntSize getWebViewSize(WebKitWebView* webView)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(webView), &allocation);
    return IntSize(allocation.width, allocation.height);
}

void redirectedWindowDamagedCallback(void* data)
{
    gtk_widget_queue_draw(GTK_WIDGET(data));
}

void AcceleratedCompositingContext::initialize()
{
    if (m_rootLayer)
        return;

    IntSize pageSize = getWebViewSize(m_webView);
    if (!m_redirectedWindow) {
        if (m_redirectedWindow = RedirectedXCompositeWindow::create(pageSize))
            m_redirectedWindow->setDamageNotifyCallback(redirectedWindowDamagedCallback, m_webView);
    } else
        m_redirectedWindow->resize(pageSize);

    if (!m_redirectedWindow)
        return;

    m_rootLayer = GraphicsLayer::create(0, this);
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(pageSize);

    // The non-composited contents are a child of the root layer.
    m_nonCompositedContentLayer = GraphicsLayer::create(0, this);
    m_nonCompositedContentLayer->setDrawsContent(true);
    m_nonCompositedContentLayer->setContentsOpaque(!m_webView->priv->transparent);
    m_nonCompositedContentLayer->setSize(pageSize);
    if (core(m_webView)->settings()->acceleratedDrawingEnabled())
        m_nonCompositedContentLayer->setAcceleratesDrawing(true);

#ifndef NDEBUG
    m_rootLayer->setName("Root layer");
    m_nonCompositedContentLayer->setName("Non-composited content");
#endif

    m_rootLayer->addChild(m_nonCompositedContentLayer.get());
    m_nonCompositedContentLayer->setNeedsDisplay();

    // The creation of the TextureMapper needs an active OpenGL context.
    GLContext* context = m_redirectedWindow->context();
    context->makeContextCurrent();

    m_textureMapper = TextureMapperGL::create();
    static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
    toTextureMapperLayer(m_rootLayer.get())->setTextureMapper(m_textureMapper.get());

    scheduleLayerFlush();
}

AcceleratedCompositingContext::~AcceleratedCompositingContext()
{
    stopAnyPendingLayerFlush();
}

void AcceleratedCompositingContext::stopAnyPendingLayerFlush()
{
    if (!m_layerFlushTimerCallbackId)
        return;
    g_source_remove(m_layerFlushTimerCallbackId);
    m_layerFlushTimerCallbackId = 0;
}

bool AcceleratedCompositingContext::enabled()
{
    return m_redirectedWindow && m_rootLayer && m_textureMapper;
}

bool AcceleratedCompositingContext::renderLayersToWindow(cairo_t* cr, const IntRect& clipRect)
{
    m_redrawPendingTime = 0;

    if (!enabled())
        return false;

    cairo_surface_t* windowSurface = m_redirectedWindow->cairoSurfaceForWidget(GTK_WIDGET(m_webView));
    if (!windowSurface)
        return true;

    cairo_rectangle(cr, clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
    cairo_set_source_surface(cr, windowSurface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_fill(cr);

    if (!m_layerFlushTimerCallbackId && (toTextureMapperLayer(m_rootLayer.get())->descendantsOrSelfHaveRunningAnimations() || m_needsExtraFlush)) {
        m_needsExtraFlush = false;
        double nextFlush = max((1 / gFramesPerSecond) - (currentTime() - m_lastFlushTime), 0.0);
        m_layerFlushTimerCallbackId = g_timeout_add_full(GDK_PRIORITY_EVENTS, 1000 * nextFlush, reinterpret_cast<GSourceFunc>(layerFlushTimerFiredCallback), this, 0);
    }

    return true;
}

GLContext* AcceleratedCompositingContext::prepareForRendering()
{
    if (!enabled())
        return 0;

    GLContext* context = m_redirectedWindow->context();
    if (!context)
        return 0;

    if (!context->makeContextCurrent())
        return 0;

    return context;
}

void AcceleratedCompositingContext::compositeLayersToContext(CompositePurpose purpose)
{
    GLContext* context = prepareForRendering();
    if (!context)
        return;

    const IntSize& windowSize = m_redirectedWindow->size();
    glViewport(0, 0, windowSize.width(), windowSize.height());

    if (purpose == ForResize) {
        glClearColor(1, 1, 1, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    m_textureMapper->beginPainting();
    toTextureMapperLayer(m_rootLayer.get())->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get());
    m_textureMapper->endPainting();

    context->swapBuffers();
}

void AcceleratedCompositingContext::clearEverywhere()
{
    GLContext* context = prepareForRendering();
    if (!context)
        return;

    const IntSize& windowSize = m_redirectedWindow->size();
    glViewport(0, 0, windowSize.width(), windowSize.height());
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    context->swapBuffers();

    // FIXME: It seems that when using double-buffering (and on some drivers single-buffering)
    // and XComposite window redirection, two swap buffers are required to force the pixmap
    // to update. This isn't a problem during animations, because swapBuffer is continuously
    // called. For non-animation situations we use this terrible hack until we can get to the
    // bottom of the issue.
    if (!toTextureMapperLayer(m_rootLayer.get())->descendantsOrSelfHaveRunningAnimations()) {
        context->swapBuffers();
        context->swapBuffers();
    }
}

void AcceleratedCompositingContext::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    // Clearing everywhere when turning on or off the layer tree prevents us from flashing
    // old content before the first flush.
    clearEverywhere();

    if (!graphicsLayer) {
        stopAnyPendingLayerFlush();

        // Shrink the offscreen window to save memory while accelerated compositing is turned off.
        if (m_redirectedWindow)
            m_redirectedWindow->resize(IntSize(1, 1));
        m_rootLayer = nullptr;
        m_nonCompositedContentLayer = nullptr;
        m_textureMapper = nullptr;
        return;
    }

    // Add the accelerated layer tree hierarchy.
    initialize();
    if (!m_redirectedWindow)
        return;

    m_nonCompositedContentLayer->removeAllChildren();
    m_nonCompositedContentLayer->addChild(graphicsLayer);

    stopAnyPendingLayerFlush();

    // FIXME: Two flushes seem necessary to get the proper rendering in some cases. It's unclear
    // if this is a bug with the RedirectedXComposite window or with this class.
    m_needsExtraFlush = true;
    scheduleLayerFlush();

    m_layerFlushTimerCallbackId = g_timeout_add_full(GDK_PRIORITY_EVENTS, 500, reinterpret_cast<GSourceFunc>(layerFlushTimerFiredCallback), this, 0);
}

void AcceleratedCompositingContext::setNonCompositedContentsNeedDisplay(const IntRect& rect)
{
    if (!m_rootLayer)
        return;
    if (rect.isEmpty()) {
        m_rootLayer->setNeedsDisplay();
        return;
    }
    m_nonCompositedContentLayer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void AcceleratedCompositingContext::resizeRootLayer(const IntSize& newSize)
{
    if (!enabled())
        return;

    if (m_rootLayer->size() == newSize)
        return;

    m_redirectedWindow->resize(newSize);
    m_rootLayer->setSize(newSize);

    // If the newSize exposes new areas of the non-composited content a setNeedsDisplay is needed
    // for those newly exposed areas.
    FloatSize oldSize = m_nonCompositedContentLayer->size();
    m_nonCompositedContentLayer->setSize(newSize);

    if (newSize.width() > oldSize.width()) {
        float height = std::min(static_cast<float>(newSize.height()), oldSize.height());
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(oldSize.width(), 0, newSize.width() - oldSize.width(), height));
    }

    if (newSize.height() > oldSize.height())
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(0, oldSize.height(), newSize.width(), newSize.height() - oldSize.height()));

    m_nonCompositedContentLayer->setNeedsDisplayInRect(IntRect(IntPoint(), newSize));
    compositeLayersToContext(ForResize);
    scheduleLayerFlush();
}

void AcceleratedCompositingContext::scrollNonCompositedContents(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    m_nonCompositedContentLayer->setNeedsDisplayInRect(scrollRect);
    scheduleLayerFlush();
}

gboolean AcceleratedCompositingContext::layerFlushTimerFiredCallback(AcceleratedCompositingContext* context)
{
    context->layerFlushTimerFired();
    return FALSE;
}

void AcceleratedCompositingContext::scheduleLayerFlush()
{
    if (!enabled())
        return;

    if (m_layerFlushTimerCallbackId)
        return;

    // We use a GLib timer because otherwise GTK+ event handling during dragging can
    // starve WebCore timers, which have a lower priority.
    double nextFlush = max(gScheduleDelay - (currentTime() - m_lastFlushTime), 0.0);
    m_layerFlushTimerCallbackId = g_timeout_add_full(GDK_PRIORITY_EVENTS, nextFlush * 1000, reinterpret_cast<GSourceFunc>(layerFlushTimerFiredCallback), this, 0);
}

bool AcceleratedCompositingContext::flushPendingLayerChanges()
{
    m_rootLayer->flushCompositingStateForThisLayerOnly();
    m_nonCompositedContentLayer->flushCompositingStateForThisLayerOnly();
    return core(m_webView)->mainFrame()->view()->flushCompositingStateIncludingSubframes();
}

void AcceleratedCompositingContext::flushAndRenderLayers()
{
    if (!enabled())
        return;

    Frame* frame = core(m_webView)->mainFrame();
    if (!frame || !frame->contentRenderer() || !frame->view())
        return;
    frame->view()->updateLayoutAndStyleIfNeededRecursive();

    if (!enabled())
        return;

    GLContext* context = m_redirectedWindow->context();
    if (context && !context->makeContextCurrent())
        return;

    if (!flushPendingLayerChanges())
        return;

    m_lastFlushTime = currentTime();
    compositeLayersToContext();

    // If it's been a long time since we've actually painted, which means that events might
    // be starving the main loop, we should force a draw now. This seems to prevent display
    // lag on http://2012.beercamp.com.
    if (m_redrawPendingTime && currentTime() - m_redrawPendingTime > gScheduleDelay) {
        gtk_widget_queue_draw(GTK_WIDGET(m_webView));
        gdk_window_process_updates(gtk_widget_get_window(GTK_WIDGET(m_webView)), FALSE);
    } else if (!m_redrawPendingTime)
        m_redrawPendingTime = currentTime();
}

void AcceleratedCompositingContext::layerFlushTimerFired()
{
    m_layerFlushTimerCallbackId = 0;
    flushAndRenderLayers();
}

void AcceleratedCompositingContext::notifyAnimationStarted(const GraphicsLayer*, double time)
{

}
void AcceleratedCompositingContext::notifyFlushRequired(const GraphicsLayer*)
{

}

void AcceleratedCompositingContext::paintContents(const GraphicsLayer*, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect& rectToPaint)
{
    context.save();
    context.clip(rectToPaint);
    core(m_webView)->mainFrame()->view()->paint(&context, rectToPaint);
    context.restore();
}

} // namespace WebKit

#endif // USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)
