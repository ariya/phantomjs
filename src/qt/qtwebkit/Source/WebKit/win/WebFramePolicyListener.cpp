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
#include "WebFramePolicyListener.h"

#include "WebFrame.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderClient.h>

using namespace WebCore;

// WebFramePolicyListener ----------------------------------------------------------------

WebFramePolicyListener::WebFramePolicyListener(PassRefPtr<Frame> frame)
    : m_refCount(0)
    , m_frame(frame)
{
    gClassCount++;
    gClassNameCount.add("WebFramePolicyListener");
}

WebFramePolicyListener::~WebFramePolicyListener()
{
    gClassCount--;
    gClassNameCount.remove("WebFramePolicyListener");
}

WebFramePolicyListener* WebFramePolicyListener::createInstance(PassRefPtr<Frame> frame)
{
    WebFramePolicyListener* instance = new WebFramePolicyListener(frame);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebFramePolicyListener::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebPolicyDecisionListener*>(this);
    else if (IsEqualGUID(riid, IID_IWebPolicyDecisionListener))
        *ppvObject = static_cast<IWebPolicyDecisionListener*>(this);
    else if (IsEqualGUID(riid, IID_IWebFormSubmissionListener))
        *ppvObject = static_cast<IWebFormSubmissionListener*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebFramePolicyListener::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebFramePolicyListener::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebPolicyDecisionListener ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebFramePolicyListener::use(void)
{
    receivedPolicyDecision(PolicyUse);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebFramePolicyListener::download(void)
{
    receivedPolicyDecision(PolicyDownload);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebFramePolicyListener::ignore(void)
{
    receivedPolicyDecision(PolicyIgnore);
    return S_OK;
}

// IWebFormSubmissionListener ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebFramePolicyListener::continueSubmit(void)
{
    receivedPolicyDecision(PolicyUse);
    return S_OK;
}

// WebFramePolicyListener ----------------------------------------------------------------
void WebFramePolicyListener::receivedPolicyDecision(PolicyAction action)
{
    RefPtr<Frame> frame = m_frame.release();
    if (frame)
        static_cast<WebFrame*>(frame->loader()->client())->receivedPolicyDecision(action);
}

void WebFramePolicyListener::invalidate()
{
    m_frame = 0;
}

