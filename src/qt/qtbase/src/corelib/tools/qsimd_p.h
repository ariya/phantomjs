/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIMD_P_H
#define QSIMD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qatomic.h>

/*
 * qt_module_config.prf defines the QT_COMPILER_SUPPORTS_XXX macros.
 * They mean the compiler supports the necessary flags and the headers
 * for the x86 and ARM intrinsics:
 *  - GCC: the -mXXX or march=YYY flag is necessary before #include
 *    up to 4.8; GCC >= 4.9 can include unconditionally
 *  - Intel CC: #include can happen unconditionally
 *  - MSVC: #include can happen unconditionally
 *  - RVCT: ???
 *
 * We will try to include all headers possible under this configuration.
 *
 * MSVC does not define __SSE2__ & family, so we will define them. MSVC 2013 &
 * up do define __AVX__ if the -arch:AVX option is passed on the command-line.
 *
 * Supported XXX are:
 *   Flag    | Arch |  GCC  | Intel CC |  MSVC  |
 *  ARM_NEON | ARM  | I & C | None     |   ?    |
 *  SSE2     | x86  | I & C | I & C    | I & C  |
 *  SSE3     | x86  | I & C | I & C    | I only |
 *  SSSE3    | x86  | I & C | I & C    | I only |
 *  SSE4_1   | x86  | I & C | I & C    | I only |
 *  SSE4_2   | x86  | I & C | I & C    | I only |
 *  AVX      | x86  | I & C | I & C    | I & C  |
 *  AVX2     | x86  | I & C | I & C    | I only |
 * I = intrinsics; C = code generation
 *
 * Code can use the following constructs to determine compiler support & status:
 * - #ifdef __XXX__      (e.g: #ifdef __AVX__  or #ifdef __ARM_NEON__)
 *   If this test passes, then the compiler is already generating code for that
 *   given sub-architecture. The intrinsics for that sub-architecture are
 *   #included and can be used without restriction or runtime check.
 *
 * - #if QT_COMPILER_SUPPORTS(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in another translation unit, given the right set of
 *   flags. Use of the intrinsics is not guaranteed. This is useful with
 *   runtime detection (see below).
 *
 * - #if QT_COMPILER_SUPPORTS_HERE(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in this translation unit, even if it is not doing
 *   that now (it might be). Individual functions may be tagged with
 *   QT_FUNCTION_TARGET(XXX) to cause the compiler to generate code for that
 *   sub-arch. Only inside such functions is the use of the intrisics
 *   guaranteed to work. This is useful with runtime detection (see below).
 *
 * Runtime detection of a CPU sub-architecture can be done with the
 * qCpuHasFeature(XXX) function. There are two strategies for generating
 * optimized code like that:
 *
 * 1) place the optimized code in a different translation unit (C or assembly
 * sources) and pass the correct flags to the compiler to enable support. Those
 * sources must not include qglobal.h, which means they cannot include this
 * file either. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if QT_COMPILER_SUPPORTS(XXX)
 *          if (qCpuHasFeature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 *
 * 2) place the optimized code in a function tagged with QT_FUNCTION_TARGET and
 * surrounded by #if QT_COMPILER_SUPPORTS_HERE(XXX). That code can freely use
 * other Qt code. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if QT_COMPILER_SUPPORTS_HERE(XXX)
 *          if (qCpuHasFeature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 */

#if defined(__MINGW64_VERSION_MAJOR) || (defined(Q_CC_MSVC) && !defined(Q_OS_WINCE))
#include <intrin.h>
#endif

#define QT_COMPILER_SUPPORTS(x)     (QT_COMPILER_SUPPORTS_ ## x - 0)

#if (defined(Q_CC_INTEL) || defined(Q_CC_MSVC) \
    || (defined(Q_CC_GNU) && !defined(Q_CC_CLANG) && (__GNUC__-0) * 100 + (__GNUC_MINOR__-0) >= 409)) \
    && !defined(QT_BOOTSTRAPPED)
