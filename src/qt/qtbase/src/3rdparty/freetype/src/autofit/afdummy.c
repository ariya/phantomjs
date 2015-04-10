/***************************************************************************/
/*                                                                         */
/*  afdummy.c                                                              */
/*                                                                         */
/*    Auto-fitter dummy routines to be used if no hinting should be        */
/*    performed (body).                                                    */
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


#include "afdummy.h"
#include "afhints.h"


  static FT_Error
  af_dummy_hints_init( AF_GlyphHints     hints,
                       AF_ScriptMetrics  metrics )
  {
    af_glyph_hints_rescale( hints,
                            metrics );
    return 0;
  }


  static FT_Error
  af_dummy_hints_apply( AF_GlyphHints  hints,
                        FT_Outline*    outline )
  {
    FT_UNUSED( hints );
    FT_UNUSED( outline );

    return 0;
  }


  AF_DEFINE_SCRIPT_CLASS(af_dummy_script_class,
    AF_SCRIPT_NONE,
    NULL,

    sizeof( AF_ScriptMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_dummy_hints_init,
    (AF_Script_ApplyHintsFunc)  af_dummy_hints_apply
  )


/* END */
