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
** $Header: //depot/main/gfx/lib/glu/libtess/mesh.c#6 $
*/

#include <assert.h>
#include <stddef.h>
#include "ThirdParty/glu/gluos.h"
#include "ThirdParty/glu/libtess/memalloc.h"
#include "ThirdParty/glu/libtess/mesh.h"

#define TRUE 1
#define FALSE 0

static GLUvertex *allocVertex()
{
   return (GLUvertex *)memAlloc( sizeof( GLUvertex ));
}

static GLUface *allocFace()
{
   return (GLUface *)memAlloc( sizeof( GLUface ));
}

/************************ Utility Routines ************************/

/* Allocate and free half-edges in pairs for efficiency.
 * The *only* place that should use this fact is allocation/free.
 */
typedef struct { GLUhalfEdge e, eSym; } EdgePair;

/* MakeEdge creates a new pair of half-edges which form their own loop.
 * No vertex or face structures are allocated, but these must be assigned
 * before the current edge operation is completed.
 */
static GLUhalfEdge *MakeEdge( GLUhalfEdge *eNext )
{
  GLUhalfEdge *e;
  GLUhalfEdge *eSym;
  GLUhalfEdge *ePrev;
  EdgePair *pair = (EdgePair *)memAlloc( sizeof( EdgePair ));
  if (pair == NULL) return NULL;

  e = &pair->e;
  eSym = &pair->eSym;

  /* Make sure eNext points to the first edge of the edge pair */
  if( eNext->Sym < eNext ) { eNext = eNext->Sym; }

  /* Insert in circular doubly-linked list before eNext.
   * Note that the prev pointer is stored in Sym->next.
   */
  ePrev = eNext->Sym->next;
  eSym->next = ePrev;
  ePrev->Sym->next = e;
  e->next = eNext;
  eNext->Sym->next = eSym;

  e->Sym = eSym;
  e->Onext = e;
  e->Lnext = eSym;
  e->Org = NULL;
  e->Lface = NULL;
  e->winding = 0;
  e->activeRegion = NULL;

  eSym->Sym = e;
  eSym->Onext = eSym;
  eSym->Lnext = e;
  eSym->Org = NULL;
  eSym->Lface = NULL;
  eSym->winding = 0;
  eSym->activeRegion = NULL;

  return e;
}

/* Splice( a, b ) is best described by the Guibas/Stolfi paper or the
 * CS348a notes (see mesh.h).  Basically it modifies the mesh so that
 * a->Onext and b->Onext are exchanged.  This can have various effects
 * depending on whether a and b belong to different face or vertex rings.
 * For more explanation see __gl_meshSplice() below.
 */
static void Splice( GLUhalfEdge *a, GLUhalfEdge *b )
{
  GLUhalfEdge *aOnext = a->Onext;
  GLUhalfEdge *bOnext = b->Onext;

  aOnext->Sym->Lnext = b;
  bOnext->Sym->Lnext = a;
  a->Onext = bOnext;
  b->Onext = aOnext;
}

/* MakeVertex( newVertex, eOrig, vNext ) attaches a new vertex and makes it the
 * origin of all edges in the vertex loop to which eOrig belongs. "vNext" gives
 * a place to insert the new vertex in the global vertex list.  We insert
 * the new vertex *before* vNext so that algorithms which walk the vertex
 * list will not see the newly created vertices.
 */
static void MakeVertex( GLUvertex *newVertex, 
			GLUhalfEdge *eOrig, GLUvertex *vNext )
{
  GLUhalfEdge *e;
  GLUvertex *vPrev;
  GLUvertex *vNew = newVertex;

  assert(vNew != NULL);

  /* insert in circular doubly-linked list before vNext */
  vPrev = vNext->prev;
  vNew->prev = vPrev;
  vPrev->next = vNew;
  vNew->next = vNext;
  vNext->prev = vNew;

  vNew->anEdge = eOrig;
  vNew->data = NULL;
  /* leave coords, s, t undefined */

  /* fix other edges on this vertex loop */
  e = eOrig;
  do {
    e->Org = vNew;
    e = e->Onext;
  } while( e != eOrig );
}

/* MakeFace( newFace, eOrig, fNext ) attaches a new face and makes it the left
 * face of all edges in the face loop to which eOrig belongs.  "fNext" gives
 * a place to insert the new face in the global face list.  We insert
 * the new face *before* fNext so that algorithms which walk the face
 * list will not see the newly created faces.
 */
