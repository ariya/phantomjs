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

#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dpaintengine.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"

#include "qwindowswindow.h"

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintDevicePrivate
{
public:
    QWindowsDirect2DPaintDevicePrivate(QWindowsDirect2DBitmap *bitmap, QInternal::PaintDeviceFlags f)
        : engine(new QWindowsDirect2DPaintEngine(bitmap))
        , bitmap(bitmap)
        , flags(f)
    {}

    QScopedPointer<QWindowsDirect2DPaintEngine> engine;
    QWindowsDirect2DBitmap *bitmap;
    QInternal::PaintDeviceFlags flags;
};

QWindowsDirect2DPaintDevice::QWindowsDirect2DPaintDevice(QWindowsDirect2DBitmap *bitmap, QInternal::PaintDeviceFlags flags)
    : d_ptr(new QWindowsDirect2DPaintDevicePrivate(bitmap, flags))
{
}

QPaintEngine *QWindowsDirect2DPaintDevice::paintEngine() const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    return d->engine.data();
}

int QWindowsDirect2DPaintDevice::devType() const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    return d->flags;
}

int QWindowsDirect2DPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    switch (metric) {
    case QPaintDevice::PdmWidth:
        return d->bitmap->bitmap()->GetPixelSize().width;
        break;
    case QPaintDevice::PdmHeight:
        return d->bitmap->bitmap()->GetPixelSize().height;
        break;
    case QPaintDevice::PdmNumColors:
        return INT_MAX;
        break;
    case QPaintDevice::PdmDepth:
        return 32;
        break;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
    {
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return x;
    }
        break;
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
    {
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return y;
    }
        break;
    case QPaintDevice::PdmDevicePixelRatio:
        return 1;
        break;
    case QPaintDevice::PdmWidthMM:
    case QPaintDevice::PdmHeightMM:
        return -1;
        break;
    }

    return -1;
}

QT_END_NAMESPACE
