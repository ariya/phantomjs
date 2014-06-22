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

#ifndef HARFBUZZ_GSUB_H
#define HARFBUZZ_GSUB_H

#include "harfbuzz-gdef.h"
#include "harfbuzz-buffer.h"

HB_BEGIN_HEADER

/* Lookup types for glyph substitution */

#define HB_GSUB_LOOKUP_SINGLE        1
#define HB_GSUB_LOOKUP_MULTIPLE      2
#define HB_GSUB_LOOKUP_ALTERNATE     3
#define HB_GSUB_LOOKUP_LIGATURE      4
#define HB_GSUB_LOOKUP_CONTEXT       5
#define HB_GSUB_LOOKUP_CHAIN         6
#define HB_GSUB_LOOKUP_EXTENSION     7
#define HB_GSUB_LOOKUP_REVERSE_CHAIN 8


/* A pointer to a function which selects the alternate glyph.  `pos' is
   the position of the glyph with index `glyphID', `num_alternates'
   gives the number of alternates in the `alternates' array.  `data'
   points to the user-defined structure specified during a call to
   HB_GSUB_Register_Alternate_Function().  The function must return an
   index into the `alternates' array.                                   */

typedef HB_UShort  (*HB_AltFunction)(HB_UInt    pos,
				      HB_UShort   glyphID,
				      HB_UShort   num_alternates,
				      HB_UShort*  alternates,
				      void*       data );


struct  HB_GSUBHeader_
{
  HB_GDEFHeader*  gdef;

  /* the next two fields are used for an alternate substitution callback
     function to select the proper alternate glyph.                      */

  void*            data;
  HB_AltFunction  altfunc;

  HB_UInt         offset;

  HB_16Dot16         Version;

  HB_ScriptList   ScriptList;
  HB_FeatureList  FeatureList;
  HB_LookupList   LookupList;
};

typedef struct HB_GSUBHeader_   HB_GSUBHeader;
typedef HB_GSUBHeader*  HB_GSUB;


HB_Error  HB_Load_GSUB_Table( HB_Stream       stream,
			      HB_GSUBHeader** gsub,
			      HB_GDEFHeader*  gdef,
                              HB_Stream       gdefStream );


HB_Error  HB_Done_GSUB_Table( HB_GSUBHeader*  gsub );


HB_Error  HB_GSUB_Select_Script( HB_GSUBHeader*  gsub,
				 HB_UInt         script_tag,
				 HB_UShort*       script_index );

HB_Error  HB_GSUB_Select_Language( HB_GSUBHeader*  gsub,
				   HB_UInt         language_tag,
				   HB_UShort        script_index,
				   HB_UShort*       language_index,
				   HB_UShort*       req_feature_index );

HB_Error  HB_GSUB_Select_Feature( HB_GSUBHeader*  gsub,
				  HB_UInt         feature_tag,
				  HB_UShort        script_index,
				  HB_UShort        language_index,
				  HB_UShort*       feature_index );


HB_Error  HB_GSUB_Query_Scripts( HB_GSUBHeader*  gsub,
				 HB_UInt**       script_tag_list );

HB_Error  HB_GSUB_Query_Languages( HB_GSUBHeader*  gsub,
				   HB_UShort        script_index,
				   HB_UInt**       language_tag_list );

HB_Error  HB_GSUB_Query_Features( HB_GSUBHeader*  gsub,
				  HB_UShort        script_index,
				  HB_UShort        language_index,
				  HB_UInt**       feature_tag_list );


HB_Error  HB_GSUB_Add_Feature( HB_GSUBHeader*  gsub,
			       HB_UShort        feature_index,
			       HB_UInt          property );

HB_Error  HB_GSUB_Clear_Features( HB_GSUBHeader*  gsub );


HB_Error  HB_GSUB_Register_Alternate_Function( HB_GSUBHeader*  gsub,
					       HB_AltFunction  altfunc,
					       void*            data );


HB_Error  HB_GSUB_Apply_String( HB_GSUBHeader*   gsub,
				HB_Buffer        buffer );

HB_END_HEADER

#endif /* HARFBUZZ_GSUB_H */
