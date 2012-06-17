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

#include "harfbuzz-stream-private.h"
#include <assert.h>
#include <stdio.h>

#define HB_MIN(a, b) ((a) < (b) ? (a) : (b))
#define HB_MAX(a, b) ((a) > (b) ? (a) : (b))

// -----------------------------------------------------------------------------------------------------
//
// The line break algorithm. See http://www.unicode.org/reports/tr14/tr14-13.html
//
// -----------------------------------------------------------------------------------------------------

/* The Unicode algorithm does in our opinion allow line breaks at some
   places they shouldn't be allowed. The following changes were thus
   made in comparison to the Unicode reference:

   EX->AL from DB to IB
   SY->AL from DB to IB
   SY->PO from DB to IB
   SY->PR from DB to IB
   SY->OP from DB to IB
   AL->PR from DB to IB
   AL->PO from DB to IB
   PR->PR from DB to IB
   PO->PO from DB to IB
   PR->PO from DB to IB
   PO->PR from DB to IB
   HY->PO from DB to IB
   HY->PR from DB to IB
   HY->OP from DB to IB
   NU->EX from PB to IB
   EX->PO from DB to IB
*/

// The following line break classes are not treated by the table:
//  AI, BK, CB, CR, LF, NL, SA, SG, SP, XX

enum break_class {
    // the first 4 values have to agree with the enum in QCharAttributes
    ProhibitedBreak,            // PB in table
    DirectBreak,                // DB in table
    IndirectBreak,              // IB in table
    CombiningIndirectBreak,     // CI in table
    CombiningProhibitedBreak    // CP in table
};
#define DB DirectBreak
#define IB IndirectBreak
#define CI CombiningIndirectBreak
#define CP CombiningProhibitedBreak
#define PB ProhibitedBreak

static const hb_uint8 breakTable[HB_LineBreak_JT+1][HB_LineBreak_JT+1] =
{
/*          OP  CL  QU  GL  NS  EX  SY  IS  PR  PO  NU  AL  ID  IN  HY  BA  BB  B2  ZW  CM  WJ  H2  H3  JL  JV  JT */
/* OP */ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, CP, PB, PB, PB, PB, PB, PB },
/* CL */ { DB, PB, IB, IB, PB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* QU */ { PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* GL */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* NS */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* EX */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* SY */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* IS */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* PR */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, IB },
/* PO */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* NU */ { IB, PB, IB, IB, IB, IB, PB, PB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* AL */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* ID */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* IN */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* HY */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* BA */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* BB */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* B2 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, PB, PB, CI, PB, DB, DB, DB, DB, DB },
/* ZW */ { DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, DB, DB, DB, DB, DB, DB, DB },
/* CM */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* WJ */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* H2 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB },
/* H3 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB },
/* JL */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, DB },
/* JV */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB },
/* JT */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB }
};
#undef DB
#undef IB
#undef CI
#undef CP
#undef PB

static const hb_uint8 graphemeTable[HB_Grapheme_LVT + 1][HB_Grapheme_LVT + 1] =
{
//      Other, CR,    LF,    Control,Extend,L,    V,     T,     LV,    LVT
    { true , true , true , true , true , true , true , true , true , true  }, // Other, 
    { true , true , true , true , true , true , true , true , true , true  }, // CR,
    { true , false, true , true , true , true , true , true , true , true  }, // LF,
    { true , true , true , true , true , true , true , true , true , true  }, // Control,
    { false, true , true , true , false, false, false, false, false, false }, // Extend,
    { true , true , true , true , true , false, true , true , true , true  }, // L, 
    { true , true , true , true , true , false, false, true , false, true  }, // V, 
    { true , true , true , true , true , true , false, false, false, false }, // T, 
    { true , true , true , true , true , false, true , true , true , true  }, // LV, 
    { true , true , true , true , true , false, true , true , true , true  }, // LVT
};
    
static void calcLineBreaks(const HB_UChar16 *uc, hb_uint32 len, HB_CharAttributes *charAttributes)
{
    if (!len)
        return;

    // ##### can this fail if the first char is a surrogate?
    HB_LineBreakClass cls;
    HB_GraphemeClass grapheme;
    HB_GetGraphemeAndLineBreakClass(*uc, &grapheme, &cls);
    // handle case where input starts with an LF
    if (cls == HB_LineBreak_LF)
        cls = HB_LineBreak_BK;

    charAttributes[0].whiteSpace = (cls == HB_LineBreak_SP || cls == HB_LineBreak_BK);
    charAttributes[0].charStop = true;

    int lcls = cls;
    for (hb_uint32 i = 1; i < len; ++i) {
        charAttributes[i].whiteSpace = false;
        charAttributes[i].charStop = true;

        HB_UChar32 code = uc[i];
        HB_GraphemeClass ngrapheme;
        HB_LineBreakClass ncls;
        HB_GetGraphemeAndLineBreakClass(code, &ngrapheme, &ncls);
        charAttributes[i].charStop = graphemeTable[ngrapheme][grapheme];
        // handle surrogates
        if (ncls == HB_LineBreak_SG) {
            if (HB_IsHighSurrogate(uc[i]) && i < len - 1 && HB_IsLowSurrogate(uc[i+1])) {
                continue;
            } else if (HB_IsLowSurrogate(uc[i]) && HB_IsHighSurrogate(uc[i-1])) {
                code = HB_SurrogateToUcs4(uc[i-1], uc[i]);
                HB_GetGraphemeAndLineBreakClass(code, &ngrapheme, &ncls);
                charAttributes[i].charStop = false;
            } else {
                ncls = HB_LineBreak_AL;
            }
        }

        // set white space and char stop flag
        if (ncls >= HB_LineBreak_SP)
            charAttributes[i].whiteSpace = true;

        HB_LineBreakType lineBreakType = HB_NoBreak;
        if (cls >= HB_LineBreak_LF) {
            lineBreakType = HB_ForcedBreak;
        } else if(cls == HB_LineBreak_CR) {
            lineBreakType = (ncls == HB_LineBreak_LF) ? HB_NoBreak : HB_ForcedBreak;
        }

        if (ncls == HB_LineBreak_SP)
            goto next_no_cls_update;
        if (ncls >= HB_LineBreak_CR)
            goto next;

        {
            int tcls = ncls;
            // for south east asian chars that require a complex (dictionary analysis), the unicode
            // standard recommends to treat them as AL. thai_attributes and other attribute methods that
            // do dictionary analysis can override
            if (tcls >= HB_LineBreak_SA)
                tcls = HB_LineBreak_AL;
            if (cls >= HB_LineBreak_SA)
                cls = HB_LineBreak_AL;

            int brk = breakTable[cls][tcls];
            switch (brk) {
            case DirectBreak:
                lineBreakType = HB_Break;
                if (uc[i-1] == 0xad) // soft hyphen
                    lineBreakType = HB_SoftHyphen;
                break;
            case IndirectBreak:
                lineBreakType = (lcls == HB_LineBreak_SP) ? HB_Break : HB_NoBreak;
                break;
            case CombiningIndirectBreak:
                lineBreakType = HB_NoBreak;
                if (lcls == HB_LineBreak_SP){
                    if (i > 1)
                        charAttributes[i-2].lineBreakType = HB_Break;
                } else {
                    goto next_no_cls_update;
                }
                break;
            case CombiningProhibitedBreak:
                lineBreakType = HB_NoBreak;
                if (lcls != HB_LineBreak_SP)
                    goto next_no_cls_update;
            case ProhibitedBreak:
            default:
                break;
            }
        }
    next:
        cls = ncls;
    next_no_cls_update:
        lcls = ncls;
        grapheme = ngrapheme;
        charAttributes[i-1].lineBreakType = lineBreakType;
    }
    charAttributes[len-1].lineBreakType = HB_ForcedBreak;
}

// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Basic processing
//
// --------------------------------------------------------------------------------------------------------------------------------------------

static inline void positionCluster(HB_ShaperItem *item, int gfrom,  int glast)
{
    int nmarks = glast - gfrom;
    assert(nmarks > 0);

    HB_Glyph *glyphs = item->glyphs;
    HB_GlyphAttributes *attributes = item->attributes;

    HB_GlyphMetrics baseMetrics;
    item->font->klass->getGlyphMetrics(item->font, glyphs[gfrom], &baseMetrics);

    if (item->item.script == HB_Script_Hebrew
        && (-baseMetrics.y) > baseMetrics.height)
        // we need to attach below the baseline, because of the hebrew iud.
        baseMetrics.height = -baseMetrics.y;

//     qDebug("---> positionCluster: cluster from %d to %d", gfrom, glast);
//     qDebug("baseInfo: %f/%f (%f/%f) off=%f/%f", baseInfo.x, baseInfo.y, baseInfo.width, baseInfo.height, baseInfo.xoff, baseInfo.yoff);

    HB_Fixed size = item->font->klass->getFontMetric(item->font, HB_FontAscent) / 10;
    HB_Fixed offsetBase = HB_FIXED_CONSTANT(1) + (size - HB_FIXED_CONSTANT(4)) / 4;
    if (size > HB_FIXED_CONSTANT(4))
        offsetBase += HB_FIXED_CONSTANT(4);
    else
        offsetBase += size;
    //qreal offsetBase = (size - 4) / 4 + qMin<qreal>(size, 4) + 1;
//     qDebug("offset = %f", offsetBase);

    // To fix some Thai character heights check for two above glyphs
    if (nmarks == 2 && (attributes[gfrom+1].combiningClass == HB_Combining_AboveRight ||
            attributes[gfrom+1].combiningClass  == HB_Combining_AboveLeft ||
            attributes[gfrom+1].combiningClass == HB_Combining_Above))
        if (attributes[gfrom+2].combiningClass == 23 ||
            attributes[gfrom+2].combiningClass == 24 ||
            attributes[gfrom+2].combiningClass == 25 ||
            attributes[gfrom+2].combiningClass == 27 ||
            attributes[gfrom+2].combiningClass == 28 ||
            attributes[gfrom+2].combiningClass == 30 ||
            attributes[gfrom+2].combiningClass == 31 ||
            attributes[gfrom+2].combiningClass == 33 ||
            attributes[gfrom+2].combiningClass == 34 ||
            attributes[gfrom+2].combiningClass == 35 ||
            attributes[gfrom+2].combiningClass == 36 ||
            attributes[gfrom+2].combiningClass == 107 ||
            attributes[gfrom+2].combiningClass == 122) {
            // Two above glyphs, check total height
            int markTotalHeight = baseMetrics.height;
            HB_GlyphMetrics markMetrics;
            item->font->klass->getGlyphMetrics(item->font, glyphs[gfrom+1], &markMetrics);
            markTotalHeight += markMetrics.height;
            item->font->klass->getGlyphMetrics(item->font, glyphs[gfrom+2], &markMetrics);
            markTotalHeight += markMetrics.height;
            if ((markTotalHeight + 2 * offsetBase) > (size * 10))
                offsetBase = ((size * 10) - markTotalHeight) / 2; // Use offset that just fits
        }

    bool rightToLeft = item->item.bidiLevel % 2;

    int i;
    unsigned char lastCmb = 0;
    HB_GlyphMetrics attachmentRect;
    memset(&attachmentRect, 0, sizeof(attachmentRect));

    for(i = 1; i <= nmarks; i++) {
        HB_Glyph mark = glyphs[gfrom+i];
        HB_GlyphMetrics markMetrics;
        item->font->klass->getGlyphMetrics(item->font, mark, &markMetrics);
        HB_FixedPoint p;
        p.x = p.y = 0;
//          qDebug("markInfo: %f/%f (%f/%f) off=%f/%f", markInfo.x, markInfo.y, markInfo.width, markInfo.height, markInfo.xoff, markInfo.yoff);

        HB_Fixed offset = offsetBase;
        unsigned char cmb = attributes[gfrom+i].combiningClass;

        // ### maybe the whole position determination should move down to heuristicSetGlyphAttributes. Would save some
        // bits  in the glyphAttributes structure.
        if (cmb < 200) {
            // fixed position classes. We approximate by mapping to one of the others.
            // currently I added only the ones for arabic, hebrew, lao and thai.

            // for Lao and Thai marks with class 0, see below (heuristicSetGlyphAttributes)

            // add a bit more offset to arabic, a bit hacky
            if (cmb >= 27 && cmb <= 36 && offset < 3)
                offset +=1;
            // below
            if ((cmb >= 10 && cmb <= 18) ||
                 cmb == 20 || cmb == 22 ||
                 cmb == 29 || cmb == 32)
                cmb = HB_Combining_Below;
            // above
            else if (cmb == 23 || cmb == 27 || cmb == 28 ||
                      cmb == 30 || cmb == 31 || (cmb >= 33 && cmb <= 36))
                cmb = HB_Combining_Above;
            //below-right
            else if (cmb == 9 || cmb == 103 || cmb == 118)
                cmb = HB_Combining_BelowRight;
            // above-right
            else if (cmb == 24 || cmb == 107 || cmb == 122)
                cmb = HB_Combining_AboveRight;
            else if (cmb == 25)
                cmb = HB_Combining_AboveLeft;
            // fixed:
            //  19 21

        }

        // Check drawing below fonts descent
        if (cmb == HB_Combining_Below || cmb == HB_Combining_BelowRight)
            if ((markMetrics.height + offset) > item->font->klass->getFontMetric(item->font, HB_FontDescent))
                offset = markMetrics.y; // Use offset from mark metrics so it won't get drawn below descent

        // combining marks of different class don't interact. Reset the rectangle.
        if (cmb != lastCmb) {
            //qDebug("resetting rect");
            attachmentRect = baseMetrics;
        }

        switch(cmb) {
        case HB_Combining_DoubleBelow:
                // ### wrong in rtl context!
        case HB_Combining_BelowLeft:
            p.y += offset;
        case HB_Combining_BelowLeftAttached:
            p.x += attachmentRect.x - markMetrics.x;
            p.y += (attachmentRect.y + attachmentRect.height) - markMetrics.y;
            break;
        case HB_Combining_Below:
            p.y += offset;
        case HB_Combining_BelowAttached:
            p.x += attachmentRect.x - markMetrics.x;
            p.y += (attachmentRect.y + attachmentRect.height) - markMetrics.y;

            p.x += (attachmentRect.width - markMetrics.width) / 2;
            break;
        case HB_Combining_BelowRight:
            p.y += offset;
        case HB_Combining_BelowRightAttached:
            p.x += attachmentRect.x + attachmentRect.width - markMetrics.width - markMetrics.x;
            p.y += attachmentRect.y + attachmentRect.height - markMetrics.y;
            break;
        case HB_Combining_Left:
            p.x -= offset;
        case HB_Combining_LeftAttached:
            break;
        case HB_Combining_Right:
            p.x += offset;
        case HB_Combining_RightAttached:
            break;
        case HB_Combining_DoubleAbove:
            // ### wrong in RTL context!
        case HB_Combining_AboveLeft:
            p.y -= offset;
        case HB_Combining_AboveLeftAttached:
            p.x += attachmentRect.x - markMetrics.x;
            p.y += attachmentRect.y - markMetrics.y - markMetrics.height;
            break;
        case HB_Combining_Above:
            p.y -= offset;
        case HB_Combining_AboveAttached:
            p.x += attachmentRect.x - markMetrics.x;
            p.y += attachmentRect.y - markMetrics.y - markMetrics.height;

            p.x += (attachmentRect.width - markMetrics.width) / 2;
            break;
        case HB_Combining_AboveRight:
            p.y -= offset;
        case HB_Combining_AboveRightAttached:
            p.x += attachmentRect.x + attachmentRect.width - markMetrics.x - markMetrics.width;
            p.y += attachmentRect.y - markMetrics.y - markMetrics.height;
            break;

        case HB_Combining_IotaSubscript:
            default:
                break;
        }
//          qDebug("char=%x combiningClass = %d offset=%f/%f", mark, cmb, p.x(), p.y());
        markMetrics.x += p.x;
        markMetrics.y += p.y;

        HB_GlyphMetrics unitedAttachmentRect = attachmentRect;
        unitedAttachmentRect.x = HB_MIN(attachmentRect.x, markMetrics.x);
        unitedAttachmentRect.y = HB_MIN(attachmentRect.y, markMetrics.y);
        unitedAttachmentRect.width = HB_MAX(attachmentRect.x + attachmentRect.width, markMetrics.x + markMetrics.width) - unitedAttachmentRect.x;
        unitedAttachmentRect.height = HB_MAX(attachmentRect.y + attachmentRect.height, markMetrics.y + markMetrics.height) - unitedAttachmentRect.y;
        attachmentRect = unitedAttachmentRect;

        lastCmb = cmb;
        if (rightToLeft) {
            item->offsets[gfrom+i].x = p.x;
            item->offsets[gfrom+i].y = p.y;
        } else {
            item->offsets[gfrom+i].x = p.x - baseMetrics.xOffset;
            item->offsets[gfrom+i].y = p.y - baseMetrics.yOffset;
        }
        item->advances[gfrom+i] = 0;
    }
}

