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
** $Header: //depot/main/gfx/lib/glu/libtess/tess.c#7 $
*/

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>

#include "ThirdParty/glu/gluos.h"
#include "ThirdParty/glu/libtess/memalloc.h"
#include "ThirdParty/glu/libtess/mesh.h"
#include "ThirdParty/glu/libtess/normal.h"
#include "ThirdParty/glu/libtess/render.h"
#include "ThirdParty/glu/libtess/sweep.h"
#include "ThirdParty/glu/libtess/tess.h"
#include "ThirdParty/glu/libtess/tessmono.h"

#define GLU_TESS_DEFAULT_TOLERANCE 0.0
#define GLU_TESS_MESH		100112	/* void (*)(GLUmesh *mesh)	    */

#define TRUE 1
#define FALSE 0

/*ARGSUSED*/ static void GLAPIENTRY noBegin( GLenum type ) {}
/*ARGSUSED*/ static void GLAPIENTRY noEdgeFlag( GLboolean boundaryEdge ) {}
/*ARGSUSED*/ static void GLAPIENTRY noVertex( void *data ) {}
/*ARGSUSED*/ static void GLAPIENTRY noEnd( void ) {}
/*ARGSUSED*/ static void GLAPIENTRY noError( GLenum errnum ) {}
/*ARGSUSED*/ static void GLAPIENTRY noCombine( GLdouble coords[3], void *data[4],
                                    GLfloat weight[4], void **dataOut ) {}
/*ARGSUSED*/ static void GLAPIENTRY noMesh( GLUmesh *mesh ) {}


/*ARGSUSED*/ void GLAPIENTRY __gl_noBeginData( GLenum type,
					     void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noEdgeFlagData( GLboolean boundaryEdge, 
				       void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noVertexData( void *data,
					      void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noEndData( void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noErrorData( GLenum errnum,
					     void *polygonData ) {}
/*ARGSUSED*/ void GLAPIENTRY __gl_noCombineData( GLdouble coords[3],
					       void *data[4],
					       GLfloat weight[4],
					       void **outData,
					       void *polygonData ) {}

/* Half-edges are allocated in pairs (see mesh.c) */
typedef struct { GLUhalfEdge e, eSym; } EdgePair;

#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MAX_FAST_ALLOC	(MAX(sizeof(EdgePair), \
			 MAX(sizeof(GLUvertex),sizeof(GLUface))))


GLUtesselator * GLAPIENTRY
internal_gluNewTess( void )
{
  GLUtesselator *tess;

  /* Only initialize fields which can be changed by the api.  Other fields
   * are initialized where they are used.
   */

  if (memInit( MAX_FAST_ALLOC ) == 0) {
     return 0;			/* out of memory */
  }
  tess = (GLUtesselator *)memAlloc( sizeof( GLUtesselator ));
  if (tess == NULL) {
     return 0;			/* out of memory */
  }

  tess->state = T_DORMANT;

  tess->normal[0] = 0;
  tess->normal[1] = 0;
  tess->normal[2] = 0;

  tess->relTolerance = GLU_TESS_DEFAULT_TOLERANCE;
  tess->windingRule = GLU_TESS_WINDING_ODD;
  tess->flagBoundary = FALSE;
  tess->boundaryOnly = FALSE;

  tess->callBegin = &noBegin;
  tess->callEdgeFlag = &noEdgeFlag;
  tess->callVertex = &noVertex;
  tess->callEnd = &noEnd;

  tess->callError = &noError;
  tess->callCombine = &noCombine;
  tess->callMesh = &noMesh;

  tess->callBeginData= &__gl_noBeginData;
  tess->callEdgeFlagData= &__gl_noEdgeFlagData;
  tess->callVertexData= &__gl_noVertexData;
  tess->callEndData= &__gl_noEndData;
  tess->callErrorData= &__gl_noErrorData;
  tess->callCombineData= &__gl_noCombineData;

  tess->polygonData= NULL;

  return tess;
}

static void MakeDormant( GLUtesselator *tess )
{
  /* Return the tessellator to its original dormant state. */

  if( tess->mesh != NULL ) {
    __gl_meshDeleteMesh( tess->mesh );
  }
  tess->state = T_DORMANT;
  tess->lastEdge = NULL;
  tess->mesh = NULL;
}

#define RequireState( tess, s )   if( tess->state != s ) GotoState(tess,s)

static void GotoState( GLUtesselator *tess, enum TessState newState )
{
  while( tess->state != newState ) {
    /* We change the current state one level at a time, to get to
     * the desired state.
     */
    if( tess->state < newState ) {
      switch( tess->state ) {
      case T_DORMANT:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_POLYGON );
	internal_gluTessBeginPolygon( tess, NULL );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_BEGIN_CONTOUR );
	internal_gluTessBeginContour( tess );
	break;
      default:
        assert(0);
        break;
      }
    } else {
      switch( tess->state ) {
      case T_IN_CONTOUR:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_CONTOUR );
	internal_gluTessEndContour( tess );
	break;
      case T_IN_POLYGON:
	CALL_ERROR_OR_ERROR_DATA( GLU_TESS_MISSING_END_POLYGON );
	/* gluTessEndPolygon( tess ) is too much work! */
	MakeDormant( tess );
	break;
      default:
        assert(0);
        break;
      }
    }
  }
}


