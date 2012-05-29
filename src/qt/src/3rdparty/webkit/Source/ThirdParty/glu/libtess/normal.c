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
** $Header: //depot/main/gfx/lib/glu/libtess/normal.c#5 $
*/

#include <assert.h>
#include <math.h>
#include "ThirdParty/glu/gluos.h"
#include "ThirdParty/glu/libtess/mesh.h"
#include "ThirdParty/glu/libtess/normal.h"
#include "ThirdParty/glu/libtess/tess.h"

#define TRUE 1
#define FALSE 0

#define Dot(u,v)	(u[0]*v[0] + u[1]*v[1] + u[2]*v[2])

#if defined(FOR_TRITE_TEST_PROGRAM) || defined(TRUE_PROJECT)
static void Normalize( GLdouble v[3] )
{
  GLdouble len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

  assert( len > 0 );
  len = sqrt( len );
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}
#endif

#define ABS(x)	((x) < 0 ? -(x) : (x))

static int LongAxis( GLdouble v[3] )
{
  int i = 0;

  if( ABS(v[1]) > ABS(v[0]) ) { i = 1; }
  if( ABS(v[2]) > ABS(v[i]) ) { i = 2; }
  return i;
}

static void ComputeNormal( GLUtesselator *tess, GLdouble norm[3] )
{
  GLUvertex *v, *v1, *v2;
  GLdouble c, tLen2, maxLen2;
  GLdouble maxVal[3], minVal[3], d1[3], d2[3], tNorm[3];
  GLUvertex *maxVert[3], *minVert[3];
  GLUvertex *vHead = &tess->mesh->vHead;
  int i;

  maxVal[0] = maxVal[1] = maxVal[2] = -2 * GLU_TESS_MAX_COORD;
  minVal[0] = minVal[1] = minVal[2] = 2 * GLU_TESS_MAX_COORD;

  for( v = vHead->next; v != vHead; v = v->next ) {
    for( i = 0; i < 3; ++i ) {
      c = v->coords[i];
      if( c < minVal[i] ) { minVal[i] = c; minVert[i] = v; }
      if( c > maxVal[i] ) { maxVal[i] = c; maxVert[i] = v; }
    }
  }

  /* Find two vertices separated by at least 1/sqrt(3) of the maximum
   * distance between any two vertices
   */
  i = 0;
  if( maxVal[1] - minVal[1] > maxVal[0] - minVal[0] ) { i = 1; }
  if( maxVal[2] - minVal[2] > maxVal[i] - minVal[i] ) { i = 2; }
  if( minVal[i] >= maxVal[i] ) {
    /* All vertices are the same -- normal doesn't matter */
    norm[0] = 0; norm[1] = 0; norm[2] = 1;
    return;
  }

  /* Look for a third vertex which forms the triangle with maximum area
   * (Length of normal == twice the triangle area)
   */
  maxLen2 = 0;
  v1 = minVert[i];
  v2 = maxVert[i];
  d1[0] = v1->coords[0] - v2->coords[0];
  d1[1] = v1->coords[1] - v2->coords[1];
  d1[2] = v1->coords[2] - v2->coords[2];
  for( v = vHead->next; v != vHead; v = v->next ) {
    d2[0] = v->coords[0] - v2->coords[0];
    d2[1] = v->coords[1] - v2->coords[1];
    d2[2] = v->coords[2] - v2->coords[2];
    tNorm[0] = d1[1]*d2[2] - d1[2]*d2[1];
    tNorm[1] = d1[2]*d2[0] - d1[0]*d2[2];
    tNorm[2] = d1[0]*d2[1] - d1[1]*d2[0];
    tLen2 = tNorm[0]*tNorm[0] + tNorm[1]*tNorm[1] + tNorm[2]*tNorm[2];
    if( tLen2 > maxLen2 ) {
      maxLen2 = tLen2;
      norm[0] = tNorm[0];
      norm[1] = tNorm[1];
      norm[2] = tNorm[2];
    }
  }

  if( maxLen2 <= 0 ) {
    /* All points lie on a single line -- any decent normal will do */
    norm[0] = norm[1] = norm[2] = 0;
    norm[LongAxis(d1)] = 1;
  }
}
  

