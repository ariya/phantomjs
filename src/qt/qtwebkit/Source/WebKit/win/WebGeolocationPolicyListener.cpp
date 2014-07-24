/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
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
#include "WebGeolocationPolicyListener.h"

#include <WebCore/Geolocation.h>

using namespace WebCore;

// WebGeolocationPolicyListener ----------------------------------------------------------------

COMPtr<WebGeolocationPolicyListener> WebGeolocationPolicyListener::createInstance(PassRefPtr<Geolocation> geolocation)
{
    return new WebGeolocationPolicyListener(geolocation);
}

WebGeolocationPolicyListener::WebGeolocationPolicyListener(PassRefPtr<Geolocation> geolocation)
    : m_refCount(0)
    , m_geolocation(geolocation)
{
    gClassCount++;
    gClassNameCount.add("WebGeolocationPolicyListener");
}

WebGeolocationPolicyListener::~WebGeolocationPolicyListener()
{
    gClassCount--;
    gClassNameCount.remove("WebGeolocationPolicyListener");
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebGeolocationPolicyListener::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, __uuidof(IUnknown)))
        *ppvObject = static_cast<IWebGeolocationPolicyListener*>(this);
    else if (IsEqualIID(riid, __uuidof(IWebGeolocationPolicyListener)))
        *ppvObject = static_cast<IWebGeolocationPolicyListener*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebGeolocationPolicyListener::AddRef()
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebGeolocationPolicyListener::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

// IWebPolicyDecisionListener ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebGeolocationPolicyListener::allow()
{
    m_geolocation->setIsAllowed(true);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebGeolocationPolicyListener::deny()
{
    m_geolocation->setIsAllowed(false);
    return S_OK;
}
