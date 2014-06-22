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

#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature greek_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('l', 'i', 'g', 'a'), CcmpProperty },
    { HB_MAKE_TAG('c', 'l', 'i', 'g'), CcmpProperty },
    {0, 0}
};
#endif

/*
  Greek decompositions
*/


typedef struct _hb_greek_decomposition {
    HB_UChar16 composed;
    HB_UChar16 base;
} hb_greek_decomposition;

static const hb_greek_decomposition decompose_0x300[] = {
    { 0x1FBA, 0x0391 },
    { 0x1FC8, 0x0395 },
    { 0x1FCA, 0x0397 },
    { 0x1FDA, 0x0399 },
    { 0x1FF8, 0x039F },
    { 0x1FEA, 0x03A5 },
    { 0x1FFA, 0x03A9 },
    { 0x1F70, 0x03B1 },
    { 0x1F72, 0x03B5 },
    { 0x1F74, 0x03B7 },
    { 0x1F76, 0x03B9 },
    { 0x1F78, 0x03BF },
    { 0x1F7A, 0x03C5 },
    { 0x1F7C, 0x03C9 },
    { 0x1FD2, 0x03CA },
    { 0x1FE2, 0x03CB },
    { 0x1F02, 0x1F00 },
    { 0, 0 }
};

static HB_UChar16 compose_0x300(HB_UChar16 base)
{
    if ((base ^ 0x1f00) < 0x100) {
        if (base <= 0x1f69 && !(base & 0x6))
            return base + 2;
        if (base == 0x1fbf)
            return 0x1fcd;
        if (base == 0x1ffe)
            return 0x1fdd;
        return 0;
    }
    {
        const hb_greek_decomposition *d = decompose_0x300;
        while (d->base && d->base != base)
            ++d;
        return d->composed;
    }
}

static const hb_greek_decomposition decompose_0x301[] = {
    { 0x0386, 0x0391 },
    { 0x0388, 0x0395 },
    { 0x0389, 0x0397 },
    { 0x038A, 0x0399 },
    { 0x038C, 0x039F },
    { 0x038E, 0x03A5 },
    { 0x038F, 0x03A9 },
    { 0x03AC, 0x03B1 },
    { 0x03AD, 0x03B5 },
    { 0x03AE, 0x03B7 },
    { 0x03AF, 0x03B9 },
    { 0x03CC, 0x03BF },
    { 0x03CD, 0x03C5 },
    { 0x03CE, 0x03C9 },
    { 0x0390, 0x03CA },
    { 0x03B0, 0x03CB },
    { 0x03D3, 0x03D2 },
    { 0, 0 }
};


static HB_UChar16 compose_0x301(HB_UChar16 base)
{
    if ((base ^ 0x1f00) < 0x100) {
        if (base <= 0x1f69 && !(base & 0x6))
            return base + 4;
        if (base == 0x1fbf)
            return 0x1fce;
        if (base == 0x1ffe)
            return 0x1fde;
    }
    {
        const hb_greek_decomposition *d = decompose_0x301;
        while (d->base && d->base != base)
            ++d;
        return d->composed;
    }
}

static const hb_greek_decomposition decompose_0x304[] = {
    { 0x1FB9, 0x0391 },
    { 0x1FD9, 0x0399 },
    { 0x1FE9, 0x03A5 },
    { 0x1FB1, 0x03B1 },
    { 0x1FD1, 0x03B9 },
    { 0x1FE1, 0x03C5 },
    { 0, 0 }
};

