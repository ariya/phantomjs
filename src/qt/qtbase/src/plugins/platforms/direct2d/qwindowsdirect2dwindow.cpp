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
#include "qwindowsdirect2dwindow.h"
#include "qwindowsdirect2ddevicecontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dplatformpixmap.h"

#include <d3d11.h>
#include <d2d1_1.h>
using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

QWindowsDirect2DWindow::QWindowsDirect2DWindow(QWindow *window, const QWindowsWindowData &data)
    : QWindowsWindow(window, data)
    , m_needsFullFlush(true)
    , m_directRendering(!(data.flags & Qt::FramelessWindowHint && window->format().hasAlpha()))
{
    if (window->type() == Qt::Desktop)
        return; // No further handling for Qt::Desktop

    if (m_directRendering)
        setupSwapChain();

    HRESULT hr = QWindowsDirect2DContext::instance()->d2dDevice()->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                m_deviceContext.GetAddressOf());
    if (FAILED(hr))
        qWarning("%s: Couldn't create Direct2D Device context: %#x", __FUNCTION__, hr);
}

QWindowsDirect2DWindow::~QWindowsDirect2DWindow()
{
}

void QWindowsDirect2DWindow::setWindowFlags(Qt::WindowFlags flags)
{
    m_directRendering = !(flags & Qt::FramelessWindowHint && window()->format().hasAlpha());
    if (!m_directRendering)
        m_swapChain.Reset(); // No need for the swap chain; release from memory
    else if (!m_swapChain)
        setupSwapChain();

    QWindowsWindow::setWindowFlags(flags);
}

QPixmap *QWindowsDirect2DWindow::pixmap()
{
    setupBitmap();

    return m_pixmap.data();
}

void QWindowsDirect2DWindow::flush(QWindowsDirect2DBitmap *bitmap, const QRegion &region, const QPoint &offset)
{
    QSize size;
    if (m_directRendering) {
        DXGI_SWAP_CHAIN_DESC1 desc;
        HRESULT hr = m_swapChain->GetDesc1(&desc);
        QRect geom = geometry();

        if ((FAILED(hr) || (desc.Width != geom.width()) || (desc.Height != geom.height()))) {
            resizeSwapChain(geom.size());
            m_swapChain->GetDesc1(&desc);
        }
        size.setWidth(desc.Width);
        size.setHeight(desc.Height);
    } else {
        size = geometry().size();
    }

    setupBitmap();
    if (!m_bitmap)
        return;

    if (bitmap != m_bitmap.data()) {
        m_bitmap->deviceContext()->begin();

        ID2D1DeviceContext *dc = m_bitmap->deviceContext()->get();
        if (!m_needsFullFlush) {
            QRegion clipped = region;
            clipped &= QRect(QPoint(), size);

            foreach (const QRect &rect, clipped.rects()) {
                QRectF rectF(rect);
                dc->DrawBitmap(bitmap->bitmap(),
                               to_d2d_rect_f(rectF),
                               1.0,
                               D2D1_INTERPOLATION_MODE_LINEAR,
                               to_d2d_rect_f(rectF.translated(offset.x(), offset.y())));
            }
        } else {
            QRectF rectF(QPoint(), size);
            dc->DrawBitmap(bitmap->bitmap(),
                           to_d2d_rect_f(rectF),
                           1.0,
                           D2D1_INTERPOLATION_MODE_LINEAR,
                           to_d2d_rect_f(rectF.translated(offset.x(), offset.y())));
            m_needsFullFlush = false;
        }

        m_bitmap->deviceContext()->end();
    }
}

void QWindowsDirect2DWindow::present(const QRegion &region)
{
    if (m_directRendering) {
        m_swapChain->Present(0, 0);
        return;
    }

    ComPtr<IDXGISurface> bitmapSurface;
    HRESULT hr = m_bitmap->bitmap()->GetSurface(&bitmapSurface);
    Q_ASSERT(SUCCEEDED(hr));
    ComPtr<IDXGISurface1> dxgiSurface;
    hr = bitmapSurface.As(&dxgiSurface);
    Q_ASSERT(SUCCEEDED(hr));

    HDC hdc;
    hr = dxgiSurface->GetDC(FALSE, &hdc);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get DC for presenting the surface");
        return;
    }

    const QRect bounds = window()->geometry();
    const SIZE size = { bounds.width(), bounds.height() };
    const POINT ptDst = { bounds.x(), bounds.y() };
    const POINT ptSrc = { 0, 0 };
    const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255.0 * opacity(), AC_SRC_ALPHA };
    const QRect r = region.boundingRect();
    const RECT dirty = { r.left(), r.top(), r.left() + r.width(), r.top() + r.height() };
    UPDATELAYEREDWINDOWINFO info = { sizeof(UPDATELAYEREDWINDOWINFO), NULL,
                                     &ptDst, &size, hdc, &ptSrc, 0, &blend, ULW_ALPHA, &dirty };
    if (!UpdateLayeredWindowIndirect(handle(), &info))
        qErrnoWarning(GetLastError(), "Failed to update the layered window");

    hr = dxgiSurface->ReleaseDC(NULL);
    if (FAILED(hr))
        qErrnoWarning(hr, "Failed to release the DC for presentation");
}

