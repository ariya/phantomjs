/*
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies)
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

/*
//  Vocabulary
//      Base ->         A consonant or an independent vowel in its full (not subscript) form. It is the
//                      center of the syllable, it can be surrounded by coeng (subscript) consonants, vowels,
//                      split vowels, signs... but there is only one base in a syllable, it has to be coded as
//                      the first character of the syllable.
//      split vowel --> vowel that has two parts placed separately (e.g. Before and after the consonant).
//                      Khmer language has five of them. Khmer split vowels either have one part before the
//                      base and one after the base or they have a part before the base and a part above the base.
//                      The first part of all Khmer split vowels is the same character, identical to
//                      the glyph of Khmer dependent vowel SRA EI
//      coeng -->  modifier used in Khmer to construct coeng (subscript) consonants
//                 Differently than indian languages, the coeng modifies the consonant that follows it,
//                 not the one preceding it  Each consonant has two forms, the base form and the subscript form
//                 the base form is the normal one (using the consonants code-point), the subscript form is
//                 displayed when the combination coeng + consonant is encountered.
//      Consonant of type 1 -> A consonant which has subscript for that only occupies space under a base consonant
//      Consonant of type 2.-> Its subscript form occupies space under and before the base (only one, RO)
//      Consonant of Type 3 -> Its subscript form occupies space under and after the base (KHO, CHHO, THHO, BA, YO, SA)
//      Consonant shifter -> Khmer has to series of consonants. The same dependent vowel has different sounds
//                           if it is attached to a consonant of the first series or a consonant of the second series
//                           Most consonants have an equivalent in the other series, but some of theme exist only in
//                           one series (for example SA). If we want to use the consonant SA with a vowel sound that
//                           can only be done with a vowel sound that corresponds to a vowel accompanying a consonant
//                           of the other series, then we need to use a consonant shifter: TRIISAP or MUSIKATOAN
//                           x17C9 y x17CA. TRIISAP changes a first series consonant to second series sound and
//                           MUSIKATOAN a second series consonant to have a first series vowel sound.
//                           Consonant shifter are both normally supercript marks, but, when they are followed by a
//                           superscript, they change shape and take the form of subscript dependent vowel SRA U.
//                           If they are in the same syllable as a coeng consonant, Unicode 3.0 says that they
//                           should be typed before the coeng. Unicode 4.0 breaks the standard and says that it should
//                           be placed after the coeng consonant.
//      Dependent vowel ->   In khmer dependent vowels can be placed above, below, before or after the base
//                           Each vowel has its own position. Only one vowel per syllable is allowed.
//      Signs            ->  Khmer has above signs and post signs. Only one above sign and/or one post sign are
//                           Allowed in a syllable.
//
//
//   order is important here! This order must be the same that is found in each horizontal
//   line in the statetable for Khmer (see khmerStateTable) .
*/
enum KhmerCharClassValues {
    CC_RESERVED             =  0,
    CC_CONSONANT            =  1, /* Consonant of type 1 or independent vowel */
    CC_CONSONANT2           =  2, /* Consonant of type 2 */
    CC_CONSONANT3           =  3, /* Consonant of type 3 */
    CC_ZERO_WIDTH_NJ_MARK   =  4, /* Zero Width non joiner character (0x200C) */
    CC_CONSONANT_SHIFTER    =  5,
    CC_ROBAT                =  6, /* Khmer special diacritic accent -treated differently in state table */
    CC_COENG                =  7, /* Subscript consonant combining character */
    CC_DEPENDENT_VOWEL      =  8,
    CC_SIGN_ABOVE           =  9,
    CC_SIGN_AFTER           = 10,
    CC_ZERO_WIDTH_J_MARK    = 11, /* Zero width joiner character */
    CC_COUNT                = 12  /* This is the number of character classes */
};


enum KhmerCharClassFlags {
    CF_CLASS_MASK    = 0x0000FFFF,