void HB_HeuristicPosition(HB_ShaperItem *item)
{
    HB_GetGlyphAdvances(item);
    HB_GlyphAttributes *attributes = item->attributes;

    int cEnd = -1;
    int i = item->num_glyphs;
    while (i--) {
        if (cEnd == -1 && attributes[i].mark) {
            cEnd = i;
        } else if (cEnd != -1 && !attributes[i].mark) {
            positionCluster(item, i, cEnd);
            cEnd = -1;
        }
    }
}

// set the glyph attributes heuristically. Assumes a 1 to 1 relationship between chars and glyphs
// and no reordering.
// also computes logClusters heuristically
void HB_HeuristicSetGlyphAttributes(HB_ShaperItem *item)
{
    const HB_UChar16 *uc = item->string + item->item.pos;
    hb_uint32 length = item->item.length;

    // ### zeroWidth and justification are missing here!!!!!

    assert(item->num_glyphs <= length);

//     qDebug("QScriptEngine::heuristicSetGlyphAttributes, num_glyphs=%d", item->num_glyphs);
    HB_GlyphAttributes *attributes = item->attributes;
    unsigned short *logClusters = item->log_clusters;

    hb_uint32 glyph_pos = 0;
    hb_uint32 i;
    for (i = 0; i < length; i++) {
        if (HB_IsHighSurrogate(uc[i]) && i < length - 1
            && HB_IsLowSurrogate(uc[i + 1])) {
            logClusters[i] = glyph_pos;
            logClusters[++i] = glyph_pos;
        } else {
            logClusters[i] = glyph_pos;
        }
        ++glyph_pos;
    }
    assert(glyph_pos == item->num_glyphs);

    // first char in a run is never (treated as) a mark
    int cStart = 0;
    const bool symbolFont = item->face->isSymbolFont;
    attributes[0].mark = false;
    attributes[0].clusterStart = true;
    attributes[0].dontPrint = (!symbolFont && uc[0] == 0x00ad) || HB_IsControlChar(uc[0]);

    int pos = 0;
    HB_CharCategory lastCat;
    int dummy;
    HB_GetUnicodeCharProperties(uc[0], &lastCat, &dummy);
    for (i = 1; i < length; ++i) {
        if (logClusters[i] == pos)
            // same glyph
            continue;
        ++pos;
        while (pos < logClusters[i]) {
            attributes[pos] = attributes[pos-1];
            ++pos;
        }
        // hide soft-hyphens by default
        if ((!symbolFont && uc[i] == 0x00ad) || HB_IsControlChar(uc[i]))
            attributes[pos].dontPrint = true;
        HB_CharCategory cat;
        int cmb;
        HB_GetUnicodeCharProperties(uc[i], &cat, &cmb);
        if (cat != HB_Mark_NonSpacing) {
            attributes[pos].mark = false;
            attributes[pos].clusterStart = true;
            attributes[pos].combiningClass = 0;
            cStart = logClusters[i];
        } else {
            if (cmb == 0) {
                // Fix 0 combining classes
                if ((uc[pos] & 0xff00) == 0x0e00) {
                    // thai or lao
                    if (uc[pos] == 0xe31 ||
                         uc[pos] == 0xe34 ||
                         uc[pos] == 0xe35 ||
                         uc[pos] == 0xe36 ||
                         uc[pos] == 0xe37 ||
                         uc[pos] == 0xe47 ||
                         uc[pos] == 0xe4c ||
                         uc[pos] == 0xe4d ||
                         uc[pos] == 0xe4e) {
                        cmb = HB_Combining_AboveRight;
                    } else if (uc[pos] == 0xeb1 ||
                                uc[pos] == 0xeb4 ||
                                uc[pos] == 0xeb5 ||
                                uc[pos] == 0xeb6 ||
                                uc[pos] == 0xeb7 ||
                                uc[pos] == 0xebb ||
                                uc[pos] == 0xecc ||
                                uc[pos] == 0xecd) {
                        cmb = HB_Combining_Above;
                    } else if (uc[pos] == 0xebc) {
                        cmb = HB_Combining_Below;
                    }
                }
            }

            attributes[pos].mark = true;
            attributes[pos].clusterStart = false;
            attributes[pos].combiningClass = cmb;
            logClusters[i] = cStart;
        }
        // one gets an inter character justification point if the current char is not a non spacing mark.
        // as then the current char belongs to the last one and one gets a space justification point
        // after the space char.
        if (lastCat == HB_Separator_Space)
            attributes[pos-1].justification = HB_Space;
        else if (cat != HB_Mark_NonSpacing)
            attributes[pos-1].justification = HB_Character;
        else
            attributes[pos-1].justification = HB_NoJustification;

        lastCat = cat;
    }
    pos = logClusters[length-1];
    if (lastCat == HB_Separator_Space)
        attributes[pos].justification = HB_Space;
    else
        attributes[pos].justification = HB_Character;
}

