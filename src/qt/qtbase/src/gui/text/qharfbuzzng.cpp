/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Konstantin Ritt
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qharfbuzzng_p.h"

#include <qstring.h>
#include <qvector.h>

#include <private/qstringiterator_p.h>

#include "qfontengine_p.h"

QT_BEGIN_NAMESPACE

// Unicode routines

static const hb_script_t _qtscript_to_hbscript[] = {
    HB_SCRIPT_UNKNOWN,
    HB_SCRIPT_INHERITED,
    HB_SCRIPT_COMMON,

    HB_SCRIPT_LATIN,
    HB_SCRIPT_GREEK,
    HB_SCRIPT_CYRILLIC,
    HB_SCRIPT_ARMENIAN,
    HB_SCRIPT_HEBREW,
    HB_SCRIPT_ARABIC,
    HB_SCRIPT_SYRIAC,
    HB_SCRIPT_THAANA,
    HB_SCRIPT_DEVANAGARI,
    HB_SCRIPT_BENGALI,
    HB_SCRIPT_GURMUKHI,
    HB_SCRIPT_GUJARATI,
    HB_SCRIPT_ORIYA,
    HB_SCRIPT_TAMIL,
    HB_SCRIPT_TELUGU,
    HB_SCRIPT_KANNADA,
    HB_SCRIPT_MALAYALAM,
    HB_SCRIPT_SINHALA,
    HB_SCRIPT_THAI,
    HB_SCRIPT_LAO,
    HB_SCRIPT_TIBETAN,
    HB_SCRIPT_MYANMAR,
    HB_SCRIPT_GEORGIAN,
    HB_SCRIPT_HANGUL,
    HB_SCRIPT_ETHIOPIC,
    HB_SCRIPT_CHEROKEE,
    HB_SCRIPT_CANADIAN_ABORIGINAL,
    HB_SCRIPT_OGHAM,
    HB_SCRIPT_RUNIC,
    HB_SCRIPT_KHMER,
    HB_SCRIPT_MONGOLIAN,
    HB_SCRIPT_HIRAGANA,
    HB_SCRIPT_KATAKANA,
    HB_SCRIPT_BOPOMOFO,
    HB_SCRIPT_HAN,
    HB_SCRIPT_YI,
    HB_SCRIPT_OLD_ITALIC,
    HB_SCRIPT_GOTHIC,
    HB_SCRIPT_DESERET,
    HB_SCRIPT_TAGALOG,
    HB_SCRIPT_HANUNOO,
    HB_SCRIPT_BUHID,
    HB_SCRIPT_TAGBANWA,
    HB_SCRIPT_COPTIC,
    HB_SCRIPT_LIMBU,
    HB_SCRIPT_TAI_LE,
    HB_SCRIPT_LINEAR_B,
    HB_SCRIPT_UGARITIC,
    HB_SCRIPT_SHAVIAN,
    HB_SCRIPT_OSMANYA,
    HB_SCRIPT_CYPRIOT,
    HB_SCRIPT_BRAILLE,
    HB_SCRIPT_BUGINESE,
    HB_SCRIPT_NEW_TAI_LUE,
    HB_SCRIPT_GLAGOLITIC,
    HB_SCRIPT_TIFINAGH,
    HB_SCRIPT_SYLOTI_NAGRI,
    HB_SCRIPT_OLD_PERSIAN,
    HB_SCRIPT_KHAROSHTHI,
    HB_SCRIPT_BALINESE,
    HB_SCRIPT_CUNEIFORM,
    HB_SCRIPT_PHOENICIAN,
    HB_SCRIPT_PHAGS_PA,
    HB_SCRIPT_NKO,
    HB_SCRIPT_SUNDANESE,
    HB_SCRIPT_LEPCHA,
    HB_SCRIPT_OL_CHIKI,
    HB_SCRIPT_VAI,
    HB_SCRIPT_SAURASHTRA,
    HB_SCRIPT_KAYAH_LI,
    HB_SCRIPT_REJANG,
    HB_SCRIPT_LYCIAN,
    HB_SCRIPT_CARIAN,
    HB_SCRIPT_LYDIAN,
    HB_SCRIPT_CHAM,
    HB_SCRIPT_TAI_THAM,
    HB_SCRIPT_TAI_VIET,
    HB_SCRIPT_AVESTAN,
    HB_SCRIPT_EGYPTIAN_HIEROGLYPHS,
    HB_SCRIPT_SAMARITAN,
    HB_SCRIPT_LISU,
    HB_SCRIPT_BAMUM,
    HB_SCRIPT_JAVANESE,
    HB_SCRIPT_MEETEI_MAYEK,
    HB_SCRIPT_IMPERIAL_ARAMAIC,
    HB_SCRIPT_OLD_SOUTH_ARABIAN,
    HB_SCRIPT_INSCRIPTIONAL_PARTHIAN,
    HB_SCRIPT_INSCRIPTIONAL_PAHLAVI,
    HB_SCRIPT_OLD_TURKIC,
    HB_SCRIPT_KAITHI,
    HB_SCRIPT_BATAK,
    HB_SCRIPT_BRAHMI,
    HB_SCRIPT_MANDAIC,
    HB_SCRIPT_CHAKMA,
    HB_SCRIPT_MEROITIC_CURSIVE,
    HB_SCRIPT_MEROITIC_HIEROGLYPHS,
    HB_SCRIPT_MIAO,
    HB_SCRIPT_SHARADA,
    HB_SCRIPT_SORA_SOMPENG,
    HB_SCRIPT_TAKRI
};
Q_STATIC_ASSERT(QChar::ScriptCount == sizeof(_qtscript_to_hbscript) / sizeof(_qtscript_to_hbscript[0]));

