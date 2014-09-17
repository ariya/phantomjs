/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __E32_COMMON_H__
#define __E32_COMMON_H__

#ifdef __KERNEL_MODE__
#include <e32cmn.h>
#include <e32panic.h>
#include "u32std.h"
#else
#include <e32std.h>
#include <e32base.h>
#include <e32math.h>
#include <e32svr.h>
#include <e32ver.h>
#include <e32hal.h>
#include <e32panic.h>
// backport of Symbian^4 allocator to Symbian^3 SDK does not contain u32exec.h
//#include <u32exec.h>
#endif

GLREF_C void Panic(TCdtPanic aPanic);
GLDEF_C void PanicBadArrayIndex();
GLREF_C TInt __DoConvertNum(TUint, TRadix, TUint, TUint8*&);
GLREF_C TInt __DoConvertNum(Uint64, TRadix, TUint, TUint8*&);

#ifdef __KERNEL_MODE__
GLREF_C void KernHeapFault(TCdtPanic aPanic);
GLREF_C void KHeapCheckThreadState();
TInt StringLength(const TUint16* aPtr);
TInt StringLength(const TUint8* aPtr);

#define	STD_CLASS					Kern
#define	STRING_LENGTH(s)			StringLength(s)
#define	STRING_LENGTH_16(s)			StringLength(s)
#define	PANIC_CURRENT_THREAD(c,r)	Kern::PanicCurrentThread(c, r)
#define __KERNEL_CHECK_RADIX(r)		__ASSERT_ALWAYS(((r)==EDecimal)||((r)==EHex),Panic(EInvalidRadix))
#define	APPEND_BUF_SIZE				10
#define	APPEND_BUF_SIZE_64			20
#define	HEAP_PANIC(r)				Kern::Printf("HEAP CORRUPTED %s %d", __FILE__, __LINE__), RHeapK::Fault(r)
#define	GET_PAGE_SIZE(x)			x = M::PageSizeInBytes()
#define	DIVISION_BY_ZERO()			FAULT()

#ifdef _DEBUG
#define	__CHECK_THREAD_STATE		RHeapK::CheckThreadState()
#else
#define	__CHECK_THREAD_STATE
#endif

#else

#define	STD_CLASS					User
#define	STRING_LENGTH(s)			User::StringLength(s)
#define	STRING_LENGTH_16(s)			User::StringLength(s)
#define	PANIC_CURRENT_THREAD(c,r)	User::Panic(c, r)
#define	MEM_COMPARE_16				Mem::Compare
#define __KERNEL_CHECK_RADIX(r)
#define	APPEND_BUF_SIZE				32
#define	APPEND_BUF_SIZE_64			64
#define	HEAP_PANIC(r)				RDebug::Printf("HEAP CORRUPTED %s %d", __FILE__, __LINE__), Panic(r)
#define	GET_PAGE_SIZE(x)			UserHal::PageSizeInBytes(x)
#define	DIVISION_BY_ZERO()			User::RaiseException(EExcIntegerDivideByZero)
#define	__CHECK_THREAD_STATE

#endif	// __KERNEL_MODE__

#endif
