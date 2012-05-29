/*
******************************************************************************
*
*   Copyright (C) 2000-2004, International Business Machines
*   Corporation and others. All Rights Reserved.
*   Copyright (C) 2007 Apple Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy of this
*   software and associated documentation files (the "Software"), to deal in the Software
*   without restriction, including without limitation the rights to use, copy, modify,
*   merge, publish, distribute, and/or sell copies of the Software, and to permit persons
*   to whom the Software is furnished to do so, provided that the above copyright notice(s)
*   and this permission notice appear in all copies of the Software and that both the above
*   copyright notice(s) and this permission notice appear in supporting documentation.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
*   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
*   PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER
*   OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR
*   CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
*   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
*   OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
*   Except as contained in this notice, the name of a copyright holder shall not be used in
*   advertising or otherwise to promote the sale, use or other dealings in this Software
*   without prior written authorization of the copyright holder.
*
******************************************************************************
*
*   Arabic letter shaping implemented by Ayman Roshdy
*/

#include "config.h"

#if USE(ATSUI)

#include "ShapeArabic.h"

#include <stdbool.h>
#include <string.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/ushape.h>
#include <wtf/Assertions.h>

/*
 * ### TODO in general for letter shaping:
 * - the letter shaping code is UTF-16-unaware; needs update
 *   + especially invertBuffer()?!
 * - needs to handle the "Arabic Tail" that is used in some legacy codepages
 *   as a glyph fragment of wide-glyph letters
 *   + IBM Unicode conversion tables map it to U+200B (ZWSP)
 *   + IBM Egypt has proposed to encode the tail in Unicode among Arabic Presentation Forms
 */

/* definitions for Arabic letter shaping ------------------------------------ */

#define IRRELEVANT 4
#define LAMTYPE    16
#define ALEFTYPE   32
#define LINKR      1
#define LINKL      2

static const UChar IrrelevantPos[] = {
    0x0, 0x2, 0x4, 0x6,
    0x8, 0xA, 0xC, 0xE,
};

