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

static const HB_UChar16 ReplacementCharacter = 0xfffd;

typedef struct {
    unsigned char shape;
    unsigned char justification;
} HB_ArabicProperties;

typedef enum {
    XIsolated,
    XFinal,
    XInitial,
    XMedial,
    /* intermediate state */
    XCausing
} ArabicShape;

/*
// these groups correspond to the groups defined in the Unicode standard.
// Some of these groups are equal with regards to both joining and line breaking behaviour,
// and thus have the same enum value
//
// I'm not sure the mapping of syriac to arabic enums is correct with regards to justification, but as
// I couldn't find any better document I'll hope for the best.
*/
typedef enum {
    /* NonJoining */
    ArabicNone,
    ArabicSpace,
    /* Transparent */
    Transparent,
    /* Causing */
    Center,
    Kashida,

    /* Arabic */
    /* Dual */
    Beh,
    Noon,
    Nya = Noon,
    Meem = Noon,
    Heh = Noon,
    KnottedHeh = Noon,
    HehGoal = Noon,
    SwashKaf = Noon,
    Yeh,
    FarsiYeh = Yeh,
    Hah,
    Seen,
    Sad = Seen,
    Tah,
    Kaf = Tah,
    Gaf = Tah,
    Lam = Tah,
    Ain,
    Feh = Ain,
    Qaf = Ain,
    /* Right */
    Alef,
    Waw,
    Dal,
    TehMarbuta = Dal,
    Reh,
    TehMarbutaGoal,
    HamzaOnHehGoal = TehMarbutaGoal, /* has been retained as a property value alias */
    YehWithTail = TehMarbutaGoal,
    YehBarree = TehMarbutaGoal,

    /* Syriac */
    /* Dual */
    Beth = Beh,
    Gamal = Ain,
    Heth = Noon,
    Teth = Hah,
    Yudh = Noon,
    Khaph = Noon,
    Lamadh = Lam,
    Mim = Noon,
    Nun = Noon,
    Semkath = Noon,
    FinalSemkath = Noon,
    SyriacE = Ain,
    Pe = Ain,
    ReversedPe = Hah,
    Qaph = Noon,
    Shin = Noon,
    Fe = Ain,

    /* Right */
    Alaph = Alef,
    DalathRish = Dal,
    He = Dal,
    SyriacWaw = Waw,
    Zhain = Alef,
    YudhHe = Waw,
    Sadhe = TehMarbutaGoal,
    Taw = Dal,

    /* Compiler bug? Otherwise ArabicGroupsEnd would be equal to Dal + 1. */
    Dummy = TehMarbutaGoal,
    ArabicGroupsEnd
} ArabicGroup;

static const unsigned char arabic_group[0x150] = {
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,

    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,

    Yeh, ArabicNone, Alef, Alef,
    Waw, Alef, Yeh, Alef,
    Beh, TehMarbuta, Beh, Beh,
    Hah, Hah, Hah, Dal,

    Dal, Reh, Reh, Seen,
    Seen, Sad, Sad, Tah,
    Tah, Ain, Ain, Gaf,
    Gaf, FarsiYeh, FarsiYeh, FarsiYeh,

    /* 0x640 */
    Kashida, Feh, Qaf, Kaf,
    Lam, Meem, Noon, Heh,
    Waw, Yeh, Yeh, Transparent,
    Transparent, Transparent, Transparent, Transparent,

    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,

    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, Beh, Qaf,

    Transparent, Alef, Alef, Alef,
    ArabicNone, Alef, Waw, Waw,
    Yeh, Beh, Beh, Beh,
    Beh, Beh, Beh, Beh,

    /* 0x680 */
    Beh, Hah, Hah, Hah,
    Hah, Hah, Hah, Hah,
    Dal, Dal, Dal, Dal,
    Dal, Dal, Dal, Dal,

    Dal, Reh, Reh, Reh,
    Reh, Reh, Reh, Reh,
    Reh, Reh, Seen, Seen,
    Seen, Sad, Sad, Tah,

    Ain, Feh, Feh, Feh,
    Feh, Feh, Feh, Qaf,
    Qaf, Gaf, SwashKaf, Gaf,
    Kaf, Kaf, Kaf, Gaf,

    Gaf, Gaf, Gaf, Gaf,
    Gaf, Lam, Lam, Lam,
    Lam, Noon, Noon, Noon,
    Noon, Nya, KnottedHeh, Hah,

    /* 0x6c0 */
    TehMarbuta, HehGoal, HehGoal, TehMarbutaGoal,
    Waw, Waw, Waw, Waw,
    Waw, Waw, Waw, Waw,
    FarsiYeh, YehWithTail, FarsiYeh, Waw,

    Yeh, Yeh, YehBarree, YehBarree,
    ArabicNone, TehMarbuta, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, ArabicNone, ArabicNone, Transparent,

    Transparent, Transparent, Transparent, Transparent,
    Transparent, ArabicNone, ArabicNone, Transparent,
    Transparent, ArabicNone, Transparent, Transparent,
    Transparent, Transparent, Dal, Reh,

    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, Seen, Sad,
    Ain, ArabicNone, ArabicNone, KnottedHeh,

    /* 0x700 */
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, ArabicNone,
    ArabicNone, ArabicNone, ArabicNone, Transparent,

    Alaph, Transparent, Beth, Gamal,
    Gamal, DalathRish, DalathRish, He,
    SyriacWaw, Zhain, Heth, Teth,
    Teth, Yudh, YudhHe, Khaph,

    Lamadh, Mim, Nun, Semkath,
    FinalSemkath, SyriacE, Pe, ReversedPe,
    Sadhe, Qaph, DalathRish, Shin,
    Taw, Beth, Gamal, DalathRish,

    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,

    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, Transparent,
    Transparent, Transparent, Transparent, ArabicNone,
    ArabicNone, Zhain, Khaph, Fe,
};

