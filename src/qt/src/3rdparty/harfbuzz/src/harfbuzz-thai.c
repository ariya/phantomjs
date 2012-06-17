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
#include "harfbuzz-external.h"

#include <assert.h>
#include <stdio.h>

#define LIBTHAI_MAJOR   0

/*
 * if libthai changed please update these codes too.
 */
struct thcell_t {
    unsigned char base;      /**< base character */
    unsigned char hilo;      /**< upper/lower vowel/diacritic */
    unsigned char top;       /**< top-level mark */
};
typedef int (*th_brk_def) (const unsigned char*, int*, size_t);
typedef int (*th_render_cell_tis_def) (struct thcell_t cell, unsigned char res[], size_t res_sz, int is_decomp_am);
typedef int (*th_render_cell_win_def) (struct thcell_t cell, unsigned char res[], size_t res_sz, int is_decomp_am);
typedef int (*th_render_cell_mac_def) (struct thcell_t cell, unsigned char res[], size_t res_sz, int is_decomp_am);
typedef size_t (*th_next_cell_def) (const unsigned char *, size_t, struct thcell_t *, int);

/* libthai releated function handles */
static th_brk_def th_brk = 0;
static th_next_cell_def th_next_cell = 0;
static th_render_cell_tis_def th_render_cell_tis = 0;
static th_render_cell_win_def th_render_cell_win = 0;
static th_render_cell_mac_def th_render_cell_mac = 0;

static int init_libthai() {
    if (!th_brk || !th_next_cell || !th_render_cell_tis || !th_render_cell_win || !th_render_cell_mac) {
        th_brk = (th_brk_def) HB_Library_Resolve("thai", (int)LIBTHAI_MAJOR, "th_brk");
        th_next_cell = (th_next_cell_def)HB_Library_Resolve("thai", LIBTHAI_MAJOR, "th_next_cell");
        th_render_cell_tis = (th_render_cell_tis_def) HB_Library_Resolve("thai", (int)LIBTHAI_MAJOR, "th_render_cell_tis");
        th_render_cell_win = (th_render_cell_win_def) HB_Library_Resolve("thai", (int)LIBTHAI_MAJOR, "th_render_cell_win");
        th_render_cell_mac = (th_render_cell_mac_def) HB_Library_Resolve("thai", (int)LIBTHAI_MAJOR, "th_render_cell_mac");
    }
    if (th_brk && th_next_cell && th_render_cell_tis && th_render_cell_win && th_render_cell_mac)
        return 1;
    else
        return 0;
}

static void to_tis620(const HB_UChar16 *string, hb_uint32 len, const char *cstr)
{
    hb_uint32 i;
    unsigned char *result = (unsigned char *)cstr;

    for (i = 0; i < len; ++i) {
        if (string[i] <= 0xa0)
            result[i] = (unsigned char)string[i];
        else if (string[i] >= 0xe01 && string[i] <= 0xe5b)
            result[i] = (unsigned char)(string[i] - 0xe00 + 0xa0);
        else
            result[i] = (unsigned char)~0; // Same encoding as libthai uses for invalid chars
    }

    result[len] = 0;
}

/*
 * ---------------------------------------------------------------------------
 * Thai Shaper / Attributes
 * ---------------------------------------------------------------------------
 */

/*
 * USe basic_features prepare for future adding.
 */
#ifndef NO_OPENTYPE
static const HB_OpenTypeFeature thai_features[] = {
    { HB_MAKE_TAG('c', 'c', 'm', 'p'), CcmpProperty },
    { HB_MAKE_TAG('l', 'i', 'g', 'a'), CcmpProperty },
    { HB_MAKE_TAG('c', 'l', 'i', 'g'), CcmpProperty },
    {0, 0}
};
#endif

/* TIS-to-Unicode glyph maps for characters 0x80-0xff */
static int tis620_0[128] = {
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    0x0020, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0e4e, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x0e5a, 0x0e5b,      0,      0,      0,      0
};

