/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <qdebug.h>
#include <private/qfontengine_p.h>
#include <private/qfontengineglyphcache_p.h>

#include "qbitmap.h"
#include "qpainter.h"
#include "qpainterpath.h"
#include "qvarlengtharray.h"
#include <qmath.h>
#include <qendian.h>
#include <private/qstringiterator_p.h>

#ifdef QT_ENABLE_HARFBUZZ_NG
#  include "qharfbuzzng_p.h"
#  include <harfbuzz/hb-ot.h>
#endif
#include <private/qharfbuzz_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static inline bool qtransform_equals_no_translate(const QTransform &a, const QTransform &b)
{
    if (a.type() <= QTransform::TxTranslate && b.type() <= QTransform::TxTranslate) {
        return true;
    } else {
        // We always use paths for perspective text anyway, so no
        // point in checking the full matrix...
        Q_ASSERT(a.type() < QTransform::TxProject);
        Q_ASSERT(b.type() < QTransform::TxProject);

        return a.m11() == b.m11()
            && a.m12() == b.m12()
            && a.m21() == b.m21()
            && a.m22() == b.m22();
    }
}

// Harfbuzz helper functions

#ifdef QT_ENABLE_HARFBUZZ_NG
bool useHarfbuzzNG = qgetenv("QT_HARFBUZZ") != "old";
#endif

Q_STATIC_ASSERT(sizeof(HB_Glyph) == sizeof(glyph_t));
Q_STATIC_ASSERT(sizeof(HB_Fixed) == sizeof(QFixed));

static HB_Bool hb_stringToGlyphs(HB_Font font, const HB_UChar16 *string, hb_uint32 length, HB_Glyph *glyphs, hb_uint32 *numGlyphs, HB_Bool rightToLeft)
{
    QFontEngine *fe = (QFontEngine *)font->userData;

    const QChar *str = reinterpret_cast<const QChar *>(string);

    QGlyphLayout qglyphs;
    qglyphs.numGlyphs = *numGlyphs;
    qglyphs.glyphs = glyphs;
    int nGlyphs = *numGlyphs;
    bool result = fe->stringToCMap(str, length, &qglyphs, &nGlyphs, QFontEngine::GlyphIndicesOnly);
    *numGlyphs = nGlyphs;

    if (rightToLeft && result && !fe->symbol) {
        QStringIterator it(str, str + length);
        while (it.hasNext()) {
            const uint ucs4 = it.next();
            const uint mirrored = QChar::mirroredChar(ucs4);
            if (Q_UNLIKELY(mirrored != ucs4))
                *glyphs = fe->glyphIndex(mirrored);
            ++glyphs;
        }
    }

    return result;
}

static void hb_getAdvances(HB_Font font, const HB_Glyph *glyphs, hb_uint32 numGlyphs, HB_Fixed *advances, int flags)
{
    QFontEngine *fe = (QFontEngine *)font->userData;

    QGlyphLayout qglyphs;
    qglyphs.numGlyphs = numGlyphs;
    qglyphs.glyphs = const_cast<glyph_t *>(glyphs);
    qglyphs.advances = reinterpret_cast<QFixed *>(advances);

    fe->recalcAdvances(&qglyphs, (flags & HB_ShaperFlag_UseDesignMetrics) ? QFontEngine::DesignMetrics : QFontEngine::ShaperFlags(0));
}

static HB_Bool hb_canRender(HB_Font font, const HB_UChar16 *string, hb_uint32 length)
{
    QFontEngine *fe = (QFontEngine *)font->userData;
    return fe->canRender(reinterpret_cast<const QChar *>(string), length);
}

static void hb_getGlyphMetrics(HB_Font font, HB_Glyph glyph, HB_GlyphMetrics *metrics)
{
    QFontEngine *fe = (QFontEngine *)font->userData;
    glyph_metrics_t m = fe->boundingBox(glyph);
    metrics->x = m.x.value();
    metrics->y = m.y.value();
    metrics->width = m.width.value();
    metrics->height = m.height.value();
    metrics->xOffset = m.xoff.value();
    metrics->yOffset = m.yoff.value();
}

static HB_Fixed hb_getFontMetric(HB_Font font, HB_FontMetric metric)
{
    if (metric == HB_FontAscent) {
        QFontEngine *fe = (QFontEngine *)font->userData;
        return fe->ascent().value();
    }
    return 0;
}

int QFontEngine::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    Q_UNUSED(glyph)
    Q_UNUSED(flags)
    Q_UNUSED(point)
    Q_UNUSED(xpos)
    Q_UNUSED(ypos)
    Q_UNUSED(nPoints)
    return Err_Not_Covered;
}

static HB_Error hb_getPointInOutline(HB_Font font, HB_Glyph glyph, int flags, hb_uint32 point, HB_Fixed *xpos, HB_Fixed *ypos, hb_uint32 *nPoints)
{
    QFontEngine *fe = (QFontEngine *)font->userData;
    return (HB_Error)fe->getPointInOutline(glyph, flags, point, (QFixed *)xpos, (QFixed *)ypos, (quint32 *)nPoints);
}

static const HB_FontClass hb_fontClass = {
    hb_stringToGlyphs, hb_getAdvances, hb_canRender, hb_getPointInOutline,
    hb_getGlyphMetrics, hb_getFontMetric
};

static HB_Error hb_getSFntTable(void *font, HB_Tag tableTag, HB_Byte *buffer, HB_UInt *length)
{
    QFontEngine *fe = (QFontEngine *)font;
    Q_ASSERT(fe->faceData.get_font_table);
    if (!fe->faceData.get_font_table(fe->faceData.user_data, tableTag, buffer, length))
        return HB_Err_Invalid_Argument;
    return HB_Err_Ok;
}

static void hb_freeFace(void *face)
{
    qHBFreeFace((HB_Face)face);
}


static bool qt_get_font_table_default(void *user_data, uint tag, uchar *buffer, uint *length)
{
    QFontEngine *fe = (QFontEngine *)user_data;
    return fe->getSfntTableData(tag, buffer, length);
}


#ifdef QT_BUILD_INTERNAL
// for testing purpose only, not thread-safe!
static QList<QFontEngine *> *enginesCollector = 0;

Q_AUTOTEST_EXPORT void QFontEngine_startCollectingEngines()
{
    delete enginesCollector;
    enginesCollector = new QList<QFontEngine *>();
}

Q_AUTOTEST_EXPORT QList<QFontEngine *> QFontEngine_stopCollectingEngines()
{
    Q_ASSERT(enginesCollector);
    QList<QFontEngine *> ret = *enginesCollector;
    delete enginesCollector;
    enginesCollector = 0;
    return ret;
}
#endif // QT_BUILD_INTERNAL


// QFontEngine

QFontEngine::QFontEngine(Type type)
    : m_type(type), ref(0),
      font_(0), font_destroy_func(0),
      face_(0), face_destroy_func(0)
{
    faceData.user_data = this;
    faceData.get_font_table = qt_get_font_table_default;

    cache_cost = 0;
    fsType = 0;
    symbol = false;

    glyphFormat = Format_None;
    m_subPixelPositionCount = 0;

#ifdef QT_BUILD_INTERNAL
    if (enginesCollector)
        enginesCollector->append(this);
#endif
}

