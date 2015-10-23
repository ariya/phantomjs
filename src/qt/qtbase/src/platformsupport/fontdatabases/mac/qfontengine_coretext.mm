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

#include "qfontengine_coretext_p.h"

#include <QtCore/qendian.h>
#include <QtCore/qsettings.h>

#include <private/qimage_p.h>

QT_BEGIN_NAMESPACE

static float SYNTHETIC_ITALIC_SKEW = tanf(14 * acosf(0) / 90);

bool QCoreTextFontEngine::ct_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length)
{
    CTFontRef ctfont = *(CTFontRef *)user_data;

    QCFType<CFDataRef> table = CTFontCopyTable(ctfont, tag, 0);
    if (!table)
        return false;

    CFIndex tableLength = CFDataGetLength(table);
    if (buffer && int(*length) >= tableLength)
        CFDataGetBytes(table, CFRangeMake(0, tableLength), buffer);
    *length = tableLength;
    Q_ASSERT(int(*length) > 0);
    return true;
}

static void loadAdvancesForGlyphs(CTFontRef ctfont,
                                  QVarLengthArray<CGGlyph> &cgGlyphs,
                                  QGlyphLayout *glyphs, int len,
                                  QFontEngine::ShaperFlags flags,
                                  const QFontDef &fontDef)
{
    Q_UNUSED(flags);
    QVarLengthArray<CGSize> advances(len);
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontHorizontalOrientation, cgGlyphs.data(), advances.data(), len);

    for (int i = 0; i < len; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            continue;
        glyphs->advances[i] = QFixed::fromReal(advances[i].width);
    }

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        for (int i = 0; i < len; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
}


int QCoreTextFontEngine::antialiasingThreshold = 0;
QFontEngine::GlyphFormat QCoreTextFontEngine::defaultGlyphFormat = QFontEngine::Format_A32;

CGAffineTransform qt_transform_from_fontdef(const QFontDef &fontDef)
{
    CGAffineTransform transform = CGAffineTransformIdentity;
    if (fontDef.stretch != 100)
        transform = CGAffineTransformMakeScale(float(fontDef.stretch) / float(100), 1);
    return transform;
}

QCoreTextFontEngine::QCoreTextFontEngine(CTFontRef font, const QFontDef &def)
    : QFontEngine(Mac)
{
    fontDef = def;
    transform = qt_transform_from_fontdef(fontDef);
    ctfont = font;
    CFRetain(ctfont);
    cgFont = CTFontCopyGraphicsFont(font, NULL);
    init();
}

QCoreTextFontEngine::QCoreTextFontEngine(CGFontRef font, const QFontDef &def)
    : QFontEngine(Mac)
{
    fontDef = def;
    transform = qt_transform_from_fontdef(fontDef);
    cgFont = font;
    // Keep reference count balanced
    CFRetain(cgFont);
    ctfont = CTFontCreateWithGraphicsFont(font, fontDef.pixelSize, &transform, NULL);
    init();
}

QCoreTextFontEngine::~QCoreTextFontEngine()
{
    CFRelease(cgFont);
    CFRelease(ctfont);
}

static QFont::Weight weightFromInteger(int weight)
{
    if (weight < 400)
        return QFont::Light;
    else if (weight < 600)
        return QFont::Normal;
    else if (weight < 700)
        return QFont::DemiBold;
    else if (weight < 800)
        return QFont::Bold;
    else
        return QFont::Black;
}

int getTraitValue(CFDictionaryRef allTraits, CFStringRef trait)
{
    if (CFDictionaryContainsKey(allTraits, trait)) {
        CFNumberRef traitNum = (CFNumberRef) CFDictionaryGetValue(allTraits, trait);
        float v = 0;
        CFNumberGetValue(traitNum, kCFNumberFloatType, &v);
        // the value we get from CFNumberRef is from -1.0 to 1.0
        int value = v * 500 + 500;
        return value;
    }

    return 0;
}

