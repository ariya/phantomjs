/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2004,2007  Red Hat, Inc.
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
 * Red Hat Author(s): Owen Taylor, Behdad Esfahbod
 */

#ifndef HARFBUZZ_BUFFER_H
#define HARFBUZZ_BUFFER_H

#include "harfbuzz-global.h"

HB_BEGIN_HEADER

typedef struct HB_GlyphItemRec_ {
  HB_UInt     gindex;
  HB_UInt     properties;
  HB_UInt     cluster;
  HB_UShort   component;
  HB_UShort   ligID;
  HB_UShort   gproperties;
} HB_GlyphItemRec, *HB_GlyphItem;

typedef struct HB_PositionRec_ {
  HB_Fixed   x_pos;
  HB_Fixed   y_pos;
  HB_Fixed   x_advance;
  HB_Fixed   y_advance;
  HB_UShort  back;            /* number of glyphs to go back
				 for drawing current glyph   */
  HB_Short  cursive_chain;   /* character to which this connects,
				 may be positive or negative; used
				 only internally                     */
  HB_Bool    new_advance;     /* if set, the advance width values are
				 absolute, i.e., they won't be
				 added to the original glyph's value
				 but rather replace them.            */
} HB_PositionRec, *HB_Position;


typedef struct HB_BufferRec_{ 
  HB_UInt    allocated;

  HB_UInt    in_length;
  HB_UInt    out_length;
  HB_UInt    in_pos;
  HB_UInt    out_pos;
  
  HB_GlyphItem  in_string;
  HB_GlyphItem  out_string;
  HB_GlyphItem  alt_string;
  HB_Position   positions;
  HB_UShort      max_ligID;
  HB_Bool       separate_out;
} HB_BufferRec, *HB_Buffer;

HB_Error
hb_buffer_new( HB_Buffer *buffer );

void
hb_buffer_free( HB_Buffer buffer );

void
hb_buffer_clear( HB_Buffer buffer );

HB_Error
hb_buffer_add_glyph( HB_Buffer buffer,
		      HB_UInt    glyph_index,
		      HB_UInt    properties,
		      HB_UInt    cluster );

HB_END_HEADER

#endif /* HARFBUZZ_BUFFER_H */
