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

/*
// Uniscribe also defines dlig for Hebrew, but we leave this out for now, as it's mostly
// ligatures one does not want in modern Hebrew (as lam-alef ligatures).
*/
#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature hebrew_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    {0, 0}
};
#endif

/* Hebrew shaping. In the non opentype case we try to use the
   presentation forms specified for Hebrew. Especially for the
   ligatures with Dagesh this gives much better results than we could
   achieve manually.
*/
HB_Bool HB_HebrewShape(HB_ShaperItem *shaper_item)
{
    enum {
        Dagesh = 0x5bc,
        ShinDot = 0x5c1,
        SinDot = 0x5c2,
        Patah = 0x5b7,
        Qamats = 0x5b8,
        Holam = 0x5b9,
        Rafe = 0x5bf
    };

    assert(shaper_item->item.script == HB_Script_Hebrew);

#ifndef NO_OPENTYPE
    if (HB_SelectScript(shaper_item, hebrew_features)) {

        const int availableGlyphs = shaper_item->num_glyphs;
        if (!HB_ConvertStringToGlyphIndices(shaper_item))
            return FALSE;

        HB_HeuristicSetGlyphAttributes(shaper_item);
        HB_OpenTypeShape(shaper_item, /*properties*/0);
        return HB_OpenTypePosition(shaper_item, availableGlyphs, /*doLogClusters*/TRUE);
    }
#endif

    {
        const HB_UChar16 *uc = shaper_item->string + shaper_item->item.pos;
        unsigned short *logClusters = shaper_item->log_clusters;
        HB_GlyphAttributes *attributes = shaper_item->attributes;

        HB_Bool haveGlyphs;
        int slen = 1;
        int cluster_start = 0;
        hb_uint32 i;

        HB_STACKARRAY(HB_UChar16, shapedChars, 2 * shaper_item->item.length);
        *shapedChars = *uc;
        logClusters[0] = 0;

        for (i = 1; i < shaper_item->item.length; ++i) {
            hb_uint16 base = shapedChars[cluster_start];
            hb_uint16 shaped = 0;
            HB_Bool invalid = FALSE;
            if (uc[i] == Dagesh) {
                if (base >= 0x5d0
                    && base <= 0x5ea
                    && base != 0x5d7
                    && base != 0x5dd
                    && base != 0x5df
                    && base != 0x5e2
                    && base != 0x5e5) {
                    shaped = base - 0x5d0 + 0xfb30;
                } else if (base == 0xfb2a || base == 0xfb2b /* Shin with Shin or Sin dot */) {
                    shaped = base + 2;
                } else {
                    invalid = TRUE;
                }
            } else if (uc[i] == ShinDot) {
                if (base == 0x05e9)
                    shaped = 0xfb2a;
                else if (base == 0xfb49)
                    shaped = 0xfb2c;
                else
                    invalid = TRUE;
            } else if (uc[i] == SinDot) {
                if (base == 0x05e9)
                    shaped = 0xfb2b;
                else if (base == 0xfb49)
                    shaped = 0xfb2d;
                else
                    invalid = TRUE;
            } else if (uc[i] == Patah) {
                if (base == 0x5d0)
                    shaped = 0xfb2e;
            } else if (uc[i] == Qamats) {
                if (base == 0x5d0)
                    shaped = 0xfb2f;
            } else if (uc[i] == Holam) {
                if (base == 0x5d5)
                    shaped = 0xfb4b;
            } else if (uc[i] == Rafe) {
                if (base == 0x5d1)
                    shaped = 0xfb4c;
                else if (base == 0x5db)
                    shaped = 0xfb4d;
                else if (base == 0x5e4)
                    shaped = 0xfb4e;
            }

            if (invalid) {
                shapedChars[slen] = 0x25cc;
                attributes[slen].clusterStart = TRUE;
                attributes[slen].mark = FALSE;
                attributes[slen].combiningClass = 0;
                cluster_start = slen;
                ++slen;
            }
            if (shaped) {
                if (shaper_item->font->klass->canRender(shaper_item->font, (HB_UChar16 *)&shaped, 1)) {
                    shapedChars[cluster_start] = shaped;
                } else
                    shaped = 0;
            }
            if (!shaped) {
                HB_CharCategory category;
                int cmb;
                shapedChars[slen] = uc[i];
                HB_GetUnicodeCharProperties(uc[i], &category, &cmb);
                if (category != HB_Mark_NonSpacing) {
                    attributes[slen].clusterStart = TRUE;
                    attributes[slen].mark = FALSE;
                    attributes[slen].combiningClass = 0;
                    attributes[slen].dontPrint = HB_IsControlChar(uc[i]);
                    cluster_start = slen;
                } else {
                    attributes[slen].clusterStart = FALSE;
                    attributes[slen].mark = TRUE;
                    attributes[slen].combiningClass = cmb;
                }
                ++slen;
            }
            logClusters[i] = cluster_start;
        }

        haveGlyphs = shaper_item->font->klass
            ->convertStringToGlyphIndices(shaper_item->font,
                                          shapedChars, slen,
                                          shaper_item->glyphs, &shaper_item->num_glyphs,
                                          shaper_item->item.bidiLevel % 2);

        HB_FREE_STACKARRAY(shapedChars);

        if (!haveGlyphs)
            return FALSE;

        HB_HeuristicPosition(shaper_item);
    }

    return TRUE;
}

