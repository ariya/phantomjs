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

#include "qt_hybridheap_symbian_p.h"

#ifdef QT_USE_NEW_SYMBIAN_ALLOCATOR

// if non zero this causes the iSlabs to be configured only when the chunk size exceeds this level
#define DELAYED_SLAB_THRESHOLD (64*1024)		// 64KB seems about right based on trace data
#define SLAB_CONFIG 0x3fff						// Use all slab sizes 4,8..56 bytes. This is more efficient for large heaps as Qt tends to have

#ifdef _DEBUG
#define __SIMULATE_ALLOC_FAIL(s)	if (CheckForSimulatedAllocFail()) {s}
#define __ALLOC_DEBUG_HEADER(s)  	(s += EDebugHdrSize)
#define __SET_DEBUG_DATA(p,n,c)   	(((SDebugCell*)(p))->nestingLevel = (n), ((SDebugCell*)(p))->allocCount = (c))
#define __GET_USER_DATA_BFR(p)   	((p!=0) ? (TUint8*)(p) + EDebugHdrSize : NULL)
#define __GET_DEBUG_DATA_BFR(p)   	((p!=0) ? (TUint8*)(p) - EDebugHdrSize : NULL)
#define	__ZAP_CELL(p)				memset( (TUint8*)p, 0xde, (AllocLen(__GET_USER_DATA_BFR(p))+EDebugHdrSize))
#define __DEBUG_SAVE(p)				TInt dbgNestLevel = ((SDebugCell*)p)->nestingLevel
#define __DEBUG_RESTORE(p)			if (p) {((SDebugCell*)p)->nestingLevel = dbgNestLevel;}
#define __DEBUG_HDR_SIZE			EDebugHdrSize
#define __REMOVE_DBG_HDR(n) 		(n*EDebugHdrSize)
#define __GET_AVAIL_BLOCK_SIZE(s)   ( (s<EDebugHdrSize) ? 0 : s-EDebugHdrSize )
#define __UPDATE_ALLOC_COUNT(o,n,c)	if (o!=n && n) {((SDebugCell*)n)->allocCount = (c);}
#define __INIT_COUNTERS(i)	        iCellCount=i,iTotalAllocSize=i
#define __INCREMENT_COUNTERS(p)	    iCellCount++, iTotalAllocSize += AllocLen(p)
#define __DECREMENT_COUNTERS(p)	    iCellCount--, iTotalAllocSize -= AllocLen(p)
#define __UPDATE_TOTAL_ALLOC(p,s)	iTotalAllocSize += (AllocLen(__GET_USER_DATA_BFR(p)) - s)

#else
#define __SIMULATE_ALLOC_FAIL(s)
#define __ALLOC_DEBUG_HEADER(s)
#define __SET_DEBUG_DATA(p,n,c)
#define __GET_USER_DATA_BFR(p)  (p)
#define __GET_DEBUG_DATA_BFR(p) (p)
#define	__ZAP_CELL(p)
#define __DEBUG_SAVE(p)			
#define __DEBUG_RESTORE(p)
#define __DEBUG_HDR_SIZE		0
#define __REMOVE_DBG_HDR(n) 	0
#define __GET_AVAIL_BLOCK_SIZE(s)   (s)
#define __UPDATE_ALLOC_COUNT(o,n,c)	
#define __INIT_COUNTERS(i)	        iCellCount=i,iTotalAllocSize=i
#define __INCREMENT_COUNTERS(p)
#define __DECREMENT_COUNTERS(p)
#define __UPDATE_TOTAL_ALLOC(p,s)

#endif


#define MEMORY_MONITORED  (iFlags & EMonitorMemory)
#define GM  (&iGlobalMallocState)
#define IS_FIXED_HEAP  (iFlags & EFixedSize)
#define __INIT_COUNTERS(i)	iCellCount=i,iTotalAllocSize=i
#define __POWER_OF_2(x)		(!((x)&((x)-1)))

#define __DL_BFR_CHECK(M,P) \
		  if ( MEMORY_MONITORED ) \
             if ( !IS_ALIGNED(P) || ((TUint8*)(P)<M->iSeg.iBase) || ((TUint8*)(P)>(M->iSeg.iBase+M->iSeg.iSize))) \
                  BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)P, (TUint32)0), HEAP_PANIC(ETHeapBadCellAddress); \
			 else DoCheckInuseChunk(M, MEM2CHUNK(P)) 

#ifndef __KERNEL_MODE__

#define __SLAB_BFR_CHECK(S,P,B) \
	  if ( MEMORY_MONITORED ) \
		  if ( ((TUint32)P & 0x3) || ((TUint8*)P<iMemBase) || ((TUint8*)(P)>(TUint8*)this))  \
			   BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)P, (TUint32)S), HEAP_PANIC(ETHeapBadCellAddress); \
		  else DoCheckSlab(S, EPartialFullSlab, P), BuildPartialSlabBitmap(B,S,P) 					  
#define __PAGE_BFR_CHECK(P) \
		if ( MEMORY_MONITORED ) \
			if ( ((TUint32)P &  ((1 << iPageSize)-1)) || ((TUint8*)P<iMemBase) || ((TUint8*)(P)>(TUint8*)this))  \
				BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)this, (TUint32)P, (TUint32)0), HEAP_PANIC(ETHeapBadCellAddress)

#endif

#ifdef _MSC_VER
// This is required while we are still using VC6 to compile, so as to avoid warnings that cannot be fixed
// without having to edit the original Doug Lea source.  The 4146 warnings are due to the original code having
// a liking for negating unsigned numbers and the 4127 warnings are due to the original code using the RTCHECK
// macro with values that are always defined as 1.  It is better to turn these warnings off than to introduce
// diffs between the original Doug Lea implementation and our adaptation of it
#pragma warning( disable : 4146 ) /* unary minus operator applied to unsigned type, result still unsigned */
#pragma warning( disable : 4127 ) /* conditional expression is constant */
#endif // _MSC_VER


/**
@SYMPatchable
@publishedPartner
@released

Defines the minimum cell size of  a heap.

The constant can be changed at ROM build time using patchdata OBY keyword.

@deprecated Patching this constant no longer has any effect.
*/
#ifdef __X86GCC__	// For X86GCC we don't use the proper data import attribute
#undef IMPORT_D		// since the constants are not really imported. GCC doesn't 
#define IMPORT_D	// allow imports from self.
#endif
IMPORT_D extern const TInt KHeapMinCellSize;

/**
@SYMPatchable
@publishedPartner
@released

This constant defines the ratio that determines the amount of hysteresis between heap growing and heap
shrinking.
It is a 32-bit fixed point number where the radix point is defined to be
between bits 7 and 8 (where the LSB is bit 0) i.e. using standard notation, a Q8 or a fx24.8
fixed point number.  For example, for a ratio of 2.0, set KHeapShrinkHysRatio=0x200.

The heap shrinking hysteresis value is calculated to be:
@code
KHeapShrinkHysRatio*(iGrowBy>>8)
@endcode
where iGrowBy is a page aligned value set by the argument, aGrowBy, to the RHeap constructor.
The default hysteresis value is iGrowBy bytes i.e. KHeapShrinkHysRatio=2.0.

Memory usage may be improved by reducing the heap shrinking hysteresis
by setting 1.0 < KHeapShrinkHysRatio < 2.0.  Heap shrinking hysteresis is disabled/removed
when KHeapShrinkHysRatio <= 1.0.

The constant can be changed at ROM build time using patchdata OBY keyword.
*/
IMPORT_D extern const TInt KHeapShrinkHysRatio;

UEXPORT_C TInt RHeap::AllocLen(const TAny* aCell) const
{
	const MAllocator* m = this;
	return m->AllocLen(aCell);
}

UEXPORT_C TAny* RHeap::Alloc(TInt aSize)
{
	const MAllocator* m = this;
	return ((MAllocator*)m)->Alloc(aSize);
}

UEXPORT_C void RHeap::Free(TAny* aCell)
{
	const MAllocator* m = this;
	((MAllocator*)m)->Free(aCell);
}

UEXPORT_C TAny* RHeap::ReAlloc(TAny* aCell, TInt aSize, TInt aMode)
{
	const MAllocator* m = this;
	return ((MAllocator*)m)->ReAlloc(aCell, aSize, aMode);
}

UEXPORT_C TInt RHeap::DebugFunction(TInt aFunc, TAny* a1, TAny* a2)
{
	const MAllocator* m = this;
	return ((MAllocator*)m)->DebugFunction(aFunc, a1, a2);
}

UEXPORT_C TInt RHeap::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
{
	const MAllocator* m = this;
	return ((MAllocator*)m)->Extension_(aExtensionId, a0, a1);
}

#ifndef __KERNEL_MODE__

EXPORT_C TInt RHeap::AllocSize(TInt& aTotalAllocSize) const
{
	const MAllocator* m = this;
	return m->AllocSize(aTotalAllocSize);
}

EXPORT_C TInt RHeap::Available(TInt& aBiggestBlock) const
{
	const MAllocator* m = this;
	return m->Available(aBiggestBlock);
}

EXPORT_C void RHeap::Reset()
{
	const MAllocator* m = this;
	((MAllocator*)m)->Reset();
}

EXPORT_C TInt RHeap::Compress()
{
	const MAllocator* m = this;
	return ((MAllocator*)m)->Compress();
}
#endif

RHybridHeap::RHybridHeap()
	{
	// This initialisation cannot be done in RHeap() for compatibility reasons
	iMaxLength = iChunkHandle = iNestingLevel = 0;
	iTop = NULL;
	iFailType = ENone;
	iTestData = NULL;
	}

void RHybridHeap::operator delete(TAny*, TAny*) 
/**
Called if constructor issued by operator new(TUint aSize, TAny* aBase) throws exception.
This is dummy as corresponding new operator does not allocate memory.
*/
{}


#ifndef __KERNEL_MODE__
void RHybridHeap::Lock() const
   /**
   @internalComponent
*/
   {((RFastLock&)iLock).Wait();}


void RHybridHeap::Unlock() const
   /**
   @internalComponent
*/
   {((RFastLock&)iLock).Signal();}


TInt RHybridHeap::ChunkHandle() const
   /**
   @internalComponent
*/
{
	return iChunkHandle;
}

#else
//
//  This method is implemented in kheap.cpp
//
//void RHybridHeap::Lock() const
   /**
   @internalComponent
*/
//   {;}



//
//  This method is implemented in kheap.cpp
//
//void RHybridHeap::Unlock() const
   /**
   @internalComponent
*/
//   {;}


TInt RHybridHeap::ChunkHandle() const
   /**
   @internalComponent
*/
{
	return 0;
}
#endif

RHybridHeap::RHybridHeap(TInt aChunkHandle, TInt aOffset, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign, TBool aSingleThread, TBool aDLOnly, TBool aUseAdjust)
/**
Constructor for a non fixed heap.  Unlike the fixed heap, this heap is quite flexible in terms of its minimum and
maximum lengths and in that it can use the hybrid allocator if all of its requirements are met.
*/
	: iOffset(aOffset), iChunkSize(aMinLength)
	{
	__ASSERT_ALWAYS(iOffset>=0, HEAP_PANIC(ETHeapNewBadOffset));	

	iChunkHandle = aChunkHandle;
	iMinLength = aMinLength;
	iMaxLength = aMaxLength;

	// If the user has explicitly specified 0 as the aGrowBy value, set it to 1 so that it will be rounded up to the nearst page size
	if (aGrowBy == 0)
		aGrowBy = 1;
	GET_PAGE_SIZE(iPageSize);
	iGrowBy = _ALIGN_UP(aGrowBy, iPageSize);

	Construct(aSingleThread, aDLOnly, aUseAdjust, aAlign);
	}

RHybridHeap::RHybridHeap(TInt aMaxLength, TInt aAlign, TBool aSingleThread)
/**
Constructor for a fixed heap.  We have restrictions in that we have fixed minimum and maximum lengths and cannot grow
and we only use DL allocator.
*/
	: iOffset(0), iChunkSize(aMaxLength)
	{
	iChunkHandle = NULL;
	iMinLength = aMaxLength;
	iMaxLength = aMaxLength;
	iGrowBy = 0;

	Construct(aSingleThread, ETrue, ETrue, aAlign);
	}

TAny* RHybridHeap::operator new(TUint aSize, TAny* aBase) __NO_THROW
{
	__ASSERT_ALWAYS(aSize>=sizeof(RHybridHeap), HEAP_PANIC(ETHeapNewBadSize));
	RHybridHeap* h = (RHybridHeap*)aBase;
	h->iBase = ((TUint8*)aBase) + aSize;
	return aBase;
}

void RHybridHeap::Construct(TBool aSingleThread, TBool aDLOnly, TBool aUseAdjust, TInt aAlign)
{
	iAlign = aAlign ? aAlign : RHybridHeap::ECellAlignment;
	__ASSERT_ALWAYS((TUint32)iAlign>=sizeof(TAny*) && __POWER_OF_2(iAlign), HEAP_PANIC(ETHeapNewBadAlignment));

	// This initialisation cannot be done in RHeap() for compatibility reasons
	iTop = NULL;
	iFailType = ENone;
	iNestingLevel = 0;
	iTestData = NULL;

	iHighWaterMark = iMinLength;
	iAllocCount = 0;
	iFlags = aSingleThread ? ESingleThreaded : 0;
	iGrowBy = _ALIGN_UP(iGrowBy, iPageSize);
	
	if ( iMinLength == iMaxLength )
		{
		iFlags |= EFixedSize;
		aDLOnly = ETrue;
		}
#ifndef __KERNEL_MODE__
#ifdef DELAYED_SLAB_THRESHOLD
	iSlabInitThreshold = DELAYED_SLAB_THRESHOLD;
#else
	iSlabInitThreshold = 0;
#endif // DELAYED_SLAB_THRESHOLD
	iUseAdjust = aUseAdjust;
	iDLOnly    = aDLOnly;
#else
	(void)aUseAdjust;	
#endif 
	// Initialise suballocators
	// if DL only is required then it cannot allocate slab or page memory
	// so these sub-allocators should be disabled. Otherwise initialise with default values
	if ( aDLOnly )
		{
		Init(0, 0);
		}
	else
		{
		Init(SLAB_CONFIG, 16);
		}
	
#ifdef ENABLE_BTRACE
	
	TUint32 traceData[4];
	traceData[0] = iMinLength;
	traceData[1] = iMaxLength;
	traceData[2] = iGrowBy;
	traceData[3] = iAlign;
	BTraceContextN(BTrace::ETest1, 90, (TUint32)this, 11, traceData, sizeof(traceData));
#endif
	
}