static ArabicGroup arabicGroup(unsigned short uc)
{
    if (uc >= 0x0600 && uc < 0x750)
        return (ArabicGroup) arabic_group[uc-0x600];
    else if (uc == 0x200d)
        return Center;
    else if (HB_GetUnicodeCharCategory(uc) == HB_Separator_Space)
        return ArabicSpace;
    else
        return ArabicNone;
}


/*
   Arabic shaping obeys a number of rules according to the joining classes (see Unicode book, section on
   arabic).

   Each unicode char has a joining class (right, dual (left&right), center (joincausing) or transparent).
   transparent joining is not encoded in HB_UChar16::joining(), but applies to all combining marks and format marks.

   Right join-causing: dual + center
   Left join-causing: dual + right + center

   Rules are as follows (for a string already in visual order, as we have it here):

   R1 Transparent characters do not affect joining behaviour.
   R2 A right joining character, that has a right join-causing char on the right will get form XRight
   (R3 A left joining character, that has a left join-causing char on the left will get form XLeft)
   Note: the above rule is meaningless, as there are no pure left joining characters defined in Unicode
   R4 A dual joining character, that has a left join-causing char on the left and a right join-causing char on
             the right will get form XMedial
   R5  A dual joining character, that has a right join causing char on the right, and no left join causing char on the left
         will get form XRight
   R6 A dual joining character, that has a  left join causing char on the left, and no right join causing char on the right
         will get form XLeft
   R7 Otherwise the character will get form XIsolated

   Additionally we have to do the minimal ligature support for lam-alef ligatures:

   L1 Transparent characters do not affect ligature behaviour.
   L2 Any sequence of Alef(XRight) + Lam(XMedial) will form the ligature Alef.Lam(XLeft)
   L3 Any sequence of Alef(XRight) + Lam(XLeft) will form the ligature Alef.Lam(XIsolated)

   The state table below handles rules R1-R7.
*/

typedef enum {
    JNone,
    JCausing,
    JDual,
    JRight,
    JTransparent
} Joining;

static const Joining joining_for_group[ArabicGroupsEnd] = {
    /* NonJoining */
    JNone, /* ArabicNone */
    JNone, /* ArabicSpace */
    /* Transparent */
    JTransparent, /* Transparent */
    /* Causing */
    JCausing, /* Center */
    JCausing, /* Kashida */
    /* Dual */
    JDual, /* Beh */
    JDual, /* Noon */
    JDual, /* Yeh */
    JDual, /* Hah */
    JDual, /* Seen */
    JDual, /* Tah */
    JDual, /* Ain */
    /* Right */
    JRight, /* Alef */
    JRight, /* Waw */
    JRight, /* Dal */
    JRight, /* Reh */
    JRight  /* TehMarbutaGoal */
};


typedef struct {
    ArabicShape form1;
    ArabicShape form2;
} JoiningPair;

static const JoiningPair joining_table[5][4] =
/* None, Causing, Dual, Right */
{
    { { XIsolated, XIsolated }, { XIsolated, XCausing }, { XIsolated, XInitial }, { XIsolated, XIsolated } }, /* XIsolated */
    { { XFinal, XIsolated }, { XFinal, XCausing }, { XFinal, XInitial }, { XFinal, XIsolated } }, /* XFinal */
    { { XIsolated, XIsolated }, { XInitial, XCausing }, { XInitial, XMedial }, { XInitial, XFinal } }, /* XInitial */
    { { XFinal, XIsolated }, { XMedial, XCausing }, { XMedial, XMedial }, { XMedial, XFinal } }, /* XMedial */
    { { XIsolated, XIsolated }, { XIsolated, XCausing }, { XIsolated, XMedial }, { XIsolated, XFinal } }, /* XCausing */
};


/*
According to http://www.microsoft.com/middleeast/Arabicdev/IE6/KBase.asp

1. Find the priority of the connecting opportunities in each word
2. Add expansion at the highest priority connection opportunity
3. If more than one connection opportunity have the same highest value,
   use the opportunity closest to the end of the word.

Following is a chart that provides the priority for connection
opportunities and where expansion occurs. The character group names
are those in table 6.6 of the UNICODE 2.0 book.


PrioritY        Glyph                   Condition                                       Kashida Location

Arabic_Kashida        User inserted Kashida   The user entered a Kashida in a position.       After the user
                (Shift+j or Shift+[E with hat])    Thus, it is the highest priority to insert an   inserted kashida
                                        automatic kashida.

Arabic_Seen        Seen, Sad               Connecting to the next character.               After the character.
                                        (Initial or medial form).

Arabic_HaaDal        Teh Marbutah, Haa, Dal  Connecting to previous character.               Before the final form
                                                                                        of these characters.

Arabic_Alef     Alef, Tah, Lam,         Connecting to previous character.               Before the final form
                Kaf and Gaf                                                             of these characters.

Arabic_BaRa     Reh, Yeh                Connected to medial Beh                         Before preceding medial Baa

Arabic_Waw        Waw, Ain, Qaf, Feh      Connecting to previous character.               Before the final form of
                                                                                        these characters.

Arabic_Normal   Other connecting        Connecting to previous character.               Before the final form
                characters                                                              of these characters.



This seems to imply that we have at most one kashida point per arabic word.

*/