void QCoreTextFontEngine::init()
{
    Q_ASSERT(ctfont != NULL);
    Q_ASSERT(cgFont != NULL);

    QCFString family = CTFontCopyFamilyName(ctfont);
    fontDef.family = family;

    QCFString styleName = (CFStringRef) CTFontCopyAttribute(ctfont, kCTFontStyleNameAttribute);
    fontDef.styleName = styleName;

    synthesisFlags = 0;
    CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(ctfont);

#if defined(Q_OS_IOS) || MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (supportsColorGlyphs() && (traits & kCTFontColorGlyphsTrait))
        glyphFormat = QFontEngine::Format_ARGB;
    else
#endif
        glyphFormat = defaultGlyphFormat;

    if (traits & kCTFontItalicTrait)
        fontDef.style = QFont::StyleItalic;

    CFDictionaryRef allTraits = CTFontCopyTraits(ctfont);
    fontDef.weight = weightFromInteger(getTraitValue(allTraits, kCTFontWeightTrait));
    int slant = getTraitValue(allTraits, kCTFontSlantTrait);
    if (slant > 500 && !(traits & kCTFontItalicTrait))
        fontDef.style = QFont::StyleOblique;
    CFRelease(allTraits);

    if (fontDef.weight >= QFont::Bold && !(traits & kCTFontBoldTrait))
        synthesisFlags |= SynthesizedBold;
    // XXX: we probably don't need to synthesis italic for oblique font
    if (fontDef.style != QFont::StyleNormal && !(traits & kCTFontItalicTrait))
        synthesisFlags |= SynthesizedItalic;

    avgCharWidth = 0;
    QByteArray os2Table = getSfntTable(MAKE_TAG('O', 'S', '/', '2'));
    unsigned emSize = CTFontGetUnitsPerEm(ctfont);
    if (os2Table.size() >= 10) {
        fsType = qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(os2Table.constData() + 8));
        // qAbs is a workaround for weird fonts like Lucida Grande
        qint16 width = qAbs(qFromBigEndian<qint16>(reinterpret_cast<const uchar *>(os2Table.constData() + 2)));
        avgCharWidth = QFixed::fromReal(width * fontDef.pixelSize / emSize);
    } else
        avgCharWidth = QFontEngine::averageCharWidth();

    cache_cost = (CTFontGetAscent(ctfont) + CTFontGetDescent(ctfont)) * avgCharWidth.toInt() * 2000;

    // HACK hb_coretext requires both CTFont and CGFont but user_data is only void*
    Q_ASSERT((void *)(&ctfont + 1) == (void *)&cgFont);
    faceData.user_data = &ctfont;
    faceData.get_font_table = ct_getSfntTable;
}

glyph_t QCoreTextFontEngine::glyphIndex(uint ucs4) const
{
    int len = 0;

    QChar str[2];
    if (Q_UNLIKELY(QChar::requiresSurrogates(ucs4))) {
        str[len++] = QChar(QChar::highSurrogate(ucs4));
        str[len++] = QChar(QChar::lowSurrogate(ucs4));
    } else {
        str[len++] = QChar(ucs4);
    }

    CGGlyph glyphIndices[2];

    CTFontGetGlyphsForCharacters(ctfont, (const UniChar *)str, glyphIndices, len);

    return glyphIndices[0];
}

