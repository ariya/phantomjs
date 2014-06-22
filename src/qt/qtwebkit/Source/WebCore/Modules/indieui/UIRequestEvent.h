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

#ifndef UIRequestEvent_h
#define UIRequestEvent_h

#include "EventTarget.h"
#include "UIEvent.h"

#if ENABLE(INDIE_UI)

namespace WebCore {
    
struct UIRequestEventInit : public UIEventInit {
    UIRequestEventInit();
    
    RefPtr<EventTarget> receiver;
};

class UIRequestEvent : public UIEvent {
public:
    static PassRefPtr<UIRequestEvent> create();
    static PassRefPtr<UIRequestEvent> create(const AtomicString& type, bool bubbles, bool cancelable, PassRefPtr<AbstractView>, int detail, PassRefPtr<EventTarget> receiver);
    
    static PassRefPtr<UIRequestEvent> create(const AtomicString& eventType, const UIRequestEventInit&);
    
    virtual ~UIRequestEvent();
    
    EventTarget* receiver() const { return m_receiver.get(); }

protected:
    UIRequestEvent(const AtomicString& type, bool bubbles, bool cancelable, PassRefPtr<AbstractView>, int detail, PassRefPtr<EventTarget> receiver);
    
    UIRequestEvent(const AtomicString& type, const UIRequestEventInit&);
    
    UIRequestEvent();
    
    const AtomicString& interfaceName() const OVERRIDE;
    
private:
    RefPtr<EventTarget> m_receiver;
};

class UIRequestEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<UIRequestEventDispatchMediator> create(PassRefPtr<UIRequestEvent> event)
    {
        return adoptRef(new UIRequestEventDispatchMediator(event));
    }
    
private:
    explicit UIRequestEventDispatchMediator(PassRefPtr<UIRequestEvent>);
    
    UIRequestEvent* event() const;
    
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};
    
} // namespace WebCore

#endif // ENABLE(INDIE_UI)

#endif // UIRequestEvent_h
