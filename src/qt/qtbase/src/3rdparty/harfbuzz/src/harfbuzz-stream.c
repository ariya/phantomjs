/*
 * Copyright (C) 2005  David Turner
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2007  Red Hat, Inc.
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include <stdlib.h>

#if 0
#include <stdio.h>
#define  LOG(x)  _hb_log x

static void
_hb_log( const char*   format, ... )
{
  va_list  ap;
 
  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );
}

#else
#define  LOG(x)  do {} while (0)
#endif

HB_INTERNAL void
_hb_close_stream( HB_Stream stream )
{
  if (!stream)
      return;
  free(stream->base);
  free(stream);
}


HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream stream )
{
  LOG(( "_hb_stream_pos() -> %ld\n", stream->pos ));
  return stream->pos;
}


HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream stream,
		 HB_UInt pos )
{
  HB_Error  error = (HB_Error)0;

  stream->pos = pos;
  if (pos > stream->size)
      error = ERR(HB_Err_Read_Error);

  LOG(( "_hb_stream_seek(%ld) -> 0x%04X\n", pos, error ));
  return error;
}


HB_INTERNAL HB_Error
_hb_stream_frame_enter( HB_Stream stream,
			HB_UInt count )
{
  HB_Error  error = HB_Err_Ok;

  /* check new position, watch for overflow */
  if (HB_UNLIKELY (stream->pos + count > stream->size ||
		   stream->pos + count < stream->pos))
  {
    error = ERR(HB_Err_Read_Error);
    goto Exit;
  }

  /* set cursor */
  stream->cursor = stream->base + stream->pos;
  stream->pos   += count;

Exit:
  LOG(( "_hb_stream_frame_enter(%ld) -> 0x%04X\n", count, error ));
  return error;
}


HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream stream )
{
  stream->cursor = NULL;

  LOG(( "_hb_stream_frame_exit()\n" ));
}
