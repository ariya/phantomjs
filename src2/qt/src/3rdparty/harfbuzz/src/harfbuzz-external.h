/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef HARFBUZZ_EXTERNAL_H
#define HARFBUZZ_EXTERNAL_H

#include "harfbuzz-global.h"

HB_BEGIN_HEADER

/* This header contains some methods that are not part of
   Harfbuzz itself, but referenced by it.
   They need to be provided by the application/library
*/


/*
 see http://www.unicode.org/reports/tr14/tr14-19.html
 we don't use the XX, AI and CB properties and map them to AL instead.
 as we don't support any EBDIC based OS'es, NL is ignored and mapped to AL as well.
*/
typedef enum {
    HB_LineBreak_OP, HB_LineBreak_CL, HB_LineBreak_QU, HB_LineBreak_GL, HB_LineBreak_NS,
    HB_LineBreak_EX, HB_LineBreak_SY, HB_LineBreak_IS, HB_LineBreak_PR, HB_LineBreak_PO,
    HB_LineBreak_NU, HB_LineBreak_AL, HB_LineBreak_ID, HB_LineBreak_IN, HB_LineBreak_HY,
    HB_LineBreak_BA, HB_LineBreak_BB, HB_LineBreak_B2, HB_LineBreak_ZW, HB_LineBreak_CM,
    HB_LineBreak_WJ, HB_LineBreak_H2, HB_LineBreak_H3, HB_LineBreak_JL, HB_LineBreak_JV,
    HB_LineBreak_JT, HB_LineBreak_SA, HB_LineBreak_SG,
    HB_LineBreak_SP, HB_LineBreak_CR, HB_LineBreak_LF, HB_LineBreak_BK
} HB_LineBreakClass;

typedef enum 
{
    HB_NoCategory,

    HB_Mark_NonSpacing,          /*   Mn */
    HB_Mark_SpacingCombining,    /*   Mc */
    HB_Mark_Enclosing,           /*   Me */

    HB_Number_DecimalDigit,      /*   Nd */
    HB_Number_Letter,            /*   Nl */
    HB_Number_Other,             /*   No */

    HB_Separator_Space,          /*   Zs */
    HB_Separator_Line,           /*   Zl */
    HB_Separator_Paragraph,      /*   Zp */

    HB_Other_Control,            /*   Cc */
    HB_Other_Format,             /*   Cf */
    HB_Other_Surrogate,          /*   Cs */
    HB_Other_PrivateUse,         /*   Co */
    HB_Other_NotAssigned,        /*   Cn */

    HB_Letter_Uppercase,         /*   Lu */
    HB_Letter_Lowercase,         /*   Ll */
    HB_Letter_Titlecase,         /*   Lt */
    HB_Letter_Modifier,          /*   Lm */
    HB_Letter_Other,             /*   Lo */

    HB_Punctuation_Connector,    /*   Pc */
    HB_Punctuation_Dash,         /*   Pd */
    HB_Punctuation_Open,         /*   Ps */
    HB_Punctuation_Close,        /*   Pe */
    HB_Punctuation_InitialQuote, /*   Pi */
    HB_Punctuation_FinalQuote,   /*   Pf */
    HB_Punctuation_Other,        /*   Po */

    HB_Symbol_Math,              /*   Sm */
    HB_Symbol_Currency,          /*   Sc */
    HB_Symbol_Modifier,          /*   Sk */
    HB_Symbol_Other              /*   So */
} HB_CharCategory;

typedef enum
{
    HB_Grapheme_Other, 
    HB_Grapheme_CR,
    HB_Grapheme_LF,
    HB_Grapheme_Control,
    HB_Grapheme_Extend,
    HB_Grapheme_L, 
    HB_Grapheme_V, 
    HB_Grapheme_T, 
    HB_Grapheme_LV, 
    HB_Grapheme_LVT
} HB_GraphemeClass;


typedef enum
{
    HB_Word_Other,
    HB_Word_Format,
    HB_Word_Katakana,
    HB_Word_ALetter,
    HB_Word_MidLetter,
    HB_Word_MidNum,
    HB_Word_Numeric,
    HB_Word_ExtendNumLet
} HB_WordClass;


typedef enum
{
    HB_Sentence_Other,
    HB_Sentence_Sep,
    HB_Sentence_Format,
    HB_Sentence_Sp,
    HB_Sentence_Lower,
    HB_Sentence_Upper,
    HB_Sentence_OLetter,
    HB_Sentence_Numeric,
    HB_Sentence_ATerm,
    HB_Sentence_STerm,
    HB_Sentence_Close
} HB_SentenceClass;

HB_GraphemeClass HB_GetGraphemeClass(HB_UChar32 ch);
HB_WordClass HB_GetWordClass(HB_UChar32 ch);
HB_SentenceClass HB_GetSentenceClass(HB_UChar32 ch);
HB_LineBreakClass HB_GetLineBreakClass(HB_UChar32 ch);

void HB_GetGraphemeAndLineBreakClass(HB_UChar32 ch, HB_GraphemeClass *grapheme, HB_LineBreakClass *lineBreak);
void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass);
HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch);
int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch);
HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch);

void *HB_Library_Resolve(const char *library, int version, const char *symbol);

HB_END_HEADER

#endif
