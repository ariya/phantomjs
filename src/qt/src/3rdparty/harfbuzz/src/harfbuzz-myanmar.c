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

#include "harfbuzz-shaper.h"
#include "harfbuzz-shaper-private.h"

#include <assert.h>
#include <stdio.h>

enum MymrCharClassValues
{
    Mymr_CC_RESERVED             =  0,
    Mymr_CC_CONSONANT            =  1, /* Consonant of type 1, that has subscript form */
    Mymr_CC_CONSONANT2           =  2, /* Consonant of type 2, that has no subscript form */
    Mymr_CC_NGA	          =  3, /* Consonant NGA */
    Mymr_CC_YA		          =  4, /* Consonant YA */
    Mymr_CC_RA		          =  5, /* Consonant RA */
    Mymr_CC_WA		          =  6, /* Consonant WA */
    Mymr_CC_HA		          =  7, /* Consonant HA */
    Mymr_CC_IND_VOWEL		  =  8, /* Independent vowel */
    Mymr_CC_ZERO_WIDTH_NJ_MARK   =  9, /* Zero Width non joiner character (0x200C) */
    Mymr_CC_VIRAMA               = 10, /* Subscript consonant combining character */
    Mymr_CC_PRE_VOWEL   	  = 11, /* Dependent vowel, prebase (Vowel e) */
    Mymr_CC_BELOW_VOWEL   	  = 12, /* Dependent vowel, prebase (Vowel u, uu) */
    Mymr_CC_ABOVE_VOWEL   	  = 13, /* Dependent vowel, prebase (Vowel i, ii, ai) */
    Mymr_CC_POST_VOWEL   	  = 14, /* Dependent vowel, prebase (Vowel aa) */
    Mymr_CC_SIGN_ABOVE           = 15,
    Mymr_CC_SIGN_BELOW           = 16,
    Mymr_CC_SIGN_AFTER           = 17,
    Mymr_CC_ZERO_WIDTH_J_MARK    = 18, /* Zero width joiner character */
    Mymr_CC_COUNT                = 19  /* This is the number of character classes */
};

enum MymrCharClassFlags
{
    Mymr_CF_CLASS_MASK    = 0x0000FFFF,

    Mymr_CF_CONSONANT     = 0x01000000,  /* flag to speed up comparing */
    Mymr_CF_MEDIAL	   = 0x02000000,  /* flag to speed up comparing */
    Mymr_CF_IND_VOWEL 	   = 0x04000000,  /* flag to speed up comparing */
    Mymr_CF_DEP_VOWEL 	   = 0x08000000,  /* flag to speed up comparing */
    Mymr_CF_DOTTED_CIRCLE = 0x10000000,  /* add a dotted circle if a character with this flag is the first in a syllable */
    Mymr_CF_VIRAMA        = 0x20000000,  /* flag to speed up comparing */

    /* position flags */
    Mymr_CF_POS_BEFORE    = 0x00080000,
    Mymr_CF_POS_BELOW     = 0x00040000,
    Mymr_CF_POS_ABOVE     = 0x00020000,
    Mymr_CF_POS_AFTER     = 0x00010000,
    Mymr_CF_POS_MASK      = 0x000f0000,

    Mymr_CF_AFTER_KINZI   = 0x00100000
};

/* Characters that get refrered to by name */
enum MymrChar
{
    Mymr_C_SIGN_ZWNJ     = 0x200C,
    Mymr_C_SIGN_ZWJ      = 0x200D,
    Mymr_C_DOTTED_CIRCLE = 0x25CC,
    Mymr_C_RA            = 0x101B,
    Mymr_C_YA            = 0x101A,
    Mymr_C_NGA           = 0x1004,
    Mymr_C_VOWEL_E       = 0x1031,
    Mymr_C_VIRAMA        = 0x1039
};

