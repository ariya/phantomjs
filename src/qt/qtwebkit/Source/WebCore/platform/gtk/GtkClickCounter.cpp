/*
 * Copyright (C) 2010, 2011 Igalia S.L.
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
#include "GtkClickCounter.h"

#include "GOwnPtrGtk.h"
#include <gtk/gtk.h>

namespace WebCore {

GtkClickCounter::GtkClickCounter()
    : m_currentClickCount(0)
    , m_previousClickButton(0)
    , m_previousClickTime(0)
{
}

void GtkClickCounter::reset()
{
    m_currentClickCount = 0;
    m_previousClickPoint = IntPoint();
    m_previousClickTime = 0;
    m_previousClickButton = 0;
}

bool GtkClickCounter::shouldProcessButtonEvent(GdkEventButton* buttonEvent)
{
    // For double and triple clicks GDK sends both a normal button press event
    // and a specific type (like GDK_2BUTTON_PRESS). If we detect a special press
    // coming up, ignore this event as it certainly generated the double or triple
    // click. The consequence of not eating this event is two DOM button press events
    // are generated.
    GOwnPtr<GdkEvent> nextEvent(gdk_event_peek());
    if (!nextEvent)
        return true;
    if (nextEvent->any.type == GDK_2BUTTON_PRESS || nextEvent->any.type == GDK_3BUTTON_PRESS)
        return false;
    return true;
}

static guint32 getEventTime(GdkEvent* event)
{
    guint32 time = gdk_event_get_time(event);
    if (time)
        return time;

    // Real events always have a non-zero time, but events synthesized
    // by the DRT do not and we must calculate a time manually. This time
    // is not calculated in the DRT, because GTK+ does not work well with
    // anything other than GDK_CURRENT_TIME on synthesized events.
    GTimeVal timeValue;
    g_get_current_time(&timeValue);
    return (timeValue.tv_sec * 1000) + (timeValue.tv_usec / 1000);
}

int GtkClickCounter::clickCountForGdkButtonEvent(GtkWidget* widget, GdkEventButton* buttonEvent)
{
    gint doubleClickDistance = 250;
    gint doubleClickTime = 5;
    GtkSettings* settings = gtk_settings_get_for_screen(gtk_widget_get_screen(widget));
    g_object_get(settings,
                 "gtk-double-click-distance", &doubleClickDistance,
                 "gtk-double-click-time", &doubleClickTime, NULL);

    // GTK+ only counts up to triple clicks, but WebCore wants to know about
    // quadruple clicks, quintuple clicks, ad infinitum. Here, we replicate the
    // GDK logic for counting clicks.
    GdkEvent* event(reinterpret_cast<GdkEvent*>(buttonEvent));
    guint32 eventTime = getEventTime(event);

    if ((event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
        || ((abs(buttonEvent->x - m_previousClickPoint.x()) < doubleClickDistance)
            && (abs(buttonEvent->y - m_previousClickPoint.y()) < doubleClickDistance)
            && (eventTime - m_previousClickTime < static_cast<guint>(doubleClickTime))
            && (buttonEvent->button == m_previousClickButton)))
        m_currentClickCount++;
    else
        m_currentClickCount = 1;

    gdouble x, y;
    gdk_event_get_coords(event, &x, &y);
    m_previousClickPoint = IntPoint(x, y);
    m_previousClickButton = buttonEvent->button;
    m_previousClickTime = eventTime;

    return m_currentClickCount;
}

} // namespace WebCore
