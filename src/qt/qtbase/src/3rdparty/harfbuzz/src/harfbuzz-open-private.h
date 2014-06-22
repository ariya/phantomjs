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

#ifndef HARFBUZZ_OPEN_PRIVATE_H
#define HARFBUZZ_OPEN_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-open.h"
#include "harfbuzz-gsub-private.h"
#include "harfbuzz-gpos-private.h"

HB_BEGIN_HEADER


struct  HB_SubTable_
{
  union
  {
    HB_GSUB_SubTable  gsub;
    HB_GPOS_SubTable  gpos;
  } st;
};


HB_INTERNAL HB_Error
_HB_OPEN_Load_ScriptList( HB_ScriptList* sl,
			   HB_Stream     input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_FeatureList( HB_FeatureList* fl,
			    HB_Stream         input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_LookupList( HB_LookupList*  ll,
			   HB_Stream        input,
			   HB_Type         type );

HB_INTERNAL HB_Error
_HB_OPEN_Load_Coverage( HB_Coverage* c,
			 HB_Stream      input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_ClassDefinition( HB_ClassDefinition* cd,
				HB_UShort             limit,
				HB_Stream             input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_EmptyOrClassDefinition( HB_ClassDefinition* cd,
					       HB_UShort             limit,
					       HB_UInt              class_offset,
					       HB_UInt              base_offset,
					       HB_Stream             input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_Device( HB_Device** d,
		       HB_Stream    input );

HB_INTERNAL void  _HB_OPEN_Free_ScriptList( HB_ScriptList*  sl );
HB_INTERNAL void  _HB_OPEN_Free_FeatureList( HB_FeatureList*  fl );
HB_INTERNAL void  _HB_OPEN_Free_LookupList( HB_LookupList*  ll,
		       HB_Type         type );

HB_INTERNAL void  _HB_OPEN_Free_Coverage( HB_Coverage*  c );
HB_INTERNAL void  _HB_OPEN_Free_ClassDefinition( HB_ClassDefinition*  cd );
HB_INTERNAL void  _HB_OPEN_Free_Device( HB_Device*  d );



HB_INTERNAL HB_Error
_HB_OPEN_Coverage_Index( HB_Coverage* c,
			  HB_UShort      glyphID,
			  HB_UShort*     index );
HB_INTERNAL HB_Error
_HB_OPEN_Get_Class( HB_ClassDefinition* cd,
		     HB_UShort             glyphID,
		    HB_UShort*          klass,
		     HB_UShort*            index );
HB_INTERNAL HB_Error
_HB_OPEN_Get_Device( HB_Device* d,
		      HB_UShort    size,
		      HB_Short*    value );

HB_END_HEADER

#endif /* HARFBUZZ_OPEN_PRIVATE_H */
