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

#ifndef HARFBUZZ_BUFFER_PRIVATE_H
#define HARFBUZZ_BUFFER_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-buffer.h"

HB_BEGIN_HEADER

#define HB_GLYPH_PROPERTIES_UNKNOWN 0xFFFF

HB_INTERNAL void
_hb_buffer_swap( HB_Buffer buffer );

HB_INTERNAL void
_hb_buffer_clear_output( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_clear_positions( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_add_output_glyphs( HB_Buffer  buffer,
			      HB_UShort  num_in,
			      HB_UShort  num_out,
			      HB_UShort *glyph_data,
			      HB_UShort  component,
			      HB_UShort  ligID );

HB_INTERNAL HB_Error
_hb_buffer_add_output_glyph ( HB_Buffer buffer,
			      HB_UInt   glyph_index,
			      HB_UShort component,
			      HB_UShort ligID );

HB_INTERNAL HB_Error
_hb_buffer_copy_output_glyph ( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_replace_output_glyph ( HB_Buffer buffer,
				  HB_UInt   glyph_index,
				  HB_Bool   inplace );

HB_INTERNAL HB_UShort
_hb_buffer_allocate_ligid( HB_Buffer buffer );


/* convenience macros */

#define IN_GLYPH( pos )        (buffer->in_string[(pos)].gindex)
#define IN_ITEM( pos )         (&buffer->in_string[(pos)])
#define IN_CURGLYPH()          (buffer->in_string[buffer->in_pos].gindex)
#define IN_CURITEM()           (&buffer->in_string[buffer->in_pos])
#define IN_PROPERTIES( pos )   (buffer->in_string[(pos)].properties)
#define IN_LIGID( pos )        (buffer->in_string[(pos)].ligID)
#define IN_COMPONENT( pos )    (buffer->in_string[(pos)].component)
#define POSITION( pos )        (&buffer->positions[(pos)])
#define OUT_GLYPH( pos )       (buffer->out_string[(pos)].gindex)
#define OUT_ITEM( pos )        (&buffer->out_string[(pos)])

#define CHECK_Property( gdef, index, flags, property )					\
          ( ( error = _HB_GDEF_Check_Property( (gdef), (index), (flags),		\
                                      (property) ) ) != HB_Err_Ok )

#define ADD_String( buffer, num_in, num_out, glyph_data, component, ligID )             \
          ( ( error = _hb_buffer_add_output_glyphs( (buffer),                            \
						    (num_in), (num_out),                \
                                                    (glyph_data), (component), (ligID)  \
                                                  ) ) != HB_Err_Ok )
#define ADD_Glyph( buffer, glyph_index, component, ligID )				\
          ( ( error = _hb_buffer_add_output_glyph( (buffer),                             \
                                                    (glyph_index), (component), (ligID) \
                                                  ) ) != HB_Err_Ok )
#define REPLACE_Glyph( buffer, glyph_index, nesting_level )				\
          ( ( error = _hb_buffer_replace_output_glyph( (buffer), (glyph_index),		\
						      (nesting_level) == 1 ) ) != HB_Err_Ok )
#define COPY_Glyph( buffer )								\
	  ( (error = _hb_buffer_copy_output_glyph ( buffer ) ) != HB_Err_Ok )

HB_END_HEADER

#endif /* HARFBUZZ_BUFFER_PRIVATE_H */