enum
{
    Mymr_xx = Mymr_CC_RESERVED,
    Mymr_c1 = Mymr_CC_CONSONANT | Mymr_CF_CONSONANT | Mymr_CF_POS_BELOW,
    Mymr_c2 = Mymr_CC_CONSONANT2 | Mymr_CF_CONSONANT,
    Mymr_ng = Mymr_CC_NGA | Mymr_CF_CONSONANT | Mymr_CF_POS_ABOVE,
    Mymr_ya = Mymr_CC_YA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_AFTER | Mymr_CF_AFTER_KINZI,
    Mymr_ra = Mymr_CC_RA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BEFORE,
    Mymr_wa = Mymr_CC_WA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BELOW,
    Mymr_ha = Mymr_CC_HA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BELOW,
    Mymr_id = Mymr_CC_IND_VOWEL | Mymr_CF_IND_VOWEL,
    Mymr_vi = Mymr_CC_VIRAMA | Mymr_CF_VIRAMA | Mymr_CF_POS_ABOVE | Mymr_CF_DOTTED_CIRCLE,
    Mymr_dl = Mymr_CC_PRE_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_BEFORE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_db = Mymr_CC_BELOW_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_BELOW | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_da = Mymr_CC_ABOVE_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_ABOVE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_dr = Mymr_CC_POST_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_AFTER | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_sa = Mymr_CC_SIGN_ABOVE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_POS_ABOVE | Mymr_CF_AFTER_KINZI,
    Mymr_sb = Mymr_CC_SIGN_BELOW | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_POS_BELOW | Mymr_CF_AFTER_KINZI,
    Mymr_sp = Mymr_CC_SIGN_AFTER | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI
};


typedef int MymrCharClass;


static const MymrCharClass mymrCharClasses[] =
{
    Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_ng, Mymr_c1, Mymr_c1, Mymr_c1,
    Mymr_c1, Mymr_c1, Mymr_c2, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, /* 1000 - 100F */
    Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1,
    Mymr_c1, Mymr_c1, Mymr_ya, Mymr_ra, Mymr_c1, Mymr_wa, Mymr_c1, Mymr_ha, /* 1010 - 101F */
    Mymr_c2, Mymr_c2, Mymr_xx, Mymr_id, Mymr_id, Mymr_id, Mymr_id, Mymr_id,
    Mymr_xx, Mymr_id, Mymr_id, Mymr_xx, Mymr_dr, Mymr_da, Mymr_da, Mymr_db, /* 1020 - 102F */
    Mymr_db, Mymr_dl, Mymr_da, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_sa, Mymr_sb,
    Mymr_sp, Mymr_vi, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1030 - 103F */
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx,
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1040 - 104F */
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx,
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1050 - 105F */
};

static MymrCharClass
getMyanmarCharClass (HB_UChar16 ch)
{
    if (ch == Mymr_C_SIGN_ZWJ)
        return Mymr_CC_ZERO_WIDTH_J_MARK;

    if (ch == Mymr_C_SIGN_ZWNJ)
        return Mymr_CC_ZERO_WIDTH_NJ_MARK;

    if (ch < 0x1000 || ch > 0x105f)
        return Mymr_CC_RESERVED;

    return mymrCharClasses[ch - 0x1000];
}

static const signed char mymrStateTable[][Mymr_CC_COUNT] =
{
/*   xx  c1, c2  ng  ya  ra  wa  ha  id zwnj vi  dl  db  da  dr  sa  sb  sp zwj */
    { 1,  4,  4,  2,  4,  4,  4,  4, 24,  1, 27, 17, 18, 19, 20, 21,  1,  1,  4}, /*  0 - ground state */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  1 - exit state (or sp to the right of the syllable) */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3, 17, 18, 19, 20, 21, -1, -1,  4}, /*  2 - NGA */
    {-1,  4,  4,  4,  4,  4,  4,  4, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  3 - Virama after NGA */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5, 17, 18, 19, 20, 21,  1,  1, -1}, /*  4 - Base consonant */
    {-2,  6, -2, -2,  7,  8,  9, 10, -2, 23, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /*  5 - First virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, 17, 18, 19, 20, 21, -1, -1, -1}, /*  6 - c1 after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  7 - ya after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  8 - ra after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  9 - wa after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 10 - ha after virama */
    {-1, -1, -1, -1,  7,  8,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 11 - Virama after NGA+zwj */
    {-2, -2, -2, -2, -2, -2, 13, 14, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 12 - Second virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 17, 18, 19, 20, 21, -1, -1, -1}, /* 13 - wa after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 14 - ha after virama */
    {-2, -2, -2, -2, -2, -2, -2, 16, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 15 - Third virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 16 - ha after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21,  1,  1, -1}, /* 17 - dl, Dependent vowel e */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, -1, 21,  1,  1, -1}, /* 18 - db, Dependent vowel u,uu */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1, -1}, /* 19 - da, Dependent vowel i,ii,ai */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1,  1,  1, -1}, /* 20 - dr, Dependent vowel aa */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1, -1}, /* 21 - sa, Sign anusvara */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 22 - atha */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1, -1}, /* 23 - zwnj for atha */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, /* 24 - Independent vowel */
    {-2, -2, -2, -2, 26, 26, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 25 - Virama after subscript consonant */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1,  1, -1}, /* 26 - ra/ya after subscript consonant + virama */
    {-1,  6, -1, -1,  7,  8,  9, 10, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 27 - Virama after ground state */
/* exit state -2 is for invalid order of medials and combination of invalids
   with virama where virama should treat as start of next syllable
 */
};