static void getArabicProperties(const unsigned short *chars, int len, HB_ArabicProperties *properties)
{
/*     qDebug("arabicSyriacOpenTypeShape: properties:"); */
    int lastPos = 0;
    int lastGroup = ArabicNone;
    int i = 0;

    ArabicGroup group = arabicGroup(chars[0]);
    Joining j = joining_for_group[group];
    ArabicShape shape = joining_table[XIsolated][j].form2;
    properties[0].justification = HB_NoJustification;

    for (i = 1; i < len; ++i) {
        /* #### fix handling for spaces and punktuation */
        properties[i].justification = HB_NoJustification;

        group = arabicGroup(chars[i]);
        j = joining_for_group[group];

        if (j == JTransparent) {
            properties[i].shape = XIsolated;
            continue;
        }

        properties[lastPos].shape = joining_table[shape][j].form1;
        shape = joining_table[shape][j].form2;

        switch(lastGroup) {
        case Seen:
            if (properties[lastPos].shape == XInitial || properties[lastPos].shape == XMedial)
                properties[i-1].justification = HB_Arabic_Seen;
            break;
        case Hah:
            if (properties[lastPos].shape == XFinal)
                properties[lastPos-1].justification = HB_Arabic_HaaDal;
            break;
        case Alef:
            if (properties[lastPos].shape == XFinal)
                properties[lastPos-1].justification = HB_Arabic_Alef;
            break;
        case Ain:
            if (properties[lastPos].shape == XFinal)
                properties[lastPos-1].justification = HB_Arabic_Waw;
            break;
        case Noon:
            if (properties[lastPos].shape == XFinal)
                properties[lastPos-1].justification = HB_Arabic_Normal;
            break;
        case ArabicNone:
            break;

        default:
            assert(FALSE);
        }

        lastGroup = ArabicNone;

        switch(group) {
        case ArabicNone:
        case Transparent:
        /* ### Center should probably be treated as transparent when it comes to justification. */
        case Center:
            break;
        case ArabicSpace:
            properties[i].justification = HB_Arabic_Space;
            break;
        case Kashida:
            properties[i].justification = HB_Arabic_Kashida;
            break;
        case Seen:
            lastGroup = Seen;
            break;

        case Hah:
        case Dal:
            lastGroup = Hah;
            break;

        case Alef:
        case Tah:
            lastGroup = Alef;
            break;

        case Yeh:
        case Reh:
            if (properties[lastPos].shape == XMedial && arabicGroup(chars[lastPos]) == Beh)
                properties[lastPos-1].justification = HB_Arabic_BaRa;
            break;

        case Ain:
        case Waw:
            lastGroup = Ain;
            break;

        case Noon:
        case Beh:
        case TehMarbutaGoal:
            lastGroup = Noon;
            break;
        case ArabicGroupsEnd:
            assert(FALSE);
        }

        lastPos = i;
    }
    properties[lastPos].shape = joining_table[shape][JNone].form1;


    /*
     for (int i = 0; i < len; ++i)
         qDebug("arabic properties(%d): uc=%x shape=%d, justification=%d", i, chars[i], properties[i].shape, properties[i].justification);
    */
}

static Joining getNkoJoining(unsigned short uc)
{
    if (uc < 0x7ca)
        return JNone;
    if (uc <= 0x7ea)
        return JDual;
    if (uc <= 0x7f3)
        return JTransparent;
    if (uc <= 0x7f9)
        return JNone;
    if (uc == 0x7fa)
        return JCausing;
    return JNone;
}

static void getNkoProperties(const unsigned short *chars, int len, HB_ArabicProperties *properties)
{
    int lastPos = 0;
    int i = 0;

    Joining j = getNkoJoining(chars[0]);
    ArabicShape shape = joining_table[XIsolated][j].form2;
    properties[0].justification = HB_NoJustification;

    for (i = 1; i < len; ++i) {
        properties[i].justification = (HB_GetUnicodeCharCategory(chars[i]) == HB_Separator_Space) ?
                                      ArabicSpace : ArabicNone;

        j = getNkoJoining(chars[i]);

        if (j == JTransparent) {
            properties[i].shape = XIsolated;
            continue;
        }

        properties[lastPos].shape = joining_table[shape][j].form1;
        shape = joining_table[shape][j].form2;


        lastPos = i;
    }
    properties[lastPos].shape = joining_table[shape][JNone].form1;


    /*
     for (int i = 0; i < len; ++i)
         qDebug("nko properties(%d): uc=%x shape=%d, justification=%d", i, chars[i], properties[i].shape, properties[i].justification);
    */
}