bool QCoreTextFontEngine::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs,
                                       int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    QVarLengthArray<CGGlyph> cgGlyphs(len);
    CTFontGetGlyphsForCharacters(ctfont, (const UniChar*)str, cgGlyphs.data(), len);

    int glyph_pos = 0;
    for (int i = 0; i < len; ++i) {
        if (cgGlyphs[i]) {
            glyphs->glyphs[glyph_pos] = cgGlyphs[i];
            if (glyph_pos < i)
                cgGlyphs[glyph_pos] = cgGlyphs[i];
        }
        glyph_pos++;

        // If it's a non-BMP char, skip the lower part of surrogate pair and go
        // directly to the next char without increasing glyph_pos
        if (str[i].isHighSurrogate() && i < len-1 && str[i+1].isLowSurrogate())
            ++i;
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    if (flags & GlyphIndicesOnly)
        return true;

    QVarLengthArray<CGSize> advances(glyph_pos);
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontHorizontalOrientation, cgGlyphs.data(), advances.data(), glyph_pos);

    for (int i = 0; i < glyph_pos; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            continue;
        glyphs->advances[i] = QFixed::fromReal(advances[i].width);
    }

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        for (int i = 0; i < glyph_pos; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
    return true;
}

glyph_metrics_t QCoreTextFontEngine::boundingBox(const QGlyphLayout &glyphs)
{
    QFixed w;
    bool round = fontDef.styleStrategy & QFont::ForceIntegerMetrics;

    for (int i = 0; i < glyphs.numGlyphs; ++i) {
        w += round ? glyphs.effectiveAdvance(i).round()
                   : glyphs.effectiveAdvance(i);
    }
    return glyph_metrics_t(0, -(ascent()), w - lastRightBearing(glyphs, round), ascent()+descent(), w, 0);
}

glyph_metrics_t QCoreTextFontEngine::boundingBox(glyph_t glyph)
{
    glyph_metrics_t ret;
    CGGlyph g = glyph;
    CGRect rect = CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontHorizontalOrientation, &g, 0, 1);
    if (synthesisFlags & QFontEngine::SynthesizedItalic) {
        rect.size.width += rect.size.height * SYNTHETIC_ITALIC_SKEW;
    }
    ret.width = QFixed::fromReal(rect.size.width);
    ret.height = QFixed::fromReal(rect.size.height);
    ret.x = QFixed::fromReal(rect.origin.x);
    ret.y = -QFixed::fromReal(rect.origin.y) - ret.height;
    CGSize advances[1];
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontHorizontalOrientation, &g, advances, 1);
    ret.xoff = QFixed::fromReal(advances[0].width);
    ret.yoff = QFixed::fromReal(advances[0].height);

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        ret.xoff = ret.xoff.round();
        ret.yoff = ret.yoff.round();
    }

    return ret;
}

QFixed QCoreTextFontEngine::ascent() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetAscent(ctfont)).round()
            : QFixed::fromReal(CTFontGetAscent(ctfont));
}
QFixed QCoreTextFontEngine::descent() const
{
    QFixed d = QFixed::fromReal(CTFontGetDescent(ctfont));
    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
        d = d.round();

    return d;
}
QFixed QCoreTextFontEngine::leading() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetLeading(ctfont)).round()
            : QFixed::fromReal(CTFontGetLeading(ctfont));
}
QFixed QCoreTextFontEngine::xHeight() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetXHeight(ctfont)).round()
            : QFixed::fromReal(CTFontGetXHeight(ctfont));
}

QFixed QCoreTextFontEngine::averageCharWidth() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? avgCharWidth.round() : avgCharWidth;
}

qreal QCoreTextFontEngine::maxCharWidth() const
{
    // ### FIXME: 'W' might not be the widest character, but this is better than nothing
    const glyph_t glyph = glyphIndex('W');
    glyph_metrics_t bb = const_cast<QCoreTextFontEngine *>(this)->boundingBox(glyph);
    return bb.xoff.toReal();
}

qreal QCoreTextFontEngine::minLeftBearing() const
{
    return 0;
}

qreal QCoreTextFontEngine::minRightBearing() const
{
    return 0;
}

