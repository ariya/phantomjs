/*
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

#ifndef HARFBUZZ_GLOBAL_H
#define HARFBUZZ_GLOBAL_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define HB_BEGIN_HEADER  extern "C" {
#define HB_END_HEADER  }
#else
#define HB_BEGIN_HEADER  /* nothing */
#define HB_END_HEADER  /* nothing */
#endif

HB_BEGIN_HEADER

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define HB_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (HB_UInt)_x1 << 24 ) |     \
            ( (HB_UInt)_x2 << 16 ) |     \
            ( (HB_UInt)_x3 <<  8 ) |     \
              (HB_UInt)_x4         )

typedef char hb_int8;
typedef unsigned char hb_uint8;
typedef short hb_int16;
typedef unsigned short hb_uint16;
typedef int hb_int32;
typedef unsigned int hb_uint32;

typedef hb_uint8 HB_Bool;

typedef hb_uint8 HB_Byte;
typedef hb_uint16 HB_UShort;
typedef hb_uint32 HB_UInt;
typedef hb_int8 HB_Char;
typedef hb_int16 HB_Short;
typedef hb_int32 HB_Int;

typedef hb_uint16 HB_UChar16;
typedef hb_uint32 HB_UChar32;
typedef hb_uint32 HB_Glyph;
typedef hb_int32 HB_Fixed; /* 26.6 */

#define HB_FIXED_CONSTANT(v) ((v) * 64)
#define HB_FIXED_ROUND(v) (((v)+32) & -64)

typedef hb_int32 HB_16Dot16; /* 16.16 */

typedef void * HB_Pointer;
typedef hb_uint32 HB_Tag;

typedef enum {
  /* no error */
  HB_Err_Ok                           = 0x0000,
  HB_Err_Not_Covered                  = 0xFFFF,

  /* _hb_err() is called whenever returning the following errors,
   * and in a couple places for HB_Err_Not_Covered too. */

  /* programmer error */
  HB_Err_Invalid_Argument             = 0x1A66,

  /* font error */
  HB_Err_Invalid_SubTable_Format      = 0x157F,
  HB_Err_Invalid_SubTable             = 0x1570,
  HB_Err_Read_Error                   = 0x6EAD,

  /* system error */
  HB_Err_Out_Of_Memory                = 0xDEAD
} HB_Error;

typedef struct {
    HB_Fixed x;
    HB_Fixed y;
} HB_FixedPoint;

typedef struct HB_Font_ *HB_Font;
typedef struct HB_StreamRec_ *HB_Stream;
typedef struct HB_FaceRec_ *HB_Face;

HB_END_HEADER

#endif