/*
// The unicode to unicode shaping codec.
// does only presentation forms B at the moment, but that should be enough for
// simple display
*/
static const hb_uint16 arabicUnicodeMapping[256][2] = {
    /* base of shaped forms, and number-1 of them (0 for non shaping,
       1 for right binding and 3 for dual binding */

    /* These are just the glyphs available in Unicode,
       some characters are in R class, but have no glyphs in Unicode. */

    { 0x0600, 0 }, /* 0x0600 */
    { 0x0601, 0 }, /* 0x0601 */
    { 0x0602, 0 }, /* 0x0602 */
    { 0x0603, 0 }, /* 0x0603 */
    { 0x0604, 0 }, /* 0x0604 */
    { 0x0605, 0 }, /* 0x0605 */
    { 0x0606, 0 }, /* 0x0606 */
    { 0x0607, 0 }, /* 0x0607 */
    { 0x0608, 0 }, /* 0x0608 */
    { 0x0609, 0 }, /* 0x0609 */
    { 0x060A, 0 }, /* 0x060A */
    { 0x060B, 0 }, /* 0x060B */
    { 0x060C, 0 }, /* 0x060C */
    { 0x060D, 0 }, /* 0x060D */
    { 0x060E, 0 }, /* 0x060E */
    { 0x060F, 0 }, /* 0x060F */

    { 0x0610, 0 }, /* 0x0610 */
    { 0x0611, 0 }, /* 0x0611 */
    { 0x0612, 0 }, /* 0x0612 */
    { 0x0613, 0 }, /* 0x0613 */
    { 0x0614, 0 }, /* 0x0614 */
    { 0x0615, 0 }, /* 0x0615 */
    { 0x0616, 0 }, /* 0x0616 */
    { 0x0617, 0 }, /* 0x0617 */
    { 0x0618, 0 }, /* 0x0618 */
    { 0x0619, 0 }, /* 0x0619 */
    { 0x061A, 0 }, /* 0x061A */
    { 0x061B, 0 }, /* 0x061B */
    { 0x061C, 0 }, /* 0x061C */
    { 0x061D, 0 }, /* 0x061D */
    { 0x061E, 0 }, /* 0x061E */
    { 0x061F, 0 }, /* 0x061F */

    { 0x0620, 0 }, /* 0x0620 */
    { 0xFE80, 0 }, /* 0x0621            HAMZA */
    { 0xFE81, 1 }, /* 0x0622    R       ALEF WITH MADDA ABOVE */
    { 0xFE83, 1 }, /* 0x0623    R       ALEF WITH HAMZA ABOVE */
    { 0xFE85, 1 }, /* 0x0624    R       WAW WITH HAMZA ABOVE */
    { 0xFE87, 1 }, /* 0x0625    R       ALEF WITH HAMZA BELOW */
    { 0xFE89, 3 }, /* 0x0626    D       YEH WITH HAMZA ABOVE */
    { 0xFE8D, 1 }, /* 0x0627    R       ALEF */
    { 0xFE8F, 3 }, /* 0x0628    D       BEH */
    { 0xFE93, 1 }, /* 0x0629    R       TEH MARBUTA */
    { 0xFE95, 3 }, /* 0x062A    D       TEH */
    { 0xFE99, 3 }, /* 0x062B    D       THEH */
    { 0xFE9D, 3 }, /* 0x062C    D       JEEM */
    { 0xFEA1, 3 }, /* 0x062D    D       HAH */
    { 0xFEA5, 3 }, /* 0x062E    D       KHAH */
    { 0xFEA9, 1 }, /* 0x062F    R       DAL */

    { 0xFEAB, 1 }, /* 0x0630    R       THAL */
    { 0xFEAD, 1 }, /* 0x0631    R       REH */
    { 0xFEAF, 1 }, /* 0x0632    R       ZHAIN */
    { 0xFEB1, 3 }, /* 0x0633    D       SEEN */
    { 0xFEB5, 3 }, /* 0x0634    D       SHEEN */
    { 0xFEB9, 3 }, /* 0x0635    D       SAD */
    { 0xFEBD, 3 }, /* 0x0636    D       DAD */
    { 0xFEC1, 3 }, /* 0x0637    D       TAH */
    { 0xFEC5, 3 }, /* 0x0638    D       ZAH */
    { 0xFEC9, 3 }, /* 0x0639    D       AIN */
    { 0xFECD, 3 }, /* 0x063A    D       GHAIN */
    { 0x063B, 0 }, /* 0x063B */
    { 0x063C, 0 }, /* 0x063C */
    { 0x063D, 0 }, /* 0x063D */
    { 0x063E, 0 }, /* 0x063E */
    { 0x063F, 0 }, /* 0x063F */

    { 0x0640, 0 }, /* 0x0640    C       TATWEEL // ### Join Causing, only one glyph */
    { 0xFED1, 3 }, /* 0x0641    D       FEH */
    { 0xFED5, 3 }, /* 0x0642    D       QAF */
    { 0xFED9, 3 }, /* 0x0643    D       KAF */
    { 0xFEDD, 3 }, /* 0x0644    D       LAM */
    { 0xFEE1, 3 }, /* 0x0645    D       MEEM */
    { 0xFEE5, 3 }, /* 0x0646    D       NOON */
    { 0xFEE9, 3 }, /* 0x0647    D       HEH */
    { 0xFEED, 1 }, /* 0x0648    R       WAW */
    { 0x0649, 3 }, /* 0x0649            ALEF MAKSURA // ### Dual, glyphs not consecutive, handle in code. */
    { 0xFEF1, 3 }, /* 0x064A    D       YEH */
    { 0x064B, 0 }, /* 0x064B */
    { 0x064C, 0 }, /* 0x064C */
    { 0x064D, 0 }, /* 0x064D */
    { 0x064E, 0 }, /* 0x064E */
    { 0x064F, 0 }, /* 0x064F */

    { 0x0650, 0 }, /* 0x0650 */
    { 0x0651, 0 }, /* 0x0651 */
    { 0x0652, 0 }, /* 0x0652 */
    { 0x0653, 0 }, /* 0x0653 */
    { 0x0654, 0 }, /* 0x0654 */
    { 0x0655, 0 }, /* 0x0655 */
    { 0x0656, 0 }, /* 0x0656 */
    { 0x0657, 0 }, /* 0x0657 */
    { 0x0658, 0 }, /* 0x0658 */
    { 0x0659, 0 }, /* 0x0659 */
    { 0x065A, 0 }, /* 0x065A */
    { 0x065B, 0 }, /* 0x065B */
    { 0x065C, 0 }, /* 0x065C */
    { 0x065D, 0 }, /* 0x065D */
    { 0x065E, 0 }, /* 0x065E */
    { 0x065F, 0 }, /* 0x065F */

    { 0x0660, 0 }, /* 0x0660 */
    { 0x0661, 0 }, /* 0x0661 */
    { 0x0662, 0 }, /* 0x0662 */
    { 0x0663, 0 }, /* 0x0663 */
    { 0x0664, 0 }, /* 0x0664 */
    { 0x0665, 0 }, /* 0x0665 */
    { 0x0666, 0 }, /* 0x0666 */
    { 0x0667, 0 }, /* 0x0667 */
    { 0x0668, 0 }, /* 0x0668 */
    { 0x0669, 0 }, /* 0x0669 */
    { 0x066A, 0 }, /* 0x066A */
    { 0x066B, 0 }, /* 0x066B */
    { 0x066C, 0 }, /* 0x066C */
    { 0x066D, 0 }, /* 0x066D */
    { 0x066E, 0 }, /* 0x066E */
    { 0x066F, 0 }, /* 0x066F */

    { 0x0670, 0 }, /* 0x0670 */
    { 0xFB50, 1 }, /* 0x0671    R       ALEF WASLA */
    { 0x0672, 0 }, /* 0x0672 */
    { 0x0673, 0 }, /* 0x0673 */
    { 0x0674, 0 }, /* 0x0674 */
    { 0x0675, 0 }, /* 0x0675 */
    { 0x0676, 0 }, /* 0x0676 */
    { 0x0677, 0 }, /* 0x0677 */
    { 0x0678, 0 }, /* 0x0678 */
    { 0xFB66, 3 }, /* 0x0679    D       TTEH */
    { 0xFB5E, 3 }, /* 0x067A    D       TTEHEH */
    { 0xFB52, 3 }, /* 0x067B    D       BEEH */
    { 0x067C, 0 }, /* 0x067C */
    { 0x067D, 0 }, /* 0x067D */
    { 0xFB56, 3 }, /* 0x067E    D       PEH */
    { 0xFB62, 3 }, /* 0x067F    D       TEHEH */

    { 0xFB5A, 3 }, /* 0x0680    D       BEHEH */
    { 0x0681, 0 }, /* 0x0681 */
    { 0x0682, 0 }, /* 0x0682 */
    { 0xFB76, 3 }, /* 0x0683    D       NYEH */
    { 0xFB72, 3 }, /* 0x0684    D       DYEH */
    { 0x0685, 0 }, /* 0x0685 */
    { 0xFB7A, 3 }, /* 0x0686    D       TCHEH */
    { 0xFB7E, 3 }, /* 0x0687    D       TCHEHEH */
    { 0xFB88, 1 }, /* 0x0688    R       DDAL */
    { 0x0689, 0 }, /* 0x0689 */
    { 0x068A, 0 }, /* 0x068A */
    { 0x068B, 0 }, /* 0x068B */
    { 0xFB84, 1 }, /* 0x068C    R       DAHAL */
    { 0xFB82, 1 }, /* 0x068D    R       DDAHAL */
    { 0xFB86, 1 }, /* 0x068E    R       DUL */
    { 0x068F, 0 }, /* 0x068F */

    { 0x0690, 0 }, /* 0x0690 */
    { 0xFB8C, 1 }, /* 0x0691    R       RREH */
    { 0x0692, 0 }, /* 0x0692 */
    { 0x0693, 0 }, /* 0x0693 */
    { 0x0694, 0 }, /* 0x0694 */
    { 0x0695, 0 }, /* 0x0695 */
    { 0x0696, 0 }, /* 0x0696 */
    { 0x0697, 0 }, /* 0x0697 */
    { 0xFB8A, 1 }, /* 0x0698    R       JEH */
    { 0x0699, 0 }, /* 0x0699 */
    { 0x069A, 0 }, /* 0x069A */
    { 0x069B, 0 }, /* 0x069B */
    { 0x069C, 0 }, /* 0x069C */
    { 0x069D, 0 }, /* 0x069D */
    { 0x069E, 0 }, /* 0x069E */
    { 0x069F, 0 }, /* 0x069F */

    { 0x06A0, 0 }, /* 0x06A0 */
    { 0x06A1, 0 }, /* 0x06A1 */
    { 0x06A2, 0 }, /* 0x06A2 */
    { 0x06A3, 0 }, /* 0x06A3 */
    { 0xFB6A, 3 }, /* 0x06A4    D       VEH */
    { 0x06A5, 0 }, /* 0x06A5 */
    { 0xFB6E, 3 }, /* 0x06A6    D       PEHEH */
    { 0x06A7, 0 }, /* 0x06A7 */
    { 0x06A8, 0 }, /* 0x06A8 */
    { 0xFB8E, 3 }, /* 0x06A9    D       KEHEH */
    { 0x06AA, 0 }, /* 0x06AA */
    { 0x06AB, 0 }, /* 0x06AB */
    { 0x06AC, 0 }, /* 0x06AC */
    { 0xFBD3, 3 }, /* 0x06AD    D       NG */
    { 0x06AE, 0 }, /* 0x06AE */
    { 0xFB92, 3 }, /* 0x06AF    D       GAF */

    { 0x06B0, 0 }, /* 0x06B0 */
    { 0xFB9A, 3 }, /* 0x06B1    D       NGOEH */
    { 0x06B2, 0 }, /* 0x06B2 */
    { 0xFB96, 3 }, /* 0x06B3    D       GUEH */
    { 0x06B4, 0 }, /* 0x06B4 */
    { 0x06B5, 0 }, /* 0x06B5 */
    { 0x06B6, 0 }, /* 0x06B6 */
    { 0x06B7, 0 }, /* 0x06B7 */
    { 0x06B8, 0 }, /* 0x06B8 */
    { 0x06B9, 0 }, /* 0x06B9 */
    { 0xFB9E, 1 }, /* 0x06BA    R       NOON GHUNNA */
    { 0xFBA0, 3 }, /* 0x06BB    D       RNOON */
    { 0x06BC, 0 }, /* 0x06BC */
    { 0x06BD, 0 }, /* 0x06BD */
    { 0xFBAA, 3 }, /* 0x06BE    D       HEH DOACHASHMEE */
    { 0x06BF, 0 }, /* 0x06BF */

    { 0xFBA4, 1 }, /* 0x06C0    R       HEH WITH YEH ABOVE */
    { 0xFBA6, 3 }, /* 0x06C1    D       HEH GOAL */
    { 0x06C2, 0 }, /* 0x06C2 */
    { 0x06C3, 0 }, /* 0x06C3 */
    { 0x06C4, 0 }, /* 0x06C4 */
    { 0xFBE0, 1 }, /* 0x06C5    R       KIRGHIZ OE */
    { 0xFBD9, 1 }, /* 0x06C6    R       OE */
    { 0xFBD7, 1 }, /* 0x06C7    R       U */
    { 0xFBDB, 1 }, /* 0x06C8    R       YU */
    { 0xFBE2, 1 }, /* 0x06C9    R       KIRGHIZ YU */
    { 0x06CA, 0 }, /* 0x06CA */
    { 0xFBDE, 1 }, /* 0x06CB    R       VE */
    { 0xFBFC, 3 }, /* 0x06CC    D       FARSI YEH */
    { 0x06CD, 0 }, /* 0x06CD */
    { 0x06CE, 0 }, /* 0x06CE */
    { 0x06CF, 0 }, /* 0x06CF */

    { 0xFBE4, 3 }, /* 0x06D0    D       E */
    { 0x06D1, 0 }, /* 0x06D1 */
    { 0xFBAE, 1 }, /* 0x06D2    R       YEH BARREE */
    { 0xFBB0, 1 }, /* 0x06D3    R       YEH BARREE WITH HAMZA ABOVE */
    { 0x06D4, 0 }, /* 0x06D4 */
    { 0x06D5, 0 }, /* 0x06D5 */
    { 0x06D6, 0 }, /* 0x06D6 */
    { 0x06D7, 0 }, /* 0x06D7 */
    { 0x06D8, 0 }, /* 0x06D8 */
    { 0x06D9, 0 }, /* 0x06D9 */
    { 0x06DA, 0 }, /* 0x06DA */
    { 0x06DB, 0 }, /* 0x06DB */
    { 0x06DC, 0 }, /* 0x06DC */
    { 0x06DD, 0 }, /* 0x06DD */
    { 0x06DE, 0 }, /* 0x06DE */
    { 0x06DF, 0 }, /* 0x06DF */

    { 0x06E0, 0 }, /* 0x06E0 */
    { 0x06E1, 0 }, /* 0x06E1 */
    { 0x06E2, 0 }, /* 0x06E2 */
    { 0x06E3, 0 }, /* 0x06E3 */
    { 0x06E4, 0 }, /* 0x06E4 */
    { 0x06E5, 0 }, /* 0x06E5 */
    { 0x06E6, 0 }, /* 0x06E6 */
    { 0x06E7, 0 }, /* 0x06E7 */
    { 0x06E8, 0 }, /* 0x06E8 */
    { 0x06E9, 0 }, /* 0x06E9 */
    { 0x06EA, 0 }, /* 0x06EA */
    { 0x06EB, 0 }, /* 0x06EB */
    { 0x06EC, 0 }, /* 0x06EC */
    { 0x06ED, 0 }, /* 0x06ED */
    { 0x06EE, 0 }, /* 0x06EE */
    { 0x06EF, 0 }, /* 0x06EF */

    { 0x06F0, 0 }, /* 0x06F0 */
    { 0x06F1, 0 }, /* 0x06F1 */
    { 0x06F2, 0 }, /* 0x06F2 */
    { 0x06F3, 0 }, /* 0x06F3 */
    { 0x06F4, 0 }, /* 0x06F4 */
    { 0x06F5, 0 }, /* 0x06F5 */
    { 0x06F6, 0 }, /* 0x06F6 */
    { 0x06F7, 0 }, /* 0x06F7 */
    { 0x06F8, 0 }, /* 0x06F8 */
    { 0x06F9, 0 }, /* 0x06F9 */
    { 0x06FA, 0 }, /* 0x06FA */
    { 0x06FB, 0 }, /* 0x06FB */
    { 0x06FC, 0 }, /* 0x06FC */
    { 0x06FD, 0 }, /* 0x06FD */
    { 0x06FE, 0 }, /* 0x06FE */
    { 0x06FF, 0 }  /* 0x06FF */
};

