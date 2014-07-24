/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
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
#include "PlatformWheelEvent.h"
#include "Scrollbar.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

// Keep this in sync with the other platform event constructors
PlatformWheelEvent::PlatformWheelEvent(GdkEventScroll* event)
{
    static const float delta = 1;

    m_type = PlatformEvent::Wheel;
    m_timestamp = currentTime();

    m_modifiers = 0;
    if (event->state & GDK_SHIFT_MASK)
        m_modifiers |= ShiftKey;
    if (event->state & GDK_CONTROL_MASK)
        m_modifiers |= CtrlKey;
    if (event->state & GDK_MOD1_MASK)
        m_modifiers |= AltKey;
    if (event->state & GDK_META_MASK)
        m_modifiers |= MetaKey;

    m_deltaX = 0;
    m_deltaY = 0;

    // Docs say an upwards scroll (away from the user) has a positive delta
    switch (event->direction) {
        case GDK_SCROLL_UP:
            m_deltaY = delta;
            break;
        case GDK_SCROLL_DOWN:
            m_deltaY = -delta;
            break;
        case GDK_SCROLL_LEFT:
            m_deltaX = delta;
            break;
        case GDK_SCROLL_RIGHT:
            m_deltaX = -delta;
            break;
#if GTK_CHECK_VERSION(3, 3, 18)
        case GDK_SCROLL_SMOOTH: {
                gdouble deltaX, deltaY;
                gdk_event_get_scroll_deltas(reinterpret_cast<GdkEvent*>(event), &deltaX, &deltaY);
                m_deltaX = -deltaX;
                m_deltaY = -deltaY;
            }
            break;
#endif
    }
    m_wheelTicksX = m_deltaX;
    m_wheelTicksY = m_deltaY;

    m_position = IntPoint(static_cast<int>(event->x), static_cast<int>(event->y));
    m_globalPosition = IntPoint(static_cast<int>(event->x_root), static_cast<int>(event->y_root));
    m_granularity = ScrollByPixelWheelEvent;
    m_directionInvertedFromDevice = false;

    // FIXME: retrieve the user setting for the number of lines to scroll on each wheel event
    m_deltaX *= static_cast<float>(Scrollbar::pixelsPerLineStep());
    m_deltaY *= static_cast<float>(Scrollbar::pixelsPerLineStep());
}

}
