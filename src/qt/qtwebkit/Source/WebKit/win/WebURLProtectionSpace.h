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

#ifndef WebURLProtectionSpace_h
#define WebURLProtectionSpace_h

#include "WebKit.h"
#include <WebCore/ProtectionSpace.h>

class WebURLProtectionSpace : public IWebURLProtectionSpace
{
public:
    static WebURLProtectionSpace* createInstance();
    static WebURLProtectionSpace* createInstance(const WebCore::ProtectionSpace&);
private:
    WebURLProtectionSpace(const WebCore::ProtectionSpace&);
    ~WebURLProtectionSpace();
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebURLProtectionSpace
    virtual HRESULT STDMETHODCALLTYPE authenticationMethod(
        /* [out, retval] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE host(
        /* [out, retval] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE initWithHost(
        /* [in] */ BSTR host, 
        /* [in] */ int port, 
        /* [in] */ BSTR protocol, 
        /* [in] */ BSTR realm, 
        /* [in] */ BSTR authenticationMethod);

    virtual HRESULT STDMETHODCALLTYPE initWithProxyHost(
        /* [in] */ BSTR host, 
        /* [in] */ int port, 
        /* [in] */ BSTR proxyType, 
        /* [in] */ BSTR realm, 
        /* [in] */ BSTR authenticationMethod);

    virtual HRESULT STDMETHODCALLTYPE isProxy(
        /* [out, retval] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE port(
        /* [out, retval] */ int* result);

    virtual HRESULT STDMETHODCALLTYPE protocol(
        /* [out, retval] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE proxyType(
        /* [out, retval] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE realm(
        /* [out, retval] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE receivesCredentialSecurely(
        /* [out, retval] */ BOOL* result);

    // WebURLProtectionSpace
    const WebCore::ProtectionSpace& protectionSpace() const;

protected:
    ULONG m_refCount;

    WebCore::ProtectionSpace m_protectionSpace;
};


#endif
