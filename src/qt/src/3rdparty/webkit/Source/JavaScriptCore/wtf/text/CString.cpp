/*
 * Copyright (C) 2003, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "CString.h"

using namespace std;

namespace WTF {

CString::CString(const char* str)
{
    if (!str)
        return;

    init(str, strlen(str));
}

CString::CString(const char* str, size_t length)
{
    init(str, length);
}

void CString::init(const char* str, size_t length)
{
    if (!str)
        return;

    // We need to be sure we can add 1 to length without overflowing.
    // Since the passed-in length is the length of an actual existing
    // string, and we know the string doesn't occupy the entire address
    // space, we can assert here and there's no need for a runtime check.
    ASSERT(length < numeric_limits<size_t>::max());

    m_buffer = CStringBuffer::create(length + 1);
    memcpy(m_buffer->mutableData(), str, length); 
    m_buffer->mutableData()[length] = '\0';
}

char* CString::mutableData()
{
    copyBufferIfNeeded();
    if (!m_buffer)
        return 0;
    return m_buffer->mutableData();
}
    
CString CString::newUninitialized(size_t length, char*& characterBuffer)
{
    if (length >= numeric_limits<size_t>::max())
        CRASH();

    CString result;
    result.m_buffer = CStringBuffer::create(length + 1);
    char* bytes = result.m_buffer->mutableData();
    bytes[length] = '\0';
    characterBuffer = bytes;
    return result;
}

void CString::copyBufferIfNeeded()
{
    if (!m_buffer || m_buffer->hasOneRef())
        return;

    RefPtr<CStringBuffer> buffer = m_buffer.release();
    size_t length = buffer->length();
    m_buffer = CStringBuffer::create(length);
    memcpy(m_buffer->mutableData(), buffer->data(), length);
}

bool operator==(const CString& a, const CString& b)
{
    if (a.isNull() != b.isNull())
        return false;
    if (a.length() != b.length())
        return false;
    return !strncmp(a.data(), b.data(), min(a.length(), b.length()));
}

} // namespace WTF
