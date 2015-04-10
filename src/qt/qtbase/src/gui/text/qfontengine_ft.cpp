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

#include "qdir.h"
#include "qmetatype.h"
#include "qtextstream.h"
#include "qvariant.h"
#include "qfontengine_ft_p.h"
#include "private/qimage_p.h"
#include <private/qstringiterator_p.h>

#ifndef QT_NO_FREETYPE

#include "qfile.h"
#include "qfileinfo.h"
#include "qthreadstorage.h"
#include <qmath.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_SYNTHESIS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TYPE1_TABLES_H
#include FT_GLYPH_H

#if defined(FT_LCD_FILTER_H)
#include FT_LCD_FILTER_H
#endif

#if defined(FT_CONFIG_OPTIONS_H)
#include FT_CONFIG_OPTIONS_H
#endif

#if defined(FT_LCD_FILTER_H) && defined(FT_CONFIG_OPTION_SUBPIXEL_RENDERING)
#define QT_USE_FREETYPE_LCDFILTER
#endif

#ifdef QT_LINUXBASE
#include FT_ERRORS_H
#endif

#if !defined(QT_MAX_CACHED_GLYPH_SIZE)
#  define QT_MAX_CACHED_GLYPH_SIZE 64
#endif

QT_BEGIN_NAMESPACE

/*
 * Freetype 2.1.7 and earlier used width/height
 * for matching sizes in the BDF and PCF loaders.
 * This has been fixed for 2.1.8.
 */
#if (FREETYPE_MAJOR*10000+FREETYPE_MINOR*100+FREETYPE_PATCH) >= 20105
#define X_SIZE(face,i) ((face)->available_sizes[i].x_ppem)
#define Y_SIZE(face,i) ((face)->available_sizes[i].y_ppem)
#else
#define X_SIZE(face,i) ((face)->available_sizes[i].width << 6)
#define Y_SIZE(face,i) ((face)->available_sizes[i].height << 6)
#endif

/* FreeType 2.1.10 starts to provide FT_GlyphSlot_Embolden */
#if (FREETYPE_MAJOR*10000+FREETYPE_MINOR*100+FREETYPE_PATCH) >= 20110
#define Q_FT_GLYPHSLOT_EMBOLDEN(slot)   FT_GlyphSlot_Embolden(slot)
#else
#define Q_FT_GLYPHSLOT_EMBOLDEN(slot)
#endif

/* FreeType 2.1.10 starts to provide FT_GlyphSlot_Oblique */
#if (FREETYPE_MAJOR*10000+FREETYPE_MINOR*100+FREETYPE_PATCH) >= 20110
#define Q_HAS_FT_GLYPHSLOT_OBLIQUE
#define Q_FT_GLYPHSLOT_OBLIQUE(slot)   FT_GlyphSlot_Oblique(slot)
#else
#define Q_FT_GLYPHSLOT_OBLIQUE(slot)
#endif

#define FLOOR(x)    ((x) & -64)
#define CEIL(x)     (((x)+63) & -64)
#define TRUNC(x)    ((x) >> 6)
#define ROUND(x)    (((x)+32) & -64)

static bool ft_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length)
{
    FT_Face face = (FT_Face)user_data;

    bool result = false;
    if (FT_IS_SFNT(face)) {
        FT_ULong len = *length;
        result = FT_Load_Sfnt_Table(face, tag, 0, buffer, &len) == FT_Err_Ok;
        *length = len;
        Q_ASSERT(!result || int(*length) > 0);
    }

    return result;
}


// -------------------------- Freetype support ------------------------------

class QtFreetypeData
{
public:
    QtFreetypeData()
        : library(0)
    { }
    ~QtFreetypeData();

    FT_Library library;
    QHash<QFontEngine::FaceId, QFreetypeFace *> faces;
};

QtFreetypeData::~QtFreetypeData()
{
    for (QHash<QFontEngine::FaceId, QFreetypeFace *>::ConstIterator iter = faces.begin(); iter != faces.end(); ++iter)
        iter.value()->cleanup();
    faces.clear();
    FT_Done_FreeType(library);
    library = 0;
}

#ifdef QT_NO_THREAD
Q_GLOBAL_STATIC(QtFreetypeData, theFreetypeData)

QtFreetypeData *qt_getFreetypeData()
{
    return theFreetypeData();
}
#else
Q_GLOBAL_STATIC(QThreadStorage<QtFreetypeData *>, theFreetypeData)

QtFreetypeData *qt_getFreetypeData()
{
    QtFreetypeData *&freetypeData = theFreetypeData()->localData();
    if (!freetypeData)
        freetypeData = new QtFreetypeData;
    return freetypeData;
}
#endif

FT_Library qt_getFreetype()
{
    QtFreetypeData *freetypeData = qt_getFreetypeData();
    if (!freetypeData->library)
        FT_Init_FreeType(&freetypeData->library);
    return freetypeData->library;
}

int QFreetypeFace::fsType() const
{
    int fsType = 0;
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2)
        fsType = os2->fsType;
    return fsType;
}

int QFreetypeFace::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    if (int error = FT_Load_Glyph(face, glyph, flags))
        return error;

    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return Err_Invalid_SubTable;

    *nPoints = face->glyph->outline.n_points;
    if (!(*nPoints))
        return Err_Ok;

    if (point > *nPoints)
        return Err_Invalid_SubTable;

    *xpos = QFixed::fromFixed(face->glyph->outline.points[point].x);
    *ypos = QFixed::fromFixed(face->glyph->outline.points[point].y);

    return Err_Ok;
}

extern QByteArray qt_fontdata_from_index(int);

/*
 * One font file can contain more than one font (bold/italic for example)
 * find the right one and return it.
 *
 * Returns the freetype face or 0 in case of an empty file or any other problems
 * (like not being able to open the file)
 */
QFreetypeFace *QFreetypeFace::getFace(const QFontEngine::FaceId &face_id,
                                      const QByteArray &fontData)
{
    if (face_id.filename.isEmpty() && fontData.isEmpty())
        return 0;

    QtFreetypeData *freetypeData = qt_getFreetypeData();
    if (!freetypeData->library)
        FT_Init_FreeType(&freetypeData->library);

    QFreetypeFace *freetype = freetypeData->faces.value(face_id, 0);
    if (freetype) {
        freetype->ref.ref();
    } else {
        QScopedPointer<QFreetypeFace> newFreetype(new QFreetypeFace);
        FT_Face face;
        if (!face_id.filename.isEmpty()) {
            QString fileName = QFile::decodeName(face_id.filename);
            if (face_id.filename.startsWith(":qmemoryfonts/")) {
                // from qfontdatabase.cpp
                QByteArray idx = face_id.filename;
                idx.remove(0, 14); // remove ':qmemoryfonts/'
                bool ok = false;
                newFreetype->fontData = qt_fontdata_from_index(idx.toInt(&ok));
                if (!ok)
                    newFreetype->fontData = QByteArray();
            } else if (!QFileInfo(fileName).isNativePath()) {
                QFile file(fileName);
                if (!file.open(QIODevice::ReadOnly)) {
                    return 0;
                }
                newFreetype->fontData = file.readAll();
            }
        } else {
            newFreetype->fontData = fontData;
        }
        if (!newFreetype->fontData.isEmpty()) {
            if (FT_New_Memory_Face(freetypeData->library, (const FT_Byte *)newFreetype->fontData.constData(), newFreetype->fontData.size(), face_id.index, &face)) {
                return 0;
            }
        } else if (FT_New_Face(freetypeData->library, face_id.filename, face_id.index, &face)) {
            return 0;
        }
        newFreetype->face = face;

        newFreetype->hbFace = 0;
        newFreetype->hbFace_destroy_func = 0;

        newFreetype->ref.store(1);
        newFreetype->xsize = 0;
        newFreetype->ysize = 0;
        newFreetype->matrix.xx = 0x10000;
        newFreetype->matrix.yy = 0x10000;
        newFreetype->matrix.xy = 0;
        newFreetype->matrix.yx = 0;
        newFreetype->unicode_map = 0;
        newFreetype->symbol_map = 0;

        memset(newFreetype->cmapCache, 0, sizeof(newFreetype->cmapCache));

        for (int i = 0; i < newFreetype->face->num_charmaps; ++i) {
            FT_CharMap cm = newFreetype->face->charmaps[i];
            switch(cm->encoding) {
            case FT_ENCODING_UNICODE:
                newFreetype->unicode_map = cm;
                break;
            case FT_ENCODING_APPLE_ROMAN:
            case FT_ENCODING_ADOBE_LATIN_1:
                if (!newFreetype->unicode_map || newFreetype->unicode_map->encoding != FT_ENCODING_UNICODE)
                    newFreetype->unicode_map = cm;
                break;
            case FT_ENCODING_ADOBE_CUSTOM:
            case FT_ENCODING_MS_SYMBOL:
                if (!newFreetype->symbol_map)
                    newFreetype->symbol_map = cm;
                break;
            default:
                break;
            }
        }

        if (!FT_IS_SCALABLE(newFreetype->face) && newFreetype->face->num_fixed_sizes == 1)
            FT_Set_Char_Size (face, X_SIZE(newFreetype->face, 0), Y_SIZE(newFreetype->face, 0), 0, 0);

        FT_Set_Charmap(newFreetype->face, newFreetype->unicode_map);
        QT_TRY {
            freetypeData->faces.insert(face_id, newFreetype.data());
        } QT_CATCH(...) {
            newFreetype.take()->release(face_id);
            // we could return null in principle instead of throwing
            QT_RETHROW;
        }
        freetype = newFreetype.take();
    }
    return freetype;
}

