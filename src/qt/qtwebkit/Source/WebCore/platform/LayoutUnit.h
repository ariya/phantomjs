/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LayoutUnit_h
#define LayoutUnit_h

#include <limits.h>
#include <limits>
#include <math.h>
#include <stdlib.h>
#include <wtf/MathExtras.h>
#include <wtf/SaturatedArithmetic.h>

namespace WebCore {

#ifdef NDEBUG

#define REPORT_OVERFLOW(doesOverflow) ((void)0)

#else

#define REPORT_OVERFLOW(doesOverflow) do \
    if (!(doesOverflow)) { \
        WTFReportError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, "!(%s)", #doesOverflow); \
    } \
while (0)

#endif

static const int kFixedPointDenominator = 64;

#if ENABLE(SUBPIXEL_LAYOUT)
static const int kEffectiveFixedPointDenominator = kFixedPointDenominator;
#else
static const int kEffectiveFixedPointDenominator = 1;
#endif
const int intMaxForLayoutUnit = INT_MAX / kEffectiveFixedPointDenominator;
const int intMinForLayoutUnit = INT_MIN / kEffectiveFixedPointDenominator;

class LayoutUnit {
public:
    LayoutUnit() : m_value(0) { }
#if ENABLE(SUBPIXEL_LAYOUT)
    LayoutUnit(int value) { setValue(value); }
    LayoutUnit(unsigned short value) { setValue(value); }
    LayoutUnit(unsigned value) { setValue(value); }
    LayoutUnit(unsigned long value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        m_value = clampTo<int>(value * kEffectiveFixedPointDenominator);
#else
        REPORT_OVERFLOW(isInBounds(static_cast<unsigned>(value)));
        m_value = value * kEffectiveFixedPointDenominator;
#endif
    }
    LayoutUnit(unsigned long long value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        m_value = clampTo<int>(value * kEffectiveFixedPointDenominator);
#else
        REPORT_OVERFLOW(isInBounds(static_cast<unsigned>(value)));
        m_value = static_cast<int>(value * kEffectiveFixedPointDenominator);
#endif
    }
    LayoutUnit(float value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        m_value = clampTo<float>(value * kEffectiveFixedPointDenominator, static_cast<float>(INT_MIN), static_cast<float>(INT_MAX));
#else
        REPORT_OVERFLOW(isInBounds(value));
        m_value = value * kEffectiveFixedPointDenominator;
#endif
    }
    LayoutUnit(double value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        m_value = clampTo<double>(value * kEffectiveFixedPointDenominator, static_cast<double>(INT_MIN), static_cast<double>(INT_MAX));
#else
        REPORT_OVERFLOW(isInBounds(value));
        m_value = value * kEffectiveFixedPointDenominator;
#endif
    }
#else
    LayoutUnit(int value) { REPORT_OVERFLOW(isInBounds(value)); m_value = value; }
    LayoutUnit(unsigned short value) { REPORT_OVERFLOW(isInBounds(value)); m_value = value; }
    LayoutUnit(unsigned value) { REPORT_OVERFLOW(isInBounds(value)); m_value = clampTo<int>(value); }
    LayoutUnit(unsigned long long value) { REPORT_OVERFLOW(isInBounds(static_cast<unsigned>(value))); m_value = clampTo<int>(value); }
    LayoutUnit(unsigned long value) { REPORT_OVERFLOW(isInBounds(static_cast<unsigned>(value))); m_value = clampTo<int>(value); }
    LayoutUnit(float value) { REPORT_OVERFLOW(isInBounds(value)); m_value = clampTo<int>(value); }
    LayoutUnit(double value) { REPORT_OVERFLOW(isInBounds(value)); m_value = clampTo<int>(value); }
#endif

    static LayoutUnit fromFloatCeil(float value)
    {
        LayoutUnit v;
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        v.m_value = clampToInteger(ceilf(value * kEffectiveFixedPointDenominator));
#else
        REPORT_OVERFLOW(isInBounds(value));
        v.m_value = ceilf(value * kEffectiveFixedPointDenominator);
#endif
        return v;
    }