/* the arabicUnicodeMapping does not work for U+0649 ALEF MAKSURA, this table does */
static const hb_uint16 alefMaksura[4] = {0xFEEF, 0xFEF0, 0xFBE8, 0xFBE9};

/*
// this is a bit tricky. Alef always binds to the right, so the second parameter descibing the shape
// of the lam can be either initial of medial. So initial maps to the isolated form of the ligature,
// medial to the final form
*/
static const hb_uint16 arabicUnicodeLamAlefMapping[6][4] = {
    { 0xfffd, 0xfffd, 0xfef5, 0xfef6 }, /* 0x622        R       Alef with Madda above */
    { 0xfffd, 0xfffd, 0xfef7, 0xfef8 }, /* 0x623        R       Alef with Hamza above */
    { 0xfffd, 0xfffd, 0xfffd, 0xfffd }, /* 0x624        // Just to fill the table ;-) */
    { 0xfffd, 0xfffd, 0xfef9, 0xfefa }, /* 0x625        R       Alef with Hamza below */
    { 0xfffd, 0xfffd, 0xfffd, 0xfffd }, /* 0x626        // Just to fill the table ;-) */
    { 0xfffd, 0xfffd, 0xfefb, 0xfefc }  /* 0x627        R       Alef */
};

static int getShape(hb_uint8 cell, int shape)
{
    /* the arabicUnicodeMapping does not work for U+0649 ALEF MAKSURA, handle this here */
    int ch = (cell != 0x49)
              ? (shape ? arabicUnicodeMapping[cell][0] + shape : 0x600+cell)
              : alefMaksura[shape] ;
    return ch;
}


