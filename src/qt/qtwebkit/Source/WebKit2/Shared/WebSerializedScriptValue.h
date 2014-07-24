/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebSerializedScriptValue_h
#define WebSerializedScriptValue_h

#include "APIObject.h"

#include "DataReference.h"
#include <WebCore/SerializedScriptValue.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class WebSerializedScriptValue : public TypedAPIObject<APIObject::TypeSerializedScriptValue> {
public:
    static PassRefPtr<WebSerializedScriptValue> create(PassRefPtr<WebCore::SerializedScriptValue> serializedValue)
    {
        return adoptRef(new WebSerializedScriptValue(serializedValue));
    }
    
    static PassRefPtr<WebSerializedScriptValue> create(JSContextRef context, JSValueRef value, JSValueRef* exception)
    {
        RefPtr<WebCore::SerializedScriptValue> serializedValue = WebCore::SerializedScriptValue::create(context, value, exception);
        if (!serializedValue)
            return 0;
        return adoptRef(new WebSerializedScriptValue(serializedValue.get()));
    }
    
    static PassRefPtr<WebSerializedScriptValue> adopt(Vector<uint8_t>& buffer)
    {
        return adoptRef(new WebSerializedScriptValue(WebCore::SerializedScriptValue::adopt(buffer)));
    }
    
    JSValueRef deserialize(JSContextRef context, JSValueRef* exception)
    {
        return m_serializedScriptValue->deserialize(context, exception);
    }

    CoreIPC::DataReference dataReference() const { return m_serializedScriptValue->data(); }

    void* internalRepresentation() { return m_serializedScriptValue.get(); }

private:
    explicit WebSerializedScriptValue(PassRefPtr<WebCore::SerializedScriptValue> serializedScriptValue)
        : m_serializedScriptValue(serializedScriptValue)
    {
    }

    RefPtr<WebCore::SerializedScriptValue> m_serializedScriptValue;
};
    
}

#endif // WebSerializedScriptValue_h
