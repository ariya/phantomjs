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

#define GM  (&iGlobalMallocState)
#define __HEAP_CORRUPTED_TRACE(t,p,l) BTraceContext12(BTrace::EHeap, BTrace::EHeapCorruption, (TUint32)t, (TUint32)p, (TUint32)l);
#define __HEAP_CORRUPTED_TEST(c,x, p,l) if (!c) { if (iFlags & (EMonitorMemory+ETraceAllocs) )  __HEAP_CORRUPTED_TRACE(this,p,l)  HEAP_PANIC(x); }
#define __HEAP_CORRUPTED_TEST_STATIC(c,t,x,p,l) if (!c) { if (t && (t->iFlags & (EMonitorMemory+ETraceAllocs) )) __HEAP_CORRUPTED_TRACE(t,p,l) HEAP_PANIC(x); }

TInt RHybridHeap::DebugFunction(TInt aFunc, TAny* a1, TAny* a2)
{
    TInt r = KErrNone;
    switch(aFunc)
        {
        
        case RAllocator::ECount:
            struct HeapInfo info;
            Lock();
            GetInfo(&info, NULL);
            *(unsigned*)a1 = info.iFreeN;
            r = info.iAllocN;
            Unlock();
            break;
            
        case RAllocator::EMarkStart:
            __DEBUG_ONLY(DoMarkStart());
            break;
            
        case RAllocator::EMarkEnd:
            __DEBUG_ONLY( r = DoMarkEnd((TInt)a1) );
            break;
            
        case RAllocator::ECheck:
            r = DoCheckHeap((SCheckInfo*)a1);
            break;
            
        case RAllocator::ESetFail:
            __DEBUG_ONLY(DoSetAllocFail((TAllocFail)(TInt)a1, (TInt)a2));
            break;

        case RHybridHeap::EGetFail:
            __DEBUG_ONLY(r = iFailType);
            break;

        case RHybridHeap::ESetBurstFail:
#if _DEBUG
            {
            SRAllocatorBurstFail* fail = (SRAllocatorBurstFail*) a2;
            DoSetAllocFail((TAllocFail)(TInt)a1, fail->iRate, fail->iBurst);
            }
#endif
            break;
            
        case RHybridHeap::ECheckFailure:
            // iRand will be incremented for each EFailNext, EBurstFailNext,
            // EDeterministic and EBurstDeterministic failure.
            r = iRand;
            break;
            
        case RAllocator::ECopyDebugInfo:
            {
            TInt nestingLevel = ((SDebugCell*)a1)[-1].nestingLevel;
            ((SDebugCell*)a2)[-1].nestingLevel = nestingLevel;
            break;
            }

		case RHybridHeap::EGetSize:
			{
			r = iChunkSize - sizeof(RHybridHeap);
			break;
			}

		case RHybridHeap::EGetMaxLength:
			{
			r = iMaxLength;
			break;
			}

		case RHybridHeap::EGetBase:
			{
			*(TAny**)a1 = iBase;
			break;
			}

		case RHybridHeap::EAlignInteger:
			{
			r = _ALIGN_UP((TInt)a1, iAlign);
			break;
			}

		case RHybridHeap::EAlignAddr:
			{
            *(TAny**)a2 = (TAny*)_ALIGN_UP((TLinAddr)a1, iAlign);
			break;
			}

        case RHybridHeap::EWalk:
            struct HeapInfo hinfo;
            SWalkInfo winfo;
            Lock();
            winfo.iFunction = (TWalkFunc)a1;
            winfo.iParam    = a2;
			winfo.iHeap     = (RHybridHeap*)this; 	
            GetInfo(&hinfo, &winfo);
            Unlock();
            break;

#ifndef __KERNEL_MODE__
			
        case RHybridHeap::EHybridHeap:
            {
			if ( !a1 )
				return KErrGeneral;
			STestCommand* cmd = (STestCommand*)a1;
			switch ( cmd->iCommand )
				{
				case EGetConfig:
					cmd->iConfig.iSlabBits = iSlabConfigBits;
					cmd->iConfig.iDelayedSlabThreshold = iPageThreshold;
					cmd->iConfig.iPagePower = iPageThreshold;
					break;
					
				case ESetConfig:
					//
					// New configuration data for slab and page allocator.
					// Reset heap to get data into use
					//
#if USE_HYBRID_HEAP
					iSlabConfigBits  = cmd->iConfig.iSlabBits & 0x3fff;
					iSlabInitThreshold = cmd->iConfig.iDelayedSlabThreshold;
					iPageThreshold = (cmd->iConfig.iPagePower & 0x1f);
					Reset();
#endif
					break;
					
				case EHeapMetaData:
					cmd->iData = this;
					break;
					
				case ETestData:
					iTestData = cmd->iData;
					break;

				default:
					return KErrNotSupported;
					
				}

            break;
			}
#endif  // __KERNEL_MODE            
            
        default:
            return KErrNotSupported;
            
        }
    return r;
}

