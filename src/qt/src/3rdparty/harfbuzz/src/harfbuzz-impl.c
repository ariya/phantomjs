/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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


HB_INTERNAL HB_Pointer
_hb_alloc(size_t     size,
	  HB_Error  *perror )
{
  HB_Error    error = (HB_Error)0;
  HB_Pointer  block = NULL;

  if ( size > 0 )
  {
    block = calloc( 1, size );
    if ( !block )
      error = ERR(HB_Err_Out_Of_Memory);
  }

  *perror = error;
  return block;
}


HB_INTERNAL HB_Pointer
_hb_realloc(HB_Pointer  block,
	    size_t      new_size,
	    HB_Error   *perror )
{
    HB_Pointer  block2 = NULL;
    HB_Error    error  = (HB_Error)0;

    block2 = realloc( block, new_size );
    if ( block2 == NULL && new_size != 0 )
        error = ERR(HB_Err_Out_Of_Memory);

    if ( !error )
        block = block2;

    *perror = error;
    return block;
}


HB_INTERNAL void
_hb_free( HB_Pointer  block )
{
  if ( block )
    free( block );
}


/* helper func to set a breakpoint on */
HB_INTERNAL HB_Error
_hb_err (HB_Error code)
{
  return code;
}