#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature basic_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('l', 'i', 'g', 'a'), LigaProperty },
    { HB_MAKE_TAG('c', 'l', 'i', 'g'), CligProperty },
    {0, 0}
};

static const HB_OpenTypeFeature disabled_features[] = {
    { HB_MAKE_TAG('c', 'p', 'c', 't'), PositioningProperties },
    { HB_MAKE_TAG('h', 'a', 'l', 't'), PositioningProperties },
    // TODO: we need to add certain HB_ShaperFlag for vertical
    // writing mode to enable these vertical writing features:
    { HB_MAKE_TAG('v', 'a', 'l', 't'), PositioningProperties },
    { HB_MAKE_TAG('v', 'h', 'a', 'l'), PositioningProperties },
    { HB_MAKE_TAG('v', 'k', 'r', 'n'), PositioningProperties },
    { HB_MAKE_TAG('v', 'p', 'a', 'l'), PositioningProperties },
    {0, 0}
};
#endif

HB_Bool HB_ConvertStringToGlyphIndices(HB_ShaperItem *shaper_item)
{
    if (shaper_item->glyphIndicesPresent) {
        shaper_item->num_glyphs = shaper_item->initialGlyphCount;
        shaper_item->glyphIndicesPresent = false;
        return true;
    }
    return shaper_item->font->klass
           ->convertStringToGlyphIndices(shaper_item->font,
                                         shaper_item->string + shaper_item->item.pos, shaper_item->item.length,
                                         shaper_item->glyphs, &shaper_item->num_glyphs,
                                         shaper_item->item.bidiLevel % 2);
}

HB_Bool HB_BasicShape(HB_ShaperItem *shaper_item)
{
#ifndef NO_OPENTYPE
    const int availableGlyphs = shaper_item->num_glyphs;
#endif

    if (!HB_ConvertStringToGlyphIndices(shaper_item))
        return false;

    HB_HeuristicSetGlyphAttributes(shaper_item);

#ifndef NO_OPENTYPE
    if (HB_SelectScript(shaper_item, basic_features)) {
        HB_OpenTypeShape(shaper_item, /*properties*/0);
        return HB_OpenTypePosition(shaper_item, availableGlyphs, /*doLogClusters*/true);
    }
#endif

    HB_HeuristicPosition(shaper_item);
    return true;
}

const HB_ScriptEngine HB_ScriptEngines[] = {
    // Common
    { HB_BasicShape, 0},
    // Greek
    { HB_GreekShape, 0},
    // Cyrillic
    { HB_BasicShape, 0},
    // Armenian
    { HB_BasicShape, 0},
    // Hebrew
    { HB_HebrewShape, 0 },
    // Arabic
    { HB_ArabicShape, 0},
    // Syriac
    { HB_ArabicShape, 0},
    // Thaana
    { HB_BasicShape, 0 },
    // Devanagari
    { HB_IndicShape, HB_IndicAttributes },
    // Bengali
    { HB_IndicShape, HB_IndicAttributes },
    // Gurmukhi
    { HB_IndicShape, HB_IndicAttributes },
    // Gujarati
    { HB_IndicShape, HB_IndicAttributes },
    // Oriya
    { HB_IndicShape, HB_IndicAttributes },
    // Tamil
    { HB_IndicShape, HB_IndicAttributes },
    // Telugu
    { HB_IndicShape, HB_IndicAttributes },
    // Kannada
    { HB_IndicShape, HB_IndicAttributes },
    // Malayalam
    { HB_IndicShape, HB_IndicAttributes },
    // Sinhala
    { HB_IndicShape, HB_IndicAttributes },
    // Thai
    { HB_ThaiShape, HB_ThaiAttributes },
    // Lao
    { HB_BasicShape, 0 },
    // Tibetan
    { HB_TibetanShape, HB_TibetanAttributes },
    // Myanmar
    { HB_MyanmarShape, HB_MyanmarAttributes },
    // Georgian
    { HB_BasicShape, 0 },
    // Hangul
    { HB_HangulShape, 0 },
    // Ogham
    { HB_BasicShape, 0 },
    // Runic
    { HB_BasicShape, 0 },
    // Khmer
    { HB_KhmerShape, HB_KhmerAttributes },
    // N'Ko
    { HB_ArabicShape, 0}
};

void HB_GetCharAttributes(const HB_UChar16 *string, hb_uint32 stringLength,
                          const HB_ScriptItem *items, hb_uint32 numItems,
                          HB_CharAttributes *attributes)
{
    memset(attributes, 0, stringLength * sizeof(HB_CharAttributes));
    calcLineBreaks(string, stringLength, attributes);

    for (hb_uint32 i = 0; i < numItems; ++i) {
        HB_Script script = items[i].script;
        if (script == HB_Script_Inherited)
            script = HB_Script_Common;
        HB_AttributeFunction attributeFunction = HB_ScriptEngines[script].charAttributes;
        if (!attributeFunction)
            continue;
        attributeFunction(script, string, items[i].pos, items[i].length, attributes);
    }
}


enum BreakRule { NoBreak = 0, Break = 1, Middle = 2 };

static const hb_uint8 wordbreakTable[HB_Word_ExtendNumLet + 1][HB_Word_ExtendNumLet + 1] = {
//        Other    Format   Katakana ALetter  MidLetter MidNum  Numeric  ExtendNumLet
    {   Break,   Break,   Break,   Break,   Break,   Break,   Break,   Break }, // Other
    {   Break,   Break,   Break,   Break,   Break,   Break,   Break,   Break }, // Format 
    {   Break,   Break, NoBreak,   Break,   Break,   Break,   Break, NoBreak }, // Katakana
    {   Break,   Break,   Break, NoBreak,  Middle,   Break, NoBreak, NoBreak }, // ALetter
    {   Break,   Break,   Break,   Break,   Break,   Break,   Break,   Break }, // MidLetter
    {   Break,   Break,   Break,   Break,   Break,   Break,   Break,   Break }, // MidNum
    {   Break,   Break,   Break, NoBreak,   Break,  Middle, NoBreak, NoBreak }, // Numeric
    {   Break,   Break, NoBreak, NoBreak,   Break,   Break, NoBreak, NoBreak }, // ExtendNumLet
};

