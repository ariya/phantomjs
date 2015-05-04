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

#ifndef  QT_NO_CURSOR
#include "qwindowscursor.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsscreen.h"
#include "qwindowsscaling.h"

#include <QtGui/QBitmap>
#include <QtGui/QImage>
#include <QtGui/QBitmap>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/private/qguiapplication_p.h> // getPixmapCursor()

#include <QtCore/QDebug>
#include <QtCore/QScopedArrayPointer>

static void initResources()
{
#if !defined (Q_OS_WINCE) && !defined (QT_NO_IMAGEFORMAT_PNG)
    Q_INIT_RESOURCE(cursors);
#endif
}

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap &p, int hbitmapFormat = 0);
Q_GUI_EXPORT HBITMAP qt_createIconMask(const QBitmap &bitmap);

/*!
    \class QWindowsCursorCacheKey
    \brief Cache key for storing values in a QHash with a QCursor as key.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsCursorCacheKey::QWindowsCursorCacheKey(const QCursor &c)
    : shape(c.shape()), bitmapCacheKey(0), maskCacheKey(0)
{
    if (shape == Qt::BitmapCursor) {
        const qint64 pixmapCacheKey = c.pixmap().cacheKey();
        if (pixmapCacheKey) {
            bitmapCacheKey = pixmapCacheKey;
        } else {
            Q_ASSERT(c.bitmap());
            Q_ASSERT(c.mask());
            bitmapCacheKey = c.bitmap()->cacheKey();
            maskCacheKey = c.mask()->cacheKey();
        }
    }
}

/*!
    \class QWindowsCursor
    \brief Platform cursor implementation

    Note that whereas under X11, a cursor can be set as a property of
    a window, there is only a global SetCursor() function on Windows.
    Each Window sets on the global cursor on receiving a Enter-event
    as do the Window manager frames (resize/move handles).

    \internal
    \ingroup qt-lighthouse-win
    \sa QWindowsWindowCursor
*/

HCURSOR QWindowsCursor::createPixmapCursor(const QPixmap &pixmap, const QPoint &hotSpot)
{
    HCURSOR cur = 0;
    QBitmap mask = pixmap.mask();
    if (mask.isNull()) {
        mask = QBitmap(pixmap.size());
        mask.fill(Qt::color1);
    }

    HBITMAP ic = qt_pixmapToWinHBITMAP(pixmap, /* HBitmapAlpha */ 2);
    const HBITMAP im = qt_createIconMask(mask);

    ICONINFO ii;
    ii.fIcon     = 0;
    ii.xHotspot  = hotSpot.x();
    ii.yHotspot  = hotSpot.y();
    ii.hbmMask   = im;
    ii.hbmColor  = ic;

    cur = CreateIconIndirect(&ii);

    DeleteObject(ic);
    DeleteObject(im);
    return cur;
}

// Create a cursor from image and mask of the format QImage::Format_Mono.
static HCURSOR createBitmapCursor(const QImage &bbits, const QImage &mbits,
                                  QPoint hotSpot = QPoint(),
                                  bool invb = false, bool invm = false)
{
    const int width = bbits.width();
    const int height = bbits.height();
    if (hotSpot.isNull())
        hotSpot = QPoint(width / 2, height / 2);
    const int n = qMax(1, width / 8);
#if !defined(Q_OS_WINCE)
    QScopedArrayPointer<uchar> xBits(new uchar[height * n]);
    QScopedArrayPointer<uchar> xMask(new uchar[height * n]);
    int x = 0;
    for (int i = 0; i < height; ++i) {
        const uchar *bits = bbits.scanLine(i);
        const uchar *mask = mbits.scanLine(i);
        for (int j = 0; j < n; ++j) {
            uchar b = bits[j];
            uchar m = mask[j];
            if (invb)
                b ^= 0xff;
            if (invm)
                m ^= 0xff;
            xBits[x] = ~m;
            xMask[x] = b ^ m;
            ++x;
        }
    }
    return CreateCursor(GetModuleHandle(0), hotSpot.x(), hotSpot.y(), width, height,
                        xBits.data(), xMask.data());
#elif defined(GWES_ICONCURS) // Q_OS_WINCE
    // Windows CE only supports fixed cursor size.
    int sysW = GetSystemMetrics(SM_CXCURSOR);
    int sysH = GetSystemMetrics(SM_CYCURSOR);
    int sysN = qMax(1, sysW / 8);
    uchar* xBits = new uchar[sysH * sysN];
    uchar* xMask = new uchar[sysH * sysN];
    int x = 0;
    for (int i = 0; i < sysH; ++i) {
        if (i >= height) {
            memset(&xBits[x] , 255, sysN);
            memset(&xMask[x] ,   0, sysN);
            x += sysN;
        } else {
            int fillWidth = n > sysN ? sysN : n;
            const uchar *bits = bbits.scanLine(i);
            const uchar *mask = mbits.scanLine(i);
            for (int j = 0; j < fillWidth; ++j) {
                uchar b = bits[j];
                uchar m = mask[j];
                if (invb)
                    b ^= 0xFF;
                if (invm)
                    m ^= 0xFF;
                xBits[x] = ~m;
                xMask[x] = b ^ m;
                ++x;
            }
            for (int j = fillWidth; j < sysN; ++j ) {
                xBits[x] = 255;
                xMask[x] = 0;
                ++x;
            }
        }
    }

    HCURSOR hcurs = CreateCursor(qWinAppInst(), hotSpot.x(), hotSpot.y(), sysW, sysH,
                                 xBits, xMask);
    delete [] xBits;
    delete [] xMask;
    return hcurs;
#else
    Q_UNUSED(n);
    Q_UNUSED(invm);
    Q_UNUSED(invb);
    Q_UNUSED(mbits);
    return 0;
#endif
}

