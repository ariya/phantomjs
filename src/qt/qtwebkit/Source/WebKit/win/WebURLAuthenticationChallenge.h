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

#ifndef WebURLAuthenticationChallenge_h
#define WebURLAuthenticationChallenge_h

#include "WebKit.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/COMPtr.h>

class DECLSPEC_UUID("FD3B2381-0BB6-4B59-AF09-0E599C8901CF") WebURLAuthenticationChallenge : public IWebURLAuthenticationChallenge {
public:
    static WebURLAuthenticationChallenge* createInstance(const WebCore::AuthenticationChallenge&);
    static WebURLAuthenticationChallenge* createInstance(const WebCore::AuthenticationChallenge&, IWebURLAuthenticationChallengeSender*);
private:
    WebURLAuthenticationChallenge(const WebCore::AuthenticationChallenge&, IWebURLAuthenticationChallengeSender*);
    ~WebURLAuthenticationChallenge();
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebURLAuthenticationChallenge
    virtual HRESULT STDMETHODCALLTYPE initWithProtectionSpace(
        /* [in] */ IWebURLProtectionSpace* space, 
        /* [in] */ IWebURLCredential* proposedCredential, 
        /* [in] */ int previousFailureCount, 
        /* [in] */ IWebURLResponse* failureResponse, 
        /* [in] */ IWebError* error, 
        /* [in] */ IWebURLAuthenticationChallengeSender* sender);

    virtual HRESULT STDMETHODCALLTYPE initWithAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge, 
        /* [in] */ IWebURLAuthenticationChallengeSender* sender);

    virtual HRESULT STDMETHODCALLTYPE error(
        /* [out, retval] */ IWebError** result);

    virtual HRESULT STDMETHODCALLTYPE failureResponse(
        /* [out, retval] */ IWebURLResponse** result);

    virtual HRESULT STDMETHODCALLTYPE previousFailureCount(
        /* [out, retval] */ UINT* result);

    virtual HRESULT STDMETHODCALLTYPE proposedCredential(
        /* [out, retval] */ IWebURLCredential** result);

    virtual HRESULT STDMETHODCALLTYPE protectionSpace(
        /* [out, retval] */ IWebURLProtectionSpace** result);

    virtual HRESULT STDMETHODCALLTYPE sender(
        /* [out, retval] */ IWebURLAuthenticationChallengeSender** sender);

    // WebURLAuthenticationChallenge
    const WebCore::AuthenticationChallenge& authenticationChallenge() const;

protected:
    ULONG m_refCount;

    WebCore::AuthenticationChallenge m_authenticationChallenge;
    COMPtr<IWebURLAuthenticationChallengeSender> m_sender;
};


#endif
