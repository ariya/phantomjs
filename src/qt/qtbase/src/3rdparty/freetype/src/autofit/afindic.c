/***************************************************************************/
/*                                                                         */
/*  afindic.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines for Indic scripts (body).               */
/*                                                                         */
/*  Copyright 2007 by                                                      */
/*  Rahul Bhalerao <rahul.bhalerao@redhat.com>, <b.rahul.pm@gmail.com>.    */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aftypes.h"
#include "aflatin.h"


#ifdef AF_CONFIG_OPTION_INDIC

#include "afindic.h"
#include "aferrors.h"
#include "afcjk.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  static FT_Error
  af_indic_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face )
  {
    /* use CJK routines */
    return af_cjk_metrics_init( metrics, face );
  }


  static void
  af_indic_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler )
  {
    /* use CJK routines */
    af_cjk_metrics_scale( metrics, scaler );
  }


  static FT_Error
  af_indic_hints_init( AF_GlyphHints    hints,
                       AF_LatinMetrics  metrics )
  {
    /* use CJK routines */
    return af_cjk_hints_init( hints, metrics );
  }


  static FT_Error
  af_indic_hints_apply( AF_GlyphHints    hints,
                        FT_Outline*      outline,
                        AF_LatinMetrics  metrics)
  {
    /* use CJK routines */
    return af_cjk_hints_apply( hints, outline, metrics );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                I N D I C   S C R I P T   C L A S S            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
#if 0
    AF_UNIRANGE_REC( 0x0100UL, 0xFFFFUL ),  /* why this? */
#endif
    AF_UNIRANGE_REC( 0x0900UL, 0x0DFFUL),    /* Indic Range */
    AF_UNIRANGE_REC(      0UL,      0UL)
  };


  AF_DEFINE_SCRIPT_CLASS(af_indic_script_class,
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_indic_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_indic_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_indic_hints_init,
    (AF_Script_ApplyHintsFunc)  af_indic_hints_apply
  )

#else /* !AF_CONFIG_OPTION_INDIC */

  static const AF_Script_UniRangeRec  af_indic_uniranges[] =
  {
    { 0, 0 }
  };


  AF_DEFINE_SCRIPT_CLASS(af_indic_script_class,
    AF_SCRIPT_INDIC,
    af_indic_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) NULL,
    (AF_Script_ScaleMetricsFunc)NULL,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   NULL,
    (AF_Script_ApplyHintsFunc)  NULL
  )

#endif /* !AF_CONFIG_OPTION_INDIC */


/* END */
