/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebTextRenderer.h"

#include "WebKitDLL.h"

WebTextRenderer* WebTextRenderer::createInstance()
{
    WebTextRenderer* instance = new WebTextRenderer;
    instance->AddRef();
    return instance;
}

WebTextRenderer::WebTextRenderer()
    : m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebTextRenderer");
}

WebTextRenderer::~WebTextRenderer()
{
    gClassCount--;
    gClassNameCount.remove("WebTextRenderer");
}

HRESULT STDMETHODCALLTYPE WebTextRenderer::QueryInterface(const IID &riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebTextRenderer*>(this);
    else if (IsEqualGUID(riid, IID_IWebTextRenderer))
        *ppvObject = static_cast<IWebTextRenderer*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebTextRenderer::AddRef()
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebTextRenderer::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT STDMETHODCALLTYPE WebTextRenderer::registerPrivateFont(
    /* [in] */ LPCOLESTR fontFilePath)
{
    if (!AddFontResourceEx(fontFilePath, FR_PRIVATE, 0))
        return E_FAIL;

    return S_OK;
}
