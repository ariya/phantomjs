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

#ifndef QPIXMAPDATA_X11_P_H
#define QPIXMAPDATA_X11_P_H

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

#include <QtGui/private/qpixmapdata_p.h>
#include <QtGui/private/qpixmapdatafactory_p.h>

#include "QtGui/qx11info_x11.h"

QT_BEGIN_NAMESPACE

class QX11PaintEngine;

struct QXImageWrapper;

class Q_GUI_EXPORT QX11PixmapData : public QPixmapData
{
public:
    QX11PixmapData(PixelType type);
//     QX11PixmapData(PixelType type, int width, int height);
//     QX11PixmapData(PixelType type, const QImage &image,
//                    Qt::ImageConversionFlags flags);
    ~QX11PixmapData();

    QPixmapData *createCompatiblePixmapData() const;

    void resize(int width, int height);
    void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
    void copy(const QPixmapData *data, const QRect &rect);
    bool scroll(int dx, int dy, const QRect &rect);

    void fill(const QColor &color);
    QBitmap mask() const;
    void setMask(const QBitmap &mask);
    bool hasAlphaChannel() const;
    void setAlphaChannel(const QPixmap &alphaChannel);
    QPixmap alphaChannel() const;
    QPixmap transformed(const QTransform &transform,
                        Qt::TransformationMode mode) const;
    QImage toImage() const;
    QImage toImage(const QRect &rect) const;
    QPaintEngine* paintEngine() const;

    Qt::HANDLE handle() const { return hd; }
    Qt::HANDLE x11ConvertToDefaultDepth();

    static Qt::HANDLE createBitmapFromImage(const QImage &image);

    void* gl_surface;
#ifndef QT_NO_XRENDER
    void convertToARGB32(bool preserveContents = true);
#endif

protected:
    int metric(QPaintDevice::PaintDeviceMetric metric) const;

private:
    friend class QPixmap;
    friend class QBitmap;
    friend class QX11PaintEngine;
    friend class QX11WindowSurface;
    friend class QRasterWindowSurface;
    friend class QGLContextPrivate; // Needs to access xinfo, gl_surface & flags
    friend class QEglContext; // Needs gl_surface
    friend class QGLContext; // Needs gl_surface
    friend class QX11GLPixmapData; // Needs gl_surface
    friend class QMeeGoLivePixmapData; // Needs gl_surface and flags
    friend bool  qt_createEGLSurfaceForPixmap(QPixmapData*, bool); // Needs gl_surface

    void release();

    QImage toImage(const QXImageWrapper &xi, const QRect &rect) const;

    QBitmap mask_to_bitmap(int screen) const;
    static Qt::HANDLE bitmap_to_mask(const QBitmap &, int screen);
    void bitmapFromImage(const QImage &image);

    bool canTakeQImageFromXImage(const QXImageWrapper &xi) const;
    QImage takeQImageFromXImage(const QXImageWrapper &xi) const;

    Qt::HANDLE hd;

    enum Flag {
         NoFlags = 0x0,
         Uninitialized = 0x1,
         Readonly = 0x2,
         InvertedWhenBoundToTexture = 0x4,
         GlSurfaceCreatedWithAlpha = 0x8
    };
    uint flags;

    QX11Info xinfo;
    Qt::HANDLE x11_mask;
    Qt::HANDLE picture;
    Qt::HANDLE mask_picture;
    Qt::HANDLE hd2; // sorted in the default display depth
    QPixmap::ShareMode share_mode;

    QX11PaintEngine *pengine;
};

QT_END_NAMESPACE

#endif // QPIXMAPDATA_X11_P_H