void QFreetypeFace::cleanup()
{
    if (hbFace && hbFace_destroy_func) {
        hbFace_destroy_func(hbFace);
        hbFace = 0;
    }
    FT_Done_Face(face);
    face = 0;
}

void QFreetypeFace::release(const QFontEngine::FaceId &face_id)
{
    if (!ref.deref()) {
        if (face) {
            QtFreetypeData *freetypeData = qt_getFreetypeData();

            cleanup();

            if (freetypeData->faces.contains(face_id))
                freetypeData->faces.take(face_id);

            if (freetypeData->faces.isEmpty()) {
                FT_Done_FreeType(freetypeData->library);
                freetypeData->library = 0;
            }
        }

        delete this;
    }
}


void QFreetypeFace::computeSize(const QFontDef &fontDef, int *xsize, int *ysize, bool *outline_drawing)
{
    *ysize = qRound(fontDef.pixelSize * 64);
    *xsize = *ysize * fontDef.stretch / 100;
    *outline_drawing = false;

    /*
     * Bitmap only faces must match exactly, so find the closest
     * one (height dominant search)
     */
    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        int best = 0;
        for (int i = 1; i < face->num_fixed_sizes; i++) {
            if (qAbs(*ysize -  Y_SIZE(face,i)) <
                qAbs (*ysize - Y_SIZE(face, best)) ||
                (qAbs (*ysize - Y_SIZE(face, i)) ==
                 qAbs (*ysize - Y_SIZE(face, best)) &&
                 qAbs (*xsize - X_SIZE(face, i)) <
                 qAbs (*xsize - X_SIZE(face, best)))) {
                best = i;
            }
        }
        if (FT_Set_Char_Size (face, X_SIZE(face, best), Y_SIZE(face, best), 0, 0) == 0) {
            *xsize = X_SIZE(face, best);
            *ysize = Y_SIZE(face, best);
        } else {
            int err = 1;
            if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) && ysize == 0 && face->num_fixed_sizes >= 1) {
                // work around FT 2.1.10 problem with BDF without PIXEL_SIZE property
                err = FT_Set_Pixel_Sizes(face, face->available_sizes[0].width, face->available_sizes[0].height);
                if (err && face->num_fixed_sizes == 1)
                    err = 0; //even more of a workaround...
            }

            if (err)
                *xsize = *ysize = 0;
        }
    } else {
        *outline_drawing = (*xsize > (QT_MAX_CACHED_GLYPH_SIZE<<6) || *ysize > (QT_MAX_CACHED_GLYPH_SIZE<<6));
    }
}

QFontEngine::Properties QFreetypeFace::properties() const
{
    QFontEngine::Properties p;
    p.postscriptName = FT_Get_Postscript_Name(face);
    PS_FontInfoRec font_info;
    if (FT_Get_PS_Font_Info(face, &font_info) == 0)
        p.copyright = font_info.notice;
    if (FT_IS_SCALABLE(face)) {
        p.ascent = face->ascender;
        p.descent = -face->descender;
        p.leading = face->height - face->ascender + face->descender;
        p.emSquare = face->units_per_EM;
        p.boundingBox = QRectF(face->bbox.xMin, -face->bbox.yMax,
                               face->bbox.xMax - face->bbox.xMin,
                               face->bbox.yMax - face->bbox.yMin);
    } else {
        p.ascent = QFixed::fromFixed(face->size->metrics.ascender);
        p.descent = QFixed::fromFixed(-face->size->metrics.descender);
        p.leading = QFixed::fromFixed(face->size->metrics.height - face->size->metrics.ascender + face->size->metrics.descender);
        p.emSquare = face->size->metrics.y_ppem;
//        p.boundingBox = QRectF(-p.ascent.toReal(), 0, (p.ascent + p.descent).toReal(), face->size->metrics.max_advance/64.);
        p.boundingBox = QRectF(0, -p.ascent.toReal(),
                               face->size->metrics.max_advance/64, (p.ascent + p.descent).toReal() );
    }
    p.italicAngle = 0;
    p.capHeight = p.ascent;
    p.lineWidth = face->underline_thickness;
    return p;
}

bool QFreetypeFace::getSfntTable(uint tag, uchar *buffer, uint *length) const
{
    return ft_getSfntTable(face, tag, buffer, length);
}

/* Some fonts (such as MingLiu rely on hinting to scale different
   components to their correct sizes. While this is really broken (it
   should be done in the component glyph itself, not the hinter) we
   will have to live with it.

   This means we can not use FT_LOAD_NO_HINTING to get the glyph
   outline. All we can do is to load the unscaled glyph and scale it
   down manually when required.
*/
static void scaleOutline(FT_Face face, FT_GlyphSlot g, FT_Fixed x_scale, FT_Fixed y_scale)
{
    x_scale = FT_MulDiv(x_scale, 1 << 10, face->units_per_EM);
    y_scale = FT_MulDiv(y_scale, 1 << 10, face->units_per_EM);
    FT_Vector *p = g->outline.points;
    const FT_Vector *e = p + g->outline.n_points;
    while (p < e) {
        p->x = FT_MulFix(p->x, x_scale);
        p->y = FT_MulFix(p->y, y_scale);
        ++p;
    }
}

void QFreetypeFace::addGlyphToPath(FT_Face face, FT_GlyphSlot g, const QFixedPoint &point, QPainterPath *path, FT_Fixed x_scale, FT_Fixed y_scale)
{
    const qreal factor = 1/64.;
    scaleOutline(face, g, x_scale, y_scale);

    QPointF cp = point.toPointF();

    // convert the outline to a painter path
    int i = 0;
    for (int j = 0; j < g->outline.n_contours; ++j) {
        int last_point = g->outline.contours[j];
        QPointF start = cp + QPointF(g->outline.points[i].x*factor, -g->outline.points[i].y*factor);
        if(!(g->outline.tags[i] & 1)) {
            start += cp + QPointF(g->outline.points[last_point].x*factor, -g->outline.points[last_point].y*factor);
            start /= 2;
        }
//         qDebug("contour: %d -- %d", i, g->outline.contours[j]);
//         qDebug("first point at %f %f", start.x(), start.y());
        path->moveTo(start);

        QPointF c[4];
        c[0] = start;
        int n = 1;
        while (i < last_point) {
            ++i;
            c[n] = cp + QPointF(g->outline.points[i].x*factor, -g->outline.points[i].y*factor);
//             qDebug() << "    i=" << i << " flag=" << (int)g->outline.tags[i] << "point=" << c[n];
            ++n;
            switch (g->outline.tags[i] & 3) {
            case 2:
                // cubic bezier element
                if (n < 4)
                    continue;
                c[3] = (c[3] + c[2])/2;
                --i;
                break;
            case 0:
                // quadratic bezier element
                if (n < 3)
                    continue;
                c[3] = (c[1] + c[2])/2;
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
                --i;
                break;
            case 1:
            case 3:
                if (n == 2) {
//                     qDebug() << "lineTo" << c[1];
                    path->lineTo(c[1]);
                    c[0] = c[1];
                    n = 1;
                    continue;
                } else if (n == 3) {
                    c[3] = c[2];
                    c[2] = (2*c[1] + c[3])/3;
                    c[1] = (2*c[1] + c[0])/3;
                }
                break;
            }
//             qDebug() << "cubicTo" << c[1] << c[2] << c[3];
            path->cubicTo(c[1], c[2], c[3]);
            c[0] = c[3];
            n = 1;
        }
        if (n == 1) {
//             qDebug() << "closeSubpath";
            path->closeSubpath();
        } else {
            c[3] = start;
            if (n == 2) {
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
            }
//             qDebug() << "cubicTo" << c[1] << c[2] << c[3];
            path->cubicTo(c[1], c[2], c[3]);
        }
        ++i;
    }
}

extern void qt_addBitmapToPath(qreal x0, qreal y0, const uchar *image_data, int bpl, int w, int h, QPainterPath *path);

void QFreetypeFace::addBitmapToPath(FT_GlyphSlot slot, const QFixedPoint &point, QPainterPath *path, bool)
{
    if (slot->format != FT_GLYPH_FORMAT_BITMAP
        || slot->bitmap.pixel_mode != FT_PIXEL_MODE_MONO)
        return;

    QPointF cp = point.toPointF();
    qt_addBitmapToPath(cp.x() + TRUNC(slot->metrics.horiBearingX), cp.y() - TRUNC(slot->metrics.horiBearingY),
                       slot->bitmap.buffer, slot->bitmap.pitch, slot->bitmap.width, slot->bitmap.rows, path);
}

QFontEngineFT::Glyph::~Glyph()
{
    delete [] data;
}

static const uint subpixel_filter[3][3] = {
    { 180, 60, 16 },
    { 38, 180, 38 },
    { 16, 60, 180 }
};

