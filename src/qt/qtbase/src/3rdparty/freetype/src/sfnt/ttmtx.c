/***************************************************************************/
/*                                                                         */
/*  ttmtx.c                                                                */
/*                                                                         */
/*    Load the metrics tables common to TTF and OTF fonts (body).          */
/*                                                                         */
/*  Copyright 2006, 2007, 2008, 2009 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttmtx.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttmtx


  /*
   *  Unfortunately, we can't enable our memory optimizations if
   *  FT_CONFIG_OPTION_OLD_INTERNALS is defined.  This is because at least
   *  one rogue client (libXfont in the X.Org XServer) is directly accessing
   *  the metrics.
   */

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hmtx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hmtx' or `vmtx' table into a face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load `vmtx'.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#ifndef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hmtx( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error   error;
    FT_ULong   tag, table_size;
    FT_ULong*  ptable_offset;
    FT_ULong*  ptable_size;


    if ( vertical )
    {
      tag           = TTAG_vmtx;
      ptable_offset = &face->vert_metrics_offset;
      ptable_size   = &face->vert_metrics_size;
    }
    else
    {
      tag           = TTAG_hmtx;
      ptable_offset = &face->horz_metrics_offset;
      ptable_size   = &face->horz_metrics_size;
    }

    error = face->goto_table( face, tag, stream, &table_size );
    if ( error )
      goto Fail;

    *ptable_size   = table_size;
    *ptable_offset = FT_STREAM_POS();

  Fail:
    return error;
  }

#else /* !FT_CONFIG_OPTION_OLD_INTERNALS */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hmtx( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_ULong   table_len;
    FT_Long    num_shorts, num_longs, num_shorts_checked;

    TT_LongMetrics*    longs;
    TT_ShortMetrics**  shorts;
    FT_Byte*           p;


    if ( vertical )
    {
      void*   lm = &face->vertical.long_metrics;
      void**  sm = &face->vertical.short_metrics;


      error = face->goto_table( face, TTAG_vmtx, stream, &table_len );
      if ( error )
        goto Fail;

      num_longs = face->vertical.number_Of_VMetrics;
      if ( (FT_ULong)num_longs > table_len / 4 )
        num_longs = (FT_Long)( table_len / 4 );

      face->vertical.number_Of_VMetrics = 0;

      longs  = (TT_LongMetrics*)lm;
      shorts = (TT_ShortMetrics**)sm;
    }
    else
    {
      void*   lm = &face->horizontal.long_metrics;
      void**  sm = &face->horizontal.short_metrics;


      error = face->goto_table( face, TTAG_hmtx, stream, &table_len );
      if ( error )
        goto Fail;

      num_longs = face->horizontal.number_Of_HMetrics;
      if ( (FT_ULong)num_longs > table_len / 4 )
        num_longs = (FT_Long)( table_len / 4 );

      face->horizontal.number_Of_HMetrics = 0;

      longs  = (TT_LongMetrics*)lm;
      shorts = (TT_ShortMetrics**)sm;
    }

    /* never trust derived values */

    num_shorts         = face->max_profile.numGlyphs - num_longs;
    num_shorts_checked = ( table_len - num_longs * 4L ) / 2;

    if ( num_shorts < 0 )
    {
      FT_TRACE0(( "tt_face_load_hmtx:"
                  " %cmtx has more metrics than glyphs.\n",
                  vertical ? "v" : "h" ));

      /* Adobe simply ignores this problem.  So we shall do the same. */
#if 0
      error = vertical ? SFNT_Err_Invalid_Vert_Metrics
                       : SFNT_Err_Invalid_Horiz_Metrics;
      goto Exit;
#else
      num_shorts = 0;
#endif
    }

    if ( FT_QNEW_ARRAY( *longs,  num_longs  ) ||
         FT_QNEW_ARRAY( *shorts, num_shorts ) )
      goto Fail;

    if ( FT_FRAME_ENTER( table_len ) )
      goto Fail;

    p = stream->cursor;

    {
      TT_LongMetrics  cur   = *longs;
      TT_LongMetrics  limit = cur + num_longs;


      for ( ; cur < limit; cur++ )
      {
        cur->advance = FT_NEXT_USHORT( p );
        cur->bearing = FT_NEXT_SHORT( p );
      }
    }

    /* do we have an inconsistent number of metric values? */
    {
      TT_ShortMetrics*  cur   = *shorts;
      TT_ShortMetrics*  limit = cur +
                                FT_MIN( num_shorts, num_shorts_checked );


      for ( ; cur < limit; cur++ )
        *cur = FT_NEXT_SHORT( p );

      /* We fill up the missing left side bearings with the     */
      /* last valid value.  Since this will occur for buggy CJK */
      /* fonts usually only, nothing serious will happen.       */
      if ( num_shorts > num_shorts_checked && num_shorts_checked > 0 )
      {
        FT_Short  val = (*shorts)[num_shorts_checked - 1];


        limit = *shorts + num_shorts;
        for ( ; cur < limit; cur++ )
          *cur = val;
      }
    }

    FT_FRAME_EXIT();

    if ( vertical )
      face->vertical.number_Of_VMetrics = (FT_UShort)num_longs;
    else
      face->horizontal.number_Of_HMetrics = (FT_UShort)num_longs;

  Fail:
    return error;
  }

#endif /* !FT_CONFIG_OPTION_OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hhea                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hhea' or 'vhea' table into a face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load `vhea'.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hhea( TT_Face    face,
                     FT_Stream  stream,
                     FT_Bool    vertical )
  {
    FT_Error        error;
    TT_HoriHeader*  header;

    const FT_Frame_Field  metrics_header_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_HoriHeader

      FT_FRAME_START( 36 ),
        FT_FRAME_ULONG ( Version ),
        FT_FRAME_SHORT ( Ascender ),
        FT_FRAME_SHORT ( Descender ),
        FT_FRAME_SHORT ( Line_Gap ),
        FT_FRAME_USHORT( advance_Width_Max ),
        FT_FRAME_SHORT ( min_Left_Side_Bearing ),
        FT_FRAME_SHORT ( min_Right_Side_Bearing ),
        FT_FRAME_SHORT ( xMax_Extent ),
        FT_FRAME_SHORT ( caret_Slope_Rise ),
        FT_FRAME_SHORT ( caret_Slope_Run ),
        FT_FRAME_SHORT ( caret_Offset ),
        FT_FRAME_SHORT ( Reserved[0] ),
        FT_FRAME_SHORT ( Reserved[1] ),
        FT_FRAME_SHORT ( Reserved[2] ),
        FT_FRAME_SHORT ( Reserved[3] ),
        FT_FRAME_SHORT ( metric_Data_Format ),
        FT_FRAME_USHORT( number_Of_HMetrics ),
      FT_FRAME_END
    };


    if ( vertical )
    {
      void  *v = &face->vertical;


      error = face->goto_table( face, TTAG_vhea, stream, 0 );
      if ( error )
        goto Fail;

      header = (TT_HoriHeader*)v;
    }
    else
    {
      error = face->goto_table( face, TTAG_hhea, stream, 0 );
      if ( error )
        goto Fail;

      header = &face->horizontal;
    }

    if ( FT_STREAM_READ_FIELDS( metrics_header_fields, header ) )
      goto Fail;

    FT_TRACE3(( "Ascender:          %5d\n", header->Ascender ));
    FT_TRACE3(( "Descender:         %5d\n", header->Descender ));
    FT_TRACE3(( "number_Of_Metrics: %5u\n", header->number_Of_HMetrics ));

    header->long_metrics  = NULL;
    header->short_metrics = NULL;

  Fail:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_get_metrics                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the horizontal or vertical metrics in font units for a     */
  /*    given glyph.  The metrics are the left side bearing (resp. top     */
  /*    side bearing) and advance width (resp. advance height).            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    header  :: A pointer to either the horizontal or vertical metrics  */
  /*               structure.                                              */
  /*                                                                       */
  /*    idx     :: The glyph index.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    bearing :: The bearing, either left side or top side.              */
  /*                                                                       */
  /*    advance :: The advance width resp. advance height.                 */
  /*                                                                       */
