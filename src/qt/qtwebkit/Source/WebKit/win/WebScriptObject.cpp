/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "WebScriptObject.h"

#include "WebKitDLL.h"

#include <wtf/Assertions.h>

// WebScriptObject ------------------------------------------------------------

WebScriptObject::WebScriptObject()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebScriptObject");
}

WebScriptObject::~WebScriptObject()
{
    gClassCount--;
    gClassNameCount.remove("WebScriptObject");
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptObject::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebScriptObject*>(this);
    else if (IsEqualGUID(riid, IID_IWebScriptObject))
        *ppvObject = static_cast<IWebScriptObject*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebScriptObject::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebScriptObject::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// WebScriptObject ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptObject::throwException( 
    /* [in] */ BSTR /*exceptionMessage*/,
    /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::callWebScriptMethod( 
    /* [in] */ BSTR /*name*/,
    /* [size_is][in] */ const VARIANT /*args*/[  ],
    /* [in] */ int /*cArgs*/,
    /* [retval][out] */ VARIANT* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::evaluateWebScript( 
    /* [in] */ BSTR /*script*/,
    /* [retval][out] */ VARIANT* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::removeWebScriptKey( 
    /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::stringRepresentation( 
    /* [retval][out] */ BSTR* /*stringRepresentation*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::webScriptValueAtIndex( 
    /* [in] */ unsigned int /*index*/,
    /* [retval][out] */ VARIANT* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::setWebScriptValueAtIndex( 
    /* [in] */ unsigned int /*index*/,
    /* [in] */ VARIANT /*val*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebScriptObject::setException( 
    /* [in] */ BSTR /*description*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