void GLAPIENTRY
internal_gluDeleteTess( GLUtesselator *tess )
{
  RequireState( tess, T_DORMANT );
  memFree( tess );
}


void GLAPIENTRY
internal_gluTessProperty( GLUtesselator *tess, GLenum which, GLdouble value )
{
  GLenum windingRule;

  switch( which ) {
  case GLU_TESS_TOLERANCE:
    if( value < 0.0 || value > 1.0 ) break;
    tess->relTolerance = value;
    return;

  case GLU_TESS_WINDING_RULE:
    windingRule = (GLenum) value;
    if( windingRule != value ) break;	/* not an integer */

    switch( windingRule ) {
    case GLU_TESS_WINDING_ODD:
    case GLU_TESS_WINDING_NONZERO:
    case GLU_TESS_WINDING_POSITIVE:
    case GLU_TESS_WINDING_NEGATIVE:
    case GLU_TESS_WINDING_ABS_GEQ_TWO:
      tess->windingRule = windingRule;
      return;
    default:
      break;
    }

  case GLU_TESS_BOUNDARY_ONLY:
    tess->boundaryOnly = (value != 0);
    return;

  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
  CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_VALUE );
}

/* Returns tessellator property */
void GLAPIENTRY
internal_gluGetTessProperty( GLUtesselator *tess, GLenum which, GLdouble *value )
{
   switch (which) {
   case GLU_TESS_TOLERANCE:
      /* tolerance should be in range [0..1] */
      assert(0.0 <= tess->relTolerance && tess->relTolerance <= 1.0);
      *value= tess->relTolerance;
      break;    
   case GLU_TESS_WINDING_RULE:
      assert(tess->windingRule == GLU_TESS_WINDING_ODD ||
	     tess->windingRule == GLU_TESS_WINDING_NONZERO ||
	     tess->windingRule == GLU_TESS_WINDING_POSITIVE ||
	     tess->windingRule == GLU_TESS_WINDING_NEGATIVE ||
	     tess->windingRule == GLU_TESS_WINDING_ABS_GEQ_TWO);
      *value= tess->windingRule;
      break;
   case GLU_TESS_BOUNDARY_ONLY:
      assert(tess->boundaryOnly == TRUE || tess->boundaryOnly == FALSE);
      *value= tess->boundaryOnly;
      break;
   default:
      *value= 0.0;
      CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
      break;
   }
} /* gluGetTessProperty() */

void GLAPIENTRY
internal_gluTessNormal( GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z )
{
  tess->normal[0] = x;
  tess->normal[1] = y;
  tess->normal[2] = z;
}

void GLAPIENTRY
internal_gluTessCallback( GLUtesselator *tess, GLenum which, void (GLAPIENTRY *fn)())
{
  switch( which ) {
  case GLU_TESS_BEGIN:
    tess->callBegin = (fn == NULL) ? &noBegin : (void (GLAPIENTRY *)(GLenum)) fn;
    return;
  case GLU_TESS_BEGIN_DATA:
    tess->callBeginData = (fn == NULL) ?
	&__gl_noBeginData : (void (GLAPIENTRY *)(GLenum, void *)) fn;
    return;
  case GLU_TESS_EDGE_FLAG:
    tess->callEdgeFlag = (fn == NULL) ? &noEdgeFlag :
					(void (GLAPIENTRY *)(GLboolean)) fn;
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_EDGE_FLAG_DATA:
    tess->callEdgeFlagData= (fn == NULL) ?
	&__gl_noEdgeFlagData : (void (GLAPIENTRY *)(GLboolean, void *)) fn; 
    /* If the client wants boundary edges to be flagged,
     * we render everything as separate triangles (no strips or fans).
     */
    tess->flagBoundary = (fn != NULL);
    return;
  case GLU_TESS_VERTEX:
    tess->callVertex = (fn == NULL) ? &noVertex :
				      (void (GLAPIENTRY *)(void *)) fn;
    return;
  case GLU_TESS_VERTEX_DATA:
    tess->callVertexData = (fn == NULL) ?
	&__gl_noVertexData : (void (GLAPIENTRY *)(void *, void *)) fn;
    return;
  case GLU_TESS_END:
    tess->callEnd = (fn == NULL) ? &noEnd : (void (GLAPIENTRY *)(void)) fn;
    return;
  case GLU_TESS_END_DATA:
    tess->callEndData = (fn == NULL) ? &__gl_noEndData : 
                                       (void (GLAPIENTRY *)(void *)) fn;
    return;
  case GLU_TESS_ERROR:
    tess->callError = (fn == NULL) ? &noError : (void (GLAPIENTRY *)(GLenum)) fn;
    return;
  case GLU_TESS_ERROR_DATA:
    tess->callErrorData = (fn == NULL) ?
	&__gl_noErrorData : (void (GLAPIENTRY *)(GLenum, void *)) fn;
    return;
  case GLU_TESS_COMBINE:
    tess->callCombine = (fn == NULL) ? &noCombine :
	(void (GLAPIENTRY *)(GLdouble [3],void *[4], GLfloat [4], void ** )) fn;
    return;
  case GLU_TESS_COMBINE_DATA:
    tess->callCombineData = (fn == NULL) ? &__gl_noCombineData :
                                           (void (GLAPIENTRY *)(GLdouble [3],
						     void *[4], 
						     GLfloat [4], 
						     void **,
						     void *)) fn;
    return;
  case GLU_TESS_MESH:
    tess->callMesh = (fn == NULL) ? &noMesh : (void (GLAPIENTRY *)(GLUmesh *)) fn;
    return;
  default:
    CALL_ERROR_OR_ERROR_DATA( GLU_INVALID_ENUM );
    return;
  }
}

