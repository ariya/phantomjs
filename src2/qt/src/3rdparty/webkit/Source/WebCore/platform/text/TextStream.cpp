/*
 * Copyright (C) 2004, 2008, 2010 Apple Inc. All rights reserved.
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
#include "TextStream.h"

#include "PlatformString.h"
#include <wtf/StringExtras.h>

using namespace std;

namespace WebCore {

static const size_t printBufferSize = 100; // large enough for any integer or floating point value in string format, including trailing null character

TextStream& TextStream::operator<<(bool b)
{
    return *this << (b ? "1" : "0");
}

TextStream& TextStream::operator<<(int i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%d", i);
    return *this << buffer;
}

TextStream& TextStream::operator<<(unsigned i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%u", i);
    return *this << buffer;
}

TextStream& TextStream::operator<<(long i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%ld", i);
    return *this << buffer;
}

TextStream& TextStream::operator<<(unsigned long i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%lu", i);
    return *this << buffer;
}

TextStream& TextStream::operator<<(float f)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%.2f", f);
    return *this << buffer;
}

TextStream& TextStream::operator<<(double d)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%.2f", d);
    return *this << buffer;
}

TextStream& TextStream::operator<<(const char* string)
{
    size_t stringLength = strlen(string);
    size_t textLength = m_text.size();
    if (stringLength > numeric_limits<size_t>::max() - textLength)
        CRASH();
    m_text.grow(textLength + stringLength);
    for (size_t i = 0; i < stringLength; ++i)
        m_text[textLength + i] = string[i];
    return *this;
}

TextStream& TextStream::operator<<(const void* p)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%p", p);
    return *this << buffer;
}

TextStream& TextStream::operator<<(const String& string)
{
    append(m_text, string);
    return *this;
}

String TextStream::release()
{
    return String::adopt(m_text);
}

#if OS(WINDOWS) && CPU(X86_64)
TextStream& TextStream::operator<<(__int64 i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%I64i", i);
    return *this << buffer;
}
TextStream& TextStream::operator<<(unsigned __int64 i)
{
    char buffer[printBufferSize];
    snprintf(buffer, sizeof(buffer) - 1, "%I64u", i);
    return *this << buffer;
}
#endif

}
