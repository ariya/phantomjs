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

#ifndef __DLA__
#define __DLA__

#define DEFAULT_TRIM_THRESHOLD ((size_t)4U * (size_t)1024U)

#define MSPACES 0
#define HAVE_MORECORE 1
#define	MORECORE_CONTIGUOUS 1
#define	HAVE_MMAP 0
#define HAVE_MREMAP 0
#define DEFAULT_GRANULARITY (4096U)
#define FOOTERS 0
#define USE_LOCKS 0
#define INSECURE 1
#define NO_MALLINFO 0

#define LACKS_SYS_TYPES_H
#ifndef LACKS_SYS_TYPES_H
#include <sys/types.h>  /* For size_t */
#else
#ifndef _SIZE_T_DECLARED
typedef unsigned int size_t;
#define	_SIZE_T_DECLARED
#endif
#endif  /* LACKS_SYS_TYPES_H */

/* The maximum possible size_t value has all bits set */
#define MAX_SIZE_T           (~(size_t)0)

#ifndef ONLY_MSPACES
	#define ONLY_MSPACES 0
#endif  /* ONLY_MSPACES */

#ifndef MSPACES
	#if ONLY_MSPACES
		#define MSPACES 1
	#else   /* ONLY_MSPACES */
		#define MSPACES 0
	#endif  /* ONLY_MSPACES */
#endif  /* MSPACES */

//#ifndef MALLOC_ALIGNMENT
//	#define MALLOC_ALIGNMENT ((size_t)8U)
//#endif  /* MALLOC_ALIGNMENT */

#ifndef FOOTERS
	#define FOOTERS 0
#endif  /* FOOTERS */

#ifndef ABORT
//	#define ABORT  abort()
//	#define ABORT  User::Invariant()// redefined so euser isn't dependant on oe
  	#define ABORT  HEAP_PANIC(ETHeapBadCellAddress)
#endif  /* ABORT */

#ifndef PROCEED_ON_ERROR
	#define PROCEED_ON_ERROR 0
#endif  /* PROCEED_ON_ERROR */

#ifndef USE_LOCKS
	#define USE_LOCKS 0
#endif  /* USE_LOCKS */

#ifndef INSECURE
	#define INSECURE 0
#endif  /* INSECURE */

#ifndef HAVE_MMAP
	#define HAVE_MMAP 1
#endif  /* HAVE_MMAP */

#ifndef MMAP_CLEARS
	#define MMAP_CLEARS 1
#endif  /* MMAP_CLEARS */

#ifndef HAVE_MREMAP
	#ifdef linux
		#define HAVE_MREMAP 1
	#else   /* linux */
		#define HAVE_MREMAP 0
	#endif  /* linux */
#endif  /* HAVE_MREMAP */

#ifndef MALLOC_FAILURE_ACTION
	//#define MALLOC_FAILURE_ACTION  errno = ENOMEM;
	#define MALLOC_FAILURE_ACTION ;
#endif  /* MALLOC_FAILURE_ACTION */

#ifndef HAVE_MORECORE
	#if ONLY_MSPACES
		#define HAVE_MORECORE 1 /*AMOD: has changed */
	#else   /* ONLY_MSPACES */
		#define HAVE_MORECORE 1
	#endif  /* ONLY_MSPACES */
#endif  /* HAVE_MORECORE */

#if !HAVE_MORECORE
	#define MORECORE_CONTIGUOUS 0
#else   /* !HAVE_MORECORE */
	#ifndef MORECORE
		#define MORECORE DLAdjust
	#endif  /* MORECORE */
	#ifndef MORECORE_CONTIGUOUS
		#define MORECORE_CONTIGUOUS 0
	#endif  /* MORECORE_CONTIGUOUS */
#endif  /* !HAVE_MORECORE */

#ifndef DEFAULT_GRANULARITY
	#if MORECORE_CONTIGUOUS
		#define DEFAULT_GRANULARITY 4096  /* 0 means to compute in init_mparams */
	#else   /* MORECORE_CONTIGUOUS */
		#define DEFAULT_GRANULARITY ((size_t)64U * (size_t)1024U)
	#endif  /* MORECORE_CONTIGUOUS */
#endif  /* DEFAULT_GRANULARITY */

#ifndef DEFAULT_TRIM_THRESHOLD
	#ifndef MORECORE_CANNOT_TRIM
		#define DEFAULT_TRIM_THRESHOLD ((size_t)2U * (size_t)1024U * (size_t)1024U)
	#else   /* MORECORE_CANNOT_TRIM */
		#define DEFAULT_TRIM_THRESHOLD MAX_SIZE_T
	#endif  /* MORECORE_CANNOT_TRIM */	
#endif  /* DEFAULT_TRIM_THRESHOLD */

#ifndef DEFAULT_MMAP_THRESHOLD
	#if HAVE_MMAP
		#define DEFAULT_MMAP_THRESHOLD ((size_t)256U * (size_t)1024U)
	#else   /* HAVE_MMAP */
		#define DEFAULT_MMAP_THRESHOLD MAX_SIZE_T
	#endif  /* HAVE_MMAP */
#endif  /* DEFAULT_MMAP_THRESHOLD */

#ifndef USE_BUILTIN_FFS
	#define USE_BUILTIN_FFS 0
#endif  /* USE_BUILTIN_FFS */

#ifndef USE_DEV_RANDOM
	#define USE_DEV_RANDOM 0