static void MakeFace( GLUface *newFace, GLUhalfEdge *eOrig, GLUface *fNext )
{
  GLUhalfEdge *e;
  GLUface *fPrev;
  GLUface *fNew = newFace;

  assert(fNew != NULL); 

  /* insert in circular doubly-linked list before fNext */
  fPrev = fNext->prev;
  fNew->prev = fPrev;
  fPrev->next = fNew;
  fNew->next = fNext;
  fNext->prev = fNew;

  fNew->anEdge = eOrig;
  fNew->data = NULL;
  fNew->trail = NULL;
  fNew->marked = FALSE;

  /* The new face is marked "inside" if the old one was.  This is a
   * convenience for the common case where a face has been split in two.
   */
  fNew->inside = fNext->inside;

  /* fix other edges on this face loop */
  e = eOrig;
  do {
    e->Lface = fNew;
    e = e->Lnext;
  } while( e != eOrig );
}

/* KillEdge( eDel ) destroys an edge (the half-edges eDel and eDel->Sym),
 * and removes from the global edge list.
 */
static void KillEdge( GLUhalfEdge *eDel )
{
  GLUhalfEdge *ePrev, *eNext;

  /* Half-edges are allocated in pairs, see EdgePair above */
  if( eDel->Sym < eDel ) { eDel = eDel->Sym; }

  /* delete from circular doubly-linked list */
  eNext = eDel->next;
  ePrev = eDel->Sym->next;
  eNext->Sym->next = ePrev;
  ePrev->Sym->next = eNext;

  memFree( eDel );
}


/* KillVertex( vDel ) destroys a vertex and removes it from the global
 * vertex list.  It updates the vertex loop to point to a given new vertex.
 */
static void KillVertex( GLUvertex *vDel, GLUvertex *newOrg )
{
  GLUhalfEdge *e, *eStart = vDel->anEdge;
  GLUvertex *vPrev, *vNext;

  /* change the origin of all affected edges */
  e = eStart;
  do {
    e->Org = newOrg;
    e = e->Onext;
  } while( e != eStart );

  /* delete from circular doubly-linked list */
  vPrev = vDel->prev;
  vNext = vDel->next;
  vNext->prev = vPrev;
  vPrev->next = vNext;

  memFree( vDel );
}

/* KillFace( fDel ) destroys a face and removes it from the global face
 * list.  It updates the face loop to point to a given new face.
 */
static void KillFace( GLUface *fDel, GLUface *newLface )
{
  GLUhalfEdge *e, *eStart = fDel->anEdge;
  GLUface *fPrev, *fNext;

  /* change the left face of all affected edges */
  e = eStart;
  do {
    e->Lface = newLface;
    e = e->Lnext;
  } while( e != eStart );

  /* delete from circular doubly-linked list */
  fPrev = fDel->prev;
  fNext = fDel->next;
  fNext->prev = fPrev;
  fPrev->next = fNext;

  memFree( fDel );
}


/****************** Basic Edge Operations **********************/

/* __gl_meshMakeEdge creates one edge, two vertices, and a loop (face).
 * The loop consists of the two new half-edges.
 */
GLUhalfEdge *__gl_meshMakeEdge( GLUmesh *mesh )
{
  GLUvertex *newVertex1= allocVertex();
  GLUvertex *newVertex2= allocVertex();
  GLUface *newFace= allocFace();
  GLUhalfEdge *e;

  /* if any one is null then all get freed */
  if (newVertex1 == NULL || newVertex2 == NULL || newFace == NULL) {
     if (newVertex1 != NULL) memFree(newVertex1);
     if (newVertex2 != NULL) memFree(newVertex2);
     if (newFace != NULL) memFree(newFace);     
     return NULL;
  } 

  e = MakeEdge( &mesh->eHead );
  if (e == NULL) return NULL;

  MakeVertex( newVertex1, e, &mesh->vHead );
  MakeVertex( newVertex2, e->Sym, &mesh->vHead );
  MakeFace( newFace, e, &mesh->fHead );
  return e;
}
  

