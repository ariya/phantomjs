/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebUserContentURLPattern.h"

#include "MarshallingHelpers.h"
#include "WebKitDLL.h"

#include <WebCore/BString.h>
#include <WebCore/KURL.h>

using namespace WebCore;

inline WebUserContentURLPattern::WebUserContentURLPattern()
    : m_refCount(0)
{
    ++gClassCount;
    gClassNameCount.add("WebUserContentURLPattern");
}

WebUserContentURLPattern::~WebUserContentURLPattern()
{
    --gClassCount;
    gClassNameCount.remove("WebUserContentURLPattern");
}

COMPtr<WebUserContentURLPattern> WebUserContentURLPattern::createInstance()
{
    return new WebUserContentURLPattern;
}

ULONG WebUserContentURLPattern::AddRef()
{
    return ++m_refCount;
}

ULONG WebUserContentURLPattern::Release()
{
    ULONG newRefCount = --m_refCount;
    if (!newRefCount)
        delete this;
    return newRefCount;
}

HRESULT WebUserContentURLPattern::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    *ppvObject = 0;

    if (IsEqualIID(riid, __uuidof(WebUserContentURLPattern)))
        *ppvObject = this;
    else if (IsEqualIID(riid, __uuidof(IWebUserContentURLPattern)))
        *ppvObject = static_cast<IWebUserContentURLPattern*>(this);
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

HRESULT WebUserContentURLPattern::parse(BSTR patternString)
{
    m_pattern = UserContentURLPattern(String(patternString, SysStringLen(patternString)));
    return S_OK;
}

HRESULT WebUserContentURLPattern::isValid(BOOL* isValid)
{
    if (!isValid)
        return E_POINTER;
    *isValid = m_pattern.isValid();
    return S_OK;
}

HRESULT WebUserContentURLPattern::scheme(BSTR* scheme)
{
    if (!scheme)
        return E_POINTER;
    *scheme = BString(m_pattern.scheme()).release();
    return S_OK;
}

HRESULT WebUserContentURLPattern::host(BSTR* host)
{
    if (!host)
        return E_POINTER;
    *host = BString(m_pattern.host()).release();
    return S_OK;
}

HRESULT WebUserContentURLPattern::matchesSubdomains(BOOL* matches)
{
    if (!matches)
        return E_POINTER;
    *matches = m_pattern.matchSubdomains();
    return S_OK;
}

HRESULT WebUserContentURLPattern::matchesURL(BSTR url, BOOL* matches)
{
    if (!matches)
        return E_POINTER;
    *matches = m_pattern.matches(MarshallingHelpers::BSTRToKURL(url));
    return S_OK;
}