    static LayoutUnit fromFloatFloor(float value)
    {
        LayoutUnit v;
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        v.m_value = clampToInteger(floorf(value * kEffectiveFixedPointDenominator));
#else
        REPORT_OVERFLOW(isInBounds(value));
        v.m_value = floorf(value * kEffectiveFixedPointDenominator);
#endif
        return v;
    }

    static LayoutUnit fromFloatRound(float value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (value >= 0)
            return clamp(value + epsilon() / 2.0f);
        return clamp(value - epsilon() / 2.0f);
#else
        if (value >= 0) {
            REPORT_OVERFLOW(isInBounds(value + epsilon() / 2.0f));
            return LayoutUnit(value + epsilon() / 2.0f);
        }
        REPORT_OVERFLOW(isInBounds(value - epsilon() / 2.0f));
        return LayoutUnit(value - epsilon() / 2.0f);
#endif
    }

#if ENABLE(SUBPIXEL_LAYOUT)
    int toInt() const { return m_value / kEffectiveFixedPointDenominator; }
    float toFloat() const { return static_cast<float>(m_value) / kEffectiveFixedPointDenominator; }
    double toDouble() const { return static_cast<double>(m_value) / kEffectiveFixedPointDenominator; }
    float ceilToFloat() const
    {
        float floatValue = toFloat();
        if (static_cast<int>(floatValue * kEffectiveFixedPointDenominator) == m_value)
            return floatValue;
        if (floatValue > 0)
            return nextafterf(floatValue, std::numeric_limits<float>::max());
        return nextafterf(floatValue, std::numeric_limits<float>::min());
    }
#else
    int toInt() const { return m_value; }
    float toFloat() const { return static_cast<float>(m_value); }
    double toDouble() const { return static_cast<double>(m_value); }
    float ceilToFloat() const { return toFloat(); }
#endif
    unsigned toUnsigned() const { REPORT_OVERFLOW(m_value >= 0); return toInt(); }

    operator int() const { return toInt(); }
    operator unsigned() const { return toUnsigned(); }
    operator float() const { return toFloat(); }
    operator double() const { return toDouble(); }
    operator bool() const { return m_value; }

    LayoutUnit operator++(int)
    {
        m_value += kEffectiveFixedPointDenominator;
        return *this;
    }

    inline int rawValue() const { return m_value; }
    inline void setRawValue(int value) { m_value = value; }
    void setRawValue(long long value)
    {
        REPORT_OVERFLOW(value > std::numeric_limits<int>::min() && value < std::numeric_limits<int>::max());
        m_value = static_cast<int>(value);
    }

    LayoutUnit abs() const
    {
        LayoutUnit returnValue;
        returnValue.setRawValue(::abs(m_value));
        return returnValue;
    }
#if OS(DARWIN)
    int wtf_ceil() const
#else
    int ceil() const
#endif
    {
#if ENABLE(SUBPIXEL_LAYOUT)
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (UNLIKELY(m_value >= INT_MAX - kEffectiveFixedPointDenominator + 1))
            return intMaxForLayoutUnit;
#endif
        if (m_value >= 0)
            return (m_value + kEffectiveFixedPointDenominator - 1) / kEffectiveFixedPointDenominator;
        return toInt();
#else
        return m_value;
#endif
    }
    int round() const
    {
#if ENABLE(SUBPIXEL_LAYOUT) && ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (m_value > 0)
            return saturatedAddition(rawValue(), kEffectiveFixedPointDenominator / 2) / kEffectiveFixedPointDenominator;
        return saturatedSubtraction(rawValue(), (kEffectiveFixedPointDenominator / 2) - 1) / kEffectiveFixedPointDenominator;
#elif ENABLE(SUBPIXEL_LAYOUT)
        if (m_value > 0)
            return (m_value + (kEffectiveFixedPointDenominator / 2)) / kEffectiveFixedPointDenominator;
        return (m_value - ((kEffectiveFixedPointDenominator / 2) - 1)) / kEffectiveFixedPointDenominator;
#else
        return m_value;
#endif
    }

