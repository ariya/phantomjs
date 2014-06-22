/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "config.h"
#include "ewk_view_private.h"
#include "PlatformWebView.h"

#include "EWebKit2.h"
#include <WebKit2/WKAPICast.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKViewEfl.h>
#include <Ecore_Evas.h>

extern bool useX11Window;

using namespace WebKit;

namespace TestWebKitAPI {

static Ecore_Evas* initEcoreEvas()
{
    if (!ecore_evas_init())
        return 0;

    Ecore_Evas* ecoreEvas;
#if defined(WTF_USE_ACCELERATED_COMPOSITING) && defined(HAVE_ECORE_X)
    ecoreEvas = ecore_evas_new("opengl_x11", 0, 0, 800, 600, 0);
    // Graceful fallback to software rendering if evas_gl engine is not available.
    if (!ecoreEvas)
#endif
    ecoreEvas = ecore_evas_new(0, 0, 0, 800, 600, 0);

    ASSERT(ecoreEvas);

    ecore_evas_show(ecoreEvas);

    return ecoreEvas;
}

static void onWebProcessCrashed(void*, Evas_Object*, void* eventInfo)
{
    bool* handled = static_cast<bool*>(eventInfo);
    *handled = true;
}

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    m_window = initEcoreEvas();

    m_view = EWKViewCreate(contextRef, pageGroupRef, ecore_evas_get(m_window), /* smart */ 0);

    WKRetainPtr<WKStringRef> wkTheme = adoptWK(WKStringCreateWithUTF8CString(TEST_THEME_DIR "/default.edj"));
    WKViewSetThemePath(EWKViewGetWKView(m_view), wkTheme.get());

    evas_object_smart_callback_add(m_view, "webprocess,crashed", onWebProcessCrashed, 0);
    resizeTo(600, 800);
}

PlatformWebView::~PlatformWebView()
{
    evas_object_del(m_view);

    ecore_evas_free(m_window);
    ecore_evas_shutdown();
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    evas_object_resize(m_view, width, height);
}

WKPageRef PlatformWebView::page() const
{
    return WKViewGetPage(EWKViewGetWKView(m_view));
}

void PlatformWebView::simulateSpacebarKeyPress()
{
    Evas* evas = evas_object_evas_get(m_view);
    evas_object_focus_set(m_view, true);
    evas_event_feed_key_down(evas, "space", "space", " ", 0, 0, 0);
    evas_event_feed_key_up(evas, "space", "space", " ", 0, 1, 0);
}

void PlatformWebView::simulateMouseMove(unsigned x, unsigned y)
{
    Evas* evas = evas_object_evas_get(m_view);
    evas_object_show(m_view);
    evas_event_feed_mouse_move(evas, x, y, 0, 0);
}

void PlatformWebView::simulateRightClick(unsigned x, unsigned y)
{
    Evas* evas = evas_object_evas_get(m_view);
    evas_object_show(m_view);
    evas_event_feed_mouse_move(evas, x, y, 0, 0);
    evas_event_feed_mouse_down(evas, 3, EVAS_BUTTON_NONE, 0, 0);
    evas_event_feed_mouse_up(evas, 3, EVAS_BUTTON_NONE, 0, 0);
}

} // namespace TestWebKitAPI