void RHybridHeap::Walk(SWalkInfo* aInfo, TAny* aBfr, TInt aLth, TCellType aBfrType, TAllocatorType aAllocatorType)
{
    //
    // This function is always called from RHybridHeap::GetInfo.
    // Actual walk function is called if SWalkInfo pointer is defined
    // 
    //
    if ( aInfo )
        {
#ifdef __KERNEL_MODE__
		(void)aAllocatorType;
#if defined(_DEBUG)		
		if ( aBfrType == EGoodAllocatedCell )
			aInfo->iFunction(aInfo->iParam, aBfrType, ((TUint8*)aBfr+EDebugHdrSize), (aLth-EDebugHdrSize) );
		else
			aInfo->iFunction(aInfo->iParam, aBfrType,  aBfr, aLth );
#else
		aInfo->iFunction(aInfo->iParam, aBfrType, aBfr, aLth );
#endif
		
#else  // __KERNEL_MODE__
		
        if ( aAllocatorType & (EFullSlab + EPartialFullSlab + EEmptySlab + ESlabSpare) )
			{
			if ( aInfo->iHeap )
				{
				TUint32 dummy;
				TInt    npages;
				aInfo->iHeap->DoCheckSlab((slab*)aBfr, aAllocatorType);
				__HEAP_CORRUPTED_TEST_STATIC(aInfo->iHeap->CheckBitmap(Floor(aBfr, PAGESIZE), PAGESIZE, dummy, npages),
											 aInfo->iHeap, ETHeapBadCellAddress, aBfr, aLth);
				}
			if ( aAllocatorType & EPartialFullSlab )
				 WalkPartialFullSlab(aInfo, (slab*)aBfr, aBfrType, aLth);	
            else if ( aAllocatorType & EFullSlab )
					WalkFullSlab(aInfo, (slab*)aBfr, aBfrType, aLth);
			}
#if defined(_DEBUG)     
        else  if ( aBfrType == EGoodAllocatedCell )
            aInfo->iFunction(aInfo->iParam, aBfrType, ((TUint8*)aBfr+EDebugHdrSize), (aLth-EDebugHdrSize) );
        else
            aInfo->iFunction(aInfo->iParam, aBfrType,  aBfr, aLth );
#else
        else
            aInfo->iFunction(aInfo->iParam, aBfrType, aBfr, aLth );
#endif

#endif // __KERNEL_MODE	
        }
}

#ifndef __KERNEL_MODE__
void RHybridHeap::WalkPartialFullSlab(SWalkInfo* aInfo, slab* aSlab, TCellType /*aBfrType*/, TInt /*aLth*/)
{
	if ( aInfo )
		{
		//
		// Build bitmap of free buffers in the partial full slab
		//
		TUint32 bitmap[4];
		__HEAP_CORRUPTED_TEST_STATIC( (aInfo->iHeap != NULL), aInfo->iHeap, ETHeapBadCellAddress, 0, aSlab);
		aInfo->iHeap->BuildPartialSlabBitmap(bitmap, aSlab);
		//
		// Find used (allocated) buffers from iPartial full slab
		//
		TUint32 h = aSlab->iHeader;
		TUint32 size = SlabHeaderSize(h);
		TUint32 count = KMaxSlabPayload / size;  // Total buffer count in slab
		TUint32 i = 0;
		TUint32 ix = 0;
		TUint32 bit = 1;				

		while ( i < count )
			{

			if ( bitmap[ix] & bit )
				{
				aInfo->iFunction(aInfo->iParam, EGoodFreeCell, &aSlab->iPayload[i*size], size ); 
				} 
			else
				{
#if defined(_DEBUG)
				aInfo->iFunction(aInfo->iParam, EGoodAllocatedCell, (&aSlab->iPayload[i*size]+EDebugHdrSize), (size-EDebugHdrSize) );
#else				
				aInfo->iFunction(aInfo->iParam, EGoodAllocatedCell, &aSlab->iPayload[i*size], size );
#endif
				}
			bit <<= 1;
			if ( bit == 0 )
				{
				bit = 1;
				ix ++;
				}

			i ++;
			}
		}

}

void RHybridHeap::WalkFullSlab(SWalkInfo* aInfo, slab* aSlab, TCellType aBfrType, TInt /*aLth*/)
{
	if ( aInfo )
		{
		TUint32 h = aSlab->iHeader;
		TUint32 size = SlabHeaderSize(h);
		TUint32 count = (SlabHeaderUsedm4(h) + 4) / size;
		TUint32 i = 0;
		while ( i < count )
			{
#if defined(_DEBUG)
			if ( aBfrType == EGoodAllocatedCell )
				aInfo->iFunction(aInfo->iParam, aBfrType, (&aSlab->iPayload[i*size]+EDebugHdrSize), (size-EDebugHdrSize) );
			else
				aInfo->iFunction(aInfo->iParam, aBfrType, &aSlab->iPayload[i*size], size );
#else
			aInfo->iFunction(aInfo->iParam, aBfrType, &aSlab->iPayload[i*size], size );
#endif      
			i ++;
			}
		}
}