void HB_GetWordBoundaries(const HB_UChar16 *string, hb_uint32 stringLength,
                          const HB_ScriptItem * /*items*/, hb_uint32 /*numItems*/,
                          HB_CharAttributes *attributes)
{
    if (stringLength == 0)
        return;
    unsigned int brk = HB_GetWordClass(string[0]);
    attributes[0].wordBoundary = true;
    for (hb_uint32 i = 1; i < stringLength; ++i) {
        if (!attributes[i].charStop) {
            attributes[i].wordBoundary = false;
            continue;
        }
        hb_uint32 nbrk = HB_GetWordClass(string[i]);
        if (nbrk == HB_Word_Format) {
            attributes[i].wordBoundary = (HB_GetSentenceClass(string[i-1]) == HB_Sentence_Sep);
            continue;
        }
        BreakRule rule = (BreakRule)wordbreakTable[brk][nbrk];
        if (rule == Middle) {
            rule = Break;
            hb_uint32 lookahead = i + 1;
            while (lookahead < stringLength) {
                hb_uint32 testbrk = HB_GetWordClass(string[lookahead]);
                if (testbrk == HB_Word_Format && HB_GetSentenceClass(string[lookahead]) != HB_Sentence_Sep) {
                    ++lookahead;
                    continue;
                }
                if (testbrk == brk) {
                    rule = NoBreak;
                    while (i < lookahead)
                        attributes[i++].wordBoundary = false;
                    nbrk = testbrk;
                }
                break;
            }
        }
        attributes[i].wordBoundary = (rule == Break);
        brk = nbrk;
    }
}


enum SentenceBreakStates {
    SB_Initial,
    SB_Upper,
    SB_UpATerm, 
    SB_ATerm,
    SB_ATermC, 
    SB_ACS, 
    SB_STerm, 
    SB_STermC, 
    SB_SCS,
    SB_BAfter, 
    SB_Break,
    SB_Look
};