#ifndef __KERNEL_MODE__
TInt RHybridHeap::ConstructLock(TUint32 aMode)
{
	TBool duplicateLock = EFalse;
	TInt r = KErrNone;
	if (!(iFlags & ESingleThreaded))
		{
		duplicateLock = aMode & UserHeap::EChunkHeapSwitchTo;
		r = iLock.CreateLocal(duplicateLock ? EOwnerThread : EOwnerProcess);
		if( r != KErrNone)
			{
			iChunkHandle = 0;
			return r;
			}
		}
	
	if ( aMode & UserHeap::EChunkHeapSwitchTo )
		User::SwitchHeap(this);
	
	iHandles = &iChunkHandle;
	if (!(iFlags & ESingleThreaded))
		{
		// now change the thread-relative chunk/semaphore handles into process-relative handles
		iHandleCount = 2;
		if(duplicateLock)
			{
			RHandleBase s = iLock;
			r = iLock.Duplicate(RThread());
			s.Close();
			}
		if (r==KErrNone && (aMode & UserHeap::EChunkHeapDuplicate))
			{
			r = ((RChunk*)&iChunkHandle)->Duplicate(RThread());
			if (r!=KErrNone)
				iLock.Close(), iChunkHandle=0;
			}
		}
	else
		{
		iHandleCount = 1;
		if (aMode & UserHeap::EChunkHeapDuplicate)
			r = ((RChunk*)&iChunkHandle)->Duplicate(RThread(), EOwnerThread);
		}
	
	return r;
}
#endif

void RHybridHeap::Init(TInt aBitmapSlab, TInt aPagePower)
{
	/*Moved code which does initialization */
	iTop  = (TUint8*)this + iMinLength;
	iBase = Ceiling(iBase, ECellAlignment);	// Align iBase address 
	
    __INIT_COUNTERS(0);
	//	memset(&mparams,0,sizeof(mparams));
	
	InitDlMalloc(iTop - iBase, 0);
	
#ifndef __KERNEL_MODE__	
	SlabInit();
	iSlabConfigBits = aBitmapSlab;
	if ( iChunkSize > iSlabInitThreshold )
		{
		iSlabInitThreshold = KMaxTInt32;
		SlabConfig(aBitmapSlab);   // Delayed slab configuration done
		}
	if ( aPagePower )
		{
		RChunk chunk;
		chunk.SetHandle(iChunkHandle);
		iMemBase = chunk.Base();    // Store base address for paged allocator
		}

	/*10-1K,11-2K,12-4k,13-8K,14-16K,15-32K,16-64K*/
	PagedInit(aPagePower);

#ifdef ENABLE_BTRACE
	TUint32 traceData[3];
	traceData[0] = aBitmapSlab;
	traceData[1] = aPagePower;
	traceData[2] = GM->iTrimCheck;
	BTraceContextN(BTrace::ETest1, 90, (TUint32)this, 0, traceData, sizeof(traceData));
#endif
#else
	(void)aBitmapSlab;
	(void)aPagePower;
#endif	// __KERNEL_MODE__
	
}


TInt RHybridHeap::AllocLen(const TAny* aCell) const
{
	aCell = __GET_DEBUG_DATA_BFR(aCell);
	
	if (PtrDiff(aCell, this) >= 0)
		{
		mchunkptr m = MEM2CHUNK(aCell);
		return CHUNKSIZE(m) - OVERHEAD_FOR(m) - __DEBUG_HDR_SIZE;
		}
#ifndef __KERNEL_MODE__		
	if ( aCell )
		{
		if (LowBits(aCell, iPageSize) )
			return SlabHeaderSize(slab::SlabFor(aCell)->iHeader) - __DEBUG_HDR_SIZE;
		
		return PagedSize((void*)aCell) - __DEBUG_HDR_SIZE;
		}
#endif	
	return 0; // NULL pointer situation, should PANIC !!
}

#ifdef __KERNEL_MODE__
TAny* RHybridHeap::Alloc(TInt aSize)
{
	__CHECK_THREAD_STATE;
	__ASSERT_ALWAYS((TUint)aSize<(KMaxTInt/2),HEAP_PANIC(ETHeapBadAllocatedCellSize));
	__SIMULATE_ALLOC_FAIL(return NULL;)
	Lock();
	__ALLOC_DEBUG_HEADER(aSize);
	TAny* addr = DlMalloc(aSize);
	if ( addr )
		{
//		iCellCount++;
		__SET_DEBUG_DATA(addr, iNestingLevel, ++iAllocCount);
		addr = __GET_USER_DATA_BFR(addr);
		__INCREMENT_COUNTERS(addr);
		memclr(addr, AllocLen(addr));
		}
	Unlock();
#ifdef ENABLE_BTRACE
	if (iFlags & ETraceAllocs)
		{
		if ( addr )
			{
			TUint32 traceData[3];
			traceData[0] = AllocLen(addr);
			traceData[1] = aSize - __DEBUG_HDR_SIZE;
			traceData[2] = 0;
			BTraceContextN(BTrace::EHeap, BTrace::EHeapAlloc, (TUint32)this, (TUint32)addr, traceData, sizeof(traceData));
			}
		else
			BTraceContext8(BTrace::EHeap, BTrace::EHeapAllocFail, (TUint32)this, (TUint32)(aSize - __DEBUG_HDR_SIZE));			
		}
#endif
	return addr;
}
#else

TAny* RHybridHeap::Alloc(TInt aSize)
{
	__ASSERT_ALWAYS((TUint)aSize<(KMaxTInt/2),HEAP_PANIC(ETHeapBadAllocatedCellSize));
	__SIMULATE_ALLOC_FAIL(return NULL;)
			
	TAny* addr;
#ifdef ENABLE_BTRACE
	TInt aSubAllocator=0;
#endif
	
	Lock();
	
	__ALLOC_DEBUG_HEADER(aSize);
	
	if (aSize < iSlabThreshold)
		{
		TInt ix = iSizeMap[(aSize+3)>>2];
		HEAP_ASSERT(ix != 0xff);
		addr = SlabAllocate(iSlabAlloc[ix]);
		if ( !addr )
			{ // Slab allocation has failed, try to allocate from DL
			addr = DlMalloc(aSize);
			}
#ifdef ENABLE_BTRACE
		else
			aSubAllocator=1;
#endif
		}else if((aSize >> iPageThreshold)==0)
			{
			addr = DlMalloc(aSize);
			}
		else
			{
			addr = PagedAllocate(aSize);
			if ( !addr )
				{ // Page allocation has failed, try to allocate from DL
				addr = DlMalloc(aSize);
				}
#ifdef ENABLE_BTRACE
			else
	      		aSubAllocator=2;
#endif
			}
	
	if ( addr )
		{
//		iCellCount++;
		__SET_DEBUG_DATA(addr, iNestingLevel, ++iAllocCount);
		addr = __GET_USER_DATA_BFR(addr);
		__INCREMENT_COUNTERS(addr);
		}
	Unlock();
	
#ifdef ENABLE_BTRACE
	if (iFlags & ETraceAllocs)
		{
		if ( addr )
			{
		    TUint32 traceData[3];
		    traceData[0] = AllocLen(addr);
		    traceData[1] = aSize - __DEBUG_HDR_SIZE;
			traceData[2] = aSubAllocator;
		    BTraceContextN(BTrace::EHeap, BTrace::EHeapAlloc, (TUint32)this, (TUint32)addr, traceData, sizeof(traceData));
			}
		else
			BTraceContext8(BTrace::EHeap, BTrace::EHeapAllocFail, (TUint32)this, (TUint32)(aSize - __DEBUG_HDR_SIZE));			
		}
#endif
	
	return addr;
}
#endif // __KERNEL_MODE__

#ifndef __KERNEL_MODE__
TInt RHybridHeap::Compress()
{
	if ( IS_FIXED_HEAP )
		return 0;
	
	Lock();
	TInt Reduced = SysTrim(GM, 0);
	if (iSparePage)
		{
		Unmap(iSparePage, iPageSize);
		iSparePage = 0;
		Reduced += iPageSize;
		}
	Unlock();
	return Reduced;
}
#endif

void RHybridHeap::Free(TAny* aPtr)
{
	__CHECK_THREAD_STATE;
	if ( !aPtr )
		return;
#ifdef ENABLE_BTRACE
	TInt aSubAllocator=0;
#endif
	Lock();
	
	aPtr = __GET_DEBUG_DATA_BFR(aPtr);

#ifndef __KERNEL_MODE__				
	if (PtrDiff(aPtr, this) >= 0)
		{
#endif
		__DL_BFR_CHECK(GM, aPtr);
		__DECREMENT_COUNTERS(__GET_USER_DATA_BFR(aPtr));
		__ZAP_CELL(aPtr);		
		DlFree( aPtr);
#ifndef __KERNEL_MODE__			
		}

	else if ( LowBits(aPtr, iPageSize) == 0 )
		{
#ifdef ENABLE_BTRACE
		aSubAllocator = 2;
#endif
		__PAGE_BFR_CHECK(aPtr);	
		__DECREMENT_COUNTERS(__GET_USER_DATA_BFR(aPtr));
		PagedFree(aPtr);
		}
	else
		{
#ifdef ENABLE_BTRACE
		aSubAllocator = 1;
#endif
		TUint32 bm[4];   		
		__SLAB_BFR_CHECK(slab::SlabFor(aPtr),aPtr,bm);
		__DECREMENT_COUNTERS(__GET_USER_DATA_BFR(aPtr));  
		__ZAP_CELL(aPtr);		
		SlabFree(aPtr);
		}
#endif  // __KERNEL_MODE__	
//	iCellCount--;
	Unlock();
#ifdef ENABLE_BTRACE
	if (iFlags & ETraceAllocs)
		{
		TUint32 traceData;
		traceData = aSubAllocator;
		BTraceContextN(BTrace::EHeap, BTrace::EHeapFree, (TUint32)this, (TUint32)__GET_USER_DATA_BFR(aPtr), &traceData, sizeof(traceData));
		}
#endif
}

#ifndef __KERNEL_MODE__
void RHybridHeap::Reset()
/**
Frees all allocated cells on this heap.
*/
{
	Lock();
	if ( !IS_FIXED_HEAP )
		{
		if ( GM->iSeg.iSize > (iMinLength - sizeof(*this)) )
			Unmap(GM->iSeg.iBase + (iMinLength - sizeof(*this)), (GM->iSeg.iSize - (iMinLength - sizeof(*this))));
		ResetBitmap();
		if ( !iDLOnly )
			Init(iSlabConfigBits, iPageThreshold);
		else
			Init(0,0);		
		}
	else Init(0,0);
	Unlock();	
}
#endif

TAny* RHybridHeap::ReAllocImpl(TAny* aPtr, TInt aSize, TInt aMode)
{
	// First handle special case of calling reallocate with NULL aPtr
	if (!aPtr)
		{
		if (( aMode & ENeverMove ) == 0 )
			{
			aPtr = Alloc(aSize - __DEBUG_HDR_SIZE);
			aPtr = __GET_DEBUG_DATA_BFR(aPtr);
			}
		return aPtr;
		}
	
	TInt oldsize = AllocLen(__GET_USER_DATA_BFR(aPtr)) + __DEBUG_HDR_SIZE;
	
	// Insist on geometric growth when reallocating memory, this reduces copying and fragmentation
	// generated during arithmetic growth of buffer/array/vector memory
	// Experiments have shown that 25% is a good threshold for this policy
	if (aSize <= oldsize)
		{
		if (aSize >= oldsize - (oldsize>>2))
			return aPtr;		// don't change if >75% original size
		}
	else
		{
		__SIMULATE_ALLOC_FAIL(return NULL;)
		if (aSize < oldsize + (oldsize>>2))
			{
			aSize = _ALIGN_UP(oldsize + (oldsize>>2), 4);	// grow to at least 125% original size
			}
		}
	__DEBUG_SAVE(aPtr);		
	
	TAny* newp;
#ifdef __KERNEL_MODE__
	Lock();
	__DL_BFR_CHECK(GM, aPtr);				
	newp = DlRealloc(aPtr, aSize, aMode);
	Unlock();
	if ( newp )
		{
	    if ( aSize > oldsize )
			memclr(((TUint8*)newp) + oldsize, (aSize-oldsize)); // Buffer has grown in place, clear extra
		__DEBUG_RESTORE(newp);
		__UPDATE_ALLOC_COUNT(aPtr, newp, ++iAllocCount);
		__UPDATE_TOTAL_ALLOC(newp, oldsize);
		}
#else
	// Decide how to reallocate based on (a) the current cell location, (b) the mode requested and (c) the new size
	if ( PtrDiff(aPtr, this) >= 0 )
		{	// current cell in Doug Lea iArena
		if ( (aMode & ENeverMove) 
			 ||
			 (!(aMode & EAllowMoveOnShrink) && (aSize < oldsize))
			 ||
			 ((aSize >= iSlabThreshold) && ((aSize >> iPageThreshold) == 0)) )
			{
			Lock();
			__DL_BFR_CHECK(GM, aPtr);			
			newp = DlRealloc(aPtr, aSize, aMode);			// old and new in DL allocator
			Unlock();
			__DEBUG_RESTORE(newp);
			__UPDATE_ALLOC_COUNT(aPtr,newp, ++iAllocCount);
			__UPDATE_TOTAL_ALLOC(newp, oldsize);
			return newp;
			}
		}
	else if (LowBits(aPtr, iPageSize) == 0)
		{	// current cell in paged iArena
		if ( (aMode & ENeverMove)     
			 ||
			 (!(aMode & EAllowMoveOnShrink) && (aSize < oldsize)) 
			 ||
			 ((aSize >> iPageThreshold) != 0) )
			{
			Lock();
			__PAGE_BFR_CHECK(aPtr);			
			newp = PagedReallocate(aPtr, aSize, aMode);		// old and new in paged allocator
			Unlock();
			__DEBUG_RESTORE(newp);
			__UPDATE_ALLOC_COUNT(aPtr,newp, ++iAllocCount);
			__UPDATE_TOTAL_ALLOC(newp, oldsize);
			return newp;
			}
		}
	else
		{	// current cell in slab iArena
		TUint32 bm[4];
		Lock();
		__SLAB_BFR_CHECK(slab::SlabFor(aPtr), aPtr, bm);
		Unlock();
		if ( aSize <= oldsize)
			return aPtr;
		if (aMode & ENeverMove)
			return NULL;		// cannot grow in slab iArena
		// just use alloc/copy/free...
		}
	
	// fallback to allocate and copy
	// shouldn't get here if we cannot move the cell
	//  	__ASSERT(mode == emobile || (mode==efixshrink && size>oldsize));
	
	newp = Alloc(aSize - __DEBUG_HDR_SIZE);
	newp = __GET_DEBUG_DATA_BFR(newp);
	if (newp)
		{
		memcpy(newp, aPtr, oldsize<aSize ? oldsize : aSize);
		__DEBUG_RESTORE(newp);
		Free(__GET_USER_DATA_BFR(aPtr));
		}
	
#endif	// __KERNEL_MODE__
	return newp;
}


