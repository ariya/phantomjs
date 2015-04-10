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

#ifndef HARFBUZZ_GPOS_PRIVATE_H
#define HARFBUZZ_GPOS_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include "harfbuzz-gpos.h"

HB_BEGIN_HEADER

/* shared tables */

#define VR_X_PLACEMENT_DEVICE 0
#define VR_Y_PLACEMENT_DEVICE 1
#define VR_X_ADVANCE_DEVICE   2
#define VR_Y_ADVANCE_DEVICE   3

#ifndef HB_SUPPORT_MULTIPLE_MASTER
#  define HB_USE_FLEXIBLE_VALUE_RECORD
#endif

struct  HB_ValueRecord_
{
  HB_Short    XPlacement;             /* horizontal adjustment for
					 placement                      */
  HB_Short    YPlacement;             /* vertical adjustment for
					 placement                      */
  HB_Short    XAdvance;               /* horizontal adjustment for
					 advance                        */
  HB_Short    YAdvance;               /* vertical adjustment for
					 advance                        */

  HB_Device** DeviceTables;           /* device tables for placement
					 and advance                    */

#ifdef HB_SUPPORT_MULTIPLE_MASTER
  HB_UShort   XIdPlacement;           /* horizontal placement metric ID */
  HB_UShort   YIdPlacement;           /* vertical placement metric ID   */
  HB_UShort   XIdAdvance;             /* horizontal advance metric ID   */
  HB_UShort   YIdAdvance;             /* vertical advance metric ID     */
#endif
};

typedef struct HB_ValueRecord_  HB_ValueRecord;


/* Mask values to scan the value format of the ValueRecord structure.
 We always expand compressed ValueRecords of the font.              */

#define HB_GPOS_FORMAT_HAVE_DEVICE_TABLES       0x00F0

#define HB_GPOS_FORMAT_HAVE_X_PLACEMENT         0x0001
#define HB_GPOS_FORMAT_HAVE_Y_PLACEMENT         0x0002
#define HB_GPOS_FORMAT_HAVE_X_ADVANCE           0x0004
#define HB_GPOS_FORMAT_HAVE_Y_ADVANCE           0x0008
#define HB_GPOS_FORMAT_HAVE_X_PLACEMENT_DEVICE  0x0010
#define HB_GPOS_FORMAT_HAVE_Y_PLACEMENT_DEVICE  0x0020
#define HB_GPOS_FORMAT_HAVE_X_ADVANCE_DEVICE    0x0040
#define HB_GPOS_FORMAT_HAVE_Y_ADVANCE_DEVICE    0x0080
#define HB_GPOS_FORMAT_HAVE_X_ID_PLACEMENT      0x0100
#define HB_GPOS_FORMAT_HAVE_Y_ID_PLACEMENT      0x0200
#define HB_GPOS_FORMAT_HAVE_X_ID_ADVANCE        0x0400
#define HB_GPOS_FORMAT_HAVE_Y_ID_ADVANCE        0x0800


struct  HB_AnchorFormat1_
{
  HB_Short   XCoordinate;             /* horizontal value */
  HB_Short   YCoordinate;             /* vertical value   */
};

typedef struct HB_AnchorFormat1_  HB_AnchorFormat1;


struct  HB_AnchorFormat2_
{
  HB_Short   XCoordinate;             /* horizontal value             */
  HB_Short   YCoordinate;             /* vertical value               */
  HB_UShort  AnchorPoint;             /* index to glyph contour point */
};

typedef struct HB_AnchorFormat2_  HB_AnchorFormat2;

#define AF3_X_DEVICE_TABLE 0
#define AF3_Y_DEVICE_TABLE 1

struct  HB_AnchorFormat3_
{
  HB_Short    XCoordinate;            /* horizontal value              */
  HB_Short    YCoordinate;            /* vertical value                */
  HB_Device** DeviceTables;           /* device tables for coordinates */
};

typedef struct HB_AnchorFormat3_  HB_AnchorFormat3;


#ifdef HB_SUPPORT_MULTIPLE_MASTER
struct  HB_AnchorFormat4_
{
  HB_UShort  XIdAnchor;               /* horizontal metric ID */
  HB_UShort  YIdAnchor;               /* vertical metric ID   */
};

typedef struct HB_AnchorFormat4_  HB_AnchorFormat4;
#endif


struct  HB_Anchor_
{
  HB_Byte  PosFormat;                 /* 1, 2, 3, or 4 -- 0 indicates
					 that there is no Anchor table */

