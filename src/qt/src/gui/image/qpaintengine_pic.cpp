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

#include "private/qpaintengine_p.h"
#include "private/qpainter_p.h"
#include "private/qpicture_p.h"
#include "private/qfont_p.h"

#ifndef QT_NO_PICTURE

#include "qbuffer.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qmath.h"
#include "qpaintengine_pic_p.h"
#include "qpicture.h"
#include "qpolygon.h"
#include "qrect.h"
#include <private/qtextengine_p.h>

//#define QT_PICTURE_DEBUG
#include <qdebug.h>


QT_BEGIN_NAMESPACE

class QPicturePaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPicturePaintEngine)
public:
    QDataStream s;
    QPainter *pt;
    QPicturePrivate *pic_d;
};

QPicturePaintEngine::QPicturePaintEngine()
    : QPaintEngine(*(new QPicturePaintEnginePrivate), AllFeatures)
{
    Q_D(QPicturePaintEngine);
    d->pt = 0;
}

QPicturePaintEngine::QPicturePaintEngine(QPaintEnginePrivate &dptr)
    : QPaintEngine(dptr, AllFeatures)
{
    Q_D(QPicturePaintEngine);
    d->pt = 0;
}

QPicturePaintEngine::~QPicturePaintEngine()
{
}

bool QPicturePaintEngine::begin(QPaintDevice *pd)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << "QPicturePaintEngine::begin()";
#endif
    Q_ASSERT(pd);
    QPicture *pic = static_cast<QPicture *>(pd);

    d->pdev = pd;
    d->pic_d = pic->d_func();
    Q_ASSERT(d->pic_d);

    d->s.setDevice(&d->pic_d->pictb);
    d->s.setVersion(d->pic_d->formatMajor);

    d->pic_d->pictb.open(QIODevice::WriteOnly | QIODevice::Truncate);
    d->s.writeRawData(qt_mfhdr_tag, 4);
    d->s << (quint16) 0 << (quint16) d->pic_d->formatMajor << (quint16) d->pic_d->formatMinor;
    d->s << (quint8) QPicturePrivate::PdcBegin << (quint8) sizeof(qint32);
    d->pic_d->brect = QRect();
    if (d->pic_d->formatMajor >= 4) {
        QRect r = pic->boundingRect();
        d->s << (qint32) r.left() << (qint32) r.top() << (qint32) r.width()
             << (qint32) r.height();
    }
    d->pic_d->trecs = 0;
    d->s << (quint32)d->pic_d->trecs; // total number of records
    d->pic_d->formatOk = false;
    setActive(true);
    return true;
}

bool QPicturePaintEngine::end()
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << "QPicturePaintEngine::end()";
#endif
    d->pic_d->trecs++;
    d->s << (quint8) QPicturePrivate::PdcEnd << (quint8) 0;
    int cs_start = sizeof(quint32);                // pos of checksum word
    int data_start = cs_start + sizeof(quint16);
    int brect_start = data_start + 2*sizeof(qint16) + 2*sizeof(quint8);
    int pos = d->pic_d->pictb.pos();
    d->pic_d->pictb.seek(brect_start);
    if (d->pic_d->formatMajor >= 4) { // bounding rectangle
        QRect r = static_cast<QPicture *>(d->pdev)->boundingRect();
        d->s << (qint32) r.left() << (qint32) r.top() << (qint32) r.width()
             << (qint32) r.height();
    }
    d->s << (quint32) d->pic_d->trecs;                        // write number of records
    d->pic_d->pictb.seek(cs_start);
    QByteArray buf = d->pic_d->pictb.buffer();
    quint16 cs = (quint16) qChecksum(buf.constData() + data_start, pos - data_start);
    d->s << cs;                                // write checksum
    d->pic_d->pictb.close();
    setActive(false);
    return true;
}

#define SERIALIZE_CMD(c) \
    d->pic_d->trecs++; \
    d->s << (quint8) c; \
    d->s << (quint8) 0; \
    pos = d->pic_d->pictb.pos()

void QPicturePaintEngine::updatePen(const QPen &pen)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updatePen(): width:" << pen.width() << "style:"
             << pen.style() << "color:" << pen.color();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetPen);
    if (d->pic_d->in_memory_only) {
        int index = d->pic_d->pen_list.size();
        d->pic_d->pen_list.append(pen);
        d->s << index;
    } else {
        d->s << pen;
    }
    writeCmdLength(pos, QRect(), false);
}

