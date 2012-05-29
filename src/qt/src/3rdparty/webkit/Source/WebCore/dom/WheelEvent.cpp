/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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
#include "WheelEvent.h"

#include "EventDispatcher.h"
#include "EventNames.h"
#include "PlatformWheelEvent.h"

#include <wtf/MathExtras.h>

namespace WebCore {

WheelEvent::WheelEvent()
    : m_wheelDeltaX(0)
    , m_wheelDeltaY(0)
    , m_rawDeltaX(0)
    , m_rawDeltaY(0)
    , m_granularity(Pixel)
{
}

WheelEvent::WheelEvent(float wheelTicksX, float wheelTicksY, float rawDeltaX, float rawDeltaY,
                       Granularity granularity, PassRefPtr<AbstractView> view,
                       int screenX, int screenY, int pageX, int pageY,
                       bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
    : MouseRelatedEvent(eventNames().mousewheelEvent,
                        true, true, view, 0, screenX, screenY, pageX, pageY,
                        ctrlKey, altKey, shiftKey, metaKey)
    , m_wheelDeltaX(lroundf(wheelTicksX * 120))
    , m_wheelDeltaY(lroundf(wheelTicksY * 120)) // Normalize to the Windows 120 multiple
    , m_rawDeltaX(rawDeltaX)
    , m_rawDeltaY(rawDeltaY)
    , m_granularity(granularity)
{
}

void WheelEvent::initWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView> view,
                                int screenX, int screenY, int pageX, int pageY,
                                bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
{
    if (dispatched())
        return;
    
    initUIEvent(eventNames().mousewheelEvent, true, true, view, 0);
    
    m_screenX = screenX;
    m_screenY = screenY;
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;
    
    // Normalize to the Windows 120 multiple
    m_wheelDeltaX = rawDeltaX * 120;
    m_wheelDeltaY = rawDeltaY * 120;
    
    m_rawDeltaX = rawDeltaX;
    m_rawDeltaY = rawDeltaY;
    m_granularity = Pixel;
    
    initCoordinates(pageX, pageY);
}

void WheelEvent::initWebKitWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView> view,
                                      int screenX, int screenY, int pageX, int pageY,
                                      bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
{
    initWheelEvent(rawDeltaX, rawDeltaY, view, screenX, screenY, pageX, pageY,
                   ctrlKey, altKey, shiftKey, metaKey);
}

bool WheelEvent::isWheelEvent() const
{
    return true;
}

inline static WheelEvent::Granularity granularity(const PlatformWheelEvent& event)
{
    return event.granularity() == ScrollByPageWheelEvent ? WheelEvent::Page : WheelEvent::Pixel;
}

WheelEventDispatchMediator::WheelEventDispatchMediator(const PlatformWheelEvent& event, PassRefPtr<AbstractView> view)
{
    if (!(event.deltaX() || event.deltaY()))
        return;

    setEvent(WheelEvent::create(event.wheelTicksX(), event.wheelTicksY(), event.deltaX(), event.deltaY(), granularity(event),
        view, event.globalX(), event.globalY(), event.x(), event.y(), event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey()));

}

WheelEvent* WheelEventDispatchMediator::event() const
{
    return static_cast<WheelEvent*>(EventDispatchMediator::event());
}

bool WheelEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    if (!event())
        return true;

    return EventDispatchMediator::dispatchEvent(dispatcher) && !event()->defaultHandled();
}

} // namespace WebCore