static inline uint filterPixel(uint red, uint green, uint blue, bool legacyFilter)
{
    uint res;
    if (legacyFilter) {
        uint high = (red*subpixel_filter[0][0] + green*subpixel_filter[0][1] + blue*subpixel_filter[0][2]) >> 8;
        uint mid = (red*subpixel_filter[1][0] + green*subpixel_filter[1][1] + blue*subpixel_filter[1][2]) >> 8;
        uint low = (red*subpixel_filter[2][0] + green*subpixel_filter[2][1] + blue*subpixel_filter[2][2]) >> 8;
        res = (mid << 24) + (high << 16) + (mid << 8) + low;
    } else {
        uint alpha = green;
        res = (alpha << 24) + (red << 16) + (green << 8) + blue;
    }
    return res;
}

static void convertRGBToARGB(const uchar *src, uint *dst, int width, int height, int src_pitch, bool bgr, bool legacyFilter)
{
    int h = height;
    const int offs = bgr ? -1 : 1;
    const int w = width * 3;
    while (h--) {
        uint *dd = dst;
        for (int x = 0; x < w; x += 3) {
            uint red = src[x+1-offs];
            uint green = src[x+1];
            uint blue = src[x+1+offs];
            *dd = filterPixel(red, green, blue, legacyFilter);
            ++dd;
        }
        dst += width;
        src += src_pitch;
    }
}

static void convertRGBToARGB_V(const uchar *src, uint *dst, int width, int height, int src_pitch, bool bgr, bool legacyFilter)
{
    int h = height;
    const int offs = bgr ? -src_pitch : src_pitch;
    while (h--) {
        for (int x = 0; x < width; x++) {
            uint red = src[x+src_pitch-offs];
            uint green = src[x+src_pitch];
            uint blue = src[x+src_pitch+offs];
            dst[x] = filterPixel(red, green, blue, legacyFilter);
        }
        dst += width;
        src += 3*src_pitch;
    }
}

static void convertGRAYToARGB(const uchar *src, uint *dst, int width, int height, int src_pitch) {
    for (int y = 0; y < height; ++y) {
        int readpos =  (y * src_pitch);
        int writepos = (y * width);
        for (int x = 0; x < width; ++x) {
            dst[writepos + x] = (0xFF << 24) + (src[readpos + x] << 16) + (src[readpos + x] << 8) + src[readpos + x];
        }
    }
}

static void convoluteBitmap(const uchar *src, uchar *dst, int width, int height, int pitch)
{
    // convolute the bitmap with a triangle filter to get rid of color fringes
    // If we take account for a gamma value of 2, we end up with
    // weights of 1, 4, 9, 4, 1. We use an approximation of 1, 3, 8, 3, 1 here,
    // as this nicely sums up to 16 :)
    int h = height;
    while (h--) {
        dst[0] = dst[1] = 0;
        //
        for (int x = 2; x < width - 2; ++x) {
            uint sum = src[x-2] + 3*src[x-1] + 8*src[x] + 3*src[x+1] + src[x+2];
            dst[x] = (uchar) (sum >> 4);
        }
        dst[width - 2] = dst[width - 1] = 0;
        src += pitch;
        dst += pitch;
    }
}

QFontEngineFT::QFontEngineFT(const QFontDef &fd)
    : QFontEngine(Freetype)
{
    fontDef = fd;
    matrix.xx = 0x10000;
    matrix.yy = 0x10000;
    matrix.xy = 0;
    matrix.yx = 0;
    cache_cost = 100;
    kerning_pairs_loaded = false;
    transform = false;
    embolden = false;
    obliquen = false;
    antialias = true;
    freetype = 0;
    default_load_flags = FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
#ifndef Q_OS_WIN
    default_hint_style = HintNone;
#else
    default_hint_style = HintFull;
#endif
    subpixelType = Subpixel_None;
    lcdFilterType = 0;
#if defined(FT_LCD_FILTER_H)
    lcdFilterType = (int)((quintptr) FT_LCD_FILTER_DEFAULT);
#endif
    defaultFormat = Format_None;
    embeddedbitmap = false;
    const QByteArray env = qgetenv("QT_NO_FT_CACHE");
    cacheEnabled = env.isEmpty() || env.toInt() == 0;
    m_subPixelPositionCount = 4;
}

QFontEngineFT::~QFontEngineFT()
{
    if (freetype)
        freetype->release(face_id);
}

bool QFontEngineFT::init(FaceId faceId, bool antialias, GlyphFormat format,
                         const QByteArray &fontData)
{
    return init(faceId, antialias, format, QFreetypeFace::getFace(faceId, fontData));
}

bool QFontEngineFT::init(FaceId faceId, bool antialias, GlyphFormat format,
                         QFreetypeFace *freetypeFace)
{
    freetype = freetypeFace;
    if (!freetype) {
        xsize = 0;
        ysize = 0;
        return false;
    }
    defaultFormat = format;
    this->antialias = antialias;

    if (!antialias)
        glyphFormat = QFontEngine::Format_Mono;
    else
        glyphFormat = defaultFormat;

    face_id = faceId;

    symbol = freetype->symbol_map != 0;
    PS_FontInfoRec psrec;
    // don't assume that type1 fonts are symbol fonts by default
    if (FT_Get_PS_Font_Info(freetype->face, &psrec) == FT_Err_Ok) {
        symbol = bool(fontDef.family.contains(QLatin1String("symbol"), Qt::CaseInsensitive));
    }

    lbearing = rbearing = SHRT_MIN;
    freetype->computeSize(fontDef, &xsize, &ysize, &defaultGlyphSet.outline_drawing);

    FT_Face face = lockFace();

    if (FT_IS_SCALABLE(face)) {
        bool fake_oblique = (fontDef.style != QFont::StyleNormal) && !(face->style_flags & FT_STYLE_FLAG_ITALIC);
        if (fake_oblique) {
#if !defined(Q_HAS_FT_GLYPHSLOT_OBLIQUE)
            matrix.xy = 0x10000*3/10;
            transform = true;
#else
            obliquen = true;
#endif
        }
        FT_Set_Transform(face, &matrix, 0);
        freetype->matrix = matrix;
        // fake bold
        if ((fontDef.weight >= QFont::Bold) && !(face->style_flags & FT_STYLE_FLAG_BOLD) && !FT_IS_FIXED_WIDTH(face))
            embolden = true;
        // underline metrics
        line_thickness =  QFixed::fromFixed(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale));
        underline_position = QFixed::fromFixed(-FT_MulFix(face->underline_position, face->size->metrics.y_scale));
    } else {
        // ad hoc algorithm
        int score = fontDef.weight * fontDef.pixelSize;
        line_thickness = score / 700;
        // looks better with thicker line for small pointsizes
        if (line_thickness < 2 && score >= 1050)
            line_thickness = 2;
        underline_position =  ((line_thickness * 2) + 3) / 6;
    }
    if (line_thickness < 1)
        line_thickness = 1;

    metrics = face->size->metrics;

    /*
       TrueType fonts with embedded bitmaps may have a bitmap font specific
       ascent/descent in the EBLC table. There is no direct public API
       to extract those values. The only way we've found is to trick freetype
       into thinking that it's not a scalable font in FT_SelectSize so that
       the metrics are retrieved from the bitmap strikes.
    */
    if (FT_IS_SCALABLE(face)) {
        for (int i = 0; i < face->num_fixed_sizes; ++i) {
            if (xsize == X_SIZE(face, i) && ysize == Y_SIZE(face, i)) {
                face->face_flags &= ~FT_FACE_FLAG_SCALABLE;

                FT_Select_Size(face, i);
                metrics.ascender = face->size->metrics.ascender;
                metrics.descender = face->size->metrics.descender;
                FT_Set_Char_Size(face, xsize, ysize, 0, 0);

                face->face_flags |= FT_FACE_FLAG_SCALABLE;
                break;
            }
        }
    }

    fontDef.styleName = QString::fromUtf8(face->style_name);

    if (!freetype->hbFace) {
        faceData.user_data = face;
        faceData.get_font_table = ft_getSfntTable;
        freetype->hbFace = harfbuzzFace();
        freetype->hbFace_destroy_func = face_destroy_func;
    } else {
        Q_ASSERT(!face_);
        face_ = freetype->hbFace;
    }
    face_destroy_func = 0; // we share the HB face in QFreeTypeFace, so do not let ~QFontEngine() destroy it

    unlockFace();

    fsType = freetype->fsType();
    return true;
}

void QFontEngineFT::setDefaultHintStyle(HintStyle style)
{
    default_hint_style = style;
}

int QFontEngineFT::loadFlags(QGlyphSet *set, GlyphFormat format, int flags,
                             bool &hsubpixel, int &vfactor) const
{
    int load_flags = FT_LOAD_DEFAULT | default_load_flags;
    int load_target = default_hint_style == HintLight
                      ? FT_LOAD_TARGET_LIGHT
                      : FT_LOAD_TARGET_NORMAL;

    if (format == Format_Mono) {
        load_target = FT_LOAD_TARGET_MONO;
    } else if (format == Format_A32) {
        if (subpixelType == QFontEngineFT::Subpixel_RGB || subpixelType == QFontEngineFT::Subpixel_BGR) {
            if (default_hint_style == HintFull)
                load_target = FT_LOAD_TARGET_LCD;
            hsubpixel = true;
        } else if (subpixelType == QFontEngineFT::Subpixel_VRGB || subpixelType == QFontEngineFT::Subpixel_VBGR) {
            if (default_hint_style == HintFull)
                load_target = FT_LOAD_TARGET_LCD_V;
            vfactor = 3;
        }
    }

    if (set && set->outline_drawing)
        load_flags = FT_LOAD_NO_BITMAP;

    if (default_hint_style == HintNone || (flags & DesignMetrics) || (set && set->outline_drawing))
        load_flags |= FT_LOAD_NO_HINTING;
    else
        load_flags |= load_target;

    return load_flags;
}