TAny* RHybridHeap::ReAlloc(TAny* aPtr, TInt aSize, TInt aMode )
{
	
	aPtr = __GET_DEBUG_DATA_BFR(aPtr);
	__ALLOC_DEBUG_HEADER(aSize);
	
	TAny* retval = ReAllocImpl(aPtr, aSize, aMode);
	
	retval = __GET_USER_DATA_BFR(retval);
	
#ifdef ENABLE_BTRACE
	if (iFlags & ETraceAllocs)
		{
		if ( retval )
			{
			TUint32 traceData[3];
			traceData[0] = AllocLen(retval);
			traceData[1] = aSize - __DEBUG_HDR_SIZE;
			traceData[2] = (TUint32)aPtr;
			BTraceContextN(BTrace::EHeap, BTrace::EHeapReAlloc,(TUint32)this, (TUint32)retval, traceData, sizeof(traceData));
			}
  		else
  		    BTraceContext12(BTrace::EHeap, BTrace::EHeapReAllocFail, (TUint32)this, (TUint32)aPtr, (TUint32)(aSize - __DEBUG_HDR_SIZE));
		}
#endif
	return retval;
}

#ifndef __KERNEL_MODE__
TInt RHybridHeap::Available(TInt& aBiggestBlock) const
/**
Gets the total free space currently available on the heap and the space
available in the largest free block.

Note that this function exists mainly for compatibility reasons.  In a modern
heap implementation such as that present in Symbian it is not appropriate to
concern oneself with details such as the amount of free memory available on a
heap and its largeset free block, because the way that a modern heap implementation
works is not simple.  The amount of available virtual memory != physical memory
and there are multiple allocation strategies used internally, which makes all
memory usage figures "fuzzy" at best.

In short, if you want to see if there is enough memory available to allocate a
block of memory, call Alloc() and if it succeeds then there is enough memory!
Messing around with functions like this is somewhat pointless with modern heap
allocators.

@param aBiggestBlock On return, contains the space available in the largest
                     free block on the heap.  Due to the internals of modern
                     heap implementations, you can probably still allocate a
                     block larger than this!

@return The total free space currently available on the heap.  Again, you can
        probably still allocate more than this!
*/
{
	struct HeapInfo info;
	Lock();
	TInt Biggest  = GetInfo(&info);
	aBiggestBlock = __GET_AVAIL_BLOCK_SIZE(Biggest);
	Unlock();
	return __GET_AVAIL_BLOCK_SIZE(info.iFreeBytes);
	
}

TInt RHybridHeap::AllocSize(TInt& aTotalAllocSize) const
   /**
   Gets the number of cells allocated on this heap, and the total space 
   allocated to them.
   
   @param aTotalAllocSize On return, contains the total space allocated
   to the cells.
   
   @return The number of cells allocated on this heap.
*/   
{
	struct HeapInfo info;
	Lock();
	GetInfo(&info);
	aTotalAllocSize = info.iAllocBytes - __REMOVE_DBG_HDR(info.iAllocN);
	Unlock();
	return info.iAllocN;
}

#endif

TInt RHybridHeap::Extension_(TUint /* aExtensionId */, TAny*& /* a0 */, TAny* /* a1 */)
{
	return KErrNotSupported;
}



///////////////////////////////////////////////////////////////////////////////
// imported from dla.cpp
///////////////////////////////////////////////////////////////////////////////

//#include <unistd.h>
//#define DEBUG_REALLOC
#ifdef DEBUG_REALLOC
#include <e32debug.h>
#endif

inline void RHybridHeap::InitBins(mstate m)
{
	/* Establish circular links for iSmallBins */
	bindex_t i;
	for (i = 0; i < NSMALLBINS; ++i) {
		sbinptr bin = SMALLBIN_AT(m,i);
		bin->iFd = bin->iBk = bin;
		}
	}
/* ---------------------------- malloc support --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
void* RHybridHeap::TmallocLarge(mstate m, size_t nb) {
	tchunkptr v = 0;
	size_t rsize = -nb; /* Unsigned negation */
	tchunkptr t;
	bindex_t idx;
	ComputeTreeIndex(nb, idx);
	
	if ((t = *TREEBIN_AT(m, idx)) != 0)
		{
		/* Traverse tree for this bin looking for node with size == nb */
		size_t sizebits = nb << LEFTSHIFT_FOR_TREE_INDEX(idx);
		tchunkptr rst = 0;  /* The deepest untaken right subtree */
		for (;;)
			{
			tchunkptr rt;
			size_t trem = CHUNKSIZE(t) - nb;
			if (trem < rsize)
				{
				v = t;
				if ((rsize = trem) == 0)
					break;
				}
			rt = t->iChild[1];
			t = t->iChild[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
			if (rt != 0 && rt != t)
				rst = rt;
			if (t == 0)
				{
				t = rst; /* set t to least subtree holding sizes > nb */
				break;
				}
			sizebits <<= 1;
			}
		}
	if (t == 0 && v == 0)
		{ /* set t to root of next non-empty treebin */
		binmap_t leftbits = LEFT_BITS(IDX2BIT(idx)) & m->iTreeMap;
		if (leftbits != 0)
			{
			bindex_t i;
			binmap_t leastbit = LEAST_BIT(leftbits);
			ComputeBit2idx(leastbit, i);
			t = *TREEBIN_AT(m, i);
			}
		}
	while (t != 0)
		{ /* Find smallest of tree or subtree */
		size_t trem = CHUNKSIZE(t) - nb;
		if (trem < rsize) {
			rsize = trem;
			v = t;
			}
		t = LEFTMOST_CHILD(t);
		}
	/*  If iDv is a better fit, return 0 so malloc will use it */
	if (v != 0 && rsize < (size_t)(m->iDvSize - nb))
		{
		if (RTCHECK(OK_ADDRESS(m, v)))
			{ /* split */
			mchunkptr r = CHUNK_PLUS_OFFSET(v, nb);
			HEAP_ASSERT(CHUNKSIZE(v) == rsize + nb);
			if (RTCHECK(OK_NEXT(v, r)))
				{
				UnlinkLargeChunk(m, v);
				if (rsize < MIN_CHUNK_SIZE)
					SET_INUSE_AND_PINUSE(m, v, (rsize + nb));
				else
					{
					SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(m, v, nb);
					SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(r, rsize);
					InsertChunk(m, r, rsize);
					}
				return CHUNK2MEM(v);
				}
			}
		//    CORRUPTION_ERROR_ACTION(m);
		}
	return 0;
	}

/* allocate a small request from the best fitting chunk in a treebin */
void* RHybridHeap::TmallocSmall(mstate m, size_t nb)
{
	tchunkptr t, v;
	size_t rsize;
	bindex_t i;
	binmap_t leastbit = LEAST_BIT(m->iTreeMap);
	ComputeBit2idx(leastbit, i);
	
	v = t = *TREEBIN_AT(m, i);
	rsize = CHUNKSIZE(t) - nb;
	
	while ((t = LEFTMOST_CHILD(t)) != 0)
		{
		size_t trem = CHUNKSIZE(t) - nb;
		if (trem < rsize)
			{
			rsize = trem;
			v = t;
			}
		}
	
	if (RTCHECK(OK_ADDRESS(m, v)))
		{
		mchunkptr r = CHUNK_PLUS_OFFSET(v, nb);
		HEAP_ASSERT(CHUNKSIZE(v) == rsize + nb);
		if (RTCHECK(OK_NEXT(v, r)))
			{
			UnlinkLargeChunk(m, v);
			if (rsize < MIN_CHUNK_SIZE)
				SET_INUSE_AND_PINUSE(m, v, (rsize + nb));
			else
				{
				SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(m, v, nb);
				SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(r, rsize);
				ReplaceDv(m, r, rsize);
				}
			return CHUNK2MEM(v);
			}
		}
	//  CORRUPTION_ERROR_ACTION(m);
	//  return 0;
	}

inline void RHybridHeap::InitTop(mstate m, mchunkptr p, size_t psize)
{
	/* Ensure alignment */
	size_t offset = ALIGN_OFFSET(CHUNK2MEM(p));
	p = (mchunkptr)((TUint8*)p + offset);
	psize -= offset;
	m->iTop = p;
	m->iTopSize = psize;
	p->iHead = psize | PINUSE_BIT;
	/* set size of fake trailing chunk holding overhead space only once */
	mchunkptr chunkPlusOff = CHUNK_PLUS_OFFSET(p, psize);
	chunkPlusOff->iHead = TOP_FOOT_SIZE;
	m->iTrimCheck = KHeapShrinkHysRatio*(iGrowBy>>8);
}


/* Unlink the first chunk from a smallbin */
inline void RHybridHeap::UnlinkFirstSmallChunk(mstate M,mchunkptr B,mchunkptr P,bindex_t& I)
{
	mchunkptr F = P->iFd;
	HEAP_ASSERT(P != B);
	HEAP_ASSERT(P != F);
	HEAP_ASSERT(CHUNKSIZE(P) == SMALL_INDEX2SIZE(I));
	if (B == F)
		CLEAR_SMALLMAP(M, I);
	else if (RTCHECK(OK_ADDRESS(M, F)))
		{
		B->iFd = F;
		F->iBk = B;
		}
	else
		{
		CORRUPTION_ERROR_ACTION(M);
		}
}
/* Link a free chunk into a smallbin  */
inline void RHybridHeap::InsertSmallChunk(mstate M,mchunkptr P, size_t S)
{
	bindex_t I  = SMALL_INDEX(S);
	mchunkptr B = SMALLBIN_AT(M, I);
	mchunkptr F = B;
	HEAP_ASSERT(S >= MIN_CHUNK_SIZE);
	if (!SMALLMAP_IS_MARKED(M, I))
		MARK_SMALLMAP(M, I);
	else if (RTCHECK(OK_ADDRESS(M, B->iFd)))
		F = B->iFd;
	else
		{
		CORRUPTION_ERROR_ACTION(M);
		}
	B->iFd = P;
	F->iBk = P;
	P->iFd = F;
	P->iBk = B;
}


inline void RHybridHeap::InsertChunk(mstate M,mchunkptr P,size_t S)
{
	if (IS_SMALL(S))
		InsertSmallChunk(M, P, S);
	else
		{
		tchunkptr TP = (tchunkptr)(P); InsertLargeChunk(M, TP, S);
		}
}

inline void RHybridHeap::UnlinkLargeChunk(mstate M,tchunkptr X)
{
	tchunkptr XP = X->iParent;
	tchunkptr R;
	if (X->iBk != X)
		{
		tchunkptr F = X->iFd;
		R = X->iBk;
		if (RTCHECK(OK_ADDRESS(M, F)))
			{
			F->iBk = R;
			R->iFd = F;
			}
		else
			{
			CORRUPTION_ERROR_ACTION(M);
			}
		}
	else
		{
		tchunkptr* RP;
		if (((R = *(RP = &(X->iChild[1]))) != 0) ||
			  ((R = *(RP = &(X->iChild[0]))) != 0))
			{
			tchunkptr* CP;
			while ((*(CP = &(R->iChild[1])) != 0) ||
				   (*(CP = &(R->iChild[0])) != 0))
				{
				R = *(RP = CP);
				}
			if (RTCHECK(OK_ADDRESS(M, RP)))
				*RP = 0;
			else
				{
				CORRUPTION_ERROR_ACTION(M);
				}
			}
		}
	if (XP != 0)
		{
		tbinptr* H = TREEBIN_AT(M, X->iIndex);
		if (X == *H)
			{
			if ((*H = R) == 0)
				CLEAR_TREEMAP(M, X->iIndex);
			}
		else if (RTCHECK(OK_ADDRESS(M, XP)))
			{
			if (XP->iChild[0] == X)
				XP->iChild[0] = R;
			else
				XP->iChild[1] = R;
			}
		else
			CORRUPTION_ERROR_ACTION(M);
		if (R != 0)
			{
			if (RTCHECK(OK_ADDRESS(M, R)))
				{
				tchunkptr C0, C1;
				R->iParent = XP;
				if ((C0 = X->iChild[0]) != 0)
					{
					if (RTCHECK(OK_ADDRESS(M, C0)))
						{
						R->iChild[0] = C0;
						C0->iParent = R;
						}
					else
						CORRUPTION_ERROR_ACTION(M);
					}
				if ((C1 = X->iChild[1]) != 0)
					{
					if (RTCHECK(OK_ADDRESS(M, C1)))
						{
						R->iChild[1] = C1;
						C1->iParent = R;
						}
					else
						CORRUPTION_ERROR_ACTION(M);
					}
				}
			else
				CORRUPTION_ERROR_ACTION(M);
			}
		}
}

/* Unlink a chunk from a smallbin  */
inline void RHybridHeap::UnlinkSmallChunk(mstate M, mchunkptr P,size_t S)
{
	mchunkptr F = P->iFd;
	mchunkptr B = P->iBk;
	bindex_t I = SMALL_INDEX(S);
	HEAP_ASSERT(P != B);
	HEAP_ASSERT(P != F);
	HEAP_ASSERT(CHUNKSIZE(P) == SMALL_INDEX2SIZE(I));
	if (F == B)
		CLEAR_SMALLMAP(M, I);
	else if (RTCHECK((F == SMALLBIN_AT(M,I) || OK_ADDRESS(M, F)) &&
					 (B == SMALLBIN_AT(M,I) || OK_ADDRESS(M, B))))
		{
		F->iBk = B;
		B->iFd = F;
		}
	else
		{
		CORRUPTION_ERROR_ACTION(M);
		}
}

inline void RHybridHeap::UnlinkChunk(mstate M, mchunkptr P, size_t S)
{
	if (IS_SMALL(S))
		UnlinkSmallChunk(M, P, S);
	else
		{
		tchunkptr TP = (tchunkptr)(P); UnlinkLargeChunk(M, TP);
		}
}

// For DL debug functions
void RHybridHeap::DoComputeTreeIndex(size_t S, bindex_t& I)
{
	ComputeTreeIndex(S, I);
}	

inline void RHybridHeap::ComputeTreeIndex(size_t S, bindex_t& I)
{
	size_t X = S >> TREEBIN_SHIFT;
	if (X == 0)
		I = 0;
	else if (X > 0xFFFF)
		I = NTREEBINS-1;
	else
		{
		unsigned int Y = (unsigned int)X;
		unsigned int N = ((Y - 0x100) >> 16) & 8;
		unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;
		N += K;
		N += K = (((Y <<= K) - 0x4000) >> 16) & 2;
		K = 14 - N + ((Y <<= K) >> 15);
		I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1));
		}
}