QFontEngine::~QFontEngine()
{
    m_glyphCaches.clear();

    if (font_ && font_destroy_func) {
        font_destroy_func(font_);
        font_ = 0;
    }
    if (face_ && face_destroy_func) {
        face_destroy_func(face_);
        face_ = 0;
    }

#ifdef QT_BUILD_INTERNAL
    if (enginesCollector)
        enginesCollector->removeOne(this);
#endif
}

QFixed QFontEngine::lineThickness() const
{
    // ad hoc algorithm
    int score = fontDef.weight * fontDef.pixelSize;
    int lw = score / 700;

    // looks better with thicker line for small pointsizes
    if (lw < 2 && score >= 1050) lw = 2;
    if (lw == 0) lw = 1;

    return lw;
}

QFixed QFontEngine::underlinePosition() const
{
    return ((lineThickness() * 2) + 3) / 6;
}

void *QFontEngine::harfbuzzFont() const
{
    Q_ASSERT(type() != QFontEngine::Multi);
#ifdef QT_ENABLE_HARFBUZZ_NG
    if (useHarfbuzzNG)
        return hb_qt_font_get_for_engine(const_cast<QFontEngine *>(this));
#endif
    if (!font_) {
        HB_Face hbFace = (HB_Face)harfbuzzFace();
        if (hbFace->font_for_init != 0)
            q_check_ptr(qHBLoadFace(hbFace));

        HB_FontRec *hbFont = (HB_FontRec *) malloc(sizeof(HB_FontRec));
        Q_CHECK_PTR(hbFont);
        hbFont->klass = &hb_fontClass;
        hbFont->userData = const_cast<QFontEngine *>(this);

        qint64 emSquare = emSquareSize().truncate();
        Q_ASSERT(emSquare == emSquareSize().toInt()); // ensure no truncation
        if (emSquare == 0)
            emSquare = 1000; // a fallback value suitable for Type1 fonts
        hbFont->y_ppem = fontDef.pixelSize;
        hbFont->x_ppem = fontDef.pixelSize * fontDef.stretch / 100;
        // same as QFixed(x)/QFixed(emSquare) but without int32 overflow for x
        hbFont->x_scale = (((qint64)hbFont->x_ppem << 6) * 0x10000L + (emSquare >> 1)) / emSquare;
        hbFont->y_scale = (((qint64)hbFont->y_ppem << 6) * 0x10000L + (emSquare >> 1)) / emSquare;

        font_ = (void *)hbFont;
        font_destroy_func = free;
    }
    return font_;
}

void *QFontEngine::harfbuzzFace() const
{
    Q_ASSERT(type() != QFontEngine::Multi);
#ifdef QT_ENABLE_HARFBUZZ_NG
    if (useHarfbuzzNG)
        return hb_qt_face_get_for_engine(const_cast<QFontEngine *>(this));
#endif
    if (!face_) {
        HB_Face hbFace = qHBNewFace(const_cast<QFontEngine *>(this), hb_getSFntTable);
        Q_CHECK_PTR(hbFace);
        hbFace->isSymbolFont = symbol;

        face_ = (void *)hbFace;
        face_destroy_func = hb_freeFace;
    }
    return face_;
}

bool QFontEngine::supportsScript(QChar::Script script) const
{
    if (type() <= QFontEngine::Multi)
        return true;

    // ### TODO: This only works for scripts that require OpenType. More generally
    // for scripts that do not require OpenType we should just look at the list of
    // supported writing systems in the font's OS/2 table.
    if (!((script >= QChar::Script_Syriac && script <= QChar::Script_Sinhala)
          || script == QChar::Script_Khmer || script == QChar::Script_Nko)) {
        return true;
    }

#ifdef Q_OS_MAC
    {
        // in AAT fonts, 'gsub' table is effectively replaced by 'mort'/'morx' table
        uint len;
        if (getSfntTableData(MAKE_TAG('m','o','r','t'), 0, &len) || getSfntTableData(MAKE_TAG('m','o','r','x'), 0, &len))
            return true;
    }
#endif

#ifdef QT_ENABLE_HARFBUZZ_NG
    if (useHarfbuzzNG) {
        bool ret = false;
        if (hb_face_t *face = hb_qt_face_get_for_engine(const_cast<QFontEngine *>(this))) {
            hb_tag_t script_tag_1, script_tag_2;
            hb_ot_tags_from_script(hb_qt_script_to_script(script), &script_tag_1, &script_tag_2);

            unsigned int script_index = -1;
            ret = hb_ot_layout_table_find_script(face, HB_OT_TAG_GSUB, script_tag_1, &script_index);
            if (!ret) {
                ret = hb_ot_layout_table_find_script(face, HB_OT_TAG_GSUB, script_tag_2, &script_index);
                if (!ret && script_tag_2 != HB_OT_TAG_DEFAULT_SCRIPT)
                    ret = hb_ot_layout_table_find_script(face, HB_OT_TAG_GSUB, HB_OT_TAG_DEFAULT_SCRIPT, &script_index);
            }

            hb_face_destroy(face);
        }
        return ret;
    }
#endif
    HB_Face hbFace = (HB_Face)harfbuzzFace();
    if (hbFace->font_for_init != 0)
        q_check_ptr(qHBLoadFace(hbFace));
    return hbFace->supported_scripts[script_to_hbscript(script)];
}

bool QFontEngine::canRender(const QChar *str, int len) const
{
    QStringIterator it(str, str + len);
    while (it.hasNext()) {
        if (glyphIndex(it.next()) == 0)
            return false;
    }

    return true;
}

glyph_metrics_t QFontEngine::boundingBox(glyph_t glyph, const QTransform &matrix)
{
    glyph_metrics_t metrics = boundingBox(glyph);

    if (matrix.type() > QTransform::TxTranslate) {
        return metrics.transformed(matrix);
    }
    return metrics;
}

QFixed QFontEngine::xHeight() const
{
    const glyph_t glyph = glyphIndex('x');
    glyph_metrics_t bb = const_cast<QFontEngine *>(this)->boundingBox(glyph);
    return bb.height;
}

QFixed QFontEngine::averageCharWidth() const
{
    const glyph_t glyph = glyphIndex('x');
    glyph_metrics_t bb = const_cast<QFontEngine *>(this)->boundingBox(glyph);
    return bb.xoff;
}

bool QFontEngine::supportsTransformation(const QTransform &transform) const
{
    return transform.type() < QTransform::TxProject;
}

