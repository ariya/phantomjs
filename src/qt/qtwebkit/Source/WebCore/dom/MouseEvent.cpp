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
#include "MouseEvent.h"

#include "Clipboard.h"
#include "EventDispatcher.h"
#include "EventNames.h"
#include "EventRetargeter.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLIFrameElement.h"
#include "PlatformMouseEvent.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

MouseEventInit::MouseEventInit()
    : screenX(0)
    , screenY(0)
    , clientX(0)
    , clientY(0)
    , ctrlKey(false)
    , altKey(false)
    , shiftKey(false)
    , metaKey(false)
    , button(0)
    , relatedTarget(0)
{
}

PassRefPtr<MouseEvent> MouseEvent::create(const AtomicString& type, const MouseEventInit& initializer)
{
    return adoptRef(new MouseEvent(type, initializer));
}

PassRefPtr<MouseEvent> MouseEvent::create(const AtomicString& eventType, PassRefPtr<AbstractView> view, const PlatformMouseEvent& event, int detail, PassRefPtr<Node> relatedTarget)
{
    ASSERT(event.type() == PlatformEvent::MouseMoved || event.button() != NoButton);

    bool isMouseEnterOrLeave = eventType == eventNames().mouseenterEvent || eventType == eventNames().mouseleaveEvent;
    bool isCancelable = eventType != eventNames().mousemoveEvent && !isMouseEnterOrLeave;
    bool canBubble = !isMouseEnterOrLeave;

    return MouseEvent::create(eventType, canBubble, isCancelable, event.timestamp(), view,
        detail, event.globalPosition().x(), event.globalPosition().y(), event.position().x(), event.position().y(),
#if ENABLE(POINTER_LOCK)
        event.movementDelta().x(), event.movementDelta().y(),
#endif
        event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(), event.button(),
        relatedTarget);
}

PassRefPtr<MouseEvent> MouseEvent::create(const AtomicString& type, bool canBubble, bool cancelable, double timestamp, PassRefPtr<AbstractView> view,
    int detail, int screenX, int screenY, int pageX, int pageY,
#if ENABLE(POINTER_LOCK)
    int movementX, int movementY,
#endif
    bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, unsigned short button,
    PassRefPtr<EventTarget> relatedTarget)

{
    return MouseEvent::create(type, canBubble, cancelable, timestamp, view,
        detail, screenX, screenY, pageX, pageY,
#if ENABLE(POINTER_LOCK)
        movementX, movementY,
#endif
        ctrlKey, altKey, shiftKey, metaKey, button, relatedTarget, 0, false);
}

PassRefPtr<MouseEvent> MouseEvent::create(const AtomicString& type, bool canBubble, bool cancelable, double timestamp, PassRefPtr<AbstractView> view,
    int detail, int screenX, int screenY, int pageX, int pageY,
#if ENABLE(POINTER_LOCK)
    int movementX, int movementY,
#endif
    bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, unsigned short button,
    PassRefPtr<EventTarget> relatedTarget, PassRefPtr<Clipboard> clipboard, bool isSimulated)
{
    return adoptRef(new MouseEvent(type, canBubble, cancelable, timestamp, view,
        detail, screenX, screenY, pageX, pageY,
#if ENABLE(POINTER_LOCK)
        movementX, movementY,
#endif
        ctrlKey, altKey, shiftKey, metaKey, button, relatedTarget, clipboard, isSimulated));
}

MouseEvent::MouseEvent()
    : m_button(0)
    , m_buttonDown(false)
{
}

MouseEvent::MouseEvent(const AtomicString& eventType, bool canBubble, bool cancelable, double timestamp, PassRefPtr<AbstractView> view,
                       int detail, int screenX, int screenY, int pageX, int pageY,
#if ENABLE(POINTER_LOCK)
                       int movementX, int movementY,
#endif
                       bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
                       unsigned short button, PassRefPtr<EventTarget> relatedTarget,
                       PassRefPtr<Clipboard> clipboard, bool isSimulated)
    : MouseRelatedEvent(eventType, canBubble, cancelable, timestamp, view, detail, IntPoint(screenX, screenY),
                        IntPoint(pageX, pageY),
#if ENABLE(POINTER_LOCK)
                        IntPoint(movementX, movementY),
#endif
                        ctrlKey, altKey, shiftKey, metaKey, isSimulated)
    , m_button(button == (unsigned short)-1 ? 0 : button)
    , m_buttonDown(button != (unsigned short)-1)
    , m_relatedTarget(relatedTarget)
    , m_clipboard(clipboard)
{
}