/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
inline void RHybridHeap::InsertLargeChunk(mstate M,tchunkptr X,size_t S)
{
	tbinptr* H;
	bindex_t I;
	ComputeTreeIndex(S, I);
	H = TREEBIN_AT(M, I);
	X->iIndex = I;
	X->iChild[0] = X->iChild[1] = 0;
	if (!TREEMAP_IS_MARKED(M, I))
		{
		MARK_TREEMAP(M, I);
		*H = X;
		X->iParent = (tchunkptr)H;
		X->iFd = X->iBk = X;
		}
	else
		{
		tchunkptr T = *H;
		size_t K = S << LEFTSHIFT_FOR_TREE_INDEX(I);
		for (;;)
			{
			if (CHUNKSIZE(T) != S) {
				tchunkptr* C = &(T->iChild[(K >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1]);
				K <<= 1;
				if (*C != 0)
					T = *C;
				else if (RTCHECK(OK_ADDRESS(M, C)))
					{
					*C = X;
					X->iParent = T;
					X->iFd = X->iBk = X;
					break;
					}
				else
					{
					CORRUPTION_ERROR_ACTION(M);
					break;
					}
				}
			else
				{
				tchunkptr F = T->iFd;
				if (RTCHECK(OK_ADDRESS(M, T) && OK_ADDRESS(M, F)))
					{
					T->iFd = F->iBk = X;
					X->iFd = F;
					X->iBk = T;
					X->iParent = 0;
					break;
					}
				else
					{
					CORRUPTION_ERROR_ACTION(M);
					break;
					}
				}
			}
		}
}

/*
Unlink steps:

1. If x is a chained node, unlink it from its same-sized iFd/iBk links
and choose its iBk node as its replacement.
2. If x was the last node of its size, but not a leaf node, it must
be replaced with a leaf node (not merely one with an open left or
right), to make sure that lefts and rights of descendents
correspond properly to bit masks.  We use the rightmost descendent
of x.  We could use any other leaf, but this is easy to locate and
tends to counteract removal of leftmosts elsewhere, and so keeps
paths shorter than minimally guaranteed.  This doesn't loop much
because on average a node in a tree is near the bottom.
3. If x is the base of a chain (i.e., has iParent links) relink
x's iParent and children to x's replacement (or null if none).
*/

/* Replace iDv node, binning the old one */
/* Used only when iDvSize known to be small */
inline void RHybridHeap::ReplaceDv(mstate M, mchunkptr P, size_t S)
{
	size_t DVS = M->iDvSize;
	if (DVS != 0)
		{
		mchunkptr DV = M->iDv;
		HEAP_ASSERT(IS_SMALL(DVS));
		InsertSmallChunk(M, DV, DVS);
		}
	M->iDvSize = S;
	M->iDv = P;
}


inline void RHybridHeap::ComputeBit2idx(binmap_t X,bindex_t& I)
{
	unsigned int Y = X - 1;
	unsigned int K = Y >> (16-4) & 16;
	unsigned int N = K;        Y >>= K;
	N += K = Y >> (8-3) &  8;  Y >>= K;
	N += K = Y >> (4-2) &  4;  Y >>= K;
	N += K = Y >> (2-1) &  2;  Y >>= K;
	N += K = Y >> (1-0) &  1;  Y >>= K;
	I = (bindex_t)(N + Y);
}



int RHybridHeap::SysTrim(mstate m, size_t pad)
{
	size_t extra = 0;
	
	if ( IS_INITIALIZED(m) )
		{
		pad += TOP_FOOT_SIZE; /* ensure enough room for segment overhead */
		
		if (m->iTopSize > pad)
			{
			extra = Floor(m->iTopSize - pad, iPageSize);
			if ( (m->iSeg.iSize - extra) < (iMinLength - sizeof(*this)) )			  
				{
				if ( m->iSeg.iSize > (iMinLength - sizeof(*this)) )				  
					extra = Floor(m->iSeg.iSize - (iMinLength - sizeof(*this)), iPageSize); /* do not shrink heap below min length */
				else extra = 0;
				}
			
			if ( extra )
				{
				Unmap(m->iSeg.iBase + m->iSeg.iSize - extra, extra);
				
				m->iSeg.iSize -= extra;
				InitTop(m, m->iTop, m->iTopSize - extra);
				CHECK_TOP_CHUNK(m, m->iTop);
				}
			}
		
		}
	
	return extra;
}

/* Get memory from system using MORECORE */

void* RHybridHeap::SysAlloc(mstate m, size_t nb)
{
	HEAP_ASSERT(m->iTop);
	/* Subtract out existing available iTop space from MORECORE request. */
//	size_t asize = _ALIGN_UP(nb - m->iTopSize + TOP_FOOT_SIZE + SIZE_T_ONE, iGrowBy);  
  	TInt asize = _ALIGN_UP(nb - m->iTopSize + SYS_ALLOC_PADDING, iGrowBy);  // From DLA version 2.8.4
	
	char* br = (char*)Map(m->iSeg.iBase+m->iSeg.iSize, asize);
	if (!br)
		return 0;
	HEAP_ASSERT(br == (char*)m->iSeg.iBase+m->iSeg.iSize);
	
	/* Merge with an existing segment */
	m->iSeg.iSize += asize;
	InitTop(m, m->iTop, m->iTopSize + asize);
	
	if (nb < m->iTopSize)
		{ /* Allocate from new or extended iTop space */
		size_t rsize = m->iTopSize -= nb;
		mchunkptr p = m->iTop;
		mchunkptr r = m->iTop = CHUNK_PLUS_OFFSET(p, nb);
		r->iHead = rsize | PINUSE_BIT;
		SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(m, p, nb);
		CHECK_TOP_CHUNK(m, m->iTop);
		CHECK_MALLOCED_CHUNK(m, CHUNK2MEM(p), nb);
		return CHUNK2MEM(p);
		}
	
	return 0;
}	


void RHybridHeap::InitDlMalloc(size_t capacity, int /*locked*/)
{
	memset(GM,0,sizeof(malloc_state));
	// The maximum amount that can be allocated can be calculated as:-
	// 2^sizeof(size_t) - sizeof(malloc_state) - TOP_FOOT_SIZE - page Size(all accordingly padded)
	// If the capacity exceeds this, no allocation will be done.
	GM->iSeg.iBase = iBase;
	GM->iSeg.iSize = capacity;
	InitBins(GM);
	InitTop(GM, (mchunkptr)iBase, capacity - TOP_FOOT_SIZE);
}

void* RHybridHeap::DlMalloc(size_t bytes) 
{
	/*
	Basic algorithm:
	If a small request (< 256 bytes minus per-chunk overhead):
	1. If one exists, use a remainderless chunk in associated smallbin.
	(Remainderless means that there are too few excess bytes to
	represent as a chunk.)
	2. If it is big enough, use the iDv chunk, which is normally the
	chunk adjacent to the one used for the most recent small request.
	3. If one exists, split the smallest available chunk in a bin,
	saving remainder in iDv.
	4. If it is big enough, use the iTop chunk.
	5. If available, get memory from system and use it
	Otherwise, for a large request:
	1. Find the smallest available binned chunk that fits, and use it
	if it is better fitting than iDv chunk, splitting if necessary.
	2. If better fitting than any binned chunk, use the iDv chunk.
	3. If it is big enough, use the iTop chunk.
	4. If request size >= mmap threshold, try to directly mmap this chunk.
	5. If available, get memory from system and use it
*/
	void* mem;
	size_t nb;
	if (bytes <= MAX_SMALL_REQUEST)
		{
		bindex_t idx;
		binmap_t smallbits;
		nb = (bytes < MIN_REQUEST)? MIN_CHUNK_SIZE : PAD_REQUEST(bytes);
		idx = SMALL_INDEX(nb);
		smallbits = GM->iSmallMap >> idx;
		
		if ((smallbits & 0x3U) != 0)
			{ /* Remainderless fit to a smallbin. */
			mchunkptr b, p;
			idx += ~smallbits & 1;			 /* Uses next bin if idx empty */
			b = SMALLBIN_AT(GM, idx);
			p = b->iFd;
			HEAP_ASSERT(CHUNKSIZE(p) == SMALL_INDEX2SIZE(idx));
			UnlinkFirstSmallChunk(GM, b, p, idx);
			SET_INUSE_AND_PINUSE(GM, p, SMALL_INDEX2SIZE(idx));
			mem = CHUNK2MEM(p);
			CHECK_MALLOCED_CHUNK(GM, mem, nb);
			return mem;
			}
		
		else if (nb > GM->iDvSize)
			{
			if (smallbits != 0)
				{ /* Use chunk in next nonempty smallbin */
				mchunkptr b, p, r;
				size_t rsize;
				bindex_t i;
				binmap_t leftbits = (smallbits << idx) & LEFT_BITS(IDX2BIT(idx));
				binmap_t leastbit = LEAST_BIT(leftbits);
				ComputeBit2idx(leastbit, i);
				b = SMALLBIN_AT(GM, i);
				p = b->iFd;
				HEAP_ASSERT(CHUNKSIZE(p) == SMALL_INDEX2SIZE(i));
				UnlinkFirstSmallChunk(GM, b, p, i);
				rsize = SMALL_INDEX2SIZE(i) - nb;
				/* Fit here cannot be remainderless if 4byte sizes */
				if (SIZE_T_SIZE != 4 && rsize < MIN_CHUNK_SIZE)
					SET_INUSE_AND_PINUSE(GM, p, SMALL_INDEX2SIZE(i));
				else
					{
					SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(GM, p, nb);
					r = CHUNK_PLUS_OFFSET(p, nb);
					SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(r, rsize);
					ReplaceDv(GM, r, rsize);
					}
				mem = CHUNK2MEM(p);
				CHECK_MALLOCED_CHUNK(GM, mem, nb);
				return mem;
				}
			
			else if (GM->iTreeMap != 0 && (mem = TmallocSmall(GM, nb)) != 0)
				{
				CHECK_MALLOCED_CHUNK(GM, mem, nb);
				return mem;
				}
			}
		}
	else if (bytes >= MAX_REQUEST)
		nb = MAX_SIZE_T; /* Too big to allocate. Force failure (in sys alloc) */
	else
		{
		nb = PAD_REQUEST(bytes);
		if (GM->iTreeMap != 0 && (mem = TmallocLarge(GM, nb)) != 0)
			{
			CHECK_MALLOCED_CHUNK(GM, mem, nb);
			return mem;
			}
		}
	
	if (nb <= GM->iDvSize)
		{
		size_t rsize = GM->iDvSize - nb;
		mchunkptr p = GM->iDv;
		if (rsize >= MIN_CHUNK_SIZE)
			{ /* split iDv */
			mchunkptr r = GM->iDv = CHUNK_PLUS_OFFSET(p, nb);
			GM->iDvSize = rsize;
			SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(r, rsize);
			SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(GM, p, nb);
			}
		else
			{ /* exhaust iDv */
			size_t dvs = GM->iDvSize;
			GM->iDvSize = 0;
			GM->iDv = 0;
			SET_INUSE_AND_PINUSE(GM, p, dvs);
			}
		mem = CHUNK2MEM(p);
		CHECK_MALLOCED_CHUNK(GM, mem, nb);
		return mem;
		}
	
	else if (nb < GM->iTopSize)
		{ /* Split iTop */
		size_t rsize = GM->iTopSize -= nb;
		mchunkptr p = GM->iTop;
		mchunkptr r = GM->iTop = CHUNK_PLUS_OFFSET(p, nb);
		r->iHead = rsize | PINUSE_BIT;
		SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(GM, p, nb);
		mem = CHUNK2MEM(p);
		CHECK_TOP_CHUNK(GM, GM->iTop);
		CHECK_MALLOCED_CHUNK(GM, mem, nb);
		return mem;
		}
	
	return SysAlloc(GM, nb);
}


void RHybridHeap::DlFree(void* mem)
{
	/*
	Consolidate freed chunks with preceding or succeeding bordering
	free chunks, if they exist, and then place in a bin.	Intermixed
	with special cases for iTop, iDv, mmapped chunks, and usage errors.
*/
	mchunkptr p	= MEM2CHUNK(mem);
	CHECK_INUSE_CHUNK(GM, p);
	if (RTCHECK(OK_ADDRESS(GM, p) && OK_CINUSE(p)))
		{
		size_t psize = CHUNKSIZE(p);
		mchunkptr next = CHUNK_PLUS_OFFSET(p, psize);
		if (!PINUSE(p))
			{
			size_t prevsize = p->iPrevFoot;
			mchunkptr prev = CHUNK_MINUS_OFFSET(p, prevsize);
			psize += prevsize;
			p = prev;
			if (RTCHECK(OK_ADDRESS(GM, prev)))
				{ /* consolidate backward */
				if (p != GM->iDv)
					{
					UnlinkChunk(GM, p, prevsize);
					}
				else if ((next->iHead & INUSE_BITS) == INUSE_BITS)
					{
					GM->iDvSize = psize;
					SET_FREE_WITH_PINUSE(p, psize, next);
					return;
					}
				}
			else
				{
				USAGE_ERROR_ACTION(GM, p);
				return;
				}
			}
		
		if (RTCHECK(OK_NEXT(p, next) && OK_PINUSE(next)))
			{
			if (!CINUSE(next))
				{	/* consolidate forward */
				if (next == GM->iTop)
					{
					size_t tsize = GM->iTopSize += psize;
					GM->iTop = p;
					p->iHead = tsize | PINUSE_BIT;
					if (p == GM->iDv)
						{
						GM->iDv = 0;
						GM->iDvSize = 0;
						}
					if ( !IS_FIXED_HEAP && SHOULD_TRIM(GM, tsize)  )					
						SysTrim(GM, 0);
					return;
					}
				else if (next == GM->iDv)
					{
					size_t dsize = GM->iDvSize += psize;
					GM->iDv = p;
					SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(p, dsize);
					return;
					}
				else
					{
					size_t nsize = CHUNKSIZE(next);
					psize += nsize;
					UnlinkChunk(GM, next, nsize);
					SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(p, psize);
					if (p == GM->iDv)
						{
						GM->iDvSize = psize;
						return;
						}
					}
				}
			else
				SET_FREE_WITH_PINUSE(p, psize, next);
			InsertChunk(GM, p, psize);
			CHECK_FREE_CHUNK(GM, p);
			return;
			}
		}
}


void* RHybridHeap::DlRealloc(void* oldmem, size_t bytes, TInt mode)
{
	mchunkptr oldp = MEM2CHUNK(oldmem);
	size_t oldsize = CHUNKSIZE(oldp);
	mchunkptr next = CHUNK_PLUS_OFFSET(oldp, oldsize);
	mchunkptr newp = 0;
	void* extra = 0;
	
	/* Try to either shrink or extend into iTop. Else malloc-copy-free */
	
	if (RTCHECK(OK_ADDRESS(GM, oldp) && OK_CINUSE(oldp) &&
				OK_NEXT(oldp, next) && OK_PINUSE(next)))
		{
		size_t nb = REQUEST2SIZE(bytes);
		if (oldsize >= nb) { /* already big enough */
			size_t rsize = oldsize - nb;
			newp = oldp;
			if (rsize >= MIN_CHUNK_SIZE)
				{     
				mchunkptr remainder = CHUNK_PLUS_OFFSET(newp, nb);
				SET_INUSE(GM, newp, nb);
//				SET_INUSE(GM, remainder, rsize);
 				SET_INUSE_AND_PINUSE(GM, remainder, rsize);  // corrected in original DLA version V2.8.4
				extra = CHUNK2MEM(remainder);
				}
			}
		else if (next == GM->iTop && oldsize + GM->iTopSize > nb)
			{
			/* Expand into iTop */
			size_t newsize = oldsize + GM->iTopSize;
			size_t newtopsize = newsize - nb;
			mchunkptr newtop = CHUNK_PLUS_OFFSET(oldp, nb);
			SET_INUSE(GM, oldp, nb);
			newtop->iHead = newtopsize |PINUSE_BIT;
			GM->iTop = newtop;
			GM->iTopSize = newtopsize;
			newp = oldp;
			}
		}
	else
		{
		USAGE_ERROR_ACTION(GM, oldmem);
		}
	
	if (newp != 0)
		{
		if (extra != 0)
			{
			DlFree(extra);
			}
		CHECK_INUSE_CHUNK(GM, newp);
		return CHUNK2MEM(newp);
		}
	else
		{
		if ( mode & ENeverMove )
			return 0;		// cannot move
		void* newmem = DlMalloc(bytes);
		if (newmem != 0)
			{
			size_t oc = oldsize - OVERHEAD_FOR(oldp);
			memcpy(newmem, oldmem, (oc < bytes)? oc : bytes);
			DlFree(oldmem);
			}
		return newmem;
		}
	//	return 0;
}

size_t RHybridHeap::DlInfo(struct HeapInfo* i, SWalkInfo* wi) const
{
	TInt max = ((GM->iTopSize-1) & ~CHUNK_ALIGN_MASK) - CHUNK_OVERHEAD;
	if ( max < 0 )
		max = 0;
	else ++i->iFreeN;			// iTop always free
	i->iFreeBytes += max;
	
	Walk(wi, GM->iTop, max, EGoodFreeCell, EDougLeaAllocator); // Introduce DL iTop buffer to the walk function 
	
	for (mchunkptr q = ALIGN_AS_CHUNK(GM->iSeg.iBase); q != GM->iTop; q = NEXT_CHUNK(q))
		{
		TInt sz = CHUNKSIZE(q);
		if (!CINUSE(q))
			{
			if ( sz > max )
				max = sz;
			i->iFreeBytes += sz;
			++i->iFreeN;
			Walk(wi, CHUNK2MEM(q), sz, EGoodFreeCell, EDougLeaAllocator); // Introduce DL free buffer to the walk function 
			}
		else
			{
			i->iAllocBytes += sz - CHUNK_OVERHEAD;
			++i->iAllocN;
			Walk(wi, CHUNK2MEM(q), (sz- CHUNK_OVERHEAD), EGoodAllocatedCell, EDougLeaAllocator); // Introduce DL allocated buffer to the walk function 
			}
		}
	return max;  // return largest available chunk size
}

//
// get statistics about the state of the allocator
//
TInt RHybridHeap::GetInfo(struct HeapInfo* i, SWalkInfo* wi) const
{
	memset(i,0,sizeof(HeapInfo));
	i->iFootprint = iChunkSize;
	i->iMaxSize = iMaxLength;
#ifndef __KERNEL_MODE__		
	PagedInfo(i, wi);
	SlabInfo(i, wi);
#endif	
	return DlInfo(i,wi);
}

//
// Methods to commit/decommit memory pages from chunk
//


void* RHybridHeap::Map(void* p, TInt sz)
//
// allocate pages in the chunk
// if p is NULL, Find an allocate the required number of pages (which must lie in the lower half)
// otherwise commit the pages specified
//
{
	HEAP_ASSERT(sz > 0);

	if ( iChunkSize + sz > iMaxLength)
		return 0;

#ifdef __KERNEL_MODE__

	TInt r = ((DChunk*)iChunkHandle)->Adjust(iChunkSize + iOffset + sz);
	if (r < 0)
		return 0;

	iChunkSize += sz;
	
#else	

	RChunk chunk;
	chunk.SetHandle(iChunkHandle);
	if ( p )
		{
		TInt r;
		if ( iUseAdjust )
			r = chunk.Adjust(iChunkSize + sz);
		else
			{
			HEAP_ASSERT(sz == Ceiling(sz, iPageSize));
			HEAP_ASSERT(p == Floor(p, iPageSize));
			r = chunk.Commit(iOffset + PtrDiff(p, this),sz);
			}			
		if (r < 0)
			return 0;
		}
	else
		{
		TInt r = chunk.Allocate(sz);
		if (r < 0)
			return 0;
		if (r > iOffset)
			{
			// can't allow page allocations in DL zone
			chunk.Decommit(r, sz);
			return 0;
			}
		p = Offset(this, r - iOffset);
		}
	iChunkSize += sz;
	
	if (iChunkSize >= iSlabInitThreshold)
		{	// set up slab system now that heap is large enough
		SlabConfig(iSlabConfigBits);
		iSlabInitThreshold = KMaxTInt32;
		}

#endif //	__KERNEL_MODE__
	
#ifdef ENABLE_BTRACE
	if(iChunkSize > iHighWaterMark)
		{
		iHighWaterMark = Ceiling(iChunkSize,16*iPageSize);
		TUint32 traceData[6];
		traceData[0] = iChunkHandle;
		traceData[1] = iMinLength;
		traceData[2] = iMaxLength;
		traceData[3] = sz;
		traceData[4] = iChunkSize;
		traceData[5] = iHighWaterMark;
		BTraceContextN(BTrace::ETest1, 90, (TUint32)this, 33, traceData, sizeof(traceData));
		}
#endif

	return p;
}

void RHybridHeap::Unmap(void* p, TInt sz)
{
	HEAP_ASSERT(sz > 0);
	
#ifdef __KERNEL_MODE__
	
	(void)p;
	HEAP_ASSERT(sz == Ceiling(sz, iPageSize));
#if defined(_DEBUG)		
	TInt r =
#endif				
   ((DChunk*)iChunkHandle)->Adjust(iChunkSize + iOffset - sz);
	HEAP_ASSERT(r >= 0);
	
#else

	RChunk chunk;
	chunk.SetHandle(iChunkHandle);
	if ( iUseAdjust )
		{
		HEAP_ASSERT(sz == Ceiling(sz, iPageSize));
#if defined(_DEBUG)		
		TInt r =
#endif				
		chunk.Adjust(iChunkSize - sz);
		HEAP_ASSERT(r >= 0);		
		}
	else
		{
		HEAP_ASSERT(sz == Ceiling(sz, iPageSize));
		HEAP_ASSERT(p == Floor(p, iPageSize));
#if defined(_DEBUG)		
		TInt r =
#endif
		chunk.Decommit(PtrDiff(p, Offset(this,-iOffset)), sz);
		HEAP_ASSERT(r >= 0);				
		}			
#endif  // __KERNEL_MODE__
	
	iChunkSize -= sz;
}


#ifndef __KERNEL_MODE__
//
// Slab allocator code
//

//inline slab* slab::SlabFor(void* p)
slab* slab::SlabFor( const void* p)
{
	return (slab*)(Floor(p, SLABSIZE));
}

//
// Remove slab s from its tree/heap (not necessarily the root), preserving the address order
// invariant of the heap
//
void RHybridHeap::TreeRemove(slab* s)
{
	slab** r = s->iParent;
	slab* c1 = s->iChild1;
	slab* c2 = s->iChild2;
	for (;;)
		{
		if (!c2)
			{
			*r = c1;
			if (c1)
				c1->iParent = r;
			return;
			}
		if (!c1)
			{
			*r = c2;
			c2->iParent = r;
			return;
			}
		if (c1 > c2)
			{
			slab* c3 = c1;
			c1 = c2;
			c2 = c3;
			}
		slab* newc2 = c1->iChild2;
		*r = c1;
		c1->iParent = r;
		c1->iChild2 = c2;
		c2->iParent = &c1->iChild2;
		s = c1;
		c1 = s->iChild1;
		c2 = newc2;
		r = &s->iChild1;
		}
}
//
// Insert slab s into the tree/heap rooted at r, preserving the address ordering
// invariant of the heap
//
void RHybridHeap::TreeInsert(slab* s,slab** r)
{
	slab* n = *r;
	for (;;)
		{
		if (!n)
			{	// tree empty
			*r = s;
			s->iParent = r;
			s->iChild1 = s->iChild2 = 0;
			break;
			}
		if (s < n)
			{	// insert between iParent and n
			*r = s;
			s->iParent = r;
			s->iChild1 = n;
			s->iChild2 = 0;
			n->iParent = &s->iChild1;
			break;
			}
		slab* c1 = n->iChild1;
		slab* c2 = n->iChild2;
		if ((c1 - 1) > (c2 - 1))
			{
			r = &n->iChild1;
			n = c1;
			}
		else
			{
			r = &n->iChild2;
			n = c2;
			}
		}
}

void* RHybridHeap::AllocNewSlab(slabset& allocator)
//
// Acquire and initialise a new slab, returning a cell from the slab
// The strategy is:
// 1. Use the lowest address free slab, if available. This is done by using the lowest slab
//    in the page at the root of the iPartialPage heap (which is address ordered). If the
//    is now fully used, remove it from the iPartialPage heap.
// 2. Allocate a new page for iSlabs if no empty iSlabs are available
//
{
	page* p = page::PageFor(iPartialPage);
	if (!p)
		return AllocNewPage(allocator);
	
	unsigned h = p->iSlabs[0].iHeader;
	unsigned pagemap = SlabHeaderPagemap(h);
	HEAP_ASSERT(&p->iSlabs[HIBIT(pagemap)] == iPartialPage);
	
	unsigned slabix = LOWBIT(pagemap);
	p->iSlabs[0].iHeader = h &~ (0x100<<slabix);
	if (!(pagemap &~ (1<<slabix)))
		{
		TreeRemove(iPartialPage);	// last free slab in page
		}
	
	return InitNewSlab(allocator, &p->iSlabs[slabix]);
}

/**Defination of this functionis not there in proto code***/
#if 0
void RHybridHeap::partial_insert(slab* s)
{
	// slab has had first cell freed and needs to be linked back into iPartial tree
	slabset& ss = iSlabAlloc[iSizeMap[s->clz]];
	
	HEAP_ASSERT(s->used == slabfull);
	s->used = ss.fulluse - s->clz;		// full-1 loading
	TreeInsert(s,&ss.iPartial);
	CHECKTREE(&ss.iPartial);
}
/**Defination of this functionis not there in proto code***/
#endif

void* RHybridHeap::AllocNewPage(slabset& allocator)
//
// Acquire and initialise a new page, returning a cell from a new slab
// The iPartialPage tree is empty (otherwise we'd have used a slab from there)
// The iPartialPage link is put in the highest addressed slab in the page, and the
// lowest addressed slab is used to fulfill the allocation request
//
{
	page* p	 = iSparePage;
	if (p)
		iSparePage = 0;
	else
		{
		p = static_cast<page*>(Map(0, iPageSize));
		if (!p)
			return 0;
		}
	HEAP_ASSERT(p == Floor(p, iPageSize));
	// Store page allocated for slab into paged_bitmap (for RHybridHeap::Reset())
	if (!PagedSetSize(p, iPageSize))
		{
		Unmap(p, iPageSize);
		return 0;
		}
	p->iSlabs[0].iHeader = ((1<<3) + (1<<2) + (1<<1))<<8;		// set pagemap
	p->iSlabs[3].iParent = &iPartialPage;
	p->iSlabs[3].iChild1 = p->iSlabs[3].iChild2 = 0;
	iPartialPage = &p->iSlabs[3];
	return InitNewSlab(allocator,&p->iSlabs[0]);
}

void RHybridHeap::FreePage(page* p)
//
// Release an unused page to the OS
// A single page is cached for reuse to reduce thrashing
// the OS allocator.
//
{
	HEAP_ASSERT(Ceiling(p, iPageSize) == p);
	if (!iSparePage)
		{
		iSparePage = p;
		return;
		}
	
	// unmapped slab page must be cleared from paged_bitmap, too
	PagedZapSize(p, iPageSize);		// clear page map
	
	Unmap(p, iPageSize);
}

void RHybridHeap::FreeSlab(slab* s)
//
// Release an empty slab to the slab manager
// The strategy is:
// 1. The page containing the slab is checked to see the state of the other iSlabs in the page by
//    inspecting the pagemap field in the iHeader of the first slab in the page.
// 2. The pagemap is updated to indicate the new unused slab
// 3. If this is the only unused slab in the page then the slab iHeader is used to add the page to
//    the iPartialPage tree/heap
// 4. If all the iSlabs in the page are now unused the page is release back to the OS
// 5. If this slab has a higher address than the one currently used to track this page in
//    the iPartialPage heap, the linkage is moved to the new unused slab
//
{
	TreeRemove(s);
	CHECKTREE(s->iParent);
	HEAP_ASSERT(SlabHeaderUsedm4(s->iHeader) == SlabHeaderSize(s->iHeader)-4);

	page* p = page::PageFor(s);
	unsigned h = p->iSlabs[0].iHeader;
	int slabix = s - &p->iSlabs[0];
	unsigned pagemap = SlabHeaderPagemap(h);
	p->iSlabs[0].iHeader = h | (0x100<<slabix);
	if (pagemap == 0)
		{	// page was full before, use this slab as link in empty heap
		TreeInsert(s, &iPartialPage);
		}
	else
		{	// Find the current empty-link slab
		slab* sl = &p->iSlabs[HIBIT(pagemap)];
		pagemap ^= (1<<slabix);
		if (pagemap == 0xf)
			{	// page is now empty so recycle page to os
			TreeRemove(sl);
			FreePage(p);
			return;
			}
		// ensure the free list link is in highest address slab in page
		if (s > sl)
			{	// replace current link with new one. Address-order tree so position stays the same
			slab** r = sl->iParent;
			slab* c1 = sl->iChild1;
			slab* c2 = sl->iChild2;
			s->iParent = r;
			s->iChild1 = c1;
			s->iChild2 = c2;
			*r = s;
			if (c1)
				c1->iParent = &s->iChild1;
			if (c2)
				c2->iParent = &s->iChild2;
			}
		CHECK(if (s < sl) s=sl);
		}
	HEAP_ASSERT(SlabHeaderPagemap(p->iSlabs[0].iHeader) != 0);
	HEAP_ASSERT(HIBIT(SlabHeaderPagemap(p->iSlabs[0].iHeader)) == unsigned(s - &p->iSlabs[0]));
}


void RHybridHeap::SlabInit()
{
	iSlabThreshold=0;
	iPartialPage = 0;
	iFullSlab = 0;
	iSparePage = 0;
	memset(&iSizeMap[0],0xff,sizeof(iSizeMap));
	memset(&iSlabAlloc[0],0,sizeof(iSlabAlloc));
}

void RHybridHeap::SlabConfig(unsigned slabbitmap)
{
	HEAP_ASSERT((slabbitmap & ~EOkBits) == 0);
	HEAP_ASSERT(MAXSLABSIZE <= 60);
	
	unsigned int ix = 0xff;
	unsigned int bit = 1<<((MAXSLABSIZE>>2)-1);
	for (int sz = MAXSLABSIZE; sz >= 0; sz -= 4, bit >>= 1)
		{
		if (slabbitmap & bit)
			{
			if (ix == 0xff)
				iSlabThreshold=sz+1;
			ix = (sz>>2)-1;
			}
		iSizeMap[sz>>2] = (TUint8) ix;
		}
}


void* RHybridHeap::SlabAllocate(slabset& ss)
//
// Allocate a cell from the given slabset
// Strategy:
// 1. Take the partially full slab at the iTop of the heap (lowest address).
// 2. If there is no such slab, allocate from a new slab
// 3. If the slab has a non-empty freelist, pop the cell from the front of the list and update the slab
// 4. Otherwise, if the slab is not full, return the cell at the end of the currently used region of
//    the slab, updating the slab
// 5. Otherwise, release the slab from the iPartial tree/heap, marking it as 'floating' and go back to
//    step 1
//
{
	for (;;)
		{
		slab *s = ss.iPartial;
		if (!s)
			break;
		unsigned h = s->iHeader;
		unsigned free = h & 0xff;		// extract free cell positioning
		if (free)
			{
			HEAP_ASSERT(((free<<2)-sizeof(slabhdr))%SlabHeaderSize(h) == 0);
			void* p = Offset(s,free<<2);
			free = *(unsigned char*)p;	// get next pos in free list
			h += (h&0x3C000)<<6;		// update usedm4
			h &= ~0xff;
			h |= free;					// update freelist
			s->iHeader = h;
			HEAP_ASSERT(SlabHeaderFree(h) == 0 || ((SlabHeaderFree(h)<<2)-sizeof(slabhdr))%SlabHeaderSize(h) == 0);
			HEAP_ASSERT(SlabHeaderUsedm4(h) <= 0x3F8u);
			HEAP_ASSERT((SlabHeaderUsedm4(h)+4)%SlabHeaderSize(h) == 0);
			return p;
			}
		unsigned h2 = h + ((h&0x3C000)<<6);
//		if (h2 < 0xfc00000)
  		if (h2 < MAXUSEDM4BITS)		
			{
			HEAP_ASSERT((SlabHeaderUsedm4(h2)+4)%SlabHeaderSize(h2) == 0);
			s->iHeader = h2;
			return Offset(s,(h>>18) + sizeof(unsigned) + sizeof(slabhdr));
			}
		h |= FLOATING_BIT;				// mark the slab as full-floating
		s->iHeader = h;
		TreeRemove(s);
		slab* c = iFullSlab;			// add to full list
		iFullSlab = s;
		s->iParent = &iFullSlab;
		s->iChild1 = c;
		s->iChild2 = 0;
		if (c)
			c->iParent = &s->iChild1;
		
		CHECKTREE(&ss.iPartial);
		// go back and try the next slab...
		}
	// no iPartial iSlabs found, so allocate from a new slab
	return AllocNewSlab(ss);
}

void RHybridHeap::SlabFree(void* p)
//
// Free a cell from the slab allocator
// Strategy:
// 1. Find the containing slab (round down to nearest 1KB boundary)
// 2. Push the cell into the slab's freelist, and update the slab usage count
// 3. If this is the last allocated cell, free the slab to the main slab manager
// 4. If the slab was full-floating then insert the slab in it's respective iPartial tree
//
{
	HEAP_ASSERT(LowBits(p,3)==0);
	slab* s = slab::SlabFor(p);
	CHECKSLAB(s,ESlabAllocator,p);
	CHECKSLABBFR(s,p);	

	unsigned pos = LowBits(p, SLABSIZE);
	unsigned h = s->iHeader;
	HEAP_ASSERT(SlabHeaderUsedm4(h) != 0x3fC);		// slab is empty already
	HEAP_ASSERT((pos-sizeof(slabhdr))%SlabHeaderSize(h) == 0);
	*(unsigned char*)p = (unsigned char)h;
	h &= ~0xFF;
	h |= (pos>>2);
	unsigned size = h & 0x3C000;
	if (int(h) >= 0)
		{
		h -= size<<6;
		if (int(h)>=0)
			{
			s->iHeader = h;
			return;
			}
		FreeSlab(s);
		return;
		}
	h -= size<<6;
    h &= ~FLOATING_BIT;	
	s->iHeader = h;
	slab** full = s->iParent;		// remove from full list
	slab* c = s->iChild1;
	*full = c;
	if (c)
		c->iParent = full;
	
	slabset& ss = iSlabAlloc[iSizeMap[size>>14]];
	TreeInsert(s,&ss.iPartial);
	CHECKTREE(&ss.iPartial);
}

void* RHybridHeap::InitNewSlab(slabset& allocator, slab* s)
//
// initialise an empty slab for this allocator and return the fist cell
// pre-condition: the slabset has no iPartial iSlabs for allocation
//
{
	HEAP_ASSERT(allocator.iPartial==0);
	TInt size = 4 + ((&allocator-&iSlabAlloc[0])<<2);	// infer size from slab allocator address
	unsigned h = s->iHeader & 0xF00;	// preserve pagemap only
	h |= (size<<12);					// set size
	h |= (size-4)<<18;					// set usedminus4 to one object minus 4
	s->iHeader = h;
	allocator.iPartial = s;
	s->iParent = &allocator.iPartial;
	s->iChild1 = s->iChild2 = 0;
	return Offset(s,sizeof(slabhdr));
}

const unsigned char slab_bitcount[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

const unsigned char slab_ext_frag[16] =
{
	0,
	16 + (1008 % 4),
	16 + (1008 % 8),
	16 + (1008 % 12),
	16 + (1008 % 16),
	16 + (1008 % 20),
	16 + (1008 % 24),
	16 + (1008 % 28),
	16 + (1008 % 32),
	16 + (1008 % 36),
	16 + (1008 % 40),
	16 + (1008 % 44),
	16 + (1008 % 48),
	16 + (1008 % 52),
	16 + (1008 % 56),
	16 + (1008 % 60)
};

void RHybridHeap::TreeWalk(slab* const* root, void (*f)(slab*, struct HeapInfo*, SWalkInfo*), struct HeapInfo* i, SWalkInfo* wi)
{
	// iterative walk around the tree at root
	
	slab* s = *root;
	if (!s)
		return;
	
	for (;;)
		{
		slab* c;
		while ((c = s->iChild1) != 0)
			s = c;		// walk down left side to end
		for (;;)
			{
			f(s, i, wi);
			c = s->iChild2;
			if (c)
				{	// one step down right side, now try and walk down left
				s = c;
				break;
				}
			for (;;)
				{	// loop to walk up right side
				slab** pp = s->iParent;
				if (pp == root)
					return;
				s = slab::SlabFor(pp);
				if (pp == &s->iChild1)
					break;
				}
			}
		}
}

void RHybridHeap::SlabEmptyInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi)
{
	Walk(wi, s, SLABSIZE, EGoodFreeCell, EEmptySlab); // Introduce an empty slab to the walk function 
	int nslab = slab_bitcount[SlabHeaderPagemap(page::PageFor(s)->iSlabs[0].iHeader)];
	i->iFreeN += nslab;
	i->iFreeBytes += nslab << SLABSHIFT;
}

void RHybridHeap::SlabPartialInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi)
{
	Walk(wi, s, SLABSIZE, EGoodAllocatedCell, EPartialFullSlab); // Introduce a full slab to the walk function 
	unsigned h = s->iHeader;
	unsigned used = SlabHeaderUsedm4(h)+4;
	unsigned size = SlabHeaderSize(h);
	unsigned free = 1024 - slab_ext_frag[size>>2] - used;
	i->iFreeN += (free/size);
	i->iFreeBytes += free;
	i->iAllocN += (used/size);
	i->iAllocBytes += used;
}

