/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
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

#if ENABLE(GESTURE_EVENTS)

#include "GestureEvent.h"

#include "Element.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

PassRefPtr<GestureEvent> GestureEvent::create()
{
    return adoptRef(new GestureEvent);
}

PassRefPtr<GestureEvent> GestureEvent::create(PassRefPtr<AbstractView> view, const PlatformGestureEvent& event)
{
    AtomicString eventType;
    switch (event.type()) {
    case PlatformEvent::GestureScrollBegin:
        eventType = eventNames().gesturescrollstartEvent; break;
    case PlatformEvent::GestureScrollEnd:
        eventType = eventNames().gesturescrollendEvent; break;
    case PlatformEvent::GestureScrollUpdate:
    case PlatformEvent::GestureScrollUpdateWithoutPropagation:
        eventType = eventNames().gesturescrollupdateEvent; break;
    case PlatformEvent::GestureTap:
        eventType = eventNames().gesturetapEvent; break;
    case PlatformEvent::GestureTapDown:
        eventType = eventNames().gesturetapdownEvent; break;
    case PlatformEvent::GestureTwoFingerTap:
    case PlatformEvent::GestureLongPress:
    case PlatformEvent::GestureTapDownCancel:
    default:
        return 0;
    }
    return adoptRef(new GestureEvent(eventType, event.timestamp(), view, event.globalPosition().x(), event.globalPosition().y(), event.position().x(), event.position().y(), event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(), event.deltaX(), event.deltaY()));
}

void GestureEvent::initGestureEvent(const AtomicString& type, PassRefPtr<AbstractView> view, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, float deltaX, float deltaY)
{
    if (dispatched())
        return;

    initUIEvent(type, true, true, view, 0);
    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;

    m_deltaX = deltaX;
    m_deltaY = deltaY;

    initCoordinates(IntPoint(clientX, clientY));
}

const AtomicString& GestureEvent::interfaceName() const
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("TBDInterface"));
    return name;
}

GestureEvent::GestureEvent()
    : m_deltaX(0)
    , m_deltaY(0)
{
}

GestureEvent::GestureEvent(const AtomicString& type, double timestamp, PassRefPtr<AbstractView> view, int screenX, int screenY, int clientX, int clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, float deltaX, float deltaY)
    : MouseRelatedEvent(type, true, true, timestamp, view, 0, IntPoint(screenX, screenY), IntPoint(clientX, clientY),
#if ENABLE(POINTER_LOCK)
                        IntPoint(0, 0),
#endif
                        ctrlKey, altKey, shiftKey, metaKey)
    , m_deltaX(deltaX)
    , m_deltaY(deltaY)
{
}

GestureEventDispatchMediator::GestureEventDispatchMediator(PassRefPtr<GestureEvent> gestureEvent)
    : EventDispatchMediator(gestureEvent)
{
}

GestureEvent* GestureEventDispatchMediator::event() const
{
    return static_cast<GestureEvent*>(EventDispatchMediator::event());
}

bool GestureEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    if (isDisabledFormControl(dispatcher->node()))
        return true;

    dispatcher->dispatch();
    ASSERT(!event()->defaultPrevented());
    return event()->defaultHandled() || event()->defaultPrevented();
}

} // namespace WebCore

#endif // ENABLE(GESTURE_EVENTS)