void QCoreTextFontEngine::draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight)
{
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;
    matrix.translate(x, y);
    getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    CGContextSetFontSize(ctx, fontDef.pixelSize);

    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);

    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, -1, 0, -paintDeviceHeight);

    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    CGContextSetTextMatrix(ctx, cgMatrix);

    CGContextSetTextDrawingMode(ctx, kCGTextFill);


    QVarLengthArray<CGSize> advances(glyphs.size());
    QVarLengthArray<CGGlyph> cgGlyphs(glyphs.size());

    for (int i = 0; i < glyphs.size() - 1; ++i) {
        advances[i].width = (positions[i + 1].x - positions[i].x).toReal();
        advances[i].height = (positions[i + 1].y - positions[i].y).toReal();
        cgGlyphs[i] = glyphs[i];
    }
    advances[glyphs.size() - 1].width = 0;
    advances[glyphs.size() - 1].height = 0;
    cgGlyphs[glyphs.size() - 1] = glyphs[glyphs.size() - 1];

    CGContextSetFont(ctx, cgFont);
    //NSLog(@"Font inDraw %@  ctfont %@", CGFontCopyFullName(cgFont), CTFontCopyFamilyName(ctfont));

    CGContextSetTextPosition(ctx, positions[0].x.toReal(), positions[0].y.toReal());

    CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, positions[0].x.toReal() + 0.5 * lineThickness().toReal(),
                                 positions[0].y.toReal());

        CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());
    }

    CGContextSetTextMatrix(ctx, oldTextMatrix);
}

struct ConvertPathInfo
{
    ConvertPathInfo(QPainterPath *newPath, const QPointF &newPos) : path(newPath), pos(newPos) {}
    QPainterPath *path;
    QPointF pos;
};

static void convertCGPathToQPainterPath(void *info, const CGPathElement *element)
{
    ConvertPathInfo *myInfo = static_cast<ConvertPathInfo *>(info);
    switch(element->type) {
        case kCGPathElementMoveToPoint:
            myInfo->path->moveTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y());
            break;
        case kCGPathElementAddLineToPoint:
            myInfo->path->lineTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y());
            break;
        case kCGPathElementAddQuadCurveToPoint:
            myInfo->path->quadTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y(),
                                 element->points[1].x + myInfo->pos.x(),
                                 element->points[1].y + myInfo->pos.y());
            break;
        case kCGPathElementAddCurveToPoint:
            myInfo->path->cubicTo(element->points[0].x + myInfo->pos.x(),
                                  element->points[0].y + myInfo->pos.y(),
                                  element->points[1].x + myInfo->pos.x(),
                                  element->points[1].y + myInfo->pos.y(),
                                  element->points[2].x + myInfo->pos.x(),
                                  element->points[2].y + myInfo->pos.y());
            break;
        case kCGPathElementCloseSubpath:
            myInfo->path->closeSubpath();
            break;
        default:
            qDebug() << "Unhandled path transform type: " << element->type;
    }

}

void QCoreTextFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nGlyphs,
                                          QPainterPath *path, QTextItem::RenderFlags)
{
    if (glyphFormat == QFontEngine::Format_ARGB)
        return; // We can't convert color-glyphs to path

    CGAffineTransform cgMatrix = CGAffineTransformIdentity;
    cgMatrix = CGAffineTransformScale(cgMatrix, 1, -1);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    for (int i = 0; i < nGlyphs; ++i) {
        QCFType<CGPathRef> cgpath = CTFontCreatePathForGlyph(ctfont, glyphs[i], &cgMatrix);
        ConvertPathInfo info(path, positions[i].toPointF());
        CGPathApply(cgpath, &info, convertCGPathToQPainterPath);
    }
}

static void qcoretextfontengine_scaleMetrics(glyph_metrics_t &br, const QTransform &matrix)
{
    if (matrix.isScaling()) {
        qreal hscale = matrix.m11();
        qreal vscale = matrix.m22();
        br.width  = QFixed::fromReal(br.width.toReal() * hscale);
        br.height = QFixed::fromReal(br.height.toReal() * vscale);
        br.x      = QFixed::fromReal(br.x.toReal() * hscale);
        br.y      = QFixed::fromReal(br.y.toReal() * vscale);
    }
}

