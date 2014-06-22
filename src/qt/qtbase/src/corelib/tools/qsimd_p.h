/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIMD_P_H
#define QSIMD_P_H

#include <qglobal.h>
#include <qatomic.h>

/*
 * qt_module_config.prf defines the QT_COMPILER_SUPPORTS_XXX macros.
 * They mean the compiler supports the necessary flags and the headers
 * for the x86 and ARM intrinsics:
 *  - GCC: the -mXXX or march=YYY flag is necessary before #include
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
 *   Flag  | Arch |  GCC  | Intel CC |  MSVC  |
 *  NEON   | ARM  | I & C | None     |   ?    |
 *  IWMMXT | ARM  | I & C | None     | I & C  |
 *  SSE2   | x86  | I & C | I & C    | I & C  |
 *  SSE3   | x86  | I & C | I & C    | I only |
 *  SSSE3  | x86  | I & C | I & C    | I only |
 *  SSE4_1 | x86  | I & C | I & C    | I only |
 *  SSE4_2 | x86  | I & C | I & C    | I only |
 *  AVX    | x86  | I & C | I & C    | I & C  |
 *  AVX2   | x86  | I & C | I & C    | I only |
 * I = intrinsics; C = code generation
 */

#if defined(__MINGW64_VERSION_MAJOR) || (defined(Q_CC_MSVC) && !defined(Q_OS_WINCE))
#include <intrin.h>
#endif

// SSE intrinsics
#if defined(__SSE2__) || (defined(QT_COMPILER_SUPPORTS_SSE2) && defined(Q_CC_MSVC))
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
#if defined(__SSE3__) || (defined(QT_COMPILER_SUPPORTS_SSE3) && defined(Q_CC_MSVC))
#include <pmmintrin.h>
#endif

// SSSE3 intrinsics
#if defined(__SSSE3__) || (defined(QT_COMPILER_SUPPORTS_SSSE3) && defined(Q_CC_MSVC))
#include <tmmintrin.h>
#endif

// SSE4.1 intrinsics
#if defined(__SSE4_1__) || (defined(QT_COMPILER_SUPPORTS_SSE4_1) && defined(Q_CC_MSVC))
#include <smmintrin.h>
#endif

// SSE4.2 intrinsics
#if defined(__SSE4_2__) || (defined(QT_COMPILER_SUPPORTS_SSE4_2) && defined(Q_CC_MSVC))
#include <nmmintrin.h>
#endif

// AVX intrinsics
#if defined(__AVX__) || (defined(QT_COMPILER_SUPPORTS_AVX) && defined(Q_CC_MSVC))
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
#if defined(Q_PROCESSOR_X86) && ((defined(Q_CC_GNU) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 404)) \
    || (defined(Q_CC_CLANG) && (__clang_major__ * 100 + __clang_minor__ >= 208)) \
    || defined(Q_CC_INTEL))
#  define QT_COMPILER_SUPPORTS_X86INTRIN
#  ifndef Q_CC_INTEL
// The Intel compiler has no <x86intrin.h> -- all intrinsics are in <immintrin.h>;
// GCC 4.4 and Clang 2.8 added a few more intrinsics there
#    include <x86intrin.h>
#  endif
#endif

// NEON intrinsics
#if defined __ARM_NEON__
#include <arm_neon.h>
#endif


// IWMMXT intrinsics
#if defined(QT_COMPILER_SUPPORTS_IWMMXT)
#include <mmintrin.h>
#if defined(Q_OS_WINCE)
#  include "qplatformdefs.h"
#endif
#endif

#if defined(QT_COMPILER_SUPPORTS_IWMMXT)
#if !defined(__IWMMXT__) && !defined(Q_OS_WINCE)
#  include <xmmintrin.h>
#elif defined(Q_OS_WINCE_STD) && defined(_X86_)
#  pragma warning(disable: 4391)
#  include <xmmintrin.h>
#endif
#endif

QT_BEGIN_NAMESPACE


enum CPUFeatures {
    IWMMXT      = 0x1,
    NEON        = 0x2,
    SSE2        = 0x4,
    SSE3        = 0x8,
    SSSE3       = 0x10,
    SSE4_1      = 0x20,
    SSE4_2      = 0x40,
    AVX         = 0x80,
    AVX2        = 0x100,
    HLE         = 0x200,
    RTM         = 0x400,

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
#if defined __IWMMXT__
        | IWMMXT
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
#  ifdef Q_CC_MSVC
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
#  elif (defined(Q_CC_CLANG) || (defined(Q_CC_GNU) && __GNUC__ * 100 + __GNUC_MINOR__ < 405)) \
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