static int tis620_1[128] = {
    0xf89e,      0,      0, 0xf88c, 0xf88f, 0xf892, 0xf895, 0xf898,
    0xf88b, 0xf88e, 0xf891, 0xf894, 0xf897,      0,      0, 0xf899,
    0xf89a,      0, 0xf884, 0xf889, 0xf885, 0xf886, 0xf887, 0xf888,
    0xf88a, 0xf88d, 0xf890, 0xf893, 0xf896,      0,      0,      0,
    /**/ 0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d,      0, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59,      0,      0, 0xf89b, 0xf89c, 0xf89d,      0
};

static int tis620_2[128] = {
    0xf700, 0xf701, 0xf702, 0xf703, 0xf704, 0x2026, 0xf705, 0xf706,
    0xf707, 0xf708, 0xf709, 0xf70a, 0xf70b, 0xf70c, 0xf70d, 0xf70e,
    0xf70f, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0xf710, 0xf711, 0xf712, 0xf713, 0xf714, 0xf715, 0xf716, 0xf717,
    0x00a0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0e4e, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x0e5a, 0x0e5b, 0xf718, 0xf719, 0xf71a,      0
};

enum ThaiFontType {
    TIS,
    WIN,
    MAC,
};

static int thai_get_glyph_index (ThaiFontType font_type, unsigned char c)
{
    switch (font_type){
        case TIS: return (c & 0x80) ? tis620_0[c & 0x7f] : c;
        case WIN: return (c & 0x80) ? tis620_1[c & 0x7f] : c;
        case MAC: return (c & 0x80) ? tis620_2[c & 0x7f] : c;
        default:  return 0;
    }
}

static int thai_contain_glyphs (HB_ShaperItem *shaper_item, const int glyph_map[128])
{
    unsigned char c;

    for (c = 0; c < 0x80; c++) {
        if ( glyph_map[c] ) {
            if ( !shaper_item->font->klass->canRender (shaper_item->font, (HB_UChar16 *) &glyph_map[c], 1) )
                return 0;
        }
    }
    return 1;
}

static ThaiFontType getThaiFontType(HB_ShaperItem *shaper_item)
{
    if ( thai_contain_glyphs (shaper_item, tis620_2) )
        return MAC;
    else if ( thai_contain_glyphs (shaper_item, tis620_1) )
        return WIN;
    else
        return TIS;
}

/*
 * convert to the correct display level of THAI vowels and marks.
 */
