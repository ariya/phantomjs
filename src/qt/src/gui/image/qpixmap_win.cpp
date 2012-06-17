/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpixmap.h"
#include "qpixmap_raster_p.h"

#include "qbitmap.h"
#include "qimage.h"
#include "qwidget.h"
#include "qpainter.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qapplication.h"
#include "qevent.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qdatetime.h"
#include "qpixmapcache.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qdebug.h"
#include "qt_windows.h"

#if defined(Q_WS_WINCE)
#include <winbase.h>
#include "qguifunctions_wince.h"
extern bool qt_wince_is_high_dpi();
extern bool qt_wince_is_pocket_pc();
#endif

#ifndef CAPTUREBLT
#define CAPTUREBLT ((DWORD)0x40000000)
#endif

QT_BEGIN_NAMESPACE

QPixmap QPixmap::grabWindow(WId winId, int x, int y, int w, int h )
{
    RECT r;
    GetClientRect(winId, &r);

    if (w < 0) w = r.right - r.left;
    if (h < 0) h = r.bottom - r.top;

#ifdef Q_WS_WINCE_WM
    if (qt_wince_is_pocket_pc()) {
        QWidget *widget = QWidget::find(winId);
        if (qobject_cast<QDesktopWidget *>(widget)) {
            RECT rect = {0,0,0,0};
            AdjustWindowRectEx(&rect, WS_BORDER | WS_CAPTION, FALSE, 0);
            int magicNumber = qt_wince_is_high_dpi() ? 4 : 2;
            y += rect.top - magicNumber;
        }
    }
#endif

    // Create and setup bitmap
    HDC display_dc = GetDC(0);
    HDC bitmap_dc = CreateCompatibleDC(display_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
    HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    // copy data
    HDC window_dc = GetDC(winId);
    BitBlt(bitmap_dc, 0, 0, w, h, window_dc, x, y, SRCCOPY
#ifndef Q_WS_WINCE
                                    | CAPTUREBLT
#endif
            );

    // clean up all but bitmap
    ReleaseDC(winId, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    QPixmap pixmap = QPixmap::fromWinHBITMAP(bitmap);

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    return pixmap;
}

HBITMAP QPixmap::toWinHBITMAP(HBitmapFormat format) const
{
    if (isNull())
        return 0;

    HBITMAP bitmap = 0;
    if (data->classId() == QPixmapData::RasterClass) {
        QRasterPixmapData* d = static_cast<QRasterPixmapData*>(data.data());
        int w = d->image.width();
        int h = d->image.height();

        HDC display_dc = GetDC(0);

        // Define the header
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = w;
        bmi.bmiHeader.biHeight      = -h;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage   = w * h * 4;

        // Create the pixmap
        uchar *pixels = 0;
        bitmap = CreateDIBSection(display_dc, &bmi, DIB_RGB_COLORS, (void **) &pixels, 0, 0);
        ReleaseDC(0, display_dc);
        if (!bitmap) {
            qErrnoWarning("QPixmap::toWinHBITMAP(), failed to create dibsection");
            return 0;
        }
        if (!pixels) {
            qErrnoWarning("QPixmap::toWinHBITMAP(), did not allocate pixel data");
            return 0;
        }

        // Copy over the data
        QImage::Format imageFormat = QImage::Format_ARGB32;
        if (format == NoAlpha)
            imageFormat = QImage::Format_RGB32;
        else if (format == PremultipliedAlpha)
            imageFormat = QImage::Format_ARGB32_Premultiplied;
        const QImage image = d->image.convertToFormat(imageFormat);
        int bytes_per_line = w * 4;
        for (int y=0; y<h; ++y)
            memcpy(pixels + y * bytes_per_line, image.scanLine(y), bytes_per_line);

    } else {
        QPixmapData *data = new QRasterPixmapData(depth() == 1 ?
                                                  QPixmapData::BitmapType : QPixmapData::PixmapType);
        data->fromImage(toImage(), Qt::AutoColor);
        return QPixmap(data).toWinHBITMAP(format);
    }
    return bitmap;
}

QPixmap QPixmap::fromWinHBITMAP(HBITMAP bitmap, HBitmapFormat format)
{
    // Verify size
    BITMAP bitmap_info;
    memset(&bitmap_info, 0, sizeof(BITMAP));

    int res = GetObject(bitmap, sizeof(BITMAP), &bitmap_info);
    if (!res) {
        qErrnoWarning("QPixmap::fromWinHBITMAP(), failed to get bitmap info");
        return QPixmap();
    }
    int w = bitmap_info.bmWidth;
    int h = bitmap_info.bmHeight;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = w * h * 4;

    QImage result;
    // Get bitmap bits
    uchar *data = (uchar *) qMalloc(bmi.bmiHeader.biSizeImage);

    HDC display_dc = GetDC(0);
    if (GetDIBits(display_dc, bitmap, 0, h, data, &bmi, DIB_RGB_COLORS)) {

        QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied;
        uint mask = 0;
        if (format == NoAlpha) {
            imageFormat = QImage::Format_RGB32;
            mask = 0xff000000;
        }

        // Create image and copy data into image.
        QImage image(w, h, imageFormat);
        if (!image.isNull()) { // failed to alloc?
            int bytes_per_line = w * sizeof(QRgb);
            for (int y=0; y<h; ++y) {
                QRgb *dest = (QRgb *) image.scanLine(y);
                const QRgb *src = (const QRgb *) (data + y * bytes_per_line);
                for (int x=0; x<w; ++x) {
                    const uint pixel = src[x];
                    if ((pixel & 0xff000000) == 0 && (pixel & 0x00ffffff) != 0)
                        dest[x] = pixel | 0xff000000;
                    else
                        dest[x] = pixel | mask;
                }
            }
        }
        result = image;
    } else {
        qWarning("QPixmap::fromWinHBITMAP(), failed to get bitmap bits");
    }
    ReleaseDC(0, display_dc);
    qFree(data);
    return fromImage(result);
}

HBITMAP qt_createIconMask(const QBitmap &bitmap)
{
    QImage bm = bitmap.toImage().convertToFormat(QImage::Format_Mono);
    int w = bm.width();
    int h = bm.height();
    int bpl = ((w+15)/16)*2;                        // bpl, 16 bit alignment
    uchar *bits = new uchar[bpl*h];
    bm.invertPixels();
    for (int y=0; y<h; y++)
        memcpy(bits+y*bpl, bm.scanLine(y), bpl);
    HBITMAP hbm = CreateBitmap(w, h, 1, 1, bits);
    delete [] bits;
    return hbm;
}

HICON QPixmap::toWinHICON() const
{
    QBitmap maskBitmap = mask();
    if (maskBitmap.isNull()) {
        maskBitmap= QBitmap(size());
        maskBitmap.fill(Qt::color1);
    }

    ICONINFO ii;
    ii.fIcon    = true;
    ii.hbmMask  = qt_createIconMask(maskBitmap);
    ii.hbmColor = toWinHBITMAP(QPixmap::Alpha);
    ii.xHotspot = 0;
    ii.yHotspot = 0;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);

    return hIcon;
}

#ifdef Q_WS_WIN
#ifndef Q_WS_WINCE

static QImage qt_fromWinHBITMAP(HDC hdc, HBITMAP bitmap, int w, int h)
{
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = w * h * 4;

    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return image;

    // Get bitmap bits
    uchar *data = (uchar *) qMalloc(bmi.bmiHeader.biSizeImage);

    if (GetDIBits(hdc, bitmap, 0, h, data, &bmi, DIB_RGB_COLORS)) {
        // Create image and copy data into image.
        for (int y=0; y<h; ++y) {
            void *dest = (void *) image.scanLine(y);
            void *src = data + y * image.bytesPerLine();
            memcpy(dest, src, image.bytesPerLine());
        }
    } else {
        qWarning("qt_fromWinHBITMAP(), failed to get bitmap bits");
    }
    qFree(data);

    return image;
}

QPixmap QPixmap::fromWinHICON(HICON icon)
{
    bool foundAlpha = false;
    HDC screenDevice = GetDC(0);
    HDC hdc = CreateCompatibleDC(screenDevice);
    ReleaseDC(0, screenDevice);

    ICONINFO iconinfo;
    bool result = GetIconInfo(icon, &iconinfo); //x and y Hotspot describes the icon center
    if (!result)
        qWarning("QPixmap::fromWinHICON(), failed to GetIconInfo()");

    int w = iconinfo.xHotspot * 2;
    int h = iconinfo.yHotspot * 2;

    BITMAPINFOHEADER bitmapInfo;
    bitmapInfo.biSize        = sizeof(BITMAPINFOHEADER);
    bitmapInfo.biWidth       = w;
    bitmapInfo.biHeight      = h;
    bitmapInfo.biPlanes      = 1;
    bitmapInfo.biBitCount    = 32;
    bitmapInfo.biCompression = BI_RGB;
    bitmapInfo.biSizeImage   = 0;
    bitmapInfo.biXPelsPerMeter = 0;
    bitmapInfo.biYPelsPerMeter = 0;
    bitmapInfo.biClrUsed       = 0;
    bitmapInfo.biClrImportant  = 0;
    DWORD* bits;

    HBITMAP winBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bitmapInfo, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
    HGDIOBJ oldhdc = (HBITMAP)SelectObject(hdc, winBitmap);
    DrawIconEx( hdc, 0, 0, icon, iconinfo.xHotspot * 2, iconinfo.yHotspot * 2, 0, 0, DI_NORMAL);
    QImage image = qt_fromWinHBITMAP(hdc, winBitmap, w, h);

    for (int y = 0 ; y < h && !foundAlpha ; y++) {
        QRgb *scanLine= reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < w ; x++) {
            if (qAlpha(scanLine[x]) != 0) {
                foundAlpha = true;
                break;
            }
        }
    }
    if (!foundAlpha) {
        //If no alpha was found, we use the mask to set alpha values
        DrawIconEx( hdc, 0, 0, icon, w, h, 0, 0, DI_MASK);
        QImage mask = qt_fromWinHBITMAP(hdc, winBitmap, w, h);

        for (int y = 0 ; y < h ; y++){
            QRgb *scanlineImage = reinterpret_cast<QRgb *>(image.scanLine(y));
            QRgb *scanlineMask = mask.isNull() ? 0 : reinterpret_cast<QRgb *>(mask.scanLine(y));
            for (int x = 0; x < w ; x++){
                if (scanlineMask && qRed(scanlineMask[x]) != 0)
                    scanlineImage[x] = 0; //mask out this pixel
                else
                    scanlineImage[x] |= 0xff000000; // set the alpha channel to 255
            }
        }
    }
    //dispose resources created by iconinfo call
    DeleteObject(iconinfo.hbmMask);
    DeleteObject(iconinfo.hbmColor);

    SelectObject(hdc, oldhdc); //restore state
    DeleteObject(winBitmap);
    DeleteDC(hdc);
    return QPixmap::fromImage(image);
}
#else //ifndef Q_WS_WINCE
QPixmap QPixmap::fromWinHICON(HICON icon)
{
    HDC screenDevice = GetDC(0);
    HDC hdc = CreateCompatibleDC(screenDevice);
    ReleaseDC(0, screenDevice);

    ICONINFO iconinfo;
    bool result = GetIconInfo(icon, &iconinfo);
    if (!result)
        qWarning("QPixmap::fromWinHICON(), failed to GetIconInfo()");

    int w = 0;
    int h = 0;
    if (!iconinfo.xHotspot || !iconinfo.yHotspot) {
        // We could not retrieve the icon size via GetIconInfo,
        // so we try again using the icon bitmap.
        BITMAP bm;
        int result = GetObject(iconinfo.hbmColor, sizeof(BITMAP), &bm);
        if (!result) result = GetObject(iconinfo.hbmMask, sizeof(BITMAP), &bm);
        if (!result) {
            qWarning("QPixmap::fromWinHICON(), failed to retrieve icon size");
            return QPixmap();
        }
        w = bm.bmWidth;
        h = bm.bmHeight;
    } else {
        // x and y Hotspot describes the icon center
        w = iconinfo.xHotspot * 2;
        h = iconinfo.yHotspot * 2;
    }
    const DWORD dwImageSize = w * h * 4;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFO);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = dwImageSize;

    uchar* bits;

    HBITMAP winBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**) &bits, 0, 0);
    if (winBitmap )
        memset(bits, 0xff, dwImageSize);
    if (!winBitmap) {
        qWarning("QPixmap::fromWinHICON(), failed to CreateDIBSection()");
        return QPixmap();
    }

    HGDIOBJ oldhdc = (HBITMAP)SelectObject(hdc, winBitmap);
    if (!DrawIconEx( hdc, 0, 0, icon, w, h, 0, 0, DI_NORMAL))
        qWarning("QPixmap::fromWinHICON(), failed to DrawIcon()");

    uint mask = 0xff000000;
    // Create image and copy data into image.
    QImage image(w, h, QImage::Format_ARGB32);

    if (!image.isNull()) { // failed to alloc?
        int bytes_per_line = w * sizeof(QRgb);
        for (int y=0; y < h; ++y) {
            QRgb *dest = (QRgb *) image.scanLine(y);
            const QRgb *src = (const QRgb *) (bits + y * bytes_per_line);
            for (int x=0; x < w; ++x) {
                dest[x] = src[x];
            }
        }
    }
    if (!DrawIconEx( hdc, 0, 0, icon, w, h, 0, 0, DI_MASK))
        qWarning("QPixmap::fromWinHICON(), failed to DrawIcon()");
    if (!image.isNull()) { // failed to alloc?
        int bytes_per_line = w * sizeof(QRgb);
        for (int y=0; y < h; ++y) {
            QRgb *dest = (QRgb *) image.scanLine(y);
            const QRgb *src = (const QRgb *) (bits + y * bytes_per_line);
            for (int x=0; x < w; ++x) {
                if (!src[x])
                    dest[x] = dest[x] | mask;
            }
        }
    }
    SelectObject(hdc, oldhdc); //restore state
    DeleteObject(winBitmap);
    DeleteDC(hdc);
    return QPixmap::fromImage(image);
}
#endif //ifndef Q_WS_WINCE
#endif //ifdef Q_WS_WIN

QT_END_NAMESPACE
