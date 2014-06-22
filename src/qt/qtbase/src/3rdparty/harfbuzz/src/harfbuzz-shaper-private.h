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

#ifndef HARFBUZZ_SHAPER_PRIVATE_H
#define HARFBUZZ_SHAPER_PRIVATE_H

HB_BEGIN_HEADER

enum {
    C_DOTTED_CIRCLE = 0x25CC
};

typedef enum 
{
    HB_Combining_BelowLeftAttached       = 200,
    HB_Combining_BelowAttached           = 202,
    HB_Combining_BelowRightAttached      = 204,
    HB_Combining_LeftAttached            = 208,
    HB_Combining_RightAttached           = 210,
    HB_Combining_AboveLeftAttached       = 212,
    HB_Combining_AboveAttached           = 214,
    HB_Combining_AboveRightAttached      = 216,

    HB_Combining_BelowLeft               = 218,
    HB_Combining_Below                   = 220,
    HB_Combining_BelowRight              = 222,
    HB_Combining_Left                    = 224,
    HB_Combining_Right                   = 226,
    HB_Combining_AboveLeft               = 228,
    HB_Combining_Above                   = 230,
    HB_Combining_AboveRight              = 232,

    HB_Combining_DoubleBelow             = 233,
    HB_Combining_DoubleAbove             = 234,
    HB_Combining_IotaSubscript           = 240
} HB_CombiningClass;

typedef enum {
    LocaProperty = 0x1,
    CcmpProperty = 0x2,
    InitProperty = 0x4,
    IsolProperty = 0x8,
    FinaProperty = 0x10,
    MediProperty = 0x20,
    RligProperty = 0x40,
    CaltProperty = 0x80,
    LigaProperty = 0x100,
    DligProperty = 0x200,
    CswhProperty = 0x400,
    MsetProperty = 0x800,

    /* used by indic and myanmar shaper */
    NuktaProperty = 0x8,
    AkhantProperty = 0x10,
    RephProperty = 0x20,
    PreFormProperty = 0x40,
    BelowFormProperty = 0x80,
    AboveFormProperty = 0x100,
    HalfFormProperty = 0x200,
    PostFormProperty = 0x400,
    ConjunctFormProperty = 0x800,
    VattuProperty = 0x1000,
    PreSubstProperty = 0x2000,
    BelowSubstProperty = 0x4000,
    AboveSubstProperty = 0x8000,
    PostSubstProperty = 0x10000,
    HalantProperty = 0x20000,
    CligProperty = 0x40000,
    IndicCaltProperty = 0x80000

} HB_OpenTypeProperty;

/* return true if ok. */
typedef HB_Bool (*HB_ShapeFunction)(HB_ShaperItem *shaper_item);
typedef void (*HB_AttributeFunction)(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

typedef struct {
    HB_ShapeFunction shape;
    HB_AttributeFunction charAttributes;
} HB_ScriptEngine;

extern const HB_ScriptEngine hb_scriptEngines[];

extern HB_Bool HB_BasicShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_GreekShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_TibetanShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_HebrewShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_ArabicShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_HangulShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_MyanmarShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_KhmerShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_IndicShape(HB_ShaperItem *shaper_item);
extern HB_Bool HB_ThaiShape(HB_ShaperItem *shaper_item);

extern void HB_TibetanAttributes(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

extern void HB_MyanmarAttributes(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

extern void HB_KhmerAttributes(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

extern void HB_IndicAttributes(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

extern void HB_ThaiAttributes(HB_Script script, const HB_UChar16 *string, hb_uint32 from, hb_uint32 len, HB_CharAttributes *attributes);

#ifndef NO_OPENTYPE
typedef struct {
    hb_uint32 tag;
    hb_uint32 property;
} HB_OpenTypeFeature;

#define PositioningProperties 0x80000000

HB_Bool HB_SelectScript(HB_ShaperItem *item, const HB_OpenTypeFeature *features);

HB_Bool HB_OpenTypeShape(HB_ShaperItem *item, const hb_uint32 *properties);
HB_Bool HB_OpenTypePosition(HB_ShaperItem *item, int availableGlyphs, HB_Bool doLogClusters);
#endif // NO_OPENTYPE

void HB_HeuristicPosition(HB_ShaperItem *item);
void HB_HeuristicSetGlyphAttributes(HB_ShaperItem *item);

#define HB_IsControlChar(uc) \
    ((uc >= 0x200b && uc <= 0x200f /* ZW Space, ZWNJ, ZWJ, LRM and RLM */) \
     || (uc >= 0x2028 && uc <= 0x202e /* LS, PS, LRE, RLE, PDF, LRO, RLO */) \
     || (uc >= 0x206a && uc <= 0x206f /* ISS, ASS, IAFS, AFS, NADS, NODS */))

HB_Bool HB_ConvertStringToGlyphIndices(HB_ShaperItem *shaper_item);

#define HB_GetGlyphAdvances(shaper_item) \
    shaper_item->font->klass->getGlyphAdvances(shaper_item->font, \
                                               shaper_item->glyphs, shaper_item->num_glyphs, \
                                               shaper_item->advances, \
                                               shaper_item->face->current_flags);

#define HB_DECLARE_STACKARRAY(Type, Name) \
    Type stack##Name[512]; \
    Type *Name = stack##Name;

#define HB_INIT_STACKARRAY(Type, Name, Length) \
    if ((Length) >= 512) \
        Name = (Type *)malloc((Length) * sizeof(Type));

#define HB_STACKARRAY(Type, Name, Length) \
    HB_DECLARE_STACKARRAY(Type, Name) \
    HB_INIT_STACKARRAY(Type, Name, Length)

#define HB_FREE_STACKARRAY(Name) \
    if (stack##Name != Name) \
        free(Name);

HB_END_HEADER

#endif