glyph_metrics_t QCoreTextFontEngine::alphaMapBoundingBox(glyph_t glyph, QFixed subPixelPosition, const QTransform &matrix, GlyphFormat format)
{
    if (matrix.type() > QTransform::TxScale)
        return QFontEngine::alphaMapBoundingBox(glyph, subPixelPosition, matrix, format);

    glyph_metrics_t br = boundingBox(glyph);
    qcoretextfontengine_scaleMetrics(br, matrix);

    // Normalize width and height
    if (br.width < 0)
        br.width = -br.width;
    if (br.height < 0)
        br.height = -br.height;

    if (format == QFontEngine::Format_A8 || format == QFontEngine::Format_A32) {
        // Drawing a glyph at x-position 0 with anti-aliasing enabled
        // will potentially fill the pixel to the left of 0, as the
        // coordinates are not aligned to the center of pixels. To
        // prevent clipping of this pixel we need to shift the glyph
        // in the bitmap one pixel to the right. The shift needs to
        // be reflected in the glyph metrics as well, so that the final
        // position of the glyph is correct, which is why doing the
        // shift in imageForGlyph() is not enough.
        br.x -= 1;

        // As we've shifted the glyph one pixel to the right, we need
        // to expand the width of the alpha map bounding box as well.
        br.width += 1;

        // But we have the same anti-aliasing problem on the right
        // hand side of the glyph, eg. if the width of the glyph
        // results in the bounding rect landing between two pixels.
        // We pad the bounding rect again to account for the possible
        // anti-aliased drawing.
        br.width += 1;

        // We also shift the glyph to right right based on the subpixel
        // position, so we pad the bounding box to take account for the
        // subpixel positions that may result in the glyph being drawn
        // one pixel to the right of the 0-subpixel position.
        br.width += 1;

        // The same same logic as for the x-position needs to be applied
        // to the y-position, except we don't need to compensate for
        // the subpixel positioning.
        br.y -= 1;
        br.height += 2;
    }

    return br;
}


QImage QCoreTextFontEngine::imageForGlyph(glyph_t glyph, QFixed subPixelPosition, bool aa, const QTransform &matrix)
{

    glyph_metrics_t br = alphaMapBoundingBox(glyph, subPixelPosition, matrix, glyphFormat);

    bool isColorGlyph = glyphFormat == QFontEngine::Format_ARGB;
    QImage::Format imageFormat = isColorGlyph ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    QImage im(br.width.ceil().toInt(), br.height.ceil().toInt(), imageFormat);
    im.fill(0);

    if (!im.width() || !im.height())
        return im;

#ifndef Q_OS_IOS
    CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
#else
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
#endif
    uint cgflags = isColorGlyph ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif

    CGContextRef ctx = CGBitmapContextCreate(im.bits(), im.width(), im.height(),
                                             8, im.bytesPerLine(), colorspace,
                                             cgflags);
    Q_ASSERT(ctx);
    CGContextSetFontSize(ctx, fontDef.pixelSize);
    const bool antialias = (aa || fontDef.pointSize > antialiasingThreshold) && !(fontDef.styleStrategy & QFont::NoAntialias);
    CGContextSetShouldAntialias(ctx, antialias);
    const bool smoothing = antialias && !(fontDef.styleStrategy & QFont::NoSubpixelAntialias);
    CGContextSetShouldSmoothFonts(ctx, smoothing);

    CGAffineTransform cgMatrix = CGAffineTransformIdentity;

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    if (!isColorGlyph) // CTFontDrawGlyphs incorporates the font's matrix already
        cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    if (matrix.isScaling())
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMakeScale(matrix.m11(), matrix.m22()));

    CGGlyph cgGlyph = glyph;
    qreal pos_x = -br.x.truncate() + subPixelPosition.toReal();
    qreal pos_y = im.height() + br.y.toReal();

    if (!isColorGlyph) {
        CGContextSetTextMatrix(ctx, cgMatrix);
        CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
        CGContextSetTextDrawingMode(ctx, kCGTextFill);
        CGContextSetFont(ctx, cgFont);
        CGContextSetTextPosition(ctx, pos_x, pos_y);

        CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &CGSizeZero, 1);

        if (synthesisFlags & QFontEngine::SynthesizedBold) {
            CGContextSetTextPosition(ctx, pos_x + 0.5 * lineThickness().toReal(), pos_y);
            CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &CGSizeZero, 1);
        }
    }
