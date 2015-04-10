/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ewk_view_private.h"
#include "PlatformWebView.h"

#include "EWebKit2.h"
#include "WebKit2/WKAPICast.h"
#include <Ecore_Evas.h>
#include <WebCore/RefPtrCairo.h>
#include <WebKit2/WKImageCairo.h>
#include <WebKit2/WKViewEfl.h>
#include <cairo.h>

using namespace WebKit;

namespace WTR {

static Ecore_Evas* initEcoreEvas()
{
    Ecore_Evas* ecoreEvas = 0;
#if defined(WTF_USE_ACCELERATED_COMPOSITING) && defined(HAVE_ECORE_X)
    const char* engine = "opengl_x11";
    ecoreEvas = ecore_evas_new(engine, 0, 0, 800, 600, 0);
    // Graceful fallback to software rendering if evas_gl engine is not available.
    if (!ecoreEvas)
#endif
    ecoreEvas = ecore_evas_new(0, 0, 0, 800, 600, 0);

    if (!ecoreEvas)
        return 0;

    ecore_evas_title_set(ecoreEvas, "EFL WebKitTestRunner");
    ecore_evas_show(ecoreEvas);

    return ecoreEvas;
}

PlatformWebView::PlatformWebView(WKContextRef context, WKPageGroupRef pageGroup, WKPageRef /* relatedPage */, WKDictionaryRef options)
    : m_options(options)
{
    WKRetainPtr<WKStringRef> useFixedLayoutKey(AdoptWK, WKStringCreateWithUTF8CString("UseFixedLayout"));
    m_usingFixedLayout = options ? WKBooleanGetValue(static_cast<WKBooleanRef>(WKDictionaryGetItemForKey(options, useFixedLayoutKey.get()))) : false;

    m_window = initEcoreEvas();

    m_view = EWKViewCreate(context, pageGroup, ecore_evas_get(m_window), /* smart */ 0);

    WKPageSetUseFixedLayout(WKViewGetPage(EWKViewGetWKView(m_view)), m_usingFixedLayout);

    if (m_usingFixedLayout)
        resizeTo(800, 600);

    ewk_view_theme_set(m_view, TEST_THEME_DIR "/default.edj");
    m_windowIsKey = false;
    evas_object_show(m_view);
}

PlatformWebView::~PlatformWebView()
{
    evas_object_del(m_view);

    ecore_evas_free(m_window);
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    evas_object_resize(m_view, width, height);
}

WKPageRef PlatformWebView::page()
{
    return WKViewGetPage(EWKViewGetWKView(m_view));
}

void PlatformWebView::focus()
{
    // In a few cases, an iframe might receive focus from JavaScript and Evas is not aware of it at all
    // (WebCoreSupport::focusedFrameChanged() does not emit any notification). We then manually remove the
    // focus from the view to make the call give focus to evas_object_focus_set(..., true) to be effectful.
    if (WKPageGetFocusedFrame(page()) != WKPageGetMainFrame(page()))
        evas_object_focus_set(m_view, false);
    evas_object_focus_set(m_view, true);
}

WKRect PlatformWebView::windowFrame()
{
    int x, y, width, height;

    ecore_evas_request_geometry_get(m_window, &x, &y, &width, &height);

    return WKRectMake(x, y, width, height);
}

void PlatformWebView::setWindowFrame(WKRect frame)
{
    ecore_evas_move_resize(m_window, frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

void PlatformWebView::addChromeInputField()
{
}

void PlatformWebView::removeChromeInputField()
{
}

void PlatformWebView::makeWebViewFirstResponder()
{
}

WKRetainPtr<WKImageRef> PlatformWebView::windowSnapshotImage()
{
    int width;
    int height;
    ecore_evas_geometry_get(m_window, 0, 0, &width, &height);
    ASSERT(width > 0 && height > 0);

    return adoptWK(WKViewCreateSnapshot(EWKViewGetWKView(m_view)));
}

bool PlatformWebView::viewSupportsOptions(WKDictionaryRef options) const
{
    WKRetainPtr<WKStringRef> useFixedLayoutKey(AdoptWK, WKStringCreateWithUTF8CString("UseFixedLayout"));

    return m_usingFixedLayout == (options ? WKBooleanGetValue(static_cast<WKBooleanRef>(WKDictionaryGetItemForKey(options, useFixedLayoutKey.get()))) : false);
}

void PlatformWebView::didInitializeClients()
{
}

} // namespace WTR

