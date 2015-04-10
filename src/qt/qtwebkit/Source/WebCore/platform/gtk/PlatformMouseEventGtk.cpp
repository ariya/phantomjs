/*
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
#include "PlatformMouseEvent.h"

#include <wtf/Assertions.h>

#include <gdk/gdk.h>

namespace WebCore {

// FIXME: Would be even better to figure out which modifier is Alt instead of always using GDK_MOD1_MASK.

// Keep this in sync with the other platform event constructors
PlatformMouseEvent::PlatformMouseEvent(GdkEventButton* event)
{
    m_timestamp = event->time;
    m_position = IntPoint((int)event->x, (int)event->y);
    m_globalPosition = IntPoint((int)event->x_root, (int)event->y_root);

    m_modifiers = 0;
    if (event->state & GDK_SHIFT_MASK)
        m_modifiers |= ShiftKey;
    if (event->state & GDK_CONTROL_MASK)
        m_modifiers |= CtrlKey;
    if (event->state & GDK_MOD1_MASK)
        m_modifiers |= AltKey;
    if (event->state & GDK_META_MASK)
        m_modifiers |= MetaKey;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        m_type = PlatformEvent::MousePressed;
        if (event->type == GDK_BUTTON_RELEASE) {
            m_type = PlatformEvent::MouseReleased;
            m_clickCount = 0;
        } else if (event->type == GDK_BUTTON_PRESS)
            m_clickCount = 1;
        else if (event->type == GDK_2BUTTON_PRESS)
            m_clickCount = 2;
        else if (event->type == GDK_3BUTTON_PRESS)
            m_clickCount = 3;

        if (event->button == 1)
            m_button = LeftButton;
        else if (event->button == 2)
            m_button = MiddleButton;
        else if (event->button == 3)
            m_button = RightButton;
        break;

    default:
        ASSERT_NOT_REACHED();
    };
}

PlatformMouseEvent::PlatformMouseEvent(GdkEventMotion* motion)
{
    m_timestamp = motion->time;
    m_position = IntPoint((int)motion->x, (int)motion->y);
    m_globalPosition = IntPoint((int)motion->x_root, (int)motion->y_root);

    m_modifiers = 0;
    if (motion->state & GDK_SHIFT_MASK)
        m_modifiers |= ShiftKey;
    if (motion->state & GDK_CONTROL_MASK)
        m_modifiers |= CtrlKey;
    if (motion->state & GDK_MOD1_MASK)
        m_modifiers |= AltKey;
    if (motion->state & GDK_META_MASK)
        m_modifiers |= MetaKey;

    switch (motion->type) {
    case GDK_MOTION_NOTIFY:
        m_type = PlatformEvent::MouseMoved;
        m_button = NoButton;
        m_clickCount = 0;
        break;
    default:
        ASSERT_NOT_REACHED();
    };

    if (motion->state & GDK_BUTTON1_MASK)
        m_button = LeftButton;
    else if (motion->state & GDK_BUTTON2_MASK)
        m_button = MiddleButton;
    else if (motion->state & GDK_BUTTON3_MASK)
        m_button = RightButton;
}
}
