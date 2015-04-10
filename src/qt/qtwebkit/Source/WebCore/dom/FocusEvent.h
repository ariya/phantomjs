/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FocusEvent_h
#define FocusEvent_h

#include "EventTarget.h"
#include "UIEvent.h"

namespace WebCore {

class Node;

struct FocusEventInit : public UIEventInit {
    FocusEventInit();

    RefPtr<EventTarget> relatedTarget;
};

class FocusEvent : public UIEvent {
public:
    static PassRefPtr<FocusEvent> create()
    {
        return adoptRef(new FocusEvent);
    }

    static PassRefPtr<FocusEvent> create(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<AbstractView> view, int detail, PassRefPtr<EventTarget> relatedTarget)
    {
        return adoptRef(new FocusEvent(type, canBubble, cancelable, view, detail, relatedTarget));
    }

    static PassRefPtr<FocusEvent> create(const AtomicString& type, const FocusEventInit& initializer)
    {
        return adoptRef(new FocusEvent(type, initializer));
    }

    EventTarget* relatedTarget() const { return m_relatedTarget.get(); }
    void setRelatedTarget(PassRefPtr<EventTarget> relatedTarget) { m_relatedTarget = relatedTarget; }

    virtual const AtomicString& interfaceName() const;
    virtual bool isFocusEvent() const;

private:
    FocusEvent();
    FocusEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<AbstractView>, int, PassRefPtr<EventTarget>);
    FocusEvent(const AtomicString& type, const FocusEventInit&);

    RefPtr<EventTarget> m_relatedTarget;
};

inline FocusEvent* toFocusEvent(Event* event)
{
    ASSERT(event && event->isFocusEvent());
    return static_cast<FocusEvent*>(event);
}

class FocusEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<FocusEventDispatchMediator> create(PassRefPtr<FocusEvent>);
private:
    explicit FocusEventDispatchMediator(PassRefPtr<FocusEvent>);
    FocusEvent* event() const { return static_cast<FocusEvent*>(EventDispatchMediator::event()); }
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};

class BlurEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<BlurEventDispatchMediator> create(PassRefPtr<FocusEvent>);
private:
    explicit BlurEventDispatchMediator(PassRefPtr<FocusEvent>);
    FocusEvent* event() const { return static_cast<FocusEvent*>(EventDispatchMediator::event()); }
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};

class FocusInEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<FocusInEventDispatchMediator> create(PassRefPtr<FocusEvent>);
private:
    explicit FocusInEventDispatchMediator(PassRefPtr<FocusEvent>);
    FocusEvent* event() const { return static_cast<FocusEvent*>(EventDispatchMediator::event()); }
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};

class FocusOutEventDispatchMediator : public EventDispatchMediator {
public:
    static PassRefPtr<FocusOutEventDispatchMediator> create(PassRefPtr<FocusEvent>);
private:
    explicit FocusOutEventDispatchMediator(PassRefPtr<FocusEvent>);
    FocusEvent* event() const { return static_cast<FocusEvent*>(EventDispatchMediator::event()); }
    virtual bool dispatchEvent(EventDispatcher*) const OVERRIDE;
};

} // namespace WebCore

#endif // FocusEvent_h
