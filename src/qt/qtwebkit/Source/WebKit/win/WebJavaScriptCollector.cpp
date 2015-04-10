/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
#include "WebKitDLL.h"
#include "WebJavaScriptCollector.h"

#include <JavaScriptCore/Heap.h>
#include <JavaScriptCore/VM.h>
#include <WebCore/GCController.h>
#include <WebCore/JSDOMWindow.h>
#include <runtime/JSLock.h>

using namespace JSC;
using namespace WebCore;

// WebJavaScriptCollector ---------------------------------------------------------------------------

WebJavaScriptCollector::WebJavaScriptCollector()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebJavaScriptCollector");
}

WebJavaScriptCollector::~WebJavaScriptCollector()
{
    gClassCount--;
    gClassNameCount.remove("WebJavaScriptCollector");
}

WebJavaScriptCollector* WebJavaScriptCollector::createInstance()
{
    WebJavaScriptCollector* instance = new WebJavaScriptCollector();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebJavaScriptCollector::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebJavaScriptCollector*>(this);
    else if (IsEqualGUID(riid, IID_IWebJavaScriptCollector))
        *ppvObject = static_cast<IWebJavaScriptCollector*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebJavaScriptCollector::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebJavaScriptCollector::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebJavaScriptCollector ------------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebJavaScriptCollector::collect()
{
    gcController().garbageCollectNow();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebJavaScriptCollector::collectOnAlternateThread( 
    /* [in] */ BOOL waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(!!waitUntilDone);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebJavaScriptCollector::objectCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    JSLockHolder lock(JSDOMWindow::commonVM());
    *count = (UINT)JSDOMWindow::commonVM()->heap.objectCount();
    return S_OK;
}