void QWindowsDirect2DWindow::setupSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};

    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    HRESULT hr = QWindowsDirect2DContext::instance()->dxgiFactory()->CreateSwapChainForHwnd(
                QWindowsDirect2DContext::instance()->d3dDevice(), // [in]   IUnknown *pDevice
                handle(),                                         // [in]   HWND hWnd
                &desc,                                            // [in]   const DXGI_SWAP_CHAIN_DESC1 *pDesc
                NULL,                                             // [in]   const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc
                NULL,                                             // [in]   IDXGIOutput *pRestrictToOutput
                m_swapChain.ReleaseAndGetAddressOf());            // [out]  IDXGISwapChain1 **ppSwapChain

    if (FAILED(hr))
        qWarning("%s: Could not create swap chain: %#x", __FUNCTION__, hr);

    m_needsFullFlush = true;
}

void QWindowsDirect2DWindow::resizeSwapChain(const QSize &size)
{
    m_pixmap.reset();
    m_bitmap.reset();
    m_deviceContext->SetTarget(Q_NULLPTR);
    m_needsFullFlush = true;

    if (!m_swapChain)
        return;

    HRESULT hr = m_swapChain->ResizeBuffers(0,
                                            size.width(), size.height(),
                                            DXGI_FORMAT_UNKNOWN,
                                            0);
    if (FAILED(hr))
        qWarning("%s: Could not resize swap chain: %#x", __FUNCTION__, hr);
}

QSharedPointer<QWindowsDirect2DBitmap> QWindowsDirect2DWindow::copyBackBuffer() const
{
    const QSharedPointer<QWindowsDirect2DBitmap> null_result;

    if (!m_bitmap)
        return null_result;

    D2D1_PIXEL_FORMAT format = m_bitmap->bitmap()->GetPixelFormat();
    D2D1_SIZE_U size = m_bitmap->bitmap()->GetPixelSize();

    FLOAT dpiX, dpiY;
    m_bitmap->bitmap()->GetDpi(&dpiX, &dpiY);

    D2D1_BITMAP_PROPERTIES1 properties = {
        format,                     // D2D1_PIXEL_FORMAT pixelFormat;
        dpiX,                       // FLOAT dpiX;
        dpiY,                       // FLOAT dpiY;
        D2D1_BITMAP_OPTIONS_TARGET, // D2D1_BITMAP_OPTIONS bitmapOptions;
        Q_NULLPTR                   // _Field_size_opt_(1) ID2D1ColorContext *colorContext;
    };
    ComPtr<ID2D1Bitmap1> copy;
    HRESULT hr = m_deviceContext.Get()->CreateBitmap(size, NULL, 0, properties, &copy);

    if (FAILED(hr)) {
        qWarning("%s: Could not create staging bitmap: %#x", __FUNCTION__, hr);
        return null_result;
    }

    hr = copy.Get()->CopyFromBitmap(NULL, m_bitmap->bitmap(), NULL);
    if (FAILED(hr)) {
        qWarning("%s: Could not copy from bitmap! %#x", __FUNCTION__, hr);
        return null_result;
    }

    return QSharedPointer<QWindowsDirect2DBitmap>(new QWindowsDirect2DBitmap(copy.Get(), Q_NULLPTR));
}

void QWindowsDirect2DWindow::setupBitmap()
{
    if (m_bitmap)
        return;

    if (!m_deviceContext)
        return;

    if (m_directRendering && !m_swapChain)
        return;

    HRESULT hr;
    ComPtr<IDXGISurface1> backBufferSurface;
    if (m_directRendering) {
        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferSurface));
        if (FAILED(hr)) {
            qWarning("%s: Could not query backbuffer for DXGI Surface: %#x", __FUNCTION__, hr);
            return;
        }
    } else {
        const QRect rect = geometry();
        CD3D11_TEXTURE2D_DESC backBufferDesc(DXGI_FORMAT_B8G8R8A8_UNORM, rect.width(), rect.height(), 1, 1);
        backBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        backBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
        ComPtr<ID3D11Texture2D> backBufferTexture;
        HRESULT hr = QWindowsDirect2DContext::instance()->d3dDevice()->CreateTexture2D(&backBufferDesc, NULL, &backBufferTexture);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to create backing texture for indirect rendering");
            return;
        }

        hr = backBufferTexture.As(&backBufferSurface);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to cast back buffer surface to DXGI surface");
            return;
        }
    }

    ComPtr<ID2D1Bitmap1> backBufferBitmap;
    hr = m_deviceContext->CreateBitmapFromDxgiSurface(backBufferSurface.Get(), NULL, backBufferBitmap.GetAddressOf());
    if (FAILED(hr)) {
        qWarning("%s: Could not create Direct2D Bitmap from DXGI Surface: %#x", __FUNCTION__, hr);
        return;
    }

    m_bitmap.reset(new QWindowsDirect2DBitmap(backBufferBitmap.Get(), m_deviceContext.Get()));

    QWindowsDirect2DPaintEngine::Flags flags = QWindowsDirect2DPaintEngine::NoFlag;
    if (!m_directRendering)
        flags |= QWindowsDirect2DPaintEngine::TranslucentTopLevelWindow;
    QWindowsDirect2DPlatformPixmap *pp = new QWindowsDirect2DPlatformPixmap(QPlatformPixmap::PixmapType,
                                                                            flags,
                                                                            m_bitmap.data());
    m_pixmap.reset(new QPixmap(pp));
}

QT_END_NAMESPACE