static inline QSize systemCursorSize() { return QSize(GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR)); }
static inline QSize standardCursorSize() { return QSize(32, 32); }

#if defined (Q_OS_WINCE) || defined (QT_NO_IMAGEFORMAT_PNG)
// Create pixmap cursors from data and scale the image if the cursor size is
// higher than the standard 32. Note that bitmap cursors as produced by
// createBitmapCursor() only work for standard sizes (32,48,64...), which does
// not work when scaling the 16x16 openhand cursor bitmaps to 150% (resulting
// in a non-standard 24x24 size).
static QCursor createPixmapCursorFromData(const QSize &systemCursorSize,
                                          // The cursor size the bitmap is targeted for
                                          const QSize &bitmapTargetCursorSize,
                                          // The actual size of the bitmap data
                                          int bitmapSize, const uchar *bits,
                                          const uchar *maskBits)
{
    QPixmap rawImage = QPixmap::fromImage(QBitmap::fromData(QSize(bitmapSize, bitmapSize), bits).toImage());
    rawImage.setMask(QBitmap::fromData(QSize(bitmapSize, bitmapSize), maskBits));

    const qreal factor = qreal(systemCursorSize.width()) / qreal(bitmapTargetCursorSize.width());
    // Scale images if the cursor size is significantly different, starting with 150% where the system cursor
    // size is 48.
    if (qAbs(factor - 1.0) > 0.4) {
        const QTransform transform = QTransform::fromScale(factor, factor);
        rawImage = rawImage.transformed(transform, Qt::SmoothTransformation);
    }
    const QPoint hotSpot(rawImage.width() / 2, rawImage.height() / 2);
    return QCursor(rawImage, hotSpot.x(), hotSpot.y());
}

