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

#include "harfbuzz-impl.h"
#include "harfbuzz-buffer-private.h"
#include "harfbuzz-gsub-private.h"
#include "harfbuzz-gpos-private.h"

/* Here is how the buffer works internally:
 *
 * There are two string pointers: in_string and out_string.  They
 * always have same allocated size, but different length and positions.
 *
 * As an optimization, both in_string and out_string may point to the
 * same piece of memory, which is owned by in_string.  This remains the
 * case as long as:
 *
 *   - copy_glyph() is called
 *   - replace_glyph() is called with inplace=TRUE
 *   - add_output_glyph() and add_output_glyphs() are not called
 *
 * In that case swap(), and copy_glyph(), and replace_glyph() are all
 * mostly no-op.
 *
 * As soon an add_output_glyph[s]() or replace_glyph() with inplace=FALSE is
 * called, out_string is moved over to an alternate buffer (alt_string), and
 * its current contents (out_length entries) are copied to the alt buffer.
 * This should all remain transparent to the user.  swap() then switches
 * in_string and alt_string.  alt_string is not allocated until its needed,
 * but after that it's grown with in_string unconditionally.
 *
 * The buffer->separate_out boolean keeps status of whether out_string points
 * to in_string (FALSE) or alt_string (TRUE).
 */

/* Internal API */

static HB_Error
hb_buffer_ensure( HB_Buffer buffer,
		   HB_UInt   size )
{
  HB_UInt new_allocated = buffer->allocated;

  if (size > new_allocated)
    {
      HB_Error error;

      while (size > new_allocated)
	new_allocated += (new_allocated >> 1) + 8;
      
      if ( buffer->positions )
        {
	  if ( REALLOC_ARRAY( buffer->positions, new_allocated, HB_PositionRec ) )
	    return error;
	}

      if ( REALLOC_ARRAY( buffer->in_string, new_allocated, HB_GlyphItemRec ) )
	return error;

      if ( buffer->separate_out )
        {
	  if ( REALLOC_ARRAY( buffer->alt_string, new_allocated, HB_GlyphItemRec ) )
	    return error;

	  buffer->out_string = buffer->alt_string;
	}
      else
        {
	  buffer->out_string = buffer->in_string;

	  if ( buffer->alt_string )
	    {
	      if ( REALLOC_ARRAY( buffer->alt_string, new_allocated, HB_GlyphItemRec ) )
		return error;
	    }
	}

      buffer->allocated = new_allocated;
    }

  return HB_Err_Ok;
}

static HB_Error
hb_buffer_duplicate_out_buffer( HB_Buffer buffer )
{
  if ( !buffer->alt_string )
    {
      HB_Error error;

      if ( ALLOC_ARRAY( buffer->alt_string, buffer->allocated, HB_GlyphItemRec ) )
	return error;
    }

  buffer->out_string = buffer->alt_string;
  memcpy( buffer->out_string, buffer->in_string, buffer->out_length * sizeof (buffer->out_string[0]) );
  buffer->separate_out = TRUE;

  return HB_Err_Ok;
}

/* Public API */

HB_Error
hb_buffer_new( HB_Buffer *pbuffer )
{
  HB_Buffer buffer;
  HB_Error error;

  if ( ALLOC( buffer, sizeof( HB_BufferRec ) ) )
    return error;

  buffer->allocated = 0;
  buffer->in_string = NULL;
  buffer->alt_string = NULL;
  buffer->positions = NULL;

  hb_buffer_clear( buffer );

  *pbuffer = buffer;

  return HB_Err_Ok;
}

void
hb_buffer_free( HB_Buffer buffer )
{
  FREE( buffer->in_string );
  FREE( buffer->alt_string );
  buffer->out_string = NULL;
  FREE( buffer->positions );
  FREE( buffer );
}

void
hb_buffer_clear( HB_Buffer buffer )
{
  buffer->in_length = 0;
  buffer->out_length = 0;
  buffer->in_pos = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
  buffer->max_ligID = 0;
}

HB_Error
hb_buffer_add_glyph( HB_Buffer buffer,
		      HB_UInt   glyph_index,
		      HB_UInt   properties,
		      HB_UInt   cluster )
{
  HB_Error error;
  HB_GlyphItem glyph;
  
  error = hb_buffer_ensure( buffer, buffer->in_length + 1 );
  if ( error )
    return error;

  glyph = &buffer->in_string[buffer->in_length];
  glyph->gindex = glyph_index;
  glyph->properties = properties;
  glyph->cluster = cluster;
  glyph->component = 0;
  glyph->ligID = 0;
  glyph->gproperties = HB_GLYPH_PROPERTIES_UNKNOWN;
  
  buffer->in_length++;

  return HB_Err_Ok;
}

/* HarfBuzz-Internal API */

HB_INTERNAL void
_hb_buffer_clear_output( HB_Buffer buffer )
{
  buffer->out_length = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
}

