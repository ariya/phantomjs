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

#ifndef QGLOBAL_H
# include <QtCore/qglobal.h>
#endif

#ifndef QPROCESSORDETECTION_H
#define QPROCESSORDETECTION_H

/*
    This file uses preprocessor #defines to set various Q_PROCESSOR_* #defines
    based on the following patterns:

    Q_PROCESSOR_{FAMILY}
    Q_PROCESSOR_{FAMILY}_{VARIANT}
    Q_PROCESSOR_{FAMILY}_{REVISION}

    The first is always defined. Defines for the various revisions/variants are
    optional and usually dependent on how the compiler was invoked. Variants
    that are a superset of another should have a define for the superset.

    In addition to the procesor family, variants, and revisions, we also set
    Q_BYTE_ORDER appropriately for the target processor. For bi-endian
    processors, we try to auto-detect the byte order using the __BIG_ENDIAN__,
    __LITTLE_ENDIAN__, or __BYTE_ORDER__ preprocessor macros.

    Note: when adding support for new processors, be sure to update
    config.tests/arch/arch.cpp to ensure that configure can detect the target
    and host architectures.
*/

/* Machine byte-order, reuse preprocessor provided macros when available */
#if defined(__ORDER_BIG_ENDIAN__)
#  define Q_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#else
#  define Q_BIG_ENDIAN 4321
#endif
#if defined(__ORDER_LITTLE_ENDIAN__)
#  define Q_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#else
#  define Q_LITTLE_ENDIAN 1234
#endif

/*
    Alpha family, no revisions or variants

    Alpha is bi-endian, use endianness auto-detection implemented below.
*/
// #elif defined(__alpha__) || defined(_M_ALPHA)
// #  define Q_PROCESSOR_ALPHA
// Q_BYTE_ORDER not defined, use endianness auto-detection

/*
    ARM family, known revisions: V5, V6, V7, V8

    ARM is bi-endian, detect using __ARMEL__ or __ARMEB__, falling back to
    auto-detection implemented below.
*/
#if defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(__arm64__)
#  define Q_PROCESSOR_ARM
#  if defined(__arm64__)
#    define Q_PROCESSOR_ARM_64
#  else
#    define Q_PROCESSOR_ARM_32
#  endif
#  if defined(__ARM64_ARCH_8__)
#    define Q_PROCESSOR_ARM_V8
#    define Q_PROCESSOR_ARM_V7
#    define Q_PROCESSOR_ARM_V6
#    define Q_PROCESSOR_ARM_V5
#  elif defined(__ARM_ARCH_7__) \
      || defined(__ARM_ARCH_7A__) \
      || defined(__ARM_ARCH_7R__) \
      || defined(__ARM_ARCH_7M__) \
      || defined(__ARM_ARCH_7S__) \
      || defined(_ARM_ARCH_7) \
      || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 7) \
      || (defined(_M_ARM) && _M_ARM-0 >= 7)
#    define Q_PROCESSOR_ARM_V7
#    define Q_PROCESSOR_ARM_V6
#    define Q_PROCESSOR_ARM_V5
#  elif defined(__ARM_ARCH_6__) \
      || defined(__ARM_ARCH_6J__) \
      || defined(__ARM_ARCH_6T2__) \
      || defined(__ARM_ARCH_6Z__) \
      || defined(__ARM_ARCH_6K__) \
      || defined(__ARM_ARCH_6ZK__) \
      || defined(__ARM_ARCH_6M__) \
      || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 6) \
      || (defined(_M_ARM) && _M_ARM-0 >= 6)
#    define Q_PROCESSOR_ARM_V6
#    define Q_PROCESSOR_ARM_V5
#  elif defined(__ARM_ARCH_5TEJ__) \
        || defined(__ARM_ARCH_5TE__) \
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 5) \
        || (defined(_M_ARM) && _M_ARM-0 >= 5)
