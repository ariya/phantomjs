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
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2ddevicecontext.h"

#include <wrl.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DDeviceContextPrivate {
public:
    QWindowsDirect2DDeviceContextPrivate(ID2D1DeviceContext *dc)
        : deviceContext(dc)
        , refCount(0)
    {
        if (!dc) {
            HRESULT hr = QWindowsDirect2DContext::instance()->d2dDevice()->CreateDeviceContext(
                        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                        &deviceContext);
            if (FAILED(hr))
                qFatal("%s: Couldn't create Direct2D Device Context: %#x", __FUNCTION__, hr);
        }

        Q_ASSERT(deviceContext);
    }

    void begin()
    {
        Q_ASSERT(deviceContext);
        Q_ASSERT(refCount >= 0);

        if (refCount == 0)
            deviceContext->BeginDraw();

        refCount++;
    }

    bool end()
    {
        Q_ASSERT(deviceContext);
        Q_ASSERT(refCount > 0);

        bool success = true;
        refCount--;

        if (refCount == 0) {
            D2D1_TAG tag1, tag2;
            HRESULT hr = deviceContext->EndDraw(&tag1, &tag2);

            if (FAILED(hr)) {
                success = false;
                qWarning("%s: EndDraw failed: %#x, tag1: %lld, tag2: %lld", __FUNCTION__, hr, tag1, tag2);
            }
        }

        return success;
    }

    ComPtr<ID2D1DeviceContext> deviceContext;
    int refCount;
};

QWindowsDirect2DDeviceContext::QWindowsDirect2DDeviceContext(ID2D1DeviceContext *dc)
    : d_ptr(new QWindowsDirect2DDeviceContextPrivate(dc))
{
}

QWindowsDirect2DDeviceContext::~QWindowsDirect2DDeviceContext()
{

}

ID2D1DeviceContext *QWindowsDirect2DDeviceContext::get() const
{
    Q_D(const QWindowsDirect2DDeviceContext);
    Q_ASSERT(d->deviceContext);

    return d->deviceContext.Get();
}

void QWindowsDirect2DDeviceContext::begin()
{
    Q_D(QWindowsDirect2DDeviceContext);
    d->begin();
}

bool QWindowsDirect2DDeviceContext::end()
{
    Q_D(QWindowsDirect2DDeviceContext);
    return d->end();
}

QT_END_NAMESPACE
