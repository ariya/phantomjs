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

#ifndef WebURLResponse_H
#define WebURLResponse_H

#include "WebKit.h"
#include <WebCore/ResourceResponse.h>

class WebURLResponse : public IWebHTTPURLResponse, IWebURLResponsePrivate
{
public:
    static WebURLResponse* createInstance();
    static WebURLResponse* createInstance(const WebCore::ResourceResponse& response);
protected:
    WebURLResponse();
    ~WebURLResponse();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebURLResponse
    virtual HRESULT STDMETHODCALLTYPE expectedContentLength( 
        /* [retval][out] */ long long *result);
    
    virtual HRESULT STDMETHODCALLTYPE initWithURL( 
        /* [in] */ BSTR url,
        /* [in] */ BSTR mimeType,
        /* [in] */ int expectedContentLength,
        /* [in] */ BSTR textEncodingName);
    
    virtual HRESULT STDMETHODCALLTYPE MIMEType( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE suggestedFilename( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE textEncodingName( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE URL( 
        /* [retval][out] */ BSTR *result);

    // IWebHTTPURLResponse
    virtual HRESULT STDMETHODCALLTYPE allHeaderFields( 
        /* [retval][out] */ IPropertyBag **headerFields);
    
    virtual HRESULT STDMETHODCALLTYPE localizedStringForStatusCode( 
        /* [in] */ int statusCode,
        /* [retval][out] */ BSTR *statusString);
    
    virtual HRESULT STDMETHODCALLTYPE statusCode( 
        /* [retval][out] */ int *statusCode);

    virtual HRESULT STDMETHODCALLTYPE isAttachment( 
        /* [retval][out] */ BOOL *attachment);

    // IWebURLResponsePrivate
    virtual HRESULT STDMETHODCALLTYPE sslPeerCertificate( 
        /* [retval][out] */ OLE_HANDLE *result);
    
    const WebCore::ResourceResponse& resourceResponse() const;

protected:
    HRESULT suggestedFileExtension(BSTR* result);

#if USE(CFNETWORK)
    CFDictionaryRef certificateDictionary() const;
#endif

protected:
    ULONG m_refCount;
    WebCore::ResourceResponse m_response;

#if USE(CFNETWORK)
    mutable RetainPtr<CFDictionaryRef> m_SSLCertificateInfo;    // this ensures certificate contexts are valid for the lifetime of this WebURLResponse.
#endif
};

#endif