void RHybridHeap::SlabFullInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi)
{
	Walk(wi, s, SLABSIZE, EGoodAllocatedCell, EFullSlab); // Introduce a full slab to the walk function 
	unsigned h = s->iHeader;
	unsigned used = SlabHeaderUsedm4(h)+4;
	unsigned size = SlabHeaderSize(h);
	HEAP_ASSERT(1024 - slab_ext_frag[size>>2] - used == 0);
	i->iAllocN += (used/size);
	i->iAllocBytes += used;
}

void RHybridHeap::SlabInfo(struct HeapInfo* i, SWalkInfo* wi) const
{
	if (iSparePage)
		{
		i->iFreeBytes += iPageSize;
		i->iFreeN = 4;
		Walk(wi, iSparePage, iPageSize, EGoodFreeCell, ESlabSpare); // Introduce Slab spare page to the walk function 
		}
	TreeWalk(&iFullSlab, &SlabFullInfo, i, wi);
	for (int ix = 0; ix < (MAXSLABSIZE>>2); ++ix)
		TreeWalk(&iSlabAlloc[ix].iPartial, &SlabPartialInfo, i, wi);
	TreeWalk(&iPartialPage, &SlabEmptyInfo, i, wi);
}


//
// Bitmap class implementation for large page allocator 
//
inline unsigned char* paged_bitmap::Addr() const {return iBase;}
inline unsigned paged_bitmap::Size() const {return iNbits;}
//

