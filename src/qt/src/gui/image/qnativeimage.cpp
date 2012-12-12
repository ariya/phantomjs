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

#include <qdebug.h>
#include "qnativeimage_p.h"
#include "qcolormap.h"

#include "private/qpaintengine_raster_p.h"

#include "private/qapplication_p.h"
#include "private/qgraphicssystem_p.h"

#if defined(Q_WS_X11) && !defined(QT_NO_MITSHM)
#include <qx11info_x11.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <qwidget.h>
#endif

#ifdef Q_WS_MAC
#include <private/qpaintengine_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_WS_WIN
typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
} BITMAPINFO_MASK;


QNativeImage::QNativeImage(int width, int height, QImage::Format format, bool isTextBuffer, QWidget *)
{
#ifndef Q_WS_WINCE
    Q_UNUSED(isTextBuffer);
#endif
    BITMAPINFO_MASK bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biSizeImage   = 0;

    if (format == QImage::Format_RGB16) {
        bmi.bmiHeader.biBitCount = 16;
#ifdef Q_WS_WINCE
        if (isTextBuffer) {
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.redMask = 0;
            bmi.greenMask = 0;
            bmi.blueMask = 0;
        } else
#endif
        {
            bmi.bmiHeader.biCompression = BI_BITFIELDS;
            bmi.redMask = 0xF800;
            bmi.greenMask = 0x07E0;
            bmi.blueMask = 0x001F;
        }
    } else {
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.redMask = 0;
        bmi.greenMask = 0;
        bmi.blueMask = 0;
    }

    HDC display_dc = GetDC(0);
    hdc = CreateCompatibleDC(display_dc);
    ReleaseDC(0, display_dc);
    Q_ASSERT(hdc);

    uchar *bits = 0;
    bitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bmi), DIB_RGB_COLORS, (void**) &bits, 0, 0);
    Q_ASSERT(bitmap);
    Q_ASSERT(bits);

    null_bitmap = (HBITMAP)SelectObject(hdc, bitmap);
    image = QImage(bits, width, height, format);

    Q_ASSERT(image.paintEngine()->type() == QPaintEngine::Raster);
    static_cast<QRasterPaintEngine *>(image.paintEngine())->setDC(hdc);

#ifndef Q_WS_WINCE
    GdiFlush();
#endif
}

QNativeImage::~QNativeImage()
{
    if (bitmap || hdc) {
        Q_ASSERT(hdc);
        Q_ASSERT(bitmap);
        if (null_bitmap)
            SelectObject(hdc, null_bitmap);
        DeleteDC(hdc);
        DeleteObject(bitmap);
    }
}

QImage::Format QNativeImage::systemFormat()
{
    if (QColormap::instance().depth() == 16)
        return QImage::Format_RGB16;
    return QImage::Format_RGB32;
}


#elif defined(Q_WS_X11) && !defined(QT_NO_MITSHM)

