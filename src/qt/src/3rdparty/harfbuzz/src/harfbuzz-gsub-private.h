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

#ifndef HARFBUZZ_GSUB_PRIVATE_H
#define HARFBUZZ_GSUB_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include "harfbuzz-gsub.h"

HB_BEGIN_HEADER

typedef union HB_GSUB_SubTable_  HB_GSUB_SubTable;

/* LookupType 1 */

struct  HB_SingleSubstFormat1_
{
  HB_Short  DeltaGlyphID;             /* constant added to get
					 substitution glyph index */
};

typedef struct HB_SingleSubstFormat1_  HB_SingleSubstFormat1;


struct  HB_SingleSubstFormat2_
{
  HB_UShort*  Substitute;             /* array of substitute glyph IDs */
  HB_UShort   GlyphCount;             /* number of glyph IDs in
					 Substitute array              */
};

typedef struct HB_SingleSubstFormat2_  HB_SingleSubstFormat2;


struct  HB_SingleSubst_
{
  union
  {
    HB_SingleSubstFormat1  ssf1;
    HB_SingleSubstFormat2  ssf2;
  } ssf;

  HB_Coverage  Coverage;             /* Coverage table */
  HB_Byte     SubstFormat;            /* 1 or 2         */
};

typedef struct HB_SingleSubst_  HB_SingleSubst;


/* LookupType 2 */

struct  HB_Sequence_
{
  HB_UShort*  Substitute;             /* string of glyph IDs to
					 substitute                 */
  HB_UShort   GlyphCount;             /* number of glyph IDs in the
					 Substitute array           */
};

typedef struct HB_Sequence_  HB_Sequence;


struct  HB_MultipleSubst_
{
  HB_Sequence*  Sequence;            /* array of Sequence tables  */
  HB_Coverage   Coverage;            /* Coverage table            */
  HB_UShort      SubstFormat;         /* always 1                  */
  HB_UShort      SequenceCount;       /* number of Sequence tables */
};

typedef struct HB_MultipleSubst_  HB_MultipleSubst;


/* LookupType 3 */

struct  HB_AlternateSet_
{
  HB_UShort*  Alternate;              /* array of alternate glyph IDs */
  HB_UShort   GlyphCount;             /* number of glyph IDs in the
					 Alternate array              */
};

typedef struct HB_AlternateSet_  HB_AlternateSet;


struct  HB_AlternateSubst_
{
  HB_AlternateSet*  AlternateSet;    /* array of AlternateSet tables  */
  HB_Coverage       Coverage;        /* Coverage table                */
  HB_UShort          SubstFormat;     /* always 1                      */
  HB_UShort          AlternateSetCount;
				      /* number of AlternateSet tables */
};

typedef struct HB_AlternateSubst_  HB_AlternateSubst;


/* LookupType 4 */

struct  HB_Ligature_
{
  HB_UShort*  Component;              /* array of component glyph IDs     */
  HB_UShort   LigGlyph;               /* glyphID of ligature
					 to substitute                    */
  HB_UShort   ComponentCount;         /* number of components in ligature */
};

typedef struct HB_Ligature_  HB_Ligature;


struct  HB_LigatureSet_
{
  HB_Ligature*  Ligature;            /* array of Ligature tables  */
  HB_UShort      LigatureCount;       /* number of Ligature tables */
};

typedef struct HB_LigatureSet_  HB_LigatureSet;


struct  HB_LigatureSubst_
{
  HB_LigatureSet*  LigatureSet;      /* array of LigatureSet tables  */
  HB_Coverage      Coverage;         /* Coverage table               */
  HB_UShort         SubstFormat;      /* always 1                     */
  HB_UShort         LigatureSetCount; /* number of LigatureSet tables */
};

typedef struct HB_LigatureSubst_  HB_LigatureSubst;


/* needed by both lookup type 5 and 6 */

struct  HB_SubstLookupRecord_
{
  HB_UShort  SequenceIndex;           /* index into current
					 glyph sequence               */
  HB_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
};

typedef struct HB_SubstLookupRecord_  HB_SubstLookupRecord;


/* LookupType 5 */

struct  HB_SubRule_
{
  HB_UShort*              Input;      /* array of input glyph IDs     */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecord
					 tables                       */
  HB_UShort               GlyphCount; /* total number of input glyphs */
  HB_UShort               SubstCount; /* number of SubstLookupRecord
					 tables                       */
};

typedef struct HB_SubRule_  HB_SubRule;


struct  HB_SubRuleSet_
{
  HB_SubRule*  SubRule;              /* array of SubRule tables  */
  HB_UShort     SubRuleCount;         /* number of SubRule tables */
};

