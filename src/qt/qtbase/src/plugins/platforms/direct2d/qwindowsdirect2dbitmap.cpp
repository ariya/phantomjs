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

#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2ddevicecontext.h"

#include <QtGui/QImage>
#include <QtGui/QColor>

#include <wrl.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DBitmapPrivate
{
public:
    QWindowsDirect2DBitmapPrivate(ID2D1DeviceContext *dc = 0, ID2D1Bitmap1 *bm = 0)
        : bitmap(bm)
        , deviceContext(new QWindowsDirect2DDeviceContext(dc))
    {
        deviceContext->get()->SetTarget(bm);
    }

    D2D1_BITMAP_PROPERTIES1 bitmapProperties() const
    {
        FLOAT dpiX, dpiY;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&dpiX, &dpiY);

        return D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_TARGET,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                      D2D1_ALPHA_MODE_PREMULTIPLIED),
                    dpiX, dpiY);

    }

    bool resize(int width, int height, const void *data = 0, int pitch = 0)
    {
        deviceContext->get()->SetTarget(0);
        bitmap.Reset();

        D2D1_SIZE_U size = {
            width, height
        };

        HRESULT hr = deviceContext->get()->CreateBitmap(size, data, pitch,
                                                        bitmapProperties(),
                                                        bitmap.ReleaseAndGetAddressOf());
        if (SUCCEEDED(hr))
            deviceContext->get()->SetTarget(bitmap.Get());
        else
            qWarning("%s: Could not create bitmap: %#x", __FUNCTION__, hr);

        return SUCCEEDED(hr);
    }

    QImage toImage(const QRect &rect)
    {
        if (!bitmap)
            return QImage();

        ComPtr<ID2D1Bitmap1> mappingCopy;

        HRESULT hr = S_OK;
        D2D1_SIZE_U size = bitmap->GetPixelSize();

        D2D1_BITMAP_PROPERTIES1 properties = bitmapProperties();
        properties.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;

        hr = deviceContext->get()->CreateBitmap(size, NULL, NULL,
                                                properties, &mappingCopy);
        if (FAILED(hr)) {
            qWarning("%s: Could not create bitmap: %#x", __FUNCTION__, hr);
            return QImage();
        }

        hr = mappingCopy->CopyFromBitmap(NULL, bitmap.Get(), NULL);
        if (FAILED(hr)) {
            qWarning("%s: Could not copy from bitmap: %#x", __FUNCTION__, hr);
            return QImage();
        }

        D2D1_MAPPED_RECT mappedRect;
        hr = mappingCopy->Map(D2D1_MAP_OPTIONS_READ, &mappedRect);
        if (FAILED(hr)) {
            qWarning("%s: Could not map: %#x", __FUNCTION__, hr);
            return QImage();
        }

        return QImage(static_cast<const uchar *>(mappedRect.bits),
                      size.width, size.height, mappedRect.pitch,
                      QImage::Format_ARGB32_Premultiplied).copy(rect);
    }

    QScopedPointer<QWindowsDirect2DDeviceContext> deviceContext;
    ComPtr<ID2D1Bitmap1> bitmap;
};

QWindowsDirect2DBitmap::QWindowsDirect2DBitmap()
    : d_ptr(new QWindowsDirect2DBitmapPrivate)
{
}

QWindowsDirect2DBitmap::QWindowsDirect2DBitmap(ID2D1Bitmap1 *bitmap, ID2D1DeviceContext *dc)
    : d_ptr(new QWindowsDirect2DBitmapPrivate(dc, bitmap))
{
}

QWindowsDirect2DBitmap::~QWindowsDirect2DBitmap()
{
}

bool QWindowsDirect2DBitmap::resize(int width, int height)
{
    Q_D(QWindowsDirect2DBitmap);
    return d->resize(width, height);
}

bool QWindowsDirect2DBitmap::fromImage(const QImage &image, Qt::ImageConversionFlags flags)
{
    Q_D(QWindowsDirect2DBitmap);

    QImage converted = image.convertToFormat(QImage::Format_ARGB32_Premultiplied, flags);
    return d->resize(converted.width(), converted.height(),
                     converted.constBits(), converted.bytesPerLine());
}

ID2D1Bitmap1* QWindowsDirect2DBitmap::bitmap() const
{
    Q_D(const QWindowsDirect2DBitmap);
    return d->bitmap.Get();
}

QWindowsDirect2DDeviceContext *QWindowsDirect2DBitmap::deviceContext() const
{
    Q_D(const QWindowsDirect2DBitmap);
    return d->deviceContext.data();
}

void QWindowsDirect2DBitmap::fill(const QColor &color)
{
    Q_D(QWindowsDirect2DBitmap);

    d->deviceContext->begin();
    d->deviceContext->get()->Clear(to_d2d_color_f(color));
    d->deviceContext->end();
}

QImage QWindowsDirect2DBitmap::toImage(const QRect &rect)
{
    Q_D(QWindowsDirect2DBitmap);
    return d->toImage(rect);
}

QSize QWindowsDirect2DBitmap::size() const
{
    Q_D(const QWindowsDirect2DBitmap);

    D2D1_SIZE_U size = d->bitmap->GetPixelSize();
    return QSize(size.width, size.height);
}

QT_END_NAMESPACE
