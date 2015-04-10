/***************************************************************************/
/*                                                                         */
/*  afcjk.h                                                                */
/*                                                                         */
/*    Auto-fitter hinting routines for CJK script (specification).         */
/*                                                                         */
/*  Copyright 2006, 2007 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AFCJK_H__
#define __AFCJK_H__

#include "afhints.h"


FT_BEGIN_HEADER


  /* the CJK-specific script class */

  AF_DECLARE_SCRIPT_CLASS(af_cjk_script_class)


  FT_LOCAL( FT_Error )
  af_cjk_metrics_init( AF_LatinMetrics  metrics,
                       FT_Face          face );

  FT_LOCAL( void )
  af_cjk_metrics_scale( AF_LatinMetrics  metrics,
                        AF_Scaler        scaler );

  FT_LOCAL( FT_Error )
  af_cjk_hints_init( AF_GlyphHints    hints,
                     AF_LatinMetrics  metrics );

  FT_LOCAL( FT_Error )
  af_cjk_hints_apply( AF_GlyphHints    hints,
                      FT_Outline*      outline,
                      AF_LatinMetrics  metrics );

/* */

FT_END_HEADER

#endif /* __AFCJK_H__ */


/* END */
