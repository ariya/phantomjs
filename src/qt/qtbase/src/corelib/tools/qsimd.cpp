/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation.
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

#include "qsimd_p.h"
#include <QByteArray>
#include <stdio.h>

#if defined(Q_OS_WIN)
#  if defined(Q_OS_WINCE)
#    include <qt_windows.h>
#    include <cmnintrin.h>
#  endif
#  if !defined(Q_CC_GNU)
#    ifndef Q_OS_WINCE
#      include <intrin.h>
#    endif
#  endif
#elif defined(Q_OS_LINUX) && (defined(Q_PROCESSOR_ARM) || defined(QT_COMPILER_SUPPORTS_IWMMXT))
#include "private/qcore_unix_p.h"

// the kernel header definitions for HWCAP_*
// (the ones we need/may need anyway)

// copied from <asm/hwcap.h> (ARM)
#define HWCAP_IWMMXT    512
#define HWCAP_CRUNCH    1024
#define HWCAP_THUMBEE   2048
#define HWCAP_NEON      4096
#define HWCAP_VFPv3     8192
#define HWCAP_VFPv3D16  16384

// copied from <linux/auxvec.h>
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */

#endif

QT_BEGIN_NAMESPACE

#if defined (Q_OS_NACL)
static inline uint detectProcessorFeatures()
{
    return 0;
}
#elif defined (Q_OS_WINCE)
static inline uint detectProcessorFeatures()
{
    uint features = 0;

#if defined (ARM)
    if (IsProcessorFeaturePresent(PF_ARM_INTEL_WMMX)) {
        features = IWMMXT;
        return features;
    }
#elif defined(_X86_)
    features = 0;
    if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
        features |= SSE2;
    if (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE))
        features |= SSE3;
    return features;
#endif
    features = 0;
    return features;
}

#elif defined(Q_PROCESSOR_ARM) || defined(QT_COMPILER_SUPPORTS_IWMMXT)
static inline uint detectProcessorFeatures()
{
    uint features = 0;

#if defined(Q_OS_LINUX)
    int auxv = qt_safe_open("/proc/self/auxv", O_RDONLY);
    if (auxv != -1) {
        unsigned long vector[64];
        int nread;
        while (features == 0) {
            nread = qt_safe_read(auxv, (char *)vector, sizeof vector);
            if (nread <= 0) {
                // EOF or error
                break;
            }

            int max = nread / (sizeof vector[0]);
            for (int i = 0; i < max; i += 2)
                if (vector[i] == AT_HWCAP) {
                    if (vector[i+1] & HWCAP_IWMMXT)
                        features |= IWMMXT;
                    if (vector[i+1] & HWCAP_NEON)
                        features |= NEON;
                    break;
                }
        }

        qt_safe_close(auxv);
        return features;
    }
    // fall back if /proc/self/auxv wasn't found
#endif

#if defined(QT_COMPILER_SUPPORTS_IWMMXT)
    // runtime detection only available when running as a previlegied process
    features = IWMMXT;
#elif defined(__ARM_NEON__)
    features = NEON;
#endif

    return features;
}

#elif defined(Q_PROCESSOR_X86)

#ifdef Q_PROCESSOR_X86_32
# define PICreg "%%ebx"
#else
# define PICreg "%%rbx"
#endif

static int maxBasicCpuidSupported()
{
#if defined(Q_CC_GNU)
    qregisterint tmp1;

# if Q_PROCESSOR_X86 < 5
    // check if the CPUID instruction is supported
    long cpuid_supported;
    asm ("pushf\n"
         "pop %0\n"
         "mov %0, %1\n"
         "xor $0x00200000, %0\n"
         "push %0\n"
         "popf\n"
         "pushf\n"
         "pop %0\n"
         "xor %1, %0\n" // %eax is now 0 if CPUID is not supported
         : "=a" (cpuid_supported), "=r" (tmp1)
         );
    if (!cpuid_supported)
        return 0;
# endif

    int result;
    asm ("xchg " PICreg", %1\n"
         "cpuid\n"
         "xchg " PICreg", %1\n"
        : "=&a" (result), "=&r" (tmp1)
        : "0" (0)
        : "ecx", "edx");
    return result;
#elif defined(Q_OS_WIN)
    // Use the __cpuid function; if the CPUID instruction isn't supported, it will return 0
    int info[4];
    __cpuid(info, 0);
    return info[0];
#else
    return 0;
#endif
}

