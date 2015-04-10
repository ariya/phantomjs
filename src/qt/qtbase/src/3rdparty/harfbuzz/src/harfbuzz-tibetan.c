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
 tibetan syllables are of the form:
    head position consonant
    first sub-joined consonant
    ....intermediate sub-joined consonants (if any)
    last sub-joined consonant
    sub-joined vowel (a-chung U+0F71)
    standard or compound vowel sign (or 'virama' for devanagari transliteration)
*/

typedef enum {
    TibetanOther,
    TibetanHeadConsonant,
    TibetanSubjoinedConsonant,
    TibetanSubjoinedVowel,
    TibetanVowel
} TibetanForm;

/* this table starts at U+0f40 */
static const unsigned char tibetanForm[0x80] = {
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,

    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,

    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,

    TibetanOther, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,

    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanOther, TibetanOther, TibetanOther
};


#define tibetan_form(c) \
    ((c) >= 0x0f40 && (c) < 0x0fc0 ? (TibetanForm)tibetanForm[(c) - 0x0f40] : TibetanOther)

#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature tibetan_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('a', 'b', 'v', 's'), AboveSubstProperty },
    { HB_MAKE_TAG('b', 'l', 'w', 's'), BelowSubstProperty },
    { HB_MAKE_TAG('c', 'a', 'l', 't'), CaltProperty },
    {0, 0}
};
#endif

static HB_Bool tibetan_shape_syllable(HB_Bool openType, HB_ShaperItem *item, HB_Bool invalid)
{
    hb_uint32 i;
    const HB_UChar16 *str = item->string + item->item.pos;
    int len = item->item.length;
#ifndef NO_OPENTYPE
    const int availableGlyphs = item->num_glyphs;
#endif
    HB_Bool haveGlyphs;
    HB_STACKARRAY(HB_UChar16, reordered, len + 4);

    if (item->num_glyphs < item->item.length + 4) {
        item->num_glyphs = item->item.length + 4;
        HB_FREE_STACKARRAY(reordered);
        return FALSE;
    }

    if (invalid) {
        *reordered = 0x25cc;
        memcpy(reordered+1, str, len*sizeof(HB_UChar16));
        len++;
        str = reordered;
    }

    haveGlyphs = item->font->klass->convertStringToGlyphIndices(item->font,
                                                                str, len,
                                                                item->glyphs, &item->num_glyphs,
                                                                item->item.bidiLevel % 2);

    HB_FREE_STACKARRAY(reordered);

    if (!haveGlyphs)
        return FALSE;

    for (i = 0; i < item->item.length; i++) {
        item->attributes[i].mark = FALSE;
        item->attributes[i].clusterStart = FALSE;
        item->attributes[i].justification = 0;
        item->attributes[i].zeroWidth = FALSE;
/*        IDEBUG("    %d: %4x", i, str[i]); */
    }

    /* now we have the syllable in the right order, and can start running it through open type. */

#ifndef NO_OPENTYPE
    if (openType) {
        HB_OpenTypeShape(item, /*properties*/0);
        if (!HB_OpenTypePosition(item, availableGlyphs, /*doLogClusters*/FALSE))
            return FALSE;
    } else {
        HB_HeuristicPosition(item);
    }
#endif

    item->attributes[0].clusterStart = TRUE;
    return TRUE;
}


static int tibetan_nextSyllableBoundary(const HB_UChar16 *s, int start, int end, HB_Bool *invalid)
{
    const HB_UChar16 *uc = s + start;

    int pos = 0;
    TibetanForm state = tibetan_form(*uc);

/*     qDebug("state[%d]=%d (uc=%4x)", pos, state, uc[pos]);*/
    pos++;

    if (state != TibetanHeadConsonant) {
        if (state != TibetanOther)
            *invalid = TRUE;
        goto finish;
    }

    while (pos < end - start) {
        TibetanForm newState = tibetan_form(uc[pos]);
        switch(newState) {
        case TibetanSubjoinedConsonant:
        case TibetanSubjoinedVowel:
            if (state != TibetanHeadConsonant &&
                 state != TibetanSubjoinedConsonant)
                goto finish;
            state = newState;
            break;
        case TibetanVowel:
            if (state != TibetanHeadConsonant &&
                 state != TibetanSubjoinedConsonant &&
                 state != TibetanSubjoinedVowel)
                goto finish;
            break;
        case TibetanOther:
        case TibetanHeadConsonant:
            goto finish;
        }
        pos++;
    }

finish:
    *invalid = FALSE;
    return start+pos;
}

HB_Bool HB_TibetanShape(HB_ShaperItem *item)
{

    HB_Bool openType = FALSE;
    unsigned short *logClusters = item->log_clusters;

    HB_ShaperItem syllable = *item;
    int first_glyph = 0;

    int sstart = item->item.pos;
    int end = sstart + item->item.length;

    assert(item->item.script == HB_Script_Tibetan);

#ifndef NO_OPENTYPE
    openType = HB_SelectScript(item, tibetan_features);
#endif

    while (sstart < end) {
        HB_Bool invalid;
        int i;
        int send = tibetan_nextSyllableBoundary(item->string, sstart, end, &invalid);
/*        IDEBUG("syllable from %d, length %d, invalid=%s", sstart, send-sstart,
                 invalid ? "TRUE" : "FALSE"); */
        syllable.item.pos = sstart;
        syllable.item.length = send-sstart;
        syllable.glyphs = item->glyphs + first_glyph;
        syllable.attributes = item->attributes + first_glyph;
        syllable.offsets = item->offsets + first_glyph;
        syllable.advances = item->advances + first_glyph;
        syllable.num_glyphs = item->num_glyphs - first_glyph;
        if (!tibetan_shape_syllable(openType, &syllable, invalid)) {
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

void HB_TibetanAttributes(HB_Script script, const HB_UChar16 *text, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes)
{
    int end = from + len;
    const HB_UChar16 *uc = text + from;
    hb_uint32 i = 0;
    HB_UNUSED(script);
    attributes += from;
    while (i < len) {
        HB_Bool invalid;
        hb_uint32 boundary = tibetan_nextSyllableBoundary(text, from+i, end, &invalid) - from;

        attributes[i].graphemeBoundary = TRUE;

        if (boundary > len-1) boundary = len;
        i++;
        while (i < boundary) {
            attributes[i].graphemeBoundary = FALSE;
            ++uc;
            ++i;
        }
        assert(i == boundary);
    }
}


