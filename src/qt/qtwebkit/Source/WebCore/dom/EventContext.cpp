/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "EventContext.h"

#include "DOMWindow.h"
#include "Document.h"
#include "Event.h"
#include "FocusEvent.h"
#include "MouseEvent.h"
#include "Node.h"
#include "TouchEvent.h"
#include "TouchList.h"

namespace WebCore {

EventContext::EventContext(PassRefPtr<Node> node, PassRefPtr<EventTarget> currentTarget, PassRefPtr<EventTarget> target)
    : m_node(node)
    , m_currentTarget(currentTarget)
    , m_target(target)
{
    ASSERT(m_node);
    ASSERT(!isUnreachableNode(m_target.get()));
}

EventContext::~EventContext()
{
}

void EventContext::handleLocalEvents(Event* event) const
{
    event->setTarget(m_target.get());
    event->setCurrentTarget(m_currentTarget.get());
    m_node->handleLocalEvents(event);
}

bool EventContext::isMouseOrFocusEventContext() const
{
    return false;
}

bool EventContext::isTouchEventContext() const
{
    return false;
}

MouseOrFocusEventContext::MouseOrFocusEventContext(PassRefPtr<Node> node, PassRefPtr<EventTarget> currentTarget, PassRefPtr<EventTarget> target)
    : EventContext(node, currentTarget, target)
    , m_relatedTarget(0)
{
}

MouseOrFocusEventContext::~MouseOrFocusEventContext()
{
}

void MouseOrFocusEventContext::handleLocalEvents(Event* event) const
{
    ASSERT(event->isMouseEvent() || event->isFocusEvent());
    if (m_relatedTarget.get() && event->isMouseEvent())
        toMouseEvent(event)->setRelatedTarget(m_relatedTarget.get());
    else if (m_relatedTarget.get() && event->isFocusEvent())
        toFocusEvent(event)->setRelatedTarget(m_relatedTarget.get());
    EventContext::handleLocalEvents(event);
}

bool MouseOrFocusEventContext::isMouseOrFocusEventContext() const
{
    return true;
}

#if ENABLE(TOUCH_EVENTS)
TouchEventContext::TouchEventContext(PassRefPtr<Node> node, PassRefPtr<EventTarget> currentTarget, PassRefPtr<EventTarget> target)
    : EventContext(node, currentTarget, target)
    , m_touches(TouchList::create())
    , m_targetTouches(TouchList::create())
    , m_changedTouches(TouchList::create())
{
}

TouchEventContext::~TouchEventContext()
{
}

void TouchEventContext::handleLocalEvents(Event* event) const
{
#ifndef NDEBUG
    checkReachability(m_touches.get());
    checkReachability(m_targetTouches.get());
    checkReachability(m_changedTouches.get());
#endif
    ASSERT(event->isTouchEvent());
    TouchEvent* touchEvent = toTouchEvent(event);
    touchEvent->setTouches(m_touches);
    touchEvent->setTargetTouches(m_targetTouches);
    touchEvent->setChangedTouches(m_changedTouches);
    EventContext::handleLocalEvents(event);
}

bool TouchEventContext::isTouchEventContext() const
{
    return true;
}

#ifndef NDEBUG
void TouchEventContext::checkReachability(TouchList* touchList) const
{
    for (size_t i = 0; i < touchList->length(); ++i)
        ASSERT(isReachable(touchList->item(i)->target()->toNode()));
}
#endif

#endif // ENABLE(TOUCH_EVENTS)

}