static int AddVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  GLUhalfEdge *e;

  e = tess->lastEdge;
  if( e == NULL ) {
    /* Make a self-loop (one vertex, one edge). */

    e = __gl_meshMakeEdge( tess->mesh );
    if (e == NULL) return 0;
    if ( !__gl_meshSplice( e, e->Sym ) ) return 0;
  } else {
    /* Create a new vertex and edge which immediately follow e
     * in the ordering around the left face.
     */
    if (__gl_meshSplitEdge( e ) == NULL) return 0;
    e = e->Lnext;
  }

  /* The new vertex is now e->Org. */
  e->Org->data = data;
  e->Org->coords[0] = coords[0];
  e->Org->coords[1] = coords[1];
  e->Org->coords[2] = coords[2];
  
  /* The winding of an edge says how the winding number changes as we
   * cross from the edge''s right face to its left face.  We add the
   * vertices in such an order that a CCW contour will add +1 to
   * the winding number of the region inside the contour.
   */
  e->winding = 1;
  e->Sym->winding = -1;

  tess->lastEdge = e;

  return 1;
}


static void CacheVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  CachedVertex *v = &tess->cache[tess->cacheCount];

  v->data = data;
  v->coords[0] = coords[0];
  v->coords[1] = coords[1];
  v->coords[2] = coords[2];
  ++tess->cacheCount;
}


static int EmptyCache( GLUtesselator *tess )
{
  CachedVertex *v = tess->cache;
  CachedVertex *vLast;

  tess->mesh = __gl_meshNewMesh();
  if (tess->mesh == NULL) return 0;

  for( vLast = v + tess->cacheCount; v < vLast; ++v ) {
    if ( !AddVertex( tess, v->coords, v->data ) ) return 0;
  }
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;

  return 1;
}


