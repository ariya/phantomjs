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

#include "qwindowsnativeimage.h"
#include "qwindowscontext.h"

#include <QtGui/private/qpaintengine_p.h>
#include <QtGui/private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
} BITMAPINFO_MASK;

/*!
    \class QWindowsNativeImage
    \brief Windows Native image

    Note that size can be 0 (widget autotests with zero size), which
    causes CreateDIBSection() to fail.

    \sa QWindowsBackingStore
    \internal
    \ingroup qt-lighthouse-win
*/

static inline HDC createDC()
{
    HDC display_dc = GetDC(0);
    HDC hdc = CreateCompatibleDC(display_dc);
    ReleaseDC(0, display_dc);
    Q_ASSERT(hdc);
    return hdc;
}

static inline HBITMAP createDIB(HDC hdc, int width, int height,
                                QImage::Format format,
                                uchar **bitsIn)
{
    BITMAPINFO_MASK bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height; // top-down.
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biSizeImage   = 0;

    if (format == QImage::Format_RGB16) {
        bmi.bmiHeader.biBitCount = 16;
        bmi.bmiHeader.biCompression = BI_BITFIELDS;
        bmi.redMask = 0xF800;
        bmi.greenMask = 0x07E0;
        bmi.blueMask = 0x001F;
    } else {
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.redMask = 0;
        bmi.greenMask = 0;
        bmi.blueMask = 0;
    }

    void *bits = 0;
    HBITMAP bitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bmi),
                                      DIB_RGB_COLORS, &bits, 0, 0);
    if (!bitmap || !bits)
        qFatal("%s: CreateDIBSection failed.", __FUNCTION__);

    *bitsIn = (uchar*)bits;
    return bitmap;
}

QWindowsNativeImage::QWindowsNativeImage(int width, int height,
                                         QImage::Format format) :
    m_hdc(createDC()),
    m_bitmap(0),
    m_null_bitmap(0)
{
    if (width != 0 && height != 0) {
        uchar *bits;
        m_bitmap = createDIB(m_hdc, width, height, format, &bits);
        m_null_bitmap = (HBITMAP)SelectObject(m_hdc, m_bitmap);
        m_image = QImage(bits, width, height, format);
        Q_ASSERT(m_image.paintEngine()->type() == QPaintEngine::Raster);
        static_cast<QRasterPaintEngine *>(m_image.paintEngine())->setDC(m_hdc);
    } else {
        m_image = QImage(width, height, format);
    }

#ifndef Q_OS_WINCE
    GdiFlush();
#endif
}

QWindowsNativeImage::~QWindowsNativeImage()
{
    if (m_hdc) {
        if (m_bitmap) {
            if (m_null_bitmap)
                SelectObject(m_hdc, m_null_bitmap);
            DeleteObject(m_bitmap);
        }
        DeleteDC(m_hdc);
    }
}

QImage::Format QWindowsNativeImage::systemFormat()
{
    static const int depth = QWindowsContext::instance()->screenDepth();
    return depth == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QT_END_NAMESPACE