static HB_Bool HB_ThaiConvertStringToGlyphIndices (HB_ShaperItem *item)
{
    char s[128];
    char *cstr = s;
    const HB_UChar16 *string = item->string + item->item.pos;
    const hb_uint32 len = item->item.length;
    unsigned short *logClusters = item->log_clusters;
    hb_uint32 i = 0, slen = 0;

    if (!init_libthai())
        return HB_BasicShape (item);

    if (len >= 128)
        cstr = (char *)malloc(len*sizeof(char) + 1);

    if (!cstr)
        return HB_BasicShape (item);

    to_tis620(string, len, cstr);

    /* Get font type */
    static ThaiFontType font_type;
    static HB_Font itemFont;
    if (itemFont != item->font) {
        font_type = getThaiFontType (item);
        itemFont = item->font;
    }

    /* allocate temporary glyphs buffers */
    HB_STACKARRAY (HB_UChar16, glyphString, (item->item.length * 2));

    while (i < item->item.length) {
        struct thcell_t tis_cell;
        unsigned char rglyphs[4];
        int cell_length;
        int lgn = 0;
        HB_Bool haveSaraAm = false;

        cell_length = th_next_cell ((const unsigned char *)cstr + i, len - i, &tis_cell, true); /* !item->fixedPitch); */
        haveSaraAm  = (cstr[i + cell_length - 1] == (char)0xd3);

        /* set shaper item's log_clusters */
        logClusters[i] = slen;
        for (int j = 1; j < cell_length; j++) {
            logClusters[i + j] = logClusters[i];
        }

        /* Find Logical Glyphs by font type */
        switch (font_type) {
            case TIS: lgn = th_render_cell_tis (tis_cell, rglyphs, sizeof(rglyphs) / sizeof(rglyphs[0]), true); break;
            case WIN: lgn = th_render_cell_mac (tis_cell, rglyphs, sizeof(rglyphs) / sizeof(rglyphs[0]), true); break;
            case MAC: lgn = th_render_cell_win (tis_cell, rglyphs, sizeof(rglyphs) / sizeof(rglyphs[0]), true); break;
        }

        /* Add glyphs to glyphs string and setting some attributes */
        for (int lgi = 0; lgi < lgn; lgi++) {
            if ( rglyphs[lgi] == 0xdd/*TH_BLANK_BASE_GLYPH*/ ) {
                glyphString[slen++] = C_DOTTED_CIRCLE;
            } else if (cstr[i] == (signed char)~0) {
                // The only glyphs that should be passed to this function that cannot be mapped to
                // tis620 are the ones of type Inherited class.  Pass these glyphs untouched.
                glyphString[slen++] = string[i];
                if (string[i] == 0x200D || string[i] == 0x200C) {
                    // Check that we do not run out of bounds when setting item->attributes.  If we do
                    // run out of bounds then this function will return false, the necessary amount of
                    // memory is reallocated, and this function will then be called again.
                    if (slen <= item->num_glyphs)
                        item->attributes[slen-1].dontPrint = true; // Hide ZWJ and ZWNJ characters
                }
            } else {
                glyphString[slen++] = (HB_UChar16) thai_get_glyph_index (font_type, rglyphs[lgi]);
            }
        }

        /* Special case to handle U+0E33 (SARA AM, ำ): SARA AM is normally written at the end of a
         * word with a base character and an optional top character before it. For example, U+0E0B
         * (base), U+0E49 (top), U+0E33 (SARA AM). The sequence should be converted to 4 glyphs:
         * base, hilo (the little circle in the top left part of SARA AM, NIKHAHIT), top, then the
         * right part of SARA AM (SARA AA).
         *
         * The painting process finds out the starting glyph and ending glyph of a character
         * sequence by checking the logClusters array. In this case, logClusters array should
         * ideally be [ 0, 1, 3 ] so that glyphsStart = 0 and glyphsEnd = 3 (slen - 1) to paint out
         * all the glyphs generated.
         *
         * A special case in this special case is when we have no base character. When an isolated
         * SARA AM is processed (cell_length = 1), libthai will produce 3 glyphs: dotted circle
         * (indicates that the base is empty), NIKHAHIT then SARA AA. If logClusters[0] = 1, it will
         * paint from the second glyph in the glyphs array. So in this case logClusters[0] should
         * point to the first glyph it produces, aka. the dotted circle. */
        if (haveSaraAm) {
            logClusters[i + cell_length - 1] = cell_length == 1 ? slen - 3 : slen - 1;
            if (tis_cell.top != 0) {
                if (cell_length > 1) {
                    /* set the logClusters[top character] to slen - 2 as it points to the second to
                     * lastglyph (slen - 2) */
                    logClusters[i + cell_length - 2] = slen - 2;
                }
            }
            /* check for overflow */
            if (logClusters[i + cell_length - 1] > slen)
                logClusters[i + cell_length - 1] = 0;
        }

        i += cell_length;
    }
    glyphString[slen] = (HB_UChar16) '\0';

    /* for check, should reallocate space or not */
    HB_Bool spaceOK = (item->num_glyphs >= slen);

    /* Convert to Glyph indices */
    HB_Bool haveGlyphs = item->font->klass->convertStringToGlyphIndices (
                                          item->font,
                                          glyphString, slen,
                                          item->glyphs, &item->num_glyphs,
                                          item->shaperFlags);

    HB_FREE_STACKARRAY (glyphString);

    if (len >= 128)
        free(cstr);

    return (haveGlyphs && spaceOK);
}