MouseEvent::MouseEvent(const AtomicString& eventType, const MouseEventInit& initializer)
    : MouseRelatedEvent(eventType, initializer.bubbles, initializer.cancelable, currentTime(), initializer.view, initializer.detail, IntPoint(initializer.screenX, initializer.screenY),
        IntPoint(0 /* pageX */, 0 /* pageY */),
#if ENABLE(POINTER_LOCK)
        IntPoint(0 /* movementX */, 0 /* movementY */),
#endif
        initializer.ctrlKey, initializer.altKey, initializer.shiftKey, initializer.metaKey, false /* isSimulated */)
    , m_button(initializer.button == (unsigned short)-1 ? 0 : initializer.button)
    , m_buttonDown(initializer.button != (unsigned short)-1)
    , m_relatedTarget(initializer.relatedTarget)
    , m_clipboard(0 /* clipboard */)
{
    initCoordinates(IntPoint(initializer.clientX, initializer.clientY));
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

    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;
    m_button = button == (unsigned short)-1 ? 0 : button;
    m_buttonDown = button != (unsigned short)-1;
    m_relatedTarget = relatedTarget;

    initCoordinates(IntPoint(clientX, clientY));

    // FIXME: m_isSimulated is not set to false here.
    // FIXME: m_clipboard is not set to 0 here.
}

const AtomicString& MouseEvent::interfaceName() const
{
    return eventNames().interfaceForMouseEvent;
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
    if (!m_buttonDown)
        return 0;
    return m_button + 1;
}

Node* MouseEvent::toElement() const
{
    // MSIE extension - "the object toward which the user is moving the mouse pointer"
    if (type() == eventNames().mouseoutEvent || type() == eventNames().mouseleaveEvent)
        return relatedTarget() ? relatedTarget()->toNode() : 0;
    
    return target() ? target()->toNode() : 0;
}

Node* MouseEvent::fromElement() const
{
    // MSIE extension - "object from which activation or the mouse pointer is exiting during the event" (huh?)
    if (type() != eventNames().mouseoutEvent && type() != eventNames().mouseleaveEvent)
        return relatedTarget() ? relatedTarget()->toNode() : 0;
    
    return target() ? target()->toNode() : 0;
}

// FIXME: Fix positioning. e.g. We need to consider border/padding.
// https://bugs.webkit.org/show_bug.cgi?id=93696
inline static int adjustedClientX(int innerClientX, HTMLIFrameElement* iframe, FrameView* frameView)
{
    return iframe->offsetLeft() - frameView->scrollX() + innerClientX;
}

inline static int adjustedClientY(int innerClientY, HTMLIFrameElement* iframe, FrameView* frameView)
{
    return iframe->offsetTop() - frameView->scrollY() + innerClientY;
}

PassRefPtr<Event> MouseEvent::cloneFor(HTMLIFrameElement* iframe) const
{
    ASSERT(iframe);
    RefPtr<MouseEvent> clonedMouseEvent = MouseEvent::create();
    Frame* frame = iframe->document()->frame();
    FrameView* frameView = frame ? frame->view() : 0;
    clonedMouseEvent->initMouseEvent(type(), bubbles(), cancelable(),
            iframe->document()->defaultView(),
            detail(), screenX(), screenY(),
            frameView ? adjustedClientX(clientX(), iframe, frameView) : 0,
            frameView ? adjustedClientY(clientY(), iframe, frameView) : 0,
            ctrlKey(), altKey(), shiftKey(), metaKey(),
            button(),
            // Nullifies relatedTarget.
            0);
    return clonedMouseEvent.release();
}