QFontEngineFT::Glyph *QFontEngineFT::loadGlyph(QGlyphSet *set, uint glyph,
                                               QFixed subPixelPosition,
                                               GlyphFormat format,
                                               bool fetchMetricsOnly) const
{
//     Q_ASSERT(freetype->lock == 1);

    if (format == Format_None) {
        if (defaultFormat != Format_None) {
            format = defaultFormat;
        } else {
            format = Format_Mono;
        }
    }

    Glyph *g = set ? set->getGlyph(glyph, subPixelPosition) : 0;
    if (g && g->format == format && (fetchMetricsOnly || g->data))
        return g;

    QFontEngineFT::GlyphInfo info;

    Q_ASSERT(format != Format_None);
    bool hsubpixel = false;
    int vfactor = 1;
    int load_flags = loadFlags(set, format, 0, hsubpixel, vfactor);

    if (format != Format_Mono && !embeddedbitmap)
        load_flags |= FT_LOAD_NO_BITMAP;

    FT_Matrix matrix = freetype->matrix;
    bool transform = matrix.xx != 0x10000
                     || matrix.yy != 0x10000
                     || matrix.xy != 0
                     || matrix.yx != 0;

    if (transform)
        load_flags |= FT_LOAD_NO_BITMAP;

    FT_Face face = freetype->face;

    FT_Vector v;
    v.x = format == Format_Mono ? 0 : FT_Pos(subPixelPosition.toReal() * 64);
    v.y = 0;
    FT_Set_Transform(face, &freetype->matrix, &v);

    FT_Error err = FT_Load_Glyph(face, glyph, load_flags);
    if (err && (load_flags & FT_LOAD_NO_BITMAP)) {
        load_flags &= ~FT_LOAD_NO_BITMAP;
        err = FT_Load_Glyph(face, glyph, load_flags);
    }
    if (err == FT_Err_Too_Few_Arguments) {
        // this is an error in the bytecode interpreter, just try to run without it
        load_flags |= FT_LOAD_FORCE_AUTOHINT;
        err = FT_Load_Glyph(face, glyph, load_flags);
    }
    if (err != FT_Err_Ok)
        qWarning("load glyph failed err=%x face=%p, glyph=%d", err, face, glyph);

    FT_GlyphSlot slot = face->glyph;

    if (embolden) Q_FT_GLYPHSLOT_EMBOLDEN(slot);
    if (obliquen) {
        Q_FT_GLYPHSLOT_OBLIQUE(slot);

        // While Embolden alters the metrics of the slot, oblique does not, so we need
        // to fix this ourselves.
        transform = true;
        FT_Matrix m;
        m.xx = 0x10000;
        m.yx = 0x0;
        m.xy = 0x6000;
        m.yy = 0x10000;

        FT_Matrix_Multiply(&m, &matrix);
    }

    FT_Library library = qt_getFreetype();

    info.xOff = TRUNC(ROUND(slot->advance.x));
    info.yOff = 0;

    if ((set && set->outline_drawing) || fetchMetricsOnly) {
        int left  = FLOOR(slot->metrics.horiBearingX);
        int right = CEIL(slot->metrics.horiBearingX + slot->metrics.width);
        int top    = CEIL(slot->metrics.horiBearingY);
        int bottom = FLOOR(slot->metrics.horiBearingY - slot->metrics.height);
        int width = right-left;
        int height = top-bottom;

        // If any of the metrics are too large to fit, don't cache them
        if (qAbs(info.xOff) >= 128
                || qAbs(TRUNC(top)) >= 128
                || TRUNC(width) >= 256
                || TRUNC(height) >= 256
                || qAbs(TRUNC(left)) >= 128
                || qAbs(TRUNC(ROUND(slot->advance.x))) >= 128) {
            return 0;
        }

        g = new Glyph;
        g->data = 0;
        g->linearAdvance = slot->linearHoriAdvance >> 10;
        g->width = TRUNC(width);
        g->height = TRUNC(height);
        g->x = TRUNC(left);
        g->y = TRUNC(top);
        g->advance = TRUNC(ROUND(slot->advance.x));
        g->format = format;

        if (set)
            set->setGlyph(glyph, subPixelPosition, g);

        return g;
    }

    uchar *glyph_buffer = 0;
    int glyph_buffer_size = 0;
#if defined(QT_USE_FREETYPE_LCDFILTER)
    bool useFreetypeRenderGlyph = false;
    if (slot->format == FT_GLYPH_FORMAT_OUTLINE && (hsubpixel || vfactor != 1)) {
        err = FT_Library_SetLcdFilter(library, (FT_LcdFilter)lcdFilterType);
        if (err == FT_Err_Ok)
            useFreetypeRenderGlyph = true;
    }

    if (useFreetypeRenderGlyph) {
        err = FT_Render_Glyph(slot, hsubpixel ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_LCD_V);

        if (err != FT_Err_Ok)
            qWarning("render glyph failed err=%x face=%p, glyph=%d", err, face, glyph);

        FT_Library_SetLcdFilter(library, FT_LCD_FILTER_NONE);

        info.height = slot->bitmap.rows / vfactor;
        info.width = hsubpixel ? slot->bitmap.width / 3 : slot->bitmap.width;
        info.x = -slot->bitmap_left;
        info.y = slot->bitmap_top;

        glyph_buffer_size = info.width * info.height * 4;
        glyph_buffer = new uchar[glyph_buffer_size];

        if (hsubpixel)
            convertRGBToARGB(slot->bitmap.buffer, (uint *)glyph_buffer, info.width, info.height, slot->bitmap.pitch, subpixelType != QFontEngineFT::Subpixel_RGB, false);
        else if (vfactor != 1)
            convertRGBToARGB_V(slot->bitmap.buffer, (uint *)glyph_buffer, info.width, info.height, slot->bitmap.pitch, subpixelType != QFontEngineFT::Subpixel_VRGB, false);
    } else
#endif
    {
    int left  = slot->metrics.horiBearingX;
    int right = slot->metrics.horiBearingX + slot->metrics.width;
    int top    = slot->metrics.horiBearingY;
    int bottom = slot->metrics.horiBearingY - slot->metrics.height;
    if(transform && slot->format != FT_GLYPH_FORMAT_BITMAP) {
        int l, r, t, b;
        FT_Vector vector;
        vector.x = left;
        vector.y = top;
        FT_Vector_Transform(&vector, &matrix);
        l = r = vector.x;
        t = b = vector.y;
        vector.x = right;
        vector.y = top;
        FT_Vector_Transform(&vector, &matrix);
        if (l > vector.x) l = vector.x;
        if (r < vector.x) r = vector.x;
        if (t < vector.y) t = vector.y;
        if (b > vector.y) b = vector.y;
        vector.x = right;
        vector.y = bottom;
        FT_Vector_Transform(&vector, &matrix);
        if (l > vector.x) l = vector.x;
        if (r < vector.x) r = vector.x;
        if (t < vector.y) t = vector.y;
        if (b > vector.y) b = vector.y;
        vector.x = left;
        vector.y = bottom;
        FT_Vector_Transform(&vector, &matrix);
        if (l > vector.x) l = vector.x;
        if (r < vector.x) r = vector.x;
        if (t < vector.y) t = vector.y;
        if (b > vector.y) b = vector.y;
        left = l;
        right = r;
        top = t;
        bottom = b;
    }
    left = FLOOR(left);
    right = CEIL(right);
    bottom = FLOOR(bottom);
    top = CEIL(top);

    int hpixels = TRUNC(right - left);
    // subpixel position requires one more pixel
    if (subPixelPosition > 0 && format != Format_Mono)
        hpixels++;

    if (hsubpixel)
        hpixels = hpixels*3 + 8;
    info.width = hpixels;
    info.height = TRUNC(top - bottom);
    info.x = -TRUNC(left);
    info.y = TRUNC(top);
    if (hsubpixel) {
        info.width /= 3;
        info.x += 1;
    }

    bool large_glyph = (((short)(slot->linearHoriAdvance>>10) != slot->linearHoriAdvance>>10)
                        || ((uchar)(info.width) != info.width)
                        || ((uchar)(info.height) != info.height)
                        || ((signed char)(info.x) != info.x)
                        || ((signed char)(info.y) != info.y)
                        || ((signed char)(info.xOff) != info.xOff));

    if (large_glyph) {
        delete [] glyph_buffer;
        return 0;
    }

    int pitch = (format == Format_Mono ? ((info.width + 31) & ~31) >> 3 :
                 (format == Format_A8 ? (info.width + 3) & ~3 : info.width * 4));
    glyph_buffer_size = pitch * info.height;
    glyph_buffer = new uchar[glyph_buffer_size];

    if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
        FT_Bitmap bitmap;
        bitmap.rows = info.height*vfactor;
        bitmap.width = hpixels;
        bitmap.pitch = format == Format_Mono ? (((info.width + 31) & ~31) >> 3) : ((bitmap.width + 3) & ~3);
        if (!hsubpixel && vfactor == 1 && format != Format_A32)
            bitmap.buffer = glyph_buffer;
        else
            bitmap.buffer = new uchar[bitmap.rows*bitmap.pitch];
        memset(bitmap.buffer, 0, bitmap.rows*bitmap.pitch);
        bitmap.pixel_mode = format == Format_Mono ? FT_PIXEL_MODE_MONO : FT_PIXEL_MODE_GRAY;
        FT_Matrix matrix;
        matrix.xx = (hsubpixel ? 3 : 1) << 16;
        matrix.yy = vfactor << 16;
        matrix.yx = matrix.xy = 0;

        FT_Outline_Transform(&slot->outline, &matrix);
        FT_Outline_Translate (&slot->outline, (hsubpixel ? -3*left +(4<<6) : -left), -bottom*vfactor);
        FT_Outline_Get_Bitmap(library, &slot->outline, &bitmap);
        if (hsubpixel) {
            Q_ASSERT (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
            Q_ASSERT(antialias);
            uchar *convoluted = new uchar[bitmap.rows*bitmap.pitch];
            bool useLegacyLcdFilter = false;
#if defined(FC_LCD_FILTER) && defined(FT_LCD_FILTER_H)
            useLegacyLcdFilter = (lcdFilterType == FT_LCD_FILTER_LEGACY);
#endif
            uchar *buffer = bitmap.buffer;
            if (!useLegacyLcdFilter) {
                convoluteBitmap(bitmap.buffer, convoluted, bitmap.width, info.height, bitmap.pitch);
                buffer = convoluted;
            }
            convertRGBToARGB(buffer + 1, (uint *)glyph_buffer, info.width, info.height, bitmap.pitch, subpixelType != QFontEngineFT::Subpixel_RGB, useLegacyLcdFilter);
            delete [] convoluted;
        } else if (vfactor != 1) {
            convertRGBToARGB_V(bitmap.buffer, (uint *)glyph_buffer, info.width, info.height, bitmap.pitch, subpixelType != QFontEngineFT::Subpixel_VRGB, true);
        } else if (format == Format_A32 && bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            convertGRAYToARGB(bitmap.buffer, (uint *)glyph_buffer, info.width, info.height, bitmap.pitch);
        }

        if (bitmap.buffer != glyph_buffer)
            delete [] bitmap.buffer;
    } else if (slot->format == FT_GLYPH_FORMAT_BITMAP) {
        Q_ASSERT(slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO);
        uchar *src = slot->bitmap.buffer;
        uchar *dst = glyph_buffer;
        int h = slot->bitmap.rows;
        if (format == Format_Mono) {
            int bytes = ((info.width + 7) & ~7) >> 3;
            while (h--) {
                memcpy (dst, src, bytes);
                dst += pitch;
                src += slot->bitmap.pitch;
            }
        } else {
            if (hsubpixel) {
                while (h--) {
                    uint *dd = (uint *)dst;
                    *dd++ = 0;
                    for (int x = 0; x < slot->bitmap.width; x++) {
                        uint a = ((src[x >> 3] & (0x80 >> (x & 7))) ? 0xffffff : 0x000000);
                        *dd++ = a;
                    }
                    *dd++ = 0;
                    dst += pitch;
                    src += slot->bitmap.pitch;
                }
            } else if (vfactor != 1) {
                while (h--) {
                    uint *dd = (uint *)dst;
                    for (int x = 0; x < slot->bitmap.width; x++) {
                        uint a = ((src[x >> 3] & (0x80 >> (x & 7))) ? 0xffffff : 0x000000);
                        *dd++ = a;
                    }
                    dst += pitch;
                    src += slot->bitmap.pitch;
                }
            } else {
                while (h--) {
                    for (int x = 0; x < slot->bitmap.width; x++) {
                        unsigned char a = ((src[x >> 3] & (0x80 >> (x & 7))) ? 0xff : 0x00);
                        dst[x] = a;
                    }
                    dst += pitch;
                    src += slot->bitmap.pitch;
                }
            }
        }
    } else {
        qWarning("QFontEngine: Glyph neither outline nor bitmap format=%d", slot->format);
        delete [] glyph_buffer;
        return 0;
    }
    }


    if (!g) {
        g = new Glyph;
        g->data = 0;
    }

    g->linearAdvance = slot->linearHoriAdvance >> 10;
    g->width = info.width;
    g->height = info.height;
    g->x = -info.x;
    g->y = info.y;
    g->advance = info.xOff;
    g->format = format;
    delete [] g->data;
    g->data = glyph_buffer;

    if (set)
        set->setGlyph(glyph, subPixelPosition, g);

    return g;
}