/* __gl_meshSplice( eOrg, eDst ) is the basic operation for changing the
 * mesh connectivity and topology.  It changes the mesh so that
 *	eOrg->Onext <- OLD( eDst->Onext )
 *	eDst->Onext <- OLD( eOrg->Onext )
 * where OLD(...) means the value before the meshSplice operation.
 *
 * This can have two effects on the vertex structure:
 *  - if eOrg->Org != eDst->Org, the two vertices are merged together
 *  - if eOrg->Org == eDst->Org, the origin is split into two vertices
 * In both cases, eDst->Org is changed and eOrg->Org is untouched.
 *
 * Similarly (and independently) for the face structure,
 *  - if eOrg->Lface == eDst->Lface, one loop is split into two
 *  - if eOrg->Lface != eDst->Lface, two distinct loops are joined into one
 * In both cases, eDst->Lface is changed and eOrg->Lface is unaffected.
 *
 * Some special cases:
 * If eDst == eOrg, the operation has no effect.
 * If eDst == eOrg->Lnext, the new face will have a single edge.
 * If eDst == eOrg->Lprev, the old face will have a single edge.
 * If eDst == eOrg->Onext, the new vertex will have a single edge.
 * If eDst == eOrg->Oprev, the old vertex will have a single edge.
 */
int __gl_meshSplice( GLUhalfEdge *eOrg, GLUhalfEdge *eDst )
{
  int joiningLoops = FALSE;
  int joiningVertices = FALSE;

  if( eOrg == eDst ) return 1;

  if( eDst->Org != eOrg->Org ) {
    /* We are merging two disjoint vertices -- destroy eDst->Org */
    joiningVertices = TRUE;
    KillVertex( eDst->Org, eOrg->Org );
  }
  if( eDst->Lface != eOrg->Lface ) {
    /* We are connecting two disjoint loops -- destroy eDst->Lface */
    joiningLoops = TRUE;
    KillFace( eDst->Lface, eOrg->Lface );
  }

  /* Change the edge structure */
  Splice( eDst, eOrg );

  if( ! joiningVertices ) {
    GLUvertex *newVertex= allocVertex();
    if (newVertex == NULL) return 0;

    /* We split one vertex into two -- the new vertex is eDst->Org.
     * Make sure the old vertex points to a valid half-edge.
     */
    MakeVertex( newVertex, eDst, eOrg->Org );
    eOrg->Org->anEdge = eOrg;
  }
  if( ! joiningLoops ) {
    GLUface *newFace= allocFace();  
    if (newFace == NULL) return 0;

    /* We split one loop into two -- the new loop is eDst->Lface.
     * Make sure the old face points to a valid half-edge.
     */
    MakeFace( newFace, eDst, eOrg->Lface );
    eOrg->Lface->anEdge = eOrg;
  }

  return 1;
}


/* __gl_meshDelete( eDel ) removes the edge eDel.  There are several cases:
 * if (eDel->Lface != eDel->Rface), we join two loops into one; the loop
 * eDel->Lface is deleted.  Otherwise, we are splitting one loop into two;
 * the newly created loop will contain eDel->Dst.  If the deletion of eDel
 * would create isolated vertices, those are deleted as well.
 *
 * This function could be implemented as two calls to __gl_meshSplice
 * plus a few calls to memFree, but this would allocate and delete
 * unnecessary vertices and faces.
 */
int __gl_meshDelete( GLUhalfEdge *eDel )
{
  GLUhalfEdge *eDelSym = eDel->Sym;
  int joiningLoops = FALSE;

  /* First step: disconnect the origin vertex eDel->Org.  We make all
   * changes to get a consistent mesh in this "intermediate" state.
   */
  if( eDel->Lface != eDel->Rface ) {
    /* We are joining two loops into one -- remove the left face */
    joiningLoops = TRUE;
    KillFace( eDel->Lface, eDel->Rface );
  }

  if( eDel->Onext == eDel ) {
    KillVertex( eDel->Org, NULL );
  } else {
    /* Make sure that eDel->Org and eDel->Rface point to valid half-edges */
    eDel->Rface->anEdge = eDel->Oprev;
    eDel->Org->anEdge = eDel->Onext;

    Splice( eDel, eDel->Oprev );
    if( ! joiningLoops ) {
      GLUface *newFace= allocFace();
      if (newFace == NULL) return 0; 

      /* We are splitting one loop into two -- create a new loop for eDel. */
      MakeFace( newFace, eDel, eDel->Lface );
    }
  }

  /* Claim: the mesh is now in a consistent state, except that eDel->Org
   * may have been deleted.  Now we disconnect eDel->Dst.
   */
  if( eDelSym->Onext == eDelSym ) {
    KillVertex( eDelSym->Org, NULL );
    KillFace( eDelSym->Lface, NULL );
  } else {
    /* Make sure that eDel->Dst and eDel->Lface point to valid half-edges */
    eDel->Lface->anEdge = eDelSym->Oprev;
    eDelSym->Org->anEdge = eDelSym->Onext;
    Splice( eDelSym, eDelSym->Oprev );
  }

  /* Any isolated vertices or faces have already been freed. */
  KillEdge( eDel );

  return 1;
}


