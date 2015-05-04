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

#ifndef QPAINTENGINE_BLITTER_P_H
#define QPAINTENGINE_BLITTER_P_H

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

#include "private/qpaintengine_raster_p.h"

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class QBlitterPaintEnginePrivate;
class QBlittablePlatformPixmap;
class QBlittable;

class Q_GUI_EXPORT QBlitterPaintEngine : public QRasterPaintEngine
{
    Q_DECLARE_PRIVATE(QBlitterPaintEngine);
public:
    QBlitterPaintEngine(QBlittablePlatformPixmap *p);

    virtual QPaintEngine::Type type() const { return Blitter; }

    virtual bool begin(QPaintDevice *pdev);
    virtual bool end();

    // Call down into QBlittable
    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void fillRect(const QRectF &rect, const QBrush &brush);
    virtual void fillRect(const QRectF &rect, const QColor &color);
    virtual void drawRects(const QRect *rects, int rectCount);
    virtual void drawRects(const QRectF *rects, int rectCount);
    void drawPixmap(const QPointF &p, const QPixmap &pm);
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);

    // State tracking
    void setState(QPainterState *s);
    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void brushChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();

    // Override to lock the QBlittable before using raster
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode);
    void fillPath(const QPainterPath &path, QSpanData *fillData);
    void fillPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void drawEllipse(const QRectF &rect);
    void drawImage(const QPointF &p, const QImage &img);
    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &sr);
    void drawTextItem(const QPointF &p, const QTextItem &textItem);
    void drawPoints(const QPointF *points, int pointCount);
    void drawPoints(const QPoint *points, int pointCount);
    void stroke(const QVectorPath &path, const QPen &pen);
    void drawStaticTextItem(QStaticTextItem *);
    bool drawCachedGlyphs(int numGlyphs, const glyph_t *glyphs, const QFixedPoint *positions, QFontEngine *fontEngine);
};

QT_END_NAMESPACE
#endif //QT_NO_BLITTABLE
#endif // QPAINTENGINE_BLITTER_P_H

