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
#include "WebURLCredential.h"

#include "WebKit.h"
#include <WebCore/BString.h>

using namespace WebCore;

// WebURLCredential ----------------------------------------------------------------

WebURLCredential::WebURLCredential(const Credential& credential)
    : m_refCount(0)
    , m_credential(credential)
{
    gClassCount++;
    gClassNameCount.add("WebURLCredential");
}

WebURLCredential::~WebURLCredential()
{
    gClassCount--;
    gClassNameCount.remove("WebURLCredential");
}

WebURLCredential* WebURLCredential::createInstance()
{
    WebURLCredential* instance = new WebURLCredential(Credential());
    instance->AddRef();
    return instance;
}

WebURLCredential* WebURLCredential::createInstance(const Credential& credential)
{
    WebURLCredential* instance = new WebURLCredential(credential);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLCredential::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, __uuidof(WebURLCredential)))
        *ppvObject = static_cast<WebURLCredential*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLCredential))
        *ppvObject = static_cast<IWebURLCredential*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebURLCredential::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebURLCredential::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebURLCredential -------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE WebURLCredential::hasPassword(
        /* [out, retval] */ BOOL* result)
{
    *result = m_credential.hasPassword();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLCredential::initWithUser(
        /* [in] */ BSTR user, 
        /* [in] */ BSTR password, 
        /* [in] */ WebURLCredentialPersistence persistence)
{
    CredentialPersistence corePersistence = CredentialPersistenceNone;
    switch (persistence) {
    case WebURLCredentialPersistenceNone:
        break;
    case WebURLCredentialPersistenceForSession:
        corePersistence = CredentialPersistenceForSession;
        break;
    case WebURLCredentialPersistencePermanent:
        corePersistence = CredentialPersistencePermanent;
        break;
    default:
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }

    m_credential = Credential(String(user, SysStringLen(user)), String(password, SysStringLen(password)), corePersistence);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLCredential::password(
        /* [out, retval] */ BSTR* password)
{
    BString str = m_credential.password();
    *password = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLCredential::persistence(
        /* [out, retval] */ WebURLCredentialPersistence* result)
{
    switch (m_credential.persistence()) {
    case CredentialPersistenceNone:
        *result = WebURLCredentialPersistenceNone;
        break;
    case CredentialPersistenceForSession:
        *result = WebURLCredentialPersistenceForSession;
        break;
    case CredentialPersistencePermanent:
        *result = WebURLCredentialPersistencePermanent;
        break;
    default:
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLCredential::user(
        /* [out, retval] */ BSTR* result)
{
    BString str = m_credential.user();
    *result = str.release();
    return S_OK;
}

const WebCore::Credential& WebURLCredential::credential() const
{
    return m_credential;
}