static void cpuidFeatures01(uint &ecx, uint &edx)
{
#if defined(Q_CC_GNU)
    qregisterint tmp1;
    asm ("xchg " PICreg", %2\n"
         "cpuid\n"
         "xchg " PICreg", %2\n"
        : "=&c" (ecx), "=&d" (edx), "=&r" (tmp1)
        : "a" (1));
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuid(info, 1);
    ecx = info[2];
    edx = info[3];
#endif
}

#ifdef Q_OS_WIN
inline void __cpuidex(int info[4], int, __int64) { memset(info, 0, 4*sizeof(int));}
#endif

static void cpuidFeatures07_00(uint &ebx)
{
#if defined(Q_CC_GNU)
    qregisteruint rbx; // in case it's 64-bit
    asm ("xchg " PICreg", %0\n"
         "cpuid\n"
         "xchg " PICreg", %0\n"
        : "=&r" (rbx)
        : "a" (7), "c" (0)
        : "%edx");
    ebx = rbx;
#elif defined(Q_OS_WIN)
    int info[4];
    __cpuidex(info, 7, 0);
    ebx = info[1];
#endif
}

#ifdef Q_OS_WIN
// fallback overload in case this intrinsic does not exist: unsigned __int64 _xgetbv(unsigned int);
inline quint64 _xgetbv(__int64) { return 0; }
#endif
static void xgetbv(uint in, uint &eax, uint &edx)
{
#if defined(Q_CC_GNU)
    asm (".byte 0x0F, 0x01, 0xD0" // xgetbv instruction
        : "=a" (eax), "=d" (edx)
        : "c" (in));
#elif defined(Q_OS_WIN)
    quint64 result = _xgetbv(in);
    eax = result;
    edx = result >> 32;
#endif
}

static inline uint detectProcessorFeatures()
{
    uint features = 0;
    int cpuidLevel = maxBasicCpuidSupported();
    if (cpuidLevel < 1)
        return 0;

    uint cpuid01ECX = 0, cpuid01EDX = 0;
    cpuidFeatures01(cpuid01ECX, cpuid01EDX);
#if defined(Q_PROCESSOR_X86_32)
    // x86 might not have SSE2 support
    if (cpuid01EDX & (1u << 26))
        features |= SSE2;
#else
    // x86-64 or x32
    features = SSE2;
#endif

    // common part between 32- and 64-bit
    if (cpuid01ECX & (1u))
        features |= SSE3;
    if (cpuid01ECX & (1u << 9))
        features |= SSSE3;
    if (cpuid01ECX & (1u << 19))
        features |= SSE4_1;
    if (cpuid01ECX & (1u << 20))
        features |= SSE4_2;
    if (cpuid01ECX & (1u << 25))
        features |= 0; // AES, enable if needed

    uint xgetbvA = 0, xgetbvD = 0;
    if (cpuid01ECX & (1u << 27)) {
        // XGETBV enabled
        xgetbv(0, xgetbvA, xgetbvD);
    }

    uint cpuid0700EBX = 0;
    if (cpuidLevel >= 7)
        cpuidFeatures07_00(cpuid0700EBX);

    if ((xgetbvA & 6) == 6) {
        // support for YMM and XMM registers is enabled
        if (cpuid01ECX & (1u << 28))
            features |= AVX;

        if (cpuid0700EBX & (1u << 5))
            features |= AVX2;
    }

    if (cpuid0700EBX & (1u << 4))
        features |= HLE; // Hardware Lock Ellision
    if (cpuid0700EBX & (1u << 11))
        features |= RTM; // Restricted Transactional Memory

    return features;
}