QCursor QWindowsCursor::customCursor(Qt::CursorShape cursorShape)
{
    // Non-standard Windows cursors are created from bitmaps
    static const uchar vsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar vsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
        0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
        0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
        0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
        0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
   static const uchar openhand_bits[] = {
        0x80,0x01,0x58,0x0e,0x64,0x12,0x64,0x52,0x48,0xb2,0x48,0x92,
        0x16,0x90,0x19,0x80,0x11,0x40,0x02,0x40,0x04,0x40,0x04,0x20,
        0x08,0x20,0x10,0x10,0x20,0x10,0x00,0x00};
    static const uchar openhandm_bits[] = {
       0x80,0x01,0xd8,0x0f,0xfc,0x1f,0xfc,0x5f,0xf8,0xff,0xf8,0xff,
       0xf6,0xff,0xff,0xff,0xff,0x7f,0xfe,0x7f,0xfc,0x7f,0xfc,0x3f,
       0xf8,0x3f,0xf0,0x1f,0xe0,0x1f,0x00,0x00};
    static const uchar closedhand_bits[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x0d,0x48,0x32,0x08,0x50,
        0x10,0x40,0x18,0x40,0x04,0x40,0x04,0x20,0x08,0x20,0x10,0x10,
        0x20,0x10,0x20,0x10,0x00,0x00,0x00,0x00};
    static const uchar closedhandm_bits[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x0d,0xf8,0x3f,0xf8,0x7f,
        0xf0,0x7f,0xf8,0x7f,0xfc,0x7f,0xfc,0x3f,0xf8,0x3f,0xf0,0x1f,
        0xe0,0x1f,0xe0,0x1f,0x00,0x00,0x00,0x00};

    static const char * const moveDragCursorXpmC[] = {
    "11 20 3 1",
    ".        c None",
    "a        c #FFFFFF",
    "X        c #000000", // X11 cursor is traditionally black
    "aa.........",
    "aXa........",
    "aXXa.......",
    "aXXXa......",
    "aXXXXa.....",
    "aXXXXXa....",
    "aXXXXXXa...",
    "aXXXXXXXa..",
    "aXXXXXXXXa.",
    "aXXXXXXXXXa",
    "aXXXXXXaaaa",
    "aXXXaXXa...",
    "aXXaaXXa...",
    "aXa..aXXa..",
    "aa...aXXa..",
    "a.....aXXa.",
    "......aXXa.",
    ".......aXXa",
    ".......aXXa",
    "........aa."};

    static const char * const copyDragCursorXpmC[] = {
    "24 30 3 1",
    ".        c None",
    "a        c #000000",
    "X        c #FFFFFF",
    "XX......................",
    "XaX.....................",
    "XaaX....................",
    "XaaaX...................",
    "XaaaaX..................",
    "XaaaaaX.................",
    "XaaaaaaX................",
    "XaaaaaaaX...............",
    "XaaaaaaaaX..............",
    "XaaaaaaaaaX.............",
    "XaaaaaaXXXX.............",
    "XaaaXaaX................",
    "XaaXXaaX................",
    "XaX..XaaX...............",
    "XX...XaaX...............",
    "X.....XaaX..............",
    "......XaaX..............",
    ".......XaaX.............",
    ".......XaaX.............",
    "........XX...aaaaaaaaaaa",
    ".............aXXXXXXXXXa",
    ".............aXXXXXXXXXa",
    ".............aXXXXaXXXXa",
    ".............aXXXXaXXXXa",
    ".............aXXaaaaaXXa",
    ".............aXXXXaXXXXa",
    ".............aXXXXaXXXXa",
    ".............aXXXXXXXXXa",
    ".............aXXXXXXXXXa",
    ".............aaaaaaaaaaa"};

    static const char * const linkDragCursorXpmC[] = {
    "24 30 3 1",
    ".        c None",
    "a        c #000000",
    "X        c #FFFFFF",
    "XX......................",
    "XaX.....................",
    "XaaX....................",
    "XaaaX...................",
    "XaaaaX..................",
    "XaaaaaX.................",
    "XaaaaaaX................",
    "XaaaaaaaX...............",
    "XaaaaaaaaX..............",
    "XaaaaaaaaaX.............",
    "XaaaaaaXXXX.............",
    "XaaaXaaX................",
    "XaaXXaaX................",
    "XaX..XaaX...............",
    "XX...XaaX...............",
    "X.....XaaX..............",
    "......XaaX..............",
    ".......XaaX.............",
    ".......XaaX.............",
    "........XX...aaaaaaaaaaa",
    ".............aXXXXXXXXXa",
    ".............aXXXaaaaXXa",
    ".............aXXXXaaaXXa",
    ".............aXXXaaaaXXa",
    ".............aXXaaaXaXXa",
    ".............aXXaaXXXXXa",
    ".............aXXaXXXXXXa",
    ".............aXXXaXXXXXa",
    ".............aXXXXXXXXXa",
    ".............aaaaaaaaaaa"};

    switch (cursorShape) {
    case Qt::SplitVCursor:
        return createPixmapCursorFromData(systemCursorSize(), standardCursorSize(), 32, vsplit_bits, vsplitm_bits);
    case Qt::SplitHCursor:
        return createPixmapCursorFromData(systemCursorSize(), standardCursorSize(), 32, hsplit_bits, hsplitm_bits);
    case Qt::OpenHandCursor:
        return createPixmapCursorFromData(systemCursorSize(), standardCursorSize(), 16, openhand_bits, openhandm_bits);
    case Qt::ClosedHandCursor:
        return createPixmapCursorFromData(systemCursorSize(), standardCursorSize(), 16, closedhand_bits, closedhandm_bits);
    case Qt::DragCopyCursor:
        return QCursor(QPixmap(copyDragCursorXpmC), 0, 0);
    case Qt::DragMoveCursor:
        return QCursor(QPixmap(moveDragCursorXpmC), 0, 0);
    case Qt::DragLinkCursor:
        return QCursor(QPixmap(linkDragCursorXpmC), 0, 0);
    }

    return QCursor();
}
#else // Q_OS_WINCE || QT_NO_IMAGEFORMAT_PNG
struct QWindowsCustomPngCursor {
    Qt::CursorShape shape;
    int size;
    const char *fileName;
    int hotSpotX;
    int hotSpotY;
};

