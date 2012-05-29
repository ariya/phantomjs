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

#ifndef HARFBUZZ_GDEF_PRIVATE_H
#define HARFBUZZ_GDEF_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include "harfbuzz-buffer-private.h"
#include "harfbuzz-gdef.h"

HB_BEGIN_HEADER

/* Attachment related structures */

struct  HB_AttachPoint_
{
  HB_UShort*  PointIndex;             /* array of contour points      */
  HB_UShort   PointCount;             /* size of the PointIndex array */
};

/* Ligature Caret related structures */

struct  HB_CaretValueFormat1_
{
  HB_Short  Coordinate;               /* x or y value (in design units) */
};

typedef struct HB_CaretValueFormat1_  HB_CaretValueFormat1;


struct  HB_CaretValueFormat2_
{
  HB_UShort  CaretValuePoint;         /* contour point index on glyph */
};

typedef struct HB_CaretValueFormat2_  HB_CaretValueFormat2;


struct  HB_CaretValueFormat3_
{
  HB_Device*  Device;                 /* Device table for x or y value  */
  HB_Short    Coordinate;             /* x or y value (in design units) */
};

typedef struct HB_CaretValueFormat3_  HB_CaretValueFormat3;


#ifdef HB_SUPPORT_MULTIPLE_MASTER
struct  HB_CaretValueFormat4_
{
  HB_UShort  IdCaretValue;            /* metric ID */
};

typedef struct HB_CaretValueFormat4_  HB_CaretValueFormat4;
#endif


struct  HB_CaretValue_
{
  union
  {
    HB_CaretValueFormat1  cvf1;
    HB_CaretValueFormat2  cvf2;
    HB_CaretValueFormat3  cvf3;
#ifdef HB_SUPPORT_MULTIPLE_MASTER
    HB_CaretValueFormat4  cvf4;
#endif
  } cvf;

  HB_Byte  CaretValueFormat;          /* 1, 2, 3, or 4 */
};

typedef struct HB_CaretValue_  HB_CaretValue;


struct  HB_LigGlyph_
{
  HB_CaretValue*  CaretValue;        /* array of caret values  */
  HB_UShort        CaretCount;        /* number of caret values */
  HB_Bool          loaded;
};


HB_INTERNAL HB_Error
_HB_GDEF_Add_Glyph_Property( HB_GDEFHeader* gdef,
				       HB_UShort        glyphID,
				       HB_UShort        property );

HB_INTERNAL HB_Error
_HB_GDEF_Check_Property( HB_GDEFHeader* gdef,
				   HB_GlyphItem    item,
				   HB_UShort        flags,
				   HB_UShort*       property );

HB_INTERNAL HB_Error
_HB_GDEF_LoadMarkAttachClassDef_From_LookupFlags( HB_GDEFHeader* gdef,
						  HB_Stream      input,
						  HB_Lookup*     lo,
						  HB_UShort      num_lookups );

HB_END_HEADER

#endif /* HARFBUZZ_GDEF_PRIVATE_H */
