/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFONTENGINEDIRECTWRITE_H
#define QFONTENGINEDIRECTWRITE_H

#ifndef QT_NO_DIRECTWRITE

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

#include "private/qfontengine_p.h"

struct IDWriteFont ;
struct IDWriteFontFace ;
struct IDWriteFactory ;
struct IDWriteBitmapRenderTarget ;
struct IDWriteGdiInterop ;

QT_BEGIN_NAMESPACE

class QFontEngineDirectWrite : public QFontEngine
{
    Q_OBJECT
public:
    explicit QFontEngineDirectWrite(IDWriteFactory *directWriteFactory,
                                    IDWriteFontFace *directWriteFontFace,
                                    qreal pixelSize);
    ~QFontEngineDirectWrite();

    QFixed lineThickness() const;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const;
    QFixed emSquareSize() const;

    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const;
    void recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags) const;

    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags);

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    glyph_metrics_t boundingBox(glyph_t g);
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph,
                                        QFixed subPixelPosition,
                                        const QTransform &matrix,
                                        GlyphFormat format);

    QFixed ascent() const;
    QFixed descent() const;
    QFixed leading() const;
    QFixed xHeight() const;
    qreal maxCharWidth() const;

    const char *name() const;

    bool supportsSubPixelPositions() const;

    QImage alphaMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    QImage alphaRGBMapForGlyph(glyph_t t, QFixed subPixelPosition, int margin,
                               const QTransform &xform);

    QFontEngine *cloneWithSize(qreal pixelSize) const;

    bool canRender(const QChar *string, int len);
    Type type() const;

private:
    friend class QRawFontPrivate;

    QImage imageForGlyph(glyph_t t, QFixed subPixelPosition, int margin, const QTransform &xform);
    void collectMetrics();

    IDWriteFontFace *m_directWriteFontFace;
    IDWriteFactory *m_directWriteFactory;
    IDWriteBitmapRenderTarget *m_directWriteBitmapRenderTarget;

    QFixed m_lineThickness;
    int m_unitsPerEm;
    QFixed m_ascent;
    QFixed m_descent;
    QFixed m_xHeight;
    QFixed m_lineGap;
    FaceId m_faceId;
};

QT_END_NAMESPACE

#endif // QT_NO_DIRECTWRITE

#endif // QFONTENGINEDIRECTWRITE_H