  union
  {
    HB_AnchorFormat1  af1;
    HB_AnchorFormat2  af2;
    HB_AnchorFormat3  af3;
#ifdef HB_SUPPORT_MULTIPLE_MASTER
    HB_AnchorFormat4  af4;
#endif
  } af;
};

typedef struct HB_Anchor_  HB_Anchor;


struct  HB_MarkRecord_
{
  HB_UShort   Class;                  /* mark class   */
  HB_Anchor  MarkAnchor;             /* anchor table */
};

typedef struct HB_MarkRecord_  HB_MarkRecord;


struct  HB_MarkArray_
{
  HB_UShort        MarkCount;         /* number of MarkRecord tables */
  HB_MarkRecord*  MarkRecord;        /* array of MarkRecord tables  */
};

typedef struct HB_MarkArray_  HB_MarkArray;


/* LookupType 1 */

struct  HB_SinglePosFormat1_
{
  HB_ValueRecord  Value;             /* ValueRecord for all covered
					 glyphs                      */
};

typedef struct HB_SinglePosFormat1_  HB_SinglePosFormat1;


struct  HB_SinglePosFormat2_
{
  HB_UShort         ValueCount;       /* number of ValueRecord tables */
  HB_ValueRecord*  Value;            /* array of ValueRecord tables  */
};

typedef struct HB_SinglePosFormat2_  HB_SinglePosFormat2;


struct  HB_SinglePos_
{
  HB_Byte       PosFormat;            /* 1 or 2         */
  HB_Coverage  Coverage;             /* Coverage table */

  HB_UShort     ValueFormat;          /* format of ValueRecord table */

  union
  {
    HB_SinglePosFormat1  spf1;
    HB_SinglePosFormat2  spf2;
  } spf;
};

typedef struct HB_SinglePos_  HB_SinglePos;


/* LookupType 2 */

struct  HB_PairValueRecord_
{
  HB_UShort        SecondGlyph;       /* glyph ID for second glyph  */
  HB_ValueRecord  Value1;            /* pos. data for first glyph  */
  HB_ValueRecord  Value2;            /* pos. data for second glyph */
};

typedef struct HB_PairValueRecord_  HB_PairValueRecord;


struct  HB_PairSet_
{
  HB_UShort             PairValueCount;
				      /* number of PairValueRecord tables */
#ifndef HB_USE_FLEXIBLE_VALUE_RECORD
  HB_PairValueRecord*  PairValueRecord;
				      /* array of PairValueRecord tables  */
#else
  HB_Short* ValueRecords;
#endif
};

typedef struct HB_PairSet_  HB_PairSet;


struct  HB_PairPosFormat1_
{
  HB_UShort     PairSetCount;         /* number of PairSet tables    */
  HB_PairSet*  PairSet;              /* array of PairSet tables     */
};

typedef struct HB_PairPosFormat1_  HB_PairPosFormat1;


struct  HB_Class2Record_
{
  HB_ValueRecord  Value1;            /* pos. data for first glyph  */
  HB_ValueRecord  Value2;            /* pos. data for second glyph */
};

typedef struct HB_Class2Record_  HB_Class2Record;


struct  HB_Class1Record_
{
  hb_uint8 IsFlexible;
  union {
    HB_Class2Record*  Class2Record;    /* array of Class2Record tables */
    HB_Short* ValueRecords;
  } c2r;
};

typedef struct HB_Class1Record_  HB_Class1Record;


struct  HB_PairPosFormat2_
{
  HB_ClassDefinition  ClassDef1;     /* class def. for first glyph     */
  HB_ClassDefinition  ClassDef2;     /* class def. for second glyph    */
  HB_UShort            Class1Count;   /* number of classes in ClassDef1
					 table                          */
  HB_UShort            Class2Count;   /* number of classes in ClassDef2
					 table                          */
  HB_Class1Record*    Class1Record;  /* array of Class1Record tables   */
};

typedef struct HB_PairPosFormat2_  HB_PairPosFormat2;


struct  HB_PairPos_
{
  HB_Byte       PosFormat;            /* 1 or 2         */
  HB_Coverage  Coverage;             /* Coverage table */
  HB_UShort     ValueFormat1;         /* format of ValueRecord table
					 for first glyph             */
  HB_UShort     ValueFormat2;         /* format of ValueRecord table
					 for second glyph            */

  union
  {
    HB_PairPosFormat1  ppf1;
    HB_PairPosFormat2  ppf2;
  } ppf;
};

typedef struct HB_PairPos_  HB_PairPos;