void QFontEngine::getGlyphPositions(const QGlyphLayout &glyphs, const QTransform &matrix, QTextItem::RenderFlags flags,
                                    QVarLengthArray<glyph_t> &glyphs_out, QVarLengthArray<QFixedPoint> &positions)
{
    QFixed xpos;
    QFixed ypos;

    const bool transform = matrix.m11() != 1.
                           || matrix.m12() != 0.
                           || matrix.m21() != 0.
                           || matrix.m22() != 1.;
    if (!transform) {
        xpos = QFixed::fromReal(matrix.dx());
        ypos = QFixed::fromReal(matrix.dy());
    }

    int current = 0;
    if (flags & QTextItem::RightToLeft) {
        int i = glyphs.numGlyphs;
        int totalKashidas = 0;
        while(i--) {
            if (glyphs.attributes[i].dontPrint)
                continue;
            xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
            totalKashidas += glyphs.justifications[i].nKashidas;
        }
        positions.resize(glyphs.numGlyphs+totalKashidas);
        glyphs_out.resize(glyphs.numGlyphs+totalKashidas);

        i = 0;
        while(i < glyphs.numGlyphs) {
            if (glyphs.attributes[i].dontPrint) {
                ++i;
                continue;
            }
            xpos -= glyphs.advances[i];

            QFixed gpos_x = xpos + glyphs.offsets[i].x;
            QFixed gpos_y = ypos + glyphs.offsets[i].y;
            if (transform) {
                QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                gpos = gpos * matrix;
                gpos_x = QFixed::fromReal(gpos.x());
                gpos_y = QFixed::fromReal(gpos.y());
            }
            positions[current].x = gpos_x;
            positions[current].y = gpos_y;
            glyphs_out[current] = glyphs.glyphs[i];
            ++current;
            if (glyphs.justifications[i].nKashidas) {
                QChar ch(0x640); // Kashida character

                glyph_t kashidaGlyph = glyphIndex(ch.unicode());
                QFixed kashidaWidth;

                QGlyphLayout g;
                g.numGlyphs = 1;
                g.glyphs = &kashidaGlyph;
                g.advances = &kashidaWidth;
                recalcAdvances(&g, 0);

                for (uint k = 0; k < glyphs.justifications[i].nKashidas; ++k) {
                    xpos -= kashidaWidth;

                    QFixed gpos_x = xpos + glyphs.offsets[i].x;
                    QFixed gpos_y = ypos + glyphs.offsets[i].y;
                    if (transform) {
                        QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                        gpos = gpos * matrix;
                        gpos_x = QFixed::fromReal(gpos.x());
                        gpos_y = QFixed::fromReal(gpos.y());
                    }
                    positions[current].x = gpos_x;
                    positions[current].y = gpos_y;
                    glyphs_out[current] = kashidaGlyph;
                    ++current;
                }
            } else {
                xpos -= QFixed::fromFixed(glyphs.justifications[i].space_18d6);
            }
            ++i;
        }
    } else {
        positions.resize(glyphs.numGlyphs);
        glyphs_out.resize(glyphs.numGlyphs);
        int i = 0;
        if (!transform) {
            while (i < glyphs.numGlyphs) {
                if (!glyphs.attributes[i].dontPrint) {
                    positions[current].x = xpos + glyphs.offsets[i].x;
                    positions[current].y = ypos + glyphs.offsets[i].y;
                    glyphs_out[current] = glyphs.glyphs[i];
                    xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
                    ++current;
                }
                ++i;
            }
        } else {
            while (i < glyphs.numGlyphs) {
                if (!glyphs.attributes[i].dontPrint) {
                    QFixed gpos_x = xpos + glyphs.offsets[i].x;
                    QFixed gpos_y = ypos + glyphs.offsets[i].y;
                    QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                    gpos = gpos * matrix;
                    positions[current].x = QFixed::fromReal(gpos.x());
                    positions[current].y = QFixed::fromReal(gpos.y());
                    glyphs_out[current] = glyphs.glyphs[i];
                    xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
                    ++current;
                }
                ++i;
            }
        }
    }
    positions.resize(current);
    glyphs_out.resize(current);
    Q_ASSERT(positions.size() == glyphs_out.size());
}

void QFontEngine::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    glyph_metrics_t gi = boundingBox(glyph);
    bool isValid = gi.isValid();
    if (leftBearing != 0)
        *leftBearing = isValid ? gi.x.toReal() : 0.0;
    if (rightBearing != 0)
        *rightBearing = isValid ? (gi.xoff - gi.x - gi.width).toReal() : 0.0;
}

glyph_metrics_t QFontEngine::tightBoundingBox(const QGlyphLayout &glyphs)
{
    glyph_metrics_t overall;

    QFixed ymax = 0;
    QFixed xmax = 0;
    for (int i = 0; i < glyphs.numGlyphs; i++) {
        glyph_metrics_t bb = boundingBox(glyphs.glyphs[i]);
        QFixed x = overall.xoff + glyphs.offsets[i].x + bb.x;
        QFixed y = overall.yoff + glyphs.offsets[i].y + bb.y;
        overall.x = qMin(overall.x, x);
        overall.y = qMin(overall.y, y);
        xmax = qMax(xmax, x + bb.width);
        ymax = qMax(ymax, y + bb.height);
        overall.xoff += bb.xoff;
        overall.yoff += bb.yoff;
    }
    overall.height = qMax(overall.height, ymax - overall.y);
    overall.width = xmax - overall.x;

    return overall;
}


void QFontEngine::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path,
                                   QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> positioned_glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y);
    getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);
    addGlyphsToPath(positioned_glyphs.data(), positions.data(), positioned_glyphs.size(), path, flags);
}

#define GRID(x, y) grid[(y)*(w+1) + (x)]
#define SET(x, y) (*(image_data + (y)*bpl + ((x) >> 3)) & (0x80 >> ((x) & 7)))

enum { EdgeRight = 0x1,
       EdgeDown = 0x2,
       EdgeLeft = 0x4,
       EdgeUp = 0x8
};

static void collectSingleContour(qreal x0, qreal y0, uint *grid, int x, int y, int w, int h, QPainterPath *path)
{
    Q_UNUSED(h);

    path->moveTo(x + x0, y + y0);
    while (GRID(x, y)) {
        if (GRID(x, y) & EdgeRight) {
            while (GRID(x, y) & EdgeRight) {
                GRID(x, y) &= ~EdgeRight;
                ++x;
            }
            Q_ASSERT(x <= w);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeDown) {
            while (GRID(x, y) & EdgeDown) {
                GRID(x, y) &= ~EdgeDown;
                ++y;
            }
            Q_ASSERT(y <= h);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeLeft) {
            while (GRID(x, y) & EdgeLeft) {
                GRID(x, y) &= ~EdgeLeft;
                --x;
            }
            Q_ASSERT(x >= 0);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeUp) {
            while (GRID(x, y) & EdgeUp) {
                GRID(x, y) &= ~EdgeUp;
                --y;
            }
            Q_ASSERT(y >= 0);
            path->lineTo(x + x0, y + y0);
            continue;
        }
    }
    path->closeSubpath();
}

Q_GUI_EXPORT void qt_addBitmapToPath(qreal x0, qreal y0, const uchar *image_data, int bpl, int w, int h, QPainterPath *path)
{
    uint *grid = new uint[(w+1)*(h+1)];
    // set up edges
    for (int y = 0; y <= h; ++y) {
        for (int x = 0; x <= w; ++x) {
            bool topLeft = (x == 0)|(y == 0) ? false : SET(x - 1, y - 1);
            bool topRight = (x == w)|(y == 0) ? false : SET(x, y - 1);
            bool bottomLeft = (x == 0)|(y == h) ? false : SET(x - 1, y);
            bool bottomRight = (x == w)|(y == h) ? false : SET(x, y);

            GRID(x, y) = 0;
            if ((!topRight) & bottomRight)
                GRID(x, y) |= EdgeRight;
            if ((!bottomRight) & bottomLeft)
                GRID(x, y) |= EdgeDown;
            if ((!bottomLeft) & topLeft)
                GRID(x, y) |= EdgeLeft;
            if ((!topLeft) & topRight)
                GRID(x, y) |= EdgeUp;
        }
    }

    // collect edges
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!GRID(x, y))
                continue;
            // found start of a contour, follow it
            collectSingleContour(x0, y0, grid, x, y, w, h, path);
        }
    }
    delete [] grid;
}