static HB_UChar16 compose_0x304(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x304;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

static const hb_greek_decomposition decompose_0x306[] = {
    { 0x1FB8, 0x0391 },
    { 0x1FD8, 0x0399 },
    { 0x1FE8, 0x03A5 },
    { 0x1FB0, 0x03B1 },
    { 0x1FD0, 0x03B9 },
    { 0x1FE0, 0x03C5 },
    { 0, 0 }
};

static HB_UChar16 compose_0x306(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x306;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

static const hb_greek_decomposition decompose_0x308[] = {
    { 0x03AA, 0x0399  },
    { 0x03AB, 0x03A5  },
    { 0x03CA, 0x03B9  },
    { 0x03CB, 0x03C5  },
    { 0x03D4, 0x03D2  },
    { 0, 0 }
};

static HB_UChar16 compose_0x308(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x308;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}


static const hb_greek_decomposition decompose_0x313[] = {
    { 0x1F08, 0x0391 },
    { 0x1F18, 0x0395 },
    { 0x1F28, 0x0397 },
    { 0x1F38, 0x0399 },
    { 0x1F48, 0x039F },
    { 0x1F68, 0x03A9 },
    { 0x1F00, 0x03B1 },
    { 0x1F10, 0x03B5 },
    { 0x1F20, 0x03B7 },
    { 0x1F30, 0x03B9 },
    { 0x1F40, 0x03BF },
    { 0x1FE4, 0x03C1 },
    { 0x1F50, 0x03C5 },
    { 0x1F60, 0x03C9 },
    { 0, 0 }
};

static HB_UChar16 compose_0x313(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x313;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

static const hb_greek_decomposition decompose_0x314[] = {
    { 0x1F09, 0x0391 },
    { 0x1F19, 0x0395 },
    { 0x1F29, 0x0397 },
    { 0x1F39, 0x0399 },
    { 0x1F49, 0x039F },
    { 0x1FEC, 0x03A1 },
    { 0x1F59, 0x03A5 },
    { 0x1F69, 0x03A9 },
    { 0x1F01, 0x03B1 },
    { 0x1F11, 0x03B5 },
    { 0x1F21, 0x03B7 },
    { 0x1F31, 0x03B9 },
    { 0x1F41, 0x03BF },
    { 0x1FE5, 0x03C1 },
    { 0x1F51, 0x03C5 },
    { 0x1F61, 0x03C9 },
    { 0, 0 }
};

static HB_UChar16 compose_0x314(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x314;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

static const hb_greek_decomposition decompose_0x342[] = {
    { 0x1FB6, 0x03B1 },
    { 0x1FC6, 0x03B7 },
    { 0x1FD6, 0x03B9 },
    { 0x1FE6, 0x03C5 },
    { 0x1FF6, 0x03C9 },
    { 0x1FD7, 0x03CA },
    { 0x1FE7, 0x03CB },
    { 0x1F06, 0x1F00 },
    { 0x1F07, 0x1F01 },
    { 0x1F0E, 0x1F08 },
    { 0x1F0F, 0x1F09 },
    { 0x1F26, 0x1F20 },
    { 0x1F27, 0x1F21 },
    { 0x1F2E, 0x1F28 },
    { 0x1F2F, 0x1F29 },
    { 0x1F36, 0x1F30 },
    { 0x1F37, 0x1F31 },
    { 0x1F3E, 0x1F38 },
    { 0x1F3F, 0x1F39 },
    { 0x1F56, 0x1F50 },
    { 0x1F57, 0x1F51 },
    { 0x1F5F, 0x1F59 },
    { 0x1F66, 0x1F60 },
    { 0x1F67, 0x1F61 },
    { 0x1F6E, 0x1F68 },
    { 0x1F6F, 0x1F69 },
    { 0x1FCF, 0x1FBF },
    { 0x1FDF, 0x1FFE },
    { 0, 0 }
};

static HB_UChar16 compose_0x342(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x342;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

static const hb_greek_decomposition decompose_0x345[] = {
    { 0x1FBC, 0x0391 },
    { 0x1FCC, 0x0397 },
    { 0x1FFC, 0x03A9 },
    { 0x1FB4, 0x03AC },
    { 0x1FC4, 0x03AE },
    { 0x1FB3, 0x03B1 },
    { 0x1FC3, 0x03B7 },
    { 0x1FF3, 0x03C9 },
    { 0x1FF4, 0x03CE },
    { 0x1F80, 0x1F00 },
    { 0x1F81, 0x1F01 },
    { 0x1F82, 0x1F02 },
    { 0x1F83, 0x1F03 },
    { 0x1F84, 0x1F04 },
    { 0x1F85, 0x1F05 },
    { 0x1F86, 0x1F06 },
    { 0x1F87, 0x1F07 },
    { 0x1F88, 0x1F08 },
    { 0x1F89, 0x1F09 },
    { 0x1F8A, 0x1F0A },
    { 0x1F8B, 0x1F0B },
    { 0x1F8C, 0x1F0C },
    { 0x1F8D, 0x1F0D },
    { 0x1F8E, 0x1F0E },
    { 0x1F8F, 0x1F0F },
    { 0x1F90, 0x1F20 },
    { 0x1F91, 0x1F21 },
    { 0x1F92, 0x1F22 },
    { 0x1F93, 0x1F23 },
    { 0x1F94, 0x1F24 },
    { 0x1F95, 0x1F25 },
    { 0x1F96, 0x1F26 },
    { 0x1F97, 0x1F27 },
    { 0x1F98, 0x1F28 },
    { 0x1F99, 0x1F29 },
    { 0x1F9A, 0x1F2A },
    { 0x1F9B, 0x1F2B },
    { 0x1F9C, 0x1F2C },
    { 0x1F9D, 0x1F2D },
    { 0x1F9E, 0x1F2E },
    { 0x1F9F, 0x1F2F },
    { 0x1FA0, 0x1F60 },
    { 0x1FA1, 0x1F61 },
    { 0x1FA2, 0x1F62 },
    { 0x1FA3, 0x1F63 },
    { 0x1FA4, 0x1F64 },
    { 0x1FA5, 0x1F65 },
    { 0x1FA6, 0x1F66 },
    { 0x1FA7, 0x1F67 },
    { 0x1FA8, 0x1F68 },
    { 0x1FA9, 0x1F69 },
    { 0x1FAA, 0x1F6A },
    { 0x1FAB, 0x1F6B },
    { 0x1FAC, 0x1F6C },
    { 0x1FAD, 0x1F6D },
    { 0x1FAE, 0x1F6E },
    { 0x1FAF, 0x1F6F },
    { 0x1FB2, 0x1F70 },
    { 0x1FC2, 0x1F74 },
    { 0x1FF2, 0x1F7C },
    { 0x1FB7, 0x1FB6 },
    { 0x1FC7, 0x1FC6 },
    { 0x1FF7, 0x1FF6 },
    { 0, 0 }
};

static HB_UChar16 compose_0x345(HB_UChar16 base)
{
    const hb_greek_decomposition *d = decompose_0x345;
    while (d->base && d->base != base)
        ++d;
    return d->composed;
}

/*
  Greek shaping. Heuristic positioning can't render polytonic greek correctly. We're a lot
  better off mapping greek chars with diacritics to the characters in the extended greek
  region in Unicode if possible.
*/
HB_Bool HB_GreekShape(HB_ShaperItem *shaper_item)
{
    const int availableGlyphs = shaper_item->num_glyphs;
    const HB_UChar16 *uc = shaper_item->string + shaper_item->item.pos;
    unsigned short *logClusters = shaper_item->log_clusters;
    HB_GlyphAttributes *attributes = shaper_item->attributes;

    HB_Bool haveGlyphs;
    int slen = 1;
    int cluster_start = 0;
    hb_uint32 i;

    HB_STACKARRAY(HB_UChar16, shapedChars, 2 * shaper_item->item.length);

    assert(shaper_item->item.script == HB_Script_Greek);

    *shapedChars = *uc;
    logClusters[0] = 0;

    attributes[0].mark = false;
    attributes[0].clusterStart = true;
    attributes[0].dontPrint = false;

    for (i = 1; i < shaper_item->item.length; ++i) {
        hb_uint16 base = shapedChars[slen-1];
        hb_uint16 shaped = 0;
        if (uc[i] == 0x300)
            shaped = compose_0x300(base);
        else if (uc[i] == 0x301)
            shaped = compose_0x301(base);
        else if (uc[i] == 0x304)
            shaped = compose_0x304(base);
        else if (uc[i] == 0x306)
            shaped = compose_0x306(base);
        else if (uc[i] == 0x308)
            shaped = compose_0x308(base);
        else if (uc[i] == 0x313)
            shaped = compose_0x313(base);
        else if (uc[i] == 0x314)
            shaped = compose_0x314(base);
        else if (uc[i] == 0x342)
            shaped = compose_0x342(base);
        else if (uc[i] == 0x345)
            shaped = compose_0x345(base);

        if (shaped) {
            if (shaper_item->font->klass->canRender(shaper_item->font, (HB_UChar16 *)&shaped, 1)) {
                shapedChars[slen-1] = shaped;
            } else {
                shaped = 0;
            }
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

#ifndef NO_OPENTYPE
    if (HB_SelectScript(shaper_item, greek_features)) {
        HB_OpenTypeShape(shaper_item, /*properties*/0);
        return HB_OpenTypePosition(shaper_item, availableGlyphs, /*doLogClusters*/TRUE);
    }
#endif
    HB_HeuristicPosition(shaper_item);

    return TRUE;
}