static const UChar araLink[178]=
{
  1           + 32 + 256 * 0x11,/*0x0622*/
  1           + 32 + 256 * 0x13,/*0x0623*/
  1                + 256 * 0x15,/*0x0624*/
  1           + 32 + 256 * 0x17,/*0x0625*/
  1 + 2            + 256 * 0x19,/*0x0626*/
  1           + 32 + 256 * 0x1D,/*0x0627*/
  1 + 2            + 256 * 0x1F,/*0x0628*/
  1                + 256 * 0x23,/*0x0629*/
  1 + 2            + 256 * 0x25,/*0x062A*/
  1 + 2            + 256 * 0x29,/*0x062B*/
  1 + 2            + 256 * 0x2D,/*0x062C*/
  1 + 2            + 256 * 0x31,/*0x062D*/
  1 + 2            + 256 * 0x35,/*0x062E*/
  1                + 256 * 0x39,/*0x062F*/
  1                + 256 * 0x3B,/*0x0630*/
  1                + 256 * 0x3D,/*0x0631*/
  1                + 256 * 0x3F,/*0x0632*/
  1 + 2            + 256 * 0x41,/*0x0633*/
  1 + 2            + 256 * 0x45,/*0x0634*/
  1 + 2            + 256 * 0x49,/*0x0635*/
  1 + 2            + 256 * 0x4D,/*0x0636*/
  1 + 2            + 256 * 0x51,/*0x0637*/
  1 + 2            + 256 * 0x55,/*0x0638*/
  1 + 2            + 256 * 0x59,/*0x0639*/
  1 + 2            + 256 * 0x5D,/*0x063A*/
  0, 0, 0, 0, 0,                /*0x063B-0x063F*/
  1 + 2,                        /*0x0640*/
  1 + 2            + 256 * 0x61,/*0x0641*/
  1 + 2            + 256 * 0x65,/*0x0642*/
  1 + 2            + 256 * 0x69,/*0x0643*/
  1 + 2       + 16 + 256 * 0x6D,/*0x0644*/
  1 + 2            + 256 * 0x71,/*0x0645*/
  1 + 2            + 256 * 0x75,/*0x0646*/
  1 + 2            + 256 * 0x79,/*0x0647*/
  1                + 256 * 0x7D,/*0x0648*/
  1                + 256 * 0x7F,/*0x0649*/
  1 + 2            + 256 * 0x81,/*0x064A*/
  4, 4, 4, 4,                   /*0x064B-0x064E*/
  4, 4, 4, 4,                   /*0x064F-0x0652*/
  4, 4, 4, 0, 0,                /*0x0653-0x0657*/
  0, 0, 0, 0,                   /*0x0658-0x065B*/
  1                + 256 * 0x85,/*0x065C*/
  1                + 256 * 0x87,/*0x065D*/
  1                + 256 * 0x89,/*0x065E*/
  1                + 256 * 0x8B,/*0x065F*/
  0, 0, 0, 0, 0,                /*0x0660-0x0664*/
  0, 0, 0, 0, 0,                /*0x0665-0x0669*/
  0, 0, 0, 0, 0, 0,             /*0x066A-0x066F*/
  4,                            /*0x0670*/
  0,                            /*0x0671*/
  1           + 32,             /*0x0672*/
  1           + 32,             /*0x0673*/
  0,                            /*0x0674*/
  1           + 32,             /*0x0675*/
  1, 1,                         /*0x0676-0x0677*/
  1+2,                          /*0x0678*/
  1+2              + 256 * 0x16,/*0x0679*/
  1+2              + 256 * 0x0E,/*0x067A*/
  1+2              + 256 * 0x02,/*0x067B*/
  1+2, 1+2,                     /*0x067C-0x067D*/
  1+2              + 256 * 0x06,/*0x067E*/
  1+2              + 256 * 0x12,/*0x067F*/
  1+2              + 256 * 0x0A,/*0x0680*/
  1+2, 1+2,                     /*0x0681-0x0682*/
  1+2              + 256 * 0x26,/*0x0683*/
  1+2              + 256 * 0x22,/*0x0684*/
  1+2,                          /*0x0685*/
  1+2              + 256 * 0x2A,/*0x0686*/
  1+2              + 256 * 0x2E,/*0x0687*/
  1                + 256 * 0x38,/*0x0688*/
  1, 1, 1,                      /*0x0689-0x068B*/
  1                + 256 * 0x34,/*0x068C*/
  1                + 256 * 0x32,/*0x068D*/
  1                + 256 * 0x36,/*0x068E*/
  1, 1,                         /*0x068F-0x0690*/
  1                + 256 * 0x3C,/*0x0691*/
  1, 1, 1, 1, 1, 1,             /*0x0692-0x0697*/
  1                + 256 * 0x3A,/*0x0698*/
  1,                            /*0x0699*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x069A-0x069F*/
  1+2, 1+2, 1+2, 1+2,           /*0x06A0-0x06A3*/
  1+2              + 256 * 0x2E,/*0x06A4*/
  1+2,                          /*0x06A5*/
  1+2              + 256 * 0x1E,/*0x06A6*/
  1+2, 1+2,                     /*0x06A7-0x06A8*/
  1+2              + 256 * 0x3E,/*0x06A9*/
  1+2, 1+2, 1+2,                /*0x06AA-0x06AC*/
  1+2              + 256 * 0x83,/*0x06AD*/
  1+2,                          /*0x06AE*/
  1+2              + 256 * 0x42,/*0x06AF*/
  1+2,                          /*0x06B0*/
  1+2              + 256 * 0x4A,/*0x06B1*/
  1+2,                          /*0x06B2*/
  1+2              + 256 * 0x46,/*0x06B3*/
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, /*0x06B4-0x06B9*/
  1+2,                          /*0x06BA*/          // FIXME: Seems to have a final form
  1+2              + 256 * 0x50,/*0x06BB*/
  1+2, 1+2,                     /*0x06BC-0x06BD*/
  1+2              + 256 * 0x5A,/*0x06BE*/
  1+2,                          /*0x06BF*/
  1,                            /*0x06C0*/
  1+2              + 256 * 0x56,/*0x06C1*/
  1+2,                          /*0x06C2*/
  1, 1,                         /*0x06C3-0x06C4*/
  1                + 256 * 0x90,/*0x06C5*/
  1                + 256 * 0x89,/*0x06C6*/
  1                + 256 * 0x87,/*0x06C7*/
  1                + 256 * 0x8B,/*0x06C8*/
  1                + 256 * 0x92,/*0x06C9*/
  1,                            /*0x06CA*/
  1                + 256 * 0x8E,/*0x06CB*/
  1+2              + 256 * 0xAC,/*0x06CC*/
  1,                            /*0x06CD*/
  1+2,                          /*0x06CE*/
  1,                            /*0x06CF*/
  1+2              + 256 * 0x94,/*0x06D0*/
  1+2,                          /*0x06D1*/
  1                + 256 * 0x5E,/*0x06D2*/
  1                + 256 * 0x60 /*0x06D3*/
};

