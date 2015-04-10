/*
 * Copyright (C) 2012 Samsung Electronics
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
#include "WebInspectorProxy.h"

#if ENABLE(INSPECTOR)

#include "EwkView.h"
#include "WebProcessProxy.h"
#include "ewk_context_private.h"
#include "ewk_page_group_private.h"
#include "ewk_settings.h"
#include "ewk_view.h"
#include "ewk_view_private.h"
#include <WebCore/EflInspectorUtilities.h>
#include <WebCore/NotImplemented.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKViewEfl.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

static void resizeInspectorWindow(Ecore_Evas* inspectorWindow)
{
    Evas_Object* inspectorView = evas_object_name_find(ecore_evas_get(inspectorWindow), "inspector");
    if (!inspectorView)
        return;

    int width, height;
    ecore_evas_geometry_get(inspectorWindow, 0, 0, &width, &height);

    evas_object_move(inspectorView, 0, 0);
    evas_object_resize(inspectorView, width, height);
}

static void destroyInspectorWindow(Ecore_Evas* inspectorWindow)
{
    Evas_Object* inspectorView = evas_object_name_find(ecore_evas_get(inspectorWindow), "inspector");
    if (inspectorView)
        evas_object_smart_callback_call(inspectorView, "inspector,view,close", 0);
}

static void closeInspectorWindow(void* userData, Evas_Object*, void*)
{
    WebInspectorProxy* inspectorProxy = static_cast<WebInspectorProxy*>(userData);

    inspectorProxy->close();
}

void WebInspectorProxy::createInspectorWindow()
{
    ecore_evas_title_set(m_inspectorWindow, "Web Inspector");
    ecore_evas_callback_resize_set(m_inspectorWindow, resizeInspectorWindow);
    ecore_evas_callback_delete_request_set(m_inspectorWindow, destroyInspectorWindow);
    ecore_evas_show(m_inspectorWindow);

    evas_object_name_set(m_inspectorView, "inspector");
    evas_object_move(m_inspectorView, 0, 0);
    evas_object_resize(m_inspectorView, initialWindowWidth, initialWindowHeight);
    evas_object_show(m_inspectorView);

    evas_object_focus_set(m_inspectorView, true);
}

WebPageProxy* WebInspectorProxy::platformCreateInspectorPage()
{
    ASSERT(m_page);

#ifdef HAVE_ECORE_X
    const char* engine = "opengl_x11";
    m_inspectorWindow = ecore_evas_new(engine, 0, 0, initialWindowWidth, initialWindowHeight, 0);

    // Gracefully fall back to software if evas_gl engine is not available.
    if (!m_inspectorWindow)
#endif
    m_inspectorWindow = ecore_evas_new(0, 0, 0, initialWindowWidth, initialWindowHeight, 0);
    if (!m_inspectorWindow)
        return 0;

    WKContextRef wkContext = toAPI(page()->process()->context());
    WKPageGroupRef wkPageGroup = toAPI(inspectorPageGroup());

    m_inspectorView = EWKViewCreate(wkContext, wkPageGroup, ecore_evas_get(m_inspectorWindow), /* smart */ 0);
    WKViewRef wkView = EWKViewGetWKView(m_inspectorView);

    WKRetainPtr<WKStringRef> wkTheme = adoptWK(WKStringCreateWithUTF8CString(TEST_THEME_DIR "/default.edj"));
    WKViewSetThemePath(wkView, wkTheme.get());

    WKPreferencesRef wkPreferences = WKPageGroupGetPreferences(wkPageGroup);
    WKPreferencesSetFileAccessFromFileURLsAllowed(wkPreferences, true);

    return toImpl(WKViewGetPage(wkView));
}

void WebInspectorProxy::platformOpen()
{
    if (m_isAttached)
        platformAttach();
    else
        createInspectorWindow();

    evas_object_smart_callback_add(m_inspectorView, "inspector,view,close", closeInspectorWindow, this);
}

void WebInspectorProxy::platformDidClose()
{
    if (m_inspectorView) {
        evas_object_del(m_inspectorView);
        m_inspectorView = 0;
    }

    if (m_inspectorWindow) {
        ecore_evas_free(m_inspectorWindow);
        m_inspectorWindow = 0;
    }
}

void WebInspectorProxy::platformHide()
{
    notImplemented();
}

void WebInspectorProxy::platformBringToFront()
{
    notImplemented();
}

bool WebInspectorProxy::platformIsFront()
{
    notImplemented();
    return false;
}

void WebInspectorProxy::platformInspectedURLChanged(const String& url)
{
    if (!m_inspectorWindow)
        return;

    String title = "WebInspector - " + url;
    ecore_evas_title_set(m_inspectorWindow, title.utf8().data());
}

String WebInspectorProxy::inspectorPageURL() const
{
    StringBuilder builder;
    builder.append(inspectorBaseURL());
    builder.appendLiteral("/inspector.html");

    return builder.toString();
}

String WebInspectorProxy::inspectorBaseURL() const
{
    return "file://" + WebCore::inspectorResourcePath();
}

unsigned WebInspectorProxy::platformInspectedWindowHeight()
{
    notImplemented();
    return 0;
}

unsigned WebInspectorProxy::platformInspectedWindowWidth()
{
    notImplemented();
    return 0;
}

void WebInspectorProxy::platformAttach()
{
    notImplemented();
}

void WebInspectorProxy::platformDetach()
{
    notImplemented();
}

void WebInspectorProxy::platformSetAttachedWindowHeight(unsigned)
{
    notImplemented();
}

void WebInspectorProxy::platformSetAttachedWindowWidth(unsigned)
{
    notImplemented();
}

void WebInspectorProxy::platformSetToolbarHeight(unsigned)
{
    notImplemented();
}

void WebInspectorProxy::platformSave(const String&, const String&, bool)
{
    notImplemented();
}

void WebInspectorProxy::platformAppend(const String&, const String&)
{
    notImplemented();
}

void WebInspectorProxy::platformAttachAvailabilityChanged(bool)
{
    notImplemented();
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
