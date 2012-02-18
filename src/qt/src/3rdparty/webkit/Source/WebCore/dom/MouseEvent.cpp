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
#include "MouseEvent.h"

#include "EventDispatcher.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "PlatformMouseEvent.h"

namespace WebCore {

PassRefPtr<MouseEvent> MouseEvent::create(const AtomicString& eventType, PassRefPtr<AbstractView> view, const PlatformMouseEvent& event, int detail, PassRefPtr<Node> relatedTarget)
{
    ASSERT(event.eventType() == MouseEventMoved || event.button() != NoButton);

    bool isCancelable = eventType != eventNames().mousemoveEvent;

    return MouseEvent::create(eventType, true, isCancelable, view,
        detail, event.globalX(), event.globalY(), event.x(), event.y(),
        event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(), event.button(),
        relatedTarget, 0, false);
}

MouseEvent::MouseEvent()
    : m_button(0)
    , m_buttonDown(false)
{
}

MouseEvent::MouseEvent(const AtomicString& eventType, bool canBubble, bool cancelable, PassRefPtr<AbstractView> view,
                       int detail, int screenX, int screenY, int pageX, int pageY,
                       bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
                       unsigned short button, PassRefPtr<EventTarget> relatedTarget,
                       PassRefPtr<Clipboard> clipboard, bool isSimulated)
    : MouseRelatedEvent(eventType, canBubble, cancelable, view, detail, screenX, screenY,
                        pageX, pageY, ctrlKey, altKey, shiftKey, metaKey, isSimulated)
    , m_button(button == (unsigned short)-1 ? 0 : button)
    , m_buttonDown(button != (unsigned short)-1)
    , m_relatedTarget(relatedTarget)
    , m_clipboard(clipboard)
{
}

MouseEvent::~MouseEvent()
{
}

void MouseEvent::initMouseEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<AbstractView> view,
                                int detail, int screenX, int screenY, int clientX, int clientY,
                                bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
                                unsigned short button, PassRefPtr<EventTarget> relatedTarget)
{
    if (dispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, detail);

    m_screenX = screenX;
    m_screenY = screenY;
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;
    m_button = button == (unsigned short)-1 ? 0 : button;
    m_buttonDown = button != (unsigned short)-1;
    m_relatedTarget = relatedTarget;

    initCoordinates(clientX, clientY);

    // FIXME: m_isSimulated is not set to false here.
    // FIXME: m_clipboard is not set to 0 here.
}

bool MouseEvent::isMouseEvent() const
{
    return true;
}

bool MouseEvent::isDragEvent() const
{
    const AtomicString& t = type();
    return t == eventNames().dragenterEvent || t == eventNames().dragoverEvent || t == eventNames().dragleaveEvent || t == eventNames().dropEvent
               || t == eventNames().dragstartEvent|| t == eventNames().dragEvent || t == eventNames().dragendEvent;
}

int MouseEvent::which() const
{
    // For the DOM, the return values for left, middle and right mouse buttons are 0, 1, 2, respectively.
    // For the Netscape "which" property, the return values for left, middle and right mouse buttons are 1, 2, 3, respectively. 
    // So we must add 1.
    return m_button + 1;
}

Node* MouseEvent::toElement() const
{
    // MSIE extension - "the object toward which the user is moving the mouse pointer"
    if (type() == eventNames().mouseoutEvent) 
        return relatedTarget() ? relatedTarget()->toNode() : 0;
    
    return target() ? target()->toNode() : 0;
}

Node* MouseEvent::fromElement() const
{
    // MSIE extension - "object from which activation or the mouse pointer is exiting during the event" (huh?)
    if (type() != eventNames().mouseoutEvent)
        return relatedTarget() ? relatedTarget()->toNode() : 0;
    
    return target() ? target()->toNode() : 0;
}

PassRefPtr<SimulatedMouseEvent> SimulatedMouseEvent::create(const AtomicString& eventType, PassRefPtr<AbstractView> view, PassRefPtr<Event> underlyingEvent)
{
    return adoptRef(new SimulatedMouseEvent(eventType, view, underlyingEvent));
}

SimulatedMouseEvent::~SimulatedMouseEvent()
{
}

SimulatedMouseEvent::SimulatedMouseEvent(const AtomicString& eventType, PassRefPtr<AbstractView> view, PassRefPtr<Event> underlyingEvent)
    : MouseEvent(eventType, true, true, view, 0, 0, 0, 0, 0, false, false, false, false, 0, 0, 0, true)
{
    if (UIEventWithKeyState* keyStateEvent = findEventWithKeyState(underlyingEvent.get())) {
        m_ctrlKey = keyStateEvent->ctrlKey();
        m_altKey = keyStateEvent->altKey();
        m_shiftKey = keyStateEvent->shiftKey();
        m_metaKey = keyStateEvent->metaKey();
    }
    setUnderlyingEvent(underlyingEvent);
}

MouseEventDispatchMediator::MouseEventDispatchMediator(PassRefPtr<MouseEvent> mouseEvent)
    : EventDispatchMediator(mouseEvent)
{
}

MouseEvent* MouseEventDispatchMediator::event() const
{
    return static_cast<MouseEvent*>(EventDispatchMediator::event());
}

bool MouseEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    if (dispatcher->node()->disabled()) // Don't even send DOM events for disabled controls..
        return true;

    if (event()->type().isEmpty())
        return false; // Shouldn't happen.

    RefPtr<EventTarget> relatedTarget = dispatcher->adjustRelatedTarget(event(), event()->relatedTarget());
    event()->setRelatedTarget(relatedTarget);

    dispatcher->dispatchEvent(event());
    bool swallowEvent = event()->defaultHandled() || event()->defaultPrevented();

    // Special case: If it's a double click event, we also send the dblclick event. This is not part
    // of the DOM specs, but is used for compatibility with the ondblclick="" attribute. This is treated
    // as a separate event in other DOM-compliant browsers like Firefox, and so we do the same.
    if (event()->type() == eventNames().clickEvent && event()->detail() == 2) {
        RefPtr<MouseEvent> doubleClickEvent = MouseEvent::create();
        doubleClickEvent->initMouseEvent(eventNames().dblclickEvent, event()->bubbles(), event()->cancelable(), event()->view(),
                event()->detail(), event()->screenX(), event()->screenY(), event()->clientX(), event()->clientY(),
                event()->ctrlKey(), event()->altKey(), event()->shiftKey(), event()->metaKey(),
                event()->button(), relatedTarget);
        if (event()->defaultHandled())
            doubleClickEvent->setDefaultHandled();
        dispatcher->dispatchEvent(doubleClickEvent);
        if (doubleClickEvent->defaultHandled() || doubleClickEvent->defaultPrevented())
            swallowEvent = true;
    }

    return swallowEvent;
}

} // namespace WebCore