static const UChar presLink[141]=
{
  1 + 2,                        /*0xFE70*/
  1 + 2,                        /*0xFE71*/
  1 + 2, 0, 1+ 2, 0, 1+ 2,      /*0xFE72-0xFE76*/
  1 + 2,                        /*0xFE77*/
  1+ 2, 1 + 2, 1+2, 1 + 2,      /*0xFE78-0xFE81*/
  1+ 2, 1 + 2, 1+2, 1 + 2,      /*0xFE82-0xFE85*/
  0, 0 + 32, 1 + 32, 0 + 32,    /*0xFE86-0xFE89*/
  1 + 32, 0, 1,  0 + 32,        /*0xFE8A-0xFE8D*/
  1 + 32, 0, 2,  1 + 2,         /*0xFE8E-0xFE91*/
  1, 0 + 32, 1 + 32, 0,         /*0xFE92-0xFE95*/
  2, 1 + 2, 1, 0,               /*0xFE96-0xFE99*/
  1, 0, 2, 1 + 2,               /*0xFE9A-0xFE9D*/
  1, 0, 2, 1 + 2,               /*0xFE9E-0xFEA1*/
  1, 0, 2, 1 + 2,               /*0xFEA2-0xFEA5*/
  1, 0, 2, 1 + 2,               /*0xFEA6-0xFEA9*/
  1, 0, 2, 1 + 2,               /*0xFEAA-0xFEAD*/
  1, 0, 1, 0,                   /*0xFEAE-0xFEB1*/
  1, 0, 1, 0,                   /*0xFEB2-0xFEB5*/
  1, 0, 2, 1+2,                 /*0xFEB6-0xFEB9*/
  1, 0, 2, 1+2,                 /*0xFEBA-0xFEBD*/
  1, 0, 2, 1+2,                 /*0xFEBE-0xFEC1*/
  1, 0, 2, 1+2,                 /*0xFEC2-0xFEC5*/
  1, 0, 2, 1+2,                 /*0xFEC6-0xFEC9*/
  1, 0, 2, 1+2,                 /*0xFECA-0xFECD*/
  1, 0, 2, 1+2,                 /*0xFECE-0xFED1*/
  1, 0, 2, 1+2,                 /*0xFED2-0xFED5*/
  1, 0, 2, 1+2,                 /*0xFED6-0xFED9*/
  1, 0, 2, 1+2,                 /*0xFEDA-0xFEDD*/
  1, 0, 2, 1+2,                 /*0xFEDE-0xFEE1*/
  1, 0 + 16, 2 + 16, 1 + 2 +16, /*0xFEE2-0xFEE5*/
  1 + 16, 0, 2, 1+2,            /*0xFEE6-0xFEE9*/
  1, 0, 2, 1+2,                 /*0xFEEA-0xFEED*/
  1, 0, 2, 1+2,                 /*0xFEEE-0xFEF1*/
  1, 0, 1, 0,                   /*0xFEF2-0xFEF5*/
  1, 0, 2, 1+2,                 /*0xFEF6-0xFEF9*/
  1, 0, 1, 0,                   /*0xFEFA-0xFEFD*/
  1, 0, 1, 0,
  1
};

