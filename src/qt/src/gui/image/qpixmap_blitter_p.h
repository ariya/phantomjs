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

#ifndef QPIXMAP_BLITTER_P_H
#define QPIXMAP_BLITTER_P_H

#include <private/qpixmapdata_p.h>
#include <private/qpaintengine_blitter_p.h>

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT  QBlittablePixmapData : public QPixmapData
{
//     Q_DECLARE_PRIVATE(QBlittablePixmapData);
public:
    QBlittablePixmapData();
    ~QBlittablePixmapData();

    virtual QBlittable *createBlittable(const QSize &size, bool alpha) const = 0;
    QBlittable *blittable() const;
    void setBlittable(QBlittable *blittable);

    void resize(int width, int height);
    int metric(QPaintDevice::PaintDeviceMetric metric) const;
    void fill(const QColor &color);
    QImage *buffer();
    QImage toImage() const;
    bool hasAlphaChannel() const;
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);

    QPaintEngine *paintEngine() const;

    void markRasterOverlay(const QRectF &);
    void markRasterOverlay(const QPointF &, const QTextItem &);
    void markRasterOverlay(const QVectorPath &);
    void markRasterOverlay(const QPainterPath &);
    void markRasterOverlay(const QRect *rects, int rectCount);
    void markRasterOverlay(const QRectF *rects, int rectCount);
    void markRasterOverlay(const QPointF *points, int pointCount);
    void markRasterOverlay(const QPoint *points, int pointCount);
    void unmarkRasterOverlay(const QRectF &);

#ifdef QT_BLITTER_RASTEROVERLAY
    void mergeOverlay();
    void unmergeOverlay();
    QImage *overlay();

#endif //QT_BLITTER_RASTEROVERLAY
protected:
    QScopedPointer<QBlitterPaintEngine> m_engine;
    QScopedPointer<QBlittable> m_blittable;
    bool m_alpha;

#ifdef QT_BLITTER_RASTEROVERLAY
    QImage *m_rasterOverlay;
    QImage *m_unmergedCopy;
    QColor m_overlayColor;

    void markRasterOverlayImpl(const QRectF &);
    void unmarkRasterOverlayImpl(const QRectF &);
    QRectF clipAndTransformRect(const QRectF &) const;
#endif //QT_BLITTER_RASTEROVERLAY

};

inline void QBlittablePixmapData::markRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(rect);
#else
   Q_UNUSED(rect)
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QVectorPath &path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    markRasterOverlayImpl(path.convertToPainterPath().boundingRect());
#else
    Q_UNUSED(path)
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QPointF &pos, const QTextItem &ti)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    QFontMetricsF fm(ti.font());
    QRectF rect = fm.tightBoundingRect(ti.text());
    rect.moveBottomLeft(pos);
    markRasterOverlay(rect);
#else
    Q_UNUSED(pos)
    Q_UNUSED(ti)
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QRect *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects)
    Q_UNUSED(rectCount)
#endif
}
inline void QBlittablePixmapData::markRasterOverlay(const QRectF *rects, int rectCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    for (int i = 0; i < rectCount; i++) {
        markRasterOverlay(rects[i]);
    }
#else
    Q_UNUSED(rects)
    Q_UNUSED(rectCount)
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QPointF *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QPoint *points, int pointCount)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(points);
    Q_UNUSED(pointCount);
#endif
}

inline void QBlittablePixmapData::markRasterOverlay(const QPainterPath& path)
{
#ifdef QT_BLITTER_RASTEROVERLAY
#error "not ported yet"
#else
    Q_UNUSED(path);
#endif
}

inline void QBlittablePixmapData::unmarkRasterOverlay(const QRectF &rect)
{
#ifdef QT_BLITTER_RASTEROVERLAY
    unmarkRasterOverlayImpl(rect);
#else
    Q_UNUSED(rect)
#endif
}

QT_END_NAMESPACE
#endif // QT_NO_BLITTABLE
#endif // QPIXMAP_BLITTER_P_H
