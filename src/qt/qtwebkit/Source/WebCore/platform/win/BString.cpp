/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "BString.h"

#include "KURL.h"
#include <windows.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace JSC;

namespace WebCore {

BString::BString()
    : m_bstr(0)
{
}

BString::BString(const wchar_t* characters)
{
    if (!characters)
        m_bstr = 0;
    else
        m_bstr = SysAllocString(characters);
}

BString::BString(const wchar_t* characters, size_t length)
{
    if (!characters)
        m_bstr = 0;
    else
        m_bstr = SysAllocStringLen(characters, length);
}

BString::BString(const String& s)
{
    if (s.isNull())
        m_bstr = 0;
    else
        m_bstr = SysAllocStringLen(s.characters(), s.length());
}

BString::BString(const KURL& url)
{
    if (url.isNull())
        m_bstr = 0;
    else
        m_bstr = SysAllocStringLen(url.string().characters(), url.string().length());
}

BString::BString(const AtomicString& s)
{
    if (s.isNull())
        m_bstr = 0;
    else
        m_bstr = SysAllocStringLen(s.characters(), s.length());
}

#if USE(CF)
BString::BString(CFStringRef cfstr)
    : m_bstr(0)
{
    if (!cfstr)
        return;

    const UniChar* uniChars = CFStringGetCharactersPtr(cfstr);
    if (uniChars) {
        m_bstr = SysAllocStringLen((LPCWSTR)uniChars, CFStringGetLength(cfstr));
        return;
    }

    CFIndex length = CFStringGetLength(cfstr);
    m_bstr = SysAllocStringLen(0, length);
    CFStringGetCharacters(cfstr, CFRangeMake(0, length), (UniChar*)m_bstr);
    m_bstr[length] = 0;
}
#endif

BString::~BString()
{
    SysFreeString(m_bstr);
}

BString::BString(const BString& other)
{
    if (!other.m_bstr)
        m_bstr = 0;
    else
        m_bstr = SysAllocString(other.m_bstr);
}

void BString::adoptBSTR(BSTR bstr)
{
    SysFreeString(m_bstr);
    m_bstr = bstr;
}

void BString::clear()
{
    SysFreeString(m_bstr);
    m_bstr = 0;
}

BString& BString::operator=(const BString& other)
{
    if (this != &other)
        *this = other.m_bstr;
    return *this;
}

BString& BString::operator=(const BSTR& other)
{
    if (other != m_bstr) {
        SysFreeString(m_bstr);
        m_bstr = other ? SysAllocString(other) : 0;
    }

    return *this;
}

bool operator ==(const BString& a, const BString& b)
{
    if (SysStringLen((BSTR)a) != SysStringLen((BSTR)b))
        return false;
    if (!(BSTR)a && !(BSTR)b)
        return true;
    if (!(BSTR)a || !(BSTR)b)
        return false;
    return !wcscmp((BSTR)a, (BSTR)b);
}

bool operator !=(const BString& a, const BString& b)
{
    return !(a==b);
}

bool operator ==(const BString& a, BSTR b)
{
    if (SysStringLen((BSTR)a) != SysStringLen(b))
        return false;
    if (!(BSTR)a && !b)
        return true;
    if (!(BSTR)a || !b)
        return false;
    return !wcscmp((BSTR)a, b);
}

bool operator !=(const BString& a, BSTR b)
{
    return !(a==b);
}

bool operator ==(BSTR a, const BString& b)
{
    if (SysStringLen(a) != SysStringLen((BSTR)b))
        return false;
    if (!a && !(BSTR)b)
        return true;
    if (!a || !(BSTR)b)
        return false;
    return !wcscmp(a, (BSTR)b);
}

bool operator !=(BSTR a, const BString& b)
{
    return !(a==b);
}

}
