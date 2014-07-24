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

#ifndef WebError_h
#define WebError_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>
#include <WebCore/ResourceError.h>
#include <wtf/RetainPtr.h>

class WebError : public IWebError, IWebErrorPrivate {
public:
    static WebError* createInstance(const WebCore::ResourceError&, IPropertyBag* userInfo = 0);
    static WebError* createInstance();
protected:
    WebError(const WebCore::ResourceError&, IPropertyBag* userInfo);
    ~WebError();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebError
    virtual HRESULT STDMETHODCALLTYPE init( 
        /* [in] */ BSTR domain,
        /* [in] */ int code,
        /* [in] */ BSTR url);

    virtual HRESULT STDMETHODCALLTYPE code( 
        /* [retval][out] */ int *result);
        
    virtual HRESULT STDMETHODCALLTYPE domain( 
        /* [retval][out] */ BSTR *result);
        
    virtual HRESULT STDMETHODCALLTYPE localizedDescription( 
        /* [retval][out] */ BSTR *result);
        
    virtual HRESULT STDMETHODCALLTYPE localizedFailureReason( 
        /* [retval][out] */ BSTR *result);
        
    virtual HRESULT STDMETHODCALLTYPE localizedRecoveryOptions( 
        /* [retval][out] */ IEnumVARIANT **result);
        
    virtual HRESULT STDMETHODCALLTYPE localizedRecoverySuggestion( 
        /* [retval][out] */ BSTR *result);
       
    virtual HRESULT STDMETHODCALLTYPE recoverAttempter( 
        /* [retval][out] */ IUnknown **result);
        
    virtual HRESULT STDMETHODCALLTYPE userInfo( 
        /* [retval][out] */ IPropertyBag **result);

    virtual HRESULT STDMETHODCALLTYPE failingURL( 
        /* [retval][out] */ BSTR *result);

    virtual HRESULT STDMETHODCALLTYPE isPolicyChangeError( 
        /* [retval][out] */ BOOL *result);

    // IWebErrorPrivate
    virtual HRESULT STDMETHODCALLTYPE sslPeerCertificate( 
        /* [retval][out] */ OLE_HANDLE *result);

    const WebCore::ResourceError& resourceError() const;

private:
    ULONG m_refCount;
    COMPtr<IPropertyBag> m_userInfo;
    RetainPtr<CFDictionaryRef> m_cfErrorUserInfoDict;
    WebCore::ResourceError m_error;
};

#endif