QFontEngine::FaceId QFontEngineFT::faceId() const
{
    return face_id;
}

QFontEngine::Properties QFontEngineFT::properties() const
{
    Properties p = freetype->properties();
    if (p.postscriptName.isEmpty()) {
        p.postscriptName = QFontEngine::convertToPostscriptFontFamilyName(fontDef.family.toUtf8());
    }

    return freetype->properties();
}

QFixed QFontEngineFT::emSquareSize() const
{
    if (FT_IS_SCALABLE(freetype->face))
        return freetype->face->units_per_EM;
    else
        return freetype->face->size->metrics.y_ppem;
}

bool QFontEngineFT::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    return ft_getSfntTable(freetype->face, tag, buffer, length);
}

int QFontEngineFT::synthesized() const
{
    int s = 0;
    if ((fontDef.style != QFont::StyleNormal) && !(freetype->face->style_flags & FT_STYLE_FLAG_ITALIC))
        s = SynthesizedItalic;
    if ((fontDef.weight >= QFont::Bold) && !(freetype->face->style_flags & FT_STYLE_FLAG_BOLD))
        s |= SynthesizedBold;
    if (fontDef.stretch != 100 && FT_IS_SCALABLE(freetype->face))
        s |= SynthesizedStretch;
    return s;
}

QFixed QFontEngineFT::ascent() const
{
    return QFixed::fromFixed(metrics.ascender);
}

QFixed QFontEngineFT::descent() const
{
    return QFixed::fromFixed(-metrics.descender);
}

QFixed QFontEngineFT::leading() const
{
    return QFixed::fromFixed(metrics.height - metrics.ascender + metrics.descender);
}

QFixed QFontEngineFT::xHeight() const
{
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(freetype->face, ft_sfnt_os2);
    if (os2 && os2->sxHeight) {
        lockFace();
        QFixed answer = QFixed(os2->sxHeight*freetype->face->size->metrics.y_ppem)/freetype->face->units_per_EM;
        unlockFace();
        return answer;
    }
    return QFontEngine::xHeight();
}

QFixed QFontEngineFT::averageCharWidth() const
{
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(freetype->face, ft_sfnt_os2);
    if (os2 && os2->xAvgCharWidth) {
        lockFace();
        QFixed answer = QFixed(os2->xAvgCharWidth*freetype->face->size->metrics.x_ppem)/freetype->face->units_per_EM;
        unlockFace();
        return answer;
    }
    return QFontEngine::averageCharWidth();
}

qreal QFontEngineFT::maxCharWidth() const
{
    return metrics.max_advance >> 6;
}

static const ushort char_table[] = {
        40,
        67,
        70,
        75,
        86,
        88,
        89,
        91,
        95,
        102,
        114,
        124,
        127,
        205,
        645,
        884,
        922,
        1070,
        12386
};

static const int char_table_entries = sizeof(char_table)/sizeof(ushort);


qreal QFontEngineFT::minLeftBearing() const
{
    if (lbearing == SHRT_MIN)
        (void) minRightBearing(); // calculates both
    return lbearing.toReal();
}

qreal QFontEngineFT::minRightBearing() const
{
    if (rbearing == SHRT_MIN) {
        lbearing = rbearing = 0;
        for (int i = 0; i < char_table_entries; ++i) {
            const glyph_t glyph = glyphIndex(char_table[i]);
            if (glyph != 0) {
                glyph_metrics_t gi = const_cast<QFontEngineFT *>(this)->boundingBox(glyph);
                lbearing = qMin(lbearing, gi.x);
                rbearing = qMin(rbearing, (gi.xoff - gi.x - gi.width));
            }
        }
    }
    return rbearing.toReal();
}

QFixed QFontEngineFT::lineThickness() const
{
    return line_thickness;
}

QFixed QFontEngineFT::underlinePosition() const
{
    return underline_position;
}

void QFontEngineFT::doKerning(QGlyphLayout *g, QFontEngine::ShaperFlags flags) const
{
    if (!kerning_pairs_loaded) {
        kerning_pairs_loaded = true;
        lockFace();
        if (freetype->face->size->metrics.x_ppem != 0) {
            QFixed scalingFactor(freetype->face->units_per_EM/freetype->face->size->metrics.x_ppem);
            unlockFace();
            const_cast<QFontEngineFT *>(this)->loadKerningPairs(scalingFactor);
        } else {
            unlockFace();
        }
    }

    if (shouldUseDesignMetrics(flags) && !(fontDef.styleStrategy & QFont::ForceIntegerMetrics))
        flags |= DesignMetrics;
    else
        flags &= ~DesignMetrics;

    QFontEngine::doKerning(g, flags);
}

