/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *           (C) 2006 Michael Emmel mike.emmel@gmail.com.  All rights reserved.
 *           (C) 2008 Kenneth Rohde Christiansen.  All rights reserved.
 *           (C) 2009 INdT - Instituto Nokia de Tecnologia.
 *           (C) 2009-2010 ProFUSION embedded systems
 *           (C) 2009-2010 Samsung Electronics
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

#include <Evas.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

enum {
    VerticalScrollDirection = 0,
    HorizontalScrollDirection = 1
};

PlatformWheelEvent::PlatformWheelEvent(const Evas_Event_Mouse_Wheel* ev)
    : PlatformEvent(PlatformEvent::Wheel, evas_key_modifier_is_set(ev->modifiers, "Shift"), evas_key_modifier_is_set(ev->modifiers, "Control"), evas_key_modifier_is_set(ev->modifiers, "Alt"), evas_key_modifier_is_set(ev->modifiers, "Meta"), currentTime())
    , m_position(IntPoint(ev->canvas.x, ev->canvas.y))
    , m_globalPosition(IntPoint(ev->canvas.x, ev->canvas.y))
    , m_granularity(ScrollByPixelWheelEvent)
    , m_directionInvertedFromDevice(false)
{
    // A negative z value means (in EFL) that we are scrolling down, so we need
    // to invert the value.
    if (ev->direction == VerticalScrollDirection) {
        m_deltaX = 0;
        m_deltaY = - ev->z;
    } else if (ev->direction == HorizontalScrollDirection) {
        m_deltaX = - ev->z;
        m_deltaY = 0;
    }

    // FIXME: retrieve the user setting for the number of lines to scroll on each wheel event
    m_wheelTicksX = m_deltaX;
    m_wheelTicksY = m_deltaY;
    m_deltaX *= static_cast<float>(Scrollbar::pixelsPerLineStep());
    m_deltaY *= static_cast<float>(Scrollbar::pixelsPerLineStep());
}

}
