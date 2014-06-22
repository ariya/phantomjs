//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// mathutil.h: Math and bit manipulation functions.

#ifndef LIBGLESV2_MATHUTIL_H_
#define LIBGLESV2_MATHUTIL_H_

#include <intrin.h>

#include "common/system.h"
#include "common/debug.h"

namespace gl
{
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

}

namespace rx
{

struct Range
{
    Range() {}
    Range(int lo, int hi) : start(lo), end(hi) { ASSERT(lo <= hi); }

    int start;
    int end;
};

}

#endif   // LIBGLESV2_MATHUTIL_H_
