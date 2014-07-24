/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "PlatformWebView.h"

#include <WebCore/GOwnPtrGtk.h>
#include <gtk/gtk.h>
#include <wtf/gobject/GOwnPtr.h>

namespace TestWebKitAPI {

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_view = WKViewCreate(contextRef, pageGroupRef);
    gtk_container_add(GTK_CONTAINER(m_window), GTK_WIDGET(m_view));
    gtk_widget_show(GTK_WIDGET(m_view));
    gtk_widget_show(m_window);
}

PlatformWebView::~PlatformWebView()
{
    gtk_widget_destroy(m_window);
}

WKPageRef PlatformWebView::page() const
{
    return WKViewGetPage(m_view);
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    gtk_window_resize(GTK_WINDOW(m_window), width, height);
}

static void doKeyStroke(GtkWidget* viewWidget, unsigned int keyVal)
{
    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_KEY_PRESS));
    event->key.keyval = keyVal;
    event->key.time = GDK_CURRENT_TIME;
    event->key.state = 0;
    event->key.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->key.window);
    gdk_event_set_device(event.get(), gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget))));

    // When synthesizing an event, an invalid hardware_keycode value can cause it to be badly processed by GTK+.
    GOwnPtr<GdkKeymapKey> keys;
    int keysCount;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keyVal, &keys.outPtr(), &keysCount))
        event->key.hardware_keycode = keys.get()[0].keycode;

    gtk_main_do_event(event.get());
    event->key.type = GDK_KEY_RELEASE;
    gtk_main_do_event(event.get());
}

void PlatformWebView::simulateSpacebarKeyPress()
{
    GtkWidget* viewWidget = GTK_WIDGET(m_view);
    if (!gtk_widget_get_realized(viewWidget))
        gtk_widget_show(m_window);
    doKeyStroke(viewWidget, GDK_KEY_KP_Space);
}

void PlatformWebView::simulateAltKeyPress()
{
    GtkWidget* viewWidget = GTK_WIDGET(m_view);
    if (!gtk_widget_get_realized(viewWidget))
        gtk_widget_show(m_window);
    doKeyStroke(viewWidget, GDK_KEY_Alt_L);
}

static void doMouseButtonEvent(GtkWidget* viewWidget, GdkEventType eventType, int x, int y, unsigned int button)
{
    GOwnPtr<GdkEvent> event(gdk_event_new(eventType));
    event->button.x = x;
    event->button.y = y;
    event->button.button = button;
    event->button.time = GDK_CURRENT_TIME;
    event->button.axes = 0;
    event->button.state = 0;
    event->button.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->button.window);
    event->button.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget)));

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(viewWidget), x, y, &xRoot, &yRoot);
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;
    gtk_main_do_event(event.get());
}

void PlatformWebView::simulateRightClick(unsigned x, unsigned y)
{
    GtkWidget* viewWidget = GTK_WIDGET(m_view);
    if (!gtk_widget_get_realized(viewWidget))
        gtk_widget_show(m_window);
    doMouseButtonEvent(viewWidget, GDK_BUTTON_PRESS, x, y, 3);
    doMouseButtonEvent(viewWidget, GDK_BUTTON_RELEASE, x, y, 3);
}

void PlatformWebView::simulateMouseMove(unsigned x, unsigned y)
{
    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_MOTION_NOTIFY));
    event->motion.x = x;
    event->motion.y = y;
    event->motion.time = GDK_CURRENT_TIME;
    event->motion.state = 0;
    event->motion.axes = 0;

    GtkWidget* viewWidget = GTK_WIDGET(m_view);
    if (!gtk_widget_get_realized(viewWidget))
        gtk_widget_show(m_window);
    event->motion.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->motion.window);
    event->motion.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget)));

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(viewWidget), x, y, &xRoot, &yRoot);
    event->motion.x_root = xRoot;
    event->motion.y_root = yRoot;
    gtk_main_do_event(event.get());
}

} // namespace TestWebKitAPI