/******************** Other Edge Operations **********************/

/* All these routines can be implemented with the basic edge
 * operations above.  They are provided for convenience and efficiency.
 */


/* __gl_meshAddEdgeVertex( eOrg ) creates a new edge eNew such that
 * eNew == eOrg->Lnext, and eNew->Dst is a newly created vertex.
 * eOrg and eNew will have the same left face.
 */
GLUhalfEdge *__gl_meshAddEdgeVertex( GLUhalfEdge *eOrg )
{
  GLUhalfEdge *eNewSym;
  GLUhalfEdge *eNew = MakeEdge( eOrg );
  if (eNew == NULL) return NULL;

  eNewSym = eNew->Sym;

  /* Connect the new edge appropriately */
  Splice( eNew, eOrg->Lnext );

  /* Set the vertex and face information */
  eNew->Org = eOrg->Dst;
  {
    GLUvertex *newVertex= allocVertex();
    if (newVertex == NULL) return NULL;

    MakeVertex( newVertex, eNewSym, eNew->Org );
  }
  eNew->Lface = eNewSym->Lface = eOrg->Lface;

  return eNew;
}


/* __gl_meshSplitEdge( eOrg ) splits eOrg into two edges eOrg and eNew,
 * such that eNew == eOrg->Lnext.  The new vertex is eOrg->Dst == eNew->Org.
 * eOrg and eNew will have the same left face.
 */
GLUhalfEdge *__gl_meshSplitEdge( GLUhalfEdge *eOrg )
{
  GLUhalfEdge *eNew;
  GLUhalfEdge *tempHalfEdge= __gl_meshAddEdgeVertex( eOrg );
  if (tempHalfEdge == NULL) return NULL;

  eNew = tempHalfEdge->Sym;

  /* Disconnect eOrg from eOrg->Dst and connect it to eNew->Org */
  Splice( eOrg->Sym, eOrg->Sym->Oprev );
  Splice( eOrg->Sym, eNew );

  /* Set the vertex and face information */
  eOrg->Dst = eNew->Org;
  eNew->Dst->anEdge = eNew->Sym;	/* may have pointed to eOrg->Sym */
  eNew->Rface = eOrg->Rface;
  eNew->winding = eOrg->winding;	/* copy old winding information */
  eNew->Sym->winding = eOrg->Sym->winding;

  return eNew;
}


/* __gl_meshConnect( eOrg, eDst ) creates a new edge from eOrg->Dst
 * to eDst->Org, and returns the corresponding half-edge eNew.
 * If eOrg->Lface == eDst->Lface, this splits one loop into two,
 * and the newly created loop is eNew->Lface.  Otherwise, two disjoint
 * loops are merged into one, and the loop eDst->Lface is destroyed.
 *
 * If (eOrg == eDst), the new face will have only two edges.
 * If (eOrg->Lnext == eDst), the old face is reduced to a single edge.
 * If (eOrg->Lnext->Lnext == eDst), the old face is reduced to two edges.
 */
GLUhalfEdge *__gl_meshConnect( GLUhalfEdge *eOrg, GLUhalfEdge *eDst )
{
  GLUhalfEdge *eNewSym;
  int joiningLoops = FALSE;  
  GLUhalfEdge *eNew = MakeEdge( eOrg );
  if (eNew == NULL) return NULL;

  eNewSym = eNew->Sym;

  if( eDst->Lface != eOrg->Lface ) {
    /* We are connecting two disjoint loops -- destroy eDst->Lface */
    joiningLoops = TRUE;
    KillFace( eDst->Lface, eOrg->Lface );
  }

  /* Connect the new edge appropriately */
  Splice( eNew, eOrg->Lnext );
  Splice( eNewSym, eDst );

  /* Set the vertex and face information */
  eNew->Org = eOrg->Dst;
  eNewSym->Org = eDst->Org;
  eNew->Lface = eNewSym->Lface = eOrg->Lface;

  /* Make sure the old face points to a valid half-edge */
  eOrg->Lface->anEdge = eNewSym;

  if( ! joiningLoops ) {
    GLUface *newFace= allocFace();
    if (newFace == NULL) return NULL;

    /* We split one loop into two -- the new loop is eNew->Lface */
    MakeFace( newFace, eNew, eOrg->Lface );
  }
  return eNew;
}


