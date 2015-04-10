/*
 * Copyright 2008, The Android Open Source Project
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#if ENABLE(TOUCH_EVENTS)

#include "TouchEvent.h"

#include "EventDispatcher.h"
#include "EventNames.h"
#include "EventRetargeter.h"
#include "TouchList.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

TouchEvent::TouchEvent()
{
}

TouchEvent::TouchEvent(TouchList* touches, TouchList* targetTouches,
        TouchList* changedTouches, const AtomicString& type, 
        PassRefPtr<AbstractView> view, int screenX, int screenY, int pageX, int pageY,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
    : MouseRelatedEvent(type, true, true, currentTime(), view, 0, IntPoint(screenX, screenY),
                        IntPoint(pageX, pageY),
#if ENABLE(POINTER_LOCK)
                        IntPoint(0, 0),
#endif
                        ctrlKey, altKey, shiftKey, metaKey)
    , m_touches(touches)
    , m_targetTouches(targetTouches)
    , m_changedTouches(changedTouches)
#if PLATFORM(BLACKBERRY)
    , m_touchHold(false)
    , m_doubleTap(false)
#endif
{
}

TouchEvent::~TouchEvent()
{
}

void TouchEvent::initTouchEvent(TouchList* touches, TouchList* targetTouches,
        TouchList* changedTouches, const AtomicString& type, 
        PassRefPtr<AbstractView> view, int screenX, int screenY, int clientX, int clientY,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
{
    if (dispatched())
        return;

    initUIEvent(type, true, true, view, 0);

    m_touches = touches;
    m_targetTouches = targetTouches;
    m_changedTouches = changedTouches;
    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;
    initCoordinates(IntPoint(clientX, clientY));
#if PLATFORM(BLACKBERRY)
    m_doubleTap = false;
    m_touchHold = false;
#endif

}

const AtomicString& TouchEvent::interfaceName() const
{
    return eventNames().interfaceForTouchEvent;
}

bool TouchEvent::isTouchEvent() const
{
    return true;
}

PassRefPtr<TouchEventDispatchMediator> TouchEventDispatchMediator::create(PassRefPtr<TouchEvent> touchEvent)
{
    return adoptRef(new TouchEventDispatchMediator(touchEvent));
}

TouchEventDispatchMediator::TouchEventDispatchMediator(PassRefPtr<TouchEvent> touchEvent)
    : EventDispatchMediator(touchEvent)
{
}

TouchEvent* TouchEventDispatchMediator::event() const
{
    return static_cast<TouchEvent*>(EventDispatchMediator::event());
}

bool TouchEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    EventRetargeter::adjustForTouchEvent(dispatcher->node(), *event(), dispatcher->eventPath());
    return dispatcher->dispatch();
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