#ifndef FT_CONFIG_OPTION_OLD_INTERNALS

  FT_LOCAL_DEF( FT_Error )
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     gindex,
                       FT_Short   *abearing,
                       FT_UShort  *aadvance )
  {
    FT_Error        error;
    FT_Stream       stream = face->root.stream;
    TT_HoriHeader*  header;
    FT_ULong        table_pos, table_size, table_end;
    FT_UShort       k;


    if ( vertical )
    {
      void*  v = &face->vertical;


      header     = (TT_HoriHeader*)v;
      table_pos  = face->vert_metrics_offset;
      table_size = face->vert_metrics_size;
    }
    else
    {
      header     = &face->horizontal;
      table_pos  = face->horz_metrics_offset;
      table_size = face->horz_metrics_size;
    }

    table_end = table_pos + table_size;

    k = header->number_Of_HMetrics;

    if ( k > 0 )
    {
      if ( gindex < (FT_UInt)k )
      {
        table_pos += 4 * gindex;
        if ( table_pos + 4 > table_end )
          goto NoData;

        if ( FT_STREAM_SEEK( table_pos ) ||
             FT_READ_USHORT( *aadvance ) ||
             FT_READ_SHORT( *abearing )  )
          goto NoData;
      }
      else
      {
        table_pos += 4 * ( k - 1 );
        if ( table_pos + 4 > table_end )
          goto NoData;

        if ( FT_STREAM_SEEK( table_pos ) ||
             FT_READ_USHORT( *aadvance ) )
          goto NoData;

        table_pos += 4 + 2 * ( gindex - k );
        if ( table_pos + 2 > table_end )
          *abearing = 0;
        else
        {
          if ( !FT_STREAM_SEEK( table_pos ) )
            (void)FT_READ_SHORT( *abearing );
        }
      }
    }
    else
    {
    NoData:
      *abearing = 0;
      *aadvance = 0;
    }

    return SFNT_Err_Ok;
  }