QNativeImage::QNativeImage(int width, int height, QImage::Format format,bool /* isTextBuffer */, QWidget *widget)
    : xshmimg(0), xshmpm(0)
{
    QX11Info info = widget->x11Info();

    int dd = info.depth();
    Visual *vis = (Visual*) info.visual();

    if (!X11->use_mitshm || format != QImage::Format_RGB16 && X11->bppForDepth.value(dd) != 32) {
        image = QImage(width, height, format);
        // follow good coding practice and set xshminfo attributes, though values not used in this case
        xshminfo.readOnly = true;
        xshminfo.shmaddr = 0;
        xshminfo.shmid = 0;
        xshminfo.shmseg = 0;
        return;
    }

    xshmimg = XShmCreateImage(X11->display, vis, dd, ZPixmap, 0, &xshminfo, width, height);
    if (!xshmimg) {
        qWarning("QNativeImage: Unable to create shared XImage.");
        return;
    }

    bool ok;
    xshminfo.shmid = shmget(IPC_PRIVATE, xshmimg->bytes_per_line * xshmimg->height,
                            IPC_CREAT | 0777);
    ok = xshminfo.shmid != -1;
    if (ok) {
        xshmimg->data = (char*)shmat(xshminfo.shmid, 0, 0);
        xshminfo.shmaddr = xshmimg->data;
        ok = (xshminfo.shmaddr != (char*)-1);
        if (ok)
            image = QImage((uchar *)xshmimg->data, width, height, format);
    }
    xshminfo.readOnly = false;
    if (ok) {
        ok = XShmAttach(X11->display, &xshminfo);
        XSync(X11->display, False);
        if (shmctl(xshminfo.shmid, IPC_RMID, 0) == -1)
            qWarning() << "Error while marking the shared memory segment to be destroyed";
    }
    if (!ok) {
        qWarning() << "QNativeImage: Unable to attach to shared memory segment.";
        if (xshmimg->data) {
            free(xshmimg->data);
            xshmimg->data = 0;
        }
        XDestroyImage(xshmimg);
        xshmimg = 0;
        if (xshminfo.shmaddr)
            shmdt(xshminfo.shmaddr);
        if (xshminfo.shmid != -1)
            shmctl(xshminfo.shmid, IPC_RMID, 0);
        return;
    }
    if (X11->use_mitshm_pixmaps) {
        xshmpm = XShmCreatePixmap(X11->display, DefaultRootWindow(X11->display), xshmimg->data,
                                  &xshminfo, width, height, dd);
        if (!xshmpm) {
            qWarning() << "QNativeImage: Unable to create shared Pixmap.";
        }
    }
}


QNativeImage::~QNativeImage()
{
    if (!xshmimg)
        return;

    if (xshmpm) {
        XFreePixmap(X11->display, xshmpm);
        xshmpm = 0;
    }
    XShmDetach(X11->display, &xshminfo);
    xshmimg->data = 0;
    XDestroyImage(xshmimg);
    xshmimg = 0;
    shmdt(xshminfo.shmaddr);
    shmctl(xshminfo.shmid, IPC_RMID, 0);
}

QImage::Format QNativeImage::systemFormat()
{
    if (QX11Info::appDepth() == 16)
        return QImage::Format_RGB16;
    return QImage::Format_RGB32;
}

#elif defined(Q_WS_MAC)

QNativeImage::QNativeImage(int width, int height, QImage::Format format, bool /* isTextBuffer */, QWidget *widget)
    : image(width, height, format)
{

    uint cgflags = kCGImageAlphaNoneSkipFirst;
    switch (format) {
    case QImage::Format_ARGB32:
        cgflags = kCGImageAlphaFirst;
        break;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
        cgflags = kCGImageAlphaPremultipliedFirst;
        break;
    default:
        break;
    }

#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif

    cg = CGBitmapContextCreate(image.bits(), width, height, 8, image.bytesPerLine(),
                               QCoreGraphicsPaintEngine::macDisplayColorSpace(widget), cgflags);
    CGContextTranslateCTM(cg, 0, height);
    CGContextScaleCTM(cg, 1, -1);

    Q_ASSERT(image.paintEngine()->type() == QPaintEngine::Raster);
    static_cast<QRasterPaintEngine *>(image.paintEngine())->setCGContext(cg);
}


QNativeImage::~QNativeImage()
{
    CGContextRelease(cg);
}

QImage::Format QNativeImage::systemFormat()
{
    return QImage::Format_RGB32;
}


#else // other platforms...

QNativeImage::QNativeImage(int width, int height, QImage::Format format,  bool /* isTextBuffer */, QWidget *)
    : image(width, height, format)
{

}


QNativeImage::~QNativeImage()
{
}

QImage::Format QNativeImage::systemFormat()
{
#ifdef Q_WS_QPA
    return QApplicationPrivate::platformIntegration()->screens().at(0)->format();
#else
    return QImage::Format_RGB32;
#endif
}

#endif // platforms

QT_END_NAMESPACE