/******************** Other Operations **********************/

/* __gl_meshZapFace( fZap ) destroys a face and removes it from the
 * global face list.  All edges of fZap will have a NULL pointer as their
 * left face.  Any edges which also have a NULL pointer as their right face
 * are deleted entirely (along with any isolated vertices this produces).
 * An entire mesh can be deleted by zapping its faces, one at a time,
 * in any order.  Zapped faces cannot be used in further mesh operations!
 */
void __gl_meshZapFace( GLUface *fZap )
{
  GLUhalfEdge *eStart = fZap->anEdge;
  GLUhalfEdge *e, *eNext, *eSym;
  GLUface *fPrev, *fNext;

  /* walk around face, deleting edges whose right face is also NULL */
  eNext = eStart->Lnext;
  do {
    e = eNext;
    eNext = e->Lnext;

    e->Lface = NULL;
    if( e->Rface == NULL ) {
      /* delete the edge -- see __gl_MeshDelete above */

      if( e->Onext == e ) {
	KillVertex( e->Org, NULL );
      } else {
	/* Make sure that e->Org points to a valid half-edge */
	e->Org->anEdge = e->Onext;
	Splice( e, e->Oprev );
      }
      eSym = e->Sym;
      if( eSym->Onext == eSym ) {
	KillVertex( eSym->Org, NULL );
      } else {
	/* Make sure that eSym->Org points to a valid half-edge */
	eSym->Org->anEdge = eSym->Onext;
	Splice( eSym, eSym->Oprev );
      }
      KillEdge( e );
    }
  } while( e != eStart );

  /* delete from circular doubly-linked list */
  fPrev = fZap->prev;
  fNext = fZap->next;
  fNext->prev = fPrev;
  fPrev->next = fNext;

  memFree( fZap );
}


/* __gl_meshNewMesh() creates a new mesh with no edges, no vertices,
 * and no loops (what we usually call a "face").
 */
GLUmesh *__gl_meshNewMesh( void )
{
  GLUvertex *v;
  GLUface *f;
  GLUhalfEdge *e;
  GLUhalfEdge *eSym;
  GLUmesh *mesh = (GLUmesh *)memAlloc( sizeof( GLUmesh ));
  if (mesh == NULL) {
     return NULL;
  }
  
  v = &mesh->vHead;
  f = &mesh->fHead;
  e = &mesh->eHead;
  eSym = &mesh->eHeadSym;

  v->next = v->prev = v;
  v->anEdge = NULL;
  v->data = NULL;

  f->next = f->prev = f;
  f->anEdge = NULL;
  f->data = NULL;
  f->trail = NULL;
  f->marked = FALSE;
  f->inside = FALSE;

  e->next = e;
  e->Sym = eSym;
  e->Onext = NULL;
  e->Lnext = NULL;
  e->Org = NULL;
  e->Lface = NULL;
  e->winding = 0;
  e->activeRegion = NULL;

  eSym->next = eSym;
  eSym->Sym = e;
  eSym->Onext = NULL;
  eSym->Lnext = NULL;
  eSym->Org = NULL;
  eSym->Lface = NULL;
  eSym->winding = 0;
  eSym->activeRegion = NULL;

  return mesh;
}


/* __gl_meshUnion( mesh1, mesh2 ) forms the union of all structures in
 * both meshes, and returns the new mesh (the old meshes are destroyed).
 */
