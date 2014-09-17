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

#ifndef QPIXMAP_RASTER_SYMBIAN_P_H
#define QPIXMAP_RASTER_SYMBIAN_P_H

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

#include <QtGui/private/qpixmap_raster_p.h>

QT_BEGIN_NAMESPACE

class CFbsBitmap;
class CFbsBitmapDevice;
class CFbsBitGc;

class QSymbianBitmapDataAccess;

class QSymbianFbsHeapLock
{
public:

    enum LockAction {
        Unlock
    };

    explicit QSymbianFbsHeapLock(LockAction a);
    ~QSymbianFbsHeapLock();
    void relock();

private:

    LockAction action;
    bool wasLocked;
};

class QSymbianRasterPixmapData : public QRasterPixmapData
{
public:
    QSymbianRasterPixmapData(PixelType type);
    ~QSymbianRasterPixmapData();

    QPixmapData *createCompatiblePixmapData() const;

    void resize(int width, int height);
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
    void copy(const QPixmapData *data, const QRect &rect);
    bool scroll(int dx, int dy, const QRect &rect);

    int metric(QPaintDevice::PaintDeviceMetric metric) const;
    void fill(const QColor &color);
    void setMask(const QBitmap &mask);
    void setAlphaChannel(const QPixmap &alphaChannel);
    QImage toImage() const;
    QPaintEngine* paintEngine() const;

    void beginDataAccess();
    void endDataAccess(bool readOnly=false) const;

    void* toNativeType(NativeType type);
    void fromNativeType(void* pixmap, NativeType type);

    void convertToDisplayMode(int mode);

private:
    void release();
    void fromSymbianBitmap(CFbsBitmap* bitmap, bool lockFormat=false);
    QImage toImage(const QRect &r) const;

    QSymbianBitmapDataAccess *symbianBitmapDataAccess;

    CFbsBitmap *cfbsBitmap;
    QPaintEngine *pengine;
    uchar* bytes;

    bool formatLocked;

    QSymbianRasterPixmapData *next;
    QSymbianRasterPixmapData *prev;

    static void qt_symbian_register_pixmap(QSymbianRasterPixmapData *pd);
    static void qt_symbian_unregister_pixmap(QSymbianRasterPixmapData *pd);
    static void qt_symbian_release_pixmaps();

    friend class QPixmap;
    friend class QS60WindowSurface;
    friend class QSymbianRasterPaintEngine;
    friend class QS60Data;
};

QT_END_NAMESPACE

#endif // QPIXMAP_RASTER_SYMBIAN_P_H