    CF_CONSONANT     = 0x01000000,  /* flag to speed up comparing */
    CF_SPLIT_VOWEL   = 0x02000000,  /* flag for a split vowel -> the first part is added in front of the syllable */
    CF_DOTTED_CIRCLE = 0x04000000,  /* add a dotted circle if a character with this flag is the first in a syllable */
    CF_COENG         = 0x08000000,  /* flag to speed up comparing */
    CF_SHIFTER       = 0x10000000,  /* flag to speed up comparing */
    CF_ABOVE_VOWEL   = 0x20000000,  /* flag to speed up comparing */

    /* position flags */
    CF_POS_BEFORE    = 0x00080000,
    CF_POS_BELOW     = 0x00040000,
    CF_POS_ABOVE     = 0x00020000,
    CF_POS_AFTER     = 0x00010000,
    CF_POS_MASK      = 0x000f0000
};


/* Characters that get referred to by name */
enum KhmerChar {
    C_SIGN_ZWNJ     = 0x200C,
    C_SIGN_ZWJ      = 0x200D,
    C_RO            = 0x179A,
    C_VOWEL_AA      = 0x17B6,
    C_SIGN_NIKAHIT  = 0x17C6,
    C_VOWEL_E       = 0x17C1,
    C_COENG         = 0x17D2
};


/*
//  simple classes, they are used in the statetable (in this file) to control the length of a syllable
//  they are also used to know where a character should be placed (location in reference to the base character)
//  and also to know if a character, when independently displayed, should be displayed with a dotted-circle to
//  indicate error in syllable construction
*/
enum {
    _xx = CC_RESERVED,
    _sa = CC_SIGN_ABOVE | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _sp = CC_SIGN_AFTER | CF_DOTTED_CIRCLE| CF_POS_AFTER,
    _c1 = CC_CONSONANT | CF_CONSONANT,
    _c2 = CC_CONSONANT2 | CF_CONSONANT,
    _c3 = CC_CONSONANT3 | CF_CONSONANT,
    _rb = CC_ROBAT | CF_POS_ABOVE | CF_DOTTED_CIRCLE,
    _cs = CC_CONSONANT_SHIFTER | CF_DOTTED_CIRCLE | CF_SHIFTER,
    _dl = CC_DEPENDENT_VOWEL | CF_POS_BEFORE | CF_DOTTED_CIRCLE,
    _db = CC_DEPENDENT_VOWEL | CF_POS_BELOW | CF_DOTTED_CIRCLE,
    _da = CC_DEPENDENT_VOWEL | CF_POS_ABOVE | CF_DOTTED_CIRCLE | CF_ABOVE_VOWEL,
    _dr = CC_DEPENDENT_VOWEL | CF_POS_AFTER | CF_DOTTED_CIRCLE,
    _co = CC_COENG | CF_COENG | CF_DOTTED_CIRCLE,

    /* split vowel */
    _va = _da | CF_SPLIT_VOWEL,
    _vr = _dr | CF_SPLIT_VOWEL
};


/*
//   Character class: a character class value
//   ORed with character class flags.
*/
typedef unsigned long KhmerCharClass;