QFontEngineFT::QGlyphSet *QFontEngineFT::loadTransformedGlyphSet(const QTransform &matrix)
{
    if (matrix.type() > QTransform::TxShear)
        return 0;

    // FT_Set_Transform only supports scalable fonts
    if (!FT_IS_SCALABLE(freetype->face))
        return 0;

    FT_Matrix m;
    m.xx = FT_Fixed(matrix.m11() * 65536);
    m.xy = FT_Fixed(-matrix.m21() * 65536);
    m.yx = FT_Fixed(-matrix.m12() * 65536);
    m.yy = FT_Fixed(matrix.m22() * 65536);

    QGlyphSet *gs = 0;

    for (int i = 0; i < transformedGlyphSets.count(); ++i) {
        const QGlyphSet &g = transformedGlyphSets.at(i);
        if (g.transformationMatrix.xx == m.xx
            && g.transformationMatrix.xy == m.xy
            && g.transformationMatrix.yx == m.yx
            && g.transformationMatrix.yy == m.yy) {

            // found a match, move it to the front
            transformedGlyphSets.move(i, 0);
            gs = &transformedGlyphSets[0];
            break;
        }
    }

    if (!gs) {
        // don't try to load huge fonts
        bool draw_as_outline = fontDef.pixelSize * qSqrt(qAbs(matrix.det())) >= QT_MAX_CACHED_GLYPH_SIZE;
        if (draw_as_outline)
            return 0;

        // don't cache more than 10 transformations
        if (transformedGlyphSets.count() >= 10) {
            transformedGlyphSets.move(transformedGlyphSets.size() - 1, 0);
        } else {
            transformedGlyphSets.prepend(QGlyphSet());
        }
        gs = &transformedGlyphSets[0];
        gs->clear();
        gs->transformationMatrix = m;
        gs->outline_drawing = draw_as_outline;
    }

    return gs;
}

bool QFontEngineFT::loadGlyphs(QGlyphSet *gs, const glyph_t *glyphs, int num_glyphs,
                               const QFixedPoint *positions,
                               GlyphFormat format)
{
    FT_Face face = 0;

    for (int i = 0; i < num_glyphs; ++i) {
        QFixed spp = subPixelPositionForX(positions[i].x);
        Glyph *glyph = gs ? gs->getGlyph(glyphs[i], spp) : 0;
        if (glyph == 0 || glyph->format != format) {
            if (!face) {
                face = lockFace();
                FT_Matrix m = matrix;
                FT_Matrix_Multiply(&gs->transformationMatrix, &m);
                FT_Set_Transform(face, &m, 0);
                freetype->matrix = m;
            }
            if (!loadGlyph(gs, glyphs[i], spp, format)) {
                unlockFace();
                return false;
            }
        }
    }

    if (face)
        unlockFace();

    return true;
}

void QFontEngineFT::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    FT_Face face = lockFace(Unscaled);
    FT_Set_Transform(face, 0, 0);
    FT_Load_Glyph(face, glyph, FT_LOAD_NO_BITMAP);

    int left  = face->glyph->metrics.horiBearingX;
    int right = face->glyph->metrics.horiBearingX + face->glyph->metrics.width;
    int top    = face->glyph->metrics.horiBearingY;
    int bottom = face->glyph->metrics.horiBearingY - face->glyph->metrics.height;

    QFixedPoint p;
    p.x = 0;
    p.y = 0;

    metrics->width = QFixed::fromFixed(right-left);
    metrics->height = QFixed::fromFixed(top-bottom);
    metrics->x = QFixed::fromFixed(left);
    metrics->y = QFixed::fromFixed(-top);
    metrics->xoff = QFixed::fromFixed(face->glyph->advance.x);

    if (!FT_IS_SCALABLE(freetype->face))
        QFreetypeFace::addBitmapToPath(face->glyph, p, path);
    else
        QFreetypeFace::addGlyphToPath(face, face->glyph, p, path, face->units_per_EM << 6, face->units_per_EM << 6);

    FT_Set_Transform(face, &freetype->matrix, 0);
    unlockFace();
}

bool QFontEngineFT::supportsTransformation(const QTransform &transform) const
{
    // The freetype engine falls back to QFontEngine for tranformed glyphs,
    // which uses fast-tranform and produces very ugly results, so we claim
    // to support just translations.
    return transform.type() <= QTransform::TxTranslate;
}

void QFontEngineFT::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    if (FT_IS_SCALABLE(freetype->face)) {
        QFontEngine::addOutlineToPath(x, y, glyphs, path, flags);
    } else {
        QVarLengthArray<QFixedPoint> positions;
        QVarLengthArray<glyph_t> positioned_glyphs;
        QTransform matrix;
        matrix.translate(x, y);
        getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);

        FT_Face face = lockFace(Unscaled);
        for (int gl = 0; gl < glyphs.numGlyphs; gl++) {
            FT_UInt glyph = positioned_glyphs[gl];
            FT_Load_Glyph(face, glyph, FT_LOAD_TARGET_MONO);
            freetype->addBitmapToPath(face->glyph, positions[gl], path);
        }
        unlockFace();
    }
}

void QFontEngineFT::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs,
                                    QPainterPath *path, QTextItem::RenderFlags)
{
    FT_Face face = lockFace(Unscaled);

    for (int gl = 0; gl < numGlyphs; gl++) {
        FT_UInt glyph = glyphs[gl];

        FT_Load_Glyph(face, glyph, FT_LOAD_NO_BITMAP);

        FT_GlyphSlot g = face->glyph;
        if (g->format != FT_GLYPH_FORMAT_OUTLINE)
            continue;
        if (embolden) Q_FT_GLYPHSLOT_EMBOLDEN(g);
        if (obliquen) Q_FT_GLYPHSLOT_OBLIQUE(g);
        QFreetypeFace::addGlyphToPath(face, g, positions[gl], path, xsize, ysize);
    }
    unlockFace();
}

glyph_t QFontEngineFT::glyphIndex(uint ucs4) const
{
    glyph_t glyph = ucs4 < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[ucs4] : 0;
    if (glyph == 0) {
        FT_Face face = freetype->face;
        glyph = FT_Get_Char_Index(face, ucs4);
        if (glyph == 0) {
            // Certain fonts don't have no-break space and tab,
            // while we usually want to render them as space
            if (ucs4 == QChar::Nbsp || ucs4 == QChar::Tabulation) {
                glyph = FT_Get_Char_Index(face, QChar::Space);
            } else if (freetype->symbol_map) {
                // Symbol fonts can have more than one CMAPs, FreeType should take the
                // correct one for us by default, so we always try FT_Get_Char_Index
                // first. If it didn't work (returns 0), we will explicitly set the
                // CMAP to symbol font one and try again. symbol_map is not always the
                // correct one because in certain fonts like Wingdings symbol_map only
                // contains PUA codepoints instead of the common ones.
                FT_Set_Charmap(face, freetype->symbol_map);
                glyph = FT_Get_Char_Index(face, ucs4);
                FT_Set_Charmap(face, freetype->unicode_map);
            }
        }
        if (ucs4 < QFreetypeFace::cmapCacheSize)
            freetype->cmapCache[ucs4] = glyph;
    }

    return glyph;
}

bool QFontEngineFT::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                                 QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    int glyph_pos = 0;
    if (freetype->symbol_map) {
        FT_Face face = freetype->face;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            uint uc = it.next();
            glyphs->glyphs[glyph_pos] = uc < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[uc] : 0;
            if ( !glyphs->glyphs[glyph_pos] ) {
                // Symbol fonts can have more than one CMAPs, FreeType should take the
                // correct one for us by default, so we always try FT_Get_Char_Index
                // first. If it didn't work (returns 0), we will explicitly set the
                // CMAP to symbol font one and try again. symbol_map is not always the
                // correct one because in certain fonts like Wingdings symbol_map only
                // contains PUA codepoints instead of the common ones.
                glyph_t glyph = FT_Get_Char_Index(face, uc);
                // Certain symbol fonts don't have no-break space (0xa0) and tab (0x9),
                // while we usually want to render them as space
                if (!glyph && (uc == 0xa0 || uc == 0x9)) {
                    uc = 0x20;
                    glyph = FT_Get_Char_Index(face, uc);
                }
                if (!glyph) {
                    FT_Set_Charmap(face, freetype->symbol_map);
                    glyph = FT_Get_Char_Index(face, uc);
                    FT_Set_Charmap(face, freetype->unicode_map);
                }
                glyphs->glyphs[glyph_pos] = glyph;
                if (uc < QFreetypeFace::cmapCacheSize)
                    freetype->cmapCache[uc] = glyph;
            }
            ++glyph_pos;
        }
    } else {
        FT_Face face = freetype->face;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            uint uc = it.next();
            glyphs->glyphs[glyph_pos] = uc < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[uc] : 0;
            if (!glyphs->glyphs[glyph_pos]) {
                {
                redo:
                    glyph_t glyph = FT_Get_Char_Index(face, uc);
                    if (!glyph && (uc == 0xa0 || uc == 0x9)) {
                        uc = 0x20;
                        goto redo;
                    }
                    glyphs->glyphs[glyph_pos] = glyph;
                    if (uc < QFreetypeFace::cmapCacheSize)
                        freetype->cmapCache[uc] = glyph;
                }
            }
            ++glyph_pos;
        }
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

bool QFontEngineFT::shouldUseDesignMetrics(QFontEngine::ShaperFlags flags) const
{
    if (!FT_IS_SCALABLE(freetype->face))
        return false;

    return default_hint_style == HintNone || default_hint_style == HintLight || (flags & DesignMetrics);
}

