//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// mathutil.h: Math and bit manipulation functions.

#ifndef LIBGLESV2_MATHUTIL_H_
#define LIBGLESV2_MATHUTIL_H_

#include "common/debug.h"
#include "common/platform.h"

#include <limits>
#include <algorithm>
#include <string.h>

namespace gl
{

const unsigned int Float32One = 0x3F800000;
const unsigned short Float16One = 0x3C00;

struct Vector4
{
    Vector4() {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    float x;
    float y;
    float z;
    float w;
};

inline bool isPow2(int x)
{
    return (x & (x - 1)) == 0 && (x != 0);
}

inline int log2(int x)
{
    int r = 0;
    while ((x >> r) > 1) r++;
    return r;
}

inline unsigned int ceilPow2(unsigned int x)
{
    if (x != 0) x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

inline int clampToInt(unsigned int x)
{
    return static_cast<int>(std::min(x, static_cast<unsigned int>(std::numeric_limits<int>::max())));
}

template <typename DestT, typename SrcT>
inline DestT clampCast(SrcT value)
{
    // This assumes SrcT can properly represent DestT::min/max
    // Unfortunately we can't use META_ASSERT without C++11 constexpr support
    ASSERT(static_cast<DestT>(static_cast<SrcT>(std::numeric_limits<DestT>::min())) == std::numeric_limits<DestT>::min());
    ASSERT(static_cast<DestT>(static_cast<SrcT>(std::numeric_limits<DestT>::max())) == std::numeric_limits<DestT>::max());

    SrcT lo = static_cast<SrcT>(std::numeric_limits<DestT>::min());
    SrcT hi = static_cast<SrcT>(std::numeric_limits<DestT>::max());
    return static_cast<DestT>(value > lo ? (value > hi ? hi : value) : lo);
}

template<typename T, typename MIN, typename MAX>
inline T clamp(T x, MIN min, MAX max)
{
    // Since NaNs fail all comparison tests, a NaN value will default to min
    return x > min ? (x > max ? max : x) : min;
}

inline float clamp01(float x)
{
    return clamp(x, 0.0f, 1.0f);
}

template<const int n>
inline unsigned int unorm(float x)
{
    const unsigned int max = 0xFFFFFFFF >> (32 - n);

    if (x > 1)
    {
        return max;
    }
    else if (x < 0)
    {
        return 0;
    }
    else
    {
        return (unsigned int)(max * x + 0.5f);
    }
}

inline bool supportsSSE2()
{
#if defined(ANGLE_PLATFORM_WINDOWS) && !defined(_M_ARM)
    static bool checked = false;
    static bool supports = false;

    if (checked)
    {
        return supports;
    }

    int info[4];
    __cpuid(info, 0);

    if (info[0] >= 1)
    {
        __cpuid(info, 1);

        supports = (info[3] >> 26) & 1;
    }

    checked = true;

    return supports;
#else
    UNIMPLEMENTED();
    return false;
#endif
}

template <typename destType, typename sourceType>
destType bitCast(const sourceType &source)
{
    size_t copySize = std::min(sizeof(destType), sizeof(sourceType));
    destType output;
    memcpy(&output, &source, copySize);
    return output;
}

inline unsigned short float32ToFloat16(float fp32)
{
    unsigned int fp32i = (unsigned int&)fp32;
    unsigned int sign = (fp32i & 0x80000000) >> 16;
    unsigned int abs = fp32i & 0x7FFFFFFF;

    if(abs > 0x47FFEFFF)   // Infinity
    {
        return sign | 0x7FFF;
    }
    else if(abs < 0x38800000)   // Denormal
    {
        unsigned int mantissa = (abs & 0x007FFFFF) | 0x00800000;
        int e = 113 - (abs >> 23);

        if(e < 24)
        {
            abs = mantissa >> e;
        }
        else
        {
            abs = 0;
        }

        return sign | (abs + 0x00000FFF + ((abs >> 13) & 1)) >> 13;
    }
    else
    {
        return sign | (abs + 0xC8000000 + 0x00000FFF + ((abs >> 13) & 1)) >> 13;
    }
}

float float16ToFloat32(unsigned short h);

unsigned int convertRGBFloatsTo999E5(float red, float green, float blue);
void convert999E5toRGBFloats(unsigned int input, float *red, float *green, float *blue);

inline unsigned short float32ToFloat11(float fp32)
{
    const unsigned int float32MantissaMask = 0x7FFFFF;
    const unsigned int float32ExponentMask = 0x7F800000;
    const unsigned int float32SignMask = 0x80000000;
    const unsigned int float32ValueMask = ~float32SignMask;
    const unsigned int float32ExponentFirstBit = 23;
    const unsigned int float32ExponentBias = 127;

    const unsigned short float11Max = 0x7BF;
    const unsigned short float11MantissaMask = 0x3F;
    const unsigned short float11ExponentMask = 0x7C0;
    const unsigned short float11BitMask = 0x7FF;
    const unsigned int float11ExponentBias = 14;

    const unsigned int float32Maxfloat11 = 0x477E0000;
    const unsigned int float32Minfloat11 = 0x38800000;

    const unsigned int float32Bits = bitCast<unsigned int>(fp32);
    const bool float32Sign = (float32Bits & float32SignMask) == float32SignMask;

    unsigned int float32Val = float32Bits & float32ValueMask;

    if ((float32Val & float32ExponentMask) == float32ExponentMask)
    {
        // INF or NAN
        if ((float32Val & float32MantissaMask) != 0)
        {
            return float11ExponentMask | (((float32Val >> 17) | (float32Val >> 11) | (float32Val >> 6) | (float32Val)) & float11MantissaMask);
        }
        else if (float32Sign)
        {
            // -INF is clamped to 0 since float11 is positive only
            return 0;
        }
        else
        {
            return float11ExponentMask;
        }
    }
    else if (float32Sign)
    {
        // float11 is positive only, so clamp to zero
        return 0;
    }
    else if (float32Val > float32Maxfloat11)
    {
        // The number is too large to be represented as a float11, set to max
        return float11Max;
    }
    else
    {
        if (float32Val < float32Minfloat11)
        {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            const unsigned int shift = (float32ExponentBias - float11ExponentBias) - (float32Val >> float32ExponentFirstBit);
            float32Val = ((1 << float32ExponentFirstBit) | (float32Val & float32MantissaMask)) >> shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized float11
            float32Val += 0xC8000000;
        }

        return ((float32Val + 0xFFFF + ((float32Val >> 17) & 1)) >> 17) & float11BitMask;
    }
}

inline unsigned short float32ToFloat10(float fp32)
{
    const unsigned int float32MantissaMask = 0x7FFFFF;
    const unsigned int float32ExponentMask = 0x7F800000;
    const unsigned int float32SignMask = 0x80000000;
    const unsigned int float32ValueMask = ~float32SignMask;
    const unsigned int float32ExponentFirstBit = 23;
    const unsigned int float32ExponentBias = 127;

    const unsigned short float10Max = 0x3DF;
    const unsigned short float10MantissaMask = 0x1F;
    const unsigned short float10ExponentMask = 0x3E0;
    const unsigned short float10BitMask = 0x3FF;
    const unsigned int float10ExponentBias = 14;

    const unsigned int float32Maxfloat10 = 0x477C0000;
    const unsigned int float32Minfloat10 = 0x38800000;

    const unsigned int float32Bits = bitCast<unsigned int>(fp32);
    const bool float32Sign = (float32Bits & float32SignMask) == float32SignMask;

    unsigned int float32Val = float32Bits & float32ValueMask;

    if ((float32Val & float32ExponentMask) == float32ExponentMask)
    {
        // INF or NAN
        if ((float32Val & float32MantissaMask) != 0)
        {
            return float10ExponentMask | (((float32Val >> 18) | (float32Val >> 13) | (float32Val >> 3) | (float32Val)) & float10MantissaMask);
        }
        else if (float32Sign)
        {
            // -INF is clamped to 0 since float11 is positive only
            return 0;
        }
        else
        {
            return float10ExponentMask;
        }
    }
    else if (float32Sign)
    {
        // float10 is positive only, so clamp to zero
        return 0;
    }
    else if (float32Val > float32Maxfloat10)
    {
        // The number is too large to be represented as a float11, set to max
        return float10Max;
    }
    else
    {
        if (float32Val < float32Minfloat10)
        {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            const unsigned int shift = (float32ExponentBias - float10ExponentBias) - (float32Val >> float32ExponentFirstBit);
            float32Val = ((1 << float32ExponentFirstBit) | (float32Val & float32MantissaMask)) >> shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized float11
            float32Val += 0xC8000000;
        }

        return ((float32Val + 0x1FFFF + ((float32Val >> 18) & 1)) >> 18) & float10BitMask;
    }
}

inline float float11ToFloat32(unsigned short fp11)
{
    unsigned short exponent = (fp11 >> 6) & 0x1F;
    unsigned short mantissa = fp11 & 0x3F;

    if (exponent == 0x1F)
    {
        // INF or NAN
        return bitCast<float>(0x7f800000 | (mantissa << 17));
    }
    else
    {
        if (exponent != 0)
        {
            // normalized
        }
        else if (mantissa != 0)
        {
            // The value is denormalized
            exponent = 1;

            do
            {
                exponent--;
                mantissa <<= 1;
            }
            while ((mantissa & 0x40) == 0);

            mantissa = mantissa & 0x3F;
        }
        else // The value is zero
        {
            exponent = -112;
        }

        return bitCast<float>(((exponent + 112) << 23) | (mantissa << 17));
    }
}

inline float float10ToFloat32(unsigned short fp11)
{
    unsigned short exponent = (fp11 >> 5) & 0x1F;
    unsigned short mantissa = fp11 & 0x1F;

    if (exponent == 0x1F)
    {
        // INF or NAN
        return bitCast<float>(0x7f800000 | (mantissa << 17));
    }
    else
    {
        if (exponent != 0)
        {
            // normalized
        }
        else if (mantissa != 0)
        {
            // The value is denormalized
            exponent = 1;

            do
            {
                exponent--;
                mantissa <<= 1;
            }
            while ((mantissa & 0x20) == 0);

            mantissa = mantissa & 0x1F;
        }
        else // The value is zero
        {
            exponent = -112;
        }

        return bitCast<float>(((exponent + 112) << 23) | (mantissa << 18));
    }
}

template <typename T>
inline float normalizedToFloat(T input)
{
    META_ASSERT(std::numeric_limits<T>::is_integer);

    const float inverseMax = 1.0f / std::numeric_limits<T>::max();
    return input * inverseMax;
}

template <unsigned int inputBitCount, typename T>
inline float normalizedToFloat(T input)
{
    META_ASSERT(std::numeric_limits<T>::is_integer);
    META_ASSERT(inputBitCount < (sizeof(T) * 8));

    const float inverseMax = 1.0f / ((1 << inputBitCount) - 1);
    return input * inverseMax;
}

template <typename T>
inline T floatToNormalized(float input)
{
    return std::numeric_limits<T>::max() * input + 0.5f;
}

template <unsigned int outputBitCount, typename T>
inline T floatToNormalized(float input)
{
    META_ASSERT(outputBitCount < (sizeof(T) * 8));
    return ((1 << outputBitCount) - 1) * input + 0.5f;
}

template <unsigned int inputBitCount, unsigned int inputBitStart, typename T>
inline T getShiftedData(T input)
{
    META_ASSERT(inputBitCount + inputBitStart <= (sizeof(T) * 8));
    const T mask = (1 << inputBitCount) - 1;
    return (input >> inputBitStart) & mask;
}

template <unsigned int inputBitCount, unsigned int inputBitStart, typename T>
inline T shiftData(T input)
{
    META_ASSERT(inputBitCount + inputBitStart <= (sizeof(T) * 8));
    const T mask = (1 << inputBitCount) - 1;
    return (input & mask) << inputBitStart;
}


inline unsigned char average(unsigned char a, unsigned char b)
{
    return ((a ^ b) >> 1) + (a & b);
}

inline signed char average(signed char a, signed char b)
{
    return ((short)a + (short)b) / 2;
}

inline unsigned short average(unsigned short a, unsigned short b)
{
    return ((a ^ b) >> 1) + (a & b);
}

inline signed short average(signed short a, signed short b)
{
    return ((int)a + (int)b) / 2;
}

inline unsigned int average(unsigned int a, unsigned int b)
{
    return ((a ^ b) >> 1) + (a & b);
}

inline signed int average(signed int a, signed int b)
{
    return ((long long)a + (long long)b) / 2;
}

inline float average(float a, float b)
{
    return (a + b) * 0.5f;
}

inline unsigned short averageHalfFloat(unsigned short a, unsigned short b)
{
    return float32ToFloat16((float16ToFloat32(a) + float16ToFloat32(b)) * 0.5f);
}

inline unsigned int averageFloat11(unsigned int a, unsigned int b)
{
    return float32ToFloat11((float11ToFloat32(a) + float11ToFloat32(b)) * 0.5f);
}

inline unsigned int averageFloat10(unsigned int a, unsigned int b)
{
    return float32ToFloat10((float10ToFloat32(a) + float10ToFloat32(b)) * 0.5f);
}

}

namespace rx
{

template <typename T>
struct Range
{
    Range() {}
    Range(T lo, T hi) : start(lo), end(hi) { ASSERT(lo <= hi); }

