/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        deviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
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

void QWindowsDirect2DDeviceContext::suspend()
{
    Q_D(QWindowsDirect2DDeviceContext);
    if (d->refCount > 0)
        d->deviceContext->EndDraw();
}

void QWindowsDirect2DDeviceContext::resume()
{
    Q_D(QWindowsDirect2DDeviceContext);
    if (d->refCount > 0)
        d->deviceContext->BeginDraw();
}

QWindowsDirect2DDeviceContextSuspender::QWindowsDirect2DDeviceContextSuspender(QWindowsDirect2DDeviceContext *dc)
    : m_dc(dc)
{
    Q_ASSERT(m_dc);
    m_dc->suspend();
}

QWindowsDirect2DDeviceContextSuspender::~QWindowsDirect2DDeviceContextSuspender()
{
    resume();
}

void QWindowsDirect2DDeviceContextSuspender::resume()
{
    if (m_dc) {
        m_dc->resume();
        m_dc = Q_NULLPTR;
    }
}

QT_END_NAMESPACE