/*
 * set the glyph attributes heuristically.
 */
static void HB_ThaiHeuristicSetGlyphAttributes (HB_ShaperItem *item)
{
    /* Set Glyph Attributes */
    hb_uint32 iCluster = 0;
    hb_uint32 length = item->item.length;
    while (iCluster < length) {
        int cluster_start = item->log_clusters[iCluster];
        ++iCluster;
        while (iCluster < length && item->log_clusters[iCluster] == cluster_start) {
            ++iCluster;
        }
        int cluster_end = (iCluster < length) ? item->log_clusters[iCluster] : item->num_glyphs;
        item->attributes[cluster_start].clusterStart = true;
        for (int i = cluster_start + 1; i < cluster_end; i++) {
            item->attributes[i].clusterStart = false;
        }
    }
}

/*
 * THAI Shaping.
 */
HB_Bool HB_ThaiShape (HB_ShaperItem *shaper_item)
{
    if ( !HB_ThaiConvertStringToGlyphIndices (shaper_item) )
        return false;

    HB_ThaiHeuristicSetGlyphAttributes (shaper_item);

#ifndef NO_OPENTYPE
    const int availableGlyphs = shaper_item->num_glyphs;
    if ( HB_SelectScript (shaper_item, thai_features) ) {
        HB_OpenTypeShape (shaper_item, /*properties*/0);
        return HB_OpenTypePosition (shaper_item, availableGlyphs, /*doLogClusters*/true);
    }
#endif

    HB_HeuristicPosition (shaper_item);
    return true;
}

/*
 * Thai Attributes: computes Word Break, Word Boundary and Char stop for THAI.
 */
static void HB_ThaiAssignAttributes(const HB_UChar16 *string, hb_uint32 len, HB_CharAttributes *attributes)
{
    char s[128];
    char *cstr = s;
    int *break_positions = 0;
    int brp[128];
    int brp_size = 0;
    hb_uint32 numbreaks, i, j, cell_length;
    struct thcell_t tis_cell;

    if (!init_libthai())
        return ;

    if (len >= 128)
        cstr = (char *)malloc(len*sizeof(char) + 1);

    to_tis620(string, len, cstr);

    for (i = 0; i < len; ++i) {
        attributes[i].lineBreakType = HB_NoBreak;
        attributes[i].wordBoundary = FALSE;
    }

    if (len > 128) {
        break_positions = (int*) malloc (sizeof(int) * len);
        memset (break_positions, 0, sizeof(int) * len);
        brp_size = len;
    }
    else {
        break_positions = brp;
        brp_size = 128;
    }

    if (break_positions) {
        attributes[0].wordBoundary = TRUE;
        numbreaks = th_brk((const unsigned char *)cstr, break_positions, brp_size);
        for (i = 0; i < numbreaks; ++i) {
            attributes[break_positions[i]].wordBoundary = TRUE;
            if (break_positions[i] > 0)
                attributes[break_positions[i]-1].lineBreakType = HB_Break;
        }

        if (break_positions != brp)
            free(break_positions);
    }

    /* manage charStop */
    i = 0;
    while (i < len) {
        cell_length = th_next_cell((const unsigned char *)cstr + i, len - i, &tis_cell, true);

        attributes[i].charStop = true;
        for (j = 1; j < cell_length; j++)
            attributes[i + j].charStop = false;

        /* Set charStop for SARA AM */
        if (cstr[i + cell_length - 1] == (char)0xd3)
            attributes[i + cell_length - 1].charStop = true;

        i += cell_length;
    }

    if (len >= 128)
        free(cstr);
}

void HB_ThaiAttributes(HB_Script script, const HB_UChar16 *text, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes)
{
    assert(script == HB_Script_Thai);
    const HB_UChar16 *uc = text + from;
    attributes += from;
    HB_UNUSED(script);
    HB_ThaiAssignAttributes(uc, len, attributes);
}

