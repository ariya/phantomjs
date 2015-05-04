/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFONTENGINE_CORETEXT_P_H
#define QFONTENGINE_CORETEXT_P_H

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

#include <private/qfontengine_p.h>
#include <private/qcore_mac_p.h>

#ifndef Q_OS_IOS
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

QT_BEGIN_NAMESPACE

class QRawFontPrivate;
class QCoreTextFontEngine : public QFontEngine
{
public:
    QCoreTextFontEngine(CTFontRef font, const QFontDef &def);
    QCoreTextFontEngine(CGFontRef font, const QFontDef &def);
    ~QCoreTextFontEngine();

    virtual glyph_t glyphIndex(uint ucs4) const;
    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const;
    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph);

    virtual QFixed ascent() const;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual QFixed xHeight() const;
    virtual qreal maxCharWidth() const;
    virtual QFixed averageCharWidth() const;

    virtual void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs,
                                 QPainterPath *path, QTextItem::RenderFlags);

    virtual bool canRender(const QChar *string, int len) const;

    virtual int synthesized() const { return synthesisFlags; }
    virtual bool supportsSubPixelPositions() const { return true; }

    void draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight);

    virtual FaceId faceId() const;
    virtual bool getSfntTableData(uint /*tag*/, uchar * /*buffer*/, uint * /*length*/) const;
    virtual void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics);
    virtual QImage alphaMapForGlyph(glyph_t, QFixed subPixelPosition);
    virtual QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t);
    virtual QImage alphaRGBMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, QFixed, const QTransform &matrix, GlyphFormat);
    virtual QImage bitmapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    virtual qreal minRightBearing() const;
    virtual qreal minLeftBearing() const;
    virtual QFixed emSquareSize() const;

    bool supportsTransformation(const QTransform &transform) const;

    virtual QFontEngine *cloneWithSize(qreal pixelSize) const;
    virtual int glyphMargin(QFontEngine::GlyphFormat format) { Q_UNUSED(format); return 0; }

    static bool supportsColorGlyphs()
    {
#if defined(Q_OS_IOS)
        return true;
#elif MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  #if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
        return &CTFontDrawGlyphs;
  #else
        return true;
  #endif
#else
        return false;
#endif
    }

    static bool ct_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length);

    static int antialiasingThreshold;
    static QFontEngine::GlyphFormat defaultGlyphFormat;

private:
    friend class QRawFontPrivate;

    void init();
    QImage imageForGlyph(glyph_t glyph, QFixed subPixelPosition, bool colorful, const QTransform &m);
    CTFontRef ctfont;
    CGFontRef cgFont;
    int synthesisFlags;
    CGAffineTransform transform;
    QFixed avgCharWidth;
};

CGAffineTransform qt_transform_from_fontdef(const QFontDef &fontDef);

QT_END_NAMESPACE

#endif // QFONTENGINE_CORETEXT_P_H
