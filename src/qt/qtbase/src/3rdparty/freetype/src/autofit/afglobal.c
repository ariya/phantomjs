/***************************************************************************/
/*                                                                         */
/*  afglobal.c                                                             */
/*                                                                         */
/*    Auto-fitter routines to compute global hinting values (body).        */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009 by                  */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "afglobal.h"
#include "afdummy.h"
#include "aflatin.h"
#include "afcjk.h"
#include "afindic.h"
#include "afpic.h"

#include "aferrors.h"

#ifdef FT_OPTION_AUTOFIT2
#include "aflatin2.h"
#endif

#ifndef FT_CONFIG_OPTION_PIC

/* when updating this table, don't forget to update 
  AF_SCRIPT_CLASSES_COUNT and autofit_module_class_pic_init */

  /* populate this list when you add new scripts */
  static AF_ScriptClass const  af_script_classes[] =
  {
    &af_dummy_script_class,
#ifdef FT_OPTION_AUTOFIT2
    &af_latin2_script_class,
#endif
    &af_latin_script_class,
    &af_cjk_script_class,
    &af_indic_script_class, 
    NULL  /* do not remove */
  };

#endif /* FT_CONFIG_OPTION_PIC */

  /* index of default script in `af_script_classes' */
#define AF_SCRIPT_LIST_DEFAULT  2
  /* a bit mask indicating an uncovered glyph       */
#define AF_SCRIPT_LIST_NONE     0x7F
  /* if this flag is set, we have an ASCII digit    */