GLUmesh *__gl_meshUnion( GLUmesh *mesh1, GLUmesh *mesh2 )
{
  GLUface *f1 = &mesh1->fHead;
  GLUvertex *v1 = &mesh1->vHead;
  GLUhalfEdge *e1 = &mesh1->eHead;
  GLUface *f2 = &mesh2->fHead;
  GLUvertex *v2 = &mesh2->vHead;
  GLUhalfEdge *e2 = &mesh2->eHead;

  /* Add the faces, vertices, and edges of mesh2 to those of mesh1 */
  if( f2->next != f2 ) {
    f1->prev->next = f2->next;
    f2->next->prev = f1->prev;
    f2->prev->next = f1;
    f1->prev = f2->prev;
  }

  if( v2->next != v2 ) {
    v1->prev->next = v2->next;
    v2->next->prev = v1->prev;
    v2->prev->next = v1;
    v1->prev = v2->prev;
  }

  if( e2->next != e2 ) {
    e1->Sym->next->Sym->next = e2->next;
    e2->next->Sym->next = e1->Sym->next;
    e2->Sym->next->Sym->next = e1;
    e1->Sym->next = e2->Sym->next;
  }

  memFree( mesh2 );
  return mesh1;
}


#ifdef DELETE_BY_ZAPPING

/* __gl_meshDeleteMesh( mesh ) will free all storage for any valid mesh.
 */
void __gl_meshDeleteMesh( GLUmesh *mesh )
{
  GLUface *fHead = &mesh->fHead;

  while( fHead->next != fHead ) {
    __gl_meshZapFace( fHead->next );
  }
  assert( mesh->vHead.next == &mesh->vHead );

  memFree( mesh );
}

#else

/* __gl_meshDeleteMesh( mesh ) will free all storage for any valid mesh.
 */
void __gl_meshDeleteMesh( GLUmesh *mesh )
{
  GLUface *f, *fNext;
  GLUvertex *v, *vNext;
  GLUhalfEdge *e, *eNext;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = fNext ) {
    fNext = f->next;
    memFree( f );
  }

  for( v = mesh->vHead.next; v != &mesh->vHead; v = vNext ) {
    vNext = v->next;
    memFree( v );
  }

  for( e = mesh->eHead.next; e != &mesh->eHead; e = eNext ) {
    /* One call frees both e and e->Sym (see EdgePair above) */
    eNext = e->next;
    memFree( e );
  }

  memFree( mesh );
}

#endif

#ifndef NDEBUG

/* __gl_meshCheckMesh( mesh ) checks a mesh for self-consistency.
 */
void __gl_meshCheckMesh( GLUmesh *mesh )
{
  GLUface *fHead = &mesh->fHead;
  GLUvertex *vHead = &mesh->vHead;
  GLUhalfEdge *eHead = &mesh->eHead;
  GLUface *f, *fPrev;
  GLUvertex *v, *vPrev;
  GLUhalfEdge *e, *ePrev;

  fPrev = fHead;
  for( fPrev = fHead ; (f = fPrev->next) != fHead; fPrev = f) {
    assert( f->prev == fPrev );
    e = f->anEdge;
    do {
      assert( e->Sym != e );
      assert( e->Sym->Sym == e );
      assert( e->Lnext->Onext->Sym == e );
      assert( e->Onext->Sym->Lnext == e );
      assert( e->Lface == f );
      e = e->Lnext;
    } while( e != f->anEdge );
  }
  assert( f->prev == fPrev && f->anEdge == NULL && f->data == NULL );

  vPrev = vHead;
  for( vPrev = vHead ; (v = vPrev->next) != vHead; vPrev = v) {
    assert( v->prev == vPrev );
    e = v->anEdge;
    do {
      assert( e->Sym != e );
      assert( e->Sym->Sym == e );
      assert( e->Lnext->Onext->Sym == e );
      assert( e->Onext->Sym->Lnext == e );
      assert( e->Org == v );
      e = e->Onext;
    } while( e != v->anEdge );
  }
  assert( v->prev == vPrev && v->anEdge == NULL && v->data == NULL );

  ePrev = eHead;
  for( ePrev = eHead ; (e = ePrev->next) != eHead; ePrev = e) {
    assert( e->Sym->next == ePrev->Sym );
    assert( e->Sym != e );
    assert( e->Sym->Sym == e );
    assert( e->Org != NULL );
    assert( e->Dst != NULL );
    assert( e->Lnext->Onext->Sym == e );
    assert( e->Onext->Sym->Lnext == e );
  }
  assert( e->Sym->next == ePrev->Sym
       && e->Sym == &mesh->eHeadSym
       && e->Sym->Sym == e
       && e->Org == NULL && e->Dst == NULL
       && e->Lface == NULL && e->Rface == NULL );
}

#endif
