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
** $Header: //depot/main/gfx/lib/glu/libtess/geom.h#5 $
*/

#ifndef __geom_h_
#define __geom_h_

#include "ThirdParty/glu/libtess/mesh.h"

#ifdef NO_BRANCH_CONDITIONS
/* MIPS architecture has special instructions to evaluate boolean
 * conditions -- more efficient than branching, IF you can get the
 * compiler to generate the right instructions (SGI compiler doesn't)
 */
#define VertEq(u,v)	(((u)->s == (v)->s) & ((u)->t == (v)->t))
#define VertLeq(u,v)	(((u)->s < (v)->s) | \
                         ((u)->s == (v)->s & (u)->t <= (v)->t))
#else
#define VertEq(u,v)	((u)->s == (v)->s && (u)->t == (v)->t)
#define VertLeq(u,v)	(((u)->s < (v)->s) || \
                         ((u)->s == (v)->s && (u)->t <= (v)->t))
#endif

#define EdgeEval(u,v,w)	__gl_edgeEval(u,v,w)
#define EdgeSign(u,v,w)	__gl_edgeSign(u,v,w)

/* Versions of VertLeq, EdgeSign, EdgeEval with s and t transposed. */

#define TransLeq(u,v)	(((u)->t < (v)->t) || \
                         ((u)->t == (v)->t && (u)->s <= (v)->s))
#define TransEval(u,v,w)	__gl_transEval(u,v,w)
#define TransSign(u,v,w)	__gl_transSign(u,v,w)


#define EdgeGoesLeft(e)		VertLeq( (e)->Dst, (e)->Org )
#define EdgeGoesRight(e)	VertLeq( (e)->Org, (e)->Dst )

#define ABS(x)	((x) < 0 ? -(x) : (x))
#define VertL1dist(u,v)	(ABS(u->s - v->s) + ABS(u->t - v->t))

#define VertCCW(u,v,w)	__gl_vertCCW(u,v,w)

int		__gl_vertLeq( GLUvertex *u, GLUvertex *v );
GLdouble	__gl_edgeEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_edgeSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
int		__gl_vertCCW( GLUvertex *u, GLUvertex *v, GLUvertex *w );
void		__gl_edgeIntersect( GLUvertex *o1, GLUvertex *d1,
				    GLUvertex *o2, GLUvertex *d2,
				    GLUvertex *v );

#endif
