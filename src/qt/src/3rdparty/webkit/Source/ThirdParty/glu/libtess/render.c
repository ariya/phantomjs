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
** $Header: //depot/main/gfx/lib/glu/libtess/render.c#5 $
*/

#include <assert.h>
#include <stddef.h>
#include "ThirdParty/glu/gluos.h"
#include "ThirdParty/glu/libtess/mesh.h"
#include "ThirdParty/glu/libtess/render.h"
#include "ThirdParty/glu/libtess/tess.h"

#define TRUE 1
#define FALSE 0

/* This structure remembers the information we need about a primitive
 * to be able to render it later, once we have determined which
 * primitive is able to use the most triangles.
 */
struct FaceCount {
  long		size;		/* number of triangles used */
  GLUhalfEdge	*eStart;	/* edge where this primitive starts */
  void		(*render)(GLUtesselator *, GLUhalfEdge *, long);
                                /* routine to render this primitive */
};

static struct FaceCount MaximumFan( GLUhalfEdge *eOrig );
static struct FaceCount MaximumStrip( GLUhalfEdge *eOrig );

static void RenderFan( GLUtesselator *tess, GLUhalfEdge *eStart, long size );
static void RenderStrip( GLUtesselator *tess, GLUhalfEdge *eStart, long size );
static void RenderTriangle( GLUtesselator *tess, GLUhalfEdge *eStart,
			    long size );

static void RenderMaximumFaceGroup( GLUtesselator *tess, GLUface *fOrig );
static void RenderLonelyTriangles( GLUtesselator *tess, GLUface *head );



/************************ Strips and Fans decomposition ******************/

/* __gl_renderMesh( tess, mesh ) takes a mesh and breaks it into triangle
 * fans, strips, and separate triangles.  A substantial effort is made
 * to use as few rendering primitives as possible (ie. to make the fans
 * and strips as large as possible).
 *
 * The rendering output is provided as callbacks (see the api).
 */
void __gl_renderMesh( GLUtesselator *tess, GLUmesh *mesh )
{
  GLUface *f;

  /* Make a list of separate triangles so we can render them all at once */
  tess->lonelyTriList = NULL;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
    f->marked = FALSE;
  }
  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {

    /* We examine all faces in an arbitrary order.  Whenever we find
     * an unprocessed face F, we output a group of faces including F
     * whose size is maximum.
     */
    if( f->inside && ! f->marked ) {
      RenderMaximumFaceGroup( tess, f );
      assert( f->marked );
    }
  }
  if( tess->lonelyTriList != NULL ) {
    RenderLonelyTriangles( tess, tess->lonelyTriList );
    tess->lonelyTriList = NULL;
  }
}


