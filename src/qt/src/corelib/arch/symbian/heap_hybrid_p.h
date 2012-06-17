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

#ifndef __HEAP_HYBRID_H__
#define __HEAP_HYBRID_H__

#include <e32cmn.h>

#ifdef __WINS__
#define USE_HYBRID_HEAP 0
#else
#define USE_HYBRID_HEAP 1
#endif

// This stuff is all temporary in order to prevent having to include dla.h from heap_hybrid.h, which causes
// problems due to its definition of size_t (and possibly other types).  This is unfortunate but we cannot
// pollute the namespace with these types or it will cause problems with Open C and other POSIX compatibility
// efforts in Symbian

#define NSMALLBINS        (32U)
#define NTREEBINS         (32U)

#ifndef MALLOC_ALIGNMENT
	#define MALLOC_ALIGNMENT ((TUint)8U)
#endif  /* MALLOC_ALIGNMENT */

#define CHUNK_OVERHEAD      (sizeof(TUint))

typedef unsigned int bindex_t;
typedef unsigned int binmap_t;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_segment msegment;
typedef struct malloc_state* mstate;
typedef struct malloc_tree_chunk* tbinptr;
typedef struct malloc_tree_chunk* tchunkptr;

struct malloc_segment {
  TUint8*        iBase;             /* base address */
  TUint       iSize;             /* allocated size */
};

struct malloc_state {
	binmap_t	 iSmallMap;
	binmap_t	 iTreeMap;
	TUint		 iDvSize;
	TUint		 iTopSize;
	mchunkptr	iDv;
	mchunkptr	iTop;
	TUint		 iTrimCheck;
	mchunkptr	iSmallBins[(NSMALLBINS+1)*2];
	tbinptr		iTreeBins[NTREEBINS];
	msegment	iSeg;
	};

class RHybridHeap : public RHeap
	{

public:
    // declarations copied from Symbian^4 RAllocator and RHeap
    typedef void (*TWalkFunc)(TAny*, RHeap::TCellType, TAny*, TInt);
	enum TFlags {ESingleThreaded=1, EFixedSize=2, ETraceAllocs=4, EMonitorMemory=8,};
	enum TAllocDebugOp
		{
		ECount, EMarkStart, EMarkEnd, ECheck, ESetFail, ECopyDebugInfo, ESetBurstFail, EGetFail,
		EGetSize=48, EGetMaxLength, EGetBase, EAlignInteger, EAlignAddr
		};
	enum TDebugOp { EWalk = 128, EHybridHeap };
	enum THybridAllocFail
		{
		ERandom, ETrueRandom, EDeterministic, EHybridNone, EFailNext, EReset, EBurstRandom,
		EBurstTrueRandom, EBurstDeterministic, EBurstFailNext, ECheckFailure,
		};
	enum { EDebugHdrSize = sizeof(SDebugCell) };
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
	struct SRAllocatorBurstFail {TInt iBurst; TInt iRate; TInt iUnused[2];};
#endif

	struct HeapInfo
		{
	    unsigned iFootprint;
	    unsigned iMaxSize;
	    unsigned iAllocBytes;
	    unsigned iAllocN;
	    unsigned iFreeBytes;
	    unsigned iFreeN;
		};

	struct SHeapCellInfo { RHybridHeap* iHeap; TInt iTotalAlloc;	TInt iTotalAllocSize; TInt iTotalFree; TInt iLevelAlloc; SDebugCell* iStranded; };


	/**
    @internalComponent
    */
	enum TAllocatorType
			{ESlabAllocator, EDougLeaAllocator, EPageAllocator, EFullSlab=0x80, EPartialFullSlab=0x40, EEmptySlab=0x20, ESlabSpare=0x10, ESlabMask=0xf0};


	/**
    @internalComponent
    */
	struct SWalkInfo {
					  /**
	                  Walk function address shall be called
	                  */
		TWalkFunc iFunction;

					  /**
	                  The first parameter for callback function
	                  */
		TAny*        iParam;
					  /**
	                  Pointer to RHybridHeap object
	                  */
		RHybridHeap* iHeap;
		};

	/**
    @internalComponent
    */
	struct SConfig {
					  /**
	                  Required slab configuration ( bit 0=4, bit 1=8 ..
	                  bit 13 = 56)
	                  */
		TUint32 iSlabBits;
					  /**
	                  Delayed slab threshold in bytes (0 = no threshold)
	                  */
		TInt   iDelayedSlabThreshold;
					  /**
	                  2^n is smallest size allocated in paged allocator (14-31 = 16 Kb --> )
	                  */
		TInt   iPagePower;

		};

	/**
	@internalComponent

	This structure is used by test code for configuring the allocators and obtaining information
	from them in order to ensure they are behaving as required.  This is internal test specific
	code and is liable to be changed without warning at any time.  You should under no circumstances
	be using it!
	*/
	struct STestCommand
		{
		TInt	iCommand;			// The test related command to be executed

		union
			{
			SConfig		iConfig;	// Configuration used by test code only
			TAny*		iData;		// Extra supporting data for the test command
			};
		};

	/**
	@internalComponent

	Commands used by test code for configuring the allocators and obtaining information them them
	*/
	enum TTestCommand { EGetConfig, ESetConfig, EHeapMetaData, ETestData };

	virtual TAny* Alloc(TInt aSize);
	virtual void Free(TAny* aPtr);
	virtual TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
	virtual TInt AllocLen(const TAny* aCell) const;
#ifndef __KERNEL_MODE__	
	virtual TInt Compress();
	virtual void Reset();
	virtual TInt AllocSize(TInt& aTotalAllocSize) const;
	virtual TInt Available(TInt& aBiggestBlock) const;
#endif	
	virtual TInt DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL);
