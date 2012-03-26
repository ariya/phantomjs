/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "MessageEvent.h"

#include "DOMWindow.h"
#include "EventNames.h"

namespace WebCore {

MessageEvent::MessageEvent()
    : m_data(SerializedScriptValue::create())
{
}

MessageEvent::MessageEvent(PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, PassRefPtr<DOMWindow> source, PassOwnPtr<MessagePortArray> ports)
    : Event(eventNames().messageEvent, false, false)
    , m_data(data)
    , m_origin(origin)
    , m_lastEventId(lastEventId)
    , m_source(source)
    , m_ports(ports)
{
}

MessageEvent::~MessageEvent()
{
}

void MessageEvent::initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, PassOwnPtr<MessagePortArray> ports)
{
    if (dispatched())
        return;
        
    initEvent(type, canBubble, cancelable);
    
    m_data = data;
    m_origin = origin;
    m_lastEventId = lastEventId;
    m_source = source;
    m_ports = ports;
}

// FIXME: remove this when we update the ObjC bindings (bug #28774).
MessagePort* MessageEvent::messagePort()
{
    if (!m_ports)
        return 0;
    ASSERT(m_ports->size() == 1);
    return (*m_ports)[0].get();
}

void MessageEvent::initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, MessagePort* port)
{
    OwnPtr<MessagePortArray> ports;
    if (port) {
        ports = adoptPtr(new MessagePortArray);
        ports->append(port);
    }
    initMessageEvent(type, canBubble, cancelable, data, origin, lastEventId, source, ports.release());
}

bool MessageEvent::isMessageEvent() const 
{
    return true;
}

} // namespace WebCore