void RHybridHeap::BuildPartialSlabBitmap(TUint32* aBitmap, slab* aSlab, TAny* aBfr)
{
	//
	// Build a bitmap of free buffers in a partial full slab
	//
	TInt i;
	TUint32 bit = 0;
	TUint32 index;  
	TUint32 h = aSlab->iHeader;
	TUint32 used = SlabHeaderUsedm4(h)+4;
	TUint32 size = SlabHeaderSize(h);
	TInt    count = (KMaxSlabPayload / size);
	TInt    free_count = count -  (used / size); // Total free buffer count in slab
	aBitmap[0] = 0, aBitmap[1] = 0,	aBitmap[2] = 0, aBitmap[3] = 0;
	TUint32 offs = (h & 0xff) << 2;

	//
	// Process first buffer in partial slab free buffer chain
	//
	while ( offs )
		{
		unsigned char* p = (unsigned char*)Offset(aSlab, offs); 		
		__HEAP_CORRUPTED_TEST( (sizeof(slabhdr) <= offs), ETHeapBadCellAddress, p, aSlab);
		offs -= sizeof(slabhdr);
		__HEAP_CORRUPTED_TEST( (offs % size == 0), ETHeapBadCellAddress, p, aSlab);
		index = (offs / size);  // Bit index in bitmap
		i = 0;
		while ( i < 4 )
			{
			if ( index < 32 )
				{
				bit = (1 << index);
				break;
				}
			index -= 32;
			i ++;
			}

		__HEAP_CORRUPTED_TEST( ((aBitmap[i] & bit) == 0), ETHeapBadCellAddress, p, aSlab);  // Buffer already in chain

		aBitmap[i] |= bit;
		free_count --;
		offs = ((unsigned)*p) << 2; // Next in free chain
		}

	__HEAP_CORRUPTED_TEST( (free_count >= 0), ETHeapBadCellAddress, aBfr, aSlab);  // free buffer count/size mismatch	
	//
	// Process next rest of the free buffers which are in the
	// wilderness (at end of the slab)
	//
	index = count - 1;
	i = index / 32;
	index = index % 32;
	while ( free_count && (i >= 0))
		{
		bit = (1 << index);
		__HEAP_CORRUPTED_TEST( ((aBitmap[i] & bit) == 0), ETHeapBadCellAddress, aBfr, aSlab);  // Buffer already in chain
		aBitmap[i] |= bit;
		if ( index )
			index --;
		else
			{
			index = 31;
			i --;
			}
		free_count --;
		}

	if ( aBfr )  // Assure that specified buffer does NOT exist in partial slab free buffer chain
		{
		offs = LowBits(aBfr, SLABSIZE);
		__HEAP_CORRUPTED_TEST( (sizeof(slabhdr) <= offs), ETHeapBadCellAddress, aBfr, aSlab);
		offs -= sizeof(slabhdr);
		__HEAP_CORRUPTED_TEST( ((offs % size) == 0), ETHeapBadCellAddress, aBfr, aSlab);
		index = (offs / size);  // Bit index in bitmap
		i = 0;
		while ( i < 4 )
			{
			if ( index < 32 )
				{
				bit = (1 << index);
				break;
				}
			index -= 32;
			i ++;
			}
		__HEAP_CORRUPTED_TEST( ((aBitmap[i] & bit) == 0), ETHeapBadCellAddress, aBfr, aSlab);  // Buffer already in chain
		}
}

#endif	// __KERNEL_MODE__

void RHybridHeap::WalkCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen)
{
    (void)aCell;
    SHeapCellInfo& info = *(SHeapCellInfo*)aPtr;
    switch(aType)
        {
        case EGoodAllocatedCell:
            {
            ++info.iTotalAlloc;
            info.iTotalAllocSize += aLen; 
#if defined(_DEBUG)
            RHybridHeap& h = *info.iHeap;
            SDebugCell* DbgCell = (SDebugCell*)((TUint8*)aCell-EDebugHdrSize);
            if ( DbgCell->nestingLevel == h.iNestingLevel )
                {
                if (++info.iLevelAlloc==1)
                    info.iStranded = DbgCell;
#ifdef __KERNEL_MODE__
                if (KDebugNum(KSERVER) || KDebugNum(KTESTFAST))
                    {
                    Kern::Printf("LEAKED KERNEL HEAP CELL @ %08x : len=%d", aCell, aLen);
                    TLinAddr base = ((TLinAddr)aCell)&~0x0f;
                    TLinAddr end = ((TLinAddr)aCell)+(TLinAddr)aLen;
                    while(base<end)
                        {
                        const TUint32* p = (const TUint32*)base;
                        Kern::Printf("%08x: %08x %08x %08x %08x", p, p[0], p[1], p[2], p[3]);
                        base += 16;
                        }
                    }
#endif
                }
#endif  
            break;
            }
        case EGoodFreeCell:
            ++info.iTotalFree;
            break;
        case EBadAllocatedCellSize:
            HEAP_PANIC(ETHeapBadAllocatedCellSize);
        case EBadAllocatedCellAddress:
            HEAP_PANIC(ETHeapBadAllocatedCellAddress);
        case EBadFreeCellAddress:
            HEAP_PANIC(ETHeapBadFreeCellAddress);
        case EBadFreeCellSize:
            HEAP_PANIC(ETHeapBadFreeCellSize);
        default:
            HEAP_PANIC(ETHeapWalkBadCellType);
        }
}