    int floor() const
    {
#if ENABLE(SUBPIXEL_LAYOUT)
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (UNLIKELY(m_value <= INT_MIN + kEffectiveFixedPointDenominator - 1))
            return intMinForLayoutUnit;
#endif
        if (m_value >= 0)
            return toInt();
        return (m_value - kEffectiveFixedPointDenominator + 1) / kEffectiveFixedPointDenominator;
#else
        return m_value;
#endif
    }

    LayoutUnit fraction() const
    {   
        // Add the fraction to the size (as opposed to the full location) to avoid overflows.
        // Compute fraction using the mod operator to preserve the sign of the value as it may affect rounding.
        LayoutUnit fraction;
        fraction.setRawValue(rawValue() % kEffectiveFixedPointDenominator);
        return fraction;
    }

#if ENABLE(SUBPIXEL_LAYOUT)
    bool mightBeSaturated() const
    {
        return rawValue() == std::numeric_limits<int>::max()
            || rawValue() == std::numeric_limits<int>::min();
    }

    static float epsilon() { return 1.0f / kEffectiveFixedPointDenominator; }
#else
    static int epsilon() { return 0; }
#endif
    static const LayoutUnit max()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::max();
        return m;
    }
    static const LayoutUnit min()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::min();
        return m;
    }

    // Versions of max/min that are slightly smaller/larger than max/min() to allow for roinding without overflowing.
    static const LayoutUnit nearlyMax()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::max() - kEffectiveFixedPointDenominator / 2;
        return m;
    }
    static const LayoutUnit nearlyMin()
    {
        LayoutUnit m;
        m.m_value = std::numeric_limits<int>::min() + kEffectiveFixedPointDenominator / 2;
        return m;
    }
    
    static LayoutUnit clamp(double value)
    {
        return clampTo<LayoutUnit>(value, LayoutUnit::min(), LayoutUnit::max());
    }

private:
    static bool isInBounds(int value)
    {
        return ::abs(value) <= std::numeric_limits<int>::max() / kEffectiveFixedPointDenominator;
    }
    static bool isInBounds(unsigned value)
    {
        return value <= static_cast<unsigned>(std::numeric_limits<int>::max()) / kEffectiveFixedPointDenominator;
    }
    static bool isInBounds(double value)
    {
        return ::fabs(value) <= std::numeric_limits<int>::max() / kEffectiveFixedPointDenominator;
    }
    
    inline void setValue(int value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (value > intMaxForLayoutUnit)
            m_value = std::numeric_limits<int>::max();
        else if (value < intMinForLayoutUnit)
            m_value = std::numeric_limits<int>::min();
        else
            m_value = value * kEffectiveFixedPointDenominator;
#else
        REPORT_OVERFLOW(isInBounds(value));
        m_value = value * kEffectiveFixedPointDenominator;
#endif
    }
    inline void setValue(unsigned value)
    {
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        if (value >= static_cast<unsigned>(intMaxForLayoutUnit))
            m_value = std::numeric_limits<int>::max();
        else
            m_value = value * kEffectiveFixedPointDenominator;
#else
        REPORT_OVERFLOW(isInBounds(value));
        m_value = value * kEffectiveFixedPointDenominator;
#endif
    }

    int m_value;
};

inline bool operator<=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() <= b.rawValue();
}

inline bool operator<=(const LayoutUnit& a, float b)
{
    return a.toFloat() <= b;
}

inline bool operator<=(const LayoutUnit& a, int b)
{
    return a <= LayoutUnit(b);
}

inline bool operator<=(const float a, const LayoutUnit& b)
{
    return a <= b.toFloat();
}

inline bool operator<=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) <= b;
}

