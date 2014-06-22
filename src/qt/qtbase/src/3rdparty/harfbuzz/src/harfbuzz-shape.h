/*
 * Copyright (C) 2006  Red Hat, Inc.
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
 *
 * Red Hat Author(s): Owen Taylor
 */

#include <stdint.h>

/* Base Types */

typedef hb_uint16 HB_CodePoint; /* UTF-16 codepoint (not character ) */
typedef char HB_Boolean;
typedef hb_uint32 HB_Fixed; /* 26.6 */
typedef hb_uint32 HB_Glyph;
typedef hb_uint32 HB_Unichar;

/* Metrics reported by the font backend for use of the shaper */
typedef struct _HB_GlyphMetrics HB_GlyphMetrics;
struct _HB_GlyphMetrics
{
    HB_Fixed advance;
    
    /* Do we need ink/logical extents for the glyph here? */
};

/*
 * HB_Font: Abstract font interface.
 *  First pass of this might just have FT_Face *getFace();
 */
typedef struct _HB_Font HB_Font;
typedef struct _HB_FontClass HB_FontClass;

struct HB_FontClass {
    HB_Glyph   (*charToGlyph)(HB_Font *font, HB_Unichar chr);
    void       (*getMetrics)(HB_Font *font, HB_Glyph glyph, HB_GlyphMetrics *metrics);
    HB_Boolean (*getSFontTable)(HB_Font *font, void **cookie, char **start, int *len);
    HB_Boolean (*freeSFontTable)(void **cookie);
};

struct _HB_Font {
    HB_FontClass *clazz;
};

/*
 * Language tags, of the form en-us; represented as interned, canonicalized
 * strings. hb_language_from_string("en_US"), hb_language_from_string("en-us")
 * both return the same (pointer-comparable) HB_Language).
 */
typedef struct HB_Language_ *HB_Language;

HB_Language hb_language_from_string(const char *str);
const char *hb_language_to_string(HB_Language language);

/* Special treatment for the edges of runs.
 */
typedef enum {
    HB_RUN_EDGE_LINE_VISUAL_EDGE    = 1 << 0,
    HB_RUN_EDGE_LINE_LOGICAL_EDGE   = 1 << 1,
    HB_RUN_EDGE_LINE_ADD_HYPHEN     = 1 << 2  /* ???? */
} HB_RunEdge;

/* Defines optional informaiton in HB_ShapeInput; this allows extension
 * of HB_ShapeInput while keeping binary compatibility
 */
typedef enum {
    HB_SHAPE_START_TYPE = 1 << 0,
    HB_SHAPE_END_TYPE   = 1 << 1
} HB_ShapeFlags;

/* Attributes types are described by "interned strings"; this is a little
 * annoying if you want to write a switch statement, but keeps things
 * simple.
 */
typedef struct _HB_AttributeType *HB_AttributeType;

HB_AttributeType hb_attribute_type_from_string(const char *str);
const char *hb_attribute_type_to_string(HB_AttributeType attribute_type);

struct HB_Attribute {
    HB_AttributeType type;
    int start; 
    int end;
};


/**
 * You could handle this like HB_Language, but an enum seems a little nicer;
 * another approach would be to use OpenType script tags.
 */
typedef enum {
    HB_SCRIPT_LATIN
    /* ... */
} HB_ShapeScript;

/* This is just the subset of direction information needed by the shaper */
typedef enum {
    HB_DIRECTION_LTR,
    HB_DIRECTION_RTL,
    HB_DIRECTION_TTB
} HB_Direction;

typedef struct _HB_ShapeInput HB_ShapeInput;
struct _HB_ShapeInput {
    /* Defines what fields the caller has initialized - fields not in
     * the enum are mandatory.
     */
    HB_ShapeFlags flags;
    
    HB_CodePoint *text;
    int length;       /* total length of text to shape */
    int shape_offset; /* start of section to shape */
    int shape_length; /* number of code points to shape */

    HB_Direction direction;
    HB_ShapeScript script;
    HB_Language language;

    HB_AttributeType *attributes;
    int n_attributes;

    HB_RunEdge start_type;
    HB_RunEdge end_type;
};

struct HB_GlyphItem {
    HB_Glyph glyph;
    
    HB_Fixed x_offset;
    HB_Fixed y_offset;
    HB_Fixed advance;

    /* Add kashida information, etc, here */
};

typedef enum {
    HB_RESULT_SUCCESS,
    HB_RESULT_NO_MEMORY,
    HB_SHAPE_RESULT_FAILED
} HB_Result;

/*
 * Buffer for output 
 */
typedef struct _HB_GlyphBuffer HB_GlyphBuffer;
struct _HB_GlyphBuffer {
    int glyph_item_size;
    int total_glyphs;
    
    int *log_clusters; /* Uniscribe style */
    int cluster_space;
  
    int glyph_space;
    void *glyph_buffer;
};

/* Making this self-allocating simplifies writing shapers and
 * also keeps things easier for caller. item_size passed in
 * must be at least sizeof(HB_GlyphItem) but can be bigger,
 * to accomodate application structures that extend HB_GlyphItem.
 * The allocated items will be zero-initialized.
 *
 * (Hack: Harfbuzz could choose to use even a *bigger* item size
 * and stick internal information before the public item structure.
 * This hack could possibly be used to unify this with HB_Buffer)
 */
HB_GlyphBuffer *hb_glyph_buffer_new             (size_t item_size);
void            hb_glyph_buffer_clear           (HB_GlyphBuffer *buf);
HB_Result       hb_glyph_buffer_extend_glyphs   (HB_GlyphBuffer *buf, int n_items);
HB_Result       hb_glyph_buffer_extend_clusters (HB_GlyphBuffer *buf, int n_clusters);
void            hb_glyph_buffer_free            (HB_GlyphBuffer *buf);


/* Accessor for a particular glyph */
#define HB_GLYPH_BUFFER_ITEM(buffer, index)

/*
 * Main shaping function
 */
HB_Result hb_shape(HB_ShapeInput *input, HB_GlyphBuffer *output);