typedef struct HB_SubRuleSet_  HB_SubRuleSet;


struct  HB_ContextSubstFormat1_
{
  HB_SubRuleSet*  SubRuleSet;        /* array of SubRuleSet tables  */
  HB_Coverage     Coverage;          /* Coverage table              */
  HB_UShort        SubRuleSetCount;   /* number of SubRuleSet tables */
};

typedef struct HB_ContextSubstFormat1_  HB_ContextSubstFormat1;


struct  HB_SubClassRule_
{
  HB_UShort*              Class;      /* array of classes                */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecord
					 tables                          */
  HB_UShort               GlyphCount; /* total number of context classes */
  HB_UShort               SubstCount; /* number of SubstLookupRecord
					 tables                          */
};

typedef struct HB_SubClassRule_  HB_SubClassRule;


struct  HB_SubClassSet_
{
  HB_SubClassRule*  SubClassRule;    /* array of SubClassRule tables  */
  HB_UShort          SubClassRuleCount;
				      /* number of SubClassRule tables */
};

typedef struct HB_SubClassSet_  HB_SubClassSet;


/* The `MaxContextLength' field is not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the context rules.                    */

struct  HB_ContextSubstFormat2_
{
  HB_SubClassSet*     SubClassSet;   /* array of SubClassSet tables  */
  HB_Coverage         Coverage;      /* Coverage table               */
  HB_ClassDefinition  ClassDef;      /* ClassDef table               */
  HB_UShort            SubClassSetCount;
				      /* number of SubClassSet tables */
  HB_UShort            MaxContextLength;
				      /* maximal context length       */
};

typedef struct HB_ContextSubstFormat2_  HB_ContextSubstFormat2;


struct  HB_ContextSubstFormat3_
{
  HB_Coverage*           Coverage;   /* array of Coverage tables      */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups */
  HB_UShort               GlyphCount; /* number of input glyphs        */
  HB_UShort               SubstCount; /* number of SubstLookupRecords  */
};

typedef struct HB_ContextSubstFormat3_  HB_ContextSubstFormat3;


struct  HB_ContextSubst_
{
  union
  {
    HB_ContextSubstFormat1  csf1;
    HB_ContextSubstFormat2  csf2;
    HB_ContextSubstFormat3  csf3;
  } csf;

  HB_Byte  SubstFormat;               /* 1, 2, or 3 */
};

typedef struct HB_ContextSubst_  HB_ContextSubst;


/* LookupType 6 */

struct  HB_ChainSubRule_
{
  HB_UShort*              Backtrack;  /* array of backtrack glyph IDs     */
  HB_UShort*              Input;      /* array of input glyph IDs         */
  HB_UShort*              Lookahead;  /* array of lookahead glyph IDs     */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecords      */
  HB_UShort               BacktrackGlyphCount;
				      /* total number of backtrack glyphs */
  HB_UShort               InputGlyphCount;
				      /* total number of input glyphs     */
  HB_UShort               LookaheadGlyphCount;
				      /* total number of lookahead glyphs */
  HB_UShort               SubstCount; /* number of SubstLookupRecords     */
};

typedef struct HB_ChainSubRule_  HB_ChainSubRule;


struct  HB_ChainSubRuleSet_
{
  HB_ChainSubRule*  ChainSubRule;    /* array of ChainSubRule tables  */
  HB_UShort          ChainSubRuleCount;
				      /* number of ChainSubRule tables */
};

typedef struct HB_ChainSubRuleSet_  HB_ChainSubRuleSet;


struct  HB_ChainContextSubstFormat1_
{
  HB_ChainSubRuleSet*  ChainSubRuleSet;
				      /* array of ChainSubRuleSet tables  */
  HB_Coverage          Coverage;     /* Coverage table                   */
  HB_UShort             ChainSubRuleSetCount;
				      /* number of ChainSubRuleSet tables */
};

typedef struct HB_ChainContextSubstFormat1_  HB_ChainContextSubstFormat1;


struct  HB_ChainSubClassRule_
{
  HB_UShort*              Backtrack;  /* array of backtrack classes      */
  HB_UShort*              Input;      /* array of context classes        */
  HB_UShort*              Lookahead;  /* array of lookahead classes      */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups   */
  HB_UShort               BacktrackGlyphCount;
				      /* total number of backtrack
					 classes                         */
  HB_UShort               InputGlyphCount;
				      /* total number of context classes */
  HB_UShort               LookaheadGlyphCount;
				      /* total number of lookahead
					 classes                         */
  HB_UShort               SubstCount; /* number of SubstLookupRecords    */
};

typedef struct HB_ChainSubClassRule_  HB_ChainSubClassRule;