#define AF_DIGIT                0x80


  /*
   *  Note that glyph_scripts[] is used to map each glyph into
   *  an index into the `af_script_classes' array.
   *
   */
  typedef struct  AF_FaceGlobalsRec_
  {
    FT_Face           face;
    FT_Long           glyph_count;    /* same as face->num_glyphs */
    FT_Byte*          glyph_scripts;

    AF_ScriptMetrics  metrics[AF_SCRIPT_MAX];

  } AF_FaceGlobalsRec;


  /* Compute the script index of each glyph within a given face. */

  static FT_Error
  af_face_globals_compute_script_coverage( AF_FaceGlobals  globals )
  {
    FT_Error    error       = AF_Err_Ok;
    FT_Face     face        = globals->face;
    FT_CharMap  old_charmap = face->charmap;
    FT_Byte*    gscripts    = globals->glyph_scripts;
    FT_UInt     ss, i;


    /* the value 255 means `uncovered glyph' */
    FT_MEM_SET( globals->glyph_scripts,
                AF_SCRIPT_LIST_NONE,
                globals->glyph_count );

    error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );
    if ( error )
    {
     /*
      *  Ignore this error; we simply use the default script.
      *  XXX: Shouldn't we rather disable hinting?
      */
      error = AF_Err_Ok;
      goto Exit;
    }

    /* scan each script in a Unicode charmap */
    for ( ss = 0; AF_SCRIPT_CLASSES_GET[ss]; ss++ )
    {
      AF_ScriptClass      clazz = AF_SCRIPT_CLASSES_GET[ss];
      AF_Script_UniRange  range;


      if ( clazz->script_uni_ranges == NULL )
        continue;

      /*
       *  Scan all unicode points in the range and set the corresponding
       *  glyph script index.
       */
      for ( range = clazz->script_uni_ranges; range->first != 0; range++ )
      {
        FT_ULong  charcode = range->first;
        FT_UInt   gindex;


        gindex = FT_Get_Char_Index( face, charcode );

        if ( gindex != 0                             &&
             gindex < (FT_ULong)globals->glyph_count &&
             gscripts[gindex] == AF_SCRIPT_LIST_NONE )
        {
          gscripts[gindex] = (FT_Byte)ss;
        }

        for (;;)
        {
          charcode = FT_Get_Next_Char( face, charcode, &gindex );

          if ( gindex == 0 || charcode > range->last )
            break;

          if ( gindex < (FT_ULong)globals->glyph_count &&
               gscripts[gindex] == AF_SCRIPT_LIST_NONE )
          {
            gscripts[gindex] = (FT_Byte)ss;
          }
        }
      }
    }

    /* mark ASCII digits */
    for ( i = 0x30; i <= 0x39; i++ )
    {
      FT_UInt  gindex = FT_Get_Char_Index( face, i );


      if ( gindex != 0 && gindex < (FT_ULong)globals->glyph_count )
        gscripts[gindex] |= AF_DIGIT;
    }

  Exit:
    /*
     *  By default, all uncovered glyphs are set to the latin script.
     *  XXX: Shouldn't we disable hinting or do something similar?
     */
    {
      FT_Long  nn;


      for ( nn = 0; nn < globals->glyph_count; nn++ )
      {
        if ( gscripts[nn] == AF_SCRIPT_LIST_NONE )
          gscripts[nn] = AF_SCRIPT_LIST_DEFAULT;
      }
    }

    FT_Set_Charmap( face, old_charmap );
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals )
  {
    FT_Error        error;
    FT_Memory       memory;
    AF_FaceGlobals  globals;


    memory = face->memory;

    if ( !FT_ALLOC( globals, sizeof ( *globals ) +
                             face->num_glyphs * sizeof ( FT_Byte ) ) )
    {
      globals->face          = face;
      globals->glyph_count   = face->num_glyphs;
      globals->glyph_scripts = (FT_Byte*)( globals + 1 );

      error = af_face_globals_compute_script_coverage( globals );
      if ( error )
      {
        af_face_globals_free( globals );
        globals = NULL;
      }
    }

    *aglobals = globals;
    return error;
  }


  FT_LOCAL_DEF( void )
  af_face_globals_free( AF_FaceGlobals  globals )
  {
    if ( globals )
    {
      FT_Memory  memory = globals->face->memory;
      FT_UInt    nn;


      for ( nn = 0; nn < AF_SCRIPT_MAX; nn++ )
      {
        if ( globals->metrics[nn] )
        {
          AF_ScriptClass  clazz = AF_SCRIPT_CLASSES_GET[nn];


          FT_ASSERT( globals->metrics[nn]->clazz == clazz );

          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( globals->metrics[nn] );

          FT_FREE( globals->metrics[nn] );
        }
      }

      globals->glyph_count   = 0;
      globals->glyph_scripts = NULL;  /* no need to free this one! */
      globals->face          = NULL;

      FT_FREE( globals );
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               FT_UInt            options,
                               AF_ScriptMetrics  *ametrics )
  {
    AF_ScriptMetrics  metrics = NULL;
    FT_UInt           gidx;
    AF_ScriptClass    clazz;
    FT_UInt           script     = options & 15;
    const FT_Offset   script_max = sizeof ( AF_SCRIPT_CLASSES_GET ) /
                                     sizeof ( AF_SCRIPT_CLASSES_GET[0] );
    FT_Error          error      = AF_Err_Ok;


    if ( gindex >= (FT_ULong)globals->glyph_count )
    {
      error = AF_Err_Invalid_Argument;
      goto Exit;
    }

    gidx = script;
    if ( gidx == 0 || gidx + 1 >= script_max )
      gidx = globals->glyph_scripts[gindex] & AF_SCRIPT_LIST_NONE;

    clazz = AF_SCRIPT_CLASSES_GET[gidx];
    if ( script == 0 )
      script = clazz->script;

    metrics = globals->metrics[clazz->script];
    if ( metrics == NULL )
    {
      /* create the global metrics object when needed */
      FT_Memory  memory = globals->face->memory;


      if ( FT_ALLOC( metrics, clazz->script_metrics_size ) )
        goto Exit;

      metrics->clazz = clazz;

      if ( clazz->script_metrics_init )
      {
        error = clazz->script_metrics_init( metrics, globals->face );
        if ( error )
        {
          if ( clazz->script_metrics_done )
            clazz->script_metrics_done( metrics );

          FT_FREE( metrics );
          goto Exit;
        }
      }

      globals->metrics[clazz->script] = metrics;
    }

  Exit:
    *ametrics = metrics;

    return error;
  }


  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex )
  {
    if ( gindex < (FT_ULong)globals->glyph_count )
      return (FT_Bool)( globals->glyph_scripts[gindex] & AF_DIGIT );

    return (FT_Bool)0;
  }


/* END */