#if defined(Q_OS_IOS) || MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    else if (supportsColorGlyphs()) {
        // CGContextSetTextMatrix does not work with color glyphs, so we use
        // the CTM instead. This means we must translate the CTM as well, to
        // set the glyph position, instead of using CGContextSetTextPosition.
        CGContextTranslateCTM(ctx, pos_x, pos_y);
        CGContextConcatCTM(ctx, cgMatrix);

        // CGContextShowGlyphsWithAdvances does not support the 'sbix' color-bitmap
        // glyphs in the Apple Color Emoji font, so we use CTFontDrawGlyphs instead.
        CTFontDrawGlyphs(ctfont, &cgGlyph, &CGPointZero, 1, ctx);
    }
#endif

    CGContextRelease(ctx);
    CGColorSpaceRelease(colorspace);

    return im;
}

QImage QCoreTextFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition)
{
    return alphaMapForGlyph(glyph, subPixelPosition, QTransform());
}

QImage QCoreTextFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &x)
{
    if (x.type() > QTransform::TxScale)
        return QFontEngine::alphaMapForGlyph(glyph, subPixelPosition, x);

    QImage im = imageForGlyph(glyph, subPixelPosition, false, x);

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uint *src = (uint*) im.scanLine(y);
        uchar *dst = indexed.scanLine(y);
        for (int x=0; x<im.width(); ++x) {
            *dst = qGray(*src);
            ++dst;
            ++src;
        }
    }

    return indexed;
}

QImage QCoreTextFontEngine::alphaRGBMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &x)
{
    if (x.type() > QTransform::TxScale)
        return QFontEngine::alphaRGBMapForGlyph(glyph, subPixelPosition, x);

    QImage im = imageForGlyph(glyph, subPixelPosition, true, x);
    qGamma_correct_back_to_linear_cs(&im);
    return im;
}

QImage QCoreTextFontEngine::bitmapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t)
{
    if (t.type() > QTransform::TxScale)
        return QFontEngine::bitmapForGlyph(glyph, subPixelPosition, t);

    return imageForGlyph(glyph, subPixelPosition, true, t);
}

void QCoreTextFontEngine::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    int i, numGlyphs = glyphs->numGlyphs;
    QVarLengthArray<CGGlyph> cgGlyphs(numGlyphs);

    for (i = 0; i < numGlyphs; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            cgGlyphs[i] = 0;
        else
            cgGlyphs[i] = glyphs->glyphs[i];
    }

    loadAdvancesForGlyphs(ctfont, cgGlyphs, glyphs, numGlyphs, flags, fontDef);
}

QFontEngine::FaceId QCoreTextFontEngine::faceId() const
{
    FaceId result;
    result.index = 0;
    QCFString name = CTFontCopyName(ctfont, kCTFontUniqueNameKey);
    result.filename = QCFString::toQString(name).toUtf8();

    return result;
}

bool QCoreTextFontEngine::canRender(const QChar *string, int len) const
{
    QVarLengthArray<CGGlyph> cgGlyphs(len);
    return CTFontGetGlyphsForCharacters(ctfont, (const UniChar *) string, cgGlyphs.data(), len);
}

