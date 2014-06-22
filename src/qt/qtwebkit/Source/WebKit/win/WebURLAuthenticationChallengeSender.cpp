/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#include <initguid.h>
#include "WebURLAuthenticationChallengeSender.h"

#include "WebKit.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLCredential.h"
#include <WebCore/AuthenticationClient.h>
#include <WebCore/COMPtr.h>

using namespace WebCore;

// WebURLAuthenticationChallengeSender ----------------------------------------------------------------

WebURLAuthenticationChallengeSender::WebURLAuthenticationChallengeSender(PassRefPtr<AuthenticationClient> client)
    : m_refCount(0)
    , m_client(client)
{
    ASSERT(m_client);
    gClassCount++;
    gClassNameCount.add("WebURLAuthenticationChallengeSender");
}

WebURLAuthenticationChallengeSender::~WebURLAuthenticationChallengeSender()
{
    gClassCount--;
    gClassNameCount.remove("WebURLAuthenticationChallengeSender");
}

WebURLAuthenticationChallengeSender* WebURLAuthenticationChallengeSender::createInstance(PassRefPtr<WebCore::AuthenticationClient> client)
{
    WebURLAuthenticationChallengeSender* instance = new WebURLAuthenticationChallengeSender(client);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, __uuidof(this)))
        *ppvObject = static_cast<WebURLAuthenticationChallengeSender*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLAuthenticationChallengeSender))
        *ppvObject = static_cast<IWebURLAuthenticationChallengeSender*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// WebURLAuthenticationChallengeSender ----------------------------------------------------------------

AuthenticationClient* WebURLAuthenticationChallengeSender::authenticationClient() const
{
    return m_client.get();
}

