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

#include "config.h"
#include "WebKitDLL.h"
#include "WebSerializedJSValue.h"

#include <WebCore/SerializedScriptValue.h>

using namespace WebCore;

WebSerializedJSValue::WebSerializedJSValue()
    : m_refCount(0)
{
    ++gClassCount;
    gClassNameCount.add("WebSerializedJSValue");
}

WebSerializedJSValue::~WebSerializedJSValue()
{
    --gClassCount;
    gClassNameCount.remove("WebSerializedJSValue");
}

COMPtr<WebSerializedJSValue> WebSerializedJSValue::createInstance()
{
    return new WebSerializedJSValue();
}

ULONG WebSerializedJSValue::AddRef()
{
    return ++m_refCount;
}

ULONG WebSerializedJSValue::Release()
{
    ULONG newRefCount = --m_refCount;
    if (!newRefCount)
        delete this;
    return newRefCount;
}

HRESULT WebSerializedJSValue::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    *ppvObject = 0;

    if (IsEqualIID(riid, __uuidof(WebSerializedJSValue)))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebSerializedJSValue*>(this);
    else if (IsEqualIID(riid, __uuidof(IWebSerializedJSValue)))
        *ppvObject = static_cast<IWebSerializedJSValue*>(this);
    else if (IsEqualIID(riid, __uuidof(IWebSerializedJSValuePrivate)))
        *ppvObject = static_cast<IWebSerializedJSValuePrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

HRESULT WebSerializedJSValue::serialize(JSContextRef sourceContext, JSValueRef value, JSValueRef* exception)
{
    ASSERT_ARG(value, value);
    ASSERT_ARG(sourceContext, sourceContext);

    if (!value || !sourceContext)
        return E_POINTER;

    m_value = SerializedScriptValue::create(sourceContext, value, exception);
        
    return S_OK;
}

HRESULT WebSerializedJSValue::deserialize(JSContextRef destinationContext, JSValueRef* outValue)
{
    if (!outValue)
        return E_POINTER;

    if (!m_value)
        *outValue = 0;
    else
        *outValue = m_value->deserialize(destinationContext, 0);

    return S_OK;
}

HRESULT WebSerializedJSValue::setInternalRepresentation(void* internalRepresentation)
{
    if (!internalRepresentation || m_value)
        return E_POINTER;

    m_value = reinterpret_cast<SerializedScriptValue*>(internalRepresentation);

    return S_OK;
}

HRESULT WebSerializedJSValue::getInternalRepresentation(void** internalRepresentation)
{
    if (!m_value)
        return E_POINTER;

    *internalRepresentation = m_value.get();
    return S_OK;
}

