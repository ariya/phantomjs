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
** $Header: //depot/main/gfx/lib/glu/libtess/tess.h#6 $
*/

#ifndef __tess_h_
#define __tess_h_

#include <setjmp.h>

#include "ThirdParty/glu/internal_glu.h"
#include "ThirdParty/glu/libtess/dict.h"
#include "ThirdParty/glu/libtess/mesh.h"
#include "ThirdParty/glu/libtess/priorityq.h"

/* The begin/end calls must be properly nested.  We keep track of
 * the current state to enforce the ordering.
 */
enum TessState { T_DORMANT, T_IN_POLYGON, T_IN_CONTOUR };

/* We cache vertex data for single-contour polygons so that we can
 * try a quick-and-dirty decomposition first.
 */
#define TESS_MAX_CACHE	100

typedef struct CachedVertex {
  GLdouble	coords[3];
  void		*data;
} CachedVertex;

struct GLUtesselator {

  /*** state needed for collecting the input data ***/

  enum TessState state;		/* what begin/end calls have we seen? */

  GLUhalfEdge	*lastEdge;	/* lastEdge->Org is the most recent vertex */
  GLUmesh	*mesh;		/* stores the input contours, and eventually
                                   the tessellation itself */

  void		(GLAPIENTRY *callError)( GLenum errnum );

  /*** state needed for projecting onto the sweep plane ***/

  GLdouble	normal[3];	/* user-specified normal (if provided) */
  GLdouble	sUnit[3];	/* unit vector in s-direction (debugging) */
  GLdouble	tUnit[3];	/* unit vector in t-direction (debugging) */

  /*** state needed for the line sweep ***/

  GLdouble	relTolerance;	/* tolerance for merging features */
  GLenum	windingRule;	/* rule for determining polygon interior */
  GLboolean	fatalError;	/* fatal error: needed combine callback */

  Dict		*dict;		/* edge dictionary for sweep line */
  PriorityQ	*pq;		/* priority queue of vertex events */
  GLUvertex	*event;		/* current sweep event being processed */

  void		(GLAPIENTRY *callCombine)( GLdouble coords[3], void *data[4],
			        GLfloat weight[4], void **outData );

  /*** state needed for rendering callbacks (see render.c) ***/

  GLboolean	flagBoundary;	/* mark boundary edges (use EdgeFlag) */
  GLboolean	boundaryOnly;	/* Extract contours, not triangles */
  GLUface	*lonelyTriList;
    /* list of triangles which could not be rendered as strips or fans */

  void		(GLAPIENTRY *callBegin)( GLenum type );
  void		(GLAPIENTRY *callEdgeFlag)( GLboolean boundaryEdge );
  void		(GLAPIENTRY *callVertex)( void *data );
  void		(GLAPIENTRY *callEnd)( void );
  void		(GLAPIENTRY *callMesh)( GLUmesh *mesh );


  /*** state needed to cache single-contour polygons for renderCache() */

  GLboolean	emptyCache;		/* empty cache on next vertex() call */
  int		cacheCount;		/* number of cached vertices */
  CachedVertex	cache[TESS_MAX_CACHE];	/* the vertex data */

  /*** rendering callbacks that also pass polygon data  ***/ 
  void		(GLAPIENTRY *callBeginData)( GLenum type, void *polygonData );
  void		(GLAPIENTRY *callEdgeFlagData)( GLboolean boundaryEdge, 
				     void *polygonData );
  void		(GLAPIENTRY *callVertexData)( void *data, void *polygonData );
  void		(GLAPIENTRY *callEndData)( void *polygonData );
  void		(GLAPIENTRY *callErrorData)( GLenum errnum, void *polygonData );
  void		(GLAPIENTRY *callCombineData)( GLdouble coords[3], void *data[4],
				    GLfloat weight[4], void **outData,
				    void *polygonData );

  jmp_buf env;			/* place to jump to when memAllocs fail */

  void *polygonData;		/* client data for current polygon */
};

void GLAPIENTRY __gl_noBeginData( GLenum type, void *polygonData );
void GLAPIENTRY __gl_noEdgeFlagData( GLboolean boundaryEdge, void *polygonData );
void GLAPIENTRY __gl_noVertexData( void *data, void *polygonData );
void GLAPIENTRY __gl_noEndData( void *polygonData );
void GLAPIENTRY __gl_noErrorData( GLenum errnum, void *polygonData );
void GLAPIENTRY __gl_noCombineData( GLdouble coords[3], void *data[4],
			 GLfloat weight[4], void **outData,
			 void *polygonData );

#define CALL_BEGIN_OR_BEGIN_DATA(a) \
   if (tess->callBeginData != &__gl_noBeginData) \
      (*tess->callBeginData)((a),tess->polygonData); \
   else (*tess->callBegin)((a));

#define CALL_VERTEX_OR_VERTEX_DATA(a) \
   if (tess->callVertexData != &__gl_noVertexData) \
      (*tess->callVertexData)((a),tess->polygonData); \
   else (*tess->callVertex)((a));

#define CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA(a) \
   if (tess->callEdgeFlagData != &__gl_noEdgeFlagData) \
      (*tess->callEdgeFlagData)((a),tess->polygonData); \
   else (*tess->callEdgeFlag)((a));

#define CALL_END_OR_END_DATA() \
   if (tess->callEndData != &__gl_noEndData) \
      (*tess->callEndData)(tess->polygonData); \
   else (*tess->callEnd)();

#define CALL_COMBINE_OR_COMBINE_DATA(a,b,c,d) \
   if (tess->callCombineData != &__gl_noCombineData) \
      (*tess->callCombineData)((a),(b),(c),(d),tess->polygonData); \
   else (*tess->callCombine)((a),(b),(c),(d));

#define CALL_ERROR_OR_ERROR_DATA(a) \
   if (tess->callErrorData != &__gl_noErrorData) \
      (*tess->callErrorData)((a),tess->polygonData); \
   else (*tess->callError)((a));

#endif
