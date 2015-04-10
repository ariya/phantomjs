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
#include "WebError.h"
#include "WebKit.h"

#include <WebCore/BString.h>

#if USE(CFNETWORK)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

using namespace WebCore;

// WebError ---------------------------------------------------------------------

WebError::WebError(const ResourceError& error, IPropertyBag* userInfo)
    : m_refCount(0)
    , m_error(error)
    , m_userInfo(userInfo)
{
    gClassCount++;
    gClassNameCount.add("WebError");
}

WebError::~WebError()
{
    gClassCount--;
    gClassNameCount.remove("WebError");
}

WebError* WebError::createInstance(const ResourceError& error, IPropertyBag* userInfo)
{
    WebError* instance = new WebError(error, userInfo);
    instance->AddRef();
    return instance;
}

WebError* WebError::createInstance()
{
    return createInstance(ResourceError());
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebError::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebError*>(this);
    else if (IsEqualGUID(riid, CLSID_WebError))
        *ppvObject = static_cast<WebError*>(this);
    else if (IsEqualGUID(riid, IID_IWebError))
        *ppvObject = static_cast<IWebError*>(this);
    else if (IsEqualGUID(riid, IID_IWebErrorPrivate))
        *ppvObject = static_cast<IWebErrorPrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebError::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebError::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebError ------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebError::init( 
    /* [in] */ BSTR domain,
    /* [in] */ int code,
    /* [in] */ BSTR url)
{
    m_error = ResourceError(String(domain, SysStringLen(domain)), code, String(url, SysStringLen(url)), String());
    return S_OK;
}
  
HRESULT STDMETHODCALLTYPE WebError::code( 
    /* [retval][out] */ int* result)
{
    *result = m_error.errorCode();
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE WebError::domain( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.domain()).release();
    return S_OK;
}
               
HRESULT STDMETHODCALLTYPE WebError::localizedDescription( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.localizedDescription()).release();

#if USE(CFNETWORK)
    if (!*result) {
        if (int code = m_error.errorCode())
            *result = BString(wkCFNetworkErrorGetLocalizedDescription(code)).release();
    }
#endif

    return S_OK;
}

        
HRESULT STDMETHODCALLTYPE WebError::localizedFailureReason( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::localizedRecoveryOptions( 
    /* [retval][out] */ IEnumVARIANT** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::localizedRecoverySuggestion( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
       
HRESULT STDMETHODCALLTYPE WebError::recoverAttempter( 
    /* [retval][out] */ IUnknown** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::userInfo( 
    /* [retval][out] */ IPropertyBag** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;

    if (!m_userInfo)
        return E_FAIL;

    return m_userInfo.copyRefTo(result);
}

HRESULT STDMETHODCALLTYPE WebError::failingURL( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.failingURL()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebError::isPolicyChangeError( 
    /* [retval][out] */ BOOL *result)
{
    if (!result)
        return E_POINTER;

    *result = m_error.domain() == String(WebKitErrorDomain)
        && m_error.errorCode() == WebKitErrorFrameLoadInterruptedByPolicyChange;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebError::sslPeerCertificate( 
    /* [retval][out] */ OLE_HANDLE* result)
{
    if (!result)
        return E_POINTER;
    *result = 0;

#if USE(CFNETWORK)
    if (!m_cfErrorUserInfoDict) {
        // copy userinfo from CFErrorRef
        CFErrorRef cfError = m_error;
        m_cfErrorUserInfoDict = adoptCF(CFErrorCopyUserInfo(cfError));
    }

    if (!m_cfErrorUserInfoDict)
        return E_FAIL;

    void* data = wkGetSSLPeerCertificateDataBytePtr(m_cfErrorUserInfoDict.get());
    if (!data)
        return E_FAIL;
    *result = (OLE_HANDLE)(ULONG64)data;
#endif
    return *result ? S_OK : E_FAIL;
}

const ResourceError& WebError::resourceError() const
{
    return m_error;
}