/*#define MYANMAR_DEBUG */
#ifdef MYANMAR_DEBUG
#define MMDEBUG qDebug
#else
#define MMDEBUG if(0) printf
#endif

/*
//  Given an input string of characters and a location in which to start looking
//  calculate, using the state table, which one is the last character of the syllable
//  that starts in the starting position.
*/
static int myanmar_nextSyllableBoundary(const HB_UChar16 *s, int start, int end, HB_Bool *invalid)
{
    const HB_UChar16 *uc = s + start;
    int state = 0;
    int pos = start;
    *invalid = FALSE;

    while (pos < end) {
        MymrCharClass charClass = getMyanmarCharClass(*uc);
        state = mymrStateTable[state][charClass & Mymr_CF_CLASS_MASK];
        if (pos == start)
            *invalid = (HB_Bool)(charClass & Mymr_CF_DOTTED_CIRCLE);

        MMDEBUG("state[%d]=%d class=%8x (uc=%4x)", pos - start, state, charClass, *uc);

        if (state < 0) {
            if (state < -1)
                --pos;
            break;
        }
        ++uc;
        ++pos;
    }
    return pos;
}

#ifndef NO_OPENTYPE
/* ###### might have to change order of above and below forms and substitutions,
   but according to Unicode below comes before above */
static const HB_OpenTypeFeature myanmar_features[] = {
    { HB_MAKE_TAG('p', 'r', 'e', 'f'), PreFormProperty },
    { HB_MAKE_TAG('b', 'l', 'w', 'f'), BelowFormProperty },
    { HB_MAKE_TAG('a', 'b', 'v', 'f'), AboveFormProperty },
    { HB_MAKE_TAG('p', 's', 't', 'f'), PostFormProperty },
    { HB_MAKE_TAG('p', 'r', 'e', 's'), PreSubstProperty },
    { HB_MAKE_TAG('b', 'l', 'w', 's'), BelowSubstProperty },
    { HB_MAKE_TAG('a', 'b', 'v', 's'), AboveSubstProperty },
    { HB_MAKE_TAG('p', 's', 't', 's'), PostSubstProperty },
    { HB_MAKE_TAG('r', 'l', 'i', 'g'), CligProperty }, /* Myanmar1 uses this instead of the other features */
    { 0, 0 }
};
#endif


/*
// Visual order before shaping should be:
//
//    [Vowel Mark E]
//    [Virama + Medial Ra]
//    [Base]
//    [Virama + Consonant]
//    [Nga + Virama] (Kinzi) ### should probably come before post forms (medial ya)
//    [Vowels]
//    [Marks]
//
// This means that we can keep the logical order apart from having to
// move the pre vowel, medial ra and kinzi
*/

