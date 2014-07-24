/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2008, 2009 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 *
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

#include <Evas.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

void PlatformMouseEvent::setClickCount(unsigned int flags)
{
    if (flags & EVAS_BUTTON_TRIPLE_CLICK)
        m_clickCount = 3;
    else if (flags & EVAS_BUTTON_DOUBLE_CLICK)
        m_clickCount = 2;
    else
        m_clickCount = 1;
}

PlatformMouseEvent::PlatformMouseEvent(const Evas_Event_Mouse_Down* event, IntPoint position)
    : PlatformEvent(PlatformEvent::MousePressed, evas_key_modifier_is_set(event->modifiers, "Shift"), evas_key_modifier_is_set(event->modifiers, "Control"), evas_key_modifier_is_set(event->modifiers, "Alt"), evas_key_modifier_is_set(event->modifiers, "Meta"), currentTime())
    , m_position(IntPoint(event->canvas.x - position.x(), event->canvas.y - position.y()))
    , m_globalPosition(IntPoint(event->canvas.x, event->canvas.y))
    , m_button(MouseButton(event->button - 1))
{
    setClickCount(event->flags);
}

PlatformMouseEvent::PlatformMouseEvent(const Evas_Event_Mouse_Up* event, IntPoint position)
    : PlatformEvent(PlatformEvent::MouseReleased, evas_key_modifier_is_set(event->modifiers, "Shift"), evas_key_modifier_is_set(event->modifiers, "Control"), evas_key_modifier_is_set(event->modifiers, "Alt"), evas_key_modifier_is_set(event->modifiers, "Meta"), currentTime())
    , m_position(IntPoint(event->canvas.x - position.x(), event->canvas.y - position.y()))
    , m_globalPosition(IntPoint(event->canvas.x, event->canvas.y))
    , m_button(MouseButton(event->button - 1))
{
    setClickCount(event->flags);
}

PlatformMouseEvent::PlatformMouseEvent(const Evas_Event_Mouse_Move* event, IntPoint position)
    : PlatformEvent(PlatformEvent::MouseMoved, evas_key_modifier_is_set(event->modifiers, "Shift"), evas_key_modifier_is_set(event->modifiers, "Control"), evas_key_modifier_is_set(event->modifiers, "Alt"), evas_key_modifier_is_set(event->modifiers, "Meta"), currentTime())
    , m_position(IntPoint(event->cur.canvas.x - position.x(), event->cur.canvas.y - position.y()))
    , m_globalPosition(IntPoint(event->cur.canvas.x, event->cur.canvas.y))
    , m_button(MouseButton(event->buttons - 1))
    , m_clickCount(0)
{
}

}
