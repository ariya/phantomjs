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
** $Header: //depot/main/gfx/lib/glu/libtess/sweep.h#5 $
*/

#ifndef __sweep_h_
#define __sweep_h_

#include "ThirdParty/glu/libtess/mesh.h"

/* __gl_computeInterior( tess ) computes the planar arrangement specified
 * by the given contours, and further subdivides this arrangement
 * into regions.  Each region is marked "inside" if it belongs
 * to the polygon, according to the rule given by tess->windingRule.
 * Each interior region is guaranteed be monotone.
 */
int __gl_computeInterior( GLUtesselator *tess );


/* The following is here *only* for access by debugging routines */

#include "dict.h"

/* For each pair of adjacent edges crossing the sweep line, there is
 * an ActiveRegion to represent the region between them.  The active
 * regions are kept in sorted order in a dynamic dictionary.  As the
 * sweep line crosses each vertex, we update the affected regions.
 */

struct ActiveRegion {
  GLUhalfEdge	*eUp;		/* upper edge, directed right to left */
  DictNode	*nodeUp;	/* dictionary node corresponding to eUp */
  int		windingNumber;	/* used to determine which regions are
                                 * inside the polygon */
  GLboolean	inside;		/* is this region inside the polygon? */
  GLboolean	sentinel;	/* marks fake edges at t = +/-infinity */
  GLboolean	dirty;		/* marks regions where the upper or lower
                                 * edge has changed, but we haven't checked
                                 * whether they intersect yet */
  GLboolean	fixUpperEdge;	/* marks temporary edges introduced when
                                 * we process a "right vertex" (one without
                                 * any edges leaving to the right) */
};

#define RegionBelow(r)	((ActiveRegion *) dictKey(dictPred((r)->nodeUp)))
#define RegionAbove(r)	((ActiveRegion *) dictKey(dictSucc((r)->nodeUp)))

#endif