void paged_bitmap::Init(unsigned char* p, unsigned size, unsigned bit)
{
	iBase = p;
	iNbits=size;
	int bytes=Ceiling(size,8)>>3;
	memset(p,bit?0xff:0,bytes);
}

inline void paged_bitmap::Set(unsigned ix, unsigned bit)
{
	if (bit)
		iBase[ix>>3] |= (1<<(ix&7));
	else
		iBase[ix>>3] &= ~(1<<(ix&7));
}

inline unsigned paged_bitmap::operator[](unsigned ix) const
{
	return 1U&(iBase[ix>>3] >> (ix&7));
}

void paged_bitmap::Setn(unsigned ix, unsigned len, unsigned bit)
{
	int l=len;
	while (--l>=0)
		Set(ix++,bit);
}

void paged_bitmap::Set(unsigned ix, unsigned len, unsigned val)
{
	int l=len;
	while (--l>=0)
		{
		Set(ix++,val&1);
		val>>=1;
		}
}

unsigned paged_bitmap::Bits(unsigned ix, unsigned len) const
{
	int l=len;
	unsigned val=0;
	unsigned bit=0;
	while (--l>=0)
		val |= (*this)[ix++]<<bit++;
	return val;
}

bool paged_bitmap::Is(unsigned ix, unsigned len, unsigned bit) const
{
	unsigned i2 = ix+len;
	if (i2 > iNbits)
		return false;
	for (;;)
		{
		if ((*this)[ix] != bit)
			return false;
		if (++ix==i2)
			return true;
		}
}

int paged_bitmap::Find(unsigned start, unsigned bit) const
{
	if (start<iNbits) do
		{
		if ((*this)[start]==bit)
			return start;
		} while (++start<iNbits);
	return -1;
}


//
// Page allocator code
//
void RHybridHeap::PagedInit(TInt aPagePower)
{
	if (aPagePower > 0)
		{
		if (aPagePower < MINPAGEPOWER)
			aPagePower = MINPAGEPOWER;
		}
	else aPagePower = 31;

	iPageThreshold = aPagePower;
	/*-------------------------------------------------------------
	 * Initialize page bitmap
	 *-------------------------------------------------------------*/
	iPageMap.Init((unsigned char*)&iBitMapBuffer, MAXSMALLPAGEBITS, 0);
}

void* RHybridHeap::PagedAllocate(unsigned size)
{
	TInt nbytes = Ceiling(size, iPageSize);
	void* p = Map(0, nbytes);
	if (!p)
		return 0;
	if (!PagedSetSize(p, nbytes))
		{
		Unmap(p, nbytes);
		return 0;
		}
	return p;
}

void* RHybridHeap::PagedReallocate(void* p, unsigned size, TInt mode)
{
	
	HEAP_ASSERT(Ceiling(p, iPageSize) == p);
	unsigned nbytes = Ceiling(size, iPageSize);
	
	unsigned osize = PagedSize(p);
	if ( nbytes == 0 )  // Special case to handle shrinking below min page threshold 
		nbytes = Min((1 << MINPAGEPOWER), osize);
	
	if (osize == nbytes)
		return p;
	
	if (nbytes < osize)
		{	// shrink in place, unmap final pages and rewrite the pagemap
		Unmap(Offset(p, nbytes), osize-nbytes);
		// zap old code and then write new code (will not fail)
		PagedZapSize(p, osize);

		TBool check = PagedSetSize(p, nbytes);
        __ASSERT_ALWAYS(check, HEAP_PANIC(ETHeapBadCellAddress));
		
		return p;
		}
	
	// nbytes > osize
	// try and extend current region first
			
	void* newp = Map(Offset(p, osize), nbytes-osize);
	if (newp)
		{	// In place growth. Possibility that pagemap may have to grow AND then fails
		if (!PagedSetSize(p, nbytes))
			{	// must release extra mapping
			Unmap(Offset(p, osize), nbytes-osize);
			return 0;
			}
		// if successful, the new length code will have overwritten the old one (it is at least as long)
		return p;
		}
	
	// fallback to  allocate/copy/free
	if (mode & ENeverMove)
		return 0;		// not allowed to move cell
	
	newp = PagedAllocate(nbytes);
	if (!newp)
		return 0;
	memcpy(newp, p, osize);
	PagedFree(p);
	return newp;
}

void RHybridHeap::PagedFree(void* p)
{
	HEAP_ASSERT(Ceiling(p, iPageSize) == p);

	
	unsigned size = PagedSize(p);
	
	PagedZapSize(p, size);		// clear page map
	Unmap(p, size);
}

void RHybridHeap::PagedInfo(struct HeapInfo* i, SWalkInfo* wi) const
{
	for (int ix = 0;(ix = iPageMap.Find(ix,1)) >= 0;)
		{
		int npage = PagedDecode(ix);
		// Introduce paged buffer to the walk function 
		TAny* bfr = Bitmap2addr(ix);
		int len = npage << PAGESHIFT;
		if ( len > iPageSize )
			{ // If buffer is not larger than one page it must be a slab page mapped into bitmap
			i->iAllocBytes += len;
			++i->iAllocN;
			Walk(wi, bfr, len, EGoodAllocatedCell, EPageAllocator);
			}
		ix += (npage<<1);
		}
}

void RHybridHeap::ResetBitmap()
/*---------------------------------------------------------
 * Go through paged_bitmap and unmap all buffers to system
 * This method is called from RHybridHeap::Reset() to unmap all page
 * allocated - and slab pages which are stored in bitmap, too
 *---------------------------------------------------------*/ 
{
	unsigned iNbits = iPageMap.Size();
	if ( iNbits )
		{
		for (int ix = 0;(ix = iPageMap.Find(ix,1)) >= 0;)
			{
			int npage = PagedDecode(ix);
			void* p = Bitmap2addr(ix);
			unsigned size = PagedSize(p);
			PagedZapSize(p, size);		// clear page map
			Unmap(p, size);
			ix += (npage<<1);
			}
		if ( (TInt)iNbits > MAXSMALLPAGEBITS )
			{
			// unmap page reserved for enlarged bitmap
			Unmap(iPageMap.Addr(), (iNbits >> 3) );
			}
		}
}

