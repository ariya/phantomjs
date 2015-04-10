/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 */

#ifndef CustomEvent_h
#define CustomEvent_h

#include "Event.h"
#include "ScriptValue.h"
#include "SerializedScriptValue.h"

namespace WebCore {

struct CustomEventInit : public EventInit {
    CustomEventInit();

    ScriptValue detail;
};

class CustomEvent : public Event {
public:
    virtual ~CustomEvent();

    static PassRefPtr<CustomEvent> create()
    {
        return adoptRef(new CustomEvent);
    }

    static PassRefPtr<CustomEvent> create(const AtomicString& type, const CustomEventInit& initializer)
    {
        return adoptRef(new CustomEvent(type, initializer));
    }

    void initCustomEvent(const AtomicString& type, bool canBubble, bool cancelable, const ScriptValue& detail);

    virtual const AtomicString& interfaceName() const;

    const ScriptValue& detail() const { return m_detail; }
    PassRefPtr<SerializedScriptValue> serializedScriptValue() { return m_serializedScriptValue; }

private:
    CustomEvent();
    CustomEvent(const AtomicString& type, const CustomEventInit& initializer);

    ScriptValue m_detail;
    RefPtr<SerializedScriptValue> m_serializedScriptValue;
};

} // namespace WebCore

#endif // CustomEvent_h