hb_script_t hb_qt_script_to_script(QChar::Script script)
{
    return _qtscript_to_hbscript[script];
}

QChar::Script hb_qt_script_from_script(hb_script_t script)
{
    uint i = QChar::ScriptCount - 1;
    while (i > QChar::Script_Unknown && _qtscript_to_hbscript[i] != script)
        --i;
    return QChar::Script(i);
}


static hb_unicode_combining_class_t
_hb_qt_unicode_combining_class(hb_unicode_funcs_t * /*ufuncs*/,
                               hb_codepoint_t unicode,
                               void * /*user_data*/)
{
    return hb_unicode_combining_class_t(QChar::combiningClass(unicode));
}

static unsigned int
_hb_qt_unicode_eastasian_width(hb_unicode_funcs_t * /*ufuncs*/,
                               hb_codepoint_t /*unicode*/,
                               void * /*user_data*/)
{
    qCritical("hb_qt_unicode_eastasian_width: not implemented!");
    return 1;
}

static const hb_unicode_general_category_t _qtcategory_to_hbcategory[] = {
    HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK,    //   Mn
    HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK,        //   Mc
    HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK,      //   Me

    HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER,      //   Nd
    HB_UNICODE_GENERAL_CATEGORY_LETTER_NUMBER,       //   Nl
    HB_UNICODE_GENERAL_CATEGORY_OTHER_NUMBER,        //   No

    HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR,     //   Zs
    HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR,      //   Zl
    HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR, //   Zp

    HB_UNICODE_GENERAL_CATEGORY_CONTROL,             //   Cc
    HB_UNICODE_GENERAL_CATEGORY_FORMAT,              //   Cf
    HB_UNICODE_GENERAL_CATEGORY_SURROGATE,           //   Cs
    HB_UNICODE_GENERAL_CATEGORY_PRIVATE_USE,         //   Co
    HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED,          //   Cn

    HB_UNICODE_GENERAL_CATEGORY_UPPERCASE_LETTER,    //   Lu
    HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER,    //   Ll
    HB_UNICODE_GENERAL_CATEGORY_TITLECASE_LETTER,    //   Lt
    HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER,     //   Lm
    HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER,        //   Lo

    HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION, //   Pc
    HB_UNICODE_GENERAL_CATEGORY_DASH_PUNCTUATION,    //   Pd
    HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION,    //   Ps
    HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION,   //   Pe
    HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION, //   Pi
    HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION,   //   Pf
    HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION,   //   Po

    HB_UNICODE_GENERAL_CATEGORY_MATH_SYMBOL,         //   Sm
    HB_UNICODE_GENERAL_CATEGORY_CURRENCY_SYMBOL,     //   Sc
    HB_UNICODE_GENERAL_CATEGORY_MODIFIER_SYMBOL,     //   Sk
    HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL         //   So
};