struct  HB_ChainSubClassSet_
{
  HB_ChainSubClassRule*  ChainSubClassRule;
				      /* array of ChainSubClassRule
					 tables                      */
  HB_UShort               ChainSubClassRuleCount;
				      /* number of ChainSubClassRule
					 tables                      */
};

typedef struct HB_ChainSubClassSet_  HB_ChainSubClassSet;


/* The `MaxXXXLength' fields are not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the specific context rules.         */

struct  HB_ChainContextSubstFormat2_
{
  HB_ChainSubClassSet*  ChainSubClassSet;
				      /* array of ChainSubClassSet
					 tables                     */
  HB_Coverage           Coverage;    /* Coverage table             */

  HB_ClassDefinition    BacktrackClassDef;
				      /* BacktrackClassDef table    */
  HB_ClassDefinition    InputClassDef;
				      /* InputClassDef table        */
  HB_ClassDefinition    LookaheadClassDef;
				      /* LookaheadClassDef table    */

  HB_UShort              ChainSubClassSetCount;
				      /* number of ChainSubClassSet
					 tables                     */
  HB_UShort              MaxBacktrackLength;
				      /* maximal backtrack length   */
  HB_UShort              MaxLookaheadLength;
				      /* maximal lookahead length   */
  HB_UShort              MaxInputLength;
				      /* maximal input length       */
};

typedef struct HB_ChainContextSubstFormat2_  HB_ChainContextSubstFormat2;


struct  HB_ChainContextSubstFormat3_
{
  HB_Coverage*           BacktrackCoverage;
				      /* array of backtrack Coverage
					 tables                        */
  HB_Coverage*           InputCoverage;
				      /* array of input coverage
					 tables                        */
  HB_Coverage*           LookaheadCoverage;
				      /* array of lookahead coverage
					 tables                        */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups */
  HB_UShort               BacktrackGlyphCount;
				      /* number of backtrack glyphs    */
  HB_UShort               InputGlyphCount;
				      /* number of input glyphs        */
  HB_UShort               LookaheadGlyphCount;
				      /* number of lookahead glyphs    */
  HB_UShort               SubstCount; /* number of SubstLookupRecords  */
};

typedef struct HB_ChainContextSubstFormat3_  HB_ChainContextSubstFormat3;


struct  HB_ChainContextSubst_
{
  union
  {
    HB_ChainContextSubstFormat1  ccsf1;
    HB_ChainContextSubstFormat2  ccsf2;
    HB_ChainContextSubstFormat3  ccsf3;
  } ccsf;

  HB_Byte  SubstFormat;               /* 1, 2, or 3 */
};

typedef struct HB_ChainContextSubst_  HB_ChainContextSubst;


#if 0
/* LookupType 7 */
struct HB_ExtensionSubst_
{
  HB_GSUB_SubTable *subtable;         /* referenced subtable */
  HB_UShort      SubstFormat;         /* always 1 */
  HB_UShort      LookuptType;         /* lookup-type of referenced subtable */
};

typedef struct HB_ExtensionSubst_  HB_ExtensionSubst;
#endif


/* LookupType 8 */
struct HB_ReverseChainContextSubst_
{
  HB_Coverage*  LookaheadCoverage;   /* array of lookahead Coverage
					 tables                          */
  HB_UShort*     Substitute;          /* array of substitute Glyph ID    */
  HB_Coverage*  BacktrackCoverage;   /* array of backtrack Coverage
					 tables                          */
  HB_Coverage   Coverage;	        /* coverage table for input glyphs */
  HB_UShort      SubstFormat;         /* always 1 */
  HB_UShort      BacktrackGlyphCount; /* number of backtrack glyphs      */
  HB_UShort      LookaheadGlyphCount; /* number of lookahead glyphs      */
  HB_UShort      GlyphCount;          /* number of Glyph IDs             */
};

typedef struct HB_ReverseChainContextSubst_  HB_ReverseChainContextSubst;


union  HB_GSUB_SubTable_
{
  HB_SingleSubst              single;
  HB_MultipleSubst            multiple;
  HB_AlternateSubst           alternate;
  HB_LigatureSubst            ligature;
  HB_ContextSubst             context;
  HB_ChainContextSubst        chain;
  HB_ReverseChainContextSubst reverse;
};




HB_INTERNAL HB_Error
_HB_GSUB_Load_SubTable( HB_GSUB_SubTable* st,
				  HB_Stream     stream,
				  HB_UShort     lookup_type );

HB_INTERNAL void
_HB_GSUB_Free_SubTable( HB_GSUB_SubTable* st,
			      HB_UShort     lookup_type );

HB_END_HEADER

#endif /* HARFBUZZ_GSUB_PRIVATE_H */
