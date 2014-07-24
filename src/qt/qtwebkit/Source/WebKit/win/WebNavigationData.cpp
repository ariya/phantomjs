/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
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
#include "WebNavigationData.h"

using namespace WebCore;

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebNavigationData::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebNavigationData*>(this);
    else if (IsEqualGUID(riid, IID_IWebNavigationData))
        *ppvObject = static_cast<IWebNavigationData*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebNavigationData::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebNavigationData::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// WebNavigationData -------------------------------------------------------------------

WebNavigationData::WebNavigationData(const String& url, const String& title, IWebURLRequest* request, IWebURLResponse* response, bool hasSubstituteData, const String& clientRedirectSource)
    : m_refCount(0)
    , m_url(url)
    , m_title(title)
    , m_request(request)
    , m_response(response)
    , m_hasSubstituteData(hasSubstituteData)
    , m_clientRedirectSource(clientRedirectSource)

{
    gClassCount++;
    gClassNameCount.add("WebNavigationData");
}

WebNavigationData::~WebNavigationData()
{
    gClassCount--;
    gClassNameCount.remove("WebNavigationData");
}

WebNavigationData* WebNavigationData::createInstance(const String& url, const String& title, IWebURLRequest* request, IWebURLResponse* response, bool hasSubstituteData, const String& clientRedirectSource)
{
    WebNavigationData* instance = new WebNavigationData(url, title, request, response, hasSubstituteData, clientRedirectSource);
    instance->AddRef();
    return instance;
}

// IWebNavigationData -------------------------------------------------------------------

HRESULT WebNavigationData::url(BSTR* url)
{
    if (!url)
        return E_POINTER;
    *url = BString(m_url).release();
    return S_OK;
}

HRESULT WebNavigationData::title(BSTR* title)
{
    if (!title)
        return E_POINTER;
    *title = BString(m_title).release();
    return S_OK;
}

HRESULT WebNavigationData::originalRequest(IWebURLRequest** request)
{
    if (!request)
        return E_POINTER;
    *request = m_request.get();
    m_request->AddRef();
    return S_OK;
}

HRESULT WebNavigationData::response(IWebURLResponse** response)
{
    if (!response)
        return E_POINTER;
    *response = m_response.get();
    m_response->AddRef();
    return S_OK;
}

HRESULT WebNavigationData::hasSubstituteData(BOOL* hasSubstituteData)
{
    if (!hasSubstituteData)
        return E_POINTER;
    *hasSubstituteData = m_hasSubstituteData;
    return S_OK;
}

HRESULT WebNavigationData::clientRedirectSource(BSTR* clientRedirectSource)
{
    if (!clientRedirectSource)
        return E_POINTER;

    *clientRedirectSource = BString(m_clientRedirectSource).release();
    return S_OK;
}