TInt RHybridHeap::DoCheckHeap(SCheckInfo* aInfo)
{
    (void)aInfo;
    SHeapCellInfo info;
    memclr(&info, sizeof(info));
    info.iHeap = this;
    struct HeapInfo hinfo;
    SWalkInfo winfo;
    Lock();
	DoCheckMallocState(GM);  // Check DL heap internal structure
#ifndef __KERNEL_MODE__
	TUint32 dummy;
	TInt    npages;
	__HEAP_CORRUPTED_TEST(CheckBitmap(NULL, 0, dummy, npages), ETHeapBadCellAddress, this, 0);  // Check page allocator buffers
	DoCheckSlabTrees();	
	DoCheckCommittedSize(npages, GM);
#endif				   
    winfo.iFunction = WalkCheckCell;
    winfo.iParam    = &info;
	winfo.iHeap     = (RHybridHeap*)this; 		
    GetInfo(&hinfo, &winfo);
    Unlock();
    
#if defined(_DEBUG)
    if (!aInfo)
        return KErrNone;
    TInt expected = aInfo->iCount;
    TInt actual = aInfo->iAll ? info.iTotalAlloc : info.iLevelAlloc;
    if (actual!=expected && !iTestData)
        {
#ifdef __KERNEL_MODE__
        Kern::Fault("KERN-ALLOC COUNT", (expected<<16)|actual );
#else
        User::Panic(_L("ALLOC COUNT"), (expected<<16)|actual );
#endif
        }
#endif
    return KErrNone;
}

#ifdef _DEBUG
void RHybridHeap::DoMarkStart()
{
    if (iNestingLevel==0)
        iAllocCount=0;
    iNestingLevel++;
}

TUint32 RHybridHeap::DoMarkEnd(TInt aExpected)
{
    if (iNestingLevel==0)
        return 0;
    SHeapCellInfo info;
    SHeapCellInfo* p = iTestData ? (SHeapCellInfo*)iTestData : &info;
    memclr(p, sizeof(info));
    p->iHeap = this;
    struct HeapInfo hinfo;
    SWalkInfo winfo;
    Lock();
    winfo.iFunction = WalkCheckCell;
    winfo.iParam    = p;
	winfo.iHeap     = (RHybridHeap*)this; 	
    GetInfo(&hinfo, &winfo);
    Unlock();
    
    if (p->iLevelAlloc != aExpected && !iTestData)
        return (TUint32)(p->iStranded + 1);
    if (--iNestingLevel == 0)
        iAllocCount = 0;
    return 0;
}

void RHybridHeap::DoSetAllocFail(TAllocFail aType, TInt aRate)
{// Default to a burst mode of 1, as aType may be a burst type.
    DoSetAllocFail(aType, aRate, 1);
}

void ResetAllocCellLevels(TAny* aPtr, RHybridHeap::TCellType aType, TAny* aCell, TInt aLen)
{
    (void)aPtr;
    (void)aLen;
    
    if (aType == RHybridHeap::EGoodAllocatedCell)
        {
        RHybridHeap::SDebugCell* DbgCell = (RHybridHeap::SDebugCell*)((TUint8*)aCell-RHybridHeap::EDebugHdrSize);
        DbgCell->nestingLevel = 0;
        }
}

// Don't change as the ETHeapBadDebugFailParameter check below and the API 
// documentation rely on this being 16 for RHybridHeap.
LOCAL_D const TInt KBurstFailRateShift = 16;
LOCAL_D const TInt KBurstFailRateMask = (1 << KBurstFailRateShift) - 1;

void RHybridHeap::DoSetAllocFail(TAllocFail aType, TInt aRate, TUint aBurst)
{
    if (aType==EReset)
        {
        // reset levels of all allocated cells to 0
        // this should prevent subsequent tests failing unnecessarily
        iFailed = EFalse;       // Reset for ECheckFailure relies on this.
        struct HeapInfo hinfo;
        SWalkInfo winfo;
        Lock();
        winfo.iFunction = (TWalkFunc)&ResetAllocCellLevels;
        winfo.iParam    = NULL;
		winfo.iHeap     = (RHybridHeap*)this; 	
        GetInfo(&hinfo, &winfo);
        Unlock();
        // reset heap allocation mark as well
        iNestingLevel=0;
        iAllocCount=0;
        aType=ENone;
        }
    
    switch (aType)
        {
        case EBurstRandom:
        case EBurstTrueRandom:
        case EBurstDeterministic:
        case EBurstFailNext:
            // If the fail type is a burst type then iFailRate is split in 2:
            // the 16 lsbs are the fail rate and the 16 msbs are the burst length.
            if (TUint(aRate) > (TUint)KMaxTUint16 || aBurst > KMaxTUint16)
                HEAP_PANIC(ETHeapBadDebugFailParameter);
            
            iFailed = EFalse;
            iFailType = aType;
            iFailRate = (aRate == 0) ? 1 : aRate;
            iFailAllocCount = -iFailRate;
            iFailRate = iFailRate | (aBurst << KBurstFailRateShift);
            break;
            
        default:
            iFailed = EFalse;
            iFailType = aType;
            iFailRate = (aRate == 0) ? 1 : aRate; // A rate of <1 is meaningless
            iFailAllocCount = 0;
            break;
        }
    
    // Set up iRand for either:
    //      - random seed value, or
    //      - a count of the number of failures so far.
    iRand = 0;
#ifndef __KERNEL_MODE__
    switch (iFailType)
        {
        case ETrueRandom:
        case EBurstTrueRandom:
            {
            TTime time;
            time.HomeTime();
            TInt64 seed = time.Int64();
            iRand = Math::Rand(seed);
            break;
            }
        case ERandom:
        case EBurstRandom:
            {
            TInt64 seed = 12345;
            iRand = Math::Rand(seed);
            break;
            }
        default:
            break;
        }
#endif
}