QCursor QWindowsCursor::customCursor(Qt::CursorShape cursorShape)
{
    static const QWindowsCustomPngCursor pngCursors[] = {
        { Qt::SplitVCursor, 32, "splitvcursor_32.png", 11, 11 },
        { Qt::SplitVCursor, 48, "splitvcursor_48.png", 16, 17 },
        { Qt::SplitVCursor, 64, "splitvcursor_64.png", 22, 22 },
        { Qt::SplitHCursor, 32, "splithcursor_32.png", 11, 11 },
        { Qt::SplitHCursor, 48, "splithcursor_48.png", 16, 17 },
        { Qt::SplitHCursor, 64, "splithcursor_64.png", 22, 22 },
        { Qt::OpenHandCursor, 32, "openhandcursor_32.png", 10, 12 },
        { Qt::OpenHandCursor, 48, "openhandcursor_48.png", 15, 16 },
        { Qt::OpenHandCursor, 64, "openhandcursor_64.png", 20, 24 },
        { Qt::ClosedHandCursor, 32, "closedhandcursor_32.png", 10, 12 },
        { Qt::ClosedHandCursor, 48, "closedhandcursor_48.png", 15, 16 },
        { Qt::ClosedHandCursor, 64, "closedhandcursor_64.png", 20, 24 },
        { Qt::DragCopyCursor, 32, "dragcopycursor_32.png", 0, 0 },
        { Qt::DragCopyCursor, 48, "dragcopycursor_48.png", 0, 0 },
        { Qt::DragCopyCursor, 64, "dragcopycursor_64.png", 0, 0 },
        { Qt::DragMoveCursor, 32, "dragmovecursor_32.png", 0, 0 },
        { Qt::DragMoveCursor, 48, "dragmovecursor_48.png", 0, 0 },
        { Qt::DragMoveCursor, 64, "dragmovecursor_64.png", 0, 0 },
        { Qt::DragLinkCursor, 32, "draglinkcursor_32.png", 0, 0 },
        { Qt::DragLinkCursor, 48, "draglinkcursor_48.png", 0, 0 },
        { Qt::DragLinkCursor, 64, "draglinkcursor_64.png", 0, 0 }
    };

    const int cursorSize = GetSystemMetrics(SM_CXCURSOR);
    const QWindowsCustomPngCursor *sEnd = pngCursors + sizeof(pngCursors) / sizeof(pngCursors[0]);
    const QWindowsCustomPngCursor *bestFit = 0;
    int sizeDelta = INT_MAX;
    for (const QWindowsCustomPngCursor *s = pngCursors; s < sEnd; ++s) {
        if (s->shape != cursorShape)
            continue;
        const int currentSizeDelta = qMax(s->size, cursorSize) - qMin(s->size, cursorSize);
        if (currentSizeDelta < sizeDelta) {
            bestFit = s;
            if (currentSizeDelta == 0)
                break; // Perfect match found
            sizeDelta = currentSizeDelta;
        }
    }

    if (!bestFit)
        return QCursor();

    const QPixmap rawImage(QStringLiteral(":/qt-project.org/windows/cursors/images/") +
                           QString::fromLatin1(bestFit->fileName));
    return QCursor(rawImage, bestFit->hotSpotX, bestFit->hotSpotY);
}
#endif // Q_OS_WINCE || QT_NO_IMAGEFORMAT_PNG

struct QWindowsStandardCursorMapping {
    Qt::CursorShape shape;
    LPCWSTR resource;
};

