/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef MessageEvent_h
#define MessageEvent_h

#include "Blob.h"
#include "DOMWindow.h"
#include "Event.h"
#include "MessagePort.h"
#include "ScriptValue.h"
#include "SerializedScriptValue.h"
#include <wtf/ArrayBuffer.h>

namespace WebCore {

class DOMWindow;

struct MessageEventInit : public EventInit {
    MessageEventInit();

    ScriptValue data;
    String origin;
    String lastEventId;
    RefPtr<DOMWindow> source;
    MessagePortArray ports;
};

class MessageEvent : public Event {
public:
    static PassRefPtr<MessageEvent> create()
    {
        return adoptRef(new MessageEvent);
    }
    static PassRefPtr<MessageEvent> create(PassOwnPtr<MessagePortArray> ports, const ScriptValue& data = ScriptValue(), const String& origin = "", const String& lastEventId = "", PassRefPtr<DOMWindow> source = 0)
    {
        return adoptRef(new MessageEvent(data, origin, lastEventId, source, ports));
    }
    static PassRefPtr<MessageEvent> create(PassOwnPtr<MessagePortArray> ports, PassRefPtr<SerializedScriptValue> data, const String& origin = "", const String& lastEventId = "", PassRefPtr<DOMWindow> source = 0)
    {
        return adoptRef(new MessageEvent(data, origin, lastEventId, source, ports));
    }
    static PassRefPtr<MessageEvent> create(const String& data, const String& origin = "")
    {
        return adoptRef(new MessageEvent(data, origin));
    }
    static PassRefPtr<MessageEvent> create(PassRefPtr<Blob> data, const String& origin = "")
    {
        return adoptRef(new MessageEvent(data, origin));
    }
    static PassRefPtr<MessageEvent> create(PassRefPtr<ArrayBuffer> data, const String& origin = "")
    {
        return adoptRef(new MessageEvent(data, origin));
    }
    static PassRefPtr<MessageEvent> create(const AtomicString& type, const MessageEventInit& initializer)
    {
        return adoptRef(new MessageEvent(type, initializer));
    }
    virtual ~MessageEvent();

    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, const ScriptValue& data, const String& origin, const String& lastEventId, DOMWindow* source, PassOwnPtr<MessagePortArray>);
    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, PassOwnPtr<MessagePortArray>);

    const String& origin() const { return m_origin; }
    const String& lastEventId() const { return m_lastEventId; }
    DOMWindow* source() const { return m_source.get(); }
    MessagePortArray ports() const { return m_ports ? *m_ports : MessagePortArray(); }

    // FIXME: Remove this when we have custom ObjC binding support.
    SerializedScriptValue* data() const;

    // Needed for Objective-C bindings (see bug 28774).
    MessagePort* messagePort();
    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, MessagePort*);

    virtual const AtomicString& interfaceName() const;

    enum DataType {
        DataTypeScriptValue,
        DataTypeSerializedScriptValue,
        DataTypeString,
        DataTypeBlob,
        DataTypeArrayBuffer
    };
    DataType dataType() const { return m_dataType; }
    const ScriptValue& dataAsScriptValue() const { ASSERT(m_dataType == DataTypeScriptValue); return m_dataAsScriptValue; }
    PassRefPtr<SerializedScriptValue> dataAsSerializedScriptValue() const { ASSERT(m_dataType == DataTypeSerializedScriptValue); return m_dataAsSerializedScriptValue; }
    String dataAsString() const { ASSERT(m_dataType == DataTypeString); return m_dataAsString; }
    Blob* dataAsBlob() const { ASSERT(m_dataType == DataTypeBlob); return m_dataAsBlob.get(); }
    ArrayBuffer* dataAsArrayBuffer() const { ASSERT(m_dataType == DataTypeArrayBuffer); return m_dataAsArrayBuffer.get(); }

private:
    MessageEvent();
    MessageEvent(const AtomicString&, const MessageEventInit&);
    MessageEvent(const ScriptValue& data, const String& origin, const String& lastEventId, PassRefPtr<DOMWindow> source, PassOwnPtr<MessagePortArray>);
    MessageEvent(PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, PassRefPtr<DOMWindow> source, PassOwnPtr<MessagePortArray>);

    explicit MessageEvent(const String& data, const String& origin);
    explicit MessageEvent(PassRefPtr<Blob> data, const String& origin);
    explicit MessageEvent(PassRefPtr<ArrayBuffer> data, const String& origin);

    DataType m_dataType;
    ScriptValue m_dataAsScriptValue;
    RefPtr<SerializedScriptValue> m_dataAsSerializedScriptValue;
    String m_dataAsString;
    RefPtr<Blob> m_dataAsBlob;
    RefPtr<ArrayBuffer> m_dataAsArrayBuffer;
    String m_origin;
    String m_lastEventId;
    RefPtr<DOMWindow> m_source;
    OwnPtr<MessagePortArray> m_ports;
};

} // namespace WebCore

#endif // MessageEvent_h
