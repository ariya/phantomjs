/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef WTF_MathExtras_h
#define WTF_MathExtras_h

#include <algorithm>
#include <cmath>
#include <float.h>
#include <limits>
#include <stdlib.h>

#if OS(QNX)
#include <math.h>
#endif

#if OS(SOLARIS)
#include <ieeefp.h>
#endif

#if OS(OPENBSD)
#include <sys/types.h>
#include <machine/ieee.h>
#endif

#if COMPILER(MSVC)
#if OS(WINCE)
#include <stdlib.h>
#endif
#include <limits>
#endif

#ifndef M_PI
const double piDouble = 3.14159265358979323846;
const float piFloat = 3.14159265358979323846f;
#else
const double piDouble = M_PI;
const float piFloat = static_cast<float>(M_PI);
#endif

#ifndef M_PI_2
const double piOverTwoDouble = 1.57079632679489661923;
const float piOverTwoFloat = 1.57079632679489661923f;
#else
const double piOverTwoDouble = M_PI_2;
const float piOverTwoFloat = static_cast<float>(M_PI_2);
#endif

#ifndef M_PI_4
const double piOverFourDouble = 0.785398163397448309616;
const float piOverFourFloat = 0.785398163397448309616f;
#else
const double piOverFourDouble = M_PI_4;
const float piOverFourFloat = static_cast<float>(M_PI_4);
#endif

#if OS(DARWIN)

// Work around a bug in the Mac OS X libc where ceil(-0.1) return +0.
inline double wtf_ceil(double x) { return copysign(ceil(x), x); }

#define ceil(x) wtf_ceil(x)

#endif

#if OS(SOLARIS)

#ifndef isfinite
inline bool isfinite(double x) { return finite(x) && !isnand(x); }
#endif
#ifndef isinf
inline bool isinf(double x) { return !finite(x) && !isnand(x); }
#endif
#ifndef signbit
inline bool signbit(double x) { return copysign(1.0, x) < 0; }
#endif

#endif

#if OS(OPENBSD)

#ifndef isfinite
inline bool isfinite(double x) { return finite(x); }
#endif
#ifndef signbit
inline bool signbit(double x) { struct ieee_double *p = (struct ieee_double *)&x; return p->dbl_sign; }
#endif

#endif

#if COMPILER(MSVC) || (COMPILER(RVCT) && !(RVCT_VERSION_AT_LEAST(3, 0, 0, 0)))

// We must not do 'num + 0.5' or 'num - 0.5' because they can cause precision loss.
static double round(double num)
{
    double integer = ceil(num);
    if (num > 0)
        return integer - num > 0.5 ? integer - 1.0 : integer;
    return integer - num >= 0.5 ? integer - 1.0 : integer;
}
static float roundf(float num)
{
    float integer = ceilf(num);
    if (num > 0)
        return integer - num > 0.5f ? integer - 1.0f : integer;
    return integer - num >= 0.5f ? integer - 1.0f : integer;
}
inline long long llround(double num) { return static_cast<long long>(round(num)); }
inline long long llroundf(float num) { return static_cast<long long>(roundf(num)); }
inline long lround(double num) { return static_cast<long>(round(num)); }
inline long lroundf(float num) { return static_cast<long>(roundf(num)); }
inline double trunc(double num) { return num > 0 ? floor(num) : ceil(num); }

#endif

#if COMPILER(MSVC)
// The 64bit version of abs() is already defined in stdlib.h which comes with VC10
#if COMPILER(MSVC9_OR_LOWER)
inline long long abs(long long num) { return _abs64(num); }
#endif

inline bool isinf(double num) { return !_finite(num) && !_isnan(num); }
inline bool isnan(double num) { return !!_isnan(num); }
inline bool signbit(double num) { return _copysign(1.0, num) < 0; }

inline double nextafter(double x, double y) { return _nextafter(x, y); }
inline float nextafterf(float x, float y) { return x > y ? x - FLT_EPSILON : x + FLT_EPSILON; }

inline double copysign(double x, double y) { return _copysign(x, y); }
inline int isfinite(double x) { return _finite(x); }

