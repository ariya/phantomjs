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

#include "config.h"
#include "CACFLayerTreeHost.h"

#if USE(ACCELERATED_COMPOSITING)

#include "CACFLayerTreeHostClient.h"
#include "DefWndProcWindowClass.h"
#include "LayerChangesFlusher.h"
#include "LegacyCACFLayerTreeHost.h"
#include "PlatformCALayer.h"
#include "WKCACFViewLayerTreeHost.h"
#include "WebCoreInstanceHandle.h"
#include <limits.h>
#include <QuartzCore/CABase.h>
#include <wtf/CurrentTime.h>
#include <wtf/OwnArrayPtr.h>

#ifdef DEBUG_ALL
#pragma comment(lib, "QuartzCore_debug")
#else
#pragma comment(lib, "QuartzCore")
#endif

inline static CGRect winRectToCGRect(RECT rc)
{
    return CGRectMake(rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top));
}

inline static CGRect winRectToCGRect(RECT rc, RECT relativeToRect)
{
    return CGRectMake(rc.left, (relativeToRect.bottom-rc.bottom), (rc.right - rc.left), (rc.bottom - rc.top));
}

namespace WebCore {

bool CACFLayerTreeHost::acceleratedCompositingAvailable()
{
    static bool available;
    static bool tested;

    if (tested)
        return available;

    tested = true;

    // Initialize available to true since this function will be called from a 
    // propagation within createRenderer(). We want to be able to return true 
    // when that happens so that the test can continue.
    available = true;
    
    HMODULE library = LoadLibrary(TEXT("d3d9.dll"));
    if (!library) {
        available = false;
        return available;
    }

    FreeLibrary(library);
#ifdef DEBUG_ALL
    library = LoadLibrary(TEXT("QuartzCore_debug.dll"));
#else
    library = LoadLibrary(TEXT("QuartzCore.dll"));
#endif
    if (!library) {
        available = false;
        return available;
    }

    FreeLibrary(library);

    // Make a dummy HWND.
    HWND testWindow = ::CreateWindow(defWndProcWindowClassName(), L"CoreAnimationTesterWindow", WS_POPUP, -500, -500, 20, 20, 0, 0, 0, 0);

    if (!testWindow) {
        available = false;
        return available;
    }

    RefPtr<CACFLayerTreeHost> host = CACFLayerTreeHost::create();
    host->setWindow(testWindow);
    available = host->createRenderer();
    host->setWindow(0);
    ::DestroyWindow(testWindow);

    return available;
}

PassRefPtr<CACFLayerTreeHost> CACFLayerTreeHost::create()
{
    if (!acceleratedCompositingAvailable())
        return 0;
    RefPtr<CACFLayerTreeHost> host = WKCACFViewLayerTreeHost::create();
    if (!host)
        host = LegacyCACFLayerTreeHost::create();
    host->initialize();
    return host.release();
}

CACFLayerTreeHost::CACFLayerTreeHost()
    : m_client(0)
    , m_rootLayer(PlatformCALayer::create(PlatformCALayer::LayerTypeRootLayer, 0))
    , m_window(0)
    , m_shouldFlushPendingGraphicsLayerChanges(false)
    , m_isFlushingLayerChanges(false)
#if !ASSERT_DISABLED
    , m_state(WindowNotSet)
#endif
{
}

void CACFLayerTreeHost::initialize()
{
    // Point the CACFContext to this
    initializeContext(this, m_rootLayer.get());

    // Under the root layer, we have a clipping layer to clip the content,
    // that contains a scroll layer that we use for scrolling the content.
    // The root layer is the size of the client area of the window.
    // The clipping layer is the size of the WebView client area (window less the scrollbars).
    // The scroll layer is the size of the root child layer.
    // Resizing the window will change the bounds of the rootLayer and the clip layer and will not
    // cause any repositioning.
    // Scrolling will affect only the position of the scroll layer without affecting the bounds.

    m_rootLayer->setName("CACFLayerTreeHost rootLayer");
    m_rootLayer->setAnchorPoint(FloatPoint3D(0, 0, 0));
    m_rootLayer->setGeometryFlipped(true);

#ifndef NDEBUG
    CGColorRef debugColor = CGColorCreateGenericRGB(1, 0, 0, 0.8);
    m_rootLayer->setBackgroundColor(debugColor);
    CGColorRelease(debugColor);
#endif
}

CACFLayerTreeHost::~CACFLayerTreeHost()
{
    ASSERT_WITH_MESSAGE(m_state != WindowSet, "Must call setWindow(0) before destroying CACFLayerTreeHost");
}

void CACFLayerTreeHost::setWindow(HWND window)
{
    if (window == m_window)
        return;

#if !ASSERT_DISABLED
    switch (m_state) {
    case WindowNotSet:
        ASSERT_ARG(window, window);
        ASSERT(!m_window);
        m_state = WindowSet;
        break;
    case WindowSet:
        ASSERT_ARG(window, !window);
        ASSERT(m_window);
        m_state = WindowCleared;
        break;
    case WindowCleared:
        ASSERT_NOT_REACHED();
        break;
    }
#endif

    if (m_window)
        destroyRenderer();

    m_window = window;
}

PlatformCALayer* CACFLayerTreeHost::rootLayer() const
{
    return m_rootLayer.get();
}

void CACFLayerTreeHost::addPendingAnimatedLayer(PassRefPtr<PlatformCALayer> layer)
{
    m_pendingAnimatedLayers.add(layer);
}

void CACFLayerTreeHost::setRootChildLayer(PlatformCALayer* layer)
{
    m_rootLayer->removeAllSublayers();
    m_rootChildLayer = layer;
    if (m_rootChildLayer)
        m_rootLayer->appendSublayer(m_rootChildLayer.get());
}
   
void CACFLayerTreeHost::layerTreeDidChange()
{
    if (m_isFlushingLayerChanges) {
        // The layer tree is changing as a result of flushing GraphicsLayer changes to their
        // underlying PlatformCALayers. We'll flush those changes to the context as part of that
        // process, so there's no need to schedule another flush here.
        return;
    }

    // The layer tree is changing as a result of someone modifying a PlatformCALayer that doesn't
    // have a corresponding GraphicsLayer. Schedule a flush since we won't schedule one through the
    // normal GraphicsLayer mechanisms.
    LayerChangesFlusher::shared().flushPendingLayerChangesSoon(this);
}

void CACFLayerTreeHost::destroyRenderer()
{
    m_rootLayer = 0;
    m_rootChildLayer = 0;
    LayerChangesFlusher::shared().cancelPendingFlush(this);
}

static void getDirtyRects(HWND window, Vector<CGRect>& outRects)
{
    ASSERT_ARG(outRects, outRects.isEmpty());

    RECT clientRect;
    if (!GetClientRect(window, &clientRect))
        return;

    OwnPtr<HRGN> region = adoptPtr(CreateRectRgn(0, 0, 0, 0));
    int regionType = GetUpdateRgn(window, region.get(), false);
    if (regionType != COMPLEXREGION) {
        RECT dirtyRect;
        if (GetUpdateRect(window, &dirtyRect, false))
            outRects.append(winRectToCGRect(dirtyRect, clientRect));
        return;
    }

    DWORD dataSize = GetRegionData(region.get(), 0, 0);
    OwnArrayPtr<unsigned char> regionDataBuffer = adoptArrayPtr(new unsigned char[dataSize]);
    RGNDATA* regionData = reinterpret_cast<RGNDATA*>(regionDataBuffer.get());
    if (!GetRegionData(region.get(), dataSize, regionData))
        return;

    outRects.resize(regionData->rdh.nCount);

    RECT* rect = reinterpret_cast<RECT*>(regionData->Buffer);
    for (size_t i = 0; i < outRects.size(); ++i, ++rect)
        outRects[i] = winRectToCGRect(*rect, clientRect);
}

void CACFLayerTreeHost::paint()
{
    Vector<CGRect> dirtyRects;
    getDirtyRects(m_window, dirtyRects);
    render(dirtyRects);
}

void CACFLayerTreeHost::flushPendingGraphicsLayerChangesSoon()
{
    m_shouldFlushPendingGraphicsLayerChanges = true;
    LayerChangesFlusher::shared().flushPendingLayerChangesSoon(this);
}

void CACFLayerTreeHost::setShouldInvertColors(bool)
{
}

void CACFLayerTreeHost::flushPendingLayerChangesNow()
{
    // Calling out to the client could cause our last reference to go away.
    RefPtr<CACFLayerTreeHost> protector(this);

    m_isFlushingLayerChanges = true;

    // Flush changes stored up in GraphicsLayers to their underlying PlatformCALayers, if
    // requested.
    if (m_client && m_shouldFlushPendingGraphicsLayerChanges) {
        m_shouldFlushPendingGraphicsLayerChanges = false;
        m_client->flushPendingGraphicsLayerChanges();
    }

    // Flush changes stored up in PlatformCALayers to the context so they will be rendered.
    flushContext();

    m_isFlushingLayerChanges = false;
}

void CACFLayerTreeHost::contextDidChange()
{
    // All pending animations will have been started with the flush. Fire the animationStarted calls.
    notifyAnimationsStarted();
}

void CACFLayerTreeHost::notifyAnimationsStarted()
{
    // Send currentTime to the pending animations. This function is called by CACF in a callback
    // which occurs after the drawInContext calls. So currentTime is very close to the time
    // the animations actually start
    double currentTime = WTF::currentTime();

    HashSet<RefPtr<PlatformCALayer> >::iterator end = m_pendingAnimatedLayers.end();
    for (HashSet<RefPtr<PlatformCALayer> >::iterator it = m_pendingAnimatedLayers.begin(); it != end; ++it)
        (*it)->animationStarted(currentTime);

    m_pendingAnimatedLayers.clear();
}

CGRect CACFLayerTreeHost::bounds() const
{
    RECT clientRect;
    GetClientRect(m_window, &clientRect);

    return winRectToCGRect(clientRect);
}

}

#endif // USE(ACCELERATED_COMPOSITING)
