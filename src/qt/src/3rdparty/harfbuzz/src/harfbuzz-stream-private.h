/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2006  Behdad Esfahbod
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
 */

#ifndef HARFBUZZ_STREAM_PRIVATE_H
#define HARFBUZZ_STREAM_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-stream.h"

HB_BEGIN_HEADER

HB_INTERNAL void
_hb_close_stream( HB_Stream stream );

HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream stream );

HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream stream,
                 HB_UInt   pos );

HB_INTERNAL HB_Error
_hb_stream_frame_enter( HB_Stream stream,
                        HB_UInt   size );

HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream stream );

/* convenience macros */

#define  SET_ERR(c)   ( (error = (c)) != 0 )

#define  GOTO_Table(tag) (0)
#define  FILE_Pos()      _hb_stream_pos( stream )
#define  FILE_Seek(pos)  SET_ERR( _hb_stream_seek( stream, pos ) )
#define  ACCESS_Frame(size)  SET_ERR( _hb_stream_frame_enter( stream, size ) )
#define  FORGET_Frame()      _hb_stream_frame_exit( stream )

#define  GET_Byte()      (*stream->cursor++)
#define  GET_Short()     (stream->cursor += 2, (HB_Short)( \
				(*(((HB_Byte*)stream->cursor)-2) << 8) | \
				 *(((HB_Byte*)stream->cursor)-1) \
			 ))
#define  GET_Long()      (stream->cursor += 4, (HB_Int)( \
				(*(((HB_Byte*)stream->cursor)-4) << 24) | \
				(*(((HB_Byte*)stream->cursor)-3) << 16) | \
				(*(((HB_Byte*)stream->cursor)-2) << 8) | \
				 *(((HB_Byte*)stream->cursor)-1) \
			 ))


#define  GET_Char()      ((HB_Char)GET_Byte())
#define  GET_UShort()    ((HB_UShort)GET_Short())
#define  GET_ULong()     ((HB_UInt)GET_Long())
#define  GET_Tag4()      GET_ULong()

HB_END_HEADER

#endif /* HARFBUZZ_STREAM_PRIVATE_H */
