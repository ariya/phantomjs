/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qpixmap_blitter_p.h"

#include <qpainter.h>
#include <qimage.h>

#include <private/qapplication_p.h>
#include <private/qgraphicssystem_p.h>
#include <private/qblittable_p.h>

#include <private/qdrawhelper_p.h>
#include <private/qfont_p.h>

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

static int global_ser_no = 0;

QBlittablePixmapData::QBlittablePixmapData()
    : QPixmapData(QPixmapData::PixmapType,BlitterClass)
    , m_alpha(false)
#ifdef QT_BLITTER_RASTEROVERLAY
    ,m_rasterOverlay(0), m_unmergedCopy(0)
#endif //QT_BLITTER_RASTEROVERLAY
{
    setSerialNumber(++global_ser_no);
}

QBlittablePixmapData::~QBlittablePixmapData()
{
#ifdef QT_BLITTER_RASTEROVERLAY
    delete m_rasterOverlay;
    delete m_unmergedCopy;
#endif //QT_BLITTER_RASTEROVERLAY
}

QBlittable *QBlittablePixmapData::blittable() const
{
    if (!m_blittable) {
        QBlittablePixmapData *that = const_cast<QBlittablePixmapData *>(this);
        that->m_blittable.reset(this->createBlittable(QSize(w,h), m_alpha));
    }

    return m_blittable.data();
}

void QBlittablePixmapData::setBlittable(QBlittable *blittable)
{
    resize(blittable->size().width(),blittable->size().height());
    m_blittable.reset(blittable);
}

void QBlittablePixmapData::resize(int width, int height)
{

    m_blittable.reset(0);
    m_engine.reset(0);
#ifdef Q_WS_QPA
    d = QApplicationPrivate::platformIntegration()->screens().at(0)->depth();
#endif
    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);
    setSerialNumber(++global_ser_no);
}

int QBlittablePixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmWidthMM:
        return qRound(w * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(h * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmDepth:
        return 32;
    case QPaintDevice::PdmDpiX: // fall-through
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY: // fall-through
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    default:
        qWarning("QRasterPixmapData::metric(): Unhandled metric type %d", metric);
        break;
    }

    return 0;
}

void QBlittablePixmapData::fill(const QColor &color)
{
    if (blittable()->capabilities() & QBlittable::AlphaFillRectCapability) {
        blittable()->unlock();
        blittable()->alphaFillRect(QRectF(0,0,w,h),color,QPainter::CompositionMode_Source);
    } else if (color.alpha() == 255 && blittable()->capabilities() & QBlittable::SolidRectCapability) {
        blittable()->unlock();
        blittable()->fillRect(QRectF(0,0,w,h),color);
    } else {
        // Need to be backed with an alpha channel now. It would be nice
        // if we could just change the format, e.g. when going from
        // RGB32 -> ARGB8888.
        if (color.alpha() != 255 && !hasAlphaChannel()) {
            m_blittable.reset(0);
            m_engine.reset(0);
            m_alpha = true;
        }

        uint pixel;
        switch (blittable()->lock()->format()) {
        case QImage::Format_ARGB32_Premultiplied:
            pixel = PREMUL(color.rgba());
            break;
        case QImage::Format_ARGB8565_Premultiplied:
            pixel = qargb8565(color.rgba()).rawValue();
            break;
        case QImage::Format_ARGB8555_Premultiplied:
            pixel = qargb8555(color.rgba()).rawValue();
            break;
        case QImage::Format_ARGB6666_Premultiplied:
            pixel = qargb6666(color.rgba()).rawValue();
            break;
        case QImage::Format_ARGB4444_Premultiplied:
            pixel = qargb4444(color.rgba()).rawValue();
            break;
        default:
            pixel = color.rgba();
            break;
        }
        //so premultiplied formats are supported and ARGB32 and RGB32
        blittable()->lock()->fill(pixel);
    }

}

QImage *QBlittablePixmapData::buffer()
{
    return blittable()->lock();
}

QImage QBlittablePixmapData::toImage() const
{
    return blittable()->lock()->copy();
}

