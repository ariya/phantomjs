/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PointerLockController.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Element.h"
#include "Event.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "VoidCallback.h"

#if ENABLE(POINTER_LOCK)

namespace WebCore {

PointerLockController::PointerLockController(Page* page)
    : m_page(page)
{
}

PassOwnPtr<PointerLockController> PointerLockController::create(Page* page)
{
    return adoptPtr(new PointerLockController(page));
}

void PointerLockController::requestPointerLock(Element* target)
{
    if (!target || !target->inDocument() || m_documentOfRemovedElementWhileWaitingForUnlock) {
        enqueueEvent(eventNames().webkitpointerlockerrorEvent, target);
        return;
    }

    if (target->document()->isSandboxed(SandboxPointerLock)) {
        // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
        target->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Blocked pointer lock on an element because the element's frame is sandboxed and the 'allow-pointer-lock' permission is not set.");
        enqueueEvent(eventNames().webkitpointerlockerrorEvent, target);
        return;
    }

    if (m_element) {
        if (m_element->document() != target->document()) {
            enqueueEvent(eventNames().webkitpointerlockerrorEvent, target);
            return;
        }
        enqueueEvent(eventNames().webkitpointerlockchangeEvent, target);
        m_element = target;
    } else if (m_page->chrome().client()->requestPointerLock()) {
        m_lockPending = true;
        m_element = target;
    } else {
        enqueueEvent(eventNames().webkitpointerlockerrorEvent, target);
    }
}

void PointerLockController::requestPointerUnlock()
{
    return m_page->chrome().client()->requestPointerUnlock();
}

void PointerLockController::elementRemoved(Element* element)
{
    if (m_element == element) {
        m_documentOfRemovedElementWhileWaitingForUnlock = m_element->document();
        // Set element null immediately to block any future interaction with it
        // including mouse events received before the unlock completes.
        clearElement();
        requestPointerUnlock();
    }
}

void PointerLockController::documentDetached(Document* document)
{
    if (m_element && m_element->document() == document) {
        clearElement();
        requestPointerUnlock();
    }
}

bool PointerLockController::lockPending() const
{
    return m_lockPending;
}

Element* PointerLockController::element() const
{
    return m_element.get();
}

void PointerLockController::didAcquirePointerLock()
{
    enqueueEvent(eventNames().webkitpointerlockchangeEvent, m_element.get());
    m_lockPending = false;
}

void PointerLockController::didNotAcquirePointerLock()
{
    enqueueEvent(eventNames().webkitpointerlockerrorEvent, m_element.get());
    clearElement();
}

void PointerLockController::didLosePointerLock()
{
    enqueueEvent(eventNames().webkitpointerlockchangeEvent, m_element ? m_element->document() : m_documentOfRemovedElementWhileWaitingForUnlock.get());
    clearElement();
    m_documentOfRemovedElementWhileWaitingForUnlock = 0;
}

void PointerLockController::dispatchLockedMouseEvent(const PlatformMouseEvent& event, const AtomicString& eventType)
{
    if (!m_element || !m_element->document()->frame())
        return;

    m_element->dispatchMouseEvent(event, eventType, event.clickCount());

    // Create click events
    if (eventType == eventNames().mouseupEvent)
        m_element->dispatchMouseEvent(event, eventNames().clickEvent, event.clickCount());
}

void PointerLockController::clearElement()
{
    m_lockPending = false;
    m_element = 0;
}

void PointerLockController::enqueueEvent(const AtomicString& type, Element* element)
{
    if (element)
        enqueueEvent(type, element->document());
}

void PointerLockController::enqueueEvent(const AtomicString& type, Document* document)
{
    if (document)
        document->enqueueDocumentEvent(Event::create(type, true, false));
}

} // namespace WebCore

#endif // ENABLE(POINTER_LOCK)