/* LookupType 3 */

struct  HB_EntryExitRecord_
{
  HB_Anchor  EntryAnchor;            /* entry Anchor table */
  HB_Anchor  ExitAnchor;             /* exit Anchor table  */
};


typedef struct HB_EntryExitRecord_  HB_EntryExitRecord;

struct  HB_CursivePos_
{
  HB_UShort             PosFormat;    /* always 1                         */
  HB_Coverage          Coverage;     /* Coverage table                   */
  HB_UShort             EntryExitCount;
				      /* number of EntryExitRecord tables */
  HB_EntryExitRecord*  EntryExitRecord;
				      /* array of EntryExitRecord tables  */
};

typedef struct HB_CursivePos_  HB_CursivePos;


/* LookupType 4 */

struct  HB_BaseRecord_
{
  HB_Anchor*  BaseAnchor;            /* array of base glyph anchor
					 tables                     */
};

typedef struct HB_BaseRecord_  HB_BaseRecord;


struct  HB_BaseArray_
{
  HB_UShort        BaseCount;         /* number of BaseRecord tables */
  HB_BaseRecord*  BaseRecord;        /* array of BaseRecord tables  */
};

typedef struct HB_BaseArray_  HB_BaseArray;


struct  HB_MarkBasePos_
{
  HB_UShort      PosFormat;           /* always 1                  */
  HB_Coverage   MarkCoverage;        /* mark glyph coverage table */
  HB_Coverage   BaseCoverage;        /* base glyph coverage table */
  HB_UShort      ClassCount;          /* number of mark classes    */
  HB_MarkArray  MarkArray;           /* mark array table          */
  HB_BaseArray  BaseArray;           /* base array table          */
};

typedef struct HB_MarkBasePos_  HB_MarkBasePos;


/* LookupType 5 */

struct  HB_ComponentRecord_
{
  HB_Anchor*  LigatureAnchor;        /* array of ligature glyph anchor
					 tables                         */
};

typedef struct HB_ComponentRecord_  HB_ComponentRecord;


struct  HB_LigatureAttach_
{
  HB_UShort             ComponentCount;
				      /* number of ComponentRecord tables */
  HB_ComponentRecord*  ComponentRecord;
				      /* array of ComponentRecord tables  */
};

typedef struct HB_LigatureAttach_  HB_LigatureAttach;


struct  HB_LigatureArray_
{
  HB_UShort            LigatureCount; /* number of LigatureAttach tables */
  HB_LigatureAttach*  LigatureAttach;
				      /* array of LigatureAttach tables  */
};

typedef struct HB_LigatureArray_  HB_LigatureArray;


struct  HB_MarkLigPos_
{
  HB_UShort          PosFormat;       /* always 1                      */
  HB_Coverage       MarkCoverage;    /* mark glyph coverage table     */
  HB_Coverage       LigatureCoverage;
				      /* ligature glyph coverage table */
  HB_UShort          ClassCount;      /* number of mark classes        */
  HB_MarkArray      MarkArray;       /* mark array table              */
  HB_LigatureArray  LigatureArray;   /* ligature array table          */
};

typedef struct HB_MarkLigPos_  HB_MarkLigPos;


/* LookupType 6 */

struct  HB_Mark2Record_
{
  HB_Anchor*  Mark2Anchor;           /* array of mark glyph anchor
					 tables                     */
};

typedef struct HB_Mark2Record_  HB_Mark2Record;


struct  HB_Mark2Array_
{
  HB_UShort         Mark2Count;       /* number of Mark2Record tables */
  HB_Mark2Record*  Mark2Record;      /* array of Mark2Record tables  */
};

typedef struct HB_Mark2Array_  HB_Mark2Array;


struct  HB_MarkMarkPos_
{
  HB_UShort       PosFormat;          /* always 1                         */
  HB_Coverage    Mark1Coverage;      /* first mark glyph coverage table  */
  HB_Coverage    Mark2Coverage;      /* second mark glyph coverave table */
  HB_UShort       ClassCount;         /* number of combining mark classes */
  HB_MarkArray   Mark1Array;         /* MarkArray table for first mark   */
  HB_Mark2Array  Mark2Array;         /* MarkArray table for second mark  */
};

typedef struct HB_MarkMarkPos_  HB_MarkMarkPos;


/* needed by both lookup type 7 and 8 */

struct  HB_PosLookupRecord_
{
  HB_UShort  SequenceIndex;           /* index into current
					 glyph sequence               */
  HB_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
};

typedef struct HB_PosLookupRecord_  HB_PosLookupRecord;