void GLAPIENTRY
internal_gluTessVertex( GLUtesselator *tess, GLdouble coords[3], void *data )
{
  int i, tooLarge = FALSE;
  GLdouble x, clamped[3];

  RequireState( tess, T_IN_CONTOUR );

  if( tess->emptyCache ) {
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
       return;
    }
    tess->lastEdge = NULL;
  }
  for( i = 0; i < 3; ++i ) {
    x = coords[i];
    if( x < - GLU_TESS_MAX_COORD ) {
      x = - GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    if( x > GLU_TESS_MAX_COORD ) {
      x = GLU_TESS_MAX_COORD;
      tooLarge = TRUE;
    }
    clamped[i] = x;
  }
  if( tooLarge ) {
    CALL_ERROR_OR_ERROR_DATA( GLU_TESS_COORD_TOO_LARGE );
  }

  if( tess->mesh == NULL ) {
    if( tess->cacheCount < TESS_MAX_CACHE ) {
      CacheVertex( tess, clamped, data );
      return;
    }
    if ( !EmptyCache( tess ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
       return;
    }
  }
  if ( !AddVertex( tess, clamped, data ) ) {
       CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
  }
}


void GLAPIENTRY
internal_gluTessBeginPolygon( GLUtesselator *tess, void *data )
{
  RequireState( tess, T_DORMANT );

  tess->state = T_IN_POLYGON;
  tess->cacheCount = 0;
  tess->emptyCache = FALSE;
  tess->mesh = NULL;

  tess->polygonData= data;
}


void GLAPIENTRY
internal_gluTessBeginContour( GLUtesselator *tess )
{
  RequireState( tess, T_IN_POLYGON );

  tess->state = T_IN_CONTOUR;
  tess->lastEdge = NULL;
  if( tess->cacheCount > 0 ) {
    /* Just set a flag so we don't get confused by empty contours
     * -- these can be generated accidentally with the obsolete
     * NextContour() interface.
     */
    tess->emptyCache = TRUE;
  }
}


void GLAPIENTRY
internal_gluTessEndContour( GLUtesselator *tess )
{
  RequireState( tess, T_IN_CONTOUR );
  tess->state = T_IN_POLYGON;
}

void GLAPIENTRY
internal_gluTessEndPolygon( GLUtesselator *tess )
{
  GLUmesh *mesh;

  if (setjmp(tess->env) != 0) {	
     /* come back here if out of memory */
     CALL_ERROR_OR_ERROR_DATA( GLU_OUT_OF_MEMORY );
     return;
  }

  RequireState( tess, T_IN_POLYGON );
  tess->state = T_DORMANT;

  if( tess->mesh == NULL ) {
    if( ! tess->flagBoundary && tess->callMesh == &noMesh ) {

      /* Try some special code to make the easy cases go quickly
       * (eg. convex polygons).  This code does NOT handle multiple contours,
       * intersections, edge flags, and of course it does not generate
       * an explicit mesh either.
       */
      if( __gl_renderCache( tess )) {
	tess->polygonData= NULL; 
	return;
      }
    }
    if ( !EmptyCache( tess ) ) longjmp(tess->env,1); /* could've used a label*/
  }

  /* Determine the polygon normal and project vertices onto the plane
   * of the polygon.
   */
  __gl_projectPolygon( tess );

  /* __gl_computeInterior( tess ) computes the planar arrangement specified
   * by the given contours, and further subdivides this arrangement
   * into regions.  Each region is marked "inside" if it belongs
   * to the polygon, according to the rule given by tess->windingRule.
   * Each interior region is guaranteed be monotone.
   */
  if ( !__gl_computeInterior( tess ) ) {
     longjmp(tess->env,1);	/* could've used a label */
  }

  mesh = tess->mesh;
  if( ! tess->fatalError ) {
    int rc = 1;

    /* If the user wants only the boundary contours, we throw away all edges
     * except those which separate the interior from the exterior.
     * Otherwise we tessellate all the regions marked "inside".
     */
    if( tess->boundaryOnly ) {
      rc = __gl_meshSetWindingNumber( mesh, 1, TRUE );
    } else {
      rc = __gl_meshTessellateInterior( mesh ); 
    }
    if (rc == 0) longjmp(tess->env,1);	/* could've used a label */

    __gl_meshCheckMesh( mesh );

    if( tess->callBegin != &noBegin || tess->callEnd != &noEnd
       || tess->callVertex != &noVertex || tess->callEdgeFlag != &noEdgeFlag 
       || tess->callBeginData != &__gl_noBeginData 
       || tess->callEndData != &__gl_noEndData
       || tess->callVertexData != &__gl_noVertexData
       || tess->callEdgeFlagData != &__gl_noEdgeFlagData )
    {
      if( tess->boundaryOnly ) {
	__gl_renderBoundary( tess, mesh );  /* output boundary contours */
      } else {
	__gl_renderMesh( tess, mesh );	   /* output strips and fans */
      }
    }
    if( tess->callMesh != &noMesh ) {

      /* Throw away the exterior faces, so that all faces are interior.
       * This way the user doesn't have to check the "inside" flag,
       * and we don't need to even reveal its existence.  It also leaves
       * the freedom for an implementation to not generate the exterior
       * faces in the first place.
       */
      __gl_meshDiscardExterior( mesh );
      (*tess->callMesh)( mesh );		/* user wants the mesh itself */
      tess->mesh = NULL;
      tess->polygonData= NULL;
      return;
    }
  }
  __gl_meshDeleteMesh( mesh );
  tess->polygonData= NULL;
  tess->mesh = NULL;
}


/*XXXblythe unused function*/
#if 0
void GLAPIENTRY
gluDeleteMesh( GLUmesh *mesh )
{
  __gl_meshDeleteMesh( mesh );
}
#endif



/*******************************************************/

/* Obsolete calls -- for backward compatibility */

#if 0
void GLAPIENTRY
gluBeginPolygon( GLUtesselator *tess )
{
  gluTessBeginPolygon( tess, NULL );
  gluTessBeginContour( tess );
}


/*ARGSUSED*/
void GLAPIENTRY
gluNextContour( GLUtesselator *tess, GLenum type )
{
  gluTessEndContour( tess );
  gluTessBeginContour( tess );
}


void GLAPIENTRY
gluEndPolygon( GLUtesselator *tess )
{
  gluTessEndContour( tess );
  gluTessEndPolygon( tess );
}
#endif