#endif  /* USE_DEV_RANDOM */

#ifndef NO_MALLINFO
	#define NO_MALLINFO 0
#endif  /* NO_MALLINFO */
#ifndef MALLINFO_FIELD_TYPE
	#define MALLINFO_FIELD_TYPE size_t
#endif  /* MALLINFO_FIELD_TYPE */

/*
  mallopt tuning options.  SVID/XPG defines four standard parameter
  numbers for mallopt, normally defined in malloc.h.  None of these
  are used in this malloc, so setting them has no effect. But this
  malloc does support the following options.
*/

#define M_TRIM_THRESHOLD     (-1)
#define M_GRANULARITY        (-2)
#define M_MMAP_THRESHOLD     (-3)

#if !NO_MALLINFO
/*
  This version of malloc supports the standard SVID/XPG mallinfo
  routine that returns a struct containing usage properties and
  statistics. It should work on any system that has a
  /usr/include/malloc.h defining struct mallinfo.  The main
  declaration needed is the mallinfo struct that is returned (by-copy)
  by mallinfo().  The malloinfo struct contains a bunch of fields that
  are not even meaningful in this version of malloc.  These fields are
  are instead filled by mallinfo() with other numbers that might be of
  interest.

  HAVE_USR_INCLUDE_MALLOC_H should be set if you have a
  /usr/include/malloc.h file that includes a declaration of struct
  mallinfo.  If so, it is included; else a compliant version is
  declared below.  These must be precisely the same for mallinfo() to
  work.  The original SVID version of this struct, defined on most
  systems with mallinfo, declares all fields as ints. But some others
  define as unsigned long. If your system defines the fields using a
  type of different width than listed here, you MUST #include your
  system version and #define HAVE_USR_INCLUDE_MALLOC_H.
*/

/* #define HAVE_USR_INCLUDE_MALLOC_H */

#ifdef HAVE_USR_INCLUDE_MALLOC_H
#include "/usr/include/malloc.h"
#else /* HAVE_USR_INCLUDE_MALLOC_H */

struct mallinfo {
  MALLINFO_FIELD_TYPE iArena;    /* non-mmapped space allocated from system */
  MALLINFO_FIELD_TYPE iOrdblks;  /* number of free chunks */
  MALLINFO_FIELD_TYPE iSmblks;   /* always 0 */
  MALLINFO_FIELD_TYPE iHblks;    /* always 0 */
  MALLINFO_FIELD_TYPE iHblkhd;   /* space in mmapped regions */
  MALLINFO_FIELD_TYPE iUsmblks;  /* maximum total allocated space */
  MALLINFO_FIELD_TYPE iFsmblks;  /* always 0 */
  MALLINFO_FIELD_TYPE iUordblks; /* total allocated space */
  MALLINFO_FIELD_TYPE iFordblks; /* total free space */
  MALLINFO_FIELD_TYPE iKeepcost; /* releasable (via malloc_trim) space */
  MALLINFO_FIELD_TYPE iCellCount;/* Number of chunks allocated*/
};

#endif /* HAVE_USR_INCLUDE_MALLOC_H */
#endif /* NO_MALLINFO */

#if MSPACES
	typedef void* mspace;
#endif /* MSPACES */

#if 0

#include <stdio.h>/* for printing in malloc_stats */

#ifndef LACKS_ERRNO_H
	#include <errno.h>       /* for MALLOC_FAILURE_ACTION */
#endif /* LACKS_ERRNO_H */

#if FOOTERS
	#include <time.h>        /* for iMagic initialization */
#endif /* FOOTERS */

#ifndef LACKS_STDLIB_H
	#include <stdlib.h>      /* for abort() */
#endif /* LACKS_STDLIB_H */

#if !defined(ASSERT)
#define ASSERT(x) __ASSERT_DEBUG(x, HEAP_PANIC(ETHeapBadCellAddress))
#endif
	
#ifndef LACKS_STRING_H
	#include <string.h>      /* for memset etc */
#endif  /* LACKS_STRING_H */

#if USE_BUILTIN_FFS
	#ifndef LACKS_STRINGS_H
		#include <strings.h>     /* for ffs */
	#endif /* LACKS_STRINGS_H */
#endif /* USE_BUILTIN_FFS */

#if HAVE_MMAP
	#ifndef LACKS_SYS_MMAN_H
		#include <sys/mman.h>    /* for mmap */
	#endif /* LACKS_SYS_MMAN_H */
	#ifndef LACKS_FCNTL_H
		#include <fcntl.h>
	#endif /* LACKS_FCNTL_H */
#endif /* HAVE_MMAP */

#if HAVE_MORECORE
	#ifndef LACKS_UNISTD_H
		#include <unistd.h>     /* for sbrk */
	extern void*     sbrk(size_t);
	#else /* LACKS_UNISTD_H */
		#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
			extern void*     sbrk(ptrdiff_t);			
			/*Amod sbrk is not defined in WIN32 need to check in symbian*/
		#endif /* FreeBSD etc */
	#endif /* LACKS_UNISTD_H */
#endif /* HAVE_MORECORE */

#endif

