/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WKCACFViewLayerTreeHost.h"

#if USE(ACCELERATED_COMPOSITING)

#include "PlatformCALayer.h"
#include "SoftLinking.h"
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

typedef struct _CACFLayer* CACFLayerRef;

namespace WebCore {

#ifdef DEBUG_ALL
SOFT_LINK_DEBUG_LIBRARY(WebKitQuartzCoreAdditions)
#else
SOFT_LINK_LIBRARY(WebKitQuartzCoreAdditions)
#endif

enum WKCACFViewDrawingDestination {
    kWKCACFViewDrawingDestinationWindow = 0,
    kWKCACFViewDrawingDestinationImage,
};
typedef enum WKCACFViewDrawingDestination WKCACFViewDrawingDestination;

SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewCreate, WKCACFViewRef, __cdecl, (WKCACFViewDrawingDestination destination), (destination))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewSetLayer, void, __cdecl, (WKCACFViewRef view, CACFLayerRef layer), (view, layer))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewUpdate, void, __cdecl, (WKCACFViewRef view, HWND window, const CGRect* bounds), (view, window, bounds))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewCanDraw, bool, __cdecl, (WKCACFViewRef view), (view))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewDraw, void, __cdecl, (WKCACFViewRef view), (view))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewFlushContext, void, __cdecl, (WKCACFViewRef view), (view))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewInvalidateRects, void, __cdecl, (WKCACFViewRef view, const CGRect rects[], size_t count), (view, rects, count))
typedef void (*WKCACFViewContextDidChangeCallback)(WKCACFViewRef view, void* info);
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewSetContextDidChangeCallback, void, __cdecl, (WKCACFViewRef view, WKCACFViewContextDidChangeCallback callback, void* info), (view, callback, info))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewGetLastCommitTime, CFTimeInterval, __cdecl, (WKCACFViewRef view), (view))
SOFT_LINK(WebKitQuartzCoreAdditions, WKCACFViewSetContextUserData, void, __cdecl, (WKCACFViewRef view, void* userData), (view, userData))
SOFT_LINK_OPTIONAL(WebKitQuartzCoreAdditions, WKCACFViewSetShouldInvertColors, void, _cdecl, (WKCACFViewRef view, bool shouldInvertColors))
SOFT_LINK_OPTIONAL(WebKitQuartzCoreAdditions, WKCACFViewGetD3DDevice9, IDirect3DDevice9*, _cdecl, (WKCACFViewRef view))

PassRefPtr<WKCACFViewLayerTreeHost> WKCACFViewLayerTreeHost::create()
{
    if (!WebKitQuartzCoreAdditionsLibrary())
        return 0;

    return adoptRef(new WKCACFViewLayerTreeHost);
}

WKCACFViewLayerTreeHost::WKCACFViewLayerTreeHost()
    : m_view(adoptCF(WKCACFViewCreate(kWKCACFViewDrawingDestinationWindow)))
    , m_viewNeedsUpdate(true)
{
}

void WKCACFViewLayerTreeHost::updateViewIfNeeded()
{
    if (!m_viewNeedsUpdate)
        return;
    m_viewNeedsUpdate = false;

    CGRect layerBounds = rootLayer()->bounds();

    CGRect bounds = this->bounds();
    WKCACFViewUpdate(m_view.get(), window(), &bounds);

    if (CGRectEqualToRect(layerBounds, rootLayer()->bounds()))
        return;

    // Flush the context so the layer's rendered bounds will match our bounds.
    flushContext();
}

void WKCACFViewLayerTreeHost::contextDidChangeCallback(WKCACFViewRef view, void* info)
{
    ASSERT_ARG(view, view);
    ASSERT_ARG(info, info);

    WKCACFViewLayerTreeHost* host = static_cast<WKCACFViewLayerTreeHost*>(info);
    ASSERT_ARG(view, view == host->m_view);
    host->contextDidChange();
}

void WKCACFViewLayerTreeHost::contextDidChange()
{
    // This should only be called on a background thread when no changes have actually 
    // been committed to the context, eg. when a video frame has been added to an image
    // queue, so return without triggering animations etc.
    if (!isMainThread())
        return;

    // Tell the WKCACFView to start rendering now that we have some contents to render.
    updateViewIfNeeded();

    CACFLayerTreeHost::contextDidChange();
}

void WKCACFViewLayerTreeHost::initializeContext(void* userData, PlatformCALayer* layer)
{
    WKCACFViewSetContextUserData(m_view.get(), userData);
    WKCACFViewSetLayer(m_view.get(), layer->platformLayer());
    WKCACFViewSetContextDidChangeCallback(m_view.get(), contextDidChangeCallback, this);
}

void WKCACFViewLayerTreeHost::resize()
{
    m_viewNeedsUpdate = true;
}

bool WKCACFViewLayerTreeHost::createRenderer()
{
    updateViewIfNeeded();
    return WKCACFViewCanDraw(m_view.get());
}

void WKCACFViewLayerTreeHost::destroyRenderer()
{
    m_viewNeedsUpdate = true;
    WKCACFViewUpdate(m_view.get(), 0, 0);
    WKCACFViewSetContextUserData(m_view.get(), 0);
    WKCACFViewSetLayer(m_view.get(), 0);
    WKCACFViewSetContextDidChangeCallback(m_view.get(), 0, 0);

    CACFLayerTreeHost::destroyRenderer();
}

CFTimeInterval WKCACFViewLayerTreeHost::lastCommitTime() const
{
    return WKCACFViewGetLastCommitTime(m_view.get());
}

void WKCACFViewLayerTreeHost::flushContext()
{
    WKCACFViewFlushContext(m_view.get());
}

void WKCACFViewLayerTreeHost::paint()
{
    updateViewIfNeeded();
    CACFLayerTreeHost::paint();
}

void WKCACFViewLayerTreeHost::render(const Vector<CGRect>& dirtyRects)
{
    WKCACFViewInvalidateRects(m_view.get(), dirtyRects.data(), dirtyRects.size());
    WKCACFViewDraw(m_view.get());
}

void WKCACFViewLayerTreeHost::setShouldInvertColors(bool shouldInvertColors)
{
    if (WKCACFViewSetShouldInvertColorsPtr())
        WKCACFViewSetShouldInvertColorsPtr()(m_view.get(), shouldInvertColors);
}

#if USE(AVFOUNDATION)
GraphicsDeviceAdapter* WKCACFViewLayerTreeHost::graphicsDeviceAdapter() const
{
    if (!WKCACFViewGetD3DDevice9Ptr())
        return 0;

    return reinterpret_cast<GraphicsDeviceAdapter*>(WKCACFViewGetD3DDevice9Ptr()(m_view.get()));
}
#endif

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
