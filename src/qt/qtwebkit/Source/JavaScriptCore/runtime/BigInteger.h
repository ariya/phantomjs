/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef BigInteger_h
#define BigInteger_h

#include <wtf/MathExtras.h>

namespace JSC {

// This is used in converting the integer part of a number to a string.
class BigInteger {
public:
    BigInteger(double number)
    {
        ASSERT(std::isfinite(number) && !std::signbit(number));
        ASSERT(number == floor(number));

        bool sign;
        int32_t exponent;
        uint64_t mantissa;
        decomposeDouble(number, sign, exponent, mantissa);
        ASSERT(!sign && exponent >= 0);

        int32_t zeroBits = exponent - 52;

        if (zeroBits < 0) {
            mantissa >>= -zeroBits;
            zeroBits = 0;
        }

        while (zeroBits >= 32) {
            m_values.append(0);
            zeroBits -= 32;
        }

        // Left align the 53 bits of the mantissa within 96 bits.
        uint32_t values[3];
        values[0] = static_cast<uint32_t>(mantissa);
        values[1] = static_cast<uint32_t>(mantissa >> 32);
        values[2] = 0;
        // Shift based on the remainder of the exponent.
        if (zeroBits) {
            values[2] = values[1] >> (32 - zeroBits);
            values[1] = (values[1] << zeroBits) | (values[0] >> (32 - zeroBits));
            values[0] = (values[0] << zeroBits);
        }
        m_values.append(values[0]);
        m_values.append(values[1]);
        m_values.append(values[2]);

        // Canonicalize; remove all trailing zeros.
        while (m_values.size() && !m_values.last())
            m_values.removeLast();
    }

    uint32_t divide(uint32_t divisor)
    {
        uint32_t carry = 0;

        for (size_t i = m_values.size(); i; ) {
            --i;
            uint64_t dividend = (static_cast<uint64_t>(carry) << 32) + static_cast<uint64_t>(m_values[i]);

            uint64_t result = dividend / static_cast<uint64_t>(divisor);
            ASSERT(result == static_cast<uint32_t>(result));
            uint64_t remainder = dividend % static_cast<uint64_t>(divisor);
            ASSERT(remainder == static_cast<uint32_t>(remainder));
            
            m_values[i] = static_cast<uint32_t>(result);
            carry = static_cast<uint32_t>(remainder);
        }

        // Canonicalize; remove all trailing zeros.
        while (m_values.size() && !m_values.last())
            m_values.removeLast();

        return carry;
    }

    bool operator!() { return !m_values.size(); }

private:
    Vector<uint32_t, 36> m_values;
};

}

#endif