#undef GRID
#undef SET


void QFontEngine::addBitmapFontToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                      QPainterPath *path, QTextItem::RenderFlags flags)
{
// TODO what to do with 'flags' ??
    Q_UNUSED(flags);
    QFixed advanceX = QFixed::fromReal(x);
    QFixed advanceY = QFixed::fromReal(y);
    for (int i=0; i < glyphs.numGlyphs; ++i) {
        glyph_metrics_t metrics = boundingBox(glyphs.glyphs[i]);
        if (metrics.width.value() == 0 || metrics.height.value() == 0) {
            advanceX += glyphs.advances[i];
            continue;
        }
        const QImage alphaMask = alphaMapForGlyph(glyphs.glyphs[i]);

        const int w = alphaMask.width();
        const int h = alphaMask.height();
        const int srcBpl = alphaMask.bytesPerLine();
        QImage bitmap;
        if (alphaMask.depth() == 1) {
            bitmap = alphaMask;
        } else {
            bitmap = QImage(w, h, QImage::Format_Mono);
            const uchar *imageData = alphaMask.bits();
            const int destBpl = bitmap.bytesPerLine();
            uchar *bitmapData = bitmap.bits();

            for (int yi = 0; yi < h; ++yi) {
                const uchar *src = imageData + yi*srcBpl;
                uchar *dst = bitmapData + yi*destBpl;
                for (int xi = 0; xi < w; ++xi) {
                    const int byte = xi / 8;
                    const int bit = xi % 8;
                    if (bit == 0)
                        dst[byte] = 0;
                    if (src[xi])
                        dst[byte] |= 128 >> bit;
                }
            }
        }
        const uchar *bitmap_data = bitmap.bits();
        QFixedPoint offset = glyphs.offsets[i];
        advanceX += offset.x;
        advanceY += offset.y;
        qt_addBitmapToPath((advanceX + metrics.x).toReal(), (advanceY + metrics.y).toReal(), bitmap_data, bitmap.bytesPerLine(), w, h, path);
        advanceX += glyphs.advances[i];
    }
}

void QFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nGlyphs,
                                  QPainterPath *path, QTextItem::RenderFlags flags)
{
    qreal x = positions[0].x.toReal();
    qreal y = positions[0].y.toReal();
    QVarLengthGlyphLayoutArray g(nGlyphs);

    for (int i = 0; i < nGlyphs - 1; ++i) {
        g.glyphs[i] = glyphs[i];
        g.advances[i] = positions[i + 1].x - positions[i].x;
    }
    g.glyphs[nGlyphs - 1] = glyphs[nGlyphs - 1];
    g.advances[nGlyphs - 1] = QFixed::fromReal(maxCharWidth());

    addBitmapFontToPath(x, y, g, path, flags);
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed /*subPixelPosition*/)
{
    // For font engines don't support subpixel positioning
    return alphaMapForGlyph(glyph);
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, const QTransform &t)
{
    QImage i = alphaMapForGlyph(glyph);
    if (t.type() > QTransform::TxTranslate)
        i = i.transformed(t).convertToFormat(QImage::Format_Indexed8);
    Q_ASSERT(i.depth() <= 8); // To verify that transformed didn't change the format...

    return i;
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t)
{
    if (! supportsSubPixelPositions())
        return alphaMapForGlyph(glyph, t);

    QImage i = alphaMapForGlyph(glyph, subPixelPosition);
    if (t.type() > QTransform::TxTranslate)
        i = i.transformed(t).convertToFormat(QImage::Format_Indexed8);
    Q_ASSERT(i.depth() <= 8); // To verify that transformed didn't change the format...

    return i;
}

QImage QFontEngine::alphaRGBMapForGlyph(glyph_t glyph, QFixed /*subPixelPosition*/, const QTransform &t)
{
    QImage alphaMask = alphaMapForGlyph(glyph, t);
    QImage rgbMask(alphaMask.width(), alphaMask.height(), QImage::Format_RGB32);

    QVector<QRgb> colorTable = alphaMask.colorTable();
    for (int y=0; y<alphaMask.height(); ++y) {
        uint *dst = (uint *) rgbMask.scanLine(y);
        uchar *src = (uchar *) alphaMask.scanLine(y);
        for (int x=0; x<alphaMask.width(); ++x) {
            int val = qAlpha(colorTable.at(src[x]));
            dst[x] = qRgb(val, val, val);
        }
    }

    return rgbMask;
}

QImage QFontEngine::bitmapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform&)
{
    Q_UNUSED(subPixelPosition);

    return QImage();
}

QFixed QFontEngine::subPixelPositionForX(QFixed x) const
{
    if (m_subPixelPositionCount <= 1 || !supportsSubPixelPositions())
        return QFixed();

    QFixed subPixelPosition;
    if (x != 0) {
        subPixelPosition = x - x.floor();
        QFixed fraction = (subPixelPosition / QFixed::fromReal(1.0 / m_subPixelPositionCount)).floor();

        // Compensate for precision loss in fixed point to make sure we are always drawing at a subpixel position over
        // the lower boundary for the selected rasterization by adding 1/64.
        subPixelPosition = fraction / QFixed(m_subPixelPositionCount) + QFixed::fromReal(0.015625);
    }
    return subPixelPosition;
}

QImage *QFontEngine::lockedAlphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition,
                                            QFontEngine::GlyphFormat neededFormat,
                                            const QTransform &t, QPoint *offset)
{
    Q_ASSERT(currentlyLockedAlphaMap.isNull());
    if (neededFormat == Format_None)
        neededFormat = Format_A32;

    if (neededFormat != Format_A32)
        currentlyLockedAlphaMap = alphaMapForGlyph(glyph, subPixelPosition, t);
    else
        currentlyLockedAlphaMap = alphaRGBMapForGlyph(glyph, subPixelPosition, t);

    if (offset != 0)
        *offset = QPoint(0, 0);

    return &currentlyLockedAlphaMap;
}

void QFontEngine::unlockAlphaMapForGlyph()
{
    Q_ASSERT(!currentlyLockedAlphaMap.isNull());
    currentlyLockedAlphaMap = QImage();
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph)
{
    glyph_metrics_t gm = boundingBox(glyph);
    int glyph_x = qFloor(gm.x.toReal());
    int glyph_y = qFloor(gm.y.toReal());
    int glyph_width = qCeil((gm.x + gm.width).toReal()) -  glyph_x;
    int glyph_height = qCeil((gm.y + gm.height).toReal()) - glyph_y;

    if (glyph_width <= 0 || glyph_height <= 0)
        return QImage();
    QFixedPoint pt;
    pt.x = -glyph_x;
    pt.y = -glyph_y; // the baseline
    QPainterPath path;
    QImage im(glyph_width + 4, glyph_height, QImage::Format_ARGB32_Premultiplied);
    im.fill(Qt::transparent);
    QPainter p(&im);
    p.setRenderHint(QPainter::Antialiasing);
    addGlyphsToPath(&glyph, &pt, 1, &path, 0);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.end();

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uchar *dst = (uchar *) indexed.scanLine(y);
        uint *src = (uint *) im.scanLine(y);
        for (int x=0; x<im.width(); ++x)
            dst[x] = qAlpha(src[x]);
    }

    return indexed;
}

