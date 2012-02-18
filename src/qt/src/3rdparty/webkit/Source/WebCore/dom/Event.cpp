/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "EventTarget.h"

#include "UserGestureIndicator.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

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

Event::~Event()
{
}

void Event::initEvent(const AtomicString& eventTypeArg, bool canBubbleArg, bool cancelableArg)
{
    if (dispatched())
        return;

    m_type = eventTypeArg;
    m_canBubble = canBubbleArg;
    m_cancelable = cancelableArg;
}

bool Event::isCustomEvent() const
{
    return false;
}

bool Event::isUIEvent() const
{
    return false;
}

bool Event::isMouseEvent() const
{
    return false;
}

bool Event::isMutationEvent() const
{
    return false;
}

bool Event::isKeyboardEvent() const
{
    return false;
}

bool Event::isTextEvent() const
{
    return false;
}

bool Event::isCompositionEvent() const
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

bool Event::isWheelEvent() const
{
    return false;
}

bool Event::isMessageEvent() const
{
    return false;
}

bool Event::isBeforeTextInsertedEvent() const
{
    return false;
}

bool Event::isOverflowEvent() const
{
    return false;
}

bool Event::isPageTransitionEvent() const
{
    return false;
}

bool Event::isPopStateEvent() const
{
    return false;
}

bool Event::isProgressEvent() const
{
    return false;
}

bool Event::isWebKitAnimationEvent() const
{
    return false;
}

bool Event::isWebKitTransitionEvent() const
{
    return false;
}

bool Event::isXMLHttpRequestProgressEvent() const
{
    return false;
}

bool Event::isBeforeLoadEvent() const
{
    return false;
}

bool Event::isHashChangeEvent() const
{
    return false;
}

#if ENABLE(SVG)
bool Event::isSVGZoomEvent() const
{
    return false;
}
#endif

#if ENABLE(DOM_STORAGE)
bool Event::isStorageEvent() const
{
    return false;
}
#endif

#if ENABLE(INDEXED_DATABASE)
bool Event::isIDBVersionChangeEvent() const
{
    return false;
}
#endif

bool Event::isErrorEvent() const
{
    return false;
}

#if ENABLE(TOUCH_EVENTS)
bool Event::isTouchEvent() const
{
    return false;
}
#endif

#if ENABLE(DEVICE_ORIENTATION)
bool Event::isDeviceMotionEvent() const
{
    return false;
}

bool Event::isDeviceOrientationEvent() const
{
    return false;
}
#endif

#if ENABLE(WEB_AUDIO)
bool Event::isAudioProcessingEvent() const
{
    return false;
}

bool Event::isOfflineAudioCompletionEvent() const
{
    return false;
}
#endif

#if ENABLE(INPUT_SPEECH)
bool Event::isSpeechInputEvent() const
{
    return false;
}
#endif

bool Event::fromUserGesture()
{
    if (!UserGestureIndicator::processingUserGesture())
        return false;

    const AtomicString& type = this->type();
    return
        // mouse events
        type == eventNames().clickEvent || type == eventNames().mousedownEvent 
        || type == eventNames().mouseupEvent || type == eventNames().dblclickEvent 
        // keyboard events
        || type == eventNames().keydownEvent || type == eventNames().keypressEvent
        || type == eventNames().keyupEvent
#if ENABLE(TOUCH_EVENTS)
        // touch events
        || type == eventNames().touchstartEvent || type == eventNames().touchmoveEvent
        || type == eventNames().touchendEvent || type == eventNames().touchcancelEvent
#endif
        // other accepted events
        || type == eventNames().selectEvent || type == eventNames().changeEvent
        || type == eventNames().focusEvent || type == eventNames().blurEvent
        || type == eventNames().submitEvent;
}

bool Event::storesResultAsString() const
{
    return false;
}

void Event::storeResult(const String&)
{
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

EventDispatchMediator::EventDispatchMediator(PassRefPtr<Event> event)
    : m_event(event)
{
}

EventDispatchMediator::~EventDispatchMediator()
{
}

bool EventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    return dispatcher->dispatchEvent(m_event.get());
}

} // namespace WebCore
