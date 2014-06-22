/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QHARFBUZZ_P_H
#define QHARFBUZZ_P_H

#include <QtCore/qchar.h>

#if defined(QT_BUILD_CORE_LIB)
#  include <harfbuzz-shaper.h>
#else
// a minimal set of HB types required for Qt libs other than Core
extern "C" {

typedef enum {
  /* no error */
  HB_Err_Ok                           = 0x0000,
  HB_Err_Not_Covered                  = 0xFFFF,

  /* _hb_err() is called whenever returning the following errors,
   * and in a couple places for HB_Err_Not_Covered too. */

  /* programmer error */
  HB_Err_Invalid_Argument             = 0x1A66,

  /* font error */
  HB_Err_Invalid_SubTable_Format      = 0x157F,
  HB_Err_Invalid_SubTable             = 0x1570,
  HB_Err_Read_Error                   = 0x6EAD,

  /* system error */
  HB_Err_Out_Of_Memory                = 0xDEAD
} HB_Error;

typedef QT_PREPEND_NAMESPACE(qint8) hb_int8;
typedef QT_PREPEND_NAMESPACE(quint8) hb_uint8;
typedef QT_PREPEND_NAMESPACE(qint16) hb_int16;
typedef QT_PREPEND_NAMESPACE(quint16) hb_uint16;
typedef QT_PREPEND_NAMESPACE(qint32) hb_int32;
typedef QT_PREPEND_NAMESPACE(quint32) hb_uint32;

typedef hb_uint8 HB_Bool;
typedef hb_uint8 HB_Byte;
typedef hb_uint16 HB_UShort;
typedef hb_uint32 HB_UInt;
typedef hb_int8 HB_Char;
typedef hb_int16 HB_Short;
typedef hb_int32 HB_Int;
typedef hb_uint16 HB_UChar16;
typedef hb_uint32 HB_UChar32;
typedef hb_uint32 HB_Glyph;
typedef hb_int32 HB_Fixed; /* 26.6 */
typedef hb_int32 HB_16Dot16; /* 16.16 */
typedef hb_uint32 HB_Tag;

typedef struct {
    HB_Fixed x;
    HB_Fixed y;
} HB_FixedPoint;

typedef enum {
    HB_Script_Common,
    HB_Script_Greek,
    HB_Script_Cyrillic,
    HB_Script_Armenian,
    HB_Script_Hebrew,
    HB_Script_Arabic,
    HB_Script_Syriac,
    HB_Script_Thaana,
    HB_Script_Devanagari,
    HB_Script_Bengali,
    HB_Script_Gurmukhi,
    HB_Script_Gujarati,
    HB_Script_Oriya,
    HB_Script_Tamil,
    HB_Script_Telugu,
    HB_Script_Kannada,
    HB_Script_Malayalam,
    HB_Script_Sinhala,
    HB_Script_Thai,
    HB_Script_Lao,
    HB_Script_Tibetan,
    HB_Script_Myanmar,
    HB_Script_Georgian,
    HB_Script_Hangul,
    HB_Script_Ogham,
    HB_Script_Runic,
    HB_Script_Khmer,
    HB_Script_Nko,
    HB_Script_Inherited,
    HB_ScriptCount = HB_Script_Inherited
} HB_Script;

#ifdef  __xlC__
typedef unsigned hb_bitfield;
#else
typedef hb_uint8 hb_bitfield;
#endif

typedef struct {
    hb_bitfield justification   :4;  /* Justification class */
    hb_bitfield clusterStart    :1;  /* First glyph of representation of cluster */
    hb_bitfield mark            :1;  /* needs to be positioned around base char */
    hb_bitfield zeroWidth       :1;  /* ZWJ, ZWNJ etc, with no width */
    hb_bitfield dontPrint       :1;
    hb_bitfield combiningClass  :8;
} HB_GlyphAttributes;

typedef void * HB_GDEF;
typedef void * HB_GSUB;
typedef void * HB_GPOS;
typedef void * HB_Buffer;

typedef HB_Error (*HB_GetFontTableFunc)(void *font, HB_Tag tag, HB_Byte *buffer, HB_UInt *length);

typedef struct HB_FaceRec_ {
    HB_Bool isSymbolFont;

    HB_GDEF gdef;
    HB_GSUB gsub;
    HB_GPOS gpos;
    HB_Bool supported_scripts[HB_ScriptCount];
    HB_Buffer buffer;
    HB_Script current_script;
    int current_flags; /* HB_ShaperFlags */
    HB_Bool has_opentype_kerning;
    HB_Bool glyphs_substituted;
    HB_GlyphAttributes *tmpAttributes;
    unsigned int *tmpLogClusters;
    int length;
    int orig_nglyphs;
    void *font_for_init;
    HB_GetFontTableFunc get_font_table_func;
} HB_FaceRec;

typedef struct {
    HB_Fixed x, y;
    HB_Fixed width, height;
    HB_Fixed xOffset, yOffset;
} HB_GlyphMetrics;

typedef enum {
    HB_FontAscent
} HB_FontMetric;

struct HB_Font_;
typedef struct HB_Font_ *HB_Font;
typedef struct HB_FaceRec_ *HB_Face;

typedef struct {
    HB_Bool  (*convertStringToGlyphIndices)(HB_Font font, const HB_UChar16 *string, hb_uint32 length, HB_Glyph *glyphs, hb_uint32 *numGlyphs, HB_Bool rightToLeft);
    void     (*getGlyphAdvances)(HB_Font font, const HB_Glyph *glyphs, hb_uint32 numGlyphs, HB_Fixed *advances, int flags /*HB_ShaperFlag*/);
    HB_Bool  (*canRender)(HB_Font font, const HB_UChar16 *string, hb_uint32 length);
    /* implementation needs to make sure to load a scaled glyph, so /no/ FT_LOAD_NO_SCALE */
    HB_Error (*getPointInOutline)(HB_Font font, HB_Glyph glyph, int flags /*HB_ShaperFlag*/, hb_uint32 point, HB_Fixed *xpos, HB_Fixed *ypos, hb_uint32 *nPoints);
    void     (*getGlyphMetrics)(HB_Font font, HB_Glyph glyph, HB_GlyphMetrics *metrics);
    HB_Fixed (*getFontMetric)(HB_Font font, HB_FontMetric metric);
} HB_FontClass;

typedef struct HB_Font_ {
    const HB_FontClass *klass;

    /* Metrics */
    HB_UShort x_ppem, y_ppem;
    HB_16Dot16 x_scale, y_scale;

    void *userData;
} HB_FontRec;

typedef enum {
    HB_LeftToRight = 0,
    HB_RightToLeft = 1
} HB_StringToGlyphsFlags;

typedef enum {
    HB_ShaperFlag_Default = 0,
    HB_ShaperFlag_NoKerning = 1,
    HB_ShaperFlag_UseDesignMetrics = 2
} HB_ShaperFlag;

typedef struct
{
    hb_uint32 pos;
    hb_uint32 length;
    HB_Script script;
    hb_uint8 bidiLevel;
} HB_ScriptItem;

typedef struct HB_ShaperItem_ HB_ShaperItem;

struct HB_ShaperItem_ {
    const HB_UChar16 *string;               /* input: the Unicode UTF16 text to be shaped */
    hb_uint32 stringLength;                 /* input: the length of the input in 16-bit words */
    HB_ScriptItem item;                     /* input: the current run to be shaped: a run of text all in the same script that is a substring of <string> */
    HB_Font font;                           /* input: the font: scale, units and function pointers supplying glyph indices and metrics */
    HB_Face face;                           /* input: the shaper state; current script, access to the OpenType tables , etc. */
    int shaperFlags;                        /* input (unused) should be set to 0; intended to support flags defined in HB_ShaperFlag */
    HB_Bool glyphIndicesPresent;            /* input: true if the <glyphs> array contains glyph indices ready to be shaped */
    hb_uint32 initialGlyphCount;            /* input: if glyphIndicesPresent is true, the number of glyph indices in the <glyphs> array */

    hb_uint32 num_glyphs;                   /* input: capacity of output arrays <glyphs>, <attributes>, <advances>, <offsets>, and <log_clusters>; */
                                            /* output: required capacity (may be larger than actual capacity) */

    HB_Glyph *glyphs;                       /* output: <num_glyphs> indices of shaped glyphs */
    HB_GlyphAttributes *attributes;         /* output: <num_glyphs> glyph attributes */
    HB_Fixed *advances;                     /* output: <num_glyphs> advances */
    HB_FixedPoint *offsets;                 /* output: <num_glyphs> offsets */
    unsigned short *log_clusters;           /* output: for each output glyph, the index in the input of the start of its logical cluster */

    /* internal */
    HB_Bool kerning_applied;                /* output: true if kerning was applied by the shaper */
};

}