// MSVC's math.h does not currently supply log2.
inline double log2(double num)
{
    // This constant is roughly M_LN2, which is not provided by default on Windows.
    return log(num) / 0.693147180559945309417232121458176568;
}

// Work around a bug in Win, where atan2(+-infinity, +-infinity) yields NaN instead of specific values.
inline double wtf_atan2(double x, double y)
{
    double posInf = std::numeric_limits<double>::infinity();
    double negInf = -std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();

    double result = nan;

    if (x == posInf && y == posInf)
        result = piOverFourDouble;
    else if (x == posInf && y == negInf)
        result = 3 * piOverFourDouble;
    else if (x == negInf && y == posInf)
        result = -piOverFourDouble;
    else if (x == negInf && y == negInf)
        result = -3 * piOverFourDouble;
    else
        result = ::atan2(x, y);

    return result;
}

// Work around a bug in the Microsoft CRT, where fmod(x, +-infinity) yields NaN instead of x.
inline double wtf_fmod(double x, double y) { return (!isinf(x) && isinf(y)) ? x : fmod(x, y); }

// Work around a bug in the Microsoft CRT, where pow(NaN, 0) yields NaN instead of 1.
inline double wtf_pow(double x, double y) { return y == 0 ? 1 : pow(x, y); }

#define atan2(x, y) wtf_atan2(x, y)
#define fmod(x, y) wtf_fmod(x, y)
#define pow(x, y) wtf_pow(x, y)

#endif // COMPILER(MSVC)

inline double deg2rad(double d)  { return d * piDouble / 180.0; }
inline double rad2deg(double r)  { return r * 180.0 / piDouble; }
inline double deg2grad(double d) { return d * 400.0 / 360.0; }
inline double grad2deg(double g) { return g * 360.0 / 400.0; }
inline double turn2deg(double t) { return t * 360.0; }
inline double deg2turn(double d) { return d / 360.0; }
inline double rad2grad(double r) { return r * 200.0 / piDouble; }
inline double grad2rad(double g) { return g * piDouble / 200.0; }

inline float deg2rad(float d)  { return d * piFloat / 180.0f; }
inline float rad2deg(float r)  { return r * 180.0f / piFloat; }
inline float deg2grad(float d) { return d * 400.0f / 360.0f; }
inline float grad2deg(float g) { return g * 360.0f / 400.0f; }
inline float turn2deg(float t) { return t * 360.0f; }
inline float deg2turn(float d) { return d / 360.0f; }
inline float rad2grad(float r) { return r * 200.0f / piFloat; }
inline float grad2rad(float g) { return g * piFloat / 200.0f; }

inline int clampToInteger(double d)
{
    const double minIntAsDouble = std::numeric_limits<int>::min();
    const double maxIntAsDouble = std::numeric_limits<int>::max();
    return static_cast<int>(std::max(std::min(d, maxIntAsDouble), minIntAsDouble));
}

inline int clampToPositiveInteger(double d)
{
    const double maxIntAsDouble = std::numeric_limits<int>::max();
    return static_cast<int>(std::max<double>(std::min(d, maxIntAsDouble), 0));
}

inline int clampToInteger(float x)
{
    static const int s_intMax = std::numeric_limits<int>::max();
    static const int s_intMin = std::numeric_limits<int>::min();
    
    if (x >= static_cast<float>(s_intMax))
        return s_intMax;
    if (x < static_cast<float>(s_intMin))
        return s_intMin;
    return static_cast<int>(x);
}

inline int clampToPositiveInteger(float x)
{
    static const int s_intMax = std::numeric_limits<int>::max();
    
    if (x >= static_cast<float>(s_intMax))
        return s_intMax;
    if (x < 0)
        return 0;
    return static_cast<int>(x);
}

inline int clampToInteger(unsigned value)
{
    return static_cast<int>(std::min(value, static_cast<unsigned>(std::numeric_limits<int>::max())));
}

#if !COMPILER(MSVC) && !(COMPILER(RVCT) && PLATFORM(BREWMP)) && !OS(SOLARIS) && !OS(SYMBIAN)
using std::isfinite;
using std::isinf;
using std::isnan;
using std::signbit;
#endif

#endif // #ifndef WTF_MathExtras_h
