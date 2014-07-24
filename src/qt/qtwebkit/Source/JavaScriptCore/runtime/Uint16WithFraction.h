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

#ifndef Uint16WithFraction_h
#define Uint16WithFraction_h

#include <wtf/MathExtras.h>

namespace JSC {

// Would be nice if this was a static const member, but the OS X linker
// seems to want a symbol in the binary in that case...
#define oneGreaterThanMaxUInt16 0x10000

// A uint16_t with an infinite precision fraction. Upon overflowing
// the uint16_t range, this class will clamp to oneGreaterThanMaxUInt16.
// This is used in converting the fraction part of a number to a string.
class Uint16WithFraction {
public:
    explicit Uint16WithFraction(double number, uint16_t divideByExponent = 0)
    {
        ASSERT(number && std::isfinite(number) && !std::signbit(number));

        // Check for values out of uint16_t range.
        if (number >= oneGreaterThanMaxUInt16) {
            m_values.append(oneGreaterThanMaxUInt16);
            m_leadingZeros = 0;
            return;
        }

        // Append the units to m_values.
        double integerPart = floor(number);
        m_values.append(static_cast<uint32_t>(integerPart));

        bool sign;
        int32_t exponent;
        uint64_t mantissa;
        decomposeDouble(number - integerPart, sign, exponent, mantissa);
        ASSERT(!sign && exponent < 0);
        exponent -= divideByExponent;

        int32_t zeroBits = -exponent;
        --zeroBits;

        // Append the append words for to m_values.
        while (zeroBits >= 32) {
            m_values.append(0);
            zeroBits -= 32;
        }

        // Left align the 53 bits of the mantissa within 96 bits.
        uint32_t values[3];
        values[0] = static_cast<uint32_t>(mantissa >> 21);
        values[1] = static_cast<uint32_t>(mantissa << 11);
        values[2] = 0;
        // Shift based on the remainder of the exponent.
        if (zeroBits) {
            values[2] = values[1] << (32 - zeroBits);
            values[1] = (values[1] >> zeroBits) | (values[0] << (32 - zeroBits));
            values[0] = (values[0] >> zeroBits);
        }
        m_values.append(values[0]);
        m_values.append(values[1]);
        m_values.append(values[2]);

        // Canonicalize; remove any trailing zeros.
        while (m_values.size() > 1 && !m_values.last())
            m_values.removeLast();

        // Count the number of leading zero, this is useful in optimizing multiplies.
        m_leadingZeros = 0;
        while (m_leadingZeros < m_values.size() && !m_values[m_leadingZeros])
            ++m_leadingZeros;
    }

    Uint16WithFraction& operator*=(uint16_t multiplier)
    {
        ASSERT(checkConsistency());

        // iteratate backwards over the fraction until we reach the leading zeros,
        // passing the carry from one calculation into the next.
        uint64_t accumulator = 0;
        for (size_t i = m_values.size(); i > m_leadingZeros; ) {
            --i;
            accumulator += static_cast<uint64_t>(m_values[i]) * static_cast<uint64_t>(multiplier);
            m_values[i] = static_cast<uint32_t>(accumulator);
            accumulator >>= 32;
        }

        if (!m_leadingZeros) {
            // With a multiplicand and multiplier in the uint16_t range, this cannot carry
            // (even allowing for the infinity value).
            ASSERT(!accumulator);
            // Check for overflow & clamp to 'infinity'.
            if (m_values[0] >= oneGreaterThanMaxUInt16) {
                m_values.shrink(1);
                m_values[0] = oneGreaterThanMaxUInt16;
                m_leadingZeros = 0;
                return *this;
            }
        } else if (accumulator) {
            // Check for carry from the last multiply, if so overwrite last leading zero.
            m_values[--m_leadingZeros] = static_cast<uint32_t>(accumulator);
            // The limited range of the multiplier should mean that even if we carry into
            // the units, we don't need to check for overflow of the uint16_t range.
            ASSERT(m_values[0] < oneGreaterThanMaxUInt16);
        }

        // Multiplication by an even value may introduce trailing zeros; if so, clean them
        // up. (Keeping the value in a normalized form makes some of the comparison operations
        // more efficient).
        while (m_values.size() > 1 && !m_values.last())
            m_values.removeLast();
        ASSERT(checkConsistency());
        return *this;
    }