/*
//  Character class tables
//  _xx character does not combine into syllable, such as numbers, puntuation marks, non-Khmer signs...
//  _sa Sign placed above the base
//  _sp Sign placed after the base
//  _c1 Consonant of type 1 or independent vowel (independent vowels behave as type 1 consonants)
//  _c2 Consonant of type 2 (only RO)
//  _c3 Consonant of type 3
//  _rb Khmer sign robat u17CC. combining mark for subscript consonants
//  _cd Consonant-shifter
//  _dl Dependent vowel placed before the base (left of the base)
//  _db Dependent vowel placed below the base
//  _da Dependent vowel placed above the base
//  _dr Dependent vowel placed behind the base (right of the base)
//  _co Khmer combining mark COENG u17D2, combines with the consonant or independent vowel following
//      it to create a subscript consonant or independent vowel
//  _va Khmer split vowel in which the first part is before the base and the second one above the base
//  _vr Khmer split vowel in which the first part is before the base and the second one behind (right of) the base
*/
static const KhmerCharClass khmerCharClasses[] = {
    _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, /* 1780 - 178F */
    _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c2, _c1, _c1, _c1, _c3, _c3, /* 1790 - 179F */
    _c1, _c3, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, /* 17A0 - 17AF */
    _c1, _c1, _c1, _c1, _dr, _dr, _dr, _da, _da, _da, _da, _db, _db, _db, _va, _vr, /* 17B0 - 17BF */
    _vr, _dl, _dl, _dl, _vr, _vr, _sa, _sp, _sp, _cs, _cs, _sa, _rb, _sa, _sa, _sa, /* 17C0 - 17CF */
    _sa, _sa, _co, _sa, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _sa, _xx, _xx  /* 17D0 - 17DF */
};

/* this enum must reflect the range of khmerCharClasses */
enum KhmerCharClassesRange {
    KhmerFirstChar = 0x1780,
    KhmerLastChar  = 0x17df
};

/*
//  Below we define how a character in the input string is either in the khmerCharClasses table
//  (in which case we get its type back), a ZWJ or ZWNJ (two characters that may appear
//  within the syllable, but are not in the table) we also get their type back, or an unknown object
//  in which case we get _xx (CC_RESERVED) back
*/
static KhmerCharClass getKhmerCharClass(HB_UChar16 uc)
{
    if (uc == C_SIGN_ZWJ) {
        return CC_ZERO_WIDTH_J_MARK;
    }

    if (uc == C_SIGN_ZWNJ) {
        return CC_ZERO_WIDTH_NJ_MARK;
    }

    if (uc < KhmerFirstChar || uc > KhmerLastChar) {
        return CC_RESERVED;
    }

    return khmerCharClasses[uc - KhmerFirstChar];
}


