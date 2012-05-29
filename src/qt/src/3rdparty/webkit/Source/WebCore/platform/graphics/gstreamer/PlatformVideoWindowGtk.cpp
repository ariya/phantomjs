/*
 * Copyright (C) 2010 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "PlatformVideoWindow.h"
#if USE(GSTREAMER)

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h> // for GDK_WINDOW_XID
#endif

using namespace WebCore;

PlatformVideoWindow::PlatformVideoWindow()
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_events(m_window, GDK_POINTER_MOTION_MASK | GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK);

    m_videoWindow = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(m_videoWindow, FALSE);
    gtk_container_add(GTK_CONTAINER(m_window), m_videoWindow);

    gtk_widget_realize(m_window);

#ifdef GDK_WINDOWING_X11
    m_videoWindowId = GDK_WINDOW_XID(gtk_widget_get_window(m_window));
#endif

}

PlatformVideoWindow::~PlatformVideoWindow()
{
    if (m_videoWindow && m_window) {
        gtk_container_remove(GTK_CONTAINER(m_window), m_videoWindow);
        gtk_widget_destroy(m_videoWindow);
        m_videoWindow = 0;
    }

    if (m_window) {
        gtk_widget_destroy(m_window);
        m_window = 0;
    }

    m_videoWindowId = 0;
}

void PlatformVideoWindow::prepareForOverlay(GstMessage*)
{
}
#endif // USE(GSTREAMER)

