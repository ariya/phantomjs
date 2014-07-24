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

/*
// Hangul is a syllable based script. Unicode reserves a large range
// for precomposed hangul, where syllables are already precomposed to
// their final glyph shape. In addition, a so called jamo range is
// defined, that can be used to express old Hangul. Modern hangul
// syllables can also be expressed as jamo, and should be composed
// into syllables. The operation is rather simple and mathematical.

// Every hangul jamo is classified as being either a Leading consonant
// (L), and intermediat Vowel (V) or a trailing consonant (T). Modern
// hangul syllables (the ones in the precomposed area can be of type
// LV or LVT.
//
// Syllable breaks do _not_ occur between:
//
// L              L, V or precomposed
// V, LV          V, T
// LVT, T         T
//
// A standard syllable is of the form L+V+T*. The above rules allow
// nonstandard syllables L*V*T*. To transform them into standard
// syllables fill characters L_f and V_f can be inserted.
*/

enum {
    Hangul_SBase = 0xac00,
    Hangul_LBase = 0x1100,
    Hangul_VBase = 0x1161,
    Hangul_TBase = 0x11a7,
    Hangul_SCount = 11172,
    Hangul_LCount = 19,
    Hangul_VCount = 21,
    Hangul_TCount = 28,
    Hangul_NCount = 21*28
};

#define hangul_isPrecomposed(uc) \
    (uc >= Hangul_SBase && uc < Hangul_SBase + Hangul_SCount)

#define hangul_isLV(uc) \
    ((uc - Hangul_SBase) % Hangul_TCount == 0)

typedef enum {
    L,
    V,
    T,
    LV,
    LVT,
    X
} HangulType;

static HangulType hangul_type(unsigned short uc) {
    if (uc > Hangul_SBase && uc < Hangul_SBase + Hangul_SCount)
        return hangul_isLV(uc) ? LV : LVT;
    if (uc < Hangul_LBase || uc > 0x11ff)
        return X;
    if (uc < Hangul_VBase)
        return L;
    if (uc < Hangul_TBase)
        return V;
    return T;
}

static int hangul_nextSyllableBoundary(const HB_UChar16 *s, int start, int end)
{
    const HB_UChar16 *uc = s + start;

    HangulType state = hangul_type(*uc);
    int pos = 1;

    while (pos < end - start) {
        HangulType newState = hangul_type(uc[pos]);
        switch(newState) {
        case X:
            goto finish;
        case L:
        case V:
        case T:
            if (state > newState)
                goto finish;
            state = newState;
            break;
        case LV:
            if (state > L)
                goto finish;
            state = V;
            break;
        case LVT:
            if (state > L)
                goto finish;
            state = T;
        }
        ++pos;
    }

 finish:
    return start+pos;
}

#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature hangul_features [] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('l', 'j', 'm', 'o'), CcmpProperty },
    { HB_MAKE_TAG('v', 'j', 'm', 'o'), CcmpProperty },
    { HB_MAKE_TAG('t', 'j', 'm', 'o'), CcmpProperty },
    { 0, 0 }
};
#endif

static HB_Bool hangul_shape_syllable(HB_ShaperItem *item, HB_Bool openType)
{
    const HB_UChar16 *ch = item->string + item->item.pos;
    int len = item->item.length;
#ifndef NO_OPENTYPE
    const int availableGlyphs = item->num_glyphs;
#endif

    int i;
    HB_UChar16 composed = 0;
    /* see if we can compose the syllable into a modern hangul */
    if (item->item.length == 2) {
        int LIndex = ch[0] - Hangul_LBase;
        int VIndex = ch[1] - Hangul_VBase;
        if (LIndex >= 0 && LIndex < Hangul_LCount &&
            VIndex >= 0 && VIndex < Hangul_VCount)
            composed = (LIndex * Hangul_VCount + VIndex) * Hangul_TCount + Hangul_SBase;
    } else if (item->item.length == 3) {
        int LIndex = ch[0] - Hangul_LBase;
        int VIndex = ch[1] - Hangul_VBase;
        int TIndex = ch[2] - Hangul_TBase;
        if (LIndex >= 0 && LIndex < Hangul_LCount &&
            VIndex >= 0 && VIndex < Hangul_VCount &&
            TIndex >= 0 && TIndex < Hangul_TCount)
            composed = (LIndex * Hangul_VCount + VIndex) * Hangul_TCount + TIndex + Hangul_SBase;
    }



    /* if we have a modern hangul use the composed form */
    if (composed) {
        ch = &composed;
        len = 1;
    }

    if (!item->font->klass->convertStringToGlyphIndices(item->font,
                                                        ch, len,
                                                        item->glyphs, &item->num_glyphs,
                                                        item->item.bidiLevel % 2))
        return FALSE;
    for (i = 0; i < len; i++) {
        item->attributes[i].mark = FALSE;
        item->attributes[i].clusterStart = FALSE;
        item->attributes[i].justification = 0;
        item->attributes[i].zeroWidth = FALSE;
        /*IDEBUG("    %d: %4x", i, ch[i].unicode()); */
    }

#ifndef NO_OPENTYPE
    if (!composed && openType) {
        HB_Bool positioned;

        HB_STACKARRAY(unsigned short, logClusters, len);
        for (i = 0; i < len; ++i)
            logClusters[i] = i;
        item->log_clusters = logClusters;

        HB_OpenTypeShape(item, /*properties*/0);

        positioned = HB_OpenTypePosition(item, availableGlyphs, /*doLogClusters*/FALSE);

        HB_FREE_STACKARRAY(logClusters);

        if (!positioned)
            return FALSE;
    } else {
        HB_HeuristicPosition(item);
    }
#endif

    item->attributes[0].clusterStart = TRUE;
    return TRUE;
}

HB_Bool HB_HangulShape(HB_ShaperItem *item)
{
    const HB_UChar16 *uc = item->string + item->item.pos;
    HB_Bool allPrecomposed = TRUE;
    int i;

    assert(item->item.script == HB_Script_Hangul);

    for (i = 0; i < (int)item->item.length; ++i) {
        if (!hangul_isPrecomposed(uc[i])) {
            allPrecomposed = FALSE;
            break;
        }
    }

    if (!allPrecomposed) {
        HB_Bool openType = FALSE;
        unsigned short *logClusters = item->log_clusters;
        HB_ShaperItem syllable;
        int first_glyph = 0;
        int sstart = item->item.pos;
        int end = sstart + item->item.length;

#ifndef NO_OPENTYPE
        openType = HB_SelectScript(item, hangul_features);
#endif
        syllable = *item;

        while (sstart < end) {
            int send = hangul_nextSyllableBoundary(item->string, sstart, end);

            syllable.item.pos = sstart;
            syllable.item.length = send-sstart;
            syllable.glyphs = item->glyphs + first_glyph;
            syllable.attributes = item->attributes + first_glyph;
            syllable.offsets = item->offsets + first_glyph;
            syllable.advances = item->advances + first_glyph;
            syllable.num_glyphs = item->num_glyphs - first_glyph;
            if (!hangul_shape_syllable(&syllable, openType)) {
                item->num_glyphs += syllable.num_glyphs;
                return FALSE;
            }
            /* fix logcluster array */
            for (i = sstart; i < send; ++i)
                logClusters[i-item->item.pos] = first_glyph;
            sstart = send;
            first_glyph += syllable.num_glyphs;
        }
        item->num_glyphs = first_glyph;
        return TRUE;
    }

    return HB_BasicShape(item);
}