TBool RHybridHeap::CheckBitmap(void* aBfr, TInt aSize, TUint32& aDummy, TInt& aNPages)
/*---------------------------------------------------------
 * If aBfr = NULL
 *   Go through paged_bitmap and unmap all buffers to system
 *   and assure that by reading the first word of each page of aBfr
 *   that aBfr is still accessible
 * else  
 *   Assure that specified buffer is mapped with correct length in
 *   page map
 *---------------------------------------------------------*/ 
{
	TBool ret;
	if ( aBfr )
		{
		__ASSERT_ALWAYS((Ceiling(aBfr, iPageSize) == aBfr), HEAP_PANIC(ETHeapBadCellAddress));		
        ret = ( aSize == (TInt)PagedSize(aBfr));
		}
	else
		{
		ret = ETrue;
		unsigned iNbits = iPageMap.Size();
		if ( iNbits )
			{
			TInt npage;
			aNPages = 0;
			for (int ix = 0;(ix = iPageMap.Find(ix,1)) >= 0;)
				{
				npage = PagedDecode(ix);
				aNPages += npage;
				void* p = Bitmap2addr(ix);
				__ASSERT_ALWAYS((Ceiling(p, iPageSize) == p), HEAP_PANIC(ETHeapBadCellAddress));						
				unsigned s = PagedSize(p);
				__ASSERT_ALWAYS((Ceiling(s, iPageSize) == s), HEAP_PANIC(ETHeapBadCellAddress));	
				while ( s )
					{
					aDummy += *(TUint32*)((TUint8*)p + (s-iPageSize));
					s -= iPageSize;
					}
				ix += (npage<<1);
				}
			if ( (TInt)iNbits > MAXSMALLPAGEBITS )
				{
				// add enlarged bitmap page(s) to total page count
                npage = (iNbits >> 3); 
				__ASSERT_ALWAYS((Ceiling(npage, iPageSize) == npage), HEAP_PANIC(ETHeapBadCellAddress));
				aNPages += (npage / iPageSize);
				}
			}
		}
	
	return ret;
}


// The paged allocations are tracked in a bitmap which has 2 bits per page
// this allows us to store allocations as small as 4KB
// The presence and size of an allocation is encoded as follows:
// let N = number of pages in the allocation, then
// 10            : N = 1			// 4KB
// 110n			 : N = 2 + n		// 8-12KB
// 1110nnnn      : N = nnnn			// 16-60KB
// 1111n[18]	 : N = n[18]		// 64KB-1GB

const struct etab { unsigned char offset, len, codelen, code;} encode_table[] =
{
	{1,2,2,0x1},
	{2,4,3,0x3},
	{0,8,4,0x7},
	{0,22,4,0xf}
};

// Return code length for specified allocation Size(assumed to be aligned to pages)
inline unsigned paged_codelen(unsigned size, unsigned pagesz)
{
	HEAP_ASSERT(size == Ceiling(size, pagesz));
	
	if (size == pagesz)
		return 2;
	else if (size < 4*pagesz)
		return 4;
	else if (size < 16*pagesz)
		return 8;
	else
		return 22;
}

inline const etab& paged_coding(unsigned npage)
{
	if (npage < 4)
		return encode_table[npage>>1];
	else if (npage < 16)
		return encode_table[2];
	else
		return encode_table[3];
}

bool RHybridHeap::PagedEncode(unsigned pos, unsigned npage)
{
	const etab& e = paged_coding(npage);
	if (pos + e.len > iPageMap.Size())
		{
		// need to grow the page bitmap to fit the cell length into the map
		// if we outgrow original bitmap buffer in RHybridHeap metadata, then just get enough pages to cover the full space:
		// * initial 68 byte bitmap mapped (68*8*4kB):2 = 1,1MB
		// * 4KB can Map(4096*8*4kB):2 = 64MB
		unsigned maxsize = Ceiling(iMaxLength, iPageSize);
		unsigned mapbits = maxsize >> (PAGESHIFT-1);
		maxsize = Ceiling(mapbits>>3, iPageSize);
		void* newb = Map(0, maxsize);
		if (!newb)
			return false;
		
		unsigned char* oldb = iPageMap.Addr();
		iPageMap.Init((unsigned char*)newb, (maxsize<<3), 0);
		memcpy(newb, oldb, Ceiling(MAXSMALLPAGEBITS,8)>>3);
		}
	// encode the allocation block size into the bitmap, starting at the bit for the start page
	unsigned bits = e.code;
	bits |= (npage - e.offset) << e.codelen;
	iPageMap.Set(pos, e.len, bits);
	return true;
}

unsigned RHybridHeap::PagedDecode(unsigned pos) const
{
	__ASSERT_ALWAYS(pos + 2 <= iPageMap.Size(), HEAP_PANIC(ETHeapBadCellAddress));
	
	unsigned bits = iPageMap.Bits(pos,2);
	__ASSERT_ALWAYS(bits & 1, HEAP_PANIC(ETHeapBadCellAddress));
	bits >>= 1;
	if (bits == 0)
		return 1;
	__ASSERT_ALWAYS(pos + 4 <= iPageMap.Size(), HEAP_PANIC(ETHeapBadCellAddress));
	bits = iPageMap.Bits(pos+2,2);
	if ((bits & 1) == 0)
		return 2 + (bits>>1);
	else if ((bits>>1) == 0)
		{
		__ASSERT_ALWAYS(pos + 8 <= iPageMap.Size(), HEAP_PANIC(ETHeapBadCellAddress));
		return iPageMap.Bits(pos+4, 4);
		}
	else
		{
		__ASSERT_ALWAYS(pos + 22 <= iPageMap.Size(), HEAP_PANIC(ETHeapBadCellAddress));
		return iPageMap.Bits(pos+4, 18);
		}
}

inline void RHybridHeap::PagedZapSize(void* p, unsigned size)
{iPageMap.Setn(PtrDiff(p, iMemBase) >> (PAGESHIFT-1), paged_codelen(size, iPageSize) ,0);}

inline unsigned RHybridHeap::PagedSize(void* p) const
   { return PagedDecode(PtrDiff(p, iMemBase) >> (PAGESHIFT-1)) << PAGESHIFT; }

inline bool RHybridHeap::PagedSetSize(void* p, unsigned size)
{ return PagedEncode(PtrDiff(p, iMemBase) >> (PAGESHIFT-1), size >> PAGESHIFT); }

inline void* RHybridHeap::Bitmap2addr(unsigned pos) const
   { return iMemBase + (1 << (PAGESHIFT-1))*pos; }


#ifndef QT_SYMBIAN4_ALLOCATOR_UNWANTED_CODE
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**
Constructor where minimum and maximum length of the heap can be defined.
It defaults the chunk heap to be created to have use a new local chunk, 
to have a grow by value of KMinHeapGrowBy, to be unaligned, not to be 
single threaded and not to have any mode flags set.

@param aMinLength    The minimum length of the heap to be created.
@param aMaxLength    The maximum length to which the heap to be created can grow.
                     If the supplied value is less than a page size, then it
                     is discarded and the page size is used instead.
*/
EXPORT_C TChunkHeapCreateInfo::TChunkHeapCreateInfo(TInt aMinLength, TInt aMaxLength) :
   iVersionNumber(EVersion0), iMinLength(aMinLength), iMaxLength(aMaxLength),
iAlign(0), iGrowBy(1), iSingleThread(EFalse), 
iOffset(0), iPaging(EUnspecified), iMode(0), iName(NULL)
{
}


/**
Sets the chunk heap to create a new chunk with the specified name.

This overriddes any previous call to TChunkHeapCreateInfo::SetNewChunkHeap() or
TChunkHeapCreateInfo::SetExistingChunkHeap() for this TChunkHeapCreateInfo object.

@param aName	The name to be given to the chunk heap to be created
If NULL, the function constructs a local chunk to host the heap.
If not NULL, a pointer to a descriptor containing the name to be 
assigned to the global chunk hosting the heap.
*/
EXPORT_C void TChunkHeapCreateInfo::SetCreateChunk(const TDesC* aName)
{
	iName = (TDesC*)aName;
	iChunk.SetHandle(KNullHandle);
}


/**
Sets the chunk heap to be created to use the chunk specified.

This overriddes any previous call to TChunkHeapCreateInfo::SetNewChunkHeap() or
TChunkHeapCreateInfo::SetExistingChunkHeap() for this TChunkHeapCreateInfo object.

@param aChunk	A handle to the chunk to use for the heap.
*/
EXPORT_C void TChunkHeapCreateInfo::SetUseChunk(const RChunk aChunk)
{
	iName = NULL;
	iChunk = aChunk;
}

EXPORT_C RHeap* UserHeap::FixedHeap(TAny* aBase, TInt aMaxLength, TInt aAlign, TBool aSingleThread)
/**
Creates a fixed length heap at a specified location.

On successful return from this function, the heap is ready to use.  This assumes that
the memory pointed to by aBase is mapped and able to be used.  You must ensure that you
pass in a large enough value for aMaxLength.  Passing in a value that is too small to
hold the metadata for the heap (~1 KB) will result in the size being rounded up and the
heap thereby running over the end of the memory assigned to it.  But then if you were to
pass in such as small value then you would not be able to do any allocations from the
heap anyway.  Moral of the story: Use a sensible value for aMaxLength!

@param aBase         A pointer to the location where the heap is to be constructed.
@param aMaxLength    The maximum length in bytes to which the heap can grow.  If the
                     supplied value is too small to hold the heap's metadata, it
                     will be increased.
@param aAlign        From Symbian^4 onwards, this value is ignored but EABI 8
                     byte alignment is guaranteed for all allocations 8 bytes or
                     more in size.  4 byte allocations will be aligned to a 4
                     byte boundary.  Best to pass in zero.
@param aSingleThread EFalse if the heap is to be accessed from multiple threads.
                     This will cause internal locks to be created, guaranteeing
                     thread safety.

@return A pointer to the new heap, or NULL if the heap could not be created.

@panic USER 56 if aMaxLength is negative.
*/
{
	__ASSERT_ALWAYS( aMaxLength>=0, ::Panic(ETHeapMaxLengthNegative));
	if ( aMaxLength < (TInt)sizeof(RHybridHeap) )
		aMaxLength = sizeof(RHybridHeap);
	
	RHybridHeap* h = new(aBase) RHybridHeap(aMaxLength, aAlign, aSingleThread);
	
	if (!aSingleThread)
		{
		TInt r = h->iLock.CreateLocal();
		if (r!=KErrNone)
			return NULL; // No need to delete the RHybridHeap instance as the new above is only a placement new
		h->iHandles = (TInt*)&h->iLock;
		h->iHandleCount = 1;
		}
	return h;
}

/**
Creates a chunk heap of the type specified by the parameter aCreateInfo.

@param aCreateInfo	A reference to a TChunkHeapCreateInfo object specifying the
type of chunk heap to create.

@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 41 if the heap's specified minimum length is greater than the specified maximum length.
@panic USER 55 if the heap's specified minimum length is negative.
@panic USER 172 if the heap's specified alignment is not a power of 2 or is less than the size of a TAny*.
*/
EXPORT_C RHeap* UserHeap::ChunkHeap(const TChunkHeapCreateInfo& aCreateInfo)
{
	// aCreateInfo must have been configured to use a new chunk or an exiting chunk.
	__ASSERT_ALWAYS(!(aCreateInfo.iMode & (TUint32)~EChunkHeapMask), ::Panic(EHeapCreateInvalidMode));
	RHeap* h = NULL;
	
	if (aCreateInfo.iChunk.Handle() == KNullHandle)
		{
		// A new chunk is to be created for this heap.
		
		__ASSERT_ALWAYS(aCreateInfo.iMinLength >= 0, ::Panic(ETHeapMinLengthNegative));
		__ASSERT_ALWAYS(aCreateInfo.iMaxLength >= aCreateInfo.iMinLength, ::Panic(ETHeapCreateMaxLessThanMin));

		TInt maxLength = aCreateInfo.iMaxLength;
		TInt page_size;
		GET_PAGE_SIZE(page_size);

		if (maxLength < page_size)
			maxLength = page_size;
		
		TChunkCreateInfo chunkInfo;
#if USE_HYBRID_HEAP
		if ( aCreateInfo.iOffset )
			chunkInfo.SetNormal(0, maxLength);  // Create DL only heap
		else
			{
			maxLength = 2*maxLength;
			chunkInfo.SetDisconnected(0, 0, maxLength); // Create hybrid heap
			}
#else
		chunkInfo.SetNormal(0, maxLength);  // Create DL only heap		
#endif			
		chunkInfo.SetOwner((aCreateInfo.iSingleThread)? EOwnerThread : EOwnerProcess);
		if (aCreateInfo.iName)
			chunkInfo.SetGlobal(*aCreateInfo.iName);
		// Set the paging attributes of the chunk.
		if (aCreateInfo.iPaging == TChunkHeapCreateInfo::EPaged)
			chunkInfo.SetPaging(TChunkCreateInfo::EPaged);
		if (aCreateInfo.iPaging == TChunkHeapCreateInfo::EUnpaged)
			chunkInfo.SetPaging(TChunkCreateInfo::EUnpaged);
		// Create the chunk.
		RChunk chunk;
		if (chunk.Create(chunkInfo) != KErrNone)
			return NULL;
		// Create the heap using the new chunk.
		TUint mode = aCreateInfo.iMode | EChunkHeapDuplicate;	// Must duplicate the handle.
		h = OffsetChunkHeap(chunk, aCreateInfo.iMinLength, aCreateInfo.iOffset,
							aCreateInfo.iGrowBy, maxLength, aCreateInfo.iAlign,
							aCreateInfo.iSingleThread, mode);
		chunk.Close();
		}
	else
		{
		h = OffsetChunkHeap(aCreateInfo.iChunk, aCreateInfo.iMinLength, aCreateInfo.iOffset,
							aCreateInfo.iGrowBy, aCreateInfo.iMaxLength, aCreateInfo.iAlign,
							aCreateInfo.iSingleThread, aCreateInfo.iMode);
		}
	return h;
}