inline bool operator>=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() >= b.rawValue();
}

inline bool operator>=(const LayoutUnit& a, int b)
{
    return a >= LayoutUnit(b);
}

inline bool operator>=(const float a, const LayoutUnit& b)
{
    return a >= b.toFloat();
}

inline bool operator>=(const LayoutUnit& a, float b)
{
    return a.toFloat() >= b;
}

inline bool operator>=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) >= b;
}

inline bool operator<(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() < b.rawValue();
}

inline bool operator<(const LayoutUnit& a, int b)
{
    return a < LayoutUnit(b);
}

inline bool operator<(const LayoutUnit& a, float b)
{
    return a.toFloat() < b;
}

inline bool operator<(const LayoutUnit& a, double b)
{
    return a.toDouble() < b;
}

inline bool operator<(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) < b;
}

inline bool operator<(const float a, const LayoutUnit& b)
{
    return a < b.toFloat();
}

inline bool operator>(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() > b.rawValue();
}

inline bool operator>(const LayoutUnit& a, double b)
{
    return a.toDouble() > b;
}

inline bool operator>(const LayoutUnit& a, float b)
{
    return a.toFloat() > b;
}

inline bool operator>(const LayoutUnit& a, int b)
{
    return a > LayoutUnit(b);
}

inline bool operator>(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) > b;
}

inline bool operator>(const float a, const LayoutUnit& b)
{
    return a > b.toFloat();
}

inline bool operator>(const double a, const LayoutUnit& b)
{
    return a > b.toDouble();
}

inline bool operator!=(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() != b.rawValue();
}

inline bool operator!=(const LayoutUnit& a, float b)
{
    return a != LayoutUnit(b);
}

inline bool operator!=(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) != b;
}

inline bool operator!=(const LayoutUnit& a, int b)
{
    return a != LayoutUnit(b);
}

inline bool operator==(const LayoutUnit& a, const LayoutUnit& b)
{
    return a.rawValue() == b.rawValue();
}

inline bool operator==(const LayoutUnit& a, int b)
{
    return a == LayoutUnit(b);
}

inline bool operator==(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) == b;
}

inline bool operator==(const LayoutUnit& a, float b)
{
    return a.toFloat() == b;
}

inline bool operator==(const float a, const LayoutUnit& b)
{
    return a == b.toFloat();
}

// For multiplication that's prone to overflow, this bounds it to LayoutUnit::max() and ::min()
inline LayoutUnit boundedMultiply(const LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    int64_t result = static_cast<int64_t>(a.rawValue()) * static_cast<int64_t>(b.rawValue()) / kEffectiveFixedPointDenominator;
    int32_t high = static_cast<int32_t>(result >> 32);
    int32_t low = static_cast<int32_t>(result);
    uint32_t saturated = (static_cast<uint32_t>(a.rawValue() ^ b.rawValue()) >> 31) + std::numeric_limits<int>::max();
    // If the higher 32 bits does not match the lower 32 with sign extension the operation overflowed.
    if (high != low >> 31)
        result = saturated;

    LayoutUnit returnVal;
    returnVal.setRawValue(static_cast<int>(result));
    return returnVal;
#else
    // FIXME: Should be bounded even in the non-subpixel case.
    return a.rawValue() * b.rawValue();
#endif
}

inline LayoutUnit operator*(const LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SUBPIXEL_LAYOUT) && ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    return boundedMultiply(a, b);
#elif ENABLE(SUBPIXEL_LAYOUT)
    LayoutUnit returnVal;
    long long rawVal = static_cast<long long>(a.rawValue()) * b.rawValue() / kEffectiveFixedPointDenominator;
    returnVal.setRawValue(rawVal);
    return returnVal;
#else
    return a.rawValue() * b.rawValue();
#endif
}    

inline double operator*(const LayoutUnit& a, double b)
{
    return a.toDouble() * b;
}

inline float operator*(const LayoutUnit& a, float b)
{
    return a.toFloat() * b;
}