#  define QT_COMPILER_SUPPORTS_SIMD_ALWAYS
#  define QT_COMPILER_SUPPORTS_HERE(x)    QT_COMPILER_SUPPORTS(x)
#  if defined(Q_CC_GNU) && !defined(Q_CC_INTEL)
     /* GCC requires attributes for a function */
#    define QT_FUNCTION_TARGET(x)  __attribute__((__target__(QT_FUNCTION_TARGET_STRING_ ## x)))
#  else
#    define QT_FUNCTION_TARGET(x)
#  endif
#else
#  define QT_COMPILER_SUPPORTS_HERE(x)    defined(__ ## x ## __)
#  define QT_FUNCTION_TARGET(x)
#endif

// SSE intrinsics
#define QT_FUNCTION_TARGET_STRING_SSE2      "sse2"
#if defined(__SSE2__) || (defined(QT_COMPILER_SUPPORTS_SSE2) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
#if defined(QT_LINUXBASE) || defined(Q_OS_ANDROID_NO_SDK)
/// this is an evil hack - the posix_memalign declaration in LSB
/// is wrong - see http://bugs.linuxbase.org/show_bug.cgi?id=2431
#  define posix_memalign _lsb_hack_posix_memalign
#  include <emmintrin.h>
#  undef posix_memalign
#else
#  include <emmintrin.h>
#endif
#if defined(Q_CC_MSVC) && (defined(_M_X64) || _M_IX86_FP >= 2)
#  define __SSE__ 1
#  define __SSE2__ 1
#endif
#endif

// SSE3 intrinsics
#define QT_FUNCTION_TARGET_STRING_SSE3      "sse3"
#if defined(__SSE3__) || (defined(QT_COMPILER_SUPPORTS_SSE3) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
#include <pmmintrin.h>
#endif

// SSSE3 intrinsics
#define QT_FUNCTION_TARGET_STRING_SSSE3     "ssse3"
#if defined(__SSSE3__) || (defined(QT_COMPILER_SUPPORTS_SSSE3) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
#include <tmmintrin.h>
#endif

// SSE4.1 intrinsics
#define QT_FUNCTION_TARGET_STRING_SSE4_1    "sse4.1"
#if defined(__SSE4_1__) || (defined(QT_COMPILER_SUPPORTS_SSE4_1) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
#include <smmintrin.h>
#endif

// SSE4.2 intrinsics
#define QT_FUNCTION_TARGET_STRING_SSE4_2    "sse4.2"
#if defined(__SSE4_2__) || (defined(QT_COMPILER_SUPPORTS_SSE4_2) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
#include <nmmintrin.h>
#endif

// AVX intrinsics
#define QT_FUNCTION_TARGET_STRING_AVX       "avx"
#define QT_FUNCTION_TARGET_STRING_AVX2      "avx2"
#if defined(__AVX__) || (defined(QT_COMPILER_SUPPORTS_AVX) && defined(QT_COMPILER_SUPPORTS_SIMD_ALWAYS))
// immintrin.h is the ultimate header, we don't need anything else after this
#include <immintrin.h>

#  if defined(Q_CC_MSVC) && (defined(_M_AVX) || defined(__AVX__))
// MS Visual Studio 2010 has no macro pre-defined to identify the use of /arch:AVX
// MS Visual Studio 2013 adds it: __AVX__
// See: http://connect.microsoft.com/VisualStudio/feedback/details/605858/arch-avx-should-define-a-predefined-macro-in-x64-and-set-a-unique-value-for-m-ix86-fp-in-win32
#    define __SSE3__ 1
#    define __SSSE3__ 1
// no Intel CPU supports SSE4a, so don't define it
#    define __SSE4_1__ 1
#    define __SSE4_2__ 1
#    ifndef __AVX__
#      define __AVX__ 1
#    endif
#  endif
#endif

// other x86 intrinsics
#if defined(Q_PROCESSOR_X86) && ((defined(Q_CC_GNU) && (Q_CC_GNU >= 404)) \
    || (defined(Q_CC_CLANG) && (Q_CC_CLANG >= 208)) \
    || defined(Q_CC_INTEL))
