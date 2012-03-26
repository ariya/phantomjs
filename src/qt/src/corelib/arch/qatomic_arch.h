/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QATOMIC_ARCH_H
#define QATOMIC_ARCH_H

QT_BEGIN_HEADER

#include "QtCore/qglobal.h"

#if defined(QT_ARCH_INTEGRITY)
#  include "QtCore/qatomic_integrity.h"
#elif defined(QT_ARCH_VXWORKS)
#  include "QtCore/qatomic_vxworks.h"
#elif defined(QT_ARCH_ALPHA)
#  include "QtCore/qatomic_alpha.h"
#elif defined(QT_ARCH_ARM)
#  include "QtCore/qatomic_arm.h"
#elif defined(QT_ARCH_ARMV6)
#  include "QtCore/qatomic_armv6.h"
#elif defined(QT_ARCH_AVR32)
#  include "QtCore/qatomic_avr32.h"
#elif defined(QT_ARCH_BFIN)
#  include "QtCore/qatomic_bfin.h"
#elif defined(QT_ARCH_GENERIC)
#  include "QtCore/qatomic_generic.h"
#elif defined(QT_ARCH_I386)
#  include "QtCore/qatomic_i386.h"
#elif defined(QT_ARCH_IA64)
#  include "QtCore/qatomic_ia64.h"
#elif defined(QT_ARCH_MACOSX)
#  include "QtCore/qatomic_macosx.h"
#elif defined(QT_ARCH_MIPS)
#  include "QtCore/qatomic_mips.h"
#elif defined(QT_ARCH_PARISC)
#  include "QtCore/qatomic_parisc.h"
#elif defined(QT_ARCH_POWERPC)
#  include "QtCore/qatomic_powerpc.h"
#elif defined(QT_ARCH_S390)
#  include "QtCore/qatomic_s390.h"
#elif defined(QT_ARCH_SPARC)
#  include "QtCore/qatomic_sparc.h"
#elif defined(QT_ARCH_WINDOWS)
#  include "QtCore/qatomic_windows.h"
#elif defined(QT_ARCH_WINDOWSCE)
#  include "QtCore/qatomic_windowsce.h"
#elif defined(QT_ARCH_X86_64)
#  include "QtCore/qatomic_x86_64.h"
#elif defined(QT_ARCH_SYMBIAN)
#  include "QtCore/qatomic_symbian.h"
#elif defined(QT_ARCH_SH)
#  include "QtCore/qatomic_sh.h"
#elif defined(QT_ARCH_SH4A)
#  include "QtCore/qatomic_sh4a.h"
#elif defined(QT_ARCH_NACL)
#  include "QtCore/qatomic_generic.h"
#else
#  error "Qt has not been ported to this architecture"
#endif

QT_END_HEADER

#endif // QATOMIC_ARCH_H