#endif // QT_BUILD_CORE_LIB


QT_BEGIN_NAMESPACE

static inline HB_Script script_to_hbscript(uchar script)
{
    switch (script) {
    case QChar::Script_Inherited: return HB_Script_Inherited;
    case QChar::Script_Common: return HB_Script_Common;
    case QChar::Script_Arabic: return HB_Script_Arabic;
    case QChar::Script_Armenian: return HB_Script_Armenian;
    case QChar::Script_Bengali: return HB_Script_Bengali;
    case QChar::Script_Cyrillic: return HB_Script_Cyrillic;
    case QChar::Script_Devanagari: return HB_Script_Devanagari;
    case QChar::Script_Georgian: return HB_Script_Georgian;
    case QChar::Script_Greek: return HB_Script_Greek;
    case QChar::Script_Gujarati: return HB_Script_Gujarati;
    case QChar::Script_Gurmukhi: return HB_Script_Gurmukhi;
    case QChar::Script_Hangul: return HB_Script_Hangul;
    case QChar::Script_Hebrew: return HB_Script_Hebrew;
    case QChar::Script_Kannada: return HB_Script_Kannada;
    case QChar::Script_Khmer: return HB_Script_Khmer;
    case QChar::Script_Lao: return HB_Script_Lao;
    case QChar::Script_Malayalam: return HB_Script_Malayalam;
    case QChar::Script_Myanmar: return HB_Script_Myanmar;
    case QChar::Script_Ogham: return HB_Script_Ogham;
    case QChar::Script_Oriya: return HB_Script_Oriya;
    case QChar::Script_Runic: return HB_Script_Runic;
    case QChar::Script_Sinhala: return HB_Script_Sinhala;
    case QChar::Script_Syriac: return HB_Script_Syriac;
    case QChar::Script_Tamil: return HB_Script_Tamil;
    case QChar::Script_Telugu: return HB_Script_Telugu;
    case QChar::Script_Thaana: return HB_Script_Thaana;
    case QChar::Script_Thai: return HB_Script_Thai;
    case QChar::Script_Tibetan: return HB_Script_Tibetan;
    case QChar::Script_Nko: return HB_Script_Nko;
    default: break;
    };
    return HB_Script_Common;
}