/*AMOD: For MALLOC_GETPAGESIZE*/
#if 0	// replaced with GET_PAGE_SIZE() defined in heap.cpp
#ifndef WIN32
	#ifndef MALLOC_GETPAGESIZE
		#ifdef _SC_PAGESIZE         /* some SVR4 systems omit an underscore */
			#ifndef _SC_PAGE_SIZE
				#define _SC_PAGE_SIZE _SC_PAGESIZE
			#endif
		#endif
		#ifdef _SC_PAGE_SIZE
			#define MALLOC_GETPAGESIZE sysconf(_SC_PAGE_SIZE)
		#else
			#if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
				extern size_t getpagesize();
				#define MALLOC_GETPAGESIZE getpagesize()
			#else
				#ifdef WIN32 /* use supplied emulation of getpagesize */
					#define MALLOC_GETPAGESIZE getpagesize()
				#else
					#ifndef LACKS_SYS_PARAM_H
						#include <sys/param.h>
					#endif
					#ifdef EXEC_PAGESIZE
						#define MALLOC_GETPAGESIZE EXEC_PAGESIZE
					#else
						#ifdef NBPG
							#ifndef CLSIZE
								#define MALLOC_GETPAGESIZE NBPG
							#else
								#define MALLOC_GETPAGESIZE (NBPG * CLSIZE)
							#endif
						#else
							#ifdef NBPC
								#define MALLOC_GETPAGESIZE NBPC
							#else
								#ifdef PAGESIZE
									#define MALLOC_GETPAGESIZE PAGESIZE
								#else /* just guess */
									#define MALLOC_GETPAGESIZE ((size_t)4096U)
								#endif
							#endif
						#endif
					#endif
				#endif
			#endif
		#endif
	#endif
#endif
#endif
/*AMOD: For MALLOC_GETPAGESIZE*/

/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define SIZE_T_SIZE         (sizeof(size_t))
#define SIZE_T_BITSIZE      (sizeof(size_t) << 3)

/* Some constants coerced to size_t */
/* Annoying but necessary to avoid errors on some plaftorms */
#define SIZE_T_ZERO         ((size_t)0)
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define FOUR_SIZE_T_SIZES   (SIZE_T_SIZE<<2)
#define SIX_SIZE_T_SIZES    (FOUR_SIZE_T_SIZES+TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T     (MAX_SIZE_T / 2U)

/* The bit mask value corresponding to MALLOC_ALIGNMENT */
#define CHUNK_ALIGN_MASK    (MALLOC_ALIGNMENT - SIZE_T_ONE)

/* True if address a has acceptable alignment */
//#define IS_ALIGNED(A)       (((size_t)((A)) & (CHUNK_ALIGN_MASK)) == 0)
#define IS_ALIGNED(A)       (((unsigned int)((A)) & (CHUNK_ALIGN_MASK)) == 0)