static void RenderMaximumFaceGroup( GLUtesselator *tess, GLUface *fOrig )
{
  /* We want to find the largest triangle fan or strip of unmarked faces
   * which includes the given face fOrig.  There are 3 possible fans
   * passing through fOrig (one centered at each vertex), and 3 possible
   * strips (one for each CCW permutation of the vertices).  Our strategy
   * is to try all of these, and take the primitive which uses the most
   * triangles (a greedy approach).
   */
  GLUhalfEdge *e = fOrig->anEdge;
  struct FaceCount max, newFace;

  max.size = 1;
  max.eStart = e;
  max.render = &RenderTriangle;

  if( ! tess->flagBoundary ) {
    newFace = MaximumFan( e ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumFan( e->Lnext ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumFan( e->Lprev ); if( newFace.size > max.size ) { max = newFace; }

    newFace = MaximumStrip( e ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumStrip( e->Lnext ); if( newFace.size > max.size ) { max = newFace; }
    newFace = MaximumStrip( e->Lprev ); if( newFace.size > max.size ) { max = newFace; }
  }
  (*(max.render))( tess, max.eStart, max.size );
}


/* Macros which keep track of faces we have marked temporarily, and allow
 * us to backtrack when necessary.  With triangle fans, this is not
 * really necessary, since the only awkward case is a loop of triangles
 * around a single origin vertex.  However with strips the situation is
 * more complicated, and we need a general tracking method like the
 * one here.
 */
#define Marked(f)	(! (f)->inside || (f)->marked)

#define AddToTrail(f,t)	((f)->trail = (t), (t) = (f), (f)->marked = TRUE)

#define FreeTrail(t)	do { \
			  while( (t) != NULL ) { \
			    (t)->marked = FALSE; t = (t)->trail; \
			  } \
			} while(0) /* absorb trailing semicolon */



static struct FaceCount MaximumFan( GLUhalfEdge *eOrig )
{
  /* eOrig->Lface is the face we want to render.  We want to find the size
   * of a maximal fan around eOrig->Org.  To do this we just walk around
   * the origin vertex as far as possible in both directions.
   */
  struct FaceCount newFace = { 0, NULL, &RenderFan };
  GLUface *trail = NULL;
  GLUhalfEdge *e;

  for( e = eOrig; ! Marked( e->Lface ); e = e->Onext ) {
    AddToTrail( e->Lface, trail );
    ++newFace.size;
  }
  for( e = eOrig; ! Marked( e->Rface ); e = e->Oprev ) {
    AddToTrail( e->Rface, trail );
    ++newFace.size;
  }
  newFace.eStart = e;
  /*LINTED*/
  FreeTrail( trail );
  return newFace;
}


#define IsEven(n)	(((n) & 1) == 0)

static struct FaceCount MaximumStrip( GLUhalfEdge *eOrig )
{
  /* Here we are looking for a maximal strip that contains the vertices
   * eOrig->Org, eOrig->Dst, eOrig->Lnext->Dst (in that order or the
   * reverse, such that all triangles are oriented CCW).
   *
   * Again we walk forward and backward as far as possible.  However for
   * strips there is a twist: to get CCW orientations, there must be
   * an *even* number of triangles in the strip on one side of eOrig.
   * We walk the strip starting on a side with an even number of triangles;
   * if both side have an odd number, we are forced to shorten one side.
   */
  struct FaceCount newFace = { 0, NULL, &RenderStrip };
  long headSize = 0, tailSize = 0;
  GLUface *trail = NULL;
  GLUhalfEdge *e, *eTail, *eHead;

  for( e = eOrig; ! Marked( e->Lface ); ++tailSize, e = e->Onext ) {
    AddToTrail( e->Lface, trail );
    ++tailSize;
    e = e->Dprev;
    if( Marked( e->Lface )) break;
    AddToTrail( e->Lface, trail );
  }
  eTail = e;

  for( e = eOrig; ! Marked( e->Rface ); ++headSize, e = e->Dnext ) {
    AddToTrail( e->Rface, trail );
    ++headSize;
    e = e->Oprev;
    if( Marked( e->Rface )) break;
    AddToTrail( e->Rface, trail );
  }
  eHead = e;

  newFace.size = tailSize + headSize;
  if( IsEven( tailSize )) {
    newFace.eStart = eTail->Sym;
  } else if( IsEven( headSize )) {
    newFace.eStart = eHead;
  } else {
    /* Both sides have odd length, we must shorten one of them.  In fact,
     * we must start from eHead to guarantee inclusion of eOrig->Lface.
     */
    --newFace.size;
    newFace.eStart = eHead->Onext;
  }
  /*LINTED*/
  FreeTrail( trail );
  return newFace;
}


static void RenderTriangle( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  /* Just add the triangle to a triangle list, so we can render all
   * the separate triangles at once.
   */
  assert( size == 1 );
  AddToTrail( e->Lface, tess->lonelyTriList );
}


static void RenderLonelyTriangles( GLUtesselator *tess, GLUface *f )
{
  /* Now we render all the separate triangles which could not be
   * grouped into a triangle fan or strip.
   */
  GLUhalfEdge *e;
  int newState;
  int edgeState = -1;	/* force edge state output for first vertex */

  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLES );

  for( ; f != NULL; f = f->trail ) {
    /* Loop once for each edge (there will always be 3 edges) */

    e = f->anEdge;
    do {
      if( tess->flagBoundary ) {
	/* Set the "edge state" to TRUE just before we output the
	 * first vertex of each edge on the polygon boundary.
	 */
	newState = ! e->Rface->inside;
	if( edgeState != newState ) {
	  edgeState = newState;
          CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA( edgeState );
	}
      }
      CALL_VERTEX_OR_VERTEX_DATA( e->Org->data );

      e = e->Lnext;
    } while( e != f->anEdge );
  }
  CALL_END_OR_END_DATA();
}


static void RenderFan( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  /* Render as many CCW triangles as possible in a fan starting from
   * edge "e".  The fan *should* contain exactly "size" triangles
   * (otherwise we've goofed up somewhere).
   */
  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLE_FAN ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 

  while( ! Marked( e->Lface )) {
    e->Lface->marked = TRUE;
    --size;
    e = e->Onext;
    CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 
  }

  assert( size == 0 );
  CALL_END_OR_END_DATA();
}


static void RenderStrip( GLUtesselator *tess, GLUhalfEdge *e, long size )
{
  /* Render as many CCW triangles as possible in a strip starting from
   * edge "e".  The strip *should* contain exactly "size" triangles
   * (otherwise we've goofed up somewhere).
   */
  CALL_BEGIN_OR_BEGIN_DATA( GL_TRIANGLE_STRIP );
  CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
  CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 

  while( ! Marked( e->Lface )) {
    e->Lface->marked = TRUE;
    --size;
    e = e->Dprev;
    CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
    if( Marked( e->Lface )) break;

    e->Lface->marked = TRUE;
    --size;
    e = e->Onext;
    CALL_VERTEX_OR_VERTEX_DATA( e->Dst->data ); 
  }

  assert( size == 0 );
  CALL_END_OR_END_DATA();
}


/************************ Boundary contour decomposition ******************/

/* __gl_renderBoundary( tess, mesh ) takes a mesh, and outputs one
 * contour for each face marked "inside".  The rendering output is
 * provided as callbacks (see the api).
 */
void __gl_renderBoundary( GLUtesselator *tess, GLUmesh *mesh )
{
  GLUface *f;
  GLUhalfEdge *e;

  for( f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
    if( f->inside ) {
      CALL_BEGIN_OR_BEGIN_DATA( GL_LINE_LOOP );
      e = f->anEdge;
      do {
        CALL_VERTEX_OR_VERTEX_DATA( e->Org->data ); 
	e = e->Lnext;
      } while( e != f->anEdge );
      CALL_END_OR_END_DATA();
    }
  }
}


/************************ Quick-and-dirty decomposition ******************/

#define SIGN_INCONSISTENT 2

static int ComputeNormal( GLUtesselator *tess, GLdouble norm[3], int check )
/*
 * If check==FALSE, we compute the polygon normal and place it in norm[].
 * If check==TRUE, we check that each triangle in the fan from v0 has a
 * consistent orientation with respect to norm[].  If triangles are
 * consistently oriented CCW, return 1; if CW, return -1; if all triangles
 * are degenerate return 0; otherwise (no consistent orientation) return
 * SIGN_INCONSISTENT.
 */
{
  CachedVertex *v0 = tess->cache;
  CachedVertex *vn = v0 + tess->cacheCount;
  CachedVertex *vc;
  GLdouble dot, xc, yc, zc, xp, yp, zp, n[3];
  int sign = 0;

  /* Find the polygon normal.  It is important to get a reasonable
   * normal even when the polygon is self-intersecting (eg. a bowtie).
   * Otherwise, the computed normal could be very tiny, but perpendicular
   * to the true plane of the polygon due to numerical noise.  Then all
   * the triangles would appear to be degenerate and we would incorrectly
   * decompose the polygon as a fan (or simply not render it at all).
   *
   * We use a sum-of-triangles normal algorithm rather than the more
   * efficient sum-of-trapezoids method (used in CheckOrientation()
   * in normal.c).  This lets us explicitly reverse the signed area
   * of some triangles to get a reasonable normal in the self-intersecting
   * case.
   */
  if( ! check ) {
    norm[0] = norm[1] = norm[2] = 0.0;
  }

  vc = v0 + 1;
  xc = vc->coords[0] - v0->coords[0];
  yc = vc->coords[1] - v0->coords[1];
  zc = vc->coords[2] - v0->coords[2];
  while( ++vc < vn ) {
    xp = xc; yp = yc; zp = zc;
    xc = vc->coords[0] - v0->coords[0];
    yc = vc->coords[1] - v0->coords[1];
    zc = vc->coords[2] - v0->coords[2];

    /* Compute (vp - v0) cross (vc - v0) */
    n[0] = yp*zc - zp*yc;
    n[1] = zp*xc - xp*zc;
    n[2] = xp*yc - yp*xc;

    dot = n[0]*norm[0] + n[1]*norm[1] + n[2]*norm[2];
    if( ! check ) {
      /* Reverse the contribution of back-facing triangles to get
       * a reasonable normal for self-intersecting polygons (see above)
       */
      if( dot >= 0 ) {
	norm[0] += n[0]; norm[1] += n[1]; norm[2] += n[2];
      } else {
	norm[0] -= n[0]; norm[1] -= n[1]; norm[2] -= n[2];
      }
    } else if( dot != 0 ) {
      /* Check the new orientation for consistency with previous triangles */
      if( dot > 0 ) {
	if( sign < 0 ) return SIGN_INCONSISTENT;
	sign = 1;
      } else {
	if( sign > 0 ) return SIGN_INCONSISTENT;
	sign = -1;
      }
    }
  }
  return sign;
}

/* __gl_renderCache( tess ) takes a single contour and tries to render it
 * as a triangle fan.  This handles convex polygons, as well as some
 * non-convex polygons if we get lucky.
 *
 * Returns TRUE if the polygon was successfully rendered.  The rendering
 * output is provided as callbacks (see the api).
 */
GLboolean __gl_renderCache( GLUtesselator *tess )
{
  CachedVertex *v0 = tess->cache;
  CachedVertex *vn = v0 + tess->cacheCount;
  CachedVertex *vc;
  GLdouble norm[3];
  int sign;

  if( tess->cacheCount < 3 ) {
    /* Degenerate contour -- no output */
    return TRUE;
  }

  norm[0] = tess->normal[0];
  norm[1] = tess->normal[1];
  norm[2] = tess->normal[2];
  if( norm[0] == 0 && norm[1] == 0 && norm[2] == 0 ) {
    ComputeNormal( tess, norm, FALSE );
  }

  sign = ComputeNormal( tess, norm, TRUE );
  if( sign == SIGN_INCONSISTENT ) {
    /* Fan triangles did not have a consistent orientation */
    return FALSE;
  }
  if( sign == 0 ) {
    /* All triangles were degenerate */
    return TRUE;
  }

  /* Make sure we do the right thing for each winding rule */
  switch( tess->windingRule ) {
  case GLU_TESS_WINDING_ODD:
  case GLU_TESS_WINDING_NONZERO:
    break;
  case GLU_TESS_WINDING_POSITIVE:
    if( sign < 0 ) return TRUE;
    break;
  case GLU_TESS_WINDING_NEGATIVE:
    if( sign > 0 ) return TRUE;
    break;
  case GLU_TESS_WINDING_ABS_GEQ_TWO:
    return TRUE;
  }

  CALL_BEGIN_OR_BEGIN_DATA( tess->boundaryOnly ? GL_LINE_LOOP
			  : (tess->cacheCount > 3) ? GL_TRIANGLE_FAN
			  : GL_TRIANGLES );

  CALL_VERTEX_OR_VERTEX_DATA( v0->data ); 
  if( sign > 0 ) {
    for( vc = v0+1; vc < vn; ++vc ) {
      CALL_VERTEX_OR_VERTEX_DATA( vc->data ); 
    }
  } else {
    for( vc = vn-1; vc > v0; --vc ) {
      CALL_VERTEX_OR_VERTEX_DATA( vc->data ); 
    }
  }
  CALL_END_OR_END_DATA();
  return TRUE;
}