static hb_unicode_general_category_t
_hb_qt_unicode_general_category(hb_unicode_funcs_t * /*ufuncs*/,
                                hb_codepoint_t unicode,
                                void * /*user_data*/)
{
    return _qtcategory_to_hbcategory[QChar::category(unicode)];
}

static hb_codepoint_t
_hb_qt_unicode_mirroring(hb_unicode_funcs_t * /*ufuncs*/,
                         hb_codepoint_t unicode,
                         void * /*user_data*/)
{
    return QChar::mirroredChar(unicode);
}

static hb_script_t
_hb_qt_unicode_script(hb_unicode_funcs_t * /*ufuncs*/,
                      hb_codepoint_t unicode,
                      void * /*user_data*/)
{
    return _qtscript_to_hbscript[QChar::script(unicode)];
}

static hb_bool_t
_hb_qt_unicode_compose(hb_unicode_funcs_t * /*ufuncs*/,
                       hb_codepoint_t a, hb_codepoint_t b,
                       hb_codepoint_t *ab,
                       void * /*user_data*/)
{
    // ### optimize
    QString s = QString::fromUcs4(&a, 1) + QString::fromUcs4(&b, 1);
    QString normalized = s.normalized(QString::NormalizationForm_C);

    QStringIterator it(normalized);
    Q_ASSERT(it.hasNext()); // size>0
    *ab = it.next();

    return !it.hasNext(); // size==1
}

static hb_bool_t
_hb_qt_unicode_decompose(hb_unicode_funcs_t * /*ufuncs*/,
                         hb_codepoint_t ab,
                         hb_codepoint_t *a, hb_codepoint_t *b,
                         void * /*user_data*/)
{
    // ### optimize
    if (QChar::decompositionTag(ab) != QChar::Canonical) // !NFD
        return false;

    QString normalized = QChar::decomposition(ab);
    if (normalized.isEmpty())
        return false;

    QStringIterator it(normalized);
    Q_ASSERT(it.hasNext()); // size>0
    *a = it.next();

    if (!it.hasNext()) { // size==1
        *b = 0;
        return *a != ab;
    }

    // size>1
    *b = it.next();
    if (!it.hasNext()) { // size==2
        // Here's the ugly part: if ab decomposes to a single character and
        // that character decomposes again, we have to detect that and undo
        // the second part :-(
        const QString recomposed = normalized.normalized(QString::NormalizationForm_C);
        QStringIterator jt(recomposed);
        Q_ASSERT(jt.hasNext()); // size>0
        const hb_codepoint_t c = jt.next();
        if (c != *a && c != ab) {
            *a = c;
            *b = 0;
        }
        return true;
    }

    // size>2
    // If decomposed to more than two characters, take the last one,
    // and recompose the rest to get the first component
    do {
        *b = it.next();
    } while (it.hasNext());
    normalized.chop(QChar::requiresSurrogates(*b) ? 2 : 1);
    const QString recomposed = normalized.normalized(QString::NormalizationForm_C);
    QStringIterator jt(recomposed);
    Q_ASSERT(jt.hasNext()); // size>0
    // We expect that recomposed has exactly one character now
    *a = jt.next();
    return true;
}

static unsigned int
_hb_qt_unicode_decompose_compatibility(hb_unicode_funcs_t * /*ufuncs*/,
                                       hb_codepoint_t u,
                                       hb_codepoint_t *decomposed,
                                       void * /*user_data*/)
{
    const QString normalized = QChar::decomposition(u);

    uint outlen = 0;
    QStringIterator it(normalized);
    while (it.hasNext()) {
        Q_ASSERT(outlen < HB_UNICODE_MAX_DECOMPOSITION_LEN);
        decomposed[outlen++] = it.next();
    }

    return outlen;
}