/*
  Two small helper functions for arabic shaping.
*/
static HB_UChar16 prevChar(const HB_UChar16 *str, int pos)
{
    /*qDebug("leftChar: pos=%d", pos); */
    const HB_UChar16 *ch = str + pos - 1;
    pos--;
    while(pos > -1) {
        if(HB_GetUnicodeCharCategory(*ch) != HB_Mark_NonSpacing)
            return *ch;
        pos--;
        ch--;
    }
    return ReplacementCharacter;
}

static HB_UChar16 nextChar(const HB_UChar16 *str, hb_uint32 len, hb_uint32 pos)
{
    const HB_UChar16 *ch = str + pos + 1;
    pos++;
    while(pos < len) {
        /*qDebug("rightChar: %d isLetter=%d, joining=%d", pos, ch.isLetter(), ch.joining()); */
        if(HB_GetUnicodeCharCategory(*ch) != HB_Mark_NonSpacing)
            return *ch;
        /* assume it's a transparent char, this might not be 100% correct */
        pos++;
        ch++;
    }
    return ReplacementCharacter;
}

static void shapedString(const HB_UChar16 *uc, hb_uint32 stringLength, hb_uint32 from, hb_uint32 len, HB_UChar16 *shapeBuffer, int *shapedLength,
                         HB_Bool reverse, HB_GlyphAttributes *attributes, unsigned short *logClusters)
{
    HB_ArabicProperties *properties;
    hb_int32 f = from;
    hb_uint32 l = len;
    const HB_UChar16 *ch;
    HB_UChar16 *data;
    int clusterStart;
    hb_uint32 i;
    HB_STACKARRAY(HB_ArabicProperties, props, len + 2);
    properties = props;

    assert(stringLength >= from + len);

    if(len == 0) {
        *shapedLength = 0;
        return;
    }

    if (from > 0) {
        --f;
        ++l;
        ++properties;
    }
    if (f + l < stringLength)
        ++l;
    getArabicProperties(uc+f, l, props);

    ch = uc + from;
    data = shapeBuffer;
    clusterStart = 0;

    for (i = 0; i < len; i++) {
        hb_uint8 r = *ch >> 8;
        int gpos = data - shapeBuffer;

        if (r != 0x06) {
            if (r == 0x20) {
                if (*ch == 0x200c || *ch == 0x200d)
                    /* remove ZWJ and ZWNJ */
                    goto skip;
            }
            if (reverse)
                *data = HB_GetMirroredChar(*ch);
            else
                *data = *ch;
        } else {
            hb_uint8 c = *ch & 0xff;
            int pos = i + from;
            int shape = properties[i].shape;
/*            qDebug("mapping U+%x to shape %d glyph=0x%x", ch->unicode(), shape, getShape(c, shape)); */
            /* take care of lam-alef ligatures (lam right of alef) */
            hb_uint16 map;
            switch (c) {
                case 0x44: { /* lam */
                    const HB_UChar16 pch = nextChar(uc, stringLength, pos);
                    if ((pch >> 8) == 0x06) {
                        switch (pch & 0xff) {
                            case 0x22:
                            case 0x23:
                            case 0x25:
                            case 0x27:
/*                                 qDebug(" lam of lam-alef ligature"); */
                                map = arabicUnicodeLamAlefMapping[(pch & 0xff) - 0x22][shape];
                                goto next;
                            default:
                                break;
                        }
                    }
                    break;
                }
                case 0x22: /* alef with madda */
                case 0x23: /* alef with hamza above */
                case 0x25: /* alef with hamza below */
                case 0x27: /* alef */
                    if (prevChar(uc, pos) == 0x0644) {
                        /* have a lam alef ligature */
                        /*qDebug(" alef of lam-alef ligature"); */
                        goto skip;
                    }
                default:
                    break;
            }
            map = getShape(c, shape);
        next:
            *data = map;
        }
        /* ##### Fixme */
        /*glyphs[gpos].attributes.zeroWidth = zeroWidth; */
        if (HB_GetUnicodeCharCategory(*ch) == HB_Mark_NonSpacing) {
            attributes[gpos].mark = TRUE;
/*             qDebug("glyph %d (char %d) is mark!", gpos, i); */
        } else {
            attributes[gpos].mark = FALSE;
            clusterStart = data - shapeBuffer;
        }
        attributes[gpos].clusterStart = !attributes[gpos].mark;
        attributes[gpos].combiningClass = HB_GetUnicodeCharCombiningClass(*ch);
        attributes[gpos].justification = properties[i].justification;
/*         qDebug("data[%d] = %x (from %x)", gpos, (uint)data->unicode(), ch->unicode());*/
        data++;
    skip:
        ch++;
        logClusters[i] = clusterStart;
    }
    *shapedLength = data - shapeBuffer;

    HB_FREE_STACKARRAY(props);
}