/* the number of bytes to offset an address to align it */
#define ALIGN_OFFSET(A)\
	((((size_t)(A) & CHUNK_ALIGN_MASK) == 0)? 0 :\
	((MALLOC_ALIGNMENT - ((size_t)(A) & CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK))
				
/* -------------------------- MMAP preliminaries ------------------------- */

/*
   If HAVE_MORECORE or HAVE_MMAP are false, we just define calls and
   checks to fail so compiler optimizer can delete code rather than
   using so many "#if"s.
*/


/* MORECORE and MMAP must return MFAIL on failure */
#define MFAIL                ((void*)(MAX_SIZE_T))
#define CMFAIL               ((TUint8*)(MFAIL)) /* defined for convenience */

#if !HAVE_MMAP
	#define IS_MMAPPED_BIT       (SIZE_T_ZERO)
	#define USE_MMAP_BIT         (SIZE_T_ZERO)
	#define CALL_MMAP(s)         MFAIL
	#define CALL_MUNMAP(a, s)    (-1)
	#define DIRECT_MMAP(s)       MFAIL
#else /* !HAVE_MMAP */
	#define IS_MMAPPED_BIT       (SIZE_T_ONE)
	#define USE_MMAP_BIT         (SIZE_T_ONE)
		#ifndef WIN32
			#define CALL_MUNMAP(a, s)    DLUMMAP((a),(s)) /*munmap((a), (s))*/
			#define MMAP_PROT            (PROT_READ|PROT_WRITE)
			#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
				#define MAP_ANONYMOUS        MAP_ANON
			#endif /* MAP_ANON */
			#ifdef MAP_ANONYMOUS       
				#define MMAP_FLAGS           (MAP_PRIVATE|MAP_ANONYMOUS)
				#define CALL_MMAP(s)         mmap(0, (s), MMAP_PROT, (int)MMAP_FLAGS, -1, 0)
			#else /* MAP_ANONYMOUS */
				/*
				   Nearly all versions of mmap support MAP_ANONYMOUS, so the following
				   is unlikely to be needed, but is supplied just in case.
				*/
				#define MMAP_FLAGS           (MAP_PRIVATE)
				//static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */
				#define CALL_MMAP(s) DLMMAP(s)
				/*#define CALL_MMAP(s) ((dev_zero_fd < 0) ? \
			           (dev_zero_fd = open("/dev/zero", O_RDWR), \
			            mmap(0, (s), MMAP_PROT, MMAP_FLAGS, dev_zero_fd, 0)) : \
			            mmap(0, (s), MMAP_PROT, MMAP_FLAGS, dev_zero_fd, 0))
			            */
				#define CALL_REMAP(a, s, d)    DLREMAP((a),(s),(d))
			#endif /* MAP_ANONYMOUS */
			#define DIRECT_MMAP(s)       CALL_MMAP(s)
		#else /* WIN32 */
			#define CALL_MMAP(s)         win32mmap(s)
			#define CALL_MUNMAP(a, s)    win32munmap((a), (s))
			#define DIRECT_MMAP(s)       win32direct_mmap(s)
		#endif /* WIN32 */
#endif /* HAVE_MMAP */

#if HAVE_MMAP && HAVE_MREMAP
	#define CALL_MREMAP(addr, osz, nsz, mv) mremap((addr), (osz), (nsz), (mv))
#else  /* HAVE_MMAP && HAVE_MREMAP */
	#define CALL_MREMAP(addr, osz, nsz, mv) MFAIL
#endif /* HAVE_MMAP && HAVE_MREMAP */

#if HAVE_MORECORE
	#define CALL_MORECORE(S)     SetBrk(S)
#else  /* HAVE_MORECORE */
	#define CALL_MORECORE(S)     MFAIL
#endif /* HAVE_MORECORE */

/* mstate bit set if continguous morecore disabled or failed */
#define USE_NONCONTIGUOUS_BIT (4U)

/* segment bit set in create_mspace_with_base */
#define EXTERN_BIT            (8U)


#if USE_LOCKS
/*
  When locks are defined, there are up to two global locks:
  * If HAVE_MORECORE, iMorecoreMutex protects sequences of calls to
    MORECORE.  In many cases sys_alloc requires two calls, that should
    not be interleaved with calls by other threads.  This does not
    protect against direct calls to MORECORE by other threads not
    using this lock, so there is still code to cope the best we can on
    interference.
  * iMagicInitMutex ensures that mparams.iMagic and other
    unique mparams values are initialized only once.
*/
	#ifndef WIN32
		/* By default use posix locks */
		#include <pthread.h>
		#define MLOCK_T pthread_mutex_t
		#define INITIAL_LOCK(l)      pthread_mutex_init(l, NULL)
		#define ACQUIRE_LOCK(l)      pthread_mutex_lock(l)
		#define RELEASE_LOCK(l)      pthread_mutex_unlock(l)
		
		#if HAVE_MORECORE
			//static MLOCK_T iMorecoreMutex = PTHREAD_MUTEX_INITIALIZER;
		#endif /* HAVE_MORECORE */
			//static MLOCK_T iMagicInitMutex = PTHREAD_MUTEX_INITIALIZER;
	#else /* WIN32 */
		#define MLOCK_T long
		#define INITIAL_LOCK(l)      *(l)=0
		#define ACQUIRE_LOCK(l)      win32_acquire_lock(l)
		#define RELEASE_LOCK(l)      win32_release_lock(l)
		#if HAVE_MORECORE
			static MLOCK_T iMorecoreMutex;
		#endif /* HAVE_MORECORE */
		static MLOCK_T iMagicInitMutex;
	#endif /* WIN32 */
	#define USE_LOCK_BIT               (2U)
#else  /* USE_LOCKS */
	#define USE_LOCK_BIT               (0U)
	#define INITIAL_LOCK(l)
#endif /* USE_LOCKS */

#if USE_LOCKS && HAVE_MORECORE
	#define ACQUIRE_MORECORE_LOCK(M)    ACQUIRE_LOCK((M->iMorecoreMutex)/*&iMorecoreMutex*/);
	#define RELEASE_MORECORE_LOCK(M)    RELEASE_LOCK((M->iMorecoreMutex)/*&iMorecoreMutex*/);
#else /* USE_LOCKS && HAVE_MORECORE */
	#define ACQUIRE_MORECORE_LOCK(M)
	#define RELEASE_MORECORE_LOCK(M)
#endif /* USE_LOCKS && HAVE_MORECORE */

#if USE_LOCKS
		/*Currently not suporting this*/				
	#define ACQUIRE_MAGIC_INIT_LOCK(M)  ACQUIRE_LOCK(((M)->iMagicInitMutex));
	//AMOD: changed #define ACQUIRE_MAGIC_INIT_LOCK()
	//#define RELEASE_MAGIC_INIT_LOCK()
	#define RELEASE_MAGIC_INIT_LOCK(M)  RELEASE_LOCK(((M)->iMagicInitMutex));
#else  /* USE_LOCKS */
	#define ACQUIRE_MAGIC_INIT_LOCK(M)
	#define RELEASE_MAGIC_INIT_LOCK(M)
#endif /* USE_LOCKS */

/*CHUNK representation*/
struct malloc_chunk {
  size_t               iPrevFoot;  /* Size of previous chunk (if free).  */
  size_t               iHead;       /* Size and inuse bits. */
  struct malloc_chunk* iFd;         /* double links -- used only if free. */
  struct malloc_chunk* iBk;
};

typedef struct malloc_chunk  mchunk;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk* sbinptr;  /* The type of bins of chunks */
typedef unsigned int bindex_t;         /* Described below */
typedef unsigned int binmap_t;         /* Described below */
typedef unsigned int flag_t;           /* The type of various bit flag sets */


/* ------------------- Chunks sizes and alignments ----------------------- */
#define MCHUNK_SIZE         (sizeof(mchunk))

//#if FOOTERS
//	#define CHUNK_OVERHEAD      (TWO_SIZE_T_SIZES)
//#else /* FOOTERS */
//	#define CHUNK_OVERHEAD      (SIZE_T_SIZE)
//#endif /* FOOTERS */

/* MMapped chunks need a second word of overhead ... */
#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
/* ... and additional padding for fake next-chunk at foot */
#define MMAP_FOOT_PAD       (FOUR_SIZE_T_SIZES)

/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE ((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* conversion from malloc headers to user pointers, and back */
#define CHUNK2MEM(p)        ((void*)((TUint8*)(p)       + TWO_SIZE_T_SIZES))
#define MEM2CHUNK(mem)      ((mchunkptr)((TUint8*)(mem) - TWO_SIZE_T_SIZES))
/* chunk associated with aligned address A */
#define ALIGN_AS_CHUNK(A)   (mchunkptr)((A) + ALIGN_OFFSET(CHUNK2MEM(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST         ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

/* pad request bytes into a usable size */
#define PAD_REQUEST(req) (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define REQUEST2SIZE(req) (((req) < MIN_REQUEST)? MIN_CHUNK_SIZE : PAD_REQUEST(req))

/* ------------------ Operations on iHead and foot fields ----------------- */

/*
  The iHead field of a chunk is or'ed with PINUSE_BIT when previous
  adjacent chunk in use, and or'ed with CINUSE_BIT if this chunk is in
  use. If the chunk was obtained with mmap, the iPrevFoot field has
  IS_MMAPPED_BIT set, otherwise holding the offset of the base of the
  mmapped region to the base of the chunk.
*/
#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD      (INUSE_BITS|SIZE_T_SIZE)

/* extraction of fields from iHead words */
#define CINUSE(p)           ((p)->iHead & CINUSE_BIT)
#define PINUSE(p)           ((p)->iHead & PINUSE_BIT)
#define CHUNKSIZE(p)        ((p)->iHead & ~(INUSE_BITS))

#define CLEAR_PINUSE(p)     ((p)->iHead &= ~PINUSE_BIT)
#define CLEAR_CINUSE(p)     ((p)->iHead &= ~CINUSE_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define CHUNK_PLUS_OFFSET(p, s)  ((mchunkptr)(((TUint8*)(p)) + (s)))
#define CHUNK_MINUS_OFFSET(p, s) ((mchunkptr)(((TUint8*)(p)) - (s)))

/* Ptr to next or previous physical malloc_chunk. */
#define NEXT_CHUNK(p) ((mchunkptr)( ((TUint8*)(p)) + ((p)->iHead & ~INUSE_BITS)))
#define PREV_CHUNK(p) ((mchunkptr)( ((TUint8*)(p)) - ((p)->iPrevFoot) ))

/* extract next chunk's PINUSE bit */
#define NEXT_PINUSE(p)  ((NEXT_CHUNK(p)->iHead) & PINUSE_BIT)

/* Get/set size at footer */
#define GET_FOOT(p, s)  (((mchunkptr)((TUint8*)(p) + (s)))->iPrevFoot)
#define SET_FOOT(p, s)  (((mchunkptr)((TUint8*)(p) + (s)))->iPrevFoot = (s))

/* Set size, PINUSE bit, and foot */
#define SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(p, s) ((p)->iHead = (s|PINUSE_BIT), SET_FOOT(p, s))

/* Set size, PINUSE bit, foot, and clear next PINUSE */
#define SET_FREE_WITH_PINUSE(p, s, n) (CLEAR_PINUSE(n), SET_SIZE_AND_PINUSE_OF_FREE_CHUNK(p, s))

#define IS_MMAPPED(p) (!((p)->iHead & PINUSE_BIT) && ((p)->iPrevFoot & IS_MMAPPED_BIT))

/* Get the internal overhead associated with chunk p */
#define OVERHEAD_FOR(p) (IS_MMAPPED(p)? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

/* Return true if malloced space is not necessarily cleared */
#if MMAP_CLEARS
	#define CALLOC_MUST_CLEAR(p) (!IS_MMAPPED(p))
#else /* MMAP_CLEARS */
	#define CALLOC_MUST_CLEAR(p) (1)
#endif /* MMAP_CLEARS */

/* ---------------------- Overlaid data structures ----------------------- */
struct malloc_tree_chunk {
  /* The first four fields must be compatible with malloc_chunk */
  size_t                    				iPrevFoot;
  size_t                    				iHead;
  struct malloc_tree_chunk*	iFd;
  struct malloc_tree_chunk*	iBk;

  struct malloc_tree_chunk* iChild[2];
  struct malloc_tree_chunk* iParent;
  bindex_t                  iIndex;
};

typedef struct malloc_tree_chunk  tchunk;
typedef struct malloc_tree_chunk* tchunkptr;
typedef struct malloc_tree_chunk* tbinptr; /* The type of bins of trees */

/* A little helper macro for trees */
#define LEFTMOST_CHILD(t) ((t)->iChild[0] != 0? (t)->iChild[0] : (t)->iChild[1])
/*Segment structur*/
//struct malloc_segment {
//  TUint8*        iBase;             /* base address */
//  size_t       iSize;             /* allocated size */
//};

#define IS_MMAPPED_SEGMENT(S)  ((S)->iSflags & IS_MMAPPED_BIT)
#define IS_EXTERN_SEGMENT(S)   ((S)->iSflags & EXTERN_BIT)

typedef struct malloc_segment  msegment;
typedef struct malloc_segment* msegmentptr;

/*Malloc State data structur*/

//#define NSMALLBINS        (32U)
//#define NTREEBINS         (32U)
#define SMALLBIN_SHIFT    (3U)
#define SMALLBIN_WIDTH    (SIZE_T_ONE << SMALLBIN_SHIFT)
#define TREEBIN_SHIFT     (8U)
#define MIN_LARGE_SIZE    (SIZE_T_ONE << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE    (MIN_LARGE_SIZE - SIZE_T_ONE)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

/*struct malloc_state {
	binmap_t	 iSmallMap;
	binmap_t	 iTreeMap;
	size_t		 iDvSize;
	size_t		 iTopSize;
	mchunkptr	iDv;
	mchunkptr	iTop;
	size_t		 iTrimCheck;
	mchunkptr	iSmallBins[(NSMALLBINS+1)*2];
	tbinptr		iTreeBins[NTREEBINS];
	msegment	iSeg;
	};*/
/*
struct malloc_state {
  binmap_t   iSmallMap;
  binmap_t   iTreeMap;
  size_t     iDvSize;
  size_t     iTopSize;
  TUint8*      iLeastAddr;
  mchunkptr  iDv;
  mchunkptr  iTop;
  size_t     iTrimCheck;
  size_t     iMagic;
  mchunkptr  iSmallBins[(NSMALLBINS+1)*2];
  tbinptr    iTreeBins[NTREEBINS];
  size_t     iFootprint;
  size_t     iMaxFootprint;
  flag_t     iMflags;
#if USE_LOCKS
  MLOCK_T    iMutex;
  MLOCK_T	iMagicInitMutex;  
  MLOCK_T	iMorecoreMutex;
#endif 
  msegment   iSeg;
};
*/
typedef struct malloc_state*    mstate;

/* ------------- Global malloc_state and malloc_params ------------------- */

/*
  malloc_params holds global properties, including those that can be
  dynamically set using mallopt. There is a single instance, mparams,
  initialized in init_mparams.
*/

struct malloc_params {
  size_t iMagic;
  size_t iPageSize;
  size_t iGranularity;
  size_t iMmapThreshold;
  size_t iTrimThreshold;
  flag_t iDefaultMflags;
#if USE_LOCKS
  MLOCK_T	iMagicInitMutex;  
#endif /* USE_LOCKS */
};

/* The global malloc_state used for all non-"mspace" calls */
/*AMOD: Need to check this as this will be the member of the class*/

//static struct malloc_state _gm_;
//#define GM                 (&_gm_)
 
//#define IS_GLOBAL(M)       ((M) == &_gm_)
/*AMOD: has changed*/
#define IS_GLOBAL(M)       ((M) == GM)
#define IS_INITIALIZED(M)  ((M)->iTop != 0)

/* -------------------------- system alloc setup ------------------------- */

/* Operations on iMflags */

#define USE_LOCK(M)           ((M)->iMflags &   USE_LOCK_BIT)
#define ENABLE_LOCK(M)        ((M)->iMflags |=  USE_LOCK_BIT)
#define DISABLE_LOCK(M)       ((M)->iMflags &= ~USE_LOCK_BIT)

#define USE_MMAP(M)           ((M)->iMflags &   USE_MMAP_BIT)
#define ENABLE_MMAP(M)        ((M)->iMflags |=  USE_MMAP_BIT)
#define DISABLE_MMAP(M)       ((M)->iMflags &= ~USE_MMAP_BIT)

#define USE_NONCONTIGUOUS(M)  ((M)->iMflags &   USE_NONCONTIGUOUS_BIT)
#define DISABLE_CONTIGUOUS(M) ((M)->iMflags |=  USE_NONCONTIGUOUS_BIT)

#define SET_LOCK(M,L) ((M)->iMflags = (L)? ((M)->iMflags | USE_LOCK_BIT) :  ((M)->iMflags & ~USE_LOCK_BIT))

/* page-align a size */
#define PAGE_ALIGN(S) (((S) + (mparams.iPageSize)) & ~(mparams.iPageSize - SIZE_T_ONE))

/* iGranularity-align a size */
#define GRANULARITY_ALIGN(S)  (((S) + (mparams.iGranularity)) & ~(mparams.iGranularity - SIZE_T_ONE))

#define IS_PAGE_ALIGNED(S)   (((size_t)(S) & (mparams.iPageSize - SIZE_T_ONE)) == 0)
#define IS_GRANULARITY_ALIGNED(S)   (((size_t)(S) & (mparams.iGranularity - SIZE_T_ONE)) == 0)

/*  True if segment S holds address A */
#define SEGMENT_HOLDS(S, A)  ((TUint8*)(A) >= S->iBase && (TUint8*)(A) < S->iBase + S->iSize)

#ifndef MORECORE_CANNOT_TRIM
	#define SHOULD_TRIM(M,s)  ((s) > (M)->iTrimCheck)
#else  /* MORECORE_CANNOT_TRIM */
	#define SHOULD_TRIM(M,s)  (0)
#endif /* MORECORE_CANNOT_TRIM */

/*
  TOP_FOOT_SIZE is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
#define TOP_FOOT_SIZE  (ALIGN_OFFSET(CHUNK2MEM(0))+PAD_REQUEST(sizeof(struct malloc_segment))+MIN_CHUNK_SIZE)

#define SYS_ALLOC_PADDING (TOP_FOOT_SIZE + MALLOC_ALIGNMENT)
/* -------------------------------  Hooks -------------------------------- */

/*
  PREACTION should be defined to return 0 on success, and nonzero on
  failure. If you are not using locking, you can redefine these to do
  anything you like.
*/

#if USE_LOCKS
	/* Ensure locks are initialized */
	#define GLOBALLY_INITIALIZE() (mparams.iPageSize == 0 && init_mparams())
	#define PREACTION(M) (USE_LOCK((M))?(ACQUIRE_LOCK((M)->iMutex),0):0) /*Action to take like lock before alloc*/
	#define POSTACTION(M) { if (USE_LOCK(M)) RELEASE_LOCK((M)->iMutex); }

#else /* USE_LOCKS */
	#ifndef PREACTION
		#define PREACTION(M) (0)
	#endif  /* PREACTION */
	#ifndef POSTACTION
		#define POSTACTION(M)
	#endif  /* POSTACTION */
#endif /* USE_LOCKS */

/*
  CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses.
  USAGE_ERROR_ACTION is triggered on detected bad frees and
  reallocs. The argument p is an address that might have triggered the
  fault. It is ignored by the two predefined actions, but might be
  useful in custom actions that try to help diagnose errors.
*/

#if PROCEED_ON_ERROR
	/* A count of the number of corruption errors causing resets */
	int malloc_corruption_error_count;
	/* default corruption action */
	static void ResetOnError(mstate m);
	#define CORRUPTION_ERROR_ACTION(m)  ResetOnError(m)
	#define USAGE_ERROR_ACTION(m, p)
#else /* PROCEED_ON_ERROR */
	#ifndef CORRUPTION_ERROR_ACTION
		#define CORRUPTION_ERROR_ACTION(m) ABORT
	#endif /* CORRUPTION_ERROR_ACTION */
	#ifndef USAGE_ERROR_ACTION
		#define USAGE_ERROR_ACTION(m,p) ABORT
	#endif /* USAGE_ERROR_ACTION */
#endif /* PROCEED_ON_ERROR */


#ifdef _DEBUG
	#define CHECK_FREE_CHUNK(M,P)       DoCheckFreeChunk(M,P)
	#define CHECK_INUSE_CHUNK(M,P)      DoCheckInuseChunk(M,P)
	#define CHECK_TOP_CHUNK(M,P)        DoCheckTopChunk(M,P)
	#define CHECK_MALLOCED_CHUNK(M,P,N) DoCheckMallocedChunk(M,P,N)
	#define CHECK_MMAPPED_CHUNK(M,P)    DoCheckMmappedChunk(M,P)
	#define CHECK_MALLOC_STATE(M)       DoCheckMallocState(M)
#else /* DEBUG */
	#define CHECK_FREE_CHUNK(M,P)
	#define CHECK_INUSE_CHUNK(M,P)
	#define CHECK_MALLOCED_CHUNK(M,P,N)
	#define CHECK_MMAPPED_CHUNK(M,P)
	#define CHECK_MALLOC_STATE(M)
	#define CHECK_TOP_CHUNK(M,P)
#endif /* DEBUG */

/* ---------------------------- Indexing Bins ---------------------------- */

#define IS_SMALL(s)         (((s) >> SMALLBIN_SHIFT) < NSMALLBINS)
#define SMALL_INDEX(s)      ((s)  >> SMALLBIN_SHIFT)
#define SMALL_INDEX2SIZE(i) ((i)  << SMALLBIN_SHIFT)
#define MIN_SMALL_INDEX     (SMALL_INDEX(MIN_CHUNK_SIZE))

/* addressing by index. See above about smallbin repositioning */
#define SMALLBIN_AT(M, i)   ((sbinptr)((TUint8*)&((M)->iSmallBins[(i)<<1])))
#define TREEBIN_AT(M,i)     (&((M)->iTreeBins[i]))


/* Bit representing maximum resolved size in a treebin at i */
#define BIT_FOR_TREE_INDEX(i) (i == NTREEBINS-1)? (SIZE_T_BITSIZE-1) : (((i) >> 1) + TREEBIN_SHIFT - 2)

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define LEFTSHIFT_FOR_TREE_INDEX(i) ((i == NTREEBINS-1)? 0 : ((SIZE_T_BITSIZE-SIZE_T_ONE) - (((i) >> 1) + TREEBIN_SHIFT - 2)))

/* The size of the smallest chunk held in bin with index i */
#define MINSIZE_FOR_TREE_INDEX(i) ((SIZE_T_ONE << (((i) >> 1) + TREEBIN_SHIFT)) |  (((size_t)((i) & SIZE_T_ONE)) << (((i) >> 1) + TREEBIN_SHIFT - 1)))


/* ------------------------ Operations on bin maps ----------------------- */
/* bit corresponding to given index */
#define IDX2BIT(i)              ((binmap_t)(1) << (i))
/* Mark/Clear bits with given index */
#define MARK_SMALLMAP(M,i)      ((M)->iSmallMap |=  IDX2BIT(i))
#define CLEAR_SMALLMAP(M,i)     ((M)->iSmallMap &= ~IDX2BIT(i))
#define SMALLMAP_IS_MARKED(M,i) ((M)->iSmallMap &   IDX2BIT(i))
#define MARK_TREEMAP(M,i)       ((M)->iTreeMap  |=  IDX2BIT(i))
#define CLEAR_TREEMAP(M,i)      ((M)->iTreeMap  &= ~IDX2BIT(i))
#define TREEMAP_IS_MARKED(M,i)  ((M)->iTreeMap  &   IDX2BIT(i))

	/* isolate the least set bit of a bitmap */
#define LEAST_BIT(x)         ((x) & -(x))

/* mask with all bits to left of least bit of x on */
#define LEFT_BITS(x)         ((x<<1) | -(x<<1))

/* mask with all bits to left of or equal to least bit of x on */
#define SAME_OR_LEFT_BITS(x) ((x) | -(x))

#if !INSECURE
	/* Check if address a is at least as high as any from MORECORE or MMAP */
	#define OK_ADDRESS(M, a) ((TUint8*)(a) >= (M)->iLeastAddr)
	/* Check if address of next chunk n is higher than base chunk p */
	#define OK_NEXT(p, n)    ((TUint8*)(p) < (TUint8*)(n))
	/* Check if p has its CINUSE bit on */
	#define OK_CINUSE(p)     CINUSE(p)
	/* Check if p has its PINUSE bit on */
	#define OK_PINUSE(p)     PINUSE(p)
#else /* !INSECURE */
	#define OK_ADDRESS(M, a) (1)
	#define OK_NEXT(b, n)    (1)
	#define OK_CINUSE(p)     (1)
	#define OK_PINUSE(p)     (1)
#endif /* !INSECURE */

#if (FOOTERS && !INSECURE)
	/* Check if (alleged) mstate m has expected iMagic field */
	#define OK_MAGIC(M)      ((M)->iMagic == mparams.iMagic)
#else  /* (FOOTERS && !INSECURE) */
	#define OK_MAGIC(M)      (1)
#endif /* (FOOTERS && !INSECURE) */

/* In gcc, use __builtin_expect to minimize impact of checks */
#if !INSECURE
	#if defined(__GNUC__) && __GNUC__ >= 3
		#define RTCHECK(e)  __builtin_expect(e, 1)
	#else /* GNUC */
		#define RTCHECK(e)  (e)
	#endif /* GNUC */

#else /* !INSECURE */
	#define RTCHECK(e)  (1)
#endif /* !INSECURE */
/* macros to set up inuse chunks with or without footers */
#if !FOOTERS
	#define MARK_INUSE_FOOT(M,p,s)
	/* Set CINUSE bit and PINUSE bit of next chunk */
	#define SET_INUSE(M,p,s)  ((p)->iHead = (((p)->iHead & PINUSE_BIT)|s|CINUSE_BIT),((mchunkptr)(((TUint8*)(p)) + (s)))->iHead |= PINUSE_BIT)
	/* Set CINUSE and PINUSE of this chunk and PINUSE of next chunk */
	#define SET_INUSE_AND_PINUSE(M,p,s) ((p)->iHead = (s|PINUSE_BIT|CINUSE_BIT),((mchunkptr)(((TUint8*)(p)) + (s)))->iHead |= PINUSE_BIT)
	/* Set size, CINUSE and PINUSE bit of this chunk */
	#define SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(M, p, s) ((p)->iHead = (s|PINUSE_BIT|CINUSE_BIT))
#else /* FOOTERS */
	/* Set foot of inuse chunk to be xor of mstate and seed */
	#define MARK_INUSE_FOOT(M,p,s) (((mchunkptr)((TUint8*)(p) + (s)))->iPrevFoot = ((size_t)(M) ^ mparams.iMagic))
	#define GET_MSTATE_FOR(p) ((mstate)(((mchunkptr)((TUint8*)(p)+(CHUNKSIZE(p))))->iPrevFoot ^ mparams.iMagic))
	#define SET_INUSE(M,p,s)\
		((p)->iHead = (((p)->iHead & PINUSE_BIT)|s|CINUSE_BIT),\
		(((mchunkptr)(((TUint8*)(p)) + (s)))->iHead |= PINUSE_BIT), \
		MARK_INUSE_FOOT(M,p,s))
	#define SET_INUSE_AND_PINUSE(M,p,s)\
	((p)->iHead = (s|PINUSE_BIT|CINUSE_BIT),\
	(((mchunkptr)(((TUint8*)(p)) + (s)))->iHead |= PINUSE_BIT),\
	MARK_INUSE_FOOT(M,p,s))
	#define SET_SIZE_AND_PINUSE_OF_INUSE_CHUNK(M, p, s)\
	((p)->iHead = (s|PINUSE_BIT|CINUSE_BIT),\
	MARK_INUSE_FOOT(M, p, s))
#endif /* !FOOTERS */


#if ONLY_MSPACES
#define INTERNAL_MALLOC(m, b) mspace_malloc(m, b)
#define INTERNAL_FREE(m, mem) mspace_free(m,mem);
#else /* ONLY_MSPACES */
	#if MSPACES
		#define INTERNAL_MALLOC(m, b) (m == GM)? dlmalloc(b) : mspace_malloc(m, b)
		#define INTERNAL_FREE(m, mem) if (m == GM) dlfree(mem); else mspace_free(m,mem);
	#else /* MSPACES */
		#define INTERNAL_MALLOC(m, b) dlmalloc(b)
		#define INTERNAL_FREE(m, mem) dlfree(mem)
	#endif /* MSPACES */
#endif /* ONLY_MSPACES */
	
	#ifndef NDEBUG
	#define CHECKING 1
	#endif
//  #define HYSTERESIS 4
    #define HYSTERESIS 1	
	#define HYSTERESIS_BYTES (2*PAGESIZE)
    #define HYSTERESIS_GROW (HYSTERESIS*PAGESIZE)	
	
	#if CHECKING
	#define CHECK(x) x
	#else
	#undef ASSERT
	#define ASSERT(x) (void)0
	#define CHECK(x) (void)0
	#endif
	
#endif/*__DLA__*/
