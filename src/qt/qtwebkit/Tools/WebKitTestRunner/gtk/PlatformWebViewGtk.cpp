/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged. All rights reserved.
 * Copyright (C) 2010 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformWebView.h"

#include <WebKit2/WKViewPrivate.h>
#include <gtk/gtk.h>

namespace WTR {

PlatformWebView::PlatformWebView(WKContextRef context, WKPageGroupRef pageGroup, WKPageRef /* relatedPage */, WKDictionaryRef options)
    : m_view(WKViewCreate(context, pageGroup))
    , m_window(gtk_window_new(GTK_WINDOW_POPUP))
    , m_windowIsKey(true)
    , m_options(options)
{
    gtk_container_add(GTK_CONTAINER(m_window), GTK_WIDGET(m_view));

    GtkAllocation size = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(m_window, &size);
    gtk_window_resize(GTK_WINDOW(m_window), 800, 600);
    gtk_widget_show_all(m_window);

    while (gtk_events_pending())
        gtk_main_iteration();
}

PlatformWebView::~PlatformWebView()
{
    gtk_widget_destroy(m_window);
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    GtkAllocation size = { 0, 0, static_cast<int>(width), static_cast<int>(height) };
    gtk_widget_size_allocate(m_window, &size);
    gtk_window_resize(GTK_WINDOW(m_window), width, height);

    while (gtk_events_pending())
        gtk_main_iteration();
}

WKPageRef PlatformWebView::page()
{
    return WKViewGetPage(m_view);
}

void PlatformWebView::focus()
{
    WKViewSetFocus(m_view, true);
    setWindowIsKey(true);
}

WKRect PlatformWebView::windowFrame()
{
    GtkAllocation geometry;
#ifdef GTK_API_VERSION_2
    gint depth;
    gdk_window_get_geometry(gtk_widget_get_window(GTK_WIDGET(m_window)),
                            &geometry.x, &geometry.y, &geometry.width, &geometry.height, &depth);
#else
    gdk_window_get_geometry(gtk_widget_get_window(GTK_WIDGET(m_window)),
                            &geometry.x, &geometry.y, &geometry.width, &geometry.height);
#endif

    WKRect frame;
    frame.origin.x = geometry.x;
    frame.origin.y = geometry.y;
    frame.size.width = geometry.width;
    frame.size.height = geometry.height;
    return frame;
}

void PlatformWebView::setWindowFrame(WKRect frame)
{
    gtk_window_move(GTK_WINDOW(m_window), frame.origin.x, frame.origin.y);
    resizeTo(frame.size.width, frame.size.height);
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
    // FIXME: implement to capture pixels in the UI process,
    // which may be necessary to capture things like 3D transforms.
    return 0;
}

void PlatformWebView::didInitializeClients()
{
}

} // namespace WTR