#ifndef NO_OPENTYPE

static const HB_OpenTypeFeature arabic_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('i', 's', 'o', 'l'), IsolProperty },
    { HB_MAKE_TAG('f', 'i', 'n', 'a'), FinaProperty },
    { HB_MAKE_TAG('m', 'e', 'd', 'i'), MediProperty },
    { HB_MAKE_TAG('i', 'n', 'i', 't'), InitProperty },
    { HB_MAKE_TAG('r', 'l', 'i', 'g'), RligProperty },
    { HB_MAKE_TAG('c', 'a', 'l', 't'), CaltProperty },
    { HB_MAKE_TAG('l', 'i', 'g', 'a'), LigaProperty },
    { HB_MAKE_TAG('d', 'l', 'i', 'g'), DligProperty },
    { HB_MAKE_TAG('c', 's', 'w', 'h'), CswhProperty },
    /* mset is used in old Win95 fonts that don't have a 'mark' positioning table. */
    { HB_MAKE_TAG('m', 's', 'e', 't'), MsetProperty },
    {0, 0}
};

static const HB_OpenTypeFeature syriac_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('i', 's', 'o', 'l'), IsolProperty },
    { HB_MAKE_TAG('f', 'i', 'n', 'a'), FinaProperty },
    { HB_MAKE_TAG('f', 'i', 'n', '2'), FinaProperty },
    { HB_MAKE_TAG('f', 'i', 'n', '3'), FinaProperty },
    { HB_MAKE_TAG('m', 'e', 'd', 'i'), MediProperty },
    { HB_MAKE_TAG('m', 'e', 'd', '2'), MediProperty },
    { HB_MAKE_TAG('i', 'n', 'i', 't'), InitProperty },
    { HB_MAKE_TAG('r', 'l', 'i', 'g'), RligProperty },
    { HB_MAKE_TAG('c', 'a', 'l', 't'), CaltProperty },
    { HB_MAKE_TAG('l', 'i', 'g', 'a'), LigaProperty },
    { HB_MAKE_TAG('d', 'l', 'i', 'g'), DligProperty },
    {0, 0}
};