void QFontEngineFT::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    FT_Face face = 0;
    bool design = shouldUseDesignMetrics(flags);
    for (int i = 0; i < glyphs->numGlyphs; i++) {
        Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyphs->glyphs[i]) : 0;
        // Since we are passing Format_None to loadGlyph, use same default format logic as loadGlyph
        GlyphFormat acceptableFormat = (defaultFormat != Format_None) ? defaultFormat : Format_Mono;
        if (g && g->format == acceptableFormat) {
            glyphs->advances[i] = design ? QFixed::fromFixed(g->linearAdvance) : QFixed(g->advance);
        } else {
            if (!face)
                face = lockFace();
            g = loadGlyph(cacheEnabled ? &defaultGlyphSet : 0, glyphs->glyphs[i], 0, Format_None, true);
            glyphs->advances[i] = design ? QFixed::fromFixed(face->glyph->linearHoriAdvance >> 10)
                                         : QFixed::fromFixed(face->glyph->metrics.horiAdvance).round();
            if (!cacheEnabled)
                delete g;
        }
    }
    if (face)
        unlockFace();

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        for (int i = 0; i < glyphs->numGlyphs; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
}

glyph_metrics_t QFontEngineFT::boundingBox(const QGlyphLayout &glyphs)
{
    FT_Face face = 0;

    glyph_metrics_t overall;
    // initialize with line height, we get the same behaviour on all platforms
    overall.y = -ascent();
    overall.height = ascent() + descent();

    QFixed ymax = 0;
    QFixed xmax = 0;
    for (int i = 0; i < glyphs.numGlyphs; i++) {
        Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyphs.glyphs[i]) : 0;
        if (!g) {
            if (!face)
                face = lockFace();
            g = loadGlyph(cacheEnabled ? &defaultGlyphSet : 0, glyphs.glyphs[i], 0, Format_None, true);
        }
        if (g) {
            QFixed x = overall.xoff + glyphs.offsets[i].x + g->x;
            QFixed y = overall.yoff + glyphs.offsets[i].y - g->y;
            overall.x = qMin(overall.x, x);
            overall.y = qMin(overall.y, y);
            xmax = qMax(xmax, x + g->width);
            ymax = qMax(ymax, y + g->height);
            overall.xoff += g->advance;
            if (!cacheEnabled)
                delete g;
        } else {
            int left  = FLOOR(face->glyph->metrics.horiBearingX);
            int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
            int top    = CEIL(face->glyph->metrics.horiBearingY);
            int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

            QFixed x = overall.xoff + glyphs.offsets[i].x - (-TRUNC(left));
            QFixed y = overall.yoff + glyphs.offsets[i].y - TRUNC(top);
            overall.x = qMin(overall.x, x);
            overall.y = qMin(overall.y, y);
            xmax = qMax(xmax, x + TRUNC(right - left));
            ymax = qMax(ymax, y + TRUNC(top - bottom));
            overall.xoff += int(TRUNC(ROUND(face->glyph->advance.x)));
        }
    }
    overall.height = qMax(overall.height, ymax - overall.y);
    overall.width = xmax - overall.x;

    if (face)
        unlockFace();

    return overall;
}

glyph_metrics_t QFontEngineFT::boundingBox(glyph_t glyph)
{
    FT_Face face = 0;
    glyph_metrics_t overall;
    Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyph) : 0;
    if (!g) {
        face = lockFace();
        g = loadGlyph(cacheEnabled ? &defaultGlyphSet : 0, glyph, 0, Format_None, true);
    }
    if (g) {
        overall.x = g->x;
        overall.y = -g->y;
        overall.width = g->width;
        overall.height = g->height;
        overall.xoff = g->advance;
        if (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            overall.xoff = overall.xoff.round();
        if (!cacheEnabled)
            delete g;
    } else {
        int left  = FLOOR(face->glyph->metrics.horiBearingX);
        int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
        int top    = CEIL(face->glyph->metrics.horiBearingY);
        int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

        overall.width = TRUNC(right-left);
        overall.height = TRUNC(top-bottom);
        overall.x = TRUNC(left);
        overall.y = -TRUNC(top);
        overall.xoff = TRUNC(ROUND(face->glyph->advance.x));
    }
    if (face)
        unlockFace();
    return overall;
}

glyph_metrics_t QFontEngineFT::boundingBox(glyph_t glyph, const QTransform &matrix)
{
    return alphaMapBoundingBox(glyph, 0, matrix, QFontEngine::Format_None);
}

static FT_Matrix QTransformToFTMatrix(const QTransform &matrix)
{
    FT_Matrix m;

    m.xx = FT_Fixed(matrix.m11() * 65536);
    m.xy = FT_Fixed(-matrix.m21() * 65536);
    m.yx = FT_Fixed(-matrix.m12() * 65536);
    m.yy = FT_Fixed(matrix.m22() * 65536);

    return m;
}

glyph_metrics_t QFontEngineFT::alphaMapBoundingBox(glyph_t glyph, QFixed subPixelPosition, const QTransform &matrix, QFontEngine::GlyphFormat format)
{
    FT_Face face = 0;
    glyph_metrics_t overall;
    QGlyphSet *glyphSet = 0;
    FT_Matrix ftMatrix = QTransformToFTMatrix(matrix);
    if (cacheEnabled) {
        if (matrix.type() > QTransform::TxTranslate && FT_IS_SCALABLE(freetype->face)) {
            // TODO move everything here to a method of its own to access glyphSets
            // to be shared with a new method that will replace loadTransformedGlyphSet()
            for (int i = 0; i < transformedGlyphSets.count(); ++i) {
                const QGlyphSet &g = transformedGlyphSets.at(i);
                if (g.transformationMatrix.xx == ftMatrix.xx
                    && g.transformationMatrix.xy == ftMatrix.xy
                    && g.transformationMatrix.yx == ftMatrix.yx
                    && g.transformationMatrix.yy == ftMatrix.yy) {

                    // found a match, move it to the front
                    transformedGlyphSets.move(i, 0);
                    glyphSet = &transformedGlyphSets[0];
                    break;
                }
            }

            if (!glyphSet) {
                // don't cache more than 10 transformations
                if (transformedGlyphSets.count() >= 10) {
                    transformedGlyphSets.move(transformedGlyphSets.size() - 1, 0);
                } else {
                    transformedGlyphSets.prepend(QGlyphSet());
                }
                glyphSet = &transformedGlyphSets[0];
                glyphSet->clear();
                glyphSet->transformationMatrix = ftMatrix;
            }
            Q_ASSERT(glyphSet);
        } else {
            glyphSet = &defaultGlyphSet;
        }
    }
    Glyph * g = glyphSet ? glyphSet->getGlyph(glyph, subPixelPosition) : 0;
    if (!g || g->format != format) {
        face = lockFace();
        FT_Matrix m = this->matrix;
        FT_Matrix_Multiply(&ftMatrix, &m);
        freetype->matrix = m;
        g = loadGlyph(glyphSet, glyph, subPixelPosition, format, false);
    }

    if (g) {
        overall.x = g->x;
        overall.y = -g->y;
        overall.width = g->width;
        overall.height = g->height;
        overall.xoff = g->advance;
        if (!glyphSet)
            delete g;
    } else {
        int left  = FLOOR(face->glyph->metrics.horiBearingX);
        int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
        int top    = CEIL(face->glyph->metrics.horiBearingY);
        int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

        overall.width = TRUNC(right-left);
        overall.height = TRUNC(top-bottom);
        overall.x = TRUNC(left);
        overall.y = -TRUNC(top);
        overall.xoff = TRUNC(ROUND(face->glyph->advance.x));
    }
    if (face)
        unlockFace();
    return overall;
}