static inline uchar hbscript_to_script(uchar script)
{
    switch (script) {
    case HB_Script_Inherited: return QChar::Script_Inherited;
    case HB_Script_Common: return QChar::Script_Common;
    case HB_Script_Arabic: return QChar::Script_Arabic;
    case HB_Script_Armenian: return QChar::Script_Armenian;
    case HB_Script_Bengali: return QChar::Script_Bengali;
    case HB_Script_Cyrillic: return QChar::Script_Cyrillic;
    case HB_Script_Devanagari: return QChar::Script_Devanagari;
    case HB_Script_Georgian: return QChar::Script_Georgian;
    case HB_Script_Greek: return QChar::Script_Greek;
    case HB_Script_Gujarati: return QChar::Script_Gujarati;
    case HB_Script_Gurmukhi: return QChar::Script_Gurmukhi;
    case HB_Script_Hangul: return QChar::Script_Hangul;
    case HB_Script_Hebrew: return QChar::Script_Hebrew;
    case HB_Script_Kannada: return QChar::Script_Kannada;
    case HB_Script_Khmer: return QChar::Script_Khmer;
    case HB_Script_Lao: return QChar::Script_Lao;
    case HB_Script_Malayalam: return QChar::Script_Malayalam;
    case HB_Script_Myanmar: return QChar::Script_Myanmar;
    case HB_Script_Ogham: return QChar::Script_Ogham;
    case HB_Script_Oriya: return QChar::Script_Oriya;
    case HB_Script_Runic: return QChar::Script_Runic;
    case HB_Script_Sinhala: return QChar::Script_Sinhala;
    case HB_Script_Syriac: return QChar::Script_Syriac;
    case HB_Script_Tamil: return QChar::Script_Tamil;
    case HB_Script_Telugu: return QChar::Script_Telugu;
    case HB_Script_Thaana: return QChar::Script_Thaana;
    case HB_Script_Thai: return QChar::Script_Thai;
    case HB_Script_Tibetan: return QChar::Script_Tibetan;
    case HB_Script_Nko: return QChar::Script_Nko;
    default: break;
    };
    return QChar::Script_Common;
}

Q_CORE_EXPORT HB_Bool qShapeItem(HB_ShaperItem *item);

// ### temporary
Q_CORE_EXPORT HB_Face qHBNewFace(void *font, HB_GetFontTableFunc tableFunc);
Q_CORE_EXPORT void qHBFreeFace(HB_Face);
Q_CORE_EXPORT HB_Face qHBLoadFace(HB_Face face);

Q_DECLARE_TYPEINFO(HB_GlyphAttributes, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(HB_FixedPoint, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QHARFBUZZ_P_H
