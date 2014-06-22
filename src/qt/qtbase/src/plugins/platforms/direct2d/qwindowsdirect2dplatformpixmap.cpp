/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2dhelpers.h"

#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtGui/QPaintDevice>
#include <QtGui/QPaintEngine>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPlatformPixmapPrivate
{
public:
    QWindowsDirect2DPlatformPixmapPrivate()
        : bitmap(new QWindowsDirect2DBitmap)
        , device(new QWindowsDirect2DPaintDevice(bitmap.data(), QInternal::Pixmap))
        , devicePixelRatio(1.0)
    {}

    QScopedPointer<QWindowsDirect2DBitmap> bitmap;
    QScopedPointer<QWindowsDirect2DPaintDevice> device;
    qreal devicePixelRatio;
};

static int qt_d2dpixmap_serno = 0;

QWindowsDirect2DPlatformPixmap::QWindowsDirect2DPlatformPixmap(PixelType pixelType)
    : QPlatformPixmap(pixelType, Direct2DClass)
    , d_ptr(new QWindowsDirect2DPlatformPixmapPrivate)
{
    setSerialNumber(qt_d2dpixmap_serno++);
}

QWindowsDirect2DPlatformPixmap::~QWindowsDirect2DPlatformPixmap()
{

}

void QWindowsDirect2DPlatformPixmap::resize(int width, int height)
{
    Q_D(QWindowsDirect2DPlatformPixmap);

    if (!d->bitmap->resize(width, height)) {
        qWarning("%s: Could not resize bitmap", __FUNCTION__);
        return;
    }

    is_null = false;
    w = width;
    h = height;
    this->d = 32;
}

void QWindowsDirect2DPlatformPixmap::fromImage(const QImage &image,
                                               Qt::ImageConversionFlags flags)
{
    Q_D(QWindowsDirect2DPlatformPixmap);

    if (!d->bitmap->fromImage(image, flags)) {
        qWarning("%s: Could not init from image", __FUNCTION__);
        return;
    }

    is_null = false;
    w = image.width();
    h = image.height();
    this->d = 32;
}

int QWindowsDirect2DPlatformPixmap::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);

    Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric);
    return qt_paint_device_metric(d->device.data(), metric);
}

void QWindowsDirect2DPlatformPixmap::fill(const QColor &color)
{
    Q_D(QWindowsDirect2DPlatformPixmap);
    d->bitmap->fill(color);
}

bool QWindowsDirect2DPlatformPixmap::hasAlphaChannel() const
{
    return true;
}

QImage QWindowsDirect2DPlatformPixmap::toImage() const
{
    return toImage(QRect());
}

QImage QWindowsDirect2DPlatformPixmap::toImage(const QRect &rect) const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);

    bool active = d->device->paintEngine()->isActive();

    if (active)
        d->device->paintEngine()->end();

    QImage result = d->bitmap->toImage(rect);

    if (active)
        d->device->paintEngine()->begin(d->device.data());

    return result;
}

QPaintEngine* QWindowsDirect2DPlatformPixmap::paintEngine() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->device->paintEngine();
}

qreal QWindowsDirect2DPlatformPixmap::devicePixelRatio() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->devicePixelRatio;
}

void QWindowsDirect2DPlatformPixmap::setDevicePixelRatio(qreal scaleFactor)
{
    Q_D(QWindowsDirect2DPlatformPixmap);
    d->devicePixelRatio = scaleFactor;
}

QWindowsDirect2DBitmap *QWindowsDirect2DPlatformPixmap::bitmap() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->bitmap.data();
}

QT_END_NAMESPACE
