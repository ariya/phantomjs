/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008, 2013 Apple Inc. All rights reserved.
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
#include "Event.h"

#include "EventDispatcher.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "UserGestureIndicator.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

EventInit::EventInit()
    : bubbles(false)
    , cancelable(false)
{
}

EventInit::EventInit(bool b, bool c)
    : bubbles(b)
    , cancelable(c)
{
}

Event::Event()
    : m_canBubble(false)
    , m_cancelable(false)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_defaultPrevented(false)
    , m_defaultHandled(false)
    , m_cancelBubble(false)
    , m_eventPhase(0)
    , m_currentTarget(0)
    , m_createTime(convertSecondsToDOMTimeStamp(currentTime()))
{
}

Event::Event(const AtomicString& eventType, bool canBubbleArg, bool cancelableArg)
    : m_type(eventType)
    , m_canBubble(canBubbleArg)
    , m_cancelable(cancelableArg)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_defaultPrevented(false)
    , m_defaultHandled(false)
    , m_cancelBubble(false)
    , m_eventPhase(0)
    , m_currentTarget(0)
    , m_createTime(convertSecondsToDOMTimeStamp(currentTime()))
{
}

Event::Event(const AtomicString& eventType, bool canBubbleArg, bool cancelableArg, double timestamp)
    : m_type(eventType)
    , m_canBubble(canBubbleArg)
    , m_cancelable(cancelableArg)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_defaultPrevented(false)
    , m_defaultHandled(false)
    , m_cancelBubble(false)
    , m_eventPhase(0)
    , m_currentTarget(0)
    , m_createTime(convertSecondsToDOMTimeStamp(timestamp))
{
}

Event::Event(const AtomicString& eventType, const EventInit& initializer)
    : m_type(eventType)
    , m_canBubble(initializer.bubbles)
    , m_cancelable(initializer.cancelable)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_defaultPrevented(false)
    , m_defaultHandled(false)
    , m_cancelBubble(false)
    , m_eventPhase(0)
    , m_currentTarget(0)
    , m_createTime(convertSecondsToDOMTimeStamp(currentTime()))
{
}

Event::~Event()
{
}

void Event::initEvent(const AtomicString& eventTypeArg, bool canBubbleArg, bool cancelableArg)
{
    if (dispatched())
        return;

    m_propagationStopped = false;
    m_immediatePropagationStopped = false;
    m_defaultPrevented = false;

    m_type = eventTypeArg;
    m_canBubble = canBubbleArg;
    m_cancelable = cancelableArg;
}

const AtomicString& Event::interfaceName() const
{
    return eventNames().interfaceForEvent;
}

bool Event::hasInterface(const AtomicString& name) const
{
    return interfaceName() == name;
}

bool Event::isUIEvent() const
{
    return false;
}

bool Event::isMouseEvent() const
{
    return false;
}

bool Event::isFocusEvent() const
{
    return false;
}

bool Event::isKeyboardEvent() const
{
    return false;
}

bool Event::isTouchEvent() const
{
    return false;
}

bool Event::isDragEvent() const
{
    return false;
}

bool Event::isClipboardEvent() const
{
    return false;
}

bool Event::storesResultAsString() const
{
    return false;
}

bool Event::isBeforeTextInsertedEvent() const
{
    return false;
}

void Event::storeResult(const String&)
{
}

PassRefPtr<Event> Event::cloneFor(HTMLIFrameElement*) const
{
    return Event::create(type(), bubbles(), cancelable());
}

void Event::setTarget(PassRefPtr<EventTarget> target)
{
    if (m_target == target)
        return;

    m_target = target;
    if (m_target)
        receivedTarget();
}

void Event::receivedTarget()
{
}

void Event::setUnderlyingEvent(PassRefPtr<Event> ue)
{
    // Prohibit creation of a cycle -- just do nothing in that case.
    for (Event* e = ue.get(); e; e = e->underlyingEvent())
        if (e == this)
            return;
    m_underlyingEvent = ue;
}

} // namespace WebCore
