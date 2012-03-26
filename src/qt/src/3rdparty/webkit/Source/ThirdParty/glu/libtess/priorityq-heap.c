/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/
/*
** Author: Eric Veach, July 1994.
**
** $Date$ $Revision$
** $Header: //depot/main/gfx/lib/glu/libtess/priorityq-heap.c#5 $
*/

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include "ThirdParty/glu/libtess/memalloc.h"
#include "ThirdParty/glu/libtess/priorityq-heap.h"

#define INIT_SIZE	32

#define TRUE 1
#define FALSE 0

#ifdef FOR_TRITE_TEST_PROGRAM
#define LEQ(x,y)	(*pq->leq)(x,y)
#else
/* Violates modularity, but a little faster */
#include "geom.h"
#define LEQ(x,y)	VertLeq((GLUvertex *)x, (GLUvertex *)y)
#endif

/* really __gl_pqHeapNewPriorityQ */
PriorityQ *pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQ *pq = (PriorityQ *)memAlloc( sizeof( PriorityQ ));
  if (pq == NULL) return NULL;

  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->nodes = (PQnode *)memAlloc( (INIT_SIZE + 1) * sizeof(pq->nodes[0]) );
  if (pq->nodes == NULL) {
     memFree(pq);
     return NULL;
  }

  pq->handles = (PQhandleElem *)memAlloc( (INIT_SIZE + 1) * sizeof(pq->handles[0]) );
  if (pq->handles == NULL) {
     memFree(pq->nodes);
     memFree(pq);
     return NULL;
  }

  pq->initialized = FALSE;
  pq->freeList = 0;
  pq->leq = leq;

  pq->nodes[1].handle = 1;	/* so that Minimum() returns NULL */
  pq->handles[1].key = NULL;
  return pq;
}

/* really __gl_pqHeapDeletePriorityQ */
void pqDeletePriorityQ( PriorityQ *pq )
{
  memFree( pq->handles );
  memFree( pq->nodes );
  memFree( pq );
}


static void FloatDown( PriorityQ *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hChild;
  long child;

  hCurr = n[curr].handle;
  for( ;; ) {
    child = curr << 1;
    if( child < pq->size && LEQ( h[n[child+1].handle].key,
				 h[n[child].handle].key )) {
      ++child;
    }

    assert(child <= pq->max);

    hChild = n[child].handle;
    if( child > pq->size || LEQ( h[hCurr].key, h[hChild].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hChild;
    h[hChild].node = curr;
    curr = child;
  }
}


static void FloatUp( PriorityQ *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hParent;
  long parent;

  hCurr = n[curr].handle;
  for( ;; ) {
    parent = curr >> 1;
    hParent = n[parent].handle;
    if( parent == 0 || LEQ( h[hParent].key, h[hCurr].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hParent;
    h[hParent].node = curr;
    curr = parent;
  }
}

/* really __gl_pqHeapInit */
void pqInit( PriorityQ *pq )
{
  long i;

  /* This method of building a heap is O(n), rather than O(n lg n). */

  for( i = pq->size; i >= 1; --i ) {
    FloatDown( pq, i );
  }
  pq->initialized = TRUE;
}

/* really __gl_pqHeapInsert */
/* returns LONG_MAX iff out of memory */
PQhandle pqInsert( PriorityQ *pq, PQkey keyNew )
{
  long curr;
  PQhandle free;

  curr = ++ pq->size;
  if( (curr*2) > pq->max ) {
    PQnode *saveNodes= pq->nodes;
    PQhandleElem *saveHandles= pq->handles;

    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->nodes = (PQnode *)memRealloc( pq->nodes, 
				     (size_t) 
				     ((pq->max + 1) * sizeof( pq->nodes[0] )));
    if (pq->nodes == NULL) {
       pq->nodes = saveNodes;	/* restore ptr to free upon return */
       return LONG_MAX;
    }
    pq->handles = (PQhandleElem *)memRealloc( pq->handles,
			                     (size_t)
			                      ((pq->max + 1) * 
					       sizeof( pq->handles[0] )));
    if (pq->handles == NULL) {
       pq->handles = saveHandles; /* restore ptr to free upon return */
       return LONG_MAX;
    }
  }

  if( pq->freeList == 0 ) {
    free = curr;
  } else {
    free = pq->freeList;
    pq->freeList = pq->handles[free].node;
  }

  pq->nodes[curr].handle = free;
  pq->handles[free].node = curr;
  pq->handles[free].key = keyNew;

  if( pq->initialized ) {
    FloatUp( pq, curr );
  }
  assert(free != LONG_MAX);
  return free;
}

/* really __gl_pqHeapExtractMin */
PQkey pqExtractMin( PriorityQ *pq )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hMin = n[1].handle;
  PQkey min = h[hMin].key;

  if( pq->size > 0 ) {
    n[1].handle = n[pq->size].handle;
    h[n[1].handle].node = 1;

    h[hMin].key = NULL;
    h[hMin].node = pq->freeList;
    pq->freeList = hMin;

    if( -- pq->size > 0 ) {
      FloatDown( pq, 1 );
    }
  }
  return min;
}

/* really __gl_pqHeapDelete */
void pqDelete( PriorityQ *pq, PQhandle hCurr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  long curr;

  assert( hCurr >= 1 && hCurr <= pq->max && h[hCurr].key != NULL );

  curr = h[hCurr].node;
  n[curr].handle = n[pq->size].handle;
  h[n[curr].handle].node = curr;

  if( curr <= -- pq->size ) {
    if( curr <= 1 || LEQ( h[n[curr>>1].handle].key, h[n[curr].handle].key )) {
      FloatDown( pq, curr );
    } else {
      FloatUp( pq, curr );
    }
  }
  h[hCurr].key = NULL;
  h[hCurr].node = pq->freeList;
  pq->freeList = hCurr;
}
