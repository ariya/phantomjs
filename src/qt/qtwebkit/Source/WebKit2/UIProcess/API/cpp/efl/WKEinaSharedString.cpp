/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKEinaSharedString.h"

#include <WebKit2/WKAPICast.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <wtf/text/CString.h>

using namespace WebKit;

template <typename WKRefType>
ALWAYS_INLINE const char* fromRefType(WKRefType strRef, bool adopt = false)
{
    const char* res = 0;
    if (strRef) {
        res = eina_stringshare_add(toImpl(strRef)->string().utf8().data());
        ASSERT(res);
        if (adopt)
            WKRelease(strRef);
    }

    return res;
}

WKEinaSharedString::WKEinaSharedString(const WKEinaSharedString& other)
    : m_string(eina_stringshare_ref(other.m_string))
{
}

WKEinaSharedString::WKEinaSharedString(const char* str)
    : m_string(eina_stringshare_add(str))
{
}

WKEinaSharedString::WKEinaSharedString(WKAdoptTag adoptTag, WKStringRef stringRef)
    : m_string(fromRefType(stringRef, /*adopt*/ true))
{
    ASSERT_UNUSED(adoptTag, adoptTag == AdoptWK); // Guard for future enum changes.
}

WKEinaSharedString::WKEinaSharedString(WKStringRef stringRef)
    : m_string(fromRefType(stringRef))
{
}

WKEinaSharedString::WKEinaSharedString(WKAdoptTag adoptTag, WKURLRef urlRef)
    : m_string(fromRefType(urlRef, /*adopt*/ true))
{
    ASSERT_UNUSED(adoptTag, adoptTag == AdoptWK); // Guard for future enum changes.
}

WKEinaSharedString::WKEinaSharedString(WKURLRef urlRef)
    : m_string(fromRefType(urlRef))
{
}

WKEinaSharedString::~WKEinaSharedString()
{
    if (m_string)
        eina_stringshare_del(m_string);
}

WKEinaSharedString& WKEinaSharedString::operator=(const WKEinaSharedString& other)
{
    if (this != &other) {
        if (m_string)
            eina_stringshare_del(m_string);
        m_string = eina_stringshare_ref(other.m_string);
    }
    return *this;
}

WKEinaSharedString& WKEinaSharedString::operator=(const char* str)
{
    eina_stringshare_replace(&m_string, str);
    return *this;
}

bool WKEinaSharedString::operator==(const char* str) const
{
    return (!str || !m_string) ? (str == m_string) : !strcmp(m_string, str);
}

WKEinaSharedString WKEinaSharedString::adopt(Eina_Stringshare* string)
{
    WKEinaSharedString sharedString;
    sharedString.m_string = static_cast<const char*>(string);
    return sharedString;
}

Eina_Stringshare* WKEinaSharedString::leakString()
{
    Eina_Stringshare* sharedString = m_string;
    m_string = 0;

    return sharedString;
}