TBool RHybridHeap::CheckForSimulatedAllocFail()
//
// Check to see if the user has requested simulated alloc failure, and if so possibly 
// Return ETrue indicating a failure.
//
{
    // For burst mode failures iFailRate is shared
    TUint16 rate  = (TUint16)(iFailRate &  KBurstFailRateMask);
    TUint16 burst = (TUint16)(iFailRate >> KBurstFailRateShift);
    TBool r = EFalse;
    switch (iFailType)
        {
#ifndef __KERNEL_MODE__
        case ERandom:
        case ETrueRandom:
            if (++iFailAllocCount>=iFailRate) 
                {   
                iFailAllocCount=0;
                if (!iFailed) // haven't failed yet after iFailRate allocations so fail now
                    return(ETrue); 
                iFailed=EFalse;
                }
            else   
                {
                if (!iFailed)
                    {
                    TInt64 seed=iRand;
                    iRand=Math::Rand(seed);
                    if (iRand%iFailRate==0)
                        {
                        iFailed=ETrue;
                        return(ETrue);
                        }
                    }
                }
            break;
            
        case EBurstRandom:
        case EBurstTrueRandom:
            if (++iFailAllocCount < 0) 
                {
                // We haven't started failing yet so should we now?
                TInt64 seed = iRand;
                iRand = Math::Rand(seed);
                if (iRand % rate == 0)
                    {// Fail now.  Reset iFailAllocCount so we fail burst times
                    iFailAllocCount = 0;
                    r = ETrue;
                    }
                }
            else
                {
                if (iFailAllocCount < burst)
                    {// Keep failing for burst times
                    r = ETrue;
                    }
                else
                    {// We've now failed burst times so start again.
                    iFailAllocCount = -(rate - 1);
                    }
                }
            break;
#endif
        case EDeterministic:
            if (++iFailAllocCount%iFailRate==0)
                {
                r=ETrue;
                iRand++;    // Keep count of how many times we have failed
                }
            break;
            
        case EBurstDeterministic:
            // This will fail burst number of times, every rate attempts.
            if (++iFailAllocCount >= 0)
                {
                if (iFailAllocCount == burst - 1)
                    {// This is the burst time we have failed so make it the last by
                    // reseting counts so we next fail after rate attempts.
                    iFailAllocCount = -rate;
                    }
                r = ETrue;
                iRand++;    // Keep count of how many times we have failed
                }
            break;
            
        case EFailNext:
            if ((++iFailAllocCount%iFailRate)==0)
                {
                iFailType=ENone;
                r=ETrue;
                iRand++;    // Keep count of how many times we have failed
                }
            break;
            
        case EBurstFailNext:
            if (++iFailAllocCount >= 0)
                {
                if (iFailAllocCount == burst - 1)
                    {// This is the burst time we have failed so make it the last.
                    iFailType = ENone;
                    }
                r = ETrue;
                iRand++;    // Keep count of how many times we have failed
                }
            break;
            
        default:
            break;
        }
    return r;
}

#endif  // DEBUG

//
//  Methods for Doug Lea allocator detailed check
//

void RHybridHeap::DoCheckAnyChunk(mstate m, mchunkptr p)
{
    __HEAP_CORRUPTED_TEST(((IS_ALIGNED(CHUNK2MEM(p))) || (p->iHead == FENCEPOST_HEAD)), ETHeapBadCellAddress, p, 0);
	(void)m;
}

