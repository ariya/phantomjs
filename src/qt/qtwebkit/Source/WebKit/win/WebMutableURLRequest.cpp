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

#include "config.h"
#include "WebKitDLL.h"
#include "WebMutableURLRequest.h"

#include "WebKit.h"
#include "MarshallingHelpers.h"
#include "WebKit.h"
#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <WebCore/FormData.h>
#include <WebCore/ResourceHandle.h>
#include <wtf/text/CString.h>
#include <wtf/RetainPtr.h>

#if USE(CF)
#include <WebCore/CertificateCFWin.h>
#endif

#if USE(CFNETWORK)
#include <CFNetwork/CFURLRequestPriv.h>
#endif

using namespace WebCore;

// IWebURLRequest ----------------------------------------------------------------

WebMutableURLRequest::WebMutableURLRequest(bool isMutable)
    : m_refCount(0)
    , m_isMutable(isMutable)
{
    gClassCount++;
    gClassNameCount.add("WebMutableURLRequest");
}

WebMutableURLRequest* WebMutableURLRequest::createInstance()
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    instance->AddRef();
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createInstance(IWebMutableURLRequest* req)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    instance->AddRef();
    instance->m_request = static_cast<WebMutableURLRequest*>(req)->m_request;
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createInstance(const ResourceRequest& request)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    instance->AddRef();
    instance->m_request = request;
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createImmutableInstance()
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(false);
    instance->AddRef();
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createImmutableInstance(const ResourceRequest& request)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(false);
    instance->AddRef();
    instance->m_request = request;
    return instance;
}

