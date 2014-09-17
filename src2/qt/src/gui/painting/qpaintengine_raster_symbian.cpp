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

#include <private/qpaintengine_raster_symbian_p.h>
#include <private/qpixmap_raster_symbian_p.h>
#include <private/qt_s60_p.h>
#include <private/qvolatileimage_p.h>

QT_BEGIN_NAMESPACE

class QSymbianRasterPaintEnginePrivate : public QRasterPaintEnginePrivate
{
public:
    QSymbianRasterPaintEnginePrivate() {}
};

QSymbianRasterPaintEngine::QSymbianRasterPaintEngine(QPaintDevice *device, QSymbianRasterPixmapData *data)
    : QRasterPaintEngine(*(new QSymbianRasterPaintEnginePrivate), device), pixmapData(data)
{
}

bool QSymbianRasterPaintEngine::begin(QPaintDevice *device)
{
    Q_D(QSymbianRasterPaintEngine);

    if (pixmapData && pixmapData->classId() == QPixmapData::RasterClass) {
        pixmapData->beginDataAccess();
        bool ret = QRasterPaintEngine::begin(device);
        // Make sure QPaintEngine::paintDevice() returns the proper device.
        // QRasterPaintEngine changes pdev to QImage in case of RasterClass QPixmapData
        // which is incorrect in Symbian.
        d->pdev = device;
        return ret;
    }
    return QRasterPaintEngine::begin(device);
}

bool QSymbianRasterPaintEngine::end()
{
    if (pixmapData && pixmapData->classId() == QPixmapData::RasterClass) {
        bool ret = QRasterPaintEngine::end();
        pixmapData->endDataAccess();
        return ret;
    }
    return QRasterPaintEngine::end();
}

void QSymbianRasterPaintEngine::drawPixmap(const QPointF &p, const QPixmap &pm)
{
    if (pm.pixmapData()->classId() == QPixmapData::RasterClass) {
        QSymbianRasterPixmapData *srcData = static_cast<QSymbianRasterPixmapData *>(pm.pixmapData());
        srcData->beginDataAccess();
        QRasterPaintEngine::drawPixmap(p, pm);
        srcData->endDataAccess();
    } else {
        QVolatileImage img = pm.pixmapData()->toVolatileImage();
        if (!img.isNull()) {
            img.beginDataAccess();
            // imageRef() would detach and since we received the QVolatileImage
            // from toVolatileImage() by value, it would cause a copy which
            // would ruin our goal. So use constImageRef() instead.
            QRasterPaintEngine::drawImage(p, img.constImageRef());
            img.endDataAccess(true);
        } else {
            QRasterPaintEngine::drawPixmap(p, pm);
        }
    }
}

void QSymbianRasterPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    if (pm.pixmapData()->classId() == QPixmapData::RasterClass) {
        QSymbianRasterPixmapData *srcData = static_cast<QSymbianRasterPixmapData *>(pm.pixmapData());
        srcData->beginDataAccess();
        QRasterPaintEngine::drawPixmap(r, pm, sr);
        srcData->endDataAccess();
    } else {
        QVolatileImage img = pm.pixmapData()->toVolatileImage();
        if (!img.isNull()) {
            img.beginDataAccess();
            QRasterPaintEngine::drawImage(r, img.constImageRef(), sr);
            img.endDataAccess(true);
        } else {
            QRasterPaintEngine::drawPixmap(r, pm, sr);
        }
    }
}

void QSymbianRasterPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &sr)
{
    if (pm.pixmapData()->classId() == QPixmapData::RasterClass) {
        QSymbianRasterPixmapData *srcData = static_cast<QSymbianRasterPixmapData *>(pm.pixmapData());
        srcData->beginDataAccess();
        QRasterPaintEngine::drawTiledPixmap(r, pm, sr);
        srcData->endDataAccess();
    } else {
        QRasterPaintEngine::drawTiledPixmap(r, pm, sr);
    }
}

// used by QSymbianRasterPixmapData::beginDataAccess()
void QSymbianRasterPaintEngine::prepare(QImage *image)
{
    QRasterBuffer *buffer = d_func()->rasterBuffer.data();
    if (buffer)
        buffer->prepare(image);
}

QT_END_NAMESPACE