/* Check properties of iTop chunk */
void RHybridHeap::DoCheckTopChunk(mstate m, mchunkptr p)
{
    msegmentptr sp = &m->iSeg;
    size_t  sz = CHUNKSIZE(p);
    __HEAP_CORRUPTED_TEST((sp != 0), ETHeapBadCellAddress, p, 0);
    __HEAP_CORRUPTED_TEST(((IS_ALIGNED(CHUNK2MEM(p))) || (p->iHead == FENCEPOST_HEAD)), ETHeapBadCellAddress, p,0);
    __HEAP_CORRUPTED_TEST((sz == m->iTopSize), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((sz > 0), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((sz == ((sp->iBase + sp->iSize) - (TUint8*)p) - TOP_FOOT_SIZE), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((PINUSE(p)), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((!NEXT_PINUSE(p)), ETHeapBadCellAddress,p,0);
}

/* Check properties of inuse chunks */
void RHybridHeap::DoCheckInuseChunk(mstate m, mchunkptr p)
{
    DoCheckAnyChunk(m, p);
    __HEAP_CORRUPTED_TEST((CINUSE(p)), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((NEXT_PINUSE(p)), ETHeapBadCellAddress,p,0);
    /* If not PINUSE and not mmapped, previous chunk has OK offset */
    __HEAP_CORRUPTED_TEST((PINUSE(p) || NEXT_CHUNK(PREV_CHUNK(p)) == p), ETHeapBadCellAddress,p,0);
}

/* Check properties of free chunks */
void RHybridHeap::DoCheckFreeChunk(mstate m, mchunkptr p)
{
    size_t sz = p->iHead & ~(PINUSE_BIT|CINUSE_BIT);
    mchunkptr next = CHUNK_PLUS_OFFSET(p, sz);
    DoCheckAnyChunk(m, p);
    __HEAP_CORRUPTED_TEST((!CINUSE(p)), ETHeapBadCellAddress,p,0);
    __HEAP_CORRUPTED_TEST((!NEXT_PINUSE(p)), ETHeapBadCellAddress,p,0);
    if (p != m->iDv && p != m->iTop)
        {
        if (sz >= MIN_CHUNK_SIZE)
            {
            __HEAP_CORRUPTED_TEST(((sz & CHUNK_ALIGN_MASK) == 0), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((IS_ALIGNED(CHUNK2MEM(p))), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((next->iPrevFoot == sz), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((PINUSE(p)), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST( (next == m->iTop || CINUSE(next)), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((p->iFd->iBk == p), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((p->iBk->iFd == p), ETHeapBadCellAddress,p,0);
            }
        else    /* markers are always of size SIZE_T_SIZE */
            __HEAP_CORRUPTED_TEST((sz == SIZE_T_SIZE), ETHeapBadCellAddress,p,0);
        }
}

/* Check properties of malloced chunks at the point they are malloced */
void RHybridHeap::DoCheckMallocedChunk(mstate m, void* mem, size_t s)
{
    if (mem != 0)
        {
        mchunkptr p = MEM2CHUNK(mem);
        size_t sz = p->iHead & ~(PINUSE_BIT|CINUSE_BIT);
        DoCheckInuseChunk(m, p);
        __HEAP_CORRUPTED_TEST(((sz & CHUNK_ALIGN_MASK) == 0), ETHeapBadCellAddress,p,0);
        __HEAP_CORRUPTED_TEST((sz >= MIN_CHUNK_SIZE), ETHeapBadCellAddress,p,0);
        __HEAP_CORRUPTED_TEST((sz >= s), ETHeapBadCellAddress,p,0);
        /* unless mmapped, size is less than MIN_CHUNK_SIZE more than request */
        __HEAP_CORRUPTED_TEST((sz < (s + MIN_CHUNK_SIZE)), ETHeapBadCellAddress,p,0);
        }
}

/* Check a tree and its subtrees.   */
void RHybridHeap::DoCheckTree(mstate m, tchunkptr t)
{
    tchunkptr head = 0;
    tchunkptr u = t;
    bindex_t tindex = t->iIndex;
    size_t tsize = CHUNKSIZE(t);
    bindex_t idx;
    DoComputeTreeIndex(tsize, idx);
    __HEAP_CORRUPTED_TEST((tindex == idx), ETHeapBadCellAddress,u,0);
    __HEAP_CORRUPTED_TEST((tsize >= MIN_LARGE_SIZE), ETHeapBadCellAddress,u,0);
    __HEAP_CORRUPTED_TEST((tsize >= MINSIZE_FOR_TREE_INDEX(idx)), ETHeapBadCellAddress,u,0);
    __HEAP_CORRUPTED_TEST(((idx == NTREEBINS-1) || (tsize < MINSIZE_FOR_TREE_INDEX((idx+1)))), ETHeapBadCellAddress,u,0);
    
    do
        { /* traverse through chain of same-sized nodes */
        DoCheckAnyChunk(m, ((mchunkptr)u));
        __HEAP_CORRUPTED_TEST((u->iIndex == tindex), ETHeapBadCellAddress,u,0);
        __HEAP_CORRUPTED_TEST((CHUNKSIZE(u) == tsize), ETHeapBadCellAddress,u,0);
        __HEAP_CORRUPTED_TEST((!CINUSE(u)), ETHeapBadCellAddress,u,0);
        __HEAP_CORRUPTED_TEST((!NEXT_PINUSE(u)), ETHeapBadCellAddress,u,0);
        __HEAP_CORRUPTED_TEST((u->iFd->iBk == u), ETHeapBadCellAddress,u,0);
        __HEAP_CORRUPTED_TEST((u->iBk->iFd == u), ETHeapBadCellAddress,u,0);
        if (u->iParent == 0)
            {
            __HEAP_CORRUPTED_TEST((u->iChild[0] == 0), ETHeapBadCellAddress,u,0);
            __HEAP_CORRUPTED_TEST((u->iChild[1] == 0), ETHeapBadCellAddress,u,0);
            }
        else
            {
            __HEAP_CORRUPTED_TEST((head == 0), ETHeapBadCellAddress,u,0); /* only one node on chain has iParent */
            head = u;
            __HEAP_CORRUPTED_TEST((u->iParent != u), ETHeapBadCellAddress,u,0);
            __HEAP_CORRUPTED_TEST( (u->iParent->iChild[0] == u ||
                    u->iParent->iChild[1] == u ||
                    *((tbinptr*)(u->iParent)) == u), ETHeapBadCellAddress,u,0);
            if (u->iChild[0] != 0)
                {
                __HEAP_CORRUPTED_TEST((u->iChild[0]->iParent == u), ETHeapBadCellAddress,u,0);
                __HEAP_CORRUPTED_TEST((u->iChild[0] != u), ETHeapBadCellAddress,u,0);
                DoCheckTree(m, u->iChild[0]);
                }
            if (u->iChild[1] != 0)
                {
                __HEAP_CORRUPTED_TEST((u->iChild[1]->iParent == u), ETHeapBadCellAddress,u,0);
                __HEAP_CORRUPTED_TEST((u->iChild[1] != u), ETHeapBadCellAddress,u,0);
                DoCheckTree(m, u->iChild[1]);
                }
            if (u->iChild[0] != 0 && u->iChild[1] != 0)
                {
                __HEAP_CORRUPTED_TEST((CHUNKSIZE(u->iChild[0]) < CHUNKSIZE(u->iChild[1])), ETHeapBadCellAddress,u,0);
                }
            }
        u = u->iFd;
        }
    while (u != t);
    __HEAP_CORRUPTED_TEST((head != 0), ETHeapBadCellAddress,u,0);
}

/*  Check all the chunks in a treebin.  */
void RHybridHeap::DoCheckTreebin(mstate m, bindex_t i)
{
    tbinptr* tb = TREEBIN_AT(m, i);
    tchunkptr t = *tb;
    int empty = (m->iTreeMap & (1U << i)) == 0;
    if (t == 0)
        __HEAP_CORRUPTED_TEST((empty), ETHeapBadCellAddress,t,0);
    if (!empty)
        DoCheckTree(m, t);
}

/*  Check all the chunks in a smallbin. */
void RHybridHeap::DoCheckSmallbin(mstate m, bindex_t i)
{
    sbinptr b = SMALLBIN_AT(m, i);
    mchunkptr p = b->iBk;
    unsigned int empty = (m->iSmallMap & (1U << i)) == 0;
    if (p == b)
        __HEAP_CORRUPTED_TEST((empty), ETHeapBadCellAddress,p,0);
    if (!empty)
        {
        for (; p != b; p = p->iBk)
            {
            size_t size = CHUNKSIZE(p);
            mchunkptr q;
            /* each chunk claims to be free */
            DoCheckFreeChunk(m, p);
            /* chunk belongs in bin */
            __HEAP_CORRUPTED_TEST((SMALL_INDEX(size) == i), ETHeapBadCellAddress,p,0);
            __HEAP_CORRUPTED_TEST((p->iBk == b || CHUNKSIZE(p->iBk) == CHUNKSIZE(p)), ETHeapBadCellAddress,p,0);
            /* chunk is followed by an inuse chunk */
            q = NEXT_CHUNK(p);
            if (q->iHead != FENCEPOST_HEAD)
                DoCheckInuseChunk(m, q);
            }
        }
}

/* Find x in a bin. Used in other check functions. */
TInt RHybridHeap::BinFind(mstate m, mchunkptr x)
{
    size_t size = CHUNKSIZE(x);
    if (IS_SMALL(size))
        {
        bindex_t sidx = SMALL_INDEX(size);
        sbinptr b = SMALLBIN_AT(m, sidx);
        if (SMALLMAP_IS_MARKED(m, sidx))
            {
            mchunkptr p = b;
            do
                {
                if (p == x)
                    return 1;
                }
            while ((p = p->iFd) != b);
            }
        }
    else
        {
        bindex_t tidx;
        DoComputeTreeIndex(size, tidx);
        if (TREEMAP_IS_MARKED(m, tidx))
            {
            tchunkptr t = *TREEBIN_AT(m, tidx);
            size_t sizebits = size << LEFTSHIFT_FOR_TREE_INDEX(tidx);
            while (t != 0 && CHUNKSIZE(t) != size)
                {
                t = t->iChild[(sizebits >> (SIZE_T_BITSIZE-SIZE_T_ONE)) & 1];
                sizebits <<= 1;
                }
            if (t != 0)
                {
                tchunkptr u = t;
                do
                    {
                    if (u == (tchunkptr)x)
                        return 1;
                    }
                while ((u = u->iFd) != t);
                }
            }
        }
    return 0;
}

/* Traverse each chunk and check it; return total */
size_t RHybridHeap::TraverseAndCheck(mstate m)
{
    size_t sum = 0;
    msegmentptr s = &m->iSeg;
    sum += m->iTopSize + TOP_FOOT_SIZE;
    mchunkptr q = ALIGN_AS_CHUNK(s->iBase);
    mchunkptr lastq = 0;
    __HEAP_CORRUPTED_TEST((PINUSE(q)), ETHeapBadCellAddress,q,0);
    while (q != m->iTop && q->iHead != FENCEPOST_HEAD)
        {
        sum += CHUNKSIZE(q);
        if (CINUSE(q))
            {
            __HEAP_CORRUPTED_TEST((!BinFind(m, q)), ETHeapBadCellAddress,q,0);
            DoCheckInuseChunk(m, q);
            }
        else
            {
            __HEAP_CORRUPTED_TEST((q == m->iDv || BinFind(m, q)), ETHeapBadCellAddress,q,0);
            __HEAP_CORRUPTED_TEST((lastq == 0 || CINUSE(lastq)), ETHeapBadCellAddress,q,0); /* Not 2 consecutive free */
            DoCheckFreeChunk(m, q);
            }
        lastq = q;
        q = NEXT_CHUNK(q);
        }
    return sum;
}

/* Check all properties of malloc_state. */
void RHybridHeap::DoCheckMallocState(mstate m)
{
    bindex_t i;
//    size_t total;
    /* check bins */
    for (i = 0; i < NSMALLBINS; ++i)
        DoCheckSmallbin(m, i);
    for (i = 0; i < NTREEBINS; ++i)
        DoCheckTreebin(m, i);
    
    if (m->iDvSize != 0)
        { /* check iDv chunk */
        DoCheckAnyChunk(m, m->iDv);
        __HEAP_CORRUPTED_TEST((m->iDvSize == CHUNKSIZE(m->iDv)), ETHeapBadCellAddress,m->iDv,0);
        __HEAP_CORRUPTED_TEST((m->iDvSize >= MIN_CHUNK_SIZE), ETHeapBadCellAddress,m->iDv,0);
        __HEAP_CORRUPTED_TEST((BinFind(m, m->iDv) == 0), ETHeapBadCellAddress,m->iDv,0);
        }
    
    if (m->iTop != 0)
        {    /* check iTop chunk */
        DoCheckTopChunk(m, m->iTop);
        __HEAP_CORRUPTED_TEST((m->iTopSize == CHUNKSIZE(m->iTop)), ETHeapBadCellAddress,m->iTop,0);
        __HEAP_CORRUPTED_TEST((m->iTopSize > 0), ETHeapBadCellAddress,m->iTop,0);
        __HEAP_CORRUPTED_TEST((BinFind(m, m->iTop) == 0), ETHeapBadCellAddress,m->iTop,0);
        }
    
//    total =
    TraverseAndCheck(m);
}

#ifndef __KERNEL_MODE__
//
//  Methods for Slab allocator detailed check
//
void RHybridHeap::DoCheckSlabTree(slab** aS, TBool aPartialPage)
{
	slab* s = *aS;
	if (!s)
		return;

	TUint size = SlabHeaderSize(s->iHeader);
	slab** parent = aS;
	slab** child2 = &s->iChild2;

	while ( s )
		{
		__HEAP_CORRUPTED_TEST((s->iParent == parent), ETHeapBadCellAddress,s,SLABSIZE);
		__HEAP_CORRUPTED_TEST((!s->iChild1 || s < s->iChild1), ETHeapBadCellAddress,s,SLABSIZE);
		__HEAP_CORRUPTED_TEST((!s->iChild2 || s < s->iChild2), ETHeapBadCellAddress,s,SLABSIZE);

		if ( aPartialPage )
			{
			if ( s->iChild1 )
				size = SlabHeaderSize(s->iChild1->iHeader);
			}
		else
			{
			__HEAP_CORRUPTED_TEST((SlabHeaderSize(s->iHeader) == size), ETHeapBadCellAddress,s,SLABSIZE);
			}
		parent = &s->iChild1;
		s = s->iChild1;

		}

	parent = child2;
	s = *child2;

	while ( s )
		{
		__HEAP_CORRUPTED_TEST((s->iParent == parent), ETHeapBadCellAddress,s,SLABSIZE);
		__HEAP_CORRUPTED_TEST((!s->iChild1 || s < s->iChild1), ETHeapBadCellAddress,s,SLABSIZE);
		__HEAP_CORRUPTED_TEST((!s->iChild2 || s < s->iChild2), ETHeapBadCellAddress,s,SLABSIZE);

		if ( aPartialPage )
			{
			if ( s->iChild2 )
				size = SlabHeaderSize(s->iChild2->iHeader);
			}
		else
			{
			__HEAP_CORRUPTED_TEST((SlabHeaderSize(s->iHeader) == size), ETHeapBadCellAddress,s,SLABSIZE);
			}
		parent = &s->iChild2;
		s = s->iChild2;

		}

}

void RHybridHeap::DoCheckSlabTrees()
{
	for (TInt i = 0; i < (MAXSLABSIZE>>2); ++i)
		DoCheckSlabTree(&iSlabAlloc[i].iPartial, EFalse);
	DoCheckSlabTree(&iPartialPage, ETrue);
}

void RHybridHeap::DoCheckSlab(slab* aSlab, TAllocatorType aSlabType, TAny* aBfr)
{
   if ( (aSlabType == ESlabSpare) || (aSlabType == EEmptySlab) )
	  return;
   
   unsigned h = aSlab->iHeader;
   __HEAP_CORRUPTED_TEST((ZEROBITS(h)), ETHeapBadCellAddress,aBfr,aSlab);   
   unsigned used = SlabHeaderUsedm4(h)+4;
   unsigned size = SlabHeaderSize(h);
   __HEAP_CORRUPTED_TEST( (used < SLABSIZE),ETHeapBadCellAddress, aBfr, aSlab);
   __HEAP_CORRUPTED_TEST( ((size > 3 ) && (size <= MAXSLABSIZE)), ETHeapBadCellAddress,aBfr,aSlab);
	unsigned count = 0;

	switch ( aSlabType )
		{
		case EFullSlab:
			count = (KMaxSlabPayload / size );			
  			__HEAP_CORRUPTED_TEST((used == count*size), ETHeapBadCellAddress,aBfr,aSlab);	
			__HEAP_CORRUPTED_TEST((HeaderFloating(h)), ETHeapBadCellAddress,aBfr,aSlab);
			break;

		case EPartialFullSlab:
			__HEAP_CORRUPTED_TEST(((used % size)==0),ETHeapBadCellAddress,aBfr,aSlab);
			__HEAP_CORRUPTED_TEST(((SlabHeaderFree(h) == 0) || (((SlabHeaderFree(h)<<2)-sizeof(slabhdr)) % SlabHeaderSize(h) == 0)),
								  ETHeapBadCellAddress,aBfr,aSlab);
			break;

		default:
            break;
			
		}
}

//
//  Check that committed size in heap equals number of pages in bitmap
//  plus size of Doug Lea region
//
void RHybridHeap::DoCheckCommittedSize(TInt aNPages, mstate aM)
{
	TInt total_committed = (aNPages * iPageSize) + aM->iSeg.iSize + (iBase - (TUint8*)this);
	__HEAP_CORRUPTED_TEST((total_committed == iChunkSize), ETHeapBadCellAddress,total_committed,iChunkSize);	
}

#endif  // __KERNEL_MODE__  

#endif /* QT_USE_NEW_SYMBIAN_ALLOCATOR */
