/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This header is a skeleton substitute for GL/glu.h which contains
// only the definitions, constants and function declarations necessary
// to compile the GLU tessellator.

#ifndef INTERNAL_GLU_H_
#define INTERNAL_GLU_H_

#include "ThirdParty/glu/gluos.h"

/* Primitives */
#define GL_LINE_LOOP                            0x0002
#define GL_TRIANGLES                            0x0004
#define GL_TRIANGLE_STRIP                       0x0005
#define GL_TRIANGLE_FAN                         0x0006

/* ErrorCode */
#define GLU_INVALID_ENUM                     100900
#define GLU_INVALID_VALUE                    100901
#define GLU_OUT_OF_MEMORY                    100902

/* TessCallback */
#define GLU_TESS_BEGIN                       100100
#define GLU_BEGIN                            100100
#define GLU_TESS_VERTEX                      100101
#define GLU_VERTEX                           100101
#define GLU_TESS_END                         100102
#define GLU_END                              100102
#define GLU_TESS_ERROR                       100103
#define GLU_TESS_EDGE_FLAG                   100104
#define GLU_EDGE_FLAG                        100104
#define GLU_TESS_COMBINE                     100105
#define GLU_TESS_BEGIN_DATA                  100106
#define GLU_TESS_VERTEX_DATA                 100107
#define GLU_TESS_END_DATA                    100108
#define GLU_TESS_ERROR_DATA                  100109
#define GLU_TESS_EDGE_FLAG_DATA              100110
#define GLU_TESS_COMBINE_DATA                100111

/* TessContour */
#define GLU_CW                               100120
#define GLU_CCW                              100121
#define GLU_INTERIOR                         100122
#define GLU_EXTERIOR                         100123
#define GLU_UNKNOWN                          100124

/* TessProperty */
#define GLU_TESS_WINDING_RULE                100140
#define GLU_TESS_BOUNDARY_ONLY               100141
#define GLU_TESS_TOLERANCE                   100142

/* TessError */
#define GLU_TESS_ERROR1                      100151
#define GLU_TESS_ERROR2                      100152
#define GLU_TESS_ERROR3                      100153
#define GLU_TESS_ERROR4                      100154
#define GLU_TESS_ERROR5                      100155
#define GLU_TESS_ERROR6                      100156
#define GLU_TESS_ERROR7                      100157
#define GLU_TESS_ERROR8                      100158
#define GLU_TESS_MISSING_BEGIN_POLYGON       100151
#define GLU_TESS_MISSING_BEGIN_CONTOUR       100152
#define GLU_TESS_MISSING_END_POLYGON         100153
#define GLU_TESS_MISSING_END_CONTOUR         100154
#define GLU_TESS_COORD_TOO_LARGE             100155
#define GLU_TESS_NEED_COMBINE_CALLBACK       100156

/* TessWinding */
#define GLU_TESS_WINDING_ODD                 100130
#define GLU_TESS_WINDING_NONZERO             100131
#define GLU_TESS_WINDING_POSITIVE            100132
#define GLU_TESS_WINDING_NEGATIVE            100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO         100134

#define GLU_TESS_MAX_COORD 1.0e150

typedef struct GLUtesselator GLUtesselator;

#ifdef __cplusplus
extern "C" {
#endif

extern GLUtesselator * GLAPIENTRY internal_gluNewTess(void);
extern void GLAPIENTRY internal_gluDeleteTess(GLUtesselator *tess);
extern void GLAPIENTRY internal_gluTessProperty(GLUtesselator *tess,
                                                GLenum which,
                                                GLdouble value);
extern void GLAPIENTRY internal_gluGetTessProperty(GLUtesselator *tess,
                                                   GLenum which,
                                                   GLdouble *value);
extern void GLAPIENTRY internal_gluTessNormal(GLUtesselator *tess,
                                              GLdouble x,
                                              GLdouble y,
                                              GLdouble z);
extern void GLAPIENTRY internal_gluTessCallback(GLUtesselator *tess,
                                                GLenum which,
                                                void (GLAPIENTRY *fn)());
extern void GLAPIENTRY internal_gluTessVertex(GLUtesselator *tess,
                                              GLdouble coords[3],
                                              void *data);
extern void GLAPIENTRY internal_gluTessBeginPolygon(GLUtesselator *tess,
                                                    void *data);
extern void GLAPIENTRY internal_gluTessBeginContour(GLUtesselator *tess);
extern void GLAPIENTRY internal_gluTessEndContour(GLUtesselator *tess);
extern void GLAPIENTRY internal_gluTessEndPolygon(GLUtesselator *tess);

#ifdef __cplusplus
}
#endif

#endif  // INTERNAL_GLU_H_