/* LookupType 7 */

struct  HB_PosRule_
{
  HB_UShort             GlyphCount;   /* total number of input glyphs     */
  HB_UShort             PosCount;     /* number of PosLookupRecord tables */
  HB_UShort*            Input;        /* array of input glyph IDs         */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_PosRule_  HB_PosRule;


struct  HB_PosRuleSet_
{
  HB_UShort     PosRuleCount;         /* number of PosRule tables */
  HB_PosRule*  PosRule;              /* array of PosRule tables  */
};

typedef struct HB_PosRuleSet_  HB_PosRuleSet;


struct  HB_ContextPosFormat1_
{
  HB_Coverage     Coverage;          /* Coverage table              */
  HB_UShort        PosRuleSetCount;   /* number of PosRuleSet tables */
  HB_PosRuleSet*  PosRuleSet;        /* array of PosRuleSet tables  */
};

typedef struct HB_ContextPosFormat1_  HB_ContextPosFormat1;


struct  HB_PosClassRule_
{
  HB_UShort             GlyphCount;   /* total number of context classes  */
  HB_UShort             PosCount;     /* number of PosLookupRecord tables */
  HB_UShort*            Class;        /* array of classes                 */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_PosClassRule_  HB_PosClassRule;


struct  HB_PosClassSet_
{
  HB_UShort          PosClassRuleCount;
				      /* number of PosClassRule tables */
  HB_PosClassRule*  PosClassRule;    /* array of PosClassRule tables  */
};

typedef struct HB_PosClassSet_  HB_PosClassSet;


/* The `MaxContextLength' field is not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the context rules.                    */

struct  HB_ContextPosFormat2_
{
  HB_UShort            MaxContextLength;
				      /* maximal context length       */
  HB_Coverage         Coverage;      /* Coverage table               */
  HB_ClassDefinition  ClassDef;      /* ClassDef table               */
  HB_UShort            PosClassSetCount;
				      /* number of PosClassSet tables */
  HB_PosClassSet*     PosClassSet;   /* array of PosClassSet tables  */
};

typedef struct HB_ContextPosFormat2_  HB_ContextPosFormat2;


struct  HB_ContextPosFormat3_
{
  HB_UShort             GlyphCount;   /* number of input glyphs           */
  HB_UShort             PosCount;     /* number of PosLookupRecord tables */
  HB_Coverage*         Coverage;     /* array of Coverage tables         */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_ContextPosFormat3_  HB_ContextPosFormat3;


struct  HB_ContextPos_
{
  HB_Byte  PosFormat;                 /* 1, 2, or 3     */

  union
  {
    HB_ContextPosFormat1  cpf1;
    HB_ContextPosFormat2  cpf2;
    HB_ContextPosFormat3  cpf3;
  } cpf;
};

typedef struct HB_ContextPos_  HB_ContextPos;


/* LookupType 8 */

struct  HB_ChainPosRule_
{
  HB_UShort*            Backtrack;    /* array of backtrack glyph IDs     */
  HB_UShort*            Input;        /* array of input glyph IDs         */
  HB_UShort*            Lookahead;    /* array of lookahead glyph IDs     */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecords       */
  HB_UShort             BacktrackGlyphCount;
				      /* total number of backtrack glyphs */
  HB_UShort             InputGlyphCount;
				      /* total number of input glyphs     */
  HB_UShort             LookaheadGlyphCount;
				      /* total number of lookahead glyphs */
  HB_UShort             PosCount;     /* number of PosLookupRecords       */
};

typedef struct HB_ChainPosRule_  HB_ChainPosRule;


struct  HB_ChainPosRuleSet_
{
  HB_UShort          ChainPosRuleCount;
				      /* number of ChainPosRule tables */
  HB_ChainPosRule*  ChainPosRule;    /* array of ChainPosRule tables  */
};

typedef struct HB_ChainPosRuleSet_  HB_ChainPosRuleSet;


struct  HB_ChainContextPosFormat1_
{
  HB_Coverage          Coverage;     /* Coverage table                   */
  HB_UShort             ChainPosRuleSetCount;
				      /* number of ChainPosRuleSet tables */
  HB_ChainPosRuleSet*  ChainPosRuleSet;
				      /* array of ChainPosRuleSet tables  */
};

typedef struct HB_ChainContextPosFormat1_  HB_ChainContextPosFormat1;


struct  HB_ChainPosClassRule_
{
  HB_UShort*            Backtrack;    /* array of backtrack classes      */
  HB_UShort*            Input;        /* array of context classes        */
  HB_UShort*            Lookahead;    /* array of lookahead classes      */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of substitution lookups   */
  HB_UShort             BacktrackGlyphCount;
				      /* total number of backtrack
					 classes                         */
  HB_UShort             InputGlyphCount;
				      /* total number of context classes */
  HB_UShort             LookaheadGlyphCount;
				      /* total number of lookahead
					 classes                         */
  HB_UShort             PosCount;     /* number of PosLookupRecords      */
};

typedef struct HB_ChainPosClassRule_  HB_ChainPosClassRule;


struct  HB_ChainPosClassSet_
{
  HB_UShort               ChainPosClassRuleCount;
				      /* number of ChainPosClassRule
					 tables                      */
  HB_ChainPosClassRule*  ChainPosClassRule;
				      /* array of ChainPosClassRule
					 tables                      */
};

typedef struct HB_ChainPosClassSet_  HB_ChainPosClassSet;


/* The `MaxXXXLength' fields are not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the specific context rules.         */

struct  HB_ChainContextPosFormat2_
{
  HB_Coverage           Coverage;    /* Coverage table             */