HB_INTERNAL HB_Error
_hb_buffer_clear_positions( HB_Buffer buffer )
{
  if ( !buffer->positions )
    {
      HB_Error error;

      if ( ALLOC_ARRAY( buffer->positions, buffer->allocated, HB_PositionRec ) )
	return error;
    }

  memset (buffer->positions, 0, sizeof (buffer->positions[0]) * buffer->in_length);

  return HB_Err_Ok;
}

HB_INTERNAL void
_hb_buffer_swap( HB_Buffer buffer )
{
  HB_GlyphItem tmp_string;
  int tmp_length;
  int tmp_pos;

  if ( buffer->separate_out )
    {
      tmp_string = buffer->in_string;
      buffer->in_string = buffer->out_string;
      buffer->out_string = tmp_string;
      buffer->alt_string = buffer->out_string;
    }

  tmp_length = buffer->in_length;
  buffer->in_length = buffer->out_length;
  buffer->out_length = tmp_length;

  tmp_pos = buffer->in_pos;
  buffer->in_pos = buffer->out_pos;
  buffer->out_pos = tmp_pos;
}

/* The following function copies `num_out' elements from `glyph_data'
   to `buffer->out_string', advancing the in array pointer in the structure
   by `num_in' elements, and the out array pointer by `num_out' elements.
   Finally, it sets the `length' field of `out' equal to
   `pos' of the `out' structure.

   If `component' is 0xFFFF, the component value from buffer->in_pos
   will copied `num_out' times, otherwise `component' itself will
   be used to fill the `component' fields.

   If `ligID' is 0xFFFF, the ligID value from buffer->in_pos
   will copied `num_out' times, otherwise `ligID' itself will
   be used to fill the `ligID' fields.

   The properties for all replacement glyphs are taken
   from the glyph at position `buffer->in_pos'.

   The cluster value for the glyph at position buffer->in_pos is used
   for all replacement glyphs */
HB_INTERNAL HB_Error
_hb_buffer_add_output_glyphs( HB_Buffer  buffer,
			      HB_UShort  num_in,
			      HB_UShort  num_out,
			      HB_UShort *glyph_data,
			      HB_UShort  component,
			      HB_UShort  ligID )
{
  HB_Error  error;
  HB_UShort i;
  HB_UInt properties;
  HB_UInt cluster;

  error = hb_buffer_ensure( buffer, buffer->out_pos + num_out );
  if ( error )
    return error;

  if ( !buffer->separate_out )
    {
      error = hb_buffer_duplicate_out_buffer( buffer );
      if ( error )
	return error;
    }

  properties = buffer->in_string[buffer->in_pos].properties;
  cluster = buffer->in_string[buffer->in_pos].cluster;
  if ( component == 0xFFFF )
    component = buffer->in_string[buffer->in_pos].component;
  if ( ligID == 0xFFFF )
    ligID = buffer->in_string[buffer->in_pos].ligID;

  for ( i = 0; i < num_out; i++ )
  {
    HB_GlyphItem item = &buffer->out_string[buffer->out_pos + i];

    item->gindex = glyph_data[i];
    item->properties = properties;
    item->cluster = cluster;
    item->component = component;
    item->ligID = ligID;
    item->gproperties = HB_GLYPH_PROPERTIES_UNKNOWN;
  }

  buffer->in_pos  += num_in;
  buffer->out_pos += num_out;

  buffer->out_length = buffer->out_pos;

  return HB_Err_Ok;
}

HB_INTERNAL HB_Error
_hb_buffer_add_output_glyph( HB_Buffer buffer,
			     HB_UInt   glyph_index,
			     HB_UShort component,
			     HB_UShort ligID )
{
  HB_UShort glyph_data =  glyph_index;

  return _hb_buffer_add_output_glyphs ( buffer, 1, 1,
					&glyph_data, component, ligID );
}

HB_INTERNAL HB_Error
_hb_buffer_copy_output_glyph ( HB_Buffer buffer )
{  
  HB_Error  error;

  error = hb_buffer_ensure( buffer, buffer->out_pos + 1 );
  if ( error )
    return error;
  
  if ( buffer->separate_out )
    {
      buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];
    }

  buffer->in_pos++;
  buffer->out_pos++;
  buffer->out_length = buffer->out_pos;

  return HB_Err_Ok;
}

HB_INTERNAL HB_Error
_hb_buffer_replace_output_glyph( HB_Buffer buffer,
				 HB_UInt   glyph_index,
				 HB_Bool   inplace )
{

  HB_Error error;

  if ( inplace )
    {
      error = _hb_buffer_copy_output_glyph ( buffer );
      if ( error )
	return error;

      buffer->out_string[buffer->out_pos-1].gindex = glyph_index;
    }
  else
    {
      return _hb_buffer_add_output_glyph( buffer, glyph_index, 0xFFFF, 0xFFFF );
    }

  return HB_Err_Ok;
}

HB_INTERNAL HB_UShort
_hb_buffer_allocate_ligid( HB_Buffer buffer )
{
  buffer->max_ligID++;
  if (HB_UNLIKELY (buffer->max_ligID == 0))
    buffer->max_ligID++;

  return buffer->max_ligID;
}
