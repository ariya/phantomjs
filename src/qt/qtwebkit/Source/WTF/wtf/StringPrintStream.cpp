/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "StringPrintStream.h"

#include <stdarg.h>
#include <stdio.h>
#include <wtf/FastMalloc.h>
#include <wtf/StringExtras.h>

namespace WTF {

StringPrintStream::StringPrintStream()
    : m_buffer(m_inlineBuffer)
    , m_next(0)
    , m_size(sizeof(m_inlineBuffer))
{
    m_buffer[0] = 0; // Make sure that we always have a null terminator.
}

StringPrintStream::~StringPrintStream()
{
    if (m_buffer == m_inlineBuffer)
        return;
    fastFree(m_buffer);
}

void StringPrintStream::vprintf(const char* format, va_list argList)
{
    ASSERT_WITH_SECURITY_IMPLICATION(m_next < m_size);
    ASSERT(!m_buffer[m_next]);
    
    va_list firstPassArgList;
#if OS(WINDOWS)
    firstPassArgList = argList;
#else
    va_copy(firstPassArgList, argList);
#endif
    
    int numberOfBytesNotIncludingTerminatorThatWouldHaveBeenWritten =
        vsnprintf(m_buffer + m_next, m_size - m_next, format, firstPassArgList);
    
    int numberOfBytesThatWouldHaveBeenWritten =
        numberOfBytesNotIncludingTerminatorThatWouldHaveBeenWritten + 1;
    
    if (m_next + numberOfBytesThatWouldHaveBeenWritten <= m_size) {
        m_next += numberOfBytesNotIncludingTerminatorThatWouldHaveBeenWritten;
        return; // This means that vsnprintf() succeeded.
    }
    
    increaseSize(m_next + numberOfBytesThatWouldHaveBeenWritten);
    
    int numberOfBytesNotIncludingTerminatorThatWereWritten =
        vsnprintf(m_buffer + m_next, m_size - m_next, format, argList);
    
    int numberOfBytesThatWereWritten = numberOfBytesNotIncludingTerminatorThatWereWritten + 1;
    
    ASSERT_UNUSED(numberOfBytesThatWereWritten, m_next + numberOfBytesThatWereWritten <= m_size);
    
    m_next += numberOfBytesNotIncludingTerminatorThatWereWritten;
    
    ASSERT_WITH_SECURITY_IMPLICATION(m_next < m_size);
    ASSERT(!m_buffer[m_next]);
}

CString StringPrintStream::toCString()
{
    ASSERT(m_next == strlen(m_buffer));
    return CString(m_buffer, m_next);
}

void StringPrintStream::reset()
{
    m_next = 0;
    m_buffer[0] = 0;
}

String StringPrintStream::toString()
{
    ASSERT(m_next == strlen(m_buffer));
    return String::fromUTF8(m_buffer, m_next);
}

void StringPrintStream::increaseSize(size_t newSize)
{
    ASSERT(newSize > m_size);
    ASSERT(newSize > sizeof(m_inlineBuffer));
    
    // Use exponential resizing to reduce thrashing.
    m_size = newSize << 1;
    
    // Use fastMalloc instead of fastRealloc because we know that for the sizes we're using,
    // fastRealloc will just do malloc+free anyway. Also, this simplifies the code since
    // we can't realloc the inline buffer.
    char* newBuffer = static_cast<char*>(fastMalloc(m_size));
    memcpy(newBuffer, m_buffer, m_next + 1);
    if (m_buffer != m_inlineBuffer)
        fastFree(m_buffer);
    m_buffer = newBuffer;
}

} // namespace WTF