struct _hb_unicode_funcs_t {
    _hb_unicode_funcs_t()
    {
        funcs = hb_unicode_funcs_create(NULL);
        hb_unicode_funcs_set_combining_class_func(funcs, _hb_qt_unicode_combining_class, NULL, NULL);
        hb_unicode_funcs_set_eastasian_width_func(funcs, _hb_qt_unicode_eastasian_width, NULL, NULL);
        hb_unicode_funcs_set_general_category_func(funcs, _hb_qt_unicode_general_category, NULL, NULL);
        hb_unicode_funcs_set_mirroring_func(funcs, _hb_qt_unicode_mirroring, NULL, NULL);
        hb_unicode_funcs_set_script_func(funcs, _hb_qt_unicode_script, NULL, NULL);
        hb_unicode_funcs_set_compose_func(funcs, _hb_qt_unicode_compose, NULL, NULL);
        hb_unicode_funcs_set_decompose_func(funcs, _hb_qt_unicode_decompose, NULL, NULL);
        hb_unicode_funcs_set_decompose_compatibility_func(funcs, _hb_qt_unicode_decompose_compatibility, NULL, NULL);
    }
    ~_hb_unicode_funcs_t()
    {
        hb_unicode_funcs_destroy(funcs);
    }

    hb_unicode_funcs_t *funcs;
};

Q_GLOBAL_STATIC(_hb_unicode_funcs_t, qt_ufuncs)

hb_unicode_funcs_t *hb_qt_get_unicode_funcs()
{
    return qt_ufuncs()->funcs;
}


// Font routines

static hb_bool_t
_hb_qt_font_get_glyph(hb_font_t * /*font*/, void *font_data,
                      hb_codepoint_t unicode, hb_codepoint_t /*variation_selector*/,
                      hb_codepoint_t *glyph,
                      void * /*user_data*/)
{
    QFontEngine *fe = (QFontEngine *)font_data;
    Q_ASSERT(fe);

    *glyph = fe->glyphIndex(unicode);

    return true;
}

static hb_position_t
_hb_qt_font_get_glyph_h_advance(hb_font_t *font, void *font_data,
                                hb_codepoint_t glyph,
                                void * /*user_data*/)
{
    QFontEngine *fe = (QFontEngine *)font_data;
    Q_ASSERT(fe);

    QFixed advance;

    QGlyphLayout g;
    g.numGlyphs = 1;
    g.glyphs = &glyph;
    g.advances = &advance;

    fe->recalcAdvances(&g, QFontEngine::ShaperFlags(hb_qt_font_get_use_design_metrics(font)));

    return advance.value();
}

static hb_position_t
_hb_qt_font_get_glyph_v_advance(hb_font_t * /*font*/, void * /*font_data*/,
                                hb_codepoint_t /*glyph*/,
                                void * /*user_data*/)
{
    qCritical("hb_qt_font_get_glyph_v_advance: vertical writing isn't supported!");
    return 0;
}

static hb_bool_t
_hb_qt_font_get_glyph_h_origin(hb_font_t * /*font*/, void * /*font_data*/,
                               hb_codepoint_t /*glyph*/,
                               hb_position_t * /*x*/, hb_position_t * /*y*/,
                               void * /*user_data*/)
{
    return true; // we always work in the horizontal coordinates
}

static hb_bool_t
_hb_qt_font_get_glyph_v_origin(hb_font_t * /*font*/, void * /*font_data*/,
                               hb_codepoint_t /*glyph*/,
                               hb_position_t * /*x*/, hb_position_t * /*y*/,
                               void * /*user_data*/)
{
    qCritical("hb_qt_get_glyph_v_origin: vertical writing isn't supported!");
    return false;
}

static hb_position_t
_hb_qt_font_get_glyph_h_kerning(hb_font_t *font, void *font_data,
                                hb_codepoint_t first_glyph, hb_codepoint_t second_glyph,
                                void * /*user_data*/)
{
    QFontEngine *fe = (QFontEngine *)font_data;
    Q_ASSERT(fe);

    glyph_t glyphs[2] = { first_glyph, second_glyph };
    QFixed advance;

    QGlyphLayout g;
    g.numGlyphs = 2;
    g.glyphs = glyphs;
    g.advances = &advance;

    fe->doKerning(&g, QFontEngine::ShaperFlags(hb_qt_font_get_use_design_metrics(font)));

    return advance.value();
}

static hb_position_t
_hb_qt_font_get_glyph_v_kerning(hb_font_t * /*font*/, void * /*font_data*/,
                                hb_codepoint_t /*first_glyph*/, hb_codepoint_t /*second_glyph*/,
                                void * /*user_data*/)
{
    qCritical("hb_qt_get_glyph_v_kerning: vertical writing isn't supported!");
    return 0;
}

