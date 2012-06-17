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

#ifndef QT_HYBRIDHEAP_SYMBIAN_H
#define QT_HYBRIDHEAP_SYMBIAN_H

#include <qglobal.h>
#include <e32cmn.h>

#if !defined(__SYMBIAN_KERNEL_HYBRID_HEAP__) && !defined(__WINS__)
//Enable the (backported) new allocator. When it is available in OS,
//this flag should be disabled for that OS version onward
#define QT_USE_NEW_SYMBIAN_ALLOCATOR
#endif

#ifdef QT_USE_NEW_SYMBIAN_ALLOCATOR

#ifdef Q_CC_RVCT
#pragma push
#pragma arm
#pragma Otime
#pragma O2
#endif

#include "common_p.h"
#ifdef QT_SYMBIAN_HAVE_U32STD_H
#include <u32std.h>
#endif
#ifdef QT_SYMBIAN_HAVE_E32BTRACE_H
#include <e32btrace.h>
// enables btrace code compiling into
#define ENABLE_BTRACE
#endif
#ifdef __KERNEL_MODE__
#include <kernel/kern_priv.h>
#endif
#include "dla_p.h"
#ifndef __KERNEL_MODE__
#include "slab_p.h"
#include "page_alloc_p.h"
#endif
#include "heap_hybrid_p.h"

// disabling Symbian import/export macros to prevent heap_hybrid.cpp, copied from Symbian^4, from exporting symbols in arm builds
// this minimises the code changes to heap_hybrid.cpp to ease future integration
#undef UEXPORT_C
#define UEXPORT_C
#undef EXPORT_C
#define EXPORT_C
#undef IMPORT_D
#define IMPORT_D

// disabling code ported from Symbian^4 that we don't want/can't have in earlier platforms
#define QT_SYMBIAN4_ALLOCATOR_UNWANTED_CODE

#if defined(SYMBIAN_VERSION_9_1) || defined(SYMBIAN_VERSION_9_2) || defined(SYMBIAN_VERSION_9_3) || defined(SYMBIAN_VERSION_9_4) || defined(SYMBIAN_VERSION_SYMBIAN2)
#define NO_NAMED_LOCAL_CHUNKS
#endif

// disabling the BTrace components of heap checking macros
#ifndef ENABLE_BTRACE
inline int noBTrace() {return 0;}
#define BTraceContext12(a,b,c,d,e) noBTrace()
#endif

// declare ETHeapBadDebugFailParameter, where missing
#define ETHeapBadDebugFailParameter ((TCdtPanic)213)

#ifndef QT_SYMBIAN_HAVE_U32STD_H
struct SThreadCreateInfo
    {
    TAny* iHandle;
    TInt iType;
    TThreadFunction iFunction;
    TAny* iPtr;
    TAny* iSupervisorStack;
    TInt iSupervisorStackSize;
    TAny* iUserStack;
    TInt iUserStackSize;
    TInt iInitialThreadPriority;
    TPtrC iName;
    TInt iTotalSize;    // Size including any extras (must be a multiple of 8 bytes)
    };

struct SStdEpocThreadCreateInfo : public SThreadCreateInfo
    {
    RAllocator* iAllocator;
    TInt iHeapInitialSize;
    TInt iHeapMaxSize;
    TInt iPadding;      // Make structure size a multiple of 8 bytes
    };

class TChunkCreate
	{
public:
	// Attributes for chunk creation that are used by both euser and the kernel
	// by classes TChunkCreateInfo and SChunkCreateInfo, respectively.
	enum TChunkCreateAtt
		{
		ENormal				= 0x00000000,
		EDoubleEnded		= 0x00000001,
		EDisconnected		= 0x00000002,
		ECache				= 0x00000003,
		EMappingMask		= 0x0000000f,
		ELocal				= 0x00000000,
		EGlobal				= 0x00000010,
		EData				= 0x00000000,
		ECode				= 0x00000020,
		EMemoryNotOwned		= 0x00000040,

		// Force local chunk to be named.  Only required for thread heap
		// chunks, all other local chunks should be nameless.
		ELocalNamed 		= 0x000000080,

		// Make global chunk read only to all processes but the controlling owner
		EReadOnly			= 0x000000100,

		// Paging attributes for chunks.
		EPagingUnspec		= 0x00000000,
		EPaged				= 0x80000000,
		EUnpaged			= 0x40000000,
		EPagingMask 		= EPaged | EUnpaged,

		EChunkCreateAttMask =	EMappingMask | EGlobal | ECode |
								ELocalNamed | EReadOnly | EPagingMask,
		};
public:
	TUint iAtt;
	TBool iForceFixed;
	TInt iInitialBottom;
	TInt iInitialTop;
	TInt iMaxSize;
	TUint8 iClearByte;
	};

#endif // QT_SYMBIAN_HAVE_U32STD_H

#endif /* QT_USE_NEW_SYMBIAN_ALLOCATOR */

#endif /* QT_HYBRIDHEAP_SYMBIAN_H */