bool QCoreTextFontEngine::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    return ct_getSfntTable((void *)&ctfont, tag, buffer, length);
}

void QCoreTextFontEngine::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metric)
{
    CGAffineTransform cgMatrix = CGAffineTransformIdentity;

    qreal emSquare = CTFontGetUnitsPerEm(ctfont);
    qreal scale = emSquare / CTFontGetSize(ctfont);
    cgMatrix = CGAffineTransformScale(cgMatrix, scale, -scale);

    QCFType<CGPathRef> cgpath = CTFontCreatePathForGlyph(ctfont, (CGGlyph) glyph, &cgMatrix);
    ConvertPathInfo info(path, QPointF(0,0));
    CGPathApply(cgpath, &info, convertCGPathToQPainterPath);

    *metric = boundingBox(glyph);
    // scale the metrics too
    metric->width  = QFixed::fromReal(metric->width.toReal() * scale);
    metric->height = QFixed::fromReal(metric->height.toReal() * scale);
    metric->x      = QFixed::fromReal(metric->x.toReal() * scale);
    metric->y      = QFixed::fromReal(metric->y.toReal() * scale);
    metric->xoff   = QFixed::fromReal(metric->xoff.toReal() * scale);
    metric->yoff   = QFixed::fromReal(metric->yoff.toReal() * scale);
}

QFixed QCoreTextFontEngine::emSquareSize() const
{
    return QFixed::QFixed(int(CTFontGetUnitsPerEm(ctfont)));
}

QFontEngine *QCoreTextFontEngine::cloneWithSize(qreal pixelSize) const
{
    QFontDef newFontDef = fontDef;
    newFontDef.pixelSize = pixelSize;
    newFontDef.pointSize = pixelSize * 72.0 / qt_defaultDpi();

    return new QCoreTextFontEngine(cgFont, newFontDef);
}

bool QCoreTextFontEngine::supportsTransformation(const QTransform &transform) const
{
    if (transform.type() < QTransform::TxScale)
        return true;
    else if (transform.type() == QTransform::TxScale &&
             transform.m11() >= 0 && transform.m22() >= 0)
        return true;
    else
        return false;
}

QFontEngine::Properties QCoreTextFontEngine::properties() const
{
    Properties result;

    QCFString psName, copyright;
    psName = CTFontCopyPostScriptName(ctfont);
    copyright = CTFontCopyName(ctfont, kCTFontCopyrightNameKey);
    result.postscriptName = QCFString::toQString(psName).toUtf8();
    result.copyright = QCFString::toQString(copyright).toUtf8();

    qreal emSquare = CTFontGetUnitsPerEm(ctfont);
    qreal scale = emSquare / CTFontGetSize(ctfont);

    CGRect cgRect = CTFontGetBoundingBox(ctfont);
    result.boundingBox = QRectF(cgRect.origin.x * scale,
                                -CTFontGetAscent(ctfont) * scale,
                                cgRect.size.width * scale,
                                cgRect.size.height * scale);

    result.emSquare = emSquareSize();
    result.ascent = QFixed::fromReal(CTFontGetAscent(ctfont) * scale);
    result.descent = QFixed::fromReal(CTFontGetDescent(ctfont) * scale);
    result.leading = QFixed::fromReal(CTFontGetLeading(ctfont) * scale);
    result.italicAngle = QFixed::fromReal(CTFontGetSlantAngle(ctfont));
    result.capHeight = QFixed::fromReal(CTFontGetCapHeight(ctfont) * scale);
    result.lineWidth = QFixed::fromReal(CTFontGetUnderlineThickness(ctfont) * scale);

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        result.ascent = result.ascent.round();
        result.descent = result.descent.round();
        result.leading = result.leading.round();
        result.italicAngle = result.italicAngle.round();
        result.capHeight = result.capHeight.round();
        result.lineWidth = result.lineWidth.round();
    }

    return result;
}

QT_END_NAMESPACE