inline LayoutUnit operator*(const LayoutUnit& a, int b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned short b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned long b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(const LayoutUnit& a, unsigned long long b)
{
    return a * LayoutUnit(b);
}

inline LayoutUnit operator*(unsigned short a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned long a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(unsigned long long a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline LayoutUnit operator*(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) * b;
}

inline float operator*(const float a, const LayoutUnit& b)
{
    return a * b.toFloat();
}

inline double operator*(const double a, const LayoutUnit& b)
{
    return a * b.toDouble();
}

inline LayoutUnit operator/(const LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    LayoutUnit returnVal;
    long long rawVal = static_cast<long long>(kEffectiveFixedPointDenominator) * a.rawValue() / b.rawValue();
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    returnVal.setRawValue(clampTo<int>(rawVal));
#else
    returnVal.setRawValue(rawVal);
#endif
    return returnVal;
#else
    return a.rawValue() / b.rawValue();
#endif
}    

inline float operator/(const LayoutUnit& a, float b)
{
    return a.toFloat() / b;
}

inline double operator/(const LayoutUnit& a, double b)
{
    return a.toDouble() / b;
}

inline LayoutUnit operator/(const LayoutUnit& a, int b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned short b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned long b)
{
    return a / LayoutUnit(b);
}

inline LayoutUnit operator/(const LayoutUnit& a, unsigned long long b)
{
    return a / LayoutUnit(b);
}

inline float operator/(const float a, const LayoutUnit& b)
{
    return a / b.toFloat();
}

inline double operator/(const double a, const LayoutUnit& b)
{
    return a / b.toDouble();
}

inline LayoutUnit operator/(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned short a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned long a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator/(unsigned long long a, const LayoutUnit& b)
{
    return LayoutUnit(a) / b;
}

inline LayoutUnit operator+(const LayoutUnit& a, const LayoutUnit& b)
{
    LayoutUnit returnVal;
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    returnVal.setRawValue(saturatedAddition(a.rawValue(), b.rawValue()));
#else
    returnVal.setRawValue(a.rawValue() + b.rawValue());
#endif
    return returnVal;
}

inline LayoutUnit operator+(const LayoutUnit& a, int b)
{
    return a + LayoutUnit(b);
}

inline float operator+(const LayoutUnit& a, float b)
{
    return a.toFloat() + b;
}

inline double operator+(const LayoutUnit& a, double b)
{
    return a.toDouble() + b;
}

inline LayoutUnit operator+(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) + b;
}

inline float operator+(const float a, const LayoutUnit& b)
{
    return a + b.toFloat();
}

inline double operator+(const double a, const LayoutUnit& b)
{
    return a + b.toDouble();
}

inline LayoutUnit operator-(const LayoutUnit& a, const LayoutUnit& b)
{
    LayoutUnit returnVal;
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    returnVal.setRawValue(saturatedSubtraction(a.rawValue(), b.rawValue()));
#else
    returnVal.setRawValue(a.rawValue() - b.rawValue());
#endif
    return returnVal;
}

inline LayoutUnit operator-(const LayoutUnit& a, int b)
{
    return a - LayoutUnit(b);
}

inline LayoutUnit operator-(const LayoutUnit& a, unsigned b)
{
    return a - LayoutUnit(b);
}

inline float operator-(const LayoutUnit& a, float b)
{
    return a.toFloat() - b;
}

inline LayoutUnit operator-(const int a, const LayoutUnit& b)
{
    return LayoutUnit(a) - b;
}

inline float operator-(const float a, const LayoutUnit& b)
{
    return a - b.toFloat();
}

inline LayoutUnit operator-(const LayoutUnit& a)
{
    LayoutUnit returnVal;
    returnVal.setRawValue(-a.rawValue());
    return returnVal;
}

// For returning the remainder after a division with integer results.
inline LayoutUnit intMod(const LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    // This calculates the modulo so that: a = static_cast<int>(a / b) * b + intMod(a, b).
    LayoutUnit returnVal;
    returnVal.setRawValue(a.rawValue() % b.rawValue());
    return returnVal;
#else
    return a.rawValue() % b.rawValue();
#endif
}

inline LayoutUnit operator%(const LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    // This calculates the modulo so that: a = (a / b) * b + a % b.
    LayoutUnit returnVal;
    long long rawVal = (static_cast<long long>(kEffectiveFixedPointDenominator) * a.rawValue()) % b.rawValue();
    returnVal.setRawValue(rawVal / kEffectiveFixedPointDenominator);
    return returnVal;
#else
    return a.rawValue() % b.rawValue();
#endif
}

inline LayoutUnit operator%(const LayoutUnit& a, int b)
{
    return a % LayoutUnit(b);
}

inline LayoutUnit operator%(int a, const LayoutUnit& b)
{
    return LayoutUnit(a) % b;
}

inline LayoutUnit& operator+=(LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    a.setRawValue(saturatedAddition(a.rawValue(), b.rawValue()));
#else
    a = a + b;
#endif
    return a;
}

inline LayoutUnit& operator+=(LayoutUnit& a, int b)
{
    a = a + b;
    return a;
}

inline LayoutUnit& operator+=(LayoutUnit& a, float b)
{
    a = a + b;
    return a;
}

inline float& operator+=(float& a, const LayoutUnit& b)
{
    a = a + b;
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, int b)
{
    a = a - b;
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, const LayoutUnit& b)
{
#if ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    a.setRawValue(saturatedSubtraction(a.rawValue(), b.rawValue()));
#else
    a = a - b;
#endif
    return a;
}

inline LayoutUnit& operator-=(LayoutUnit& a, float b)
{
    a = a - b;
    return a;
}

inline float& operator-=(float& a, const LayoutUnit& b)
{
    a = a - b;
    return a;
}

inline LayoutUnit& operator*=(LayoutUnit& a, const LayoutUnit& b)
{
    a = a * b;
    return a;
}
// operator*=(LayoutUnit& a, int b) is supported by the operator above plus LayoutUnit(int).

inline LayoutUnit& operator*=(LayoutUnit& a, float b)
{
    a = a * b;
    return a;
}

inline float& operator*=(float& a, const LayoutUnit& b)
{
    a = a * b;
    return a;
}

inline LayoutUnit& operator/=(LayoutUnit& a, const LayoutUnit& b)
{
    a = a / b;
    return a;
}
// operator/=(LayoutUnit& a, int b) is supported by the operator above plus LayoutUnit(int).

inline LayoutUnit& operator/=(LayoutUnit& a, float b)
{
    a = a / b;
    return a;
}

inline float& operator/=(float& a, const LayoutUnit& b)
{
    a = a / b;
    return a;
}

inline int snapSizeToPixel(LayoutUnit size, LayoutUnit location) 
{
    LayoutUnit fraction = location.fraction();
    return (fraction + size).round() - fraction.round();
}

inline int roundToInt(LayoutUnit value)
{
    return value.round();
}

inline int floorToInt(LayoutUnit value)
{
    return value.floor();
}

inline LayoutUnit roundedLayoutUnit(float value)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return LayoutUnit::fromFloatRound(value);
#else
    return static_cast<int>(lroundf(value));
#endif
}

inline LayoutUnit ceiledLayoutUnit(float value)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return LayoutUnit::fromFloatCeil(value);
#else
    return ceilf(value);
#endif
}

inline LayoutUnit absoluteValue(const LayoutUnit& value)
{
    return value.abs();
}

inline LayoutUnit layoutMod(const LayoutUnit& numerator, const LayoutUnit& denominator)
{
    return numerator % denominator;
}

inline bool isIntegerValue(const LayoutUnit value)
{
    return value.toInt() == value;
}

} // namespace WebCore

#endif // LayoutUnit_h
