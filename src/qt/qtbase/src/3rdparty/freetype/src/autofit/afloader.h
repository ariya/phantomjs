/***************************************************************************/
/*                                                                         */
/*  afloader.h                                                             */
/*                                                                         */
/*    Auto-fitter glyph loading routines (specification).                  */
/*                                                                         */
/*  Copyright 2003, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AF_LOADER_H__
#define __AF_LOADER_H__

#include "afhints.h"
#include "afglobal.h"


FT_BEGIN_HEADER

  typedef struct AF_LoaderRec_
  {
    FT_Face           face;           /* current face */
    AF_FaceGlobals    globals;        /* current face globals */
    FT_GlyphLoader    gloader;        /* glyph loader */
    AF_GlyphHintsRec  hints;
    AF_ScriptMetrics  metrics;
    FT_Bool           transformed;
    FT_Matrix         trans_matrix;
    FT_Vector         trans_delta;
    FT_Vector         pp1;
    FT_Vector         pp2;
    /* we don't handle vertical phantom points */

  } AF_LoaderRec, *AF_Loader;


  FT_LOCAL( FT_Error )
  af_loader_init( AF_Loader  loader,
                  FT_Memory  memory );


  FT_LOCAL( FT_Error )
  af_loader_reset( AF_Loader  loader,
                   FT_Face    face );


  FT_LOCAL( void )
  af_loader_done( AF_Loader  loader );


  FT_LOCAL( FT_Error )
  af_loader_load_glyph( AF_Loader  loader,
                        FT_Face    face,
                        FT_UInt    gindex,
                        FT_UInt32  load_flags );

/* */


FT_END_HEADER

#endif /* __AF_LOADER_H__ */


/* END */