protected:
	virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);

public:
	TAny* operator new(TUint aSize, TAny* aBase) __NO_THROW;
	void operator delete(TAny*, TAny*);
	
private:
	TInt DoCountAllocFree(TInt& aFree);
	TInt DoCheckHeap(SCheckInfo* aInfo);
	void DoMarkStart();
	TUint32 DoMarkEnd(TInt aExpected);
	void DoSetAllocFail(TAllocFail aType, TInt aRate);
	TBool CheckForSimulatedAllocFail();
	void DoSetAllocFail(TAllocFail aType, TInt aRate, TUint aBurst);
	
	void Lock() const;
	void Unlock() const;
	TInt ChunkHandle() const;

	RHybridHeap(TInt aChunkHandle, TInt aOffset, TInt aMinLength, TInt aMaxLength, TInt aGrowBy, TInt aAlign, TBool aSingleThread, TBool aDlOnly, TBool aUseAdjust);	
	RHybridHeap(TInt aMaxLength, TInt aAlign=0, TBool aSingleThread=ETrue);
	RHybridHeap();
	
	void Init(TInt aBitmapSlab, TInt aPagePower);
	inline void InitBins(mstate m);
	inline void InitTop(mstate m, mchunkptr p, TUint psize);
    void* SysAlloc(mstate m, TUint nb);
	int SysTrim(mstate m, TUint pad);
	void* TmallocLarge(mstate m, TUint nb);
	void* TmallocSmall(mstate m, TUint nb);
	/*MACROS converted functions*/
	static inline void UnlinkFirstSmallChunk(mstate M,mchunkptr B,mchunkptr P,bindex_t& I);
	static inline void InsertSmallChunk(mstate M,mchunkptr P, TUint S);
	static inline void InsertChunk(mstate M,mchunkptr P,TUint S);
	static inline void UnlinkLargeChunk(mstate M,tchunkptr X);
	static inline void UnlinkSmallChunk(mstate M, mchunkptr P,TUint S);
	static inline void UnlinkChunk(mstate M, mchunkptr P, TUint S);
	static inline void ComputeTreeIndex(TUint S, bindex_t& I);
	static inline void InsertLargeChunk(mstate M,tchunkptr X,TUint S);
	static inline void ReplaceDv(mstate M, mchunkptr P, TUint S);
	static inline void ComputeBit2idx(binmap_t X,bindex_t& I);

	void DoComputeTreeIndex(TUint S, bindex_t& I);	
	void DoCheckAnyChunk(mstate m, mchunkptr p);
	void DoCheckTopChunk(mstate m, mchunkptr p);
	void DoCheckInuseChunk(mstate m, mchunkptr p);
	void DoCheckFreeChunk(mstate m, mchunkptr p);
	void DoCheckMallocedChunk(mstate m, void* mem, TUint s);
	void DoCheckTree(mstate m, tchunkptr t);
	void DoCheckTreebin(mstate m, bindex_t i);
	void DoCheckSmallbin(mstate m, bindex_t i);
	TInt BinFind(mstate m, mchunkptr x);
	TUint TraverseAndCheck(mstate m);
	void DoCheckMallocState(mstate m);	
	
	TInt GetInfo(struct HeapInfo* i, SWalkInfo* wi=NULL) const;	
	void InitDlMalloc(TUint capacity, int locked);
	void* DlMalloc(TUint);
	void  DlFree(void*);
	void* DlRealloc(void*, TUint, TInt);
	TUint DlInfo(struct HeapInfo* i, SWalkInfo* wi) const;
	void DoCheckCommittedSize(TInt aNPages, mstate aM);	
	
	TAny* ReAllocImpl(TAny* aPtr, TInt aSize, TInt aMode);
	void Construct(TBool aSingleThread, TBool aDLOnly, TBool aUseAdjust, TInt aAlign);