void QFontEngine::removeGlyphFromCache(glyph_t)
{
}

QFontEngine::Properties QFontEngine::properties() const
{
    Properties p;
    QByteArray psname = QFontEngine::convertToPostscriptFontFamilyName(fontDef.family.toUtf8());
    psname += '-';
    psname += QByteArray::number(fontDef.style);
    psname += '-';
    psname += QByteArray::number(fontDef.weight);

    p.postscriptName = psname;
    p.ascent = ascent();
    p.descent = descent();
    p.leading = leading();
    p.emSquare = p.ascent;
    p.boundingBox = QRectF(0, -p.ascent.toReal(), maxCharWidth(), (p.ascent + p.descent).toReal());
    p.italicAngle = 0;
    p.capHeight = p.ascent;
    p.lineWidth = lineThickness();
    return p;
}

void QFontEngine::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    *metrics = boundingBox(glyph);
    QFixedPoint p;
    p.x = 0;
    p.y = 0;
    addGlyphsToPath(&glyph, &p, 1, path, QFlag(0));
}

/*!
    Returns \c true if the font table idetified by \a tag exists in the font;
    returns \c false otherwise.

    If \a buffer is NULL, stores the size of the buffer required for the font table data,
    in bytes, in \a length. If \a buffer is not NULL and the capacity
    of the buffer, passed in \a length, is sufficient to store the font table data,
    also copies the font table data to \a buffer.

    Note: returning \c false when the font table exists could lead to an undefined behavior.
*/
bool QFontEngine::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    Q_UNUSED(tag)
    Q_UNUSED(buffer)
    Q_UNUSED(length)
    return false;
}

QByteArray QFontEngine::getSfntTable(uint tag) const
{
    QByteArray table;
    uint len = 0;
    if (!getSfntTableData(tag, 0, &len))
        return table;
    table.resize(len);
    if (!getSfntTableData(tag, reinterpret_cast<uchar *>(table.data()), &len))
        return QByteArray();
    return table;
}

void QFontEngine::clearGlyphCache(const void *key)
{
    for (QLinkedList<GlyphCacheEntry>::iterator it = m_glyphCaches.begin(), end = m_glyphCaches.end(); it != end; ) {
        if (it->context == key)
            it = m_glyphCaches.erase(it);
        else
            ++it;
    }
}

void QFontEngine::setGlyphCache(const void *key, QFontEngineGlyphCache *data)
{
    Q_ASSERT(data);

    GlyphCacheEntry entry;
    entry.context = key;
    entry.cache = data;
    if (m_glyphCaches.contains(entry))
        return;

    // Limit the glyph caches to 4. This covers all 90 degree rotations and limits
    // memory use when there is continuous or random rotation
    if (m_glyphCaches.size() == 4)
        m_glyphCaches.removeLast();

    m_glyphCaches.push_front(entry);

}

QFontEngineGlyphCache *QFontEngine::glyphCache(const void *key, GlyphFormat format, const QTransform &transform) const
{
    for (QLinkedList<GlyphCacheEntry>::const_iterator it = m_glyphCaches.constBegin(), end = m_glyphCaches.constEnd(); it != end; ++it) {
        QFontEngineGlyphCache *c = it->cache.data();
        if (key == it->context
            && format == c->glyphFormat()
            && qtransform_equals_no_translate(c->m_transform, transform)) {
            return c;
        }
    }
    return 0;
}

static inline QFixed kerning(int left, int right, const QFontEngine::KernPair *pairs, int numPairs)
{
    uint left_right = (left << 16) + right;

    left = 0, right = numPairs - 1;
    while (left <= right) {
        int middle = left + ( ( right - left ) >> 1 );

        if(pairs[middle].left_right == left_right)
            return pairs[middle].adjust;

        if (pairs[middle].left_right < left_right)
            left = middle + 1;
        else
            right = middle - 1;
    }
    return 0;
}

void QFontEngine::doKerning(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    int numPairs = kerning_pairs.size();
    if(!numPairs)
        return;

    const KernPair *pairs = kerning_pairs.constData();

    if (flags & DesignMetrics) {
        for(int i = 0; i < glyphs->numGlyphs - 1; ++i)
            glyphs->advances[i] += kerning(glyphs->glyphs[i], glyphs->glyphs[i+1] , pairs, numPairs);
    } else {
        for(int i = 0; i < glyphs->numGlyphs - 1; ++i)
            glyphs->advances[i] += qRound(kerning(glyphs->glyphs[i], glyphs->glyphs[i+1] , pairs, numPairs));
    }
}

void QFontEngine::loadKerningPairs(QFixed scalingFactor)
{
    kerning_pairs.clear();

    QByteArray tab = getSfntTable(MAKE_TAG('k', 'e', 'r', 'n'));
    if (tab.isEmpty())
        return;

    const uchar *table = reinterpret_cast<const uchar *>(tab.constData());

    unsigned short version = qFromBigEndian<quint16>(table);
    if (version != 0) {
//        qDebug("wrong version");
       return;
    }

    unsigned short numTables = qFromBigEndian<quint16>(table + 2);
    {
        int offset = 4;
        for(int i = 0; i < numTables; ++i) {
            if (offset + 6 > tab.size()) {
//                qDebug("offset out of bounds");
                goto end;
            }
            const uchar *header = table + offset;

            ushort version = qFromBigEndian<quint16>(header);
            ushort length = qFromBigEndian<quint16>(header+2);
            ushort coverage = qFromBigEndian<quint16>(header+4);
//            qDebug("subtable: version=%d, coverage=%x",version, coverage);
            if(version == 0 && coverage == 0x0001) {
                if (offset + length > tab.size()) {
//                    qDebug("length ouf ot bounds");
                    goto end;
                }
                const uchar *data = table + offset + 6;

                ushort nPairs = qFromBigEndian<quint16>(data);
                if(nPairs * 6 + 8 > length - 6) {
//                    qDebug("corrupt table!");
                    // corrupt table
                    goto end;
                }

                int off = 8;
                for(int i = 0; i < nPairs; ++i) {
                    QFontEngine::KernPair p;
                    p.left_right = (((uint)qFromBigEndian<quint16>(data+off)) << 16) + qFromBigEndian<quint16>(data+off+2);
                    p.adjust = QFixed(((int)(short)qFromBigEndian<quint16>(data+off+4))) / scalingFactor;
                    kerning_pairs.append(p);
                    off += 6;
                }
            }
            offset += length;
        }
    }
end:
    std::sort(kerning_pairs.begin(), kerning_pairs.end());
//    for (int i = 0; i < kerning_pairs.count(); ++i)
//        qDebug() << 'i' << i << "left_right" << hex << kerning_pairs.at(i).left_right;
}


int QFontEngine::glyphCount() const
{
    QByteArray maxpTable = getSfntTable(MAKE_TAG('m', 'a', 'x', 'p'));
    if (maxpTable.size() < 6)
        return 0;
    return qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(maxpTable.constData() + 4));
}

