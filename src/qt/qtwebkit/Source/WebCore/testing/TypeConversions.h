/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TypeConversions_h
#define TypeConversions_h

#include <wtf/FastMalloc.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class TypeConversions : public RefCounted<TypeConversions> {
public:
    static PassRefPtr<TypeConversions> create() { return adoptRef(new TypeConversions()); }

    long testLong() { return m_long; }
    void setTestLong(long value) { m_long = value; }
    long testEnforceRangeLong() { return m_long; }
    void setTestEnforceRangeLong(long value) { m_long = value; }
    unsigned long testUnsignedLong() { return m_unsignedLong; }
    void setTestUnsignedLong(unsigned long value) { m_unsignedLong = value; }
    unsigned long testEnforceRangeUnsignedLong() { return m_unsignedLong; }
    void setTestEnforceRangeUnsignedLong(unsigned long value) { m_unsignedLong = value; }

    long long testLongLong() { return m_longLong; }
    void setTestLongLong(long long value) { m_longLong = value; }
    long long testEnforceRangeLongLong() { return m_longLong; }
    void setTestEnforceRangeLongLong(long long value) { m_longLong = value; }
    unsigned long long testUnsignedLongLong() { return m_unsignedLongLong; }
    void setTestUnsignedLongLong(unsigned long long value) { m_unsignedLongLong = value; }
    unsigned long long testEnforceRangeUnsignedLongLong() { return m_unsignedLongLong; }
    void setTestEnforceRangeUnsignedLongLong(unsigned long long value) { m_unsignedLongLong = value; }

    int8_t testByte() { return m_byte; }
    void setTestByte(int8_t value) { m_byte = value; }
    int8_t testEnforceRangeByte() { return m_byte; }
    void setTestEnforceRangeByte(int8_t value) { m_byte = value; }

    uint8_t testOctet() { return m_octet; }
    void setTestOctet(uint8_t value) { m_octet = value; }
    uint8_t testEnforceRangeOctet() { return m_octet; }
    void setTestEnforceRangeOctet(uint8_t value) { m_octet = value; }
private:
    TypeConversions()
    {
    }

    long m_long;
    unsigned long m_unsignedLong;
    long long m_longLong;
    unsigned long long m_unsignedLongLong;
    int8_t m_byte;
    uint8_t m_octet;
};

} // namespace WebCore

#endif