static const UChar convertFEto06[] =
{
/***********0******1******2******3******4******5******6******7******8******9******A******B******C******D******E******F***/
/*FE7*/   0x64B, 0x64B, 0x64C, 0x64C, 0x64D, 0x64D, 0x64E, 0x64E, 0x64F, 0x64F, 0x650, 0x650, 0x651, 0x651, 0x652, 0x652,
/*FE8*/   0x621, 0x622, 0x622, 0x623, 0x623, 0x624, 0x624, 0x625, 0x625, 0x626, 0x626, 0x626, 0x626, 0x627, 0x627, 0x628,
/*FE9*/   0x628, 0x628, 0x628, 0x629, 0x629, 0x62A, 0x62A, 0x62A, 0x62A, 0x62B, 0x62B, 0x62B, 0x62B, 0x62C, 0x62C, 0x62C,
/*FEA*/   0x62C, 0x62D, 0x62D, 0x62D, 0x62D, 0x62E, 0x62E, 0x62E, 0x62E, 0x62F, 0x62F, 0x630, 0x630, 0x631, 0x631, 0x632,
/*FEB*/   0x632, 0x633, 0x633, 0x633, 0x633, 0x634, 0x634, 0x634, 0x634, 0x635, 0x635, 0x635, 0x635, 0x636, 0x636, 0x636,
/*FEC*/   0x636, 0x637, 0x637, 0x637, 0x637, 0x638, 0x638, 0x638, 0x638, 0x639, 0x639, 0x639, 0x639, 0x63A, 0x63A, 0x63A,
/*FED*/   0x63A, 0x641, 0x641, 0x641, 0x641, 0x642, 0x642, 0x642, 0x642, 0x643, 0x643, 0x643, 0x643, 0x644, 0x644, 0x644,
/*FEE*/   0x644, 0x645, 0x645, 0x645, 0x645, 0x646, 0x646, 0x646, 0x646, 0x647, 0x647, 0x647, 0x647, 0x648, 0x648, 0x649,
/*FEF*/   0x649, 0x64A, 0x64A, 0x64A, 0x64A, 0x65C, 0x65C, 0x65D, 0x65D, 0x65E, 0x65E, 0x65F, 0x65F
};

static const UChar shapeTable[4][4][4]=
{
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,1} },
  { {0,0,2,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} },
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,3} },
  { {0,0,1,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} }
};

/*
 *Name     : changeLamAlef
 *Function : Converts the Alef characters into an equivalent
 *           LamAlef location in the 0x06xx Range, this is an
 *           intermediate stage in the operation of the program
 *           later it'll be converted into the 0xFExx LamAlefs
 *           in the shaping function.
 */
static UChar
changeLamAlef(UChar ch) {

    switch(ch) {
    case 0x0622 :
        return(0x065C);
        break;
    case 0x0623 :
        return(0x065D);
        break;
    case 0x0625 :
        return(0x065E);
        break;
    case 0x0627 :
        return(0x065F);
        break;
    default :
        return(0);
        break;
    }
}

/*
 *Name     : specialChar
 *Function : Special Arabic characters need special handling in the shapeUnicode
 *           function, this function returns 1 or 2 for these special characters
 */
static int32_t
specialChar(UChar ch) {

    if( (ch>0x0621 && ch<0x0626)||(ch==0x0627)||(ch>0x062e && ch<0x0633)||
        (ch>0x0647 && ch<0x064a)||(ch==0x0629) ) {
        return (1);
    }
    else
    if( ch>=0x064B && ch<= 0x0652 )
        return (2);
    else
    if( (ch>=0x0653 && ch<= 0x0655) || ch == 0x0670 ||
        (ch>=0xFE70 && ch<= 0xFE7F) )
        return (3);
    else
        return (0);
}