#ifndef __KERNEL_MODE__	
	TInt ConstructLock(TUint32 aMode);
#endif	
	static void Walk(SWalkInfo* aInfo, TAny* aBfr, TInt aLth, TCellType aBfrType, TAllocatorType aAlloctorType);
	static void WalkCheckCell(TAny* aPtr, TCellType aType, TAny* aCell, TInt aLen);
	void* Map(void* p, TInt sz);
	void Unmap(void* p,TInt sz);

private:
	TInt iMinLength;
	TInt iOffset;				// offset of RHeap object from chunk base
	TInt iGrowBy;
	TInt iMinCell;
	TInt iPageSize;

	// Temporarily commented out and exported from RHeap to prevent source breaks from req417-52840.
	// This will be moved with another REQ after submission and subsequent fixing of bad code
	//TInt iNestingLevel;
	TInt iAllocCount;
	// Temporarily commented out.  See comment above regarding req417-52840 source breaks
	//TAllocFail iFailType;
	TInt iFailRate;
	TBool iFailed;
	TInt iFailAllocCount;
	TInt iRand;
	// Temporarily commented out.  See comment above regarding req417-52840 source breaks
	//TAny* iTestData;

	TInt iChunkSize;				
	TInt iHighWaterMark;
	TBool iUseAdjust;
	TBool iDLOnly;

	malloc_state iGlobalMallocState;
	
#ifdef __KERNEL_MODE__
	
	friend class RHeapK;
	
#else
	
	friend class UserHeap;
	friend class HybridHeap;
	friend class TestHybridHeap;
	