EXPORT_C RHeap* UserHeap::ChunkHeap(const TDesC* aName, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign, TBool aSingleThread)
/**
Creates a heap in a local or global chunk.

The chunk hosting the heap can be local or global.

A local chunk is one which is private to the process creating it and is not
intended for access by other user processes.  A global chunk is one which is
visible to all processes.

The hosting chunk is local, if the pointer aName is NULL, otherwise the
hosting chunk is global and the descriptor *aName is assumed to contain
the name to be assigned to it.

Ownership of the host chunk is vested in the current process.

A minimum and a maximum size for the heap can be specified. On successful
return from this function, the size of the heap is at least aMinLength.
If subsequent requests for allocation of memory from the heap cannot be
satisfied by compressing the heap, the size of the heap is extended in
increments of aGrowBy until the request can be satisfied.  Attempts to extend
the heap causes the size of the host chunk to be adjusted.

Note that the size of the heap cannot be adjusted by more than aMaxLength.

@param aName         If NULL, the function constructs a local chunk to host
                     the heap.  If not NULL, a pointer to a descriptor containing
                     the name to be assigned to the global chunk hosting the heap.
@param aMinLength    The minimum length of the heap in bytes.  This will be
                     rounded up to the nearest page size by the allocator.
@param aMaxLength    The maximum length in bytes to which the heap can grow.  This
                     will be rounded up to the nearest page size by the allocator.
@param aGrowBy       The number of bytes by which the heap will grow when more
                     memory is required.  This will be rounded up to the nearest
                     page size by the allocator.  If a value is not explicitly
                     specified, the page size is taken by default.
@param aAlign        From Symbian^4 onwards, this value is ignored but EABI 8
                     byte alignment is guaranteed for all allocations 8 bytes or
                     more in size.  4 byte allocations will be aligned to a 4
                     byte boundary.  Best to pass in zero.
@param aSingleThread EFalse if the heap is to be accessed from multiple threads.
                     This will cause internal locks to be created, guaranteeing
                     thread safety.

@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 41 if aMaxLength is < aMinLength.
@panic USER 55 if aMinLength is negative.
@panic USER 56 if aMaxLength is negative.
*/
	{
	TInt page_size;
	GET_PAGE_SIZE(page_size);
	TInt minLength = _ALIGN_UP(aMinLength, page_size);
	TInt maxLength = Max(aMaxLength, minLength);

	TChunkHeapCreateInfo createInfo(minLength, maxLength);
	createInfo.SetCreateChunk(aName);
	createInfo.SetGrowBy(aGrowBy);
	createInfo.SetAlignment(aAlign);
	createInfo.SetSingleThread(aSingleThread);

	return ChunkHeap(createInfo);
	}
#endif // QT_SYMBIAN4_ALLOCATOR_UNWANTED_CODE

EXPORT_C RHeap* UserHeap::ChunkHeap(RChunk aChunk, TInt aMinLength, TInt aGrowBy, TInt aMaxLength, TInt aAlign, TBool aSingleThread, TUint32 aMode)
/**
Creates a heap in an existing chunk.

This function is intended to be used to create a heap in a user writable code
chunk as created by a call to RChunk::CreateLocalCode().  This type of heap can
be used to hold code fragments from a JIT compiler.

@param aChunk        The chunk that will host the heap.
@param aMinLength    The minimum length of the heap in bytes.  This will be
                     rounded up to the nearest page size by the allocator.
@param aGrowBy       The number of bytes by which the heap will grow when more
                     memory is required.  This will be rounded up to the nearest
                     page size by the allocator.  If a value is not explicitly
                     specified, the page size is taken by default.
@param aMaxLength    The maximum length in bytes to which the heap can grow.  This
                     will be rounded up to the nearest page size by the allocator.
                     If 0 is passed in, the maximum lengt of the chunk is used.
@param aAlign        From Symbian^4 onwards, this value is ignored but EABI 8
                     byte alignment is guaranteed for all allocations 8 bytes or
                     more in size.  4 byte allocations will be aligned to a 4
                     byte boundary.  Best to pass in zero.
@param aSingleThread EFalse if the heap is to be accessed from multiple threads.
                     This will cause internal locks to be created, guaranteeing
                     thread safety.
@param aMode         Flags controlling the heap creation.  See RAllocator::TFlags.

@return A pointer to the new heap or NULL if the heap could not be created.

@see UserHeap::OffsetChunkHeap()
*/
	{
	return OffsetChunkHeap(aChunk, aMinLength, 0, aGrowBy, aMaxLength, aAlign, aSingleThread, aMode);
	}

EXPORT_C RHeap* UserHeap::OffsetChunkHeap(RChunk aChunk, TInt aMinLength, TInt aOffset, TInt aGrowBy, TInt aMaxLength, TInt aAlign, TBool aSingleThread, TUint32 aMode)
/**
Creates a heap in an existing chunk, offset from the beginning of the chunk.

This function is intended to be used to create a heap using a chunk which has
some of its memory already used, at the start of that that chunk.  The maximum
length to which the heap can grow is the maximum size of the chunk, minus the
data at the start of the chunk.

The offset at which to create the heap is passed in as the aOffset parameter.
Legacy heap implementations always respected the aOffset value, however more
modern heap implementations are more sophisticated and cannot necessarily respect
this value.  Therefore, if possible, you should always use an aOffset of 0 unless
you have a very explicit requirement for using a non zero value.  Using a non zero
value will result in a less efficient heap algorithm being used in order to respect
the offset.

Another issue to consider when using this function is the type of the chunk passed
in.  In order for the most efficient heap algorithms to be used, the chunk passed
in should always be a disconnected chunk.  Passing in a non disconnected chunk will
again result in a less efficient heap algorithm being used.

Finally, another requirement for the most efficient heap algorithms to be used is
for the heap to be able to expand.  Therefore, unless you have a specific reason to
do so, always specify aMaxLength > aMinLength.

So, if possible, use aOffset == zero, aMaxLength > aMinLength and a disconnected
chunk for best results!

@param aChunk        The chunk that will host the heap.
@param aMinLength    The minimum length of the heap in bytes.  This will be
                     rounded up to the nearest page size by the allocator.
@param aOffset       The offset in bytes from the start of the chunk at which to
                     create the heap.  If used (and it shouldn't really be!)
                     then it will be rounded up to a multiple of 8, to respect
                     EABI 8 byte alignment requirements.
@param aGrowBy       The number of bytes by which the heap will grow when more
                     memory is required.  This will be rounded up to the nearest
                     page size by the allocator.  If a value is not explicitly
                     specified, the page size is taken by default.
@param aMaxLength    The maximum length in bytes to which the heap can grow.  This
                     will be rounded up to the nearest page size by the allocator.
                     If 0 is passed in, the maximum length of the chunk is used.
@param aAlign        From Symbian^4 onwards, this value is ignored but EABI 8
                     byte alignment is guaranteed for all allocations 8 bytes or
                     more in size.  4 byte allocations will be aligned to a 4
                     byte boundary.  Best to pass in zero.
@param aSingleThread EFalse if the heap is to be accessed from multiple threads.
                     This will cause internal locks to be created, guaranteeing
                     thread safety.
@param aMode         Flags controlling the heap creation.  See RAllocator::TFlags.

@return A pointer to the new heap or NULL if the heap could not be created.

@panic USER 41 if aMaxLength is < aMinLength.
@panic USER 55 if aMinLength is negative.
@panic USER 56 if aMaxLength is negative.
@panic USER 168 if aOffset is negative.
*/
	{
	TBool dlOnly = EFalse;
	TInt pageSize;
	GET_PAGE_SIZE(pageSize);
	TInt align = RHybridHeap::ECellAlignment; // Always use EABI 8 byte alignment

	__ASSERT_ALWAYS(aMinLength>=0, ::Panic(ETHeapMinLengthNegative));
	__ASSERT_ALWAYS(aMaxLength>=0, ::Panic(ETHeapMaxLengthNegative));

	if ( aMaxLength > 0 ) 
		__ASSERT_ALWAYS(aMaxLength>=aMinLength, ::Panic(ETHeapCreateMaxLessThanMin));

	// Stick to EABI alignment for the start offset, if any
	aOffset = _ALIGN_UP(aOffset, align);

	// Using an aOffset > 0 means that we can't use the hybrid allocator and have to revert to Doug Lea only
	if (aOffset > 0)
		dlOnly = ETrue;

	// Ensure that the minimum length is enough to hold the RHybridHeap object itself
	TInt minCell = _ALIGN_UP(Max((TInt)RHybridHeap::EAllocCellSize, (TInt)RHybridHeap::EFreeCellSize), align);
	TInt hybridHeapSize = (sizeof(RHybridHeap) + minCell);
	if (aMinLength < hybridHeapSize)
		aMinLength = hybridHeapSize;

	// Round the minimum length up to a multiple of the page size, taking into account that the
	// offset takes up a part of the chunk's memory
	aMinLength = _ALIGN_UP((aMinLength + aOffset), pageSize);

	// If aMaxLength is 0 then use the entire chunk
	TInt chunkSize = aChunk.MaxSize();
	if (aMaxLength == 0)
		{
		aMaxLength = chunkSize;
		}
	// Otherwise round the maximum length up to a multiple of the page size, taking into account that
	// the offset takes up a part of the chunk's memory.  We also clip the maximum length to the chunk
	// size, so the user may get a little less than requested if the chunk size is not large enough
	else
		{
		aMaxLength = _ALIGN_UP((aMaxLength + aOffset), pageSize);
		if (aMaxLength > chunkSize)
			aMaxLength = chunkSize;
		}
	
	// If the rounded up values don't make sense then a crazy aMinLength or aOffset must have been passed
	// in, so fail the heap creation
	if (aMinLength > aMaxLength)
		return NULL;

	// Adding the offset into the minimum and maximum length was only necessary for ensuring a good fit of
	// the heap into the chunk.  Re-adjust them now back to non offset relative sizes
	aMinLength -= aOffset;
	aMaxLength -= aOffset;

	// If we are still creating the hybrid allocator (call parameter
	// aOffset is 0 and aMaxLength > aMinLength), we must reduce heap
	// aMaxLength size to the value aMaxLength/2 and set the aOffset to point in the middle of chunk.
	TInt offset = aOffset;
	TInt maxLength = aMaxLength;
	if (!dlOnly && (aMaxLength > aMinLength))
		maxLength = offset = _ALIGN_UP(aMaxLength >> 1, pageSize);

	// Try to use commit to map aMinLength physical memory for the heap, taking into account the offset.  If
	// the operation fails, suppose that the chunk is not a disconnected heap and try to map physical memory
	// with adjust.  In this case, we also can't use the hybrid allocator and have to revert to Doug Lea only
	TBool useAdjust = EFalse;
	TInt r = aChunk.Commit(offset, aMinLength);
	if (r == KErrGeneral)
		{
		dlOnly = useAdjust = ETrue;
		r = aChunk.Adjust(aMinLength);
		if (r != KErrNone)
			return NULL;
		}
	else if (r == KErrNone)
		{
		// We have a disconnected chunk reset aOffset and aMaxlength
		aOffset = offset;
		aMaxLength = maxLength;
		}

	else
		return NULL;

	// Parameters have been mostly verified and we know whether to use the hybrid allocator or Doug Lea only.  The
	// constructor for the hybrid heap will automatically drop back to Doug Lea if it determines that aMinLength
	// == aMaxLength, so no need to worry about that requirement here.  The user specified alignment is not used but
	// is passed in so that it can be sanity checked in case the user is doing something totally crazy with it
	RHybridHeap* h = new (aChunk.Base() + aOffset) RHybridHeap(aChunk.Handle(), aOffset, aMinLength, aMaxLength,
		aGrowBy, aAlign, aSingleThread, dlOnly, useAdjust);

	if (h->ConstructLock(aMode) != KErrNone)
		return NULL;

	// Return the heap address
	return h;
	}

#define UserTestDebugMaskBit(bit) (TBool)(UserSvr::DebugMask(bit>>5) & (1<<(bit&31)))

_LIT(KLitDollarHeap,"$HEAP");
EXPORT_C TInt UserHeap::CreateThreadHeap(SStdEpocThreadCreateInfo& aInfo, RHeap*& aHeap, TInt aAlign, TBool aSingleThread)
/**
@internalComponent
*/
//
// Create a user-side heap
//
{
	TInt page_size;
	GET_PAGE_SIZE(page_size);
	TInt minLength = _ALIGN_UP(aInfo.iHeapInitialSize, page_size);
	TInt maxLength = Max(aInfo.iHeapMaxSize, minLength);
#ifdef ENABLE_BTRACE
	if (UserTestDebugMaskBit(96)) // 96 == KUSERHEAPTRACE in nk_trace.h
		aInfo.iFlags |= ETraceHeapAllocs;
#endif // ENABLE_BTRACE
	// Create the thread's heap chunk.
	RChunk c;
#ifndef NO_NAMED_LOCAL_CHUNKS
	TChunkCreateInfo createInfo;

	createInfo.SetThreadHeap(0, maxLength, KLitDollarHeap());	// Initialise with no memory committed.	
#if USE_HYBRID_HEAP
	//
	// Create disconnected chunk for hybrid heap with double max length value
	//
	maxLength = 2*maxLength;
	createInfo.SetDisconnected(0, 0, maxLength);
#endif	
#ifdef SYMBIAN_WRITABLE_DATA_PAGING
	// Set the paging policy of the heap chunk based on the thread's paging policy.
	TUint pagingflags = aInfo.iFlags & EThreadCreateFlagPagingMask;
	switch (pagingflags)
		{
		case EThreadCreateFlagPaged:
			createInfo.SetPaging(TChunkCreateInfo::EPaged);
			break;
		case EThreadCreateFlagUnpaged:
			createInfo.SetPaging(TChunkCreateInfo::EUnpaged);
			break;
		case EThreadCreateFlagPagingUnspec:
			// Leave the chunk paging policy unspecified so the process's 
			// paging policy is used.
			break;
		}
#endif // SYMBIAN_WRITABLE_DATA_PAGING
	
	TInt r = c.Create(createInfo);
#else
	TInt r = c.CreateDisconnectedLocal(0, 0, maxLength * 2);
#endif
	if (r!=KErrNone)
		return r;
	
	aHeap = ChunkHeap(c, minLength, page_size, maxLength, aAlign, aSingleThread, EChunkHeapSwitchTo|EChunkHeapDuplicate);
	c.Close();
	
	if ( !aHeap )
		return KErrNoMemory;
	
#ifdef ENABLE_BTRACE
	if (aInfo.iFlags & ETraceHeapAllocs)
		{
		aHeap->iFlags |= RHeap::ETraceAllocs;
    	BTraceContext8(BTrace::EHeap, BTrace::EHeapCreate,(TUint32)aHeap, RHybridHeap::EAllocCellSize);
		TInt chunkId = ((RHandleBase&)((RHybridHeap*)aHeap)->iChunkHandle).BTraceId();
		BTraceContext8(BTrace::EHeap, BTrace::EHeapChunkCreate, (TUint32)aHeap, chunkId);
		}
	if (aInfo.iFlags & EMonitorHeapMemory)
		aHeap->iFlags |= RHeap::EMonitorMemory;
#endif // ENABLE_BTRACE
	
	return KErrNone;
}

#endif  // __KERNEL_MODE__

#endif /* QT_USE_NEW_SYMBIAN_ALLOCATOR */