HCURSOR QWindowsCursor::createSystemCursor(const QCursor &c)
{
    static const QWindowsStandardCursorMapping standardCursors[] = {
        { Qt::ArrowCursor, IDC_ARROW},
        { Qt::UpArrowCursor, IDC_UPARROW },
        { Qt::CrossCursor, IDC_CROSS },
        { Qt::WaitCursor, IDC_WAIT },
        { Qt::IBeamCursor, IDC_IBEAM },
        { Qt::SizeVerCursor, IDC_SIZENS },
        { Qt::SizeHorCursor, IDC_SIZEWE },
        { Qt::SizeBDiagCursor, IDC_SIZENESW },
        { Qt::SizeFDiagCursor, IDC_SIZENWSE },
        { Qt::SizeAllCursor, IDC_SIZEALL },
        { Qt::ForbiddenCursor, IDC_NO },
        { Qt::WhatsThisCursor, IDC_HELP },
        { Qt::BusyCursor, IDC_APPSTARTING },
        { Qt::PointingHandCursor, IDC_HAND }
    };

    const Qt::CursorShape cursorShape = c.shape();
    switch (cursorShape) {
    case Qt::BitmapCursor: {
        const QPixmap pixmap = c.pixmap();
        if (!pixmap.isNull())
            return QWindowsCursor::createPixmapCursor(pixmap, c.hotSpot());
        const QImage bbits = c.bitmap()->toImage().convertToFormat(QImage::Format_Mono);
        const QImage mbits = c.mask()->toImage().convertToFormat(QImage::Format_Mono);
        const bool invb = bbits.colorCount() > 1 && qGray(bbits.color(0)) < qGray(bbits.color(1));
        const bool invm = mbits.colorCount() > 1 && qGray(mbits.color(0)) < qGray(mbits.color(1));
        return createBitmapCursor(bbits, mbits, c.hotSpot(), invb, invm);
    }
    case Qt::BlankCursor: {
        QImage blank = QImage(systemCursorSize(), QImage::Format_Mono);
        blank.fill(0); // ignore color table
        return createBitmapCursor(blank, blank);
    }
    case Qt::SplitVCursor:
    case Qt::SplitHCursor:
    case Qt::OpenHandCursor:
    case Qt::ClosedHandCursor:
    case Qt::DragCopyCursor:
    case Qt::DragMoveCursor:
    case Qt::DragLinkCursor:
        return createSystemCursor(customCursor(cursorShape));
    default:
        break;
    }

    // Load available standard cursors from resources
    const QWindowsStandardCursorMapping *sEnd = standardCursors + sizeof(standardCursors) / sizeof(standardCursors[0]);
    for (const QWindowsStandardCursorMapping *s = standardCursors; s < sEnd; ++s) {
        if (s->shape == cursorShape) {
#ifndef Q_OS_WINCE
            return (HCURSOR)LoadImage(0, s->resource, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
#else
            return LoadCursor(0, s->resource);
#endif
        }
    }

    qWarning("%s: Invalid cursor shape %d", __FUNCTION__, cursorShape);
    return 0;
}

/*!
    \brief Return cached standard cursor resources or create new ones.
*/

QWindowsWindowCursor QWindowsCursor::standardWindowCursor(Qt::CursorShape shape)
{
    const QWindowsCursorCacheKey key(shape);
    CursorCache::iterator it = m_cursorCache.find(key);
    if (it == m_cursorCache.end())
        it = m_cursorCache.insert(key, QWindowsWindowCursor(QCursor(shape)));
    return it.value();
}

/*!
    \brief Return cached pixmap cursor or create new one.
*/

QWindowsWindowCursor QWindowsCursor::pixmapWindowCursor(const QCursor &c)
{
    const  QWindowsCursorCacheKey cacheKey(c);
    CursorCache::iterator it = m_cursorCache.find(cacheKey);
    if (it == m_cursorCache.end()) {
        if (m_cursorCache.size() > 50) {
            // Prevent the cursor cache from growing indefinitely hitting GDI resource
            // limits if new pixmap cursors are created repetitively by purging out
            // all-noncurrent pixmap cursors (QTBUG-43515)
            const HCURSOR currentCursor = GetCursor();
            for (it = m_cursorCache.begin(); it != m_cursorCache.end() ; ) {
                if (it.key().bitmapCacheKey && it.value().handle() != currentCursor)
                    it = m_cursorCache.erase(it);
                else
                    ++it;
            }
        }
        it = m_cursorCache.insert(cacheKey, QWindowsWindowCursor(c));
    }
    return it.value();
}

QWindowsCursor::QWindowsCursor()
{
    initResources();
}

/*!
    \brief Set a cursor on a window.

    This is called frequently as the mouse moves over widgets in the window
    (QLineEdits, etc).
*/

void QWindowsCursor::changeCursor(QCursor *cursorIn, QWindow *window)
{
    if (!window)
        return;
    if (!cursorIn) {
        QWindowsWindow::baseWindowOf(window)->setCursor(QWindowsWindowCursor());
        return;
    }
    const QWindowsWindowCursor wcursor =
        cursorIn->shape() == Qt::BitmapCursor ?
        pixmapWindowCursor(*cursorIn) : standardWindowCursor(cursorIn->shape());
    if (wcursor.handle()) {
        QWindowsWindow::baseWindowOf(window)->setCursor(wcursor);
    } else {
        qWarning("%s: Unable to obtain system cursor for %d",
                 __FUNCTION__, cursorIn->shape());
    }
}

QPoint QWindowsCursor::mousePosition()
{
    POINT p;
    GetCursorPos(&p);
    return QPoint(p.x, p.y);
}

QWindowsCursor::CursorState QWindowsCursor::cursorState()
{
#ifndef Q_OS_WINCE
    enum { cursorShowing = 0x1, cursorSuppressed = 0x2 }; // Windows 8: CURSOR_SUPPRESSED
    CURSORINFO cursorInfo;
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (GetCursorInfo(&cursorInfo)) {
        if (cursorInfo.flags & CursorShowing)
            return CursorShowing;
        if (cursorInfo.flags & cursorSuppressed)
            return CursorSuppressed;
    }
#endif // !Q_OS_WINCE
    return CursorHidden;
}

QPoint QWindowsCursor::pos() const
{
    return mousePosition() / QWindowsScaling::factor();
}

void QWindowsCursor::setPos(const QPoint &pos)
{
    const QPoint posDp = pos * QWindowsScaling::factor();
    SetCursorPos(posDp.x() , posDp.y());
}

/*!
    \class QWindowsWindowCursor
    \brief Per-Window cursor. Contains a QCursor and manages its associated system
     cursor handle resource.

    Based on QSharedDataPointer, so that it can be passed around and
    used as a property of QWindowsBaseWindow.

    \internal
    \ingroup qt-lighthouse-win
    \sa QWindowsCursor
*/

class QWindowsWindowCursorData : public QSharedData
{
public:
    QWindowsWindowCursorData() : m_cursor(Qt::ArrowCursor), m_handle(0) {}
    explicit QWindowsWindowCursorData(const QCursor &c);
    ~QWindowsWindowCursorData();

    const QCursor m_cursor;
    const HCURSOR m_handle;
};

QWindowsWindowCursorData::QWindowsWindowCursorData(const QCursor &c) :
    m_cursor(c),
    m_handle(QWindowsCursor::createSystemCursor(c))
{
}

QWindowsWindowCursorData::~QWindowsWindowCursorData()
{
    if (m_handle)
        DestroyCursor(m_handle);
}

QWindowsWindowCursor::QWindowsWindowCursor() :
    m_data(new QWindowsWindowCursorData)
{
}

QWindowsWindowCursor::QWindowsWindowCursor(const QCursor &c) :
    m_data(new QWindowsWindowCursorData(c))
{
}

QWindowsWindowCursor::~QWindowsWindowCursor()
{
}

QWindowsWindowCursor::QWindowsWindowCursor(const QWindowsWindowCursor &rhs) :
    m_data(rhs.m_data)
{
}

QWindowsWindowCursor & QWindowsWindowCursor::operator =(const QWindowsWindowCursor &rhs)
{
    if (this != &rhs)
        m_data.operator =(rhs.m_data);
    return *this;
}

bool QWindowsWindowCursor::isNull() const
{
    return m_data->m_handle == 0;
}

QCursor QWindowsWindowCursor::cursor() const
{
    return m_data->m_cursor;
}

HCURSOR QWindowsWindowCursor::handle() const
{
    return m_data->m_handle;
}

QT_END_NAMESPACE

#endif // !QT_NO_CURSOR
