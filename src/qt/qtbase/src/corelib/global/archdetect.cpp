/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the FOO module of the Qt Toolkit.
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

#include "qprocessordetection.h"

// main part: processor type
#if defined(Q_PROCESSOR_ALPHA)
#  define ARCH_PROCESSOR "alpha"
#elif defined(Q_PROCESSOR_ARM_32)
#  define ARCH_PROCESSOR "arm"
#elif defined(Q_PROCESSOR_ARM_64)
#  define ARCH_PROCESSOR "arm64"
#elif defined(Q_PROCESSOR_AVR32)
#  define ARCH_PROCESSOR "avr32"
#elif defined(Q_PROCESSOR_BLACKFIN)
#  define ARCH_PROCESSOR "bfin"
#elif defined(Q_PROCESSOR_X86_32)
#  define ARCH_PROCESSOR "i386"
#elif defined(Q_PROCESSOR_X86_64)
#  define ARCH_PROCESSOR "x86_64"
#elif defined(Q_PROCESSOR_IA64)
#  define ARCH_PROCESSOR "ia64"
#elif defined(Q_PROCESSOR_MIPS_64)
#  define ARCH_PROCESSOR "mips64"
#elif defined(Q_PROCESSOR_MIPS)
#  define ARCH_PROCESSOR "mips"
#elif defined(Q_PROCESSOR_POWER_32)
#  define ARCH_PROCESSOR "power"
#elif defined(Q_PROCESSOR_POWER_64)
#  define ARCH_PROCESSOR "power64"
#elif defined(Q_PROCESSOR_S390_X)
#  define ARCH_PROCESSOR "s390x"
#elif defined(Q_PROCESSOR_S390)
#  define ARCH_PROCESSOR "s390"
#elif defined(Q_PROCESSOR_SH)
#  define ARCH_PROCESSOR "sh"
#elif defined(Q_PROCESSORS_SPARC_64)
#  define ARCH_PROCESSOR "sparc64"
#elif defined(Q_PROCESSOR_SPARC_V9)
#  define ARCH_PROCESSOR "sparcv9"
#elif defined(Q_PROCESSOR_SPARC)
#  define ARCH_PROCESSOR "sparc"
#else
#  define ARCH_PROCESSOR "unknown"
#endif

// endianness
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#  define ARCH_ENDIANNESS "little_endian"
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
#  define ARCH_ENDIANNESS "big_endian"
#endif

// pointer type
#if defined(Q_OS_WIN64) || (defined(Q_OS_WINRT) && defined(_M_X64))
#  define ARCH_POINTER "llp64"
#elif defined(__LP64__) || QT_POINTER_SIZE - 0 == 8
#  define ARCH_POINTER "lp64"
#else
#  define ARCH_POINTER "ilp32"
#endif

// qreal type, if not double (includes the dash)
#ifdef QT_COORD_TYPE_STRING
#  define ARCH_COORD_TYPE   "-qreal_" QT_COORD_TYPE_STRING
#else
#  define ARCH_COORD_TYPE  ""
#endif

// secondary: ABI string (includes the dash)
#if defined(__ARM_EABI__) || defined(__mips_eabi)
#  define ARCH_ABI1 "-eabi"
#elif defined(_MIPS_SIM)
#  if _MIPS_SIM == _ABIO32
#    define ARCH_ABI1 "-o32"
#  elif _MIPS_SIM == _ABIN32
#    define ARCH_ABI1 "-n32"
#  elif _MIPS_SIM == _ABI64
#    define ARCH_ABI1 "-n64"
#  elif _MIPS_SIM == _ABIO64
#    define ARCH_ABI1 "-o64"
#  endif
#else
#  define ARCH_ABI1 ""
#endif
#if defined(__ARM_PCS_VFP) || defined(__mips_hard_float)
#  define ARCH_ABI2 "-hardfloat"
#else
#  define ARCH_ABI2 ""
#endif

#define ARCH_ABI ARCH_ABI1 ARCH_ABI2

#define ARCH_FULL ARCH_PROCESSOR "-" ARCH_ENDIANNESS "-" ARCH_POINTER ARCH_COORD_TYPE ARCH_ABI