/*
//  The stateTable is used to calculate the end (the length) of a well
//  formed Khmer Syllable.
//
//  Each horizontal line is ordered exactly the same way as the values in KhmerClassTable
//  CharClassValues. This coincidence of values allows the follow up of the table.
//
//  Each line corresponds to a state, which does not necessarily need to be a type
//  of component... for example, state 2 is a base, with is always a first character
//  in the syllable, but the state could be produced a consonant of any type when
//  it is the first character that is analysed (in ground state).
//
//  Differentiating 3 types of consonants is necessary in order to
//  forbid the use of certain combinations, such as having a second
//  coeng after a coeng RO,
//  The inexistent possibility of having a type 3 after another type 3 is permitted,
//  eliminating it would very much complicate the table, and it does not create typing
//  problems, as the case above.
//
//  The table is quite complex, in order to limit the number of coeng consonants
//  to 2 (by means of the table).
//
//  There a peculiarity, as far as Unicode is concerned:
//  - The consonant-shifter is considered in two possible different
//    locations, the one considered in Unicode 3.0 and the one considered in
//    Unicode 4.0. (there is a backwards compatibility problem in this standard).
//
//
//  xx    independent character, such as a number, punctuation sign or non-khmer char
//
//  c1    Khmer consonant of type 1 or an independent vowel
//        that is, a letter in which the subscript for is only under the
//        base, not taking any space to the right or to the left
//
//  c2    Khmer consonant of type 2, the coeng form takes space under
//        and to the left of the base (only RO is of this type)
//
//  c3    Khmer consonant of type 3. Its subscript form takes space under
//        and to the right of the base.
//
//  cs    Khmer consonant shifter
//
//  rb    Khmer robat
//
//  co    coeng character (u17D2)
//
//  dv    dependent vowel (including split vowels, they are treated in the same way).
//        even if dv is not defined above, the component that is really tested for is
//        KhmerClassTable::CC_DEPENDENT_VOWEL, which is common to all dependent vowels
//
//  zwj   Zero Width joiner
//
//  zwnj  Zero width non joiner
//
//  sa    above sign
//
//  sp    post sign
//
//  there are lines with equal content but for an easier understanding
//  (and maybe change in the future) we did not join them
*/
static const signed char khmerStateTable[][CC_COUNT] =
{
    /* xx  c1  c2  c3 zwnj cs  rb  co  dv  sa  sp zwj */
    { 1,  2,  2,  2,  1,  1,  1,  6,  1,  1,  1,  2}, /*  0 - ground state */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  1 - exit state (or sign to the right of the syllable) */
    {-1, -1, -1, -1,  3,  4,  5,  6, 16, 17,  1, -1}, /*  2 - Base consonant */
    {-1, -1, -1, -1, -1,  4, -1, -1, 16, -1, -1, -1}, /*  3 - First ZWNJ before a register shifter It can only be followed by a shifter or a vowel */
    {-1, -1, -1, -1, 15, -1, -1,  6, 16, 17,  1, 14}, /*  4 - First register shifter */
    {-1, -1, -1, -1, -1, -1, -1, -1, 20, -1,  1, -1}, /*  5 - Robat */
    {-1,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1}, /*  6 - First Coeng */
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  7 - First consonant of type 1 after coeng */
    {-1, -1, -1, -1, 12, 13, -1, -1, 16, 17,  1, 14}, /*  8 - First consonant of type 2 after coeng */
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  9 - First consonant or type 3 after ceong */
    {-1, 11, 11, 11, -1, -1, -1, -1, -1, -1, -1, -1}, /* 10 - Second Coeng (no register shifter before) */
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 11 - Second coeng consonant (or ind. vowel) no register shifter before */
    {-1, -1, -1, -1, -1, 13, -1, -1, 16, -1, -1, -1}, /* 12 - Second ZWNJ before a register shifter */
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 13 - Second register shifter */
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 14 - ZWJ before vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 15 - ZWNJ before vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17,  1, 18}, /* 16 - dependent vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, 18}, /* 17 - sign above */
    {-1, -1, -1, -1, -1, -1, -1, 19, -1, -1, -1, -1}, /* 18 - ZWJ after vowel */
    {-1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 19 - Third coeng */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, /* 20 - dependent vowel after a Robat */
};


/*  #define KHMER_DEBUG */
#ifdef KHMER_DEBUG
#define KHDEBUG qDebug
#else
#define KHDEBUG if(0) printf
#endif

/*
//  Given an input string of characters and a location in which to start looking
//  calculate, using the state table, which one is the last character of the syllable
//  that starts in the starting position.
*/
static int khmer_nextSyllableBoundary(const HB_UChar16 *s, int start, int end, HB_Bool *invalid)
{
    const HB_UChar16 *uc = s + start;
    int state = 0;
    int pos = start;
    *invalid = FALSE;

    while (pos < end) {
        KhmerCharClass charClass = getKhmerCharClass(*uc);
        if (pos == start) {
            *invalid = (charClass > 0) && ! (charClass & CF_CONSONANT);
        }
        state = khmerStateTable[state][charClass & CF_CLASS_MASK];

        KHDEBUG("state[%d]=%d class=%8lx (uc=%4x)", pos - start, state,
                charClass, *uc );

        if (state < 0) {
            break;
        }
        ++uc;
        ++pos;
    }
    return pos;
}