#  define QT_COMPILER_SUPPORTS_X86INTRIN
#  ifdef Q_CC_INTEL
// The Intel compiler has no <x86intrin.h> -- all intrinsics are in <immintrin.h>;
#    include <immintrin.h>
#  else
// GCC 4.4 and Clang 2.8 added a few more intrinsics there
#    include <x86intrin.h>
#  endif
#endif

// NEON intrinsics
// note: as of GCC 4.9, does not support function targets for ARM
#if defined __ARM_NEON__
#include <arm_neon.h>
#define QT_FUNCTION_TARGET_STRING_ARM_NEON      "neon"
#endif

#undef QT_COMPILER_SUPPORTS_SIMD_ALWAYS

QT_BEGIN_NAMESPACE


enum CPUFeatures {
    NEON        = 0x2,  ARM_NEON = NEON,
    SSE2        = 0x4,
    SSE3        = 0x8,
    SSSE3       = 0x10,
    SSE4_1      = 0x20,
    SSE4_2      = 0x40,
    AVX         = 0x80,
    AVX2        = 0x100,
    HLE         = 0x200,
    RTM         = 0x400,
    DSP         = 0x800,
    DSPR2       = 0x1000,

    // used only to indicate that the CPU detection was initialised
    QSimdInitialized = 0x80000000
};

static const uint qCompilerCpuFeatures = 0
#if defined __RTM__
        | RTM
#endif
#if defined __HLE__
        | HLE
#endif
#if defined __AVX2__
        | AVX2
#endif
#if defined __AVX__
        | AVX
#endif
#if defined __SSE4_2__
        | SSE4_2
#endif
#if defined __SSE4_1__
        | SSE4_1
#endif
#if defined __SSSE3__
        | SSSE3
#endif
#if defined __SSE3__
        | SSE3
#endif
#if defined __SSE2__
        | SSE2
#endif
#if defined __ARM_NEON__
        | NEON
#endif
#if defined __mips_dsp
        | DSP
#endif
#if defined __mips_dspr2
        | DSPR2
#endif
        ;

extern Q_CORE_EXPORT QBasicAtomicInt qt_cpu_features;
Q_CORE_EXPORT void qDetectCpuFeatures();

static inline uint qCpuFeatures()
{
    int features = qt_cpu_features.load();
    if (Q_UNLIKELY(features == 0)) {
        qDetectCpuFeatures();
        features = qt_cpu_features.load();
        Q_ASSUME(features != 0);
    }
    return uint(features);
}

#define qCpuHasFeature(feature)  ((qCompilerCpuFeatures & (feature)) || (qCpuFeatures() & (feature)))

#ifdef Q_PROCESSOR_X86
// Bit scan functions for x86
#  if defined(Q_CC_MSVC) && !defined(Q_OS_WINCE)
// MSVC calls it _BitScanReverse and returns the carry flag, which we don't need
static __forceinline unsigned long _bit_scan_reverse(uint val)
{
    unsigned long result;
    _BitScanReverse(&result, val);
    return result;
}
static __forceinline unsigned long _bit_scan_forward(uint val)
{
    unsigned long result;
    _BitScanForward(&result, val);
    return result;
}
#  elif (defined(Q_CC_CLANG) || (defined(Q_CC_GNU) && Q_CC_GNU < 405)) \
    && !defined(Q_CC_INTEL)
// Clang is missing the intrinsic for _bit_scan_reverse
// GCC only added it in version 4.5
static inline __attribute__((always_inline))
unsigned _bit_scan_reverse(unsigned val)
{
    unsigned result;
    asm("bsr %1, %0" : "=r" (result) : "r" (val));
    return result;
}
static inline __attribute__((always_inline))
unsigned _bit_scan_forward(unsigned val)
{
    unsigned result;
    asm("bsf %1, %0" : "=r" (result) : "r" (val));
    return result;
}
#  endif
#endif // Q_PROCESSOR_X86

#define ALIGNMENT_PROLOGUE_16BYTES(ptr, i, length) \
    for (; i < static_cast<int>(qMin(static_cast<quintptr>(length), ((4 - ((reinterpret_cast<quintptr>(ptr) >> 2) & 0x3)) & 0x3))); ++i)

QT_END_NAMESPACE

#endif // QSIMD_P_H