static HB_Bool myanmar_shape_syllable(HB_Bool openType, HB_ShaperItem *item, HB_Bool invalid)
{
    /*
//    MMDEBUG("\nsyllable from %d len %d, str='%s'", item->item.pos, item->item.length,
//	    item->string->mid(item->from, item->length).toUtf8().data());
    */

#ifndef NO_OPENTYPE
    const int availableGlyphs = item->num_glyphs;
#endif
    const HB_UChar16 *uc = item->string + item->item.pos;
    int vowel_e = -1;
    int kinzi = -1;
    int medial_ra = -1;
    int base = -1;
    int i;
    int len = 0;
    unsigned short reordered[32];
    unsigned char properties[32];
    enum {
	AboveForm = 0x01,
	PreForm = 0x02,
	PostForm = 0x04,
	BelowForm = 0x08
    };
    HB_Bool lastWasVirama = FALSE;
    int basePos = -1;

    memset(properties, 0, 32*sizeof(unsigned char));

    /* according to the table the max length of a syllable should be around 14 chars */
    assert(item->item.length < 32);

#ifdef MYANMAR_DEBUG
    printf("original:");
    for (i = 0; i < (int)item->item.length; i++) {
        printf("    %d: %4x", i, uc[i]);
    }
#endif
    for (i = 0; i < (int)item->item.length; ++i) {
        HB_UChar16 chr = uc[i];

        if (chr == Mymr_C_VOWEL_E) {
            vowel_e = i;
            continue;
        }
        if (i == 0
            && chr == Mymr_C_NGA
            && i + 2 < (int)item->item.length
            && uc[i+1] == Mymr_C_VIRAMA) {
            int mc = getMyanmarCharClass(uc[i+2]);
            /*MMDEBUG("maybe kinzi: mc=%x", mc);*/
            if ((mc & Mymr_CF_CONSONANT) == Mymr_CF_CONSONANT) {
                kinzi = i;
                continue;
            }
        }
        if (base >= 0
            && chr == Mymr_C_VIRAMA
            && i + 1 < (int)item->item.length
            && uc[i+1] == Mymr_C_RA) {
            medial_ra = i;
            continue;
        }
        if (base < 0)
            base = i;
    }

    MMDEBUG("\n  base=%d, vowel_e=%d, kinzi=%d, medial_ra=%d", base, vowel_e, kinzi, medial_ra);
    /* write vowel_e if found */
    if (vowel_e >= 0) {
        reordered[0] = Mymr_C_VOWEL_E;
        len = 1;
    }
    /* write medial_ra */
    if (medial_ra >= 0) {
        reordered[len] = Mymr_C_VIRAMA;
        reordered[len+1] = Mymr_C_RA;
        properties[len] = PreForm;
        properties[len+1] = PreForm;
        len += 2;
    }

    /* shall we add a dotted circle?
       If in the position in which the base should be (first char in the string) there is
       a character that has the Dotted circle flag (a character that cannot be a base)
       then write a dotted circle */
    if (invalid) {
        reordered[len] = C_DOTTED_CIRCLE;
        ++len;
    }

    /* copy the rest of the syllable to the output, inserting the kinzi
       at the correct place */
    for (i = 0; i < (int)item->item.length; ++i) {
        hb_uint16 chr = uc[i];
        MymrCharClass cc;
        if (i == vowel_e)
            continue;
        if (i == medial_ra || i == kinzi) {
            ++i;
            continue;
        }

        cc = getMyanmarCharClass(uc[i]);
        if (kinzi >= 0 && i > base && (cc & Mymr_CF_AFTER_KINZI)) {
            reordered[len] = Mymr_C_NGA;
            reordered[len+1] = Mymr_C_VIRAMA;
            if (len > 0)
                properties[len-1] = AboveForm;
            properties[len] = AboveForm;
            len += 2;
            kinzi = -1;
        }

        if (lastWasVirama) {
            int prop = 0;
            switch(cc & Mymr_CF_POS_MASK) {
            case Mymr_CF_POS_BEFORE:
                prop = PreForm;
                break;
            case Mymr_CF_POS_BELOW:
                prop = BelowForm;
                break;
            case Mymr_CF_POS_ABOVE:
                prop = AboveForm;
                break;
            case Mymr_CF_POS_AFTER:
                prop = PostForm;
                break;
            default:
                break;
            }
            properties[len-1] = prop;
            properties[len] = prop;
            if(basePos >= 0 && basePos == len-2)
                properties[len-2] = prop;
        }
        lastWasVirama = (chr == Mymr_C_VIRAMA);
        if(i == base)
            basePos = len;

        if ((chr != Mymr_C_SIGN_ZWNJ && chr != Mymr_C_SIGN_ZWJ) || !len) {
            reordered[len] = chr;
            ++len;
        }
    }
    if (kinzi >= 0) {
        reordered[len] = Mymr_C_NGA;
        reordered[len+1] = Mymr_C_VIRAMA;
        properties[len] = AboveForm;
        properties[len+1] = AboveForm;
        len += 2;
    }

    if (!item->font->klass->convertStringToGlyphIndices(item->font,
                                                        reordered, len,
                                                        item->glyphs, &item->num_glyphs,
                                                        item->item.bidiLevel % 2))
        return FALSE;

    MMDEBUG("after shaping: len=%d", len);
    for (i = 0; i < len; i++) {
	item->attributes[i].mark = FALSE;
	item->attributes[i].clusterStart = FALSE;
	item->attributes[i].justification = 0;
	item->attributes[i].zeroWidth = FALSE;
	MMDEBUG("    %d: %4x property=%x", i, reordered[i], properties[i]);
    }

    /* now we have the syllable in the right order, and can start running it through open type. */

#ifndef NO_OPENTYPE
    if (openType) {
 	hb_uint32 where[32];

        for (i = 0; i < len; ++i) {
            where[i] = ~(PreSubstProperty
                         | BelowSubstProperty
                         | AboveSubstProperty
                         | PostSubstProperty
                         | CligProperty
                         | PositioningProperties);
            if (properties[i] & PreForm)
                where[i] &= ~PreFormProperty;
            if (properties[i] & BelowForm)
                where[i] &= ~BelowFormProperty;
            if (properties[i] & AboveForm)
                where[i] &= ~AboveFormProperty;
            if (properties[i] & PostForm)
                where[i] &= ~PostFormProperty;
        }

        HB_OpenTypeShape(item, where);
        if (!HB_OpenTypePosition(item, availableGlyphs, /*doLogClusters*/FALSE))
            return FALSE;
    } else
#endif
    {
	MMDEBUG("Not using openType");
        HB_HeuristicPosition(item);
    }

    item->attributes[0].clusterStart = TRUE;
    return TRUE;
}