static void CheckOrientation( GLUtesselator *tess )
{
  GLdouble area;
  GLUface *f, *fHead = &tess->mesh->fHead;
  GLUvertex *v, *vHead = &tess->mesh->vHead;
  GLUhalfEdge *e;

  /* When we compute the normal automatically, we choose the orientation
   * so that the the sum of the signed areas of all contours is non-negative.
   */
  area = 0;
  for( f = fHead->next; f != fHead; f = f->next ) {
    e = f->anEdge;
    if( e->winding <= 0 ) continue;
    do {
      area += (e->Org->s - e->Dst->s) * (e->Org->t + e->Dst->t);
      e = e->Lnext;
    } while( e != f->anEdge );
  }
  if( area < 0 ) {
    /* Reverse the orientation by flipping all the t-coordinates */
    for( v = vHead->next; v != vHead; v = v->next ) {
      v->t = - v->t;
    }
    tess->tUnit[0] = - tess->tUnit[0];
    tess->tUnit[1] = - tess->tUnit[1];
    tess->tUnit[2] = - tess->tUnit[2];
  }
}

#ifdef FOR_TRITE_TEST_PROGRAM
#include <stdlib.h>
extern int RandomSweep;
#define S_UNIT_X	(RandomSweep ? (2*drand48()-1) : 1.0)
#define S_UNIT_Y	(RandomSweep ? (2*drand48()-1) : 0.0)
#else
#if defined(SLANTED_SWEEP) 
/* The "feature merging" is not intended to be complete.  There are
 * special cases where edges are nearly parallel to the sweep line
 * which are not implemented.  The algorithm should still behave
 * robustly (ie. produce a reasonable tesselation) in the presence
 * of such edges, however it may miss features which could have been
 * merged.  We could minimize this effect by choosing the sweep line
 * direction to be something unusual (ie. not parallel to one of the
 * coordinate axes).
 */
#define S_UNIT_X	0.50941539564955385	/* Pre-normalized */
#define S_UNIT_Y	0.86052074622010633
#else
#define S_UNIT_X	1.0
#define S_UNIT_Y	0.0
#endif
#endif

/* Determine the polygon normal and project vertices onto the plane
 * of the polygon.
 */
void __gl_projectPolygon( GLUtesselator *tess )
{
  GLUvertex *v, *vHead = &tess->mesh->vHead;
  GLdouble norm[3];
  GLdouble *sUnit, *tUnit;
  int i, computedNormal = FALSE;

  norm[0] = tess->normal[0];
  norm[1] = tess->normal[1];
  norm[2] = tess->normal[2];
  if( norm[0] == 0 && norm[1] == 0 && norm[2] == 0 ) {
    ComputeNormal( tess, norm );
    computedNormal = TRUE;
  }
  sUnit = tess->sUnit;
  tUnit = tess->tUnit;
  i = LongAxis( norm );

#if defined(FOR_TRITE_TEST_PROGRAM) || defined(TRUE_PROJECT)
  /* Choose the initial sUnit vector to be approximately perpendicular
   * to the normal.
   */
  Normalize( norm );

  sUnit[i] = 0;
  sUnit[(i+1)%3] = S_UNIT_X;
  sUnit[(i+2)%3] = S_UNIT_Y;

  /* Now make it exactly perpendicular */
  w = Dot( sUnit, norm );
  sUnit[0] -= w * norm[0];
  sUnit[1] -= w * norm[1];
  sUnit[2] -= w * norm[2];
  Normalize( sUnit );

  /* Choose tUnit so that (sUnit,tUnit,norm) form a right-handed frame */
  tUnit[0] = norm[1]*sUnit[2] - norm[2]*sUnit[1];
  tUnit[1] = norm[2]*sUnit[0] - norm[0]*sUnit[2];
  tUnit[2] = norm[0]*sUnit[1] - norm[1]*sUnit[0];
  Normalize( tUnit );
#else
  /* Project perpendicular to a coordinate axis -- better numerically */
  sUnit[i] = 0;
  sUnit[(i+1)%3] = S_UNIT_X;
  sUnit[(i+2)%3] = S_UNIT_Y;
  
  tUnit[i] = 0;
  tUnit[(i+1)%3] = (norm[i] > 0) ? -S_UNIT_Y : S_UNIT_Y;
  tUnit[(i+2)%3] = (norm[i] > 0) ? S_UNIT_X : -S_UNIT_X;
#endif

  /* Project the vertices onto the sweep plane */
  for( v = vHead->next; v != vHead; v = v->next ) {
    v->s = Dot( v->coords, sUnit );
    v->t = Dot( v->coords, tUnit );
  }
  if( computedNormal ) {
    CheckOrientation( tess );
  }
}