const uchar *QFontEngine::getCMap(const uchar *table, uint tableSize, bool *isSymbolFont, int *cmapSize)
{
    const uchar *header = table;
    if (tableSize < 4)
        return 0;

    const uchar *endPtr = table + tableSize;

    // version check
    if (qFromBigEndian<quint16>(header) != 0)
        return 0;

    unsigned short numTables = qFromBigEndian<quint16>(header + 2);
    const uchar *maps = table + 4;
    if (maps + 8 * numTables > endPtr)
        return 0;

    enum {
        Invalid,
        AppleRoman,
        Symbol,
        Unicode11,
        Unicode,
        MicrosoftUnicode,
        MicrosoftUnicodeExtended
    };

    int symbolTable = -1;
    int tableToUse = -1;
    int score = Invalid;
    for (int n = 0; n < numTables; ++n) {
        const quint16 platformId = qFromBigEndian<quint16>(maps + 8 * n);
        const quint16 platformSpecificId = qFromBigEndian<quint16>(maps + 8 * n + 2);
        switch (platformId) {
        case 0: // Unicode
            if (score < Unicode &&
                (platformSpecificId == 0 ||
                 platformSpecificId == 2 ||
                 platformSpecificId == 3)) {
                tableToUse = n;
                score = Unicode;
            } else if (score < Unicode11 && platformSpecificId == 1) {
                tableToUse = n;
                score = Unicode11;
            }
            break;
        case 1: // Apple
            if (score < AppleRoman && platformSpecificId == 0) { // Apple Roman
                tableToUse = n;
                score = AppleRoman;
            }
            break;
        case 3: // Microsoft
            switch (platformSpecificId) {
            case 0:
                symbolTable = n;
                if (score < Symbol) {
                    tableToUse = n;
                    score = Symbol;
                }
                break;
            case 1:
                if (score < MicrosoftUnicode) {
                    tableToUse = n;
                    score = MicrosoftUnicode;
                }
                break;
            case 0xa:
                if (score < MicrosoftUnicodeExtended) {
                    tableToUse = n;
                    score = MicrosoftUnicodeExtended;
                }
                break;
            default:
                break;
            }
        default:
            break;
        }
    }
    if(tableToUse < 0)
        return 0;

resolveTable:
    *isSymbolFont = (symbolTable > -1);

    unsigned int unicode_table = qFromBigEndian<quint32>(maps + 8*tableToUse + 4);

    if (!unicode_table || unicode_table + 8 > tableSize)
        return 0;

    // get the header of the unicode table
    header = table + unicode_table;

    unsigned short format = qFromBigEndian<quint16>(header);
    unsigned int length;
    if(format < 8)
        length = qFromBigEndian<quint16>(header + 2);
    else
        length = qFromBigEndian<quint32>(header + 4);

    if (table + unicode_table + length > endPtr)
        return 0;
    *cmapSize = length;

    // To support symbol fonts that contain a unicode table for the symbol area
    // we check the cmap tables and fall back to symbol font unless that would
    // involve losing information from the unicode table
    if (symbolTable > -1 && ((score == Unicode) || (score == Unicode11))) {
        const uchar *selectedTable = table + unicode_table;

        // Check that none of the latin1 range are in the unicode table
        bool unicodeTableHasLatin1 = false;
        for (int uc=0x00; uc<0x100; ++uc) {
            if (getTrueTypeGlyphIndex(selectedTable, uc) != 0) {
                unicodeTableHasLatin1 = true;
                break;
            }
        }

        // Check that at least one symbol char is in the unicode table
        bool unicodeTableHasSymbols = false;
        if (!unicodeTableHasLatin1) {
            for (int uc=0xf000; uc<0xf100; ++uc) {
                if (getTrueTypeGlyphIndex(selectedTable, uc) != 0) {
                    unicodeTableHasSymbols = true;
                    break;
                }
            }
        }

        // Fall back to symbol table
        if (!unicodeTableHasLatin1 && unicodeTableHasSymbols) {
            tableToUse = symbolTable;
            score = Symbol;
            goto resolveTable;
        }
    }

    return table + unicode_table;
}

quint32 QFontEngine::getTrueTypeGlyphIndex(const uchar *cmap, uint unicode)
{
    unsigned short format = qFromBigEndian<quint16>(cmap);
    if (format == 0) {
        if (unicode < 256)
            return (int) *(cmap+6+unicode);
    } else if (format == 4) {
        /* some fonts come with invalid cmap tables, where the last segment
           specified end = start = rangeoffset = 0xffff, delta = 0x0001
           Since 0xffff is never a valid Unicode char anyway, we just get rid of the issue
           by returning 0 for 0xffff
        */
        if(unicode >= 0xffff)
            return 0;
        quint16 segCountX2 = qFromBigEndian<quint16>(cmap + 6);
        const unsigned char *ends = cmap + 14;
        int i = 0;
        for (; i < segCountX2/2 && qFromBigEndian<quint16>(ends + 2*i) < unicode; i++) {}

        const unsigned char *idx = ends + segCountX2 + 2 + 2*i;
        quint16 startIndex = qFromBigEndian<quint16>(idx);

        if (startIndex > unicode)
            return 0;

        idx += segCountX2;
        qint16 idDelta = (qint16)qFromBigEndian<quint16>(idx);
        idx += segCountX2;
        quint16 idRangeoffset_t = (quint16)qFromBigEndian<quint16>(idx);

        quint16 glyphIndex;
        if (idRangeoffset_t) {
            quint16 id = qFromBigEndian<quint16>(idRangeoffset_t + 2*(unicode - startIndex) + idx);
            if (id)
                glyphIndex = (idDelta + id) % 0x10000;
            else
                glyphIndex = 0;
        } else {
            glyphIndex = (idDelta + unicode) % 0x10000;
        }
        return glyphIndex;
    } else if (format == 6) {
        quint16 tableSize = qFromBigEndian<quint16>(cmap + 2);

        quint16 firstCode6 = qFromBigEndian<quint16>(cmap + 6);
        if (unicode < firstCode6)
            return 0;

        quint16 entryCount6 = qFromBigEndian<quint16>(cmap + 8);
        if (entryCount6 * 2 + 10 > tableSize)
            return 0;

        quint16 sentinel6 = firstCode6 + entryCount6;
        if (unicode >= sentinel6)
            return 0;

        quint16 entryIndex6 = unicode - firstCode6;
        return qFromBigEndian<quint16>(cmap + 10 + (entryIndex6 * 2));
    } else if (format == 12) {
        quint32 nGroups = qFromBigEndian<quint32>(cmap + 12);

        cmap += 16; // move to start of groups

        int left = 0, right = nGroups - 1;
        while (left <= right) {
            int middle = left + ( ( right - left ) >> 1 );

            quint32 startCharCode = qFromBigEndian<quint32>(cmap + 12*middle);
            if(unicode < startCharCode)
                right = middle - 1;
            else {
                quint32 endCharCode = qFromBigEndian<quint32>(cmap + 12*middle + 4);
                if(unicode <= endCharCode)
                    return qFromBigEndian<quint32>(cmap + 12*middle + 8) + unicode - startCharCode;
                left = middle + 1;
            }
        }
    } else {
        qDebug("cmap table of format %d not implemented", format);
    }

    return 0;
}

QByteArray QFontEngine::convertToPostscriptFontFamilyName(const QByteArray &family)
{
    QByteArray f = family;
    f.replace(' ', "");
    f.replace('(', "");
    f.replace(')', "");
    f.replace('<', "");
    f.replace('>', "");
    f.replace('[', "");
    f.replace(']', "");
    f.replace('{', "");
    f.replace('}', "");
    f.replace('/', "");
    f.replace('%', "");
    return f;
}