private:

	static void TreeRemove(slab* s);
	static void TreeInsert(slab* s,slab** r);

	enum {EOkBits = (1<<(MAXSLABSIZE>>2))-1};
	
	void SlabInit();
	void SlabConfig(unsigned slabbitmap);
	void* SlabAllocate(slabset& allocator);
	void SlabFree(void* p);
	void* AllocNewSlab(slabset& allocator);
	void* AllocNewPage(slabset& allocator);
	void* InitNewSlab(slabset& allocator, slab* s);
	void FreeSlab(slab* s);
	void FreePage(page* p);
	void SlabInfo(struct HeapInfo* i, SWalkInfo* wi) const;
	static void SlabFullInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi);
	static void SlabPartialInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi);
	static void SlabEmptyInfo(slab* s, struct HeapInfo* i, SWalkInfo* wi);
	static void TreeWalk(slab* const* root, void (*f)(slab*, struct HeapInfo*, SWalkInfo*), struct HeapInfo* i, SWalkInfo* wi);

	static void WalkPartialFullSlab(SWalkInfo* aInfo, slab* aSlab, TCellType aBfrType, TInt aLth);
	static void WalkFullSlab(SWalkInfo* aInfo, slab* aSlab, TCellType aBfrType, TInt aLth);
	void DoCheckSlab(slab* aSlab, TAllocatorType aSlabType, TAny* aBfr=NULL);
	void DoCheckSlabTrees();	
	void DoCheckSlabTree(slab** aS, TBool aPartialPage);
	void BuildPartialSlabBitmap(TUint32* aBitmap, slab* aSlab, TAny* aBfr=NULL);
	
	static inline unsigned SlabHeaderFree(unsigned h) 
	{return (h&0x000000ff);}
	static inline unsigned SlabHeaderPagemap(unsigned h) 
	{return (h&0x00000f00)>>8;}
	static inline unsigned SlabHeaderSize(unsigned h) 
	{return (h&0x0003f000)>>12;}
	static inline unsigned SlabHeaderUsedm4(unsigned h) 
	{return (h&0x0ffc0000)>>18;}
	/***paged allocator code***/
	void PagedInit(TInt aPagePower);
	void* PagedAllocate(unsigned size);
	void PagedFree(void* p);	
	void* PagedReallocate(void* p, unsigned size, TInt mode);
	
	bool PagedEncode(unsigned pos, unsigned npage);
	unsigned PagedDecode(unsigned pos) const;
	inline unsigned PagedSize(void* p) const;
	inline bool PagedSetSize(void* p, unsigned size);
	inline void PagedZapSize(void* p, unsigned size);
	inline void* Bitmap2addr(unsigned pos) const;
	void PagedInfo(struct HeapInfo* i, SWalkInfo* wi) const;
	void ResetBitmap();
	TBool CheckBitmap(void* aBfr, TInt aSize, TUint32& aDummy, TInt& aNPages);	
	
private:
	paged_bitmap iPageMap;							// bitmap representing page allocator's pages
	TUint8*		iMemBase;							// bottom of paged/slab memory (chunk base)
	TUint8		iBitMapBuffer[MAXSMALLPAGEBITS>>3];	// buffer for initial page bitmap
	TInt		iSlabThreshold;						// allocations < than this are done by the slab allocator
	TInt		iPageThreshold;						// 2^n is smallest cell size allocated in paged allocator
	TInt		iSlabInitThreshold;					// slab allocator will be used after chunk reaches this size
	TUint32		iSlabConfigBits;					// set of bits that specify which slab sizes to use
	slab*		iPartialPage;						// partial-use page tree
	slab*		iFullSlab;							// full slabs list (so we can find them when walking)
	page*		iSparePage;							// cached, to avoid kernel exec calls for unmapping/remapping
	TUint8		iSizeMap[(MAXSLABSIZE>>2)+1];		// index of slabset indexes based on size class
	slabset		iSlabAlloc[MAXSLABSIZE>>2];			// array of pointers to slabsets

#endif // __KERNEL_MODE__	
};

#define HEAP_ASSERT(x) __ASSERT_DEBUG(x, HEAP_PANIC(ETHeapBadCellAddress))

template <class T> inline T Floor(const T addr, unsigned aln)
{return T((unsigned(addr))&~(aln-1));}
template <class T> inline T Ceiling(T addr, unsigned aln)
{return T((unsigned(addr)+(aln-1))&~(aln-1));}
template <class T> inline unsigned LowBits(T addr, unsigned aln)
{return unsigned(addr)&(aln-1);}
template <class T1, class T2> inline int PtrDiff(const T1* a1, const T2* a2)
{return reinterpret_cast<const unsigned char*>(a1) - reinterpret_cast<const unsigned char*>(a2);}
template <class T> inline T Offset(T addr, unsigned ofs)
{return T(unsigned(addr)+ofs);}

#endif //__HEAP_HYBRID_H__
