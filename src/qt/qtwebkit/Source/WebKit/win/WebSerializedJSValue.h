/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef WebSerializedJSValue_h
#define WebSerializedJSValue_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>

typedef const struct OpaqueJSContext* JSContextRef;
typedef const struct OpaqueJSValue* JSValueRef;

namespace WebCore {
    class SerializedScriptValue;
}

class WebSerializedJSValue : public IWebSerializedJSValue, public IWebSerializedJSValuePrivate {
    WTF_MAKE_NONCOPYABLE(WebSerializedJSValue);
public:
    static COMPtr<WebSerializedJSValue> createInstance();

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    virtual HRESULT STDMETHODCALLTYPE serialize(JSContextRef, JSValueRef value, JSValueRef* exception);
    virtual HRESULT STDMETHODCALLTYPE deserialize(JSContextRef, JSValueRef* result);
    virtual HRESULT STDMETHODCALLTYPE setInternalRepresentation(void* internalRepresentation);
    virtual HRESULT STDMETHODCALLTYPE getInternalRepresentation(void** internalRepresentation);

private:
    WebSerializedJSValue();
    ~WebSerializedJSValue();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppvObject);

    ULONG m_refCount;
    RefPtr<WebCore::SerializedScriptValue> m_value;
};

#endif // WebSerializedJSValue_h