static hb_bool_t
_hb_qt_font_get_glyph_extents(hb_font_t * /*font*/, void *font_data,
                              hb_codepoint_t glyph,
                              hb_glyph_extents_t *extents,
                              void * /*user_data*/)
{
    QFontEngine *fe = (QFontEngine *)font_data;
    Q_ASSERT(fe);

    glyph_metrics_t gm = fe->boundingBox(glyph);

    extents->x_bearing = gm.x.value();
    extents->y_bearing = gm.y.value();
    extents->width = gm.width.value();
    extents->height = gm.height.value();

    return true;
}

static hb_bool_t
_hb_qt_font_get_glyph_contour_point(hb_font_t * /*font*/, void *font_data,
                                    hb_codepoint_t glyph,
                                    unsigned int point_index, hb_position_t *x, hb_position_t *y,
                                    void * /*user_data*/)
{
    QFontEngine *fe = (QFontEngine *)font_data;
    Q_ASSERT(fe);

    QFixed xpos, ypos;
    quint32 numPoints = 1;
    if (Q_LIKELY(fe->getPointInOutline(glyph, 0, point_index, &xpos, &ypos, &numPoints) == 0)) {
        *x = xpos.value();
        *y = ypos.value();
        return true;
    }

    *x = *y = 0;
    return false;
}

static hb_bool_t
_hb_qt_font_get_glyph_name(hb_font_t * /*font*/, void * /*font_data*/,
                           hb_codepoint_t /*glyph*/,
                           char *name, unsigned int size,
                           void * /*user_data*/)
{
    qCritical("hb_qt_font_get_glyph_name: not implemented!");
    if (size)
        *name = '\0';
    return false;
}

static hb_bool_t
_hb_qt_font_get_glyph_from_name(hb_font_t * /*font*/, void * /*font_data*/,
                                const char * /*name*/, int /*len*/,
                                hb_codepoint_t *glyph,
                                void * /*user_data*/)
{
    qCritical("hb_qt_font_get_glyph_from_name: not implemented!");
    *glyph = 0;
    return false;
}


static hb_user_data_key_t _useDesignMetricsKey;

void hb_qt_font_set_use_design_metrics(hb_font_t *font, uint value)
{
    hb_font_set_user_data(font, &_useDesignMetricsKey, (void *)quintptr(value), NULL, true);
}

uint hb_qt_font_get_use_design_metrics(hb_font_t *font)
{
    return quintptr(hb_font_get_user_data(font, &_useDesignMetricsKey));
}


struct _hb_qt_font_funcs_t {
    _hb_qt_font_funcs_t()
    {
        funcs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_func(funcs, _hb_qt_font_get_glyph, NULL, NULL);
        hb_font_funcs_set_glyph_h_advance_func(funcs, _hb_qt_font_get_glyph_h_advance, NULL, NULL);
        hb_font_funcs_set_glyph_v_advance_func(funcs, _hb_qt_font_get_glyph_v_advance, NULL, NULL);
        hb_font_funcs_set_glyph_h_origin_func(funcs, _hb_qt_font_get_glyph_h_origin, NULL, NULL);
        hb_font_funcs_set_glyph_v_origin_func(funcs, _hb_qt_font_get_glyph_v_origin, NULL, NULL);
        hb_font_funcs_set_glyph_h_kerning_func(funcs, _hb_qt_font_get_glyph_h_kerning, NULL, NULL);
        hb_font_funcs_set_glyph_v_kerning_func(funcs, _hb_qt_font_get_glyph_v_kerning, NULL, NULL);
        hb_font_funcs_set_glyph_extents_func(funcs, _hb_qt_font_get_glyph_extents, NULL, NULL);
        hb_font_funcs_set_glyph_contour_point_func(funcs, _hb_qt_font_get_glyph_contour_point, NULL, NULL);
        hb_font_funcs_set_glyph_name_func(funcs, _hb_qt_font_get_glyph_name, NULL, NULL);
        hb_font_funcs_set_glyph_from_name_func(funcs, _hb_qt_font_get_glyph_from_name, NULL, NULL);
    }
    ~_hb_qt_font_funcs_t()
    {
        hb_font_funcs_destroy(funcs);
    }

    hb_font_funcs_t *funcs;
};

Q_GLOBAL_STATIC(_hb_qt_font_funcs_t, qt_ffuncs)

