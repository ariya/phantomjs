/***************************************************************************/
/*                                                                         */
/*  afglobal.h                                                             */
/*                                                                         */
/*    Auto-fitter routines to compute global hinting values                */
/*    (specification).                                                     */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2007, 2009 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AF_GLOBAL_H__
#define __AF_GLOBAL_H__


#include "aftypes.h"


FT_BEGIN_HEADER


  /************************************************************************/
  /************************************************************************/
  /*****                                                              *****/
  /*****                  F A C E   G L O B A L S                     *****/
  /*****                                                              *****/
  /************************************************************************/
  /************************************************************************/


  /*
   *  model the global hints data for a given face, decomposed into
   *  script-specific items
   */
  typedef struct AF_FaceGlobalsRec_*   AF_FaceGlobals;


  FT_LOCAL( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals );

  FT_LOCAL( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               FT_UInt            options,
                               AF_ScriptMetrics  *ametrics );

  FT_LOCAL( void )
  af_face_globals_free( AF_FaceGlobals  globals );

  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex );

  /* */


FT_END_HEADER

#endif /* __AF_GLOBALS_H__ */


/* END */