#else
static inline uint detectProcessorFeatures()
{
    return 0;
}
#endif

/*
 * Use kdesdk/scripts/generate_string_table.pl to update the table below.
 * Here's the data (don't forget the ONE leading space):
 iwmmxt
 neon
 sse2
 sse3
 ssse3
 sse4.1
 sse4.2
 avx
 avx2
 hle
 rtm
  */

// begin generated
static const char features_string[] =
    " iwmmxt\0"
    " neon\0"
    " sse2\0"
    " sse3\0"
    " ssse3\0"
    " sse4.1\0"
    " sse4.2\0"
    " avx\0"
    " avx2\0"
    " hle\0"
    " rtm\0"
    "\0";

static const int features_indices[] = {
    0,    8,   14,   20,   26,   33,   41,   49,
   54,   60,   65,   -1
};
// end generated

static const int features_count = (sizeof features_indices - 1) / (sizeof features_indices[0]);

// record what CPU features were enabled by default in this Qt build
// don't define for HLE, since the HLE prefix can be run on older CPUs
static const uint minFeature = qCompilerCpuFeatures & ~HLE;

#ifdef Q_OS_WIN
#if defined(Q_CC_GNU)
#  define ffs __builtin_ffs
#else
int ffs(int i)
{
#ifndef Q_OS_WINCE
    unsigned long result;
    return _BitScanForward(&result, i) ? result : 0;
#else
    return 0;
#endif
}
#endif
#endif // Q_OS_WIN

QBasicAtomicInt qt_cpu_features = Q_BASIC_ATOMIC_INITIALIZER(0);

void qDetectCpuFeatures()
{
#if defined(Q_CC_GNU) && !defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
# if (__GNUC__ * 100 + __GNUC_MINOR__) < 403
    // GCC 4.2 (at least the one that comes with Apple's XCode, on Mac) is
    // known to be broken beyond repair in dealing with the inline assembly
    // above. It will generate bad code that could corrupt important registers
    // like the PIC register. The behaviour of code after this function would
    // be totally unpredictable.
    //
    // For that reason, simply forego the CPUID check at all and return the set
    // of features that we found at compile time, through the #defines from the
    // compiler. This should at least allow code to execute, even if none of
    // the specialized code found in Qt GUI and elsewhere will ever be enabled
    // (it's the user's fault for using a broken compiler).
    //
    // This also disables the runtime checking that the processor actually
    // contains all the features that the code required. Qt 4 ran for years
    // like that, so it shouldn't be a problem.

    qt_cpu_features.store(minFeature | QSimdInitialized);
    return;
# endif
#endif
    uint f = detectProcessorFeatures();
    QByteArray disable = qgetenv("QT_NO_CPU_FEATURE");
    if (!disable.isEmpty()) {
        disable.prepend(' ');
        for (int i = 0; i < features_count; ++i) {
            if (disable.contains(features_string + features_indices[i]))
                f &= ~(1 << i);
        }
    }

    if (minFeature != 0 && (f & minFeature) != minFeature) {
        uint missing = minFeature & ~f;
        fprintf(stderr, "Incompatible processor. This Qt build requires the following features:\n   ");
        for (int i = 0; i < features_count; ++i) {
            if (missing & (1 << i))
                fprintf(stderr, "%s", features_string + features_indices[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        qFatal("Aborted. Incompatible processor: missing feature 0x%x -%s.", missing,
               features_string + features_indices[ffs(missing) - 1]);
    }

    qt_cpu_features.store(f | QSimdInitialized);
}

void qDumpCPUFeatures()
{
    uint features = qCpuFeatures();
    printf("Processor features: ");
    for (int i = 0; i < features_count; ++i) {
        if (features & (1 << i))
            printf("%s%s", features_string + features_indices[i],
                   minFeature & (1 << i) ? "[required]" : "");
    }
    puts("");
}

QT_END_NAMESPACE