/*
 *Name     : getLink
 *Function : Resolves the link between the characters as
 *           Arabic characters have four forms :
 *           Isolated, Initial, Middle and Final Form
 */
static UChar
getLink(UChar ch) {

    if(ch >= 0x0622 && ch <= 0x06D3) {
        return(araLink[ch-0x0622]);
    } else if(ch == 0x200D) {
        return(3);
    } else if(ch >= 0x206D && ch <= 0x206F) {
        return(4);
    } else if(ch >= 0xFE70 && ch <= 0xFEFC) {
        return(presLink[ch-0xFE70]);
    } else {
        return(0);
    }
}

/*
 *Name     : isTashkeelChar
 *Function : Returns 1 for Tashkeel characters else return 0
 */
static int32_t
isTashkeelChar(UChar ch) {

    if( ch>=0x064B && ch<= 0x0652 )
        return (1);
    else
        return (0);
}

/*
 *Name     : shapeUnicode
 *Function : Converts an Arabic Unicode buffer in 06xx Range into a shaped
 *           arabic Unicode buffer in FExx Range
 */
static int32_t
shapeUnicode(UChar *dest, int32_t sourceLength,
             int32_t destSize,
             int tashkeelFlag) {

    int32_t          i, iend;
    int32_t          prevPos, lastPos,Nx, Nw;
    unsigned int     Shape;
    int32_t          flag;
    int32_t          lamalef_found = 0;
    UChar            prevLink = 0, lastLink = 0, currLink, nextLink = 0;
    UChar            wLamalef;

    /*
     * Converts the input buffer from FExx Range into 06xx Range
     * to make sure that all characters are in the 06xx range
     * even the lamalef is converted to the special region in
     * the 06xx range
     */
    for (i = 0; i < sourceLength; i++) {
        UChar inputChar = dest[i];
        if ( (inputChar >= 0xFE70) && (inputChar <= 0xFEFC)) {
            dest[i] = convertFEto06 [ (inputChar - 0xFE70) ] ;
        }
    }

    /* sets the index to the end of the buffer, together with the step point to -1 */
    i = 0;
    iend = sourceLength;

    /*
     * This function resolves the link between the characters .
     * Arabic characters have four forms :
     * Isolated Form, Initial Form, Middle Form and Final Form
     */
    currLink = getLink(dest[i]);

    prevPos = i;
    lastPos = i;
    Nx = sourceLength + 2, Nw = 0;

    while (i != iend) {
        /* If high byte of currLink > 0 then more than one shape */
        if ((currLink & 0xFF00) > 0 || isTashkeelChar(dest[i])) {
            Nw = i + 1;
            while (Nx >= sourceLength) {         /* we need to know about next char */
                if(Nw == iend) {
                    nextLink = 0;
                    Nx = -1;
                } else {
                    nextLink = getLink(dest[Nw]);
                    if((nextLink & IRRELEVANT) == 0) {
                        Nx = Nw;
                    } else {
                        Nw = Nw + 1;
                    }
                }
            }

            if ( ((currLink & ALEFTYPE) > 0)  &&  ((lastLink & LAMTYPE) > 0) ) {
                lamalef_found = 1;
                wLamalef = changeLamAlef(dest[i]); /*get from 0x065C-0x065f */
                if ( wLamalef != 0) {
                    dest[i] = ' ';               /* The default case is to drop the Alef and replace */
                    dest[lastPos] =wLamalef;     /* it by a space.                                   */
                    i=lastPos;
                }
                lastLink = prevLink;
                currLink = getLink(wLamalef);
            }
            /*
             * get the proper shape according to link ability of neighbors
             * and of character; depends on the order of the shapes
             * (isolated, initial, middle, final) in the compatibility area
             */
             flag  = specialChar(dest[i]);

             Shape = shapeTable[nextLink & (LINKR + LINKL)]
                               [lastLink & (LINKR + LINKL)]
                               [currLink & (LINKR + LINKL)];

             if (flag == 1) {
                 Shape = (Shape == 1 || Shape == 3) ? 1 : 0;
             }
             else
             if(flag == 2) {
                 if( (lastLink & LINKL) && (nextLink & LINKR) && (tashkeelFlag == 1) &&
                      dest[i] != 0x064C && dest[i] != 0x064D ) {
                     Shape = 1;
                     if( (nextLink&ALEFTYPE) == ALEFTYPE && (lastLink&LAMTYPE) == LAMTYPE )
                         Shape = 0;
                 }
                 else {
                     Shape = 0;
                 }
             }

             if(flag == 2) {
                 dest[i] =  0xFE70 + IrrelevantPos[(dest[i] - 0x064B)] + Shape;
             }
             else
                 dest[i] = (UChar)((dest[i] < 0x0670 ? 0xFE70 : 0xFB50) + (currLink >> 8) + Shape);
        }

        /* move one notch forward */
        if ((currLink & IRRELEVANT) == 0) {
              prevLink = lastLink;
              lastLink = currLink;
              prevPos = lastPos;
              lastPos = i;
        }

        i++;
        if (i == Nx) {
            currLink = nextLink;
            Nx = sourceLength + 2;
        }
        else if(i != iend) {
            currLink = getLink(dest[i]);
        }
    }

    destSize = sourceLength;

    return destSize;
}