QFixed QFontEngine::lastRightBearing(const QGlyphLayout &glyphs, bool round)
{
    if (glyphs.numGlyphs >= 1) {
        glyph_t glyph = glyphs.glyphs[glyphs.numGlyphs - 1];
        glyph_metrics_t gi = boundingBox(glyph);
        if (gi.isValid())
            return round ? QFixed(qRound(gi.xoff - gi.x - gi.width))
                         : QFixed(gi.xoff - gi.x - gi.width);
    }
    return 0;
}


QFontEngine::GlyphCacheEntry::GlyphCacheEntry()
    : context(0)
{
}

QFontEngine::GlyphCacheEntry::GlyphCacheEntry(const GlyphCacheEntry &o)
    : context(o.context), cache(o.cache)
{
}

QFontEngine::GlyphCacheEntry::~GlyphCacheEntry()
{
}

QFontEngine::GlyphCacheEntry &QFontEngine::GlyphCacheEntry::operator=(const GlyphCacheEntry &o)
{
    context = o.context;
    cache = o.cache;
    return *this;
}

// ------------------------------------------------------------------
// The box font engine
// ------------------------------------------------------------------

QFontEngineBox::QFontEngineBox(int size)
    : QFontEngine(Box),
      _size(size)
{
    cache_cost = sizeof(QFontEngineBox);
}

QFontEngineBox::QFontEngineBox(Type type, int size)
    : QFontEngine(type),
      _size(size)
{
    cache_cost = sizeof(QFontEngineBox);
}

QFontEngineBox::~QFontEngineBox()
{
}

glyph_t QFontEngineBox::glyphIndex(uint ucs4) const
{
    Q_UNUSED(ucs4)
    return 0;
}

bool QFontEngineBox::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    int ucs4Length = 0;
    QStringIterator it(str, str + len);
    while (it.hasNext()) {
        it.advance();
        glyphs->glyphs[ucs4Length++] = 0;
    }

    *nglyphs = ucs4Length;
    glyphs->numGlyphs = ucs4Length;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

void QFontEngineBox::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags) const
{
    for (int i = 0; i < glyphs->numGlyphs; i++)
        glyphs->advances[i] = _size;
}

void QFontEngineBox::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> positioned_glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y - _size);
    getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);

    QSize s(_size - 3, _size - 3);
    for (int k = 0; k < positions.size(); k++)
        path->addRect(QRectF(positions[k].toPointF(), s));
}

glyph_metrics_t QFontEngineBox::boundingBox(const QGlyphLayout &glyphs)
{
    glyph_metrics_t overall;
    overall.width = _size*glyphs.numGlyphs;
    overall.height = _size;
    overall.xoff = overall.width;
    return overall;
}

void QFontEngineBox::draw(QPaintEngine *p, qreal x, qreal y, const QTextItemInt &ti)
{
    if (!ti.glyphs.numGlyphs)
        return;

    // any fixes here should probably also be done in QPaintEnginePrivate::drawBoxTextItem
    QSize s(_size - 3, _size - 3);

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y - _size);
    ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;


    QPainter *painter = p->painter();
    painter->save();
    painter->setBrush(Qt::NoBrush);
    QPen pen = painter->pen();
    pen.setWidthF(lineThickness().toReal());
    painter->setPen(pen);
    for (int k = 0; k < positions.size(); k++)
        painter->drawRect(QRectF(positions[k].toPointF(), s));
    painter->restore();
}

glyph_metrics_t QFontEngineBox::boundingBox(glyph_t)
{
    return glyph_metrics_t(0, -_size, _size, _size, _size, 0);
}

QFontEngine *QFontEngineBox::cloneWithSize(qreal pixelSize) const
{
    QFontEngineBox *fe = new QFontEngineBox(pixelSize);
    return fe;
}

QFixed QFontEngineBox::ascent() const
{
    return _size;
}

QFixed QFontEngineBox::descent() const
{
    return 0;
}

QFixed QFontEngineBox::leading() const
{
    QFixed l = _size * QFixed::fromReal(qreal(0.15));
    return l.ceil();
}

qreal QFontEngineBox::maxCharWidth() const
{
    return _size;
}

bool QFontEngineBox::canRender(const QChar *, int) const
{
    return true;
}