static const hb_uint8 sentenceBreakTable[HB_Sentence_Close + 1][HB_Sentence_Close + 1] = {
//        Other       Sep         Format      Sp          Lower       Upper       OLetter     Numeric     ATerm       STerm       Close
      { SB_Initial, SB_BAfter , SB_Initial, SB_Initial, SB_Initial, SB_Upper  , SB_Initial, SB_Initial, SB_ATerm  , SB_STerm  , SB_Initial }, // SB_Initial,
      { SB_Initial, SB_BAfter , SB_Upper  , SB_Initial, SB_Initial, SB_Upper  , SB_Initial, SB_Initial, SB_UpATerm, SB_STerm  , SB_Initial }, // SB_Upper
      
      { SB_Look   , SB_BAfter , SB_UpATerm, SB_ACS    , SB_Initial, SB_Upper  , SB_Break  , SB_Initial, SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_UpATerm
      { SB_Look   , SB_BAfter , SB_ATerm  , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Initial, SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_ATerm
      { SB_Look   , SB_BAfter , SB_ATermC , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Look   , SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_ATermC,
      { SB_Look   , SB_BAfter , SB_ACS    , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Look   , SB_ATerm  , SB_STerm  , SB_Look    }, // SB_ACS,
      
      { SB_Break  , SB_BAfter , SB_STerm  , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_STermC  }, // SB_STerm,
      { SB_Break  , SB_BAfter , SB_STermC , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_STermC  }, // SB_STermC,
      { SB_Break  , SB_BAfter , SB_SCS    , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_Break   }, // SB_SCS,
      { SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break   }, // SB_BAfter,
};

void HB_GetSentenceBoundaries(const HB_UChar16 *string, hb_uint32 stringLength,
                              const HB_ScriptItem * /*items*/, hb_uint32 /*numItems*/,
                              HB_CharAttributes *attributes)
{
    if (stringLength == 0)
        return;
    hb_uint32 brk = sentenceBreakTable[SB_Initial][HB_GetSentenceClass(string[0])];
    attributes[0].sentenceBoundary = true;
    for (hb_uint32 i = 1; i < stringLength; ++i) {
        if (!attributes[i].charStop) {
            attributes[i].sentenceBoundary = false;
            continue;
        }
        brk = sentenceBreakTable[brk][HB_GetSentenceClass(string[i])];
        if (brk == SB_Look) {
            brk = SB_Break;
            hb_uint32 lookahead = i + 1;
            while (lookahead < stringLength) {
                hb_uint32 sbrk = HB_GetSentenceClass(string[lookahead]);
                if (sbrk != HB_Sentence_Other && sbrk != HB_Sentence_Numeric && sbrk != HB_Sentence_Close) {
                    break;
                } else if (sbrk == HB_Sentence_Lower) {
                    brk = SB_Initial;
                    break;
                }
                ++lookahead;
            }
            if (brk == SB_Initial) {
                while (i < lookahead)
                    attributes[i++].sentenceBoundary = false;
            }
        }
        if (brk == SB_Break) {
            attributes[i].sentenceBoundary = true;
            brk = sentenceBreakTable[SB_Initial][HB_GetSentenceClass(string[i])];
        } else {
            attributes[i].sentenceBoundary = false;
        }
    }
}


static inline char *tag_to_string(HB_UInt tag)
{
    static char string[5];
    string[0] = (tag >> 24)&0xff;
    string[1] = (tag >> 16)&0xff;
    string[2] = (tag >> 8)&0xff;
    string[3] = tag&0xff;
    string[4] = 0;
    return string;
}

#ifdef OT_DEBUG
static void dump_string(HB_Buffer buffer)
{
    for (uint i = 0; i < buffer->in_length; ++i) {
        qDebug("    %x: cluster=%d", buffer->in_string[i].gindex, buffer->in_string[i].cluster);
    }
}
#define DEBUG printf
#else
#define DEBUG if (1) ; else printf
#endif

#define DefaultLangSys 0xffff
#define DefaultScript HB_MAKE_TAG('D', 'F', 'L', 'T')

enum {
    RequiresGsub = 1,
    RequiresGpos = 2
};

struct OTScripts {
    unsigned int tag;
    int flags;
};
static const OTScripts ot_scripts [] = {
    // Common
    { HB_MAKE_TAG('l', 'a', 't', 'n'), 0 },
    // Greek
    { HB_MAKE_TAG('g', 'r', 'e', 'k'), 0 },
    // Cyrillic
    { HB_MAKE_TAG('c', 'y', 'r', 'l'), 0 },
    // Armenian
    { HB_MAKE_TAG('a', 'r', 'm', 'n'), 0 },
    // Hebrew
    { HB_MAKE_TAG('h', 'e', 'b', 'r'), 1 },
    // Arabic
    { HB_MAKE_TAG('a', 'r', 'a', 'b'), 1 },
    // Syriac
    { HB_MAKE_TAG('s', 'y', 'r', 'c'), 1 },
    // Thaana
    { HB_MAKE_TAG('t', 'h', 'a', 'a'), 1 },
    // Devanagari
    { HB_MAKE_TAG('d', 'e', 'v', 'a'), 1 },
    // Bengali
    { HB_MAKE_TAG('b', 'e', 'n', 'g'), 1 },
    // Gurmukhi
    { HB_MAKE_TAG('g', 'u', 'r', 'u'), 1 },
    // Gujarati
    { HB_MAKE_TAG('g', 'u', 'j', 'r'), 1 },
    // Oriya
    { HB_MAKE_TAG('o', 'r', 'y', 'a'), 1 },
    // Tamil
    { HB_MAKE_TAG('t', 'a', 'm', 'l'), 1 },
    // Telugu
    { HB_MAKE_TAG('t', 'e', 'l', 'u'), 1 },
    // Kannada
    { HB_MAKE_TAG('k', 'n', 'd', 'a'), 1 },
    // Malayalam
    { HB_MAKE_TAG('m', 'l', 'y', 'm'), 1 },
    // Sinhala
    { HB_MAKE_TAG('s', 'i', 'n', 'h'), 1 },
    // Thai
    { HB_MAKE_TAG('t', 'h', 'a', 'i'), 1 },
    // Lao
    { HB_MAKE_TAG('l', 'a', 'o', ' '), 1 },
    // Tibetan
    { HB_MAKE_TAG('t', 'i', 'b', 't'), 1 },
    // Myanmar
    { HB_MAKE_TAG('m', 'y', 'm', 'r'), 1 },
    // Georgian
    { HB_MAKE_TAG('g', 'e', 'o', 'r'), 0 },
    // Hangul
    { HB_MAKE_TAG('h', 'a', 'n', 'g'), 1 },
    // Ogham
    { HB_MAKE_TAG('o', 'g', 'a', 'm'), 0 },
    // Runic
    { HB_MAKE_TAG('r', 'u', 'n', 'r'), 0 },
    // Khmer
    { HB_MAKE_TAG('k', 'h', 'm', 'r'), 1 },
    // N'Ko
    { HB_MAKE_TAG('n', 'k', 'o', ' '), 1 }
};
enum { NumOTScripts = sizeof(ot_scripts)/sizeof(OTScripts) };

static HB_Bool checkScript(HB_Face face, int script)
{
    assert(script < HB_ScriptCount);

    if (!face->gsub && !face->gpos)
        return false;

    unsigned int tag = ot_scripts[script].tag;
    int requirements = ot_scripts[script].flags;

    if (requirements & RequiresGsub) {
        if (!face->gsub)
            return false;

        HB_UShort script_index;
        HB_Error error = HB_GSUB_Select_Script(face->gsub, tag, &script_index);
        if (error) {
            DEBUG("could not select script %d in GSub table: %d", (int)script, error);
            error = HB_GSUB_Select_Script(face->gsub, HB_MAKE_TAG('D', 'F', 'L', 'T'), &script_index);
            if (error)
                return false;
        }
    }

    if (requirements & RequiresGpos) {
        if (!face->gpos)
            return false;

        HB_UShort script_index;
        HB_Error error = HB_GPOS_Select_Script(face->gpos, script, &script_index);
        if (error) {
            DEBUG("could not select script in gpos table: %d", error);
            error = HB_GPOS_Select_Script(face->gpos, HB_MAKE_TAG('D', 'F', 'L', 'T'), &script_index);
            if (error)
                return false;
        }

    }
    return true;
}

static HB_Stream getTableStream(void *font, HB_GetFontTableFunc tableFunc, HB_Tag tag)
{
    HB_Error error;
    HB_UInt length = 0;
    HB_Stream stream = 0;

    if (!font)
        return 0;

    error = tableFunc(font, tag, 0, &length);
    if (error)
        return 0;
    stream = (HB_Stream)malloc(sizeof(HB_StreamRec));
    if (!stream)
        return 0;
    stream->base = (HB_Byte*)malloc(length);
    if (!stream->base) {
        free(stream);
        return 0;
    }
    error = tableFunc(font, tag, stream->base, &length);
    if (error) {
        _hb_close_stream(stream);
        return 0;
    }
    stream->size = length;
    stream->pos = 0;
    stream->cursor = NULL;
    return stream;
}

HB_Face HB_NewFace(void *font, HB_GetFontTableFunc tableFunc)
{
    HB_Face face = (HB_Face )malloc(sizeof(HB_FaceRec));
    if (!face)
        return 0;

    face->isSymbolFont = false;
    face->gdef = 0;
    face->gpos = 0;
    face->gsub = 0;
    face->current_script = HB_ScriptCount;
    face->current_flags = HB_ShaperFlag_Default;
    face->has_opentype_kerning = false;
    face->tmpAttributes = 0;
    face->tmpLogClusters = 0;
    face->glyphs_substituted = false;
    face->buffer = 0;

    HB_Error error = HB_Err_Ok;
    HB_Stream stream;
    HB_Stream gdefStream;

    gdefStream = getTableStream(font, tableFunc, TTAG_GDEF);
    error = HB_Err_Not_Covered;
    if (!gdefStream || (error = HB_Load_GDEF_Table(gdefStream, &face->gdef))) {
        //DEBUG("error loading gdef table: %d", error);
        face->gdef = 0;
    }

    //DEBUG() << "trying to load gsub table";
    stream = getTableStream(font, tableFunc, TTAG_GSUB);
    error = HB_Err_Not_Covered;
    if (!stream || (error = HB_Load_GSUB_Table(stream, &face->gsub, face->gdef, gdefStream))) {
        face->gsub = 0;
        if (error != HB_Err_Not_Covered) {
            //DEBUG("error loading gsub table: %d", error);
        } else {
            //DEBUG("face doesn't have a gsub table");
        }
    }
    _hb_close_stream(stream);

    stream = getTableStream(font, tableFunc, TTAG_GPOS);
    error = HB_Err_Not_Covered;
    if (!stream || (error = HB_Load_GPOS_Table(stream, &face->gpos, face->gdef, gdefStream))) {
        face->gpos = 0;
        DEBUG("error loading gpos table: %d", error);
    }
    _hb_close_stream(stream);

    _hb_close_stream(gdefStream);

    for (unsigned int i = 0; i < HB_ScriptCount; ++i)
        face->supported_scripts[i] = checkScript(face, i);

    if (hb_buffer_new(&face->buffer) != HB_Err_Ok) {
        HB_FreeFace(face);
        return 0;
    }

    return face;
}

void HB_FreeFace(HB_Face face)
{
    if (!face)
        return;
    if (face->gpos)
        HB_Done_GPOS_Table(face->gpos);
    if (face->gsub)
        HB_Done_GSUB_Table(face->gsub);
    if (face->gdef)
        HB_Done_GDEF_Table(face->gdef);
    if (face->buffer)
        hb_buffer_free(face->buffer);
    if (face->tmpAttributes)
        free(face->tmpAttributes);
    if (face->tmpLogClusters)
        free(face->tmpLogClusters);
    free(face);
}

HB_Bool HB_SelectScript(HB_ShaperItem *shaper_item, const HB_OpenTypeFeature *features)
{
    HB_Script script = shaper_item->item.script;

    HB_Face face = shaper_item->face;
    if (face->current_script == script && face->current_flags == shaper_item->shaperFlags)
        return shaper_item->face->supported_scripts[script] ? true : false;

    face->current_script = script;
    face->current_flags = shaper_item->shaperFlags;

    if (!shaper_item->face->supported_scripts[script])
        return false;

    assert(script < HB_ScriptCount);
    // find script in our list of supported scripts.
    unsigned int tag = ot_scripts[script].tag;

    if (face->gsub && features) {
#ifdef OT_DEBUG
        {
            HB_FeatureList featurelist = face->gsub->FeatureList;
            int numfeatures = featurelist.FeatureCount;
            DEBUG("gsub table has %d features", numfeatures);
            for (int i = 0; i < numfeatures; i++) {
                HB_FeatureRecord *r = featurelist.FeatureRecord + i;
                DEBUG("   feature '%s'", tag_to_string(r->FeatureTag));
            }
        }
#endif
        HB_GSUB_Clear_Features(face->gsub);
        HB_UShort script_index;
        HB_Error error = HB_GSUB_Select_Script(face->gsub, tag, &script_index);
        if (!error) {
            DEBUG("script %s has script index %d", tag_to_string(script), script_index);
            while (features->tag) {
                HB_UShort feature_index;
                error = HB_GSUB_Select_Feature(face->gsub, features->tag, script_index, 0xffff, &feature_index);
                if (!error) {
                    DEBUG("  adding feature %s", tag_to_string(features->tag));
                    HB_GSUB_Add_Feature(face->gsub, feature_index, features->property);
                }
                ++features;
            }
        }
    }

    // reset
    face->has_opentype_kerning = false;

    if (face->gpos) {
        HB_GPOS_Clear_Features(face->gpos);
        HB_UShort script_index;
        HB_Error error = HB_GPOS_Select_Script(face->gpos, tag, &script_index);
        if (!error) {
#ifdef OT_DEBUG
            {
                HB_FeatureList featurelist = face->gpos->FeatureList;
                int numfeatures = featurelist.FeatureCount;
                DEBUG("gpos table has %d features", numfeatures);
                for(int i = 0; i < numfeatures; i++) {
                    HB_FeatureRecord *r = featurelist.FeatureRecord + i;
                    HB_UShort feature_index;
                    HB_GPOS_Select_Feature(face->gpos, r->FeatureTag, script_index, 0xffff, &feature_index);
                    DEBUG("   feature '%s'", tag_to_string(r->FeatureTag));
                }
            }
#endif
            HB_UInt *feature_tag_list_buffer;
            error = HB_GPOS_Query_Features(face->gpos, script_index, 0xffff, &feature_tag_list_buffer);
            if (!error) {
                HB_UInt *feature_tag_list = feature_tag_list_buffer;
                while (*feature_tag_list) {
                    HB_UShort feature_index;
                    bool skip = false;
                    if (*feature_tag_list == HB_MAKE_TAG('k', 'e', 'r', 'n')) {
                        if (face->current_flags & HB_ShaperFlag_NoKerning)
                            skip = true;
                        else
                            face->has_opentype_kerning = true;
                    }
                    features = disabled_features;
                    while (features->tag) {
                        if (*feature_tag_list == features->tag) {
                            skip = true;
                            break;
                        }
                        ++features;
                    }
                    // 'palt' should be turned off by default unless 'kern' is on
                    if (!face->has_opentype_kerning &&
                        *feature_tag_list == HB_MAKE_TAG('p', 'a', 'l', 't'))
                        skip = true;

                    if (skip) {
                        ++feature_tag_list;
                        continue;
                    }
                    error = HB_GPOS_Select_Feature(face->gpos, *feature_tag_list, script_index, 0xffff, &feature_index);
                    if (!error)
                        HB_GPOS_Add_Feature(face->gpos, feature_index, PositioningProperties);
                    ++feature_tag_list;
                }
                FREE(feature_tag_list_buffer);
            }
        }
    }

    return true;
}

HB_Bool HB_OpenTypeShape(HB_ShaperItem *item, const hb_uint32 *properties)
{
    HB_GlyphAttributes *tmpAttributes;
    unsigned int *tmpLogClusters;

    HB_Face face = item->face;

    face->length = item->num_glyphs;

    hb_buffer_clear(face->buffer);

    tmpAttributes = (HB_GlyphAttributes *) realloc(face->tmpAttributes, face->length*sizeof(HB_GlyphAttributes));
    if (!tmpAttributes)
        return false;
    face->tmpAttributes = tmpAttributes;

    tmpLogClusters = (unsigned int *) realloc(face->tmpLogClusters, face->length*sizeof(unsigned int));
    if (!tmpLogClusters)
        return false;
    face->tmpLogClusters = tmpLogClusters;

    for (int i = 0; i < face->length; ++i) {
        hb_buffer_add_glyph(face->buffer, item->glyphs[i], properties ? properties[i] : 0, i);
        face->tmpAttributes[i] = item->attributes[i];
        face->tmpLogClusters[i] = item->log_clusters[i];
    }

#ifdef OT_DEBUG
    DEBUG("-----------------------------------------");
//     DEBUG("log clusters before shaping:");
//     for (int j = 0; j < length; j++)
//         DEBUG("    log[%d] = %d", j, item->log_clusters[j]);
    DEBUG("original glyphs: %p", item->glyphs);
    for (int i = 0; i < length; ++i)
        DEBUG("   glyph=%4x", hb_buffer->in_string[i].gindex);
//     dump_string(hb_buffer);
#endif

    face->glyphs_substituted = false;
    if (face->gsub) {
        unsigned int error = HB_GSUB_Apply_String(face->gsub, face->buffer);
        if (error && error != HB_Err_Not_Covered)
            return false;
        face->glyphs_substituted = (error != HB_Err_Not_Covered);
    }

#ifdef OT_DEBUG
//     DEBUG("log clusters before shaping:");
//     for (int j = 0; j < length; j++)
//         DEBUG("    log[%d] = %d", j, item->log_clusters[j]);
    DEBUG("shaped glyphs:");
    for (int i = 0; i < length; ++i)
        DEBUG("   glyph=%4x", hb_buffer->in_string[i].gindex);
    DEBUG("-----------------------------------------");
//     dump_string(hb_buffer);
#endif

    return true;
}

HB_Bool HB_OpenTypePosition(HB_ShaperItem *item, int availableGlyphs, HB_Bool doLogClusters)
{
    HB_Face face = item->face;

    bool glyphs_positioned = false;
    if (face->gpos) {
        if (face->buffer->positions)
            memset(face->buffer->positions, 0, face->buffer->in_length*sizeof(HB_PositionRec));
        // #### check that passing "false,false" is correct
        glyphs_positioned = HB_GPOS_Apply_String(item->font, face->gpos, face->current_flags, face->buffer, false, false) != HB_Err_Not_Covered;
    }

    if (!face->glyphs_substituted && !glyphs_positioned) {
        HB_HeuristicPosition(item);
        return true; // nothing to do for us
    }

    // make sure we have enough space to write everything back
    if (availableGlyphs < (int)face->buffer->in_length) {
        item->num_glyphs = face->buffer->in_length;
        return false;
    }

    HB_Glyph *glyphs = item->glyphs;
    HB_GlyphAttributes *attributes = item->attributes;

    for (unsigned int i = 0; i < face->buffer->in_length; ++i) {
        glyphs[i] = face->buffer->in_string[i].gindex;
        attributes[i] = face->tmpAttributes[face->buffer->in_string[i].cluster];
        if (i && face->buffer->in_string[i].cluster == face->buffer->in_string[i-1].cluster)
            attributes[i].clusterStart = false; //FIXME - Shouldn't we otherwise set this to true, rather than leaving it?
    }
    item->num_glyphs = face->buffer->in_length;

    if (doLogClusters && face->glyphs_substituted) {
        // we can't do this for indic, as we pass the stuf in syllables and it's easier to do it in the shaper.
        // #### the reconstruction of the logclusters currently does not work if the original string
        // contains surrogate pairs

        unsigned short *logClusters = item->log_clusters;
        int clusterStart = 0;
        int oldIntermediateIndex = 0;

        // This code makes a mapping, logClusters, between the original utf16 string (item->string) and the final
        // set of glyphs (in_string).
        //
        // The code sets the value of logClusters[i] to the index of in_string containing the glyph that will render
        // item->string[i].
        //
        // This is complicated slightly because in_string[i].cluster is an index to an intermediate
        // array of glyphs - the array that we were passed as the original value of item->glyphs.
        // To map from the original string to the intermediate array of glyphs we have tmpLogClusters.
        //
        // So we have three groups of indexes:
        //
        // i,clusterStart = index to in_length, the final set of glyphs.  Also an index to attributes
        // intermediateIndex = index to the glyphs originally passed in.
        // stringIndex = index to item->string, the original string.

        int stringIndex = 0;
        // Iterate over the final set of glyphs...
        for (unsigned int i = 0; i < face->buffer->in_length; ++i) {
            // Get the index into the intermediate string for the start of the cluster of chars
            int intermediateIndex = face->buffer->in_string[i].cluster;
            if (intermediateIndex != oldIntermediateIndex) {
                // We have found the end of the cluster of chars in the intermediate string
                while (face->tmpLogClusters[stringIndex] < intermediateIndex) {
                    logClusters[stringIndex++] = clusterStart;
                }
                clusterStart = i;
                oldIntermediateIndex = intermediateIndex;
            }
        }
        while (stringIndex < face->length) {
            logClusters[stringIndex++] = clusterStart;
        }
    }

    // calulate the advances for the shaped glyphs
//     DEBUG("unpositioned: ");

    // positioning code:
    if (glyphs_positioned) {
        HB_GetGlyphAdvances(item);
        HB_Position positions = face->buffer->positions;
        HB_Fixed *advances = item->advances;

//         DEBUG("positioned glyphs:");
        for (unsigned int i = 0; i < face->buffer->in_length; i++) {
//             DEBUG("    %d:\t orig advance: (%d/%d)\tadv=(%d/%d)\tpos=(%d/%d)\tback=%d\tnew_advance=%d", i,
//                    glyphs[i].advance.x.toInt(), glyphs[i].advance.y.toInt(),
//                    (int)(positions[i].x_advance >> 6), (int)(positions[i].y_advance >> 6),
//                    (int)(positions[i].x_pos >> 6), (int)(positions[i].y_pos >> 6),
//                    positions[i].back, positions[i].new_advance);

            HB_Fixed adjustment = positions[i].x_advance;

            if (!(face->current_flags & HB_ShaperFlag_UseDesignMetrics))
                adjustment = HB_FIXED_ROUND(adjustment);

            if (positions[i].new_advance) {
                advances[i] = adjustment;
            } else {
                advances[i] += adjustment;
            }

            int back = 0;
            HB_FixedPoint *offsets = item->offsets;
            offsets[i].x = positions[i].x_pos;
            offsets[i].y = positions[i].y_pos;
            while (positions[i - back].back) {
                back += positions[i - back].back;
                offsets[i].x += positions[i - back].x_pos;
                offsets[i].y += positions[i - back].y_pos;
            }
            offsets[i].y = -offsets[i].y;

            if (item->item.bidiLevel % 2) {
                // ### may need to go back multiple glyphs like in ltr
                back = positions[i].back;
                while (back--)
                    offsets[i].x -= advances[i-back];
            } else {
                back = 0;
                while (positions[i - back].back) {
                    back += positions[i - back].back;
                    offsets[i].x -= advances[i-back];
                }
            }
//             DEBUG("   ->\tadv=%d\tpos=(%d/%d)",
//                    glyphs[i].advance.x.toInt(), glyphs[i].offset.x.toInt(), glyphs[i].offset.y.toInt());
        }
        item->kerning_applied = face->has_opentype_kerning;
    } else {
        HB_HeuristicPosition(item);
    }

#ifdef OT_DEBUG
    if (doLogClusters) {
        DEBUG("log clusters after shaping:\n");
        for (unsigned int j = 0; j < item->item.length; j++)
            DEBUG("    log[%d] = %d\n", j, item->log_clusters[j]);
    }
    DEBUG("final glyphs:\n");
    for (unsigned int i = 0; i < item->num_glyphs; ++i)
        DEBUG("   glyph=%4x char_index=%d mark: %d cmp: %d, clusterStart: %d advance=%d offset=%d/%d\n",
               glyphs[i], face->buffer->in_string[i].cluster, attributes[i].mark,
               attributes[i].combiningClass, attributes[i].clusterStart,
               item->advances[i] >> 6,
               item->offsets[i].x >> 6, item->offsets[i].y >> 6);
    DEBUG("-----------------------------------------\n");
#endif
    return true;
}

HB_Bool HB_ShapeItem(HB_ShaperItem *shaper_item)
{
    HB_Bool result = false;
    if (shaper_item->num_glyphs < shaper_item->item.length) {
        shaper_item->num_glyphs = shaper_item->item.length;
        return false;
    }
    assert(shaper_item->item.script < HB_ScriptCount);
    result = HB_ScriptEngines[shaper_item->item.script].shape(shaper_item);
    shaper_item->glyphIndicesPresent = false;
    return result;
}