#    define Q_PROCESSOR_ARM_V5
#  endif
#  if defined(__ARMEL__)
#    define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#  elif defined(__ARMEB__)
#    define Q_BYTE_ORDER Q_BIG_ENDIAN
#  else
// Q_BYTE_ORDER not defined, use endianness auto-detection
#endif

/*
    AVR32 family, no revisions or variants

    AVR32 is big-endian.
*/
// #elif defined(__avr32__)
// #  define Q_PROCESSOR_AVR32
// #  define Q_BYTE_ORDER Q_BIG_ENDIAN

/*
    Blackfin family, no revisions or variants

    Blackfin is little-endian.
*/
// #elif defined(__bfin__)
// #  define Q_PROCESSOR_BLACKFIN
// #  define Q_BYTE_ORDER Q_LITTLE_ENDIAN

/*
    X86 family, known variants: 32- and 64-bit

    X86 is little-endian.
*/
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#  define Q_PROCESSOR_X86_32
#  define Q_BYTE_ORDER Q_LITTLE_ENDIAN

/*
 * We define Q_PROCESSOR_X86 == 6 for anything above a equivalent or better
 * than a Pentium Pro (the processor whose architecture was called P6) or an
 * Athlon.
 *
 * All processors since the Pentium III and the Athlon 4 have SSE support, so
 * we use that to detect. That leaves the original Athlon, Pentium Pro and
 * Pentium II.
 */

#  if defined(_M_IX86)
#    define Q_PROCESSOR_X86     (_M_IX86/100)
#  elif defined(__i686__) || defined(__athlon__) || defined(__SSE__)
#    define Q_PROCESSOR_X86     6
#  elif defined(__i586__) || defined(__k6__)
#    define Q_PROCESSOR_X86     5
#  elif defined(__i486__)
#    define Q_PROCESSOR_X86     4
#  else
#    define Q_PROCESSOR_X86     3
#  endif

#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#  define Q_PROCESSOR_X86       6
#  define Q_PROCESSOR_X86_64
#  define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#  define Q_PROCESSOR_WORDSIZE   8

/*
    Itanium (IA-64) family, no revisions or variants

    Itanium is bi-endian, use endianness auto-detection implemented below.
*/
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#  define Q_PROCESSOR_IA64
#  define Q_PROCESSOR_WORDSIZE   8
// Q_BYTE_ORDER not defined, use endianness auto-detection

/*
    MIPS family, known revisions: I, II, III, IV, 32, 64

    MIPS is bi-endian, use endianness auto-detection implemented below.
*/
#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#  define Q_PROCESSOR_MIPS
#  if defined(_MIPS_ARCH_MIPS1) || (defined(__mips) && __mips - 0 >= 1)
#    define Q_PROCESSOR_MIPS_I
#  endif
#  if defined(_MIPS_ARCH_MIPS2) || (defined(__mips) && __mips - 0 >= 2)
#    define Q_PROCESSOR_MIPS_II
#  endif
#  if defined(_MIPS_ARCH_MIPS32) || defined(__mips32)
#    define Q_PROCESSOR_MIPS_32
#  endif
#  if defined(_MIPS_ARCH_MIPS3) || (defined(__mips) && __mips - 0 >= 3)
#    define Q_PROCESSOR_MIPS_III
#  endif
#  if defined(_MIPS_ARCH_MIPS4) || (defined(__mips) && __mips - 0 >= 4)
#    define Q_PROCESSOR_MIPS_IV
#  endif
#  if defined(_MIPS_ARCH_MIPS5) || (defined(__mips) && __mips - 0 >= 5)
#    define Q_PROCESSOR_MIPS_V
#  endif
#  if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
#    define Q_PROCESSOR_MIPS_64
#  endif
#  if defined(__MIPSEL__)
#    define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#  elif defined(__MIPSEB__)
#    define Q_BYTE_ORDER Q_BIG_ENDIAN
#  else
// Q_BYTE_ORDER not defined, use endianness auto-detection
#  endif

