/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#include "PlatformScreen.h"

#include "FloatRect.h"
#include "GtkVersioning.h"
#include "HostWindow.h"
#include "NotImplemented.h"
#include "ScrollView.h"
#include "Widget.h"

#include <gtk/gtk.h>

namespace WebCore {

static GtkWidget* getToplevel(GtkWidget* widget)
{
    GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
    return gtk_widget_is_toplevel(toplevel) ? toplevel : 0;
}

static GdkVisual* getVisual(Widget* widget)
{
    GtkWidget* container = widget ? GTK_WIDGET(widget->root()->hostWindow()->platformPageClient()) : 0;
    if (!container) {
        GdkScreen* screen = gdk_screen_get_default();
        return screen ? gdk_screen_get_system_visual(screen) : 0;
    }

    if (!gtk_widget_get_realized(container))
        container = getToplevel(container);
    return container ? gdk_window_get_visual(gtk_widget_get_window(container)) : 0;
}

int screenDepth(Widget* widget)
{
    GdkVisual* visual = getVisual(widget);
    if (!visual)
        return 24;
    return gdk_visual_get_depth(visual);
}

int screenDepthPerComponent(Widget* widget)
{
    GdkVisual* visual = getVisual(widget);
    if (!visual)
        return 8;

    return gdk_visual_get_bits_per_rgb(visual);
}

bool screenIsMonochrome(Widget* widget)
{
    return screenDepth(widget) < 2;
}


static GdkScreen* getScreen(GtkWidget* widget)
{
    return gtk_widget_has_screen(widget) ? gtk_widget_get_screen(widget) : gdk_screen_get_default();
}

FloatRect screenRect(Widget* widget)
{
    GtkWidget* container = widget ? GTK_WIDGET(widget->root()->hostWindow()->platformPageClient()) : 0;
    if (container)
        container = getToplevel(container);

    GdkScreen* screen = container ? getScreen(container) : gdk_screen_get_default();
    if (!screen)
        return FloatRect();

    gint monitor = container ? gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(container)) : 0;

    GdkRectangle geometry;
    gdk_screen_get_monitor_geometry(screen, monitor, &geometry);

    return FloatRect(geometry.x, geometry.y, geometry.width, geometry.height);
}

FloatRect screenAvailableRect(Widget* widget)
{
    GtkWidget* container = widget ? GTK_WIDGET(widget->root()->hostWindow()->platformPageClient()) : 0;
    if (container && !gtk_widget_get_realized(container))
        return screenRect(widget);

    GdkScreen* screen = container ? getScreen(container) : gdk_screen_get_default();
    if (!screen)
        return FloatRect();

    gint monitor = container ? gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(container)) : 0;

    GdkRectangle workArea;
    gdk_screen_get_monitor_workarea(screen, monitor, &workArea);

    return FloatRect(workArea.x, workArea.y, workArea.width, workArea.height);

}

void screenColorProfile(ColorProfile&)
{
    notImplemented();
}

} // namespace WebCore