PassRefPtr<SimulatedMouseEvent> SimulatedMouseEvent::create(const AtomicString& eventType, PassRefPtr<AbstractView> view, PassRefPtr<Event> underlyingEvent)
{
    return adoptRef(new SimulatedMouseEvent(eventType, view, underlyingEvent));
}

SimulatedMouseEvent::~SimulatedMouseEvent()
{
}

SimulatedMouseEvent::SimulatedMouseEvent(const AtomicString& eventType, PassRefPtr<AbstractView> view, PassRefPtr<Event> underlyingEvent)
    : MouseEvent(eventType, true, true, underlyingEvent ? underlyingEvent->timeStamp() : currentTime(), view, 0, 0, 0, 0, 0,
#if ENABLE(POINTER_LOCK)
                 0, 0,
#endif
                 false, false, false, false, 0, 0, 0, true)
{
    if (UIEventWithKeyState* keyStateEvent = findEventWithKeyState(underlyingEvent.get())) {
        m_ctrlKey = keyStateEvent->ctrlKey();
        m_altKey = keyStateEvent->altKey();
        m_shiftKey = keyStateEvent->shiftKey();
        m_metaKey = keyStateEvent->metaKey();
    }
    setUnderlyingEvent(underlyingEvent);

    if (this->underlyingEvent() && this->underlyingEvent()->isMouseEvent()) {
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(this->underlyingEvent());
        m_screenLocation = mouseEvent->screenLocation();
        initCoordinates(mouseEvent->clientLocation());
    }
}

PassRefPtr<MouseEventDispatchMediator> MouseEventDispatchMediator::create(PassRefPtr<MouseEvent> mouseEvent, MouseEventType mouseEventType)
{
    return adoptRef(new MouseEventDispatchMediator(mouseEvent, mouseEventType));
}

MouseEventDispatchMediator::MouseEventDispatchMediator(PassRefPtr<MouseEvent> mouseEvent, MouseEventType mouseEventType)
    : EventDispatchMediator(mouseEvent), m_mouseEventType(mouseEventType)
{
}

MouseEvent* MouseEventDispatchMediator::event() const
{
    return static_cast<MouseEvent*>(EventDispatchMediator::event());
}

bool MouseEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    if (isSyntheticMouseEvent()) {
        EventRetargeter::adjustForMouseEvent(dispatcher->node(), *event(),  dispatcher->eventPath());
        return dispatcher->dispatch();
    }

    if (isDisabledFormControl(dispatcher->node()))
        return false;

    if (event()->type().isEmpty())
        return true; // Shouldn't happen.

    ASSERT(!event()->target() || event()->target() != event()->relatedTarget());

    EventTarget* relatedTarget = event()->relatedTarget();
    EventRetargeter::adjustForMouseEvent(dispatcher->node(), *event(),  dispatcher->eventPath());

    dispatcher->dispatch();
    bool swallowEvent = event()->defaultHandled() || event()->defaultPrevented();

    if (event()->type() != eventNames().clickEvent || event()->detail() != 2)
        return !swallowEvent;

    // Special case: If it's a double click event, we also send the dblclick event. This is not part
    // of the DOM specs, but is used for compatibility with the ondblclick="" attribute. This is treated
    // as a separate event in other DOM-compliant browsers like Firefox, and so we do the same.
    RefPtr<MouseEvent> doubleClickEvent = MouseEvent::create();
    doubleClickEvent->initMouseEvent(eventNames().dblclickEvent, event()->bubbles(), event()->cancelable(), event()->view(),
                                     event()->detail(), event()->screenX(), event()->screenY(), event()->clientX(), event()->clientY(),
                                     event()->ctrlKey(), event()->altKey(), event()->shiftKey(), event()->metaKey(),
                                     event()->button(), relatedTarget);
    if (event()->defaultHandled())
        doubleClickEvent->setDefaultHandled();
    EventDispatcher::dispatchEvent(dispatcher->node(), MouseEventDispatchMediator::create(doubleClickEvent));
    if (doubleClickEvent->defaultHandled() || doubleClickEvent->defaultPrevented())
        return false;
    return !swallowEvent;
}

} // namespace WebCore