QImage QFontEngineBox::alphaMapForGlyph(glyph_t)
{
    QImage image(_size, _size, QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    image.setColorTable(colors);
    image.fill(0);

    // can't use qpainter for index8; so use setPixel to draw our rectangle.
    for (int i=2; i <= _size-3; ++i) {
        image.setPixel(i, 2, 255);
        image.setPixel(i, _size-3, 255);
        image.setPixel(2, i, 255);
        image.setPixel(_size-3, i, 255);
    }
    return image;
}

// ------------------------------------------------------------------
// Multi engine
// ------------------------------------------------------------------

static inline uchar highByte(glyph_t glyph)
{ return glyph >> 24; }

// strip high byte from glyph
static inline glyph_t stripped(glyph_t glyph)
{ return glyph & 0x00ffffff; }

QFontEngineMulti::QFontEngineMulti(int engineCount)
    : QFontEngine(Multi)
{
    engines.fill(0, engineCount);
    cache_cost = 0;
}

QFontEngineMulti::~QFontEngineMulti()
{
    for (int i = 0; i < engines.size(); ++i) {
        QFontEngine *fontEngine = engines.at(i);
        if (fontEngine && !fontEngine->ref.deref())
            delete fontEngine;
    }
}

glyph_t QFontEngineMulti::glyphIndex(uint ucs4) const
{
    glyph_t glyph = engine(0)->glyphIndex(ucs4);
    if (glyph == 0 && ucs4 != QChar::LineSeparator) {
        const_cast<QFontEngineMulti *>(this)->ensureFallbackFamiliesQueried();
        for (int x = 1, n = qMin(engines.size(), 256); x < n; ++x) {
            QFontEngine *engine = engines.at(x);
            if (!engine) {
                if (!shouldLoadFontEngineForCharacter(x, ucs4))
                    continue;
                const_cast<QFontEngineMulti *>(this)->loadEngine(x);
                engine = engines.at(x);
            }
            Q_ASSERT(engine != 0);
            if (engine->type() == Box)
                continue;

            glyph = engine->glyphIndex(ucs4);
            if (glyph != 0) {
                // set the high byte to indicate which engine the glyph came from
                glyph |= (x << 24);
                break;
            }
        }
    }

    return glyph;
}

bool QFontEngineMulti::stringToCMap(const QChar *str, int len,
                                    QGlyphLayout *glyphs, int *nglyphs,
                                    QFontEngine::ShaperFlags flags) const
{
    if (!engine(0)->stringToCMap(str, len, glyphs, nglyphs, flags))
        return false;

    const_cast<QFontEngineMulti *>(this)->ensureFallbackFamiliesQueried();
    int glyph_pos = 0;
    QStringIterator it(str, str + len);
    while (it.hasNext()) {
        const uint ucs4 = it.peekNext();
        if (glyphs->glyphs[glyph_pos] == 0 && ucs4 != QChar::LineSeparator) {
            for (int x = 1, n = qMin(engines.size(), 256); x < n; ++x) {
                if (engines.at(x) == 0 && !shouldLoadFontEngineForCharacter(x, ucs4))
                    continue;

                QFontEngine *engine = engines.at(x);
                if (!engine) {
                    const_cast<QFontEngineMulti *>(this)->loadEngine(x);
                    engine = engines.at(x);
                }
                Q_ASSERT(engine != 0);
                if (engine->type() == Box)
                    continue;

                glyph_t glyph = engine->glyphIndex(ucs4);
                if (glyph != 0) {
                    glyphs->glyphs[glyph_pos] = glyph;
                    if (!(flags & GlyphIndicesOnly)) {
                        QGlyphLayout g = glyphs->mid(glyph_pos, 1);
                        engine->recalcAdvances(&g, flags);
                    }
                    // set the high byte to indicate which engine the glyph came from
                    glyphs->glyphs[glyph_pos] |= (x << 24);
                    break;
                }
            }
        }

        it.advance();
        ++glyph_pos;
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    return true;
}

bool QFontEngineMulti::shouldLoadFontEngineForCharacter(int at, uint ucs4) const
{
    Q_UNUSED(at);
    Q_UNUSED(ucs4);
    return true;
}

glyph_metrics_t QFontEngineMulti::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs <= 0)
        return glyph_metrics_t();

    glyph_metrics_t overall;

    int which = highByte(glyphs.glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs.numGlyphs; ++end) {
        const int e = highByte(glyphs.glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

        // merge the bounding box for this run
        const glyph_metrics_t gm = engine(which)->boundingBox(glyphs.mid(start, end - start));

        overall.x = qMin(overall.x, gm.x);
        overall.y = qMin(overall.y, gm.y);
        overall.width = overall.xoff + gm.width;
        overall.height = qMax(overall.height + overall.y, gm.height + gm.y) -
                         qMin(overall.y, gm.y);
        overall.xoff += gm.xoff;
        overall.yoff += gm.yoff;

        // reset the high byte for all glyphs
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = hi | glyphs.glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

    // merge the bounding box for this run
    const glyph_metrics_t gm = engine(which)->boundingBox(glyphs.mid(start, end - start));

    overall.x = qMin(overall.x, gm.x);
    overall.y = qMin(overall.y, gm.y);
    overall.width = overall.xoff + gm.width;
    overall.height = qMax(overall.height + overall.y, gm.height + gm.y) -
                     qMin(overall.y, gm.y);
    overall.xoff += gm.xoff;
    overall.yoff += gm.yoff;

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = hi | glyphs.glyphs[i];

    return overall;
}

void QFontEngineMulti::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    int which = highByte(glyph);
    ensureEngineAt(which);
    engine(which)->getGlyphBearings(stripped(glyph), leftBearing, rightBearing);
}

void QFontEngineMulti::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                        QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (glyphs.numGlyphs <= 0)
        return;

    int which = highByte(glyphs.glyphs[0]);
    int start = 0;
    int end, i;
    if (flags & QTextItem::RightToLeft) {
        for (int gl = 0; gl < glyphs.numGlyphs; gl++)
            x += glyphs.advances[gl].toReal();
    }
    for (end = 0; end < glyphs.numGlyphs; ++end) {
        const int e = highByte(glyphs.glyphs[end]);
        if (e == which)
            continue;

        if (flags & QTextItem::RightToLeft) {
            for (i = start; i < end; ++i)
                x -= glyphs.advances[i].toReal();
        }

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);
        engine(which)->addOutlineToPath(x, y, glyphs.mid(start, end - start), path, flags);
        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = hi | glyphs.glyphs[i];

        if (!(flags & QTextItem::RightToLeft)) {
            for (i = start; i < end; ++i)
                x += glyphs.advances[i].toReal();
        }

        // change engine
        start = end;
        which = e;
    }

    if (flags & QTextItem::RightToLeft) {
        for (i = start; i < end; ++i)
            x -= glyphs.advances[i].toReal();
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

    engine(which)->addOutlineToPath(x, y, glyphs.mid(start, end - start), path, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = hi | glyphs.glyphs[i];
}

void QFontEngineMulti::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    if (glyphs->numGlyphs <= 0)
        return;

    int which = highByte(glyphs->glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs->numGlyphs; ++end) {
        const int e = highByte(glyphs->glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

        QGlyphLayout offs = glyphs->mid(start, end - start);
        engine(which)->recalcAdvances(&offs, flags);

        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = hi | glyphs->glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

    QGlyphLayout offs = glyphs->mid(start, end - start);
    engine(which)->recalcAdvances(&offs, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = hi | glyphs->glyphs[i];
}

void QFontEngineMulti::doKerning(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    if (glyphs->numGlyphs <= 0)
        return;

    int which = highByte(glyphs->glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs->numGlyphs; ++end) {
        const int e = highByte(glyphs->glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

        QGlyphLayout offs = glyphs->mid(start, end - start);
        engine(which)->doKerning(&offs, flags);

        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = hi | glyphs->glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

    QGlyphLayout offs = glyphs->mid(start, end - start);
    engine(which)->doKerning(&offs, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = hi | glyphs->glyphs[i];
}

glyph_metrics_t QFontEngineMulti::boundingBox(glyph_t glyph)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->boundingBox(stripped(glyph));
}

QFixed QFontEngineMulti::ascent() const
{ return engine(0)->ascent(); }

QFixed QFontEngineMulti::descent() const
{ return engine(0)->descent(); }

QFixed QFontEngineMulti::leading() const
{
    return engine(0)->leading();
}

QFixed QFontEngineMulti::xHeight() const
{
    return engine(0)->xHeight();
}

QFixed QFontEngineMulti::averageCharWidth() const
{
    return engine(0)->averageCharWidth();
}

QFixed QFontEngineMulti::lineThickness() const
{
    return engine(0)->lineThickness();
}

QFixed QFontEngineMulti::underlinePosition() const
{
    return engine(0)->underlinePosition();
}

qreal QFontEngineMulti::maxCharWidth() const
{
    return engine(0)->maxCharWidth();
}

qreal QFontEngineMulti::minLeftBearing() const
{
    return engine(0)->minLeftBearing();
}

qreal QFontEngineMulti::minRightBearing() const
{
    return engine(0)->minRightBearing();
}

bool QFontEngineMulti::canRender(const QChar *string, int len) const
{
    if (engine(0)->canRender(string, len))
        return true;

    int nglyphs = len;

    QVarLengthArray<glyph_t> glyphs(nglyphs);

    QGlyphLayout g;
    g.numGlyphs = nglyphs;
    g.glyphs = glyphs.data();
    if (!stringToCMap(string, len, &g, &nglyphs, GlyphIndicesOnly))
        Q_UNREACHABLE();

    for (int i = 0; i < nglyphs; i++) {
        if (glyphs[i] == 0)
            return false;
    }

    return true;
}

/* Implement alphaMapForGlyph() which is called by Lighthouse/Windows code.
 * Ideally, that code should be fixed to correctly handle QFontEngineMulti. */

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->alphaMapForGlyph(stripped(glyph));
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->alphaMapForGlyph(stripped(glyph), subPixelPosition);
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph, const QTransform &t)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->alphaMapForGlyph(stripped(glyph), t);
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->alphaMapForGlyph(stripped(glyph), subPixelPosition, t);
}

QImage QFontEngineMulti::alphaRGBMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t)
{
    const int which = highByte(glyph);
    Q_ASSERT(which < engines.size());
    return engine(which)->alphaRGBMapForGlyph(stripped(glyph), subPixelPosition, t);
}

QT_END_NAMESPACE