#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature khmer_features[] = {
    { HB_MAKE_TAG( 'p', 'r', 'e', 'f' ), PreFormProperty },
    { HB_MAKE_TAG( 'b', 'l', 'w', 'f' ), BelowFormProperty },
    { HB_MAKE_TAG( 'a', 'b', 'v', 'f' ), AboveFormProperty },
    { HB_MAKE_TAG( 'p', 's', 't', 'f' ), PostFormProperty },
    { HB_MAKE_TAG( 'p', 'r', 'e', 's' ), PreSubstProperty },
    { HB_MAKE_TAG( 'b', 'l', 'w', 's' ), BelowSubstProperty },
    { HB_MAKE_TAG( 'a', 'b', 'v', 's' ), AboveSubstProperty },
    { HB_MAKE_TAG( 'p', 's', 't', 's' ), PostSubstProperty },
    { HB_MAKE_TAG( 'c', 'l', 'i', 'g' ), CligProperty },
    { 0, 0 }
};
#endif


static HB_Bool khmer_shape_syllable(HB_Bool openType, HB_ShaperItem *item)
{
/*    KHDEBUG("syllable from %d len %d, str='%s'", item->from, item->length,
  	    item->string->mid(item->from, item->length).toUtf8().data()); */

    int len = 0;
    int syllableEnd = item->item.pos + item->item.length;
    unsigned short reordered[16];
    unsigned char properties[16];
    enum {
	AboveForm = 0x01,
	PreForm = 0x02,
	PostForm = 0x04,
	BelowForm = 0x08
    };
#ifndef NO_OPENTYPE
    const int availableGlyphs = item->num_glyphs;
#endif
    int coengRo;
    int i;

    /* according to the specs this is the max length one can get
       ### the real value should be smaller */
    assert(item->item.length < 13);

    memset(properties, 0, 16*sizeof(unsigned char));

#ifdef KHMER_DEBUG
    qDebug("original:");
    for (int i = from; i < syllableEnd; i++) {
        qDebug("    %d: %4x", i, string[i]);
    }
#endif

    /*
    // write a pre vowel or the pre part of a split vowel first
    // and look out for coeng + ro. RO is the only vowel of type 2, and
    // therefore the only one that requires saving space before the base.
    */
    coengRo = -1;  /* There is no Coeng Ro, if found this value will change */
    for (i = item->item.pos; i < syllableEnd; i += 1) {
        KhmerCharClass charClass = getKhmerCharClass(item->string[i]);

        /* if a split vowel, write the pre part. In Khmer the pre part
           is the same for all split vowels, same glyph as pre vowel C_VOWEL_E */
        if (charClass & CF_SPLIT_VOWEL) {
            reordered[len] = C_VOWEL_E;
            properties[len] = PreForm;
            ++len;
            break; /* there can be only one vowel */
        }
        /* if a vowel with pos before write it out */
        if (charClass & CF_POS_BEFORE) {
            reordered[len] = item->string[i];
            properties[len] = PreForm;
            ++len;
            break; /* there can be only one vowel */
        }
        /* look for coeng + ro and remember position
           works because coeng + ro is always in front of a vowel (if there is a vowel)
           and because CC_CONSONANT2 is enough to identify it, as it is the only consonant
           with this flag */
        if ( (charClass & CF_COENG) && (i + 1 < syllableEnd) &&
              ( (getKhmerCharClass(item->string[i+1]) & CF_CLASS_MASK) == CC_CONSONANT2) ) {
            coengRo = i;
        }
    }

    /* write coeng + ro if found */
    if (coengRo > -1) {
        reordered[len] = C_COENG;
        properties[len] = PreForm;
        ++len;
        reordered[len] = C_RO;
        properties[len] = PreForm;
        ++len;
    }

    /*
       shall we add a dotted circle?
       If in the position in which the base should be (first char in the string) there is
       a character that has the Dotted circle flag (a character that cannot be a base)
       then write a dotted circle */
    if (getKhmerCharClass(item->string[item->item.pos]) & CF_DOTTED_CIRCLE) {
        reordered[len] = C_DOTTED_CIRCLE;
        ++len;
    }

    /* copy what is left to the output, skipping before vowels and
       coeng Ro if they are present */
    for (i = item->item.pos; i < syllableEnd; i += 1) {
        HB_UChar16 uc = item->string[i];
        KhmerCharClass charClass = getKhmerCharClass(uc);

        /* skip a before vowel, it was already processed */
        if (charClass & CF_POS_BEFORE) {
            continue;
        }

        /* skip coeng + ro, it was already processed */
        if (i == coengRo) {
            i += 1;
            continue;
        }

        switch (charClass & CF_POS_MASK)
        {
            case CF_POS_ABOVE :
                reordered[len] = uc;
                properties[len] = AboveForm;
                ++len;
                break;

            case CF_POS_AFTER :
                reordered[len] = uc;
                properties[len] = PostForm;
                ++len;
                break;

            case CF_POS_BELOW :
                reordered[len] = uc;
                properties[len] = BelowForm;
                ++len;
                break;

            default:
                /* assign the correct flags to a coeng consonant
                   Consonants of type 3 are taged as Post forms and those type 1 as below forms */
                if ( (charClass & CF_COENG) && i + 1 < syllableEnd ) {
                    unsigned char property = (getKhmerCharClass(item->string[i+1]) & CF_CLASS_MASK) == CC_CONSONANT3 ?
                                              PostForm : BelowForm;
                    reordered[len] = uc;
                    properties[len] = property;
                    ++len;
                    i += 1;
                    reordered[len] = item->string[i];
                    properties[len] = property;
                    ++len;
                    break;
                }

                /* if a shifter is followed by an above vowel change the shifter to below form,
                   an above vowel can have two possible positions i + 1 or i + 3
                   (position i+1 corresponds to unicode 3, position i+3 to Unicode 4)
                   and there is an extra rule for C_VOWEL_AA + C_SIGN_NIKAHIT also for two
                   different positions, right after the shifter or after a vowel (Unicode 4) */
                if ( (charClass & CF_SHIFTER) && (i + 1 < syllableEnd) ) {
                    if (getKhmerCharClass(item->string[i+1]) & CF_ABOVE_VOWEL ) {
                        reordered[len] = uc;
                        properties[len] = BelowForm;
                        ++len;
                        break;
                    }
                    if (i + 2 < syllableEnd &&
                        (item->string[i+1] == C_VOWEL_AA) &&
                        (item->string[i+2] == C_SIGN_NIKAHIT) )
                    {
                        reordered[len] = uc;
                        properties[len] = BelowForm;
                        ++len;
                        break;
                    }
                    if (i + 3 < syllableEnd && (getKhmerCharClass(item->string[i+3]) & CF_ABOVE_VOWEL) ) {
                        reordered[len] = uc;
                        properties[len] = BelowForm;
                        ++len;
                        break;
                    }
                    if (i + 4 < syllableEnd &&
                        (item->string[i+3] == C_VOWEL_AA) &&
                        (item->string[i+4] == C_SIGN_NIKAHIT) )
                    {
                        reordered[len] = uc;
                        properties[len] = BelowForm;
                        ++len;
                        break;
                    }
                }

                /* default - any other characters */
                reordered[len] = uc;
                ++len;
                break;
        } /* switch */
    } /* for */

    if (!item->font->klass->convertStringToGlyphIndices(item->font,
                                                        reordered, len,
                                                        item->glyphs, &item->num_glyphs,
                                                        item->item.bidiLevel % 2))
        return FALSE;


    KHDEBUG("after shaping: len=%d", len);
    for (i = 0; i < len; i++) {
	item->attributes[i].mark = FALSE;
	item->attributes[i].clusterStart = FALSE;
	item->attributes[i].justification = 0;
	item->attributes[i].zeroWidth = FALSE;
	KHDEBUG("    %d: %4x property=%x", i, reordered[i], properties[i]);
    }

    /* now we have the syllable in the right order, and can start running it through open type. */

#ifndef NO_OPENTYPE
    if (openType) {
 	hb_uint32 where[16];
        for (i = 0; i < len; ++i) {
            where[i] = ~(PreSubstProperty
                         | BelowSubstProperty
                         | AboveSubstProperty
                         | PostSubstProperty
                         | CligProperty
                         | PositioningProperties);
            if (properties[i] == PreForm)
                where[i] &= ~PreFormProperty;
            else if (properties[i] == BelowForm)
                where[i] &= ~BelowFormProperty;
            else if (properties[i] == AboveForm)
                where[i] &= ~AboveFormProperty;
            else if (properties[i] == PostForm)
                where[i] &= ~PostFormProperty;
        }

        HB_OpenTypeShape(item, where);
        if (!HB_OpenTypePosition(item, availableGlyphs, /*doLogClusters*/FALSE))
            return FALSE;
    } else
#endif
    {
	KHDEBUG("Not using openType");
        HB_HeuristicPosition(item);
    }

    item->attributes[0].clusterStart = TRUE;
    return TRUE;
}