void QPicturePaintEngine::updateCompositionMode(QPainter::CompositionMode cmode)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateCompositionMode():" << cmode;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetCompositionMode);
    d->s << (qint32)cmode;
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateClipEnabled(bool enabled)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateClipEnabled():" << enabled;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetClipEnabled);
    d->s << enabled;
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateOpacity(qreal opacity)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateOpacity():" << opacity;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetOpacity);
    d->s << double(opacity);
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateBrush(const QBrush &brush)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateBrush(): style:" << brush.style();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetBrush);
    if (d->pic_d->in_memory_only) {
        int index = d->pic_d->brush_list.size();
        d->pic_d->brush_list.append(brush);
        d->s << index;
    } else {
        d->s << brush;
    }
    writeCmdLength(pos, QRect(), false);
}

void QPicturePaintEngine::updateBrushOrigin(const QPointF &p)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateBrushOrigin(): " << p;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetBrushOrigin);
    d->s << p;
    writeCmdLength(pos, QRect(), false);
}

void QPicturePaintEngine::updateFont(const QFont &font)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateFont(): pt sz:" << font.pointSize();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetFont);
    QFont fnt = font;
    d->s << fnt;
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateBackground(Qt::BGMode bgMode, const QBrush &bgBrush)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateBackground(): mode:" << bgMode << "style:" << bgBrush.style();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetBkColor);
    d->s << bgBrush.color();
    writeCmdLength(pos, QRect(), false);

    SERIALIZE_CMD(QPicturePrivate::PdcSetBkMode);
    d->s << (qint8) bgMode;
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateMatrix(const QTransform &matrix)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateMatrix():" << matrix;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetWMatrix);
    d->s << matrix << (qint8) false;
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateClipRegion(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateClipRegion(): op:" << op
             << "bounding rect:" << region.boundingRect();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetClipRegion);
    d->s << region << qint8(op);
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateClipPath(const QPainterPath &path, Qt::ClipOperation op)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateClipPath(): op:" << op
             << "bounding rect:" << path.boundingRect();
#endif
    int pos;

    SERIALIZE_CMD(QPicturePrivate::PdcSetClipPath);
    d->s << path << qint8(op);
    writeCmdLength(pos, QRectF(), false);
}

void QPicturePaintEngine::updateRenderHints(QPainter::RenderHints hints)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> updateRenderHints(): " << hints;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcSetRenderHint);
    d->s << (quint32) hints;
    writeCmdLength(pos, QRect(), false);
}

void QPicturePaintEngine::writeCmdLength(int pos, const QRectF &r, bool corr)
{
    Q_D(QPicturePaintEngine);
    int newpos = d->pic_d->pictb.pos();            // new position
    int length = newpos - pos;
    QRectF br(r);

    if (length < 255) {                         // write 8-bit length
        d->pic_d->pictb.seek(pos - 1);             // position to right index
        d->s << (quint8)length;
    } else {                                    // write 32-bit length
        d->s << (quint32)0;                    // extend the buffer
        d->pic_d->pictb.seek(pos - 1);             // position to right index
        d->s << (quint8)255;                   // indicate 32-bit length
        char *p = d->pic_d->pictb.buffer().data();
        memmove(p+pos+4, p+pos, length);        // make room for 4 byte
        d->s << (quint32)length;
        newpos += 4;
    }
    d->pic_d->pictb.seek(newpos);                  // set to new position

    if (br.width() > 0.0 || br.height() > 0.0) {
        if (corr) {                             // widen bounding rect
            int w2 = painter()->pen().width() / 2;
            br.setCoords(br.left() - w2, br.top() - w2,
                         br.right() + w2, br.bottom() + w2);
        }
        br = painter()->transform().mapRect(br);
        if (painter()->hasClipping()) {
            QRect cr = painter()->clipRegion().boundingRect();
            br &= cr;
        }

        if (br.width() > 0.0 || br.height() > 0.0) {
            int minx = qFloor(br.left());
            int miny = qFloor(br.top());
            int maxx = qCeil(br.right());
            int maxy = qCeil(br.bottom());

            if (d->pic_d->brect.width() > 0 || d->pic_d->brect.height() > 0) {
                minx = qMin(minx, d->pic_d->brect.left());
                miny = qMin(miny, d->pic_d->brect.top());
                maxx = qMax(maxx, d->pic_d->brect.x() + d->pic_d->brect.width());
                maxy = qMax(maxy, d->pic_d->brect.y() + d->pic_d->brect.height());
                d->pic_d->brect = QRect(minx, miny, maxx - minx, maxy - miny);
            } else {
                d->pic_d->brect = QRect(minx, miny, maxx - minx, maxy - miny);
            }
        }
    }
}

void QPicturePaintEngine::drawEllipse(const QRectF &rect)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawEllipse():" << rect;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcDrawEllipse);
    d->s << rect;
    writeCmdLength(pos, rect, true);
}

void QPicturePaintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawPath():" << path.boundingRect();
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcDrawPath);
    d->s << path;
    writeCmdLength(pos, path.boundingRect(), true);
}