WebMutableURLRequest::~WebMutableURLRequest()
{
    gClassCount--;
    gClassNameCount.remove("WebMutableURLRequest");
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, CLSID_WebMutableURLRequest))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebURLRequest*>(this);
    else if (IsEqualGUID(riid, IID_IWebMutableURLRequest) && m_isMutable)
        *ppvObject = static_cast<IWebMutableURLRequest*>(this);
    else if (IsEqualGUID(riid, __uuidof(IWebMutableURLRequestPrivate)) && m_isMutable)
        *ppvObject = static_cast<IWebMutableURLRequestPrivate*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLRequest))
        *ppvObject = static_cast<IWebURLRequest*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebMutableURLRequest::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebMutableURLRequest::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebURLRequest --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::requestWithURL( 
    /* [in] */ BSTR /*theURL*/,
    /* [optional][in] */ WebURLRequestCachePolicy /*cachePolicy*/,
    /* [optional][in] */ double /*timeoutInterval*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::allHTTPHeaderFields( 
    /* [retval][out] */ IPropertyBag** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::cachePolicy( 
    /* [retval][out] */ WebURLRequestCachePolicy* result)
{
    *result = kit(m_request.cachePolicy());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::HTTPBody( 
    /* [retval][out] */ IStream** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::HTTPBodyStream( 
    /* [retval][out] */ IStream** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::HTTPMethod( 
    /* [retval][out] */ BSTR* result)
{
    BString httpMethod = BString(m_request.httpMethod());
    *result = httpMethod.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::HTTPShouldHandleCookies( 
    /* [retval][out] */ BOOL* result)
{
    bool shouldHandleCookies = m_request.allowCookies();

    *result = shouldHandleCookies ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::initWithURL( 
    /* [in] */ BSTR url,
    /* [optional][in] */ WebURLRequestCachePolicy cachePolicy,
    /* [optional][in] */ double timeoutInterval)
{
    m_request.setURL(MarshallingHelpers::BSTRToKURL(url));
    m_request.setCachePolicy(core(cachePolicy));
    m_request.setTimeoutInterval(timeoutInterval);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::mainDocumentURL( 
    /* [retval][out] */ BSTR* result)
{
    *result = MarshallingHelpers::KURLToBSTR(m_request.firstPartyForCookies());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::timeoutInterval( 
    /* [retval][out] */ double* result)
{
    *result = m_request.timeoutInterval();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::URL( 
    /* [retval][out] */ BSTR* result)
{
    *result = MarshallingHelpers::KURLToBSTR(m_request.url());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::valueForHTTPHeaderField( 
    /* [in] */ BSTR field,
    /* [retval][out] */ BSTR* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = BString(m_request.httpHeaderField(String(field, SysStringLen(field)))).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::isEmpty(
    /* [retval][out] */ BOOL* result)
{
    *result = m_request.isEmpty();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::isEqual(
        /* [in] */ IWebURLRequest* other,
        /* [out, retval] */ BOOL* result)
{
    COMPtr<WebMutableURLRequest> requestImpl(Query, other);

    if (!requestImpl) {
        *result = FALSE;
        return S_OK;
    }

    *result = m_request == requestImpl->resourceRequest();
    return S_OK;
}


// IWebMutableURLRequest --------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::addValue( 
    /* [in] */ BSTR value,
    /* [in] */ BSTR field)
{
    m_request.addHTTPHeaderField(WTF::AtomicString(value, SysStringLen(value)), String(field, SysStringLen(field)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setAllHTTPHeaderFields( 
    /* [in] */ IPropertyBag* /*headerFields*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setCachePolicy( 
    /* [in] */ WebURLRequestCachePolicy policy)
{
    m_request.setCachePolicy(core(policy));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setHTTPBody( 
    /* [in] */ IStream* /*data*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setHTTPBodyStream( 
    /* [in] */ IStream* /*data*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setHTTPMethod( 
    /* [in] */ BSTR method)
{
    m_request.setHTTPMethod(String(method));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setHTTPShouldHandleCookies( 
    /* [in] */ BOOL handleCookies)
{
    m_request.setAllowCookies(handleCookies);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setMainDocumentURL( 
    /* [in] */ BSTR theURL)
{
    m_request.setFirstPartyForCookies(MarshallingHelpers::BSTRToKURL(theURL));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setTimeoutInterval( 
    /* [in] */ double timeoutInterval)
{
    m_request.setTimeoutInterval(timeoutInterval);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setURL( 
    /* [in] */ BSTR url)
{
    m_request.setURL(MarshallingHelpers::BSTRToKURL(url));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setValue( 
    /* [in] */ BSTR value,
    /* [in] */ BSTR field)
{
    String valueString(value, SysStringLen(value));
    String fieldString(field, SysStringLen(field));
    m_request.setHTTPHeaderField(fieldString, valueString);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setAllowsAnyHTTPSCertificate(void)
{
    ResourceHandle::setHostAllowsAnyHTTPSCertificate(m_request.url().host());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::setClientCertificate(
    /* [in] */ OLE_HANDLE cert)
{
    if (!cert)
        return E_POINTER;

    PCCERT_CONTEXT certContext = reinterpret_cast<PCCERT_CONTEXT>((ULONG64)cert);
    RetainPtr<CFDataRef> certData = WebCore::copyCertificateToData(certContext);
    ResourceHandle::setClientCertificate(m_request.url().host(), certData.get());
    return S_OK;
}

CFURLRequestRef STDMETHODCALLTYPE WebMutableURLRequest::cfRequest()
{
    return m_request.cfURLRequest(UpdateHTTPBody);
}

HRESULT STDMETHODCALLTYPE WebMutableURLRequest::mutableCopy(
        /* [out, retval] */ IWebMutableURLRequest** result)
{
    if (!result)
        return E_POINTER;

#if USE(CFNETWORK)
    RetainPtr<CFMutableURLRequestRef> mutableRequest = adoptCF(CFURLRequestCreateMutableCopy(kCFAllocatorDefault, m_request.cfURLRequest(UpdateHTTPBody)));
    *result = createInstance(ResourceRequest(mutableRequest.get()));
    return S_OK;
#else
    *result = createInstance(m_request);
    return S_OK;
#endif
}

// IWebMutableURLRequest ----------------------------------------------------

void WebMutableURLRequest::setFormData(const PassRefPtr<FormData> data)
{
    m_request.setHTTPBody(data);
}

const PassRefPtr<FormData> WebMutableURLRequest::formData() const
{
    return m_request.httpBody();
}

void WebMutableURLRequest::addHTTPHeaderFields(const HTTPHeaderMap& headerFields)
{
    m_request.addHTTPHeaderFields(headerFields);
}

const HTTPHeaderMap& WebMutableURLRequest::httpHeaderFields() const
{
    return m_request.httpHeaderFields();
}

const ResourceRequest& WebMutableURLRequest::resourceRequest() const
{
    return m_request;
}