int32_t shapeArabic(const UChar *source, int32_t sourceLength, UChar *dest, int32_t destCapacity, uint32_t options, UErrorCode *pErrorCode) {
    int32_t destLength;

    /* usual error checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* make sure that no reserved options values are used; allow dest==NULL only for preflighting */
    if( source==NULL || sourceLength<-1 ||
        (dest==NULL && destCapacity!=0) || destCapacity<0 ||
        options>=U_SHAPE_DIGIT_TYPE_RESERVED ||
        (options&U_SHAPE_DIGITS_MASK)>=U_SHAPE_DIGITS_RESERVED
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* determine the source length */
    if(sourceLength==-1) {
        sourceLength=u_strlen(source);
    }
    if(sourceLength==0) {
        return 0;
    }

    /* check that source and destination do not overlap */
    if( dest!=NULL &&
        ((source<=dest && dest<source+sourceLength) ||
         (dest<=source && source<dest+destCapacity))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if((options&U_SHAPE_LETTERS_MASK)!=U_SHAPE_LETTERS_NOOP) {
        int32_t outputSize = sourceLength;

        /* calculate destination size */
        /* TODO: do we ever need to do this pure preflighting? */
        ASSERT((options&U_SHAPE_LENGTH_MASK) != U_SHAPE_LENGTH_GROW_SHRINK);

        if(outputSize>destCapacity) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return outputSize;
        }

        /* Start of Arabic letter shaping part */
        memcpy(dest, source, sourceLength*U_SIZEOF_UCHAR);

        ASSERT((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL);

        switch(options&U_SHAPE_LETTERS_MASK) {
        case U_SHAPE_LETTERS_SHAPE :
            /* Call the shaping function with tashkeel flag == 1 */
            destLength = shapeUnicode(dest,sourceLength,destCapacity,1);
            break;
        case U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED :
            /* Call the shaping function with tashkeel flag == 0 */
            destLength = shapeUnicode(dest,sourceLength,destCapacity,0);
            break;
        case U_SHAPE_LETTERS_UNSHAPE :
            ASSERT_NOT_REACHED();
            break;
        default :
            /* will never occur because of validity checks above */
            destLength = 0;
            break;
        }

        /* End of Arabic letter shaping part */
    } else
        ASSERT_NOT_REACHED();

    ASSERT((options & U_SHAPE_DIGITS_MASK) == U_SHAPE_DIGITS_NOOP); 

    return sourceLength;
}

#endif // USE(ATSUI)