HB_Bool HB_KhmerShape(HB_ShaperItem *item)
{
    HB_Bool openType = FALSE;
    unsigned short *logClusters = item->log_clusters;
    int i;

    HB_ShaperItem syllable = *item;
    int first_glyph = 0;

    int sstart = item->item.pos;
    int end = sstart + item->item.length;

    assert(item->item.script == HB_Script_Khmer);

#ifndef NO_OPENTYPE
    openType = HB_SelectScript(item, khmer_features);
#endif

    KHDEBUG("khmer_shape: from %d length %d", item->item.pos, item->item.length);
    while (sstart < end) {
        HB_Bool invalid;
        int send = khmer_nextSyllableBoundary(item->string, sstart, end, &invalid);
        KHDEBUG("syllable from %d, length %d, invalid=%s", sstart, send-sstart,
               invalid ? "TRUE" : "FALSE");
        syllable.item.pos = sstart;
        syllable.item.length = send-sstart;
        syllable.glyphs = item->glyphs + first_glyph;
        syllable.attributes = item->attributes + first_glyph;
        syllable.offsets = item->offsets + first_glyph;
        syllable.advances = item->advances + first_glyph;
        syllable.num_glyphs = item->num_glyphs - first_glyph;
        if (!khmer_shape_syllable(openType, &syllable)) {
            KHDEBUG("syllable shaping failed, syllable requests %d glyphs", syllable.num_glyphs);
            item->num_glyphs += syllable.num_glyphs;
            return FALSE;
        }
        /* fix logcluster array */
        KHDEBUG("syllable:");
        for (i = first_glyph; i < first_glyph + (int)syllable.num_glyphs; ++i)
            KHDEBUG("        %d -> glyph %x", i, item->glyphs[i]);
        KHDEBUG("    logclusters:");
        for (i = sstart; i < send; ++i) {
            KHDEBUG("        %d -> glyph %d", i, first_glyph);
            logClusters[i-item->item.pos] = first_glyph;
        }
        sstart = send;
        first_glyph += syllable.num_glyphs;
    }
    item->num_glyphs = first_glyph;
    return TRUE;
}

void HB_KhmerAttributes(HB_Script script, const HB_UChar16 *text, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes)
{
    int end = from + len;
    const HB_UChar16 *uc = text + from;
    hb_uint32 i = 0;
    HB_UNUSED(script);
    attributes += from;
    while ( i < len ) {
	HB_Bool invalid;
	hb_uint32 boundary = khmer_nextSyllableBoundary( text, from+i, end, &invalid ) - from;

    attributes[i].graphemeBoundary = TRUE;

	if ( boundary > len-1 ) boundary = len;
	i++;
	while ( i < boundary ) {
        attributes[i].graphemeBoundary = FALSE;
	    ++uc;
	    ++i;
	}
	assert( i == boundary );
    }
}

