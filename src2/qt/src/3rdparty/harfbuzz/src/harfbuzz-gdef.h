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

#ifndef HARFBUZZ_GDEF_H
#define HARFBUZZ_GDEF_H

#include "harfbuzz-open.h"
#include "harfbuzz-stream.h"

HB_BEGIN_HEADER

/* GDEF glyph properties.  Note that HB_GDEF_COMPONENT has no corresponding
 * flag in the LookupFlag field.     */
#define HB_GDEF_BASE_GLYPH  0x0002
#define HB_GDEF_LIGATURE    0x0004
#define HB_GDEF_MARK        0x0008
#define HB_GDEF_COMPONENT   0x0010


typedef struct HB_AttachPoint_  HB_AttachPoint;


struct  HB_AttachList_
{
  HB_AttachPoint*   AttachPoint;      /* array of AttachPoint tables */
  HB_Coverage       Coverage;         /* Coverage table              */
  HB_UShort         GlyphCount;       /* number of glyphs with
					 attachments                 */
  HB_Bool           loaded;
};

typedef struct HB_AttachList_  HB_AttachList;

typedef struct HB_LigGlyph_  HB_LigGlyph;

struct  HB_LigCaretList_
{
  HB_LigGlyph*   LigGlyph;            /* array of LigGlyph tables  */
  HB_Coverage    Coverage;            /* Coverage table            */
  HB_UShort      LigGlyphCount;       /* number of ligature glyphs */
  HB_Bool        loaded;
};

typedef struct HB_LigCaretList_  HB_LigCaretList;



/* The `NewGlyphClasses' field is not defined in the TTO specification.
   We use it for fonts with a constructed `GlyphClassDef' structure
   (i.e., which don't have a GDEF table) to collect glyph classes
   assigned during the lookup process.  The number of arrays in this
   pointer array is GlyphClassDef->cd.cd2.ClassRangeCount+1; the nth
   array then contains the glyph class values of the glyphs not covered
   by the ClassRangeRecords structures with index n-1 and n.  We store
   glyph class values for four glyphs in a single array element.

   `LastGlyph' is identical to the number of glyphs minus one in the
   font; we need it only if `NewGlyphClasses' is not NULL (to have an
   upper bound for the last array).

   Note that we first store the file offset to the `MarkAttachClassDef'
   field (which has been introduced in OpenType 1.2) -- since the
   `Version' field value hasn't been increased to indicate that we have
   one more field for some obscure reason, we must parse the GSUB table
   to find out whether class values refer to this table.  Only then we
   can finally load the MarkAttachClassDef structure if necessary.      */

struct  HB_GDEFHeader_
{
  HB_UShort**          NewGlyphClasses;
  HB_UInt             offset;
  HB_UInt             MarkAttachClassDef_offset;

  HB_16Dot16             Version;

  HB_ClassDefinition   GlyphClassDef;
  HB_AttachList        AttachList;
  HB_LigCaretList      LigCaretList;
  HB_ClassDefinition   MarkAttachClassDef;        /* new in OT 1.2 */

  HB_UShort            LastGlyph;
};

typedef struct HB_GDEFHeader_   HB_GDEFHeader;
typedef struct HB_GDEFHeader_*  HB_GDEF;


HB_Error  HB_New_GDEF_Table( HB_GDEFHeader** retptr );
      

HB_Error  HB_Load_GDEF_Table( HB_Stream       stream,
			      HB_GDEFHeader** gdef );


HB_Error  HB_Done_GDEF_Table ( HB_GDEFHeader* gdef );


HB_Error  HB_GDEF_Get_Glyph_Property( HB_GDEFHeader*  gdef,
				      HB_UShort        glyphID,
				      HB_UShort*       property );

HB_Error  HB_GDEF_Build_ClassDefinition( HB_GDEFHeader*  gdef,
					 HB_UShort        num_glyphs,
					 HB_UShort        glyph_count,
					 HB_UShort*       glyph_array,
					 HB_UShort*       class_array );

HB_END_HEADER

#endif /* HARFBUZZ_GDEF_H */