    T start;
    T end;

    T length() const { return end - start; }
};

typedef Range<int> RangeI;
typedef Range<unsigned int> RangeUI;

template <typename T>
T roundUp(const T value, const T alignment)
{
    return value + alignment - 1 - (value - 1) % alignment;
}

inline unsigned int UnsignedCeilDivide(unsigned int value, unsigned int divisor)
{
    unsigned int divided = value / divisor;
    return (divided + ((value % divisor == 0) ? 0 : 1));
}

template <class T>
inline bool IsUnsignedAdditionSafe(T lhs, T rhs)
{
    META_ASSERT(!std::numeric_limits<T>::is_signed);
    return (rhs <= std::numeric_limits<T>::max() - lhs);
}

template <class T>
inline bool IsUnsignedMultiplicationSafe(T lhs, T rhs)
{
    META_ASSERT(!std::numeric_limits<T>::is_signed);
    return (lhs == T(0) || rhs == T(0) || (rhs <= std::numeric_limits<T>::max() / lhs));
}

template <class SmallIntT, class BigIntT>
inline bool IsIntegerCastSafe(BigIntT bigValue)
{
    return (static_cast<BigIntT>(static_cast<SmallIntT>(bigValue)) == bigValue);
}

}

#endif   // LIBGLESV2_MATHUTIL_H_