QImage *QFontEngineFT::lockedAlphaMapForGlyph(glyph_t glyphIndex, QFixed subPixelPosition,
                                              QFontEngine::GlyphFormat neededFormat,
                                              const QTransform &t, QPoint *offset)
{
    Q_ASSERT(currentlyLockedAlphaMap.isNull());
    lockFace();

    if (isBitmapFont())
        neededFormat = Format_Mono;
    else if (neededFormat == Format_None && defaultFormat != Format_None)
        neededFormat = defaultFormat;
    else if (neededFormat == Format_None)
        neededFormat = Format_A8;

    QImage::Format format;
    switch (neededFormat) {
    case Format_Mono:
        format = QImage::Format_Mono;
        break;
    case Format_A8:
        format = QImage::Format_Indexed8;
        break;
    case Format_A32:
        format = QImage::Format_ARGB32;
        break;
    default:
        Q_ASSERT(false);
        format = QImage::Format_Invalid;
    };

    QFontEngineFT::Glyph *glyph;
    QScopedPointer<QFontEngineFT::Glyph> glyphGuard;
    if (cacheEnabled) {
        QFontEngineFT::QGlyphSet *gset = &defaultGlyphSet;
        QFontEngine::HintStyle hintStyle = default_hint_style;
        if (t.type() >= QTransform::TxScale) {
            // disable hinting if the glyphs are transformed
            default_hint_style = HintNone;
            if (t.isAffine())
                gset = loadTransformedGlyphSet(t);
            else
                gset = 0;
        }

        if (gset) {
            FT_Matrix m = matrix;
            FT_Matrix_Multiply(&gset->transformationMatrix, &m);
            FT_Set_Transform(freetype->face, &m, 0);
            freetype->matrix = m;
        }

        if (!gset || gset->outline_drawing || !loadGlyph(gset, glyphIndex, subPixelPosition,
                                                         neededFormat)) {
            default_hint_style = hintStyle;
            return QFontEngine::lockedAlphaMapForGlyph(glyphIndex, subPixelPosition, neededFormat, t,
                                                       offset);
        }
        default_hint_style = hintStyle;

        glyph = gset->getGlyph(glyphIndex, subPixelPosition);
    } else {
        FT_Matrix m = matrix;
        FT_Matrix extra = QTransformToFTMatrix(t);
        FT_Matrix_Multiply(&extra, &m);
        FT_Set_Transform(freetype->face, &m, 0);
        freetype->matrix = m;
        glyph = loadGlyph(0, glyphIndex, subPixelPosition, neededFormat);
        glyphGuard.reset(glyph);
    }

    if (glyph == 0 || glyph->data == 0 || glyph->width == 0 || glyph->height == 0) {
        unlockFace();
        return 0;
    }

    int pitch;
    switch (neededFormat) {
    case Format_Mono:
        pitch = ((glyph->width + 31) & ~31) >> 3;
        break;
    case Format_A8:
        pitch = (glyph->width + 3) & ~3;
        break;
    case Format_A32:
        pitch = glyph->width * 4;
        break;
    default:
        Q_ASSERT(false);
        pitch = 0;
    };

    if (offset != 0)
        *offset = QPoint(glyph->x, -glyph->y);

    currentlyLockedAlphaMap = QImage(glyph->data, glyph->width, glyph->height, pitch, format);
    if (!glyphGuard.isNull())
        currentlyLockedAlphaMap = currentlyLockedAlphaMap.copy();
    Q_ASSERT(!currentlyLockedAlphaMap.isNull());

    QImageData *data = currentlyLockedAlphaMap.data_ptr();
    data->is_locked = true;

    return &currentlyLockedAlphaMap;
}

void QFontEngineFT::unlockAlphaMapForGlyph()
{
    Q_ASSERT(!currentlyLockedAlphaMap.isNull());
    unlockFace();
    currentlyLockedAlphaMap = QImage();
}

QFontEngineFT::Glyph *QFontEngineFT::loadGlyphFor(glyph_t g, QFixed subPixelPosition, GlyphFormat format)
{
    return defaultGlyphSet.outline_drawing ? 0 :
            loadGlyph(cacheEnabled ? &defaultGlyphSet : 0, g, subPixelPosition, format);
}

QImage QFontEngineFT::alphaMapForGlyph(glyph_t g, QFixed subPixelPosition)
{
    lockFace();

    QScopedPointer<Glyph> glyph(loadGlyphFor(g, subPixelPosition, antialias ? Format_A8 : Format_Mono));
    if (!glyph || !glyph->data) {
        unlockFace();
        return QFontEngine::alphaMapForGlyph(g);
    }

    const int pitch = antialias ? (glyph->width + 3) & ~3 : ((glyph->width + 31)/32) * 4;

    QImage img(glyph->width, glyph->height, antialias ? QImage::Format_Indexed8 : QImage::Format_Mono);
    if (antialias) {
        QVector<QRgb> colors(256);
        for (int i=0; i<256; ++i)
            colors[i] = qRgba(0, 0, 0, i);
        img.setColorTable(colors);
    } else {
        QVector<QRgb> colors(2);
        colors[0] = qRgba(0, 0, 0, 0);
        colors[1] = qRgba(0, 0, 0, 255);
        img.setColorTable(colors);
    }
    Q_ASSERT(img.bytesPerLine() == pitch);
    if (glyph->width) {
        for (int y = 0; y < glyph->height; ++y)
            memcpy(img.scanLine(y), &glyph->data[y * pitch], pitch);
    }
    if (cacheEnabled)
        glyph.take();
    unlockFace();

    return img;
}

QImage QFontEngineFT::alphaRGBMapForGlyph(glyph_t g, QFixed subPixelPosition, const QTransform &t)
{
    if (t.type() > QTransform::TxTranslate)
        return QFontEngine::alphaRGBMapForGlyph(g, subPixelPosition, t);

    lockFace();

    QScopedPointer<Glyph> glyph(loadGlyphFor(g, subPixelPosition, Format_A32));
    if (!glyph || !glyph->data) {
        unlockFace();
        return QFontEngine::alphaRGBMapForGlyph(g, subPixelPosition, t);
    }

    QImage img(glyph->width, glyph->height, QImage::Format_RGB32);
    memcpy(img.bits(), glyph->data, 4 * glyph->width * glyph->height);

    if (cacheEnabled)
        glyph.take();
    unlockFace();

    return img;
}

void QFontEngineFT::removeGlyphFromCache(glyph_t glyph)
{
    defaultGlyphSet.removeGlyphFromCache(glyph, 0);
}

int QFontEngineFT::glyphCount() const
{
    int count = 0;
    FT_Face face = lockFace();
    if (face) {
        count = face->num_glyphs;
        unlockFace();
    }
    return count;
}

FT_Face QFontEngineFT::lockFace(Scaling scale) const
{
    freetype->lock();
    FT_Face face = freetype->face;
    if (scale == Unscaled) {
        FT_Set_Char_Size(face, face->units_per_EM << 6, face->units_per_EM << 6, 0, 0);
        freetype->xsize = face->units_per_EM << 6;
        freetype->ysize = face->units_per_EM << 6;
    } else if (freetype->xsize != xsize || freetype->ysize != ysize) {
        FT_Set_Char_Size(face, xsize, ysize, 0, 0);
        freetype->xsize = xsize;
        freetype->ysize = ysize;
    }
    if (freetype->matrix.xx != matrix.xx ||
        freetype->matrix.yy != matrix.yy ||
        freetype->matrix.xy != matrix.xy ||
        freetype->matrix.yx != matrix.yx) {
        freetype->matrix = matrix;
        FT_Set_Transform(face, &freetype->matrix, 0);
    }

    return face;
}

void QFontEngineFT::unlockFace() const
{
    freetype->unlock();
}

FT_Face QFontEngineFT::non_locked_face() const
{
    return freetype->face;
}


QFontEngineFT::QGlyphSet::QGlyphSet()
    : outline_drawing(false)
{
    transformationMatrix.xx = 0x10000;
    transformationMatrix.yy = 0x10000;
    transformationMatrix.xy = 0;
    transformationMatrix.yx = 0;
    memset(fast_glyph_data, 0, sizeof(fast_glyph_data));
    fast_glyph_count = 0;
}

QFontEngineFT::QGlyphSet::~QGlyphSet()
{
    clear();
}

void QFontEngineFT::QGlyphSet::clear()
{
    if (fast_glyph_count > 0) {
        for (int i = 0; i < 256; ++i) {
            if (fast_glyph_data[i]) {
                delete fast_glyph_data[i];
                fast_glyph_data[i] = 0;
            }
        }
        fast_glyph_count = 0;
    }
    qDeleteAll(glyph_data);
    glyph_data.clear();
}

void QFontEngineFT::QGlyphSet::removeGlyphFromCache(glyph_t index, QFixed subPixelPosition)
{
    if (useFastGlyphData(index, subPixelPosition)) {
        if (fast_glyph_data[index]) {
            delete fast_glyph_data[index];
            fast_glyph_data[index] = 0;
            if (fast_glyph_count > 0)
                --fast_glyph_count;
        }
    } else {
        delete glyph_data.take(GlyphAndSubPixelPosition(index, subPixelPosition));
    }
}

void QFontEngineFT::QGlyphSet::setGlyph(glyph_t index, QFixed subPixelPosition, Glyph *glyph)
{
    if (useFastGlyphData(index, subPixelPosition)) {
        if (!fast_glyph_data[index])
            ++fast_glyph_count;
        fast_glyph_data[index] = glyph;
    } else {
        glyph_data.insert(GlyphAndSubPixelPosition(index, subPixelPosition), glyph);
    }
}

int QFontEngineFT::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    lockFace();
    bool hsubpixel = true;
    int vfactor = 1;
    int load_flags = loadFlags(0, Format_A8, flags, hsubpixel, vfactor);
    int result = freetype->getPointInOutline(glyph, load_flags, point, xpos, ypos, nPoints);
    unlockFace();
    return result;
}

bool QFontEngineFT::initFromFontEngine(const QFontEngineFT *fe)
{
    if (!init(fe->faceId(), fe->antialias, fe->defaultFormat, fe->freetype))
        return false;

    // Increase the reference of this QFreetypeFace since one more QFontEngineFT
    // will be using it
    freetype->ref.ref();

    default_load_flags = fe->default_load_flags;
    default_hint_style = fe->default_hint_style;
    antialias = fe->antialias;
    transform = fe->transform;
    embolden = fe->embolden;
    obliquen = fe->obliquen;
    subpixelType = fe->subpixelType;
    lcdFilterType = fe->lcdFilterType;
    embeddedbitmap = fe->embeddedbitmap;

    return true;
}

QFontEngine *QFontEngineFT::cloneWithSize(qreal pixelSize) const
{
    QFontDef fontDef(this->fontDef);
    fontDef.pixelSize = pixelSize;
    QFontEngineFT *fe = new QFontEngineFT(fontDef);
    if (!fe->initFromFontEngine(this)) {
        delete fe;
        return 0;
    } else {
        return fe;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_FREETYPE