void QPicturePaintEngine::drawPolygon(const QPointF *points, int numPoints, PolygonDrawMode mode)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawPolygon(): size=" << numPoints;
#endif
    int pos;

    QPolygonF polygon;
    for (int i=0; i<numPoints; ++i)
        polygon << points[i];

    if (mode == PolylineMode) {
        SERIALIZE_CMD(QPicturePrivate::PdcDrawPolyline);
        d->s << polygon;
    } else {
        SERIALIZE_CMD(QPicturePrivate::PdcDrawPolygon);
        d->s << polygon;
        d->s << (qint8)(mode == OddEvenMode ? 0 : 1);
    }

    writeCmdLength(pos, polygon.boundingRect(), true);
}

void QPicturePaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawPixmap():" << r;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcDrawPixmap);

    if (d->pic_d->in_memory_only) {
        int index = d->pic_d->pixmap_list.size();
        d->pic_d->pixmap_list.append(pm);
        d->s << r << index << sr;
    } else {
        d->s << r << pm << sr;
    }
    writeCmdLength(pos, r, false);
}

void QPicturePaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawTiledPixmap():" << r << s;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcDrawTiledPixmap);
    if (d->pic_d->in_memory_only) {
        int index = d->pic_d->pixmap_list.size();
        d->pic_d->pixmap_list.append(pixmap);
        d->s << r << index << s;
    } else {
        d->s << r << pixmap << s;
    }
    writeCmdLength(pos, r, false);
}

void QPicturePaintEngine::drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                                    Qt::ImageConversionFlags flags)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawImage():" << r << sr;
#endif
    int pos;
    SERIALIZE_CMD(QPicturePrivate::PdcDrawImage);
    if (d->pic_d->in_memory_only) {
        int index = d->pic_d->image_list.size();
        d->pic_d->image_list.append(image);
        d->s << r << index << sr << (quint32) flags;
    } else {
        d->s << r << image << sr << (quint32) flags;
    }
    writeCmdLength(pos, r, false);
}

void QPicturePaintEngine::drawTextItem(const QPointF &p , const QTextItem &ti)
{
    Q_D(QPicturePaintEngine);
#ifdef QT_PICTURE_DEBUG
    qDebug() << " -> drawTextItem():" << p << ti.text();
#endif

    const QTextItemInt &si = static_cast<const QTextItemInt &>(ti);
    if (si.chars == 0)
        QPaintEngine::drawTextItem(p, ti); // Draw as path

    if (d->pic_d->formatMajor >= 9) {
        int pos;
        SERIALIZE_CMD(QPicturePrivate::PdcDrawTextItem);
        QFont fnt = ti.font();
        fnt.setUnderline(false);
        fnt.setStrikeOut(false);
        fnt.setOverline(false);

        qreal justificationWidth = 0;
        if (si.justified)
            justificationWidth = si.width.toReal();

        d->s << p << ti.text() << fnt << ti.renderFlags() << double(fnt.d->dpi)/qt_defaultDpi() << justificationWidth;
        writeCmdLength(pos, /*brect=*/QRectF(), /*corr=*/false);
    } else if (d->pic_d->formatMajor >= 8) {
        // old old (buggy) format
        int pos;
        SERIALIZE_CMD(QPicturePrivate::PdcDrawTextItem);
        d->s << QPointF(p.x(), p.y() - ti.ascent()) << ti.text() << ti.font() << ti.renderFlags();
        writeCmdLength(pos, /*brect=*/QRectF(), /*corr=*/false);
    } else {
        // old (buggy) format
        int pos;
        SERIALIZE_CMD(QPicturePrivate::PdcDrawText2);
        d->s << p << ti.text();
        writeCmdLength(pos, QRectF(p, QSizeF(1,1)), true);
    }
}

void QPicturePaintEngine::updateState(const QPaintEngineState &state)
{
    QPaintEngine::DirtyFlags flags = state.state();
    if (flags & DirtyPen) updatePen(state.pen());
    if (flags & DirtyBrush) updateBrush(state.brush());
    if (flags & DirtyBrushOrigin) updateBrushOrigin(state.brushOrigin());
    if (flags & DirtyFont) updateFont(state.font());
    if (flags & DirtyBackground) updateBackground(state.backgroundMode(), state.backgroundBrush());
    if (flags & DirtyTransform) updateMatrix(state.transform());
    if (flags & DirtyClipEnabled) updateClipEnabled(state.isClipEnabled());
    if (flags & DirtyClipRegion) updateClipRegion(state.clipRegion(), state.clipOperation());
    if (flags & DirtyClipPath) updateClipPath(state.clipPath(), state.clipOperation());
    if (flags & DirtyHints) updateRenderHints(state.renderHints());
    if (flags & DirtyCompositionMode) updateCompositionMode(state.compositionMode());
    if (flags & DirtyOpacity) updateOpacity(state.opacity());
}

QT_END_NAMESPACE

#endif // QT_NO_PICTURE