    bool operator<(const Uint16WithFraction& other)
    {
        ASSERT(checkConsistency());
        ASSERT(other.checkConsistency());

        // Iterate over the common lengths of arrays.
        size_t minSize = std::min(m_values.size(), other.m_values.size());
        for (size_t index = 0; index < minSize; ++index) {
            // If we find a value that is not equal, compare and return.
            uint32_t fromThis = m_values[index];
            uint32_t fromOther = other.m_values[index];
            if (fromThis != fromOther)
                return fromThis < fromOther;
        }
        // If these numbers have the same lengths, they are equal,
        // otherwise which ever number has a longer fraction in larger.
        return other.m_values.size() > minSize;
    }

    // Return the floor (non-fractional portion) of the number, clearing this to zero,
    // leaving the fractional part unchanged.
    uint32_t floorAndSubtract()
    {
        // 'floor' is simple the integer portion of the value.
        uint32_t floor = m_values[0];

        // If floor is non-zero, 
        if (floor) {
            m_values[0] = 0;
            m_leadingZeros = 1;
            while (m_leadingZeros < m_values.size() && !m_values[m_leadingZeros])
                ++m_leadingZeros;
        }

        return floor;
    }

    // Compare this value to 0.5, returns -1 for less than, 0 for equal, 1 for greater.
    int comparePoint5()
    {
        ASSERT(checkConsistency());
        // If units != 0, this is greater than 0.5.
        if (m_values[0])
            return 1;
        // If size == 1 this value is 0, hence < 0.5.
        if (m_values.size() == 1)
            return -1;
        // Compare to 0.5.
        if (m_values[1] > 0x80000000ul)
            return 1;
        if (m_values[1] < 0x80000000ul)
            return -1;
        // Check for more words - since normalized numbers have no trailing zeros, if
        // there are more that two digits we can assume at least one more is non-zero,
        // and hence the value is > 0.5.
        return m_values.size() > 2 ? 1 : 0;
    }

    // Return true if the sum of this plus addend would be greater than 1.
    bool sumGreaterThanOne(const Uint16WithFraction& addend) 
    {
        ASSERT(checkConsistency());
        ASSERT(addend.checkConsistency());

        // First, sum the units. If the result is greater than one, return true.
        // If equal to one, return true if either number has a fractional part.
        uint32_t sum = m_values[0] + addend.m_values[0];
        if (sum)
            return sum > 1 || std::max(m_values.size(), addend.m_values.size()) > 1;

        // We could still produce a result greater than zero if addition of the next
        // word from the fraction were to carry, leaving a result > 0.

        // Iterate over the common lengths of arrays.
        size_t minSize = std::min(m_values.size(), addend.m_values.size());
        for (size_t index = 1; index < minSize; ++index) {
            // Sum the next word from this & the addend.
            uint32_t fromThis = m_values[index];
            uint32_t fromAddend = addend.m_values[index];
            sum = fromThis + fromAddend;

            // Check for overflow. If so, check whether the remaining result is non-zero,
            // or if there are any further words in the fraction.
            if (sum < fromThis)
                return sum || (index + 1) < std::max(m_values.size(), addend.m_values.size());

            // If the sum is uint32_t max, then we would carry a 1 if addition of the next
            // digits in the number were to overflow.
            if (sum != 0xFFFFFFFF)
                return false;
        }
        return false;
    }

private:
    bool checkConsistency() const
    {
        // All values should have at least one value.
        return (m_values.size())
            // The units value must be a uint16_t, or the value is the overflow value.
            && (m_values[0] < oneGreaterThanMaxUInt16 || (m_values[0] == oneGreaterThanMaxUInt16 && m_values.size() == 1))
            // There should be no trailing zeros (unless this value is zero!).
            && (m_values.last() || m_values.size() == 1);
    }

    // The internal storage of the number. This vector is always at least one entry in size,
    // with the first entry holding the portion of the number greater than zero. The first
    // value always hold a value in the uint16_t range, or holds the value oneGreaterThanMaxUInt16 to
    // indicate the value has overflowed to >= 0x10000. If the units value is oneGreaterThanMaxUInt16,
    // there can be no fraction (size must be 1).
    //
    // Subsequent values in the array represent portions of the fractional part of this number.
    // The total value of the number is the sum of (m_values[i] / pow(2^32, i)), for each i
    // in the array. The vector should contain no trailing zeros, except for the value '0',
    // represented by a vector contianing a single zero value. These constraints are checked
    // by 'checkConsistency()', above.
    //
    // The inline capacity of the vector is set to be able to contain any IEEE double (1 for
    // the units column, 32 for zeros introduced due to an exponent up to -3FE, and 2 for
    // bits taken from the mantissa).
    Vector<uint32_t, 36> m_values;

    // Cache a count of the number of leading zeros in m_values. We can use this to optimize
    // methods that would otherwise need visit all words in the vector, e.g. multiplication.
    size_t m_leadingZeros;
};

}

#endif

