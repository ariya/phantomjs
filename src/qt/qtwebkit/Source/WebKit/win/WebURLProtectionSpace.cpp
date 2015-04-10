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
#include "WebKit.h"
#include "WebKitDLL.h"
#include "WebURLProtectionSpace.h"

#include <WebCore/BString.h>

using namespace WebCore;

// WebURLProtectionSpace ----------------------------------------------------------------

WebURLProtectionSpace::WebURLProtectionSpace(const ProtectionSpace& protectionSpace)
    : m_refCount(0)
    , m_protectionSpace(protectionSpace)
{
    gClassCount++;
    gClassNameCount.add("WebURLProtectionSpace");
}

WebURLProtectionSpace::~WebURLProtectionSpace()
{
    gClassCount--;
    gClassNameCount.remove("WebURLProtectionSpace");
}

WebURLProtectionSpace* WebURLProtectionSpace::createInstance()
{
    WebURLProtectionSpace* instance = new WebURLProtectionSpace(ProtectionSpace());
    instance->AddRef();
    return instance;
}

WebURLProtectionSpace* WebURLProtectionSpace::createInstance(const ProtectionSpace& protectionSpace)
{
    WebURLProtectionSpace* instance = new WebURLProtectionSpace(protectionSpace);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, CLSID_WebURLProtectionSpace))
        *ppvObject = static_cast<WebURLProtectionSpace*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLProtectionSpace))
        *ppvObject = static_cast<IWebURLProtectionSpace*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebURLProtectionSpace::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebURLProtectionSpace::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebURLProtectionSpace -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::authenticationMethod(
    /* [out, retval] */ BSTR* result)
{
    switch (m_protectionSpace.authenticationScheme()) {
    case ProtectionSpaceAuthenticationSchemeDefault:
        *result = SysAllocString(WebURLAuthenticationMethodDefault);
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        *result = SysAllocString(WebURLAuthenticationMethodHTTPBasic);
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        *result = SysAllocString(WebURLAuthenticationMethodHTTPDigest);
        break;
    case ProtectionSpaceAuthenticationSchemeHTMLForm:
        *result = SysAllocString(WebURLAuthenticationMethodHTMLForm);
        break;
    default:
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::host(
    /* [out, retval] */ BSTR* result)
{
    BString str = m_protectionSpace.host();
    *result = str.release();
    return S_OK;
}

static ProtectionSpaceAuthenticationScheme coreScheme(BSTR authenticationMethod)
{
    ProtectionSpaceAuthenticationScheme scheme = ProtectionSpaceAuthenticationSchemeDefault;
    if (BString(authenticationMethod) == BString(WebURLAuthenticationMethodDefault))
        scheme = ProtectionSpaceAuthenticationSchemeDefault;
    else if (BString(authenticationMethod) == BString(WebURLAuthenticationMethodHTTPBasic))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
    else if (BString(authenticationMethod) == BString(WebURLAuthenticationMethodHTTPDigest))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
    else if (BString(authenticationMethod) == BString(WebURLAuthenticationMethodHTMLForm))
        scheme = ProtectionSpaceAuthenticationSchemeHTMLForm;
    else
        ASSERT_NOT_REACHED();
    return scheme;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::initWithHost(
    /* [in] */ BSTR host, 
    /* [in] */ int port, 
    /* [in] */ BSTR protocol, 
    /* [in] */ BSTR realm, 
    /* [in] */ BSTR authenticationMethod)
{
    static BString& webURLProtectionSpaceHTTPBString = *new BString(WebURLProtectionSpaceHTTP);
    static BString& webURLProtectionSpaceHTTPSBString = *new BString(WebURLProtectionSpaceHTTPS);
    static BString& webURLProtectionSpaceFTPBString = *new BString(WebURLProtectionSpaceFTP);
    static BString& webURLProtectionSpaceFTPSBString = *new BString(WebURLProtectionSpaceFTPS);

    ProtectionSpaceServerType serverType = ProtectionSpaceServerHTTP;
    if (BString(protocol) == webURLProtectionSpaceHTTPBString)
        serverType = ProtectionSpaceServerHTTP;
    else if (BString(protocol) == webURLProtectionSpaceHTTPSBString)
        serverType = ProtectionSpaceServerHTTPS;
    else if (BString(protocol) == webURLProtectionSpaceFTPBString)
        serverType = ProtectionSpaceServerFTP;
    else if (BString(protocol) == webURLProtectionSpaceFTPSBString)
        serverType = ProtectionSpaceServerFTPS;

    m_protectionSpace = ProtectionSpace(String(host, SysStringLen(host)), port, serverType, 
        String(realm, SysStringLen(realm)), coreScheme(authenticationMethod));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::initWithProxyHost(
    /* [in] */ BSTR host, 
    /* [in] */ int port, 
    /* [in] */ BSTR proxyType, 
    /* [in] */ BSTR realm, 
    /* [in] */ BSTR authenticationMethod)
{
    static BString& webURLProtectionSpaceHTTPProxyBString = *new BString(WebURLProtectionSpaceHTTPProxy);
    static BString& webURLProtectionSpaceHTTPSProxyBString = *new BString(WebURLProtectionSpaceHTTPSProxy);
    static BString& webURLProtectionSpaceFTPProxyBString = *new BString(WebURLProtectionSpaceFTPProxy);
    static BString& webURLProtectionSpaceSOCKSProxyBString = *new BString(WebURLProtectionSpaceSOCKSProxy);

    ProtectionSpaceServerType serverType = ProtectionSpaceProxyHTTP;
    if (BString(proxyType) == webURLProtectionSpaceHTTPProxyBString)
        serverType = ProtectionSpaceProxyHTTP;
    else if (BString(proxyType) == webURLProtectionSpaceHTTPSProxyBString)
        serverType = ProtectionSpaceProxyHTTPS;
    else if (BString(proxyType) == webURLProtectionSpaceFTPProxyBString)
        serverType = ProtectionSpaceProxyFTP;
    else if (BString(proxyType) == webURLProtectionSpaceSOCKSProxyBString)
        serverType = ProtectionSpaceProxySOCKS;
    else
        ASSERT_NOT_REACHED();

    m_protectionSpace = ProtectionSpace(String(host, SysStringLen(host)), port, serverType, 
        String(realm, SysStringLen(realm)), coreScheme(authenticationMethod));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::isProxy(
    /* [out, retval] */ BOOL* result)
{
    *result = m_protectionSpace.isProxy();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::port(
    /* [out, retval] */ int* result)
{
    *result = m_protectionSpace.port();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::protocol(
    /* [out, retval] */ BSTR* result)
{
    switch (m_protectionSpace.serverType()) {
    case ProtectionSpaceServerHTTP:
        *result = SysAllocString(WebURLProtectionSpaceHTTP);
        break;
    case ProtectionSpaceServerHTTPS:
        *result = SysAllocString(WebURLProtectionSpaceHTTPS);
        break;
    case ProtectionSpaceServerFTP:
        *result = SysAllocString(WebURLProtectionSpaceFTP);
        break;
    case ProtectionSpaceServerFTPS:
        *result = SysAllocString(WebURLProtectionSpaceFTPS);
        break;
    default:
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::proxyType(
    /* [out, retval] */ BSTR* result)
{
    switch (m_protectionSpace.serverType()) {
    case ProtectionSpaceProxyHTTP:
        *result = SysAllocString(WebURLProtectionSpaceHTTPProxy);
        break;
    case ProtectionSpaceProxyHTTPS:
        *result = SysAllocString(WebURLProtectionSpaceHTTPSProxy);
        break;
    case ProtectionSpaceProxyFTP:
        *result = SysAllocString(WebURLProtectionSpaceFTPProxy);
        break;
    case ProtectionSpaceProxySOCKS:
        *result = SysAllocString(WebURLProtectionSpaceSOCKSProxy);
        break;
    default:
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::realm(
    /* [out, retval] */ BSTR* result)
{
    BString bstring = m_protectionSpace.realm();
    *result = bstring.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLProtectionSpace::receivesCredentialSecurely(
    /* [out, retval] */ BOOL* result)
{
    *result = m_protectionSpace.receivesCredentialSecurely();
    return S_OK;
}

// WebURLProtectionSpace -------------------------------------------------------------------
const ProtectionSpace& WebURLProtectionSpace::protectionSpace() const
{
    return m_protectionSpace;
}