/*
    Power family, known variants: 32- and 64-bit

    There are many more known variants/revisions that we do not handle/detect.
    See http://en.wikipedia.org/wiki/Power_Architecture
    and http://en.wikipedia.org/wiki/File:PowerISA-evolution.svg

    Power is bi-endian, use endianness auto-detection implemented below.
*/
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \
      || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \
      || defined(_M_MPPC) || defined(_M_PPC)
#  define Q_PROCESSOR_POWER
#  if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
#    define Q_PROCESSOR_POWER_64
#  else
#    define Q_PROCESSOR_POWER_32
#  endif
// Q_BYTE_ORDER not defined, use endianness auto-detection

/*
    S390 family, known variant: S390X (64-bit)

    S390 is big-endian.
*/
#elif defined(__s390__)
#  define Q_PROCESSOR_S390
#  if defined(__s390x__)
#    define Q_PROCESSOR_S390_X
#  endif
#  define Q_BYTE_ORDER Q_BIG_ENDIAN

/*
    SuperH family, optional revision: SH-4A

    SuperH is bi-endian, use endianness auto-detection implemented below.
*/
// #elif defined(__sh__)
// #  define Q_PROCESSOR_SH
// #  if defined(__sh4a__)
// #    define Q_PROCESSOR_SH_4A
// #  endif
// Q_BYTE_ORDER not defined, use endianness auto-detection

/*
    SPARC family, optional revision: V9

    SPARC is big-endian only prior to V9, while V9 is bi-endian with big-endian
    as the default byte order. Assume all SPARC systems are big-endian.
*/
// #elif defined(__sparc__)
// #  define Q_PROCESSOR_SPARC
// #  if defined(__sparc_v9__)
// #    define Q_PROCESSOR_SPARC_V9
// #  endif
// #  define Q_BYTE_ORDER Q_BIG_ENDIAN

#endif

/*
  NOTE:
  GCC 4.6 added __BYTE_ORDER__, __ORDER_BIG_ENDIAN__, __ORDER_LITTLE_ENDIAN__
  and __ORDER_PDP_ENDIAN__ in SVN r165881. If you are using GCC 4.6 or newer,
  this code will properly detect your target byte order; if you are not, and
  the __LITTLE_ENDIAN__ or __BIG_ENDIAN__ macros are not defined, then this
  code will fail to detect the target byte order.
*/
// Some processors support either endian format, try to detect which we are using.
#if !defined(Q_BYTE_ORDER)
#  if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == Q_BIG_ENDIAN || __BYTE_ORDER__ == Q_LITTLE_ENDIAN)
// Reuse __BYTE_ORDER__ as-is, since our Q_*_ENDIAN #defines match the preprocessor defaults
#    define Q_BYTE_ORDER __BYTE_ORDER__
#  elif defined(__BIG_ENDIAN__) || defined(_big_endian__) || defined(_BIG_ENDIAN)
#    define Q_BYTE_ORDER Q_BIG_ENDIAN
#  elif defined(__LITTLE_ENDIAN__) || defined(_little_endian__) || defined(_LITTLE_ENDIAN) \
        || defined(_WIN32_WCE) || defined(WINAPI_FAMILY) // Windows CE is always little-endian according to MSDN.
#    define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#  else
#    error "Unable to determine byte order!"
#  endif
#endif

/*
   Define Q_PROCESSOR_WORDSIZE to be the size of the machine's word (usually,
   the size of the register). On some architectures where a pointer could be
   smaller than the register, the macro is defined above.

   Falls back to QT_POINTER_SIZE if not set explicitly for the platform.
*/
#ifndef Q_PROCESSOR_WORDSIZE
#  ifdef __SIZEOF_POINTER__
     /* GCC & friends define this */
#    define Q_PROCESSOR_WORDSIZE        __SIZEOF_POINTER__
#  elif defined(_LP64) || defined(__LP64__) || defined(WIN64) || defined(_WIN64)
#    define Q_PROCESSOR_WORDSIZE        8
#  else
#    define Q_PROCESSOR_WORDSIZE        QT_POINTER_SIZE
#  endif
#endif

#endif // QPROCESSORDETECTION_H