HB_Bool HB_MyanmarShape(HB_ShaperItem *item)
{
    HB_Bool openType = FALSE;
    unsigned short *logClusters = item->log_clusters;

    HB_ShaperItem syllable = *item;
    int first_glyph = 0;

    int sstart = item->item.pos;
    int end = sstart + item->item.length;
    int i = 0;

    assert(item->item.script == HB_Script_Myanmar);
#ifndef NO_OPENTYPE
    openType = HB_SelectScript(item, myanmar_features);
#endif

    MMDEBUG("myanmar_shape: from %d length %d", item->item.pos, item->item.length);
    while (sstart < end) {
        HB_Bool invalid;
        int send = myanmar_nextSyllableBoundary(item->string, sstart, end, &invalid);
        MMDEBUG("syllable from %d, length %d, invalid=%s", sstart, send-sstart,
               invalid ? "TRUE" : "FALSE");
        syllable.item.pos = sstart;
        syllable.item.length = send-sstart;
        syllable.glyphs = item->glyphs + first_glyph;
        syllable.attributes = item->attributes + first_glyph;
        syllable.advances = item->advances + first_glyph;
        syllable.offsets = item->offsets + first_glyph;
        syllable.num_glyphs = item->num_glyphs - first_glyph;
        if (!myanmar_shape_syllable(openType, &syllable, invalid)) {
            MMDEBUG("syllable shaping failed, syllable requests %d glyphs", syllable.num_glyphs);
            item->num_glyphs += syllable.num_glyphs;
            return FALSE;
        }

        /* fix logcluster array */
        MMDEBUG("syllable:");
        for (i = first_glyph; i < first_glyph + (int)syllable.num_glyphs; ++i)
            MMDEBUG("        %d -> glyph %x", i, item->glyphs[i]);
        MMDEBUG("    logclusters:");
        for (i = sstart; i < send; ++i) {
            MMDEBUG("        %d -> glyph %d", i, first_glyph);
            logClusters[i-item->item.pos] = first_glyph;
        }
        sstart = send;
        first_glyph += syllable.num_glyphs;
    }
    item->num_glyphs = first_glyph;
    return TRUE;
}

void HB_MyanmarAttributes(HB_Script script, const HB_UChar16 *text, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes)
{
    int end = from + len;
    const HB_UChar16 *uc = text + from;
    hb_uint32 i = 0;
    HB_UNUSED(script);
    attributes += from;
    while (i < len) {
	HB_Bool invalid;
	hb_uint32 boundary = myanmar_nextSyllableBoundary(text, from+i, end, &invalid) - from;

	attributes[i].charStop = TRUE;
        if (i)
            attributes[i-1].lineBreakType = HB_Break;

	if (boundary > len-1)
            boundary = len;
	i++;
	while (i < boundary) {
	    attributes[i].charStop = FALSE;
	    ++uc;
	    ++i;
	}
	assert(i == boundary);
    }
}