bool QBlittablePixmapData::hasAlphaChannel() const
{
    return blittable()->lock()->hasAlphaChannel();
}

void QBlittablePixmapData::fromImage(const QImage &image,
                                     Qt::ImageConversionFlags flags)
{
    m_alpha = image.hasAlphaChannel();
    resize(image.width(),image.height());
    markRasterOverlay(QRect(0,0,w,h));
    QImage *thisImg = buffer();

    QImage correctFormatPic = image;
    if (correctFormatPic.format() != thisImg->format())
        correctFormatPic = correctFormatPic.convertToFormat(thisImg->format(), flags);

    uchar *mem = thisImg->bits();
    const uchar *bits = correctFormatPic.bits();
    int bytesCopied = 0;
    while (bytesCopied < correctFormatPic.byteCount()) {
        memcpy(mem,bits,correctFormatPic.bytesPerLine());
        mem += thisImg->bytesPerLine();
        bits += correctFormatPic.bytesPerLine();
        bytesCopied+=correctFormatPic.bytesPerLine();
    }
}

QPaintEngine *QBlittablePixmapData::paintEngine() const
{
    if (!m_engine) {
        QBlittablePixmapData *that = const_cast<QBlittablePixmapData *>(this);
        that->m_engine.reset(new QBlitterPaintEngine(that));
    }
    return m_engine.data();
}

#ifdef QT_BLITTER_RASTEROVERLAY

static bool showRasterOverlay = !qgetenv("QT_BLITTER_RASTEROVERLAY").isEmpty();

void QBlittablePixmapData::mergeOverlay()
{
    if (m_unmergedCopy || !showRasterOverlay)
        return;
    m_unmergedCopy = new QImage(buffer()->copy());
    QPainter p(buffer());
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawImage(0,0,*overlay());
    p.end();
}

void QBlittablePixmapData::unmergeOverlay()
{
    if (!m_unmergedCopy || !showRasterOverlay)
        return;
    QPainter p(buffer());
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(0,0,*m_unmergedCopy);
    p.end();

    delete m_unmergedCopy;
    m_unmergedCopy = 0;
}

QImage *QBlittablePixmapData::overlay()
{
    if (!m_rasterOverlay||
        m_rasterOverlay->size() != QSize(w,h)){
        m_rasterOverlay = new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
        m_rasterOverlay->fill(0x00000000);
        uint color = (qrand() % 11)+7;
        m_overlayColor = QColor(Qt::GlobalColor(color));
        m_overlayColor.setAlpha(0x88);

    }
    return m_rasterOverlay;
}

void QBlittablePixmapData::markRasterOverlayImpl(const QRectF &rect)
{
    if (!showRasterOverlay)
        return;
    QRectF transformationRect = clipAndTransformRect(rect);
    if(!transformationRect.isEmpty()) {
        QPainter p(overlay());
        p.setBrush(m_overlayColor);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(transformationRect,QBrush(m_overlayColor));
    }
}

void QBlittablePixmapData::unmarkRasterOverlayImpl(const QRectF &rect)
{
    if (!showRasterOverlay)
        return;
    QRectF transformationRect = clipAndTransformRect(rect);
    if (!transformationRect.isEmpty()) {
        QPainter p(overlay());
        QColor color(0x00,0x00,0x00,0x00);
        p.setBrush(color);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(transformationRect,QBrush(color));
    }
}

QRectF QBlittablePixmapData::clipAndTransformRect(const QRectF &rect) const
{
    QRectF transformationRect = rect;
    paintEngine();
    if (m_engine->state()) {
        transformationRect = m_engine->state()->matrix.mapRect(rect);
        const QClipData *clipData = m_engine->clip();
        if (clipData) {
            if (clipData->hasRectClip) {
                transformationRect &= clipData->clipRect;
            } else if (clipData->hasRegionClip) {
                const QVector<QRect> rects = clipData->clipRegion.rects();
                for (int i = 0; i < rects.size(); i++) {
                    transformationRect &= rects.at(i);
                }
            }
        }
    }
    return transformationRect;
}

#endif //QT_BLITTER_RASTEROVERLAY

QT_END_NAMESPACE

#endif //QT_NO_BLITTABLE