hb_font_funcs_t *hb_qt_get_font_funcs()
{
    return qt_ffuncs()->funcs;
}


static hb_blob_t *
_hb_qt_reference_table(hb_face_t * /*face*/, hb_tag_t tag, void *user_data)
{
    QFontEngine::FaceData *data = (QFontEngine::FaceData *)user_data;
    Q_ASSERT(data);

    qt_get_font_table_func_t get_font_table = data->get_font_table;
    Q_ASSERT(get_font_table);

    uint length = 0;
    if (Q_UNLIKELY(!get_font_table(data->user_data, tag, 0, &length)))
        return hb_blob_get_empty();

    char *buffer = (char *)malloc(length);
    Q_CHECK_PTR(buffer);

    if (Q_UNLIKELY(!get_font_table(data->user_data, tag, reinterpret_cast<uchar *>(buffer), &length)))
        length = 0;

    return hb_blob_create(const_cast<const char *>(buffer), length,
                          HB_MEMORY_MODE_WRITABLE,
                          buffer, free);
}

static inline hb_face_t *
_hb_qt_face_create(QFontEngine *fe)
{
    QFontEngine::FaceData *data = (QFontEngine::FaceData *)malloc(sizeof(QFontEngine::FaceData));
    Q_CHECK_PTR(data);
    data->user_data = fe->faceData.user_data;
    data->get_font_table = fe->faceData.get_font_table;

    hb_face_t *face = hb_face_create_for_tables(_hb_qt_reference_table, (void *)data, free);
    if (Q_UNLIKELY(hb_face_is_immutable(face))) {
        hb_face_destroy(face);
        return NULL;
    }

    hb_face_set_index(face, fe->faceId().index);
    hb_face_set_upem(face, fe->emSquareSize().truncate());

    return face;
}

static void
_hb_qt_face_release(void *user_data)
{
    if (Q_LIKELY(user_data))
        hb_face_destroy((hb_face_t *)user_data);
}

hb_face_t *hb_qt_face_get_for_engine(QFontEngine *fe)
{
    Q_ASSERT(fe && fe->type() != QFontEngine::Multi);

    if (Q_UNLIKELY(!fe->face_)) {
        fe->face_ = _hb_qt_face_create(fe);
        if (Q_UNLIKELY(!fe->face_))
            return NULL;
        fe->face_destroy_func = _hb_qt_face_release;
    }

    return hb_face_reference((hb_face_t *)fe->face_);
}


static inline hb_font_t *
_hb_qt_font_create(QFontEngine *fe)
{
    hb_face_t *face = hb_qt_face_get_for_engine(fe);
    if (Q_UNLIKELY(!face))
        return NULL;

    hb_font_t *font = hb_font_create(face);

    hb_face_destroy(face); // ref-ed in hb_qt_face_get_for_engine()

    if (Q_UNLIKELY(hb_font_is_immutable(font))) {
        hb_font_destroy(font);
        return NULL;
    }

    const int y_ppem = fe->fontDef.pixelSize;
    const int x_ppem = (fe->fontDef.pixelSize * fe->fontDef.stretch) / 100;

    hb_font_set_funcs(font, hb_qt_get_font_funcs(), (void *)fe, NULL);
#ifdef Q_OS_MAC
    hb_font_set_scale(font, QFixed(x_ppem).value(), QFixed(y_ppem).value());
#else
    hb_font_set_scale(font, QFixed(x_ppem).value(), -QFixed(y_ppem).value());
#endif
    hb_font_set_ppem(font, x_ppem, y_ppem);

    return font;
}

static void
_hb_qt_font_release(void *user_data)
{
    if (Q_LIKELY(user_data))
        hb_font_destroy((hb_font_t *)user_data);
}

hb_font_t *hb_qt_font_get_for_engine(QFontEngine *fe)
{
    Q_ASSERT(fe && fe->type() != QFontEngine::Multi);

    if (Q_UNLIKELY(!fe->font_)) {
        fe->font_ = _hb_qt_font_create(fe);
        if (Q_UNLIKELY(!fe->font_))
            return NULL;
        fe->font_destroy_func = _hb_qt_font_release;
    }

    return hb_font_reference((hb_font_t *)fe->font_);
}

QT_END_NAMESPACE