  HB_UShort              MaxBacktrackLength;
				      /* maximal backtrack length   */
  HB_ClassDefinition    BacktrackClassDef;
				      /* BacktrackClassDef table    */
  HB_UShort              MaxInputLength;
				      /* maximal input length       */
  HB_ClassDefinition    InputClassDef;
				      /* InputClassDef table        */
  HB_UShort              MaxLookaheadLength;
				      /* maximal lookahead length   */
  HB_ClassDefinition    LookaheadClassDef;
				      /* LookaheadClassDef table    */

  HB_UShort              ChainPosClassSetCount;
				      /* number of ChainPosClassSet
					 tables                     */
  HB_ChainPosClassSet*  ChainPosClassSet;
				      /* array of ChainPosClassSet
					 tables                     */
};

typedef struct HB_ChainContextPosFormat2_  HB_ChainContextPosFormat2;


struct  HB_ChainContextPosFormat3_
{
  HB_UShort             BacktrackGlyphCount;
				      /* number of backtrack glyphs    */
  HB_Coverage*         BacktrackCoverage;
				      /* array of backtrack Coverage
					 tables                        */
  HB_UShort             InputGlyphCount;
				      /* number of input glyphs        */
  HB_Coverage*         InputCoverage;
				      /* array of input coverage
					 tables                        */
  HB_UShort             LookaheadGlyphCount;
				      /* number of lookahead glyphs    */
  HB_Coverage*         LookaheadCoverage;
				      /* array of lookahead coverage
					 tables                        */
  HB_UShort             PosCount;     /* number of PosLookupRecords    */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of substitution lookups */
};

typedef struct HB_ChainContextPosFormat3_  HB_ChainContextPosFormat3;


struct  HB_ChainContextPos_
{
  HB_Byte  PosFormat;               /* 1, 2, or 3 */

  union
  {
    HB_ChainContextPosFormat1  ccpf1;
    HB_ChainContextPosFormat2  ccpf2;
    HB_ChainContextPosFormat3  ccpf3;
  } ccpf;
};

typedef struct HB_ChainContextPos_  HB_ChainContextPos;


#if 0
/* LookupType 10 */
struct HB_ExtensionPos_
{
  HB_UShort      PosFormat;           /* always 1 */
  HB_UShort      LookuptType;         /* lookup-type of referenced subtable */
  HB_GPOS_SubTable *subtable;         /* referenced subtable */
};

typedef struct HB_ExtensionPos_  HB_ExtensionPos;
#endif


union  HB_GPOS_SubTable_
{
  HB_SinglePos        single;
  HB_PairPos          pair;
  HB_CursivePos       cursive;
  HB_MarkBasePos      markbase;
  HB_MarkLigPos       marklig;
  HB_MarkMarkPos      markmark;
  HB_ContextPos       context;
  HB_ChainContextPos  chain;
};

typedef union HB_GPOS_SubTable_  HB_GPOS_SubTable;



HB_INTERNAL HB_Error
_HB_GPOS_Load_SubTable( HB_GPOS_SubTable* st,
				  HB_Stream     stream,
				  HB_UShort     lookup_type );

HB_INTERNAL void
_HB_GPOS_Free_SubTable( HB_GPOS_SubTable* st,
			      HB_UShort     lookup_type );

HB_END_HEADER

#endif /* HARFBUZZ_GPOS_PRIVATE_H */