#else /* !FT_CONFIG_OPTION_OLD_INTERNALS */

  FT_LOCAL_DEF( FT_Error )
  tt_face_get_metrics( TT_Face     face,
                       FT_Bool     vertical,
                       FT_UInt     gindex,
                       FT_Short*   abearing,
                       FT_UShort*  aadvance )
  {
    void*           v = &face->vertical;
    void*           h = &face->horizontal;
    TT_HoriHeader*  header = vertical ? (TT_HoriHeader*)v
                                      : (TT_HoriHeader*)h;
    TT_LongMetrics  longs_m;
    FT_UShort       k = header->number_Of_HMetrics;


    if ( k == 0                                         ||
         !header->long_metrics                          ||
         gindex >= (FT_UInt)face->max_profile.numGlyphs )
    {
      *abearing = *aadvance = 0;
      return SFNT_Err_Ok;
    }

    if ( gindex < (FT_UInt)k )
    {
      longs_m   = (TT_LongMetrics)header->long_metrics + gindex;
      *abearing = longs_m->bearing;
      *aadvance = longs_m->advance;
    }
    else
    {
      *abearing = ((TT_ShortMetrics*)header->short_metrics)[gindex - k];
      *aadvance = ((TT_LongMetrics)header->long_metrics)[k - 1].advance;
    }

    return SFNT_Err_Ok;
  }

#endif /* !FT_CONFIG_OPTION_OLD_INTERNALS */


/* END */