static HB_Bool arabicSyriacOpenTypeShape(HB_ShaperItem *item, HB_Bool *ot_ok)
{
    const HB_UChar16 *uc;
    const int nglyphs = item->num_glyphs;
    hb_int32 f;
    hb_uint32 l;
    HB_ArabicProperties *properties;
    HB_DECLARE_STACKARRAY(HB_ArabicProperties, props)
    HB_DECLARE_STACKARRAY(hb_uint32, apply)
    HB_Bool shaped;
    int i = 0;

    *ot_ok = TRUE;

    if (!HB_ConvertStringToGlyphIndices(item))
        return FALSE;
    HB_HeuristicSetGlyphAttributes(item);

    HB_INIT_STACKARRAY(HB_ArabicProperties, props, item->item.length + 2);
    HB_INIT_STACKARRAY(hb_uint32, apply, item->num_glyphs);

    uc = item->string + item->item.pos;

    properties = props;
    f = 0;
    l = item->item.length;
    if (item->item.pos > 0) {
        --f;
        ++l;
        ++properties;
    }
    if (f + l + item->item.pos < item->stringLength) {
        ++l;
    }
    if (item->item.script == HB_Script_Nko)
        getNkoProperties(uc+f, l, props);
    else
        getArabicProperties(uc+f, l, props);

    for (i = 0; i < (int)item->num_glyphs; i++) {
        apply[i] = 0;

        if (properties[i].shape == XIsolated)
            apply[i] |= MediProperty|FinaProperty|InitProperty;
        else if (properties[i].shape == XMedial)
            apply[i] |= IsolProperty|FinaProperty|InitProperty;
        else if (properties[i].shape == XFinal)
            apply[i] |= IsolProperty|MediProperty|InitProperty;
        else if (properties[i].shape == XInitial)
            apply[i] |= IsolProperty|MediProperty|FinaProperty;

        item->attributes[i].justification = properties[i].justification;
    }

    HB_FREE_STACKARRAY(props);

    shaped = HB_OpenTypeShape(item, apply);

    HB_FREE_STACKARRAY(apply);

    if (!shaped) {
        *ot_ok = FALSE;
        return FALSE;
    }
    return HB_OpenTypePosition(item, nglyphs, /*doLogClusters*/TRUE);
}

#endif

/* #### stil missing: identify invalid character combinations */
HB_Bool HB_ArabicShape(HB_ShaperItem *item)
{
    int slen;
    HB_Bool haveGlyphs;
    HB_STACKARRAY(HB_UChar16, shapedChars, item->item.length);

    assert(item->item.script == HB_Script_Arabic || item->item.script == HB_Script_Syriac
           || item->item.script == HB_Script_Nko);

#ifndef NO_OPENTYPE

    if (HB_SelectScript(item, item->item.script == HB_Script_Arabic ? arabic_features : syriac_features)) {
        HB_Bool ot_ok;
        if (arabicSyriacOpenTypeShape(item, &ot_ok)) {
            HB_FREE_STACKARRAY(shapedChars);
            return TRUE;
        }
        if (ot_ok) {
            HB_FREE_STACKARRAY(shapedChars);
            return FALSE;
            /* fall through to the non OT code*/
        }
    }
#endif

    if (item->item.script != HB_Script_Arabic) {
        HB_FREE_STACKARRAY(shapedChars);
        return HB_BasicShape(item);
    }

    shapedString(item->string, item->stringLength, item->item.pos, item->item.length, shapedChars, &slen,
                  item->item.bidiLevel % 2,
                  item->attributes, item->log_clusters);

    haveGlyphs = item->font->klass
        ->convertStringToGlyphIndices(item->font,
                                      shapedChars, slen,
                                      item->glyphs, &item->num_glyphs,
                                      item->item.bidiLevel % 2);

    HB_FREE_STACKARRAY(shapedChars);

    if (!haveGlyphs)
        return FALSE;

    HB_HeuristicPosition(item);
    return TRUE;
}


