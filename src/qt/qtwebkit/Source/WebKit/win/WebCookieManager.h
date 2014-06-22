/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef WebCookieManager_h
#define WebCookieManager_h

#include "WebKit.h"

typedef struct OpaqueCFHTTPCookieStorage*  CFHTTPCookieStorageRef;

class WebCookieManager : public IWebCookieManager {
public:
    static WebCookieManager* createInstance();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void** ppvObject);

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebCookieManager
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE cookieStorage( 
        /* [retval][out] */ CFHTTPCookieStorageRef* storage);

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE setCookieStorage( 
        /* [in] */ CFHTTPCookieStorageRef storage);

private:
    WebCookieManager();
    ~WebCookieManager();

    ULONG m_refCount;
};

#endif
