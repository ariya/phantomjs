/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "UIRequestEvent.h"

#include "EventDispatcher.h"

#if ENABLE(INDIE_UI)

namespace WebCore {

UIRequestEventInit::UIRequestEventInit()
    : UIEventInit(true, true)
    , receiver(0)
{
}
    
PassRefPtr<UIRequestEvent> UIRequestEvent::create()
{
    return adoptRef(new UIRequestEvent);
}
    
PassRefPtr<UIRequestEvent> UIRequestEvent::create(const AtomicString& type, const UIRequestEventInit& initializer)
{
    return adoptRef(new UIRequestEvent(type, initializer));
}

PassRefPtr<UIRequestEvent> UIRequestEvent::create(const AtomicString& type, bool bubbles, bool cancelable, PassRefPtr<AbstractView> view, int detail, PassRefPtr<EventTarget> receiver)
{
    return adoptRef(new UIRequestEvent(type, bubbles, cancelable, view, detail, receiver));
}

UIRequestEvent::UIRequestEvent(const AtomicString& type, const UIRequestEventInit& initializer)
    : UIEvent(type, initializer.bubbles, initializer.cancelable, initializer.view, initializer.detail)
    , m_receiver(initializer.receiver)
{
}
    
UIRequestEvent::UIRequestEvent(const AtomicString& type, bool bubbles, bool cancelable, PassRefPtr<AbstractView> view, int detail, PassRefPtr<EventTarget> receiver)
    : UIEvent(type, bubbles, cancelable, view, detail)
    , m_receiver(receiver)
{
}

UIRequestEvent::UIRequestEvent()
    : m_receiver(0)
{
}

UIRequestEvent::~UIRequestEvent()
{
}
    
const AtomicString& UIRequestEvent::interfaceName() const
{
    return eventNames().interfaceForUIRequestEvent;
}

UIRequestEventDispatchMediator::UIRequestEventDispatchMediator(PassRefPtr<UIRequestEvent> event)
    : EventDispatchMediator(event)
{
}

UIRequestEvent* UIRequestEventDispatchMediator::event() const
{
    return static_cast<UIRequestEvent*>(EventDispatchMediator::event());
}

bool UIRequestEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    dispatcher->dispatch();
    return event()->defaultHandled() || event()->defaultPrevented();
}
    
} // namespace WebCore

#endif // ENABLE(INDIE_UI)

