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

// Uncomment the next line to enable the MIT Shared Memory extension
//
// WARNING:  This has some problems:
//
//    1. Consumes a 800x600 pixmap
//    2. Qt does not handle the ShmCompletion message, so you will
//        get strange effects if you xForm() repeatedly.
//
// #define QT_MITSHM

#if defined(Q_OS_WIN32) && defined(QT_MITSHM)
#undef QT_MITSHM
#endif

#include "qplatformdefs.h"

#include "qdebug.h"
#include "qiodevice.h"
#include "qpixmap_x11_p.h"
#include "qbitmap.h"
#include "qcolormap.h"
#include "qimage.h"
#include "qmatrix.h"
#include "qapplication.h"
#include <private/qpaintengine_x11_p.h>
#include <private/qt_x11_p.h>
#include "qx11info_x11.h"
#include <private/qdrawhelper_p.h>
#include <private/qimage_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

#include <stdlib.h>

#if defined(Q_CC_MIPS)
#  define for if(0){}else for
#endif

QT_BEGIN_NAMESPACE

QPixmap qt_toX11Pixmap(const QImage &image)
{
    QPixmapData *data =
        new QX11PixmapData(image.depth() == 1
                           ? QPixmapData::BitmapType
                           : QPixmapData::PixmapType);

    data->fromImage(image, Qt::AutoColor);

    return QPixmap(data);
}

QPixmap qt_toX11Pixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return QPixmap();

    if (QPixmap(pixmap).data_ptr()->classId() == QPixmapData::X11Class)
        return pixmap;

    return qt_toX11Pixmap(pixmap.toImage());
}

// For thread-safety:
//   image->data does not belong to X11, so we must free it ourselves.

inline static void qSafeXDestroyImage(XImage *x)
{
    if (x->data) {
        free(x->data);
        x->data = 0;
    }
    XDestroyImage(x);
}

QBitmap QX11PixmapData::mask_to_bitmap(int screen) const
{
    if (!x11_mask)
        return QBitmap();
    QPixmap::x11SetDefaultScreen(screen);
    QBitmap bm(w, h);
    GC gc = XCreateGC(X11->display, bm.handle(), 0, 0);
    XCopyArea(X11->display, x11_mask, bm.handle(), gc, 0, 0,
              bm.data->width(), bm.data->height(), 0, 0);
    XFreeGC(X11->display, gc);
    return bm;
}

Qt::HANDLE QX11PixmapData::bitmap_to_mask(const QBitmap &bitmap, int screen)
{
    if (bitmap.isNull())
        return 0;
    QBitmap bm = bitmap;
    bm.x11SetScreen(screen);

    Pixmap mask = XCreatePixmap(X11->display, RootWindow(X11->display, screen),
                                bm.data->width(), bm.data->height(), 1);
    GC gc = XCreateGC(X11->display, mask, 0, 0);
    XCopyArea(X11->display, bm.handle(), mask, gc, 0, 0,
              bm.data->width(), bm.data->height(), 0, 0);
    XFreeGC(X11->display, gc);
    return mask;
}


/*****************************************************************************
  MIT Shared Memory Extension support: makes xForm noticeably (~20%) faster.
 *****************************************************************************/

#if defined(QT_MITSHM)

static bool               xshminit = false;
static XShmSegmentInfo xshminfo;
static XImage              *xshmimg = 0;
static Pixmap               xshmpm  = 0;

static void qt_cleanup_mitshm()
{
    if (xshmimg == 0)
        return;
    Display *dpy = QX11Info::appDisplay();
    if (xshmpm) {
        XFreePixmap(dpy, xshmpm);
        xshmpm = 0;
    }
    XShmDetach(dpy, &xshminfo); xshmimg->data = 0;
    qSafeXDestroyImage(xshmimg); xshmimg = 0;
    shmdt(xshminfo.shmaddr);
    shmctl(xshminfo.shmid, IPC_RMID, 0);
}

static bool qt_create_mitshm_buffer(const QPaintDevice* dev, int w, int h)
{
    static int major, minor;
    static Bool pixmaps_ok;
    Display *dpy = dev->data->xinfo->display();
    int dd         = dev->x11Depth();
    Visual *vis         = (Visual*)dev->x11Visual();

    if (xshminit) {
        qt_cleanup_mitshm();
    } else {
        if (!XShmQueryVersion(dpy, &major, &minor, &pixmaps_ok))
            return false;                        // MIT Shm not supported
        qAddPostRoutine(qt_cleanup_mitshm);
        xshminit = true;
    }

    xshmimg = XShmCreateImage(dpy, vis, dd, ZPixmap, 0, &xshminfo, w, h);
    if (!xshmimg)
        return false;

    bool ok;
    xshminfo.shmid = shmget(IPC_PRIVATE,
                             xshmimg->bytes_per_line * xshmimg->height,
                             IPC_CREAT | 0777);
    ok = xshminfo.shmid != -1;
    if (ok) {
        xshmimg->data = (char*)shmat(xshminfo.shmid, 0, 0);
        xshminfo.shmaddr = xshmimg->data;
        ok = (xshminfo.shmaddr != (char*)-1);
    }
    xshminfo.readOnly = false;
    if (ok)
        ok = XShmAttach(dpy, &xshminfo);
    if (!ok) {
        qSafeXDestroyImage(xshmimg);
        xshmimg = 0;
        if (xshminfo.shmaddr)
            shmdt(xshminfo.shmaddr);
        if (xshminfo.shmid != -1)
            shmctl(xshminfo.shmid, IPC_RMID, 0);
        return false;
    }
    if (pixmaps_ok)
        xshmpm = XShmCreatePixmap(dpy, DefaultRootWindow(dpy), xshmimg->data,
                                   &xshminfo, w, h, dd);

    return true;
}

#else

// If extern, need a dummy.
//
// static bool qt_create_mitshm_buffer(QPaintDevice*, int, int)
// {
//     return false;
// }

#endif // QT_MITSHM


/*****************************************************************************
  Internal functions
 *****************************************************************************/

extern const uchar *qt_get_bitflip_array();                // defined in qimage.cpp

// Returns position of highest bit set or -1 if none
static int highest_bit(uint v)
{
    int i;
    uint b = (uint)1 << 31;
    for (i=31; ((b & v) == 0) && i>=0;         i--)
        b >>= 1;
    return i;
}

// Returns position of lowest set bit in 'v' as an integer (0-31), or -1
static int lowest_bit(uint v)
{
    int i;
    ulong lb;
    lb = 1;
    for (i=0; ((v & lb) == 0) && i<32;  i++, lb<<=1) {}
    return i==32 ? -1 : i;
}

// Counts the number of bits set in 'v'
static uint n_bits(uint v)
{
    int i = 0;
    while (v) {
        v = v & (v - 1);
        i++;
    }
    return i;
}

static uint *red_scale_table   = 0;
static uint *green_scale_table = 0;
static uint *blue_scale_table  = 0;

static void cleanup_scale_tables()
{
    delete[] red_scale_table;
    delete[] green_scale_table;
    delete[] blue_scale_table;
}

/*
  Could do smart bitshifting, but the "obvious" algorithm only works for
  nBits >= 4. This is more robust.
*/
static void build_scale_table(uint **table, uint nBits)
{
    if (nBits > 7) {
        qWarning("build_scale_table: internal error, nBits = %i", nBits);
        return;
    }
    if (!*table) {
        static bool firstTable = true;
        if (firstTable) {
            qAddPostRoutine(cleanup_scale_tables);
            firstTable = false;
        }
        *table = new uint[256];
    }
    int   maxVal   = (1 << nBits) - 1;
    int   valShift = 8 - nBits;
    int i;
    for(i = 0 ; i < maxVal + 1 ; i++)
        (*table)[i << valShift] = i*255/maxVal;
}

static int defaultScreen = -1;

/*****************************************************************************
  QPixmap member functions
 *****************************************************************************/

QBasicAtomicInt qt_pixmap_serial = Q_BASIC_ATOMIC_INITIALIZER(0);
int Q_GUI_EXPORT qt_x11_preferred_pixmap_depth = 0;

QX11PixmapData::QX11PixmapData(PixelType type)
    : QPixmapData(type, X11Class), gl_surface(0), hd(0),
      flags(Uninitialized), x11_mask(0), picture(0), mask_picture(0), hd2(0),
      share_mode(QPixmap::ImplicitlyShared), pengine(0)
{
}

QPixmapData *QX11PixmapData::createCompatiblePixmapData() const
{
    return new QX11PixmapData(pixelType());
}

void QX11PixmapData::resize(int width, int height)
{
    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);

    if (defaultScreen >= 0 && defaultScreen != xinfo.screen()) {
        QX11InfoData* xd = xinfo.getX11Data(true);
        xd->screen = defaultScreen;
        xd->depth = QX11Info::appDepth(xd->screen);
        xd->cells = QX11Info::appCells(xd->screen);
        xd->colormap = QX11Info::appColormap(xd->screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(xd->screen);
        xd->visual = (Visual *)QX11Info::appVisual(xd->screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(xd->screen);
        xinfo.setX11Data(xd);
    }

    int dd = xinfo.depth();

    if (qt_x11_preferred_pixmap_depth)
        dd = qt_x11_preferred_pixmap_depth;

    bool make_null = w <= 0 || h <= 0;                // create null pixmap
    d = (pixelType() == BitmapType ? 1 : dd);
    if (make_null || d == 0) {
        w = 0;
        h = 0;
        is_null = true;
        hd = 0;
        picture = 0;
        d = 0;
        if (!make_null)
            qWarning("QPixmap: Invalid pixmap parameters");
        return;
    }
    hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                   RootWindow(X11->display, xinfo.screen()),
                                   w, h, d);
#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 1
                                    ? XRenderFindStandardFormat(X11->display, PictStandardA1)
                                    : XRenderFindVisualFormat(X11->display, (Visual *)xinfo.visual());
        picture = XRenderCreatePicture(X11->display, hd, format, 0, 0);
    }
#endif // QT_NO_XRENDER
}

struct QX11AlphaDetector
{
    bool hasAlpha() const {
        if (checked)
            return has;
        // Will implicitly also check format and return quickly for opaque types...
        checked = true;
        has = image->isNull() ? false : const_cast<QImage *>(image)->data_ptr()->checkForAlphaPixels();
        return has;
    }

    bool hasXRenderAndAlpha() const {
        if (!X11->use_xrender)
            return false;
        return hasAlpha();
    }

    QX11AlphaDetector(const QImage *i, Qt::ImageConversionFlags flags)
        : image(i), checked(false), has(false)
    {
        if (flags & Qt::NoOpaqueDetection) {
            checked = true;
            has = image->hasAlphaChannel();
        }
    }

    const QImage *image;
    mutable bool checked;
    mutable bool has;
};

void QX11PixmapData::fromImage(const QImage &img,
                               Qt::ImageConversionFlags flags)
{
    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    w = img.width();
    h = img.height();
    d = img.depth();
    is_null = (w <= 0 || h <= 0);

    if (is_null) {
        w = h = 0;
        return;
    }

    if (defaultScreen >= 0 && defaultScreen != xinfo.screen()) {
        QX11InfoData* xd = xinfo.getX11Data(true);
        xd->screen = defaultScreen;
        xd->depth = QX11Info::appDepth(xd->screen);
        xd->cells = QX11Info::appCells(xd->screen);
        xd->colormap = QX11Info::appColormap(xd->screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(xd->screen);
        xd->visual = (Visual *)QX11Info::appVisual(xd->screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(xd->screen);
        xinfo.setX11Data(xd);
    }

    if (pixelType() == BitmapType) {
        bitmapFromImage(img);
        return;
    }

    if (uint(w) >= 32768 || uint(h) >= 32768) {
        w = h = 0;
        is_null = true;
        return;
    }

    QX11AlphaDetector alphaCheck(&img, flags);
    int dd = alphaCheck.hasXRenderAndAlpha() ? 32 : xinfo.depth();

    if (qt_x11_preferred_pixmap_depth)
        dd = qt_x11_preferred_pixmap_depth;

    QImage image = img;

    // must be monochrome
    if (dd == 1 || (flags & Qt::ColorMode_Mask) == Qt::MonoOnly) {
        if (d != 1) {
            // dither
            image = image.convertToFormat(QImage::Format_MonoLSB, flags);
            d = 1;
        }
    } else { // can be both
        bool conv8 = false;
        if (d > 8 && dd <= 8) { // convert to 8 bit
            if ((flags & Qt::DitherMode_Mask) == Qt::AutoDither)
                flags = (flags & ~Qt::DitherMode_Mask)
                        | Qt::PreferDither;
            conv8 = true;
        } else if ((flags & Qt::ColorMode_Mask) == Qt::ColorOnly) {
            conv8 = (d == 1);                        // native depth wanted
        } else if (d == 1) {
            if (image.colorCount() == 2) {
                QRgb c0 = image.color(0);        // Auto: convert to best
                QRgb c1 = image.color(1);
                conv8 = qMin(c0,c1) != qRgb(0,0,0) || qMax(c0,c1) != qRgb(255,255,255);
            } else {
                // eg. 1-color monochrome images (they do exist).
                conv8 = true;
            }
        }
        if (conv8) {
            image = image.convertToFormat(QImage::Format_Indexed8, flags);
            d = 8;
        }
    }

    if (d == 1 || d == 16 || d == 24) {
        image = image.convertToFormat(QImage::Format_RGB32, flags);
        fromImage(image, Qt::AutoColor);
        return;
    }

    Display *dpy   = X11->display;
    Visual *visual = (Visual *)xinfo.visual();
    XImage *xi = 0;
    bool    trucol = (visual->c_class >= TrueColor);
    int     nbytes = image.byteCount();
    uchar  *newbits= 0;

#ifndef QT_NO_XRENDER
    if (alphaCheck.hasXRenderAndAlpha()) {
        const QImage &cimage = image;

        d = 32;

        if (QX11Info::appDepth() != d) {
            if (xinfo.x11data) {
                xinfo.x11data->depth = d;
            } else {
                QX11InfoData *xd = xinfo.getX11Data(true);
                xd->screen = QX11Info::appScreen();
                xd->depth = d;
                xd->cells = QX11Info::appCells();
                xd->colormap = QX11Info::appColormap();
                xd->defaultColormap = QX11Info::appDefaultColormap();
                xd->visual = (Visual *)QX11Info::appVisual();
                xd->defaultVisual = QX11Info::appDefaultVisual();
                xinfo.setX11Data(xd);
            }
        }

        hd = (Qt::HANDLE)XCreatePixmap(dpy, RootWindow(dpy, xinfo.screen()),
                                       w, h, d);
        picture = XRenderCreatePicture(X11->display, hd,
                                       XRenderFindStandardFormat(X11->display, PictStandardARGB32), 0, 0);

        xi = XCreateImage(dpy, visual, d, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        xi->data = (char *)newbits;

        switch(cimage.format()) {
        case QImage::Format_Indexed8: {
            QVector<QRgb> colorTable = cimage.colorTable();
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const uchar *p = cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = colorTable[p[x]];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }
        }
            break;
        case QImage::Format_RGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x)
                    *xidata++ = p[x] | 0xff000000;
            }
        }
            break;
        case QImage::Format_ARGB32: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    const QRgb rgb = p[x];
                    const int a = qAlpha(rgb);
                    if (a == 0xff)
                        *xidata = rgb;
                    else
                        // RENDER expects premultiplied alpha
                        *xidata = qRgba(qt_div_255(qRed(rgb) * a),
                                        qt_div_255(qGreen(rgb) * a),
                                        qt_div_255(qBlue(rgb) * a),
                                        a);
                    ++xidata;
                }
            }

        }
            break;
        case QImage::Format_ARGB32_Premultiplied: {
            uint *xidata = (uint *)xi->data;
            for (int y = 0; y < h; ++y) {
                const QRgb *p = (const QRgb *) cimage.scanLine(y);
                memcpy(xidata, p, w*sizeof(QRgb));
                xidata += w;
            }
        }
            break;
        default:
            Q_ASSERT(false);
        }

        if ((xi->byte_order == MSBFirst) != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
            uint *xidata = (uint *)xi->data;
            uint *xiend = xidata + w*h;
            while (xidata < xiend) {
                *xidata = (*xidata >> 24)
                          | ((*xidata >> 8) & 0xff00)
                          | ((*xidata << 8) & 0xff0000)
                          | (*xidata << 24);
                ++xidata;
            }
        }

        GC gc = XCreateGC(dpy, hd, 0, 0);
        XPutImage(dpy, hd, gc, xi, 0, 0, 0, 0, w, h);
        XFreeGC(dpy, gc);

        qSafeXDestroyImage(xi);

        return;
    }
#endif // QT_NO_XRENDER

    if (trucol) {                                // truecolor display
        if (image.format() == QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);

        const QImage &cimage = image;
        QRgb  pix[256];                                // pixel translation table
        const bool  d8 = (d == 8);
        const uint  red_mask          = (uint)visual->red_mask;
        const uint  green_mask  = (uint)visual->green_mask;
        const uint  blue_mask          = (uint)visual->blue_mask;
        const int   red_shift          = highest_bit(red_mask)   - 7;
        const int   green_shift = highest_bit(green_mask) - 7;
        const int   blue_shift  = highest_bit(blue_mask)  - 7;
        const uint  rbits = highest_bit(red_mask) - lowest_bit(red_mask) + 1;
        const uint  gbits = highest_bit(green_mask) - lowest_bit(green_mask) + 1;
        const uint  bbits = highest_bit(blue_mask) - lowest_bit(blue_mask) + 1;

        if (d8) {                                // setup pixel translation
            QVector<QRgb> ctable = cimage.colorTable();
            for (int i=0; i < cimage.colorCount(); i++) {
                int r = qRed  (ctable[i]);
                int g = qGreen(ctable[i]);
                int b = qBlue (ctable[i]);
                r = red_shift        > 0 ? r << red_shift   : r >> -red_shift;
                g = green_shift > 0 ? g << green_shift : g >> -green_shift;
                b = blue_shift        > 0 ? b << blue_shift  : b >> -blue_shift;
                pix[i] = (b & blue_mask) | (g & green_mask) | (r & red_mask)
                         | ~(blue_mask | green_mask | red_mask);
            }
        }

        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        Q_CHECK_PTR(xi);
        newbits = (uchar *)malloc(xi->bytes_per_line*h);
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return;
        int bppc = xi->bits_per_pixel;

        bool contig_bits = n_bits(red_mask) == rbits &&
                           n_bits(green_mask) == gbits &&
                           n_bits(blue_mask) == bbits;
        bool dither_tc =
            // Want it?
            (flags & Qt::Dither_Mask) != Qt::ThresholdDither &&
            (flags & Qt::DitherMode_Mask) != Qt::AvoidDither &&
            // Need it?
            bppc < 24 && !d8 &&
            // Can do it? (Contiguous bits?)
            contig_bits;

        static bool init=false;
        static int D[16][16];
        if (dither_tc && !init) {
            // I also contributed this code to XV - WWA.
            /*
              The dither matrix, D, is obtained with this formula:

              D2 = [0 2]
              [3 1]


              D2*n = [4*Dn       4*Dn+2*Un]
              [4*Dn+3*Un  4*Dn+1*Un]
            */
            int n,i,j;
            init=1;

            /* Set D2 */
            D[0][0]=0;
            D[1][0]=2;
            D[0][1]=3;
            D[1][1]=1;

            /* Expand using recursive definition given above */
            for (n=2; n<16; n*=2) {
                for (i=0; i<n; i++) {
                    for (j=0; j<n; j++) {
                        D[i][j]*=4;
                        D[i+n][j]=D[i][j]+2;
                        D[i][j+n]=D[i][j]+3;
                        D[i+n][j+n]=D[i][j]+1;
                    }
                }
            }
            init=true;
        }

        enum { BPP8,
               BPP16_565, BPP16_555,
               BPP16_MSB, BPP16_LSB,
               BPP24_888,
               BPP24_MSB, BPP24_LSB,
               BPP32_8888,
               BPP32_MSB, BPP32_LSB
        } mode = BPP8;

        bool same_msb_lsb = (xi->byte_order == MSBFirst) == (QSysInfo::ByteOrder == QSysInfo::BigEndian);

        if(bppc == 8) // 8 bit
            mode = BPP8;
        else if(bppc == 16) { // 16 bit MSB/LSB
            if(red_shift == 8 && green_shift == 3 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_565;
            else if(red_shift == 7 && green_shift == 2 && blue_shift == -3 && !d8 && same_msb_lsb)
                mode = BPP16_555;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP16_LSB : BPP16_MSB;
        } else if(bppc == 24) { // 24 bit MSB/LSB
            if (red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP24_888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP24_LSB : BPP24_MSB;
        } else if(bppc == 32) { // 32 bit MSB/LSB
            if(red_shift == 16 && green_shift == 8 && blue_shift == 0 && !d8 && same_msb_lsb)
                mode = BPP32_8888;
            else
                mode = (xi->byte_order == LSBFirst) ? BPP32_LSB : BPP32_MSB;
        } else
            qFatal("Logic error 3");

#define GET_PIXEL                                                       \
        uint pixel;                                                     \
        if (d8) pixel = pix[*src++];                                    \
        else {                                                          \
            int r = qRed  (*p);                                         \
            int g = qGreen(*p);                                         \
            int b = qBlue (*p++);                                       \
            r = red_shift   > 0                                         \
                ? r << red_shift   : r >> -red_shift;                   \
            g = green_shift > 0                                         \
                ? g << green_shift : g >> -green_shift;                 \
            b = blue_shift  > 0                                         \
                ? b << blue_shift  : b >> -blue_shift;                  \
            pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask)   \
                    | ~(blue_mask | green_mask | red_mask);             \
        }

#define GET_PIXEL_DITHER_TC                                             \
        int r = qRed  (*p);                                             \
        int g = qGreen(*p);                                             \
        int b = qBlue (*p++);                                           \
        const int thres = D[x%16][y%16];                                \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        r = red_shift   > 0                                             \
            ? r << red_shift   : r >> -red_shift;                       \
        g = green_shift > 0                                             \
            ? g << green_shift : g >> -green_shift;                     \
        b = blue_shift  > 0                                             \
            ? b << blue_shift  : b >> -blue_shift;                      \
        uint pixel = (r & red_mask)|(g & green_mask) | (b & blue_mask);

// again, optimized case
// can't be optimized that much :(
#define GET_PIXEL_DITHER_TC_OPT(red_shift,green_shift,blue_shift,red_mask,green_mask,blue_mask, \
                                rbits,gbits,bbits)                      \
        const int thres = D[x%16][y%16];                                \
        int r = qRed  (*p);                                             \
        if (r <= (255-(1<<(8-rbits))) && ((r<<rbits) & 255)             \
            > thres)                                                    \
            r += (1<<(8-rbits));                                        \
        int g = qGreen(*p);                                             \
        if (g <= (255-(1<<(8-gbits))) && ((g<<gbits) & 255)             \
            > thres)                                                    \
            g += (1<<(8-gbits));                                        \
        int b = qBlue (*p++);                                           \
        if (b <= (255-(1<<(8-bbits))) && ((b<<bbits) & 255)             \
            > thres)                                                    \
            b += (1<<(8-bbits));                                        \
        uint pixel = ((r red_shift) & red_mask)                         \
                     | ((g green_shift) & green_mask)                   \
                     | ((b blue_shift) & blue_mask);

#define CYCLE(body)                                             \
        for (int y=0; y<h; y++) {                               \
            const uchar* src = cimage.scanLine(y);              \
            uchar* dst = newbits + xi->bytes_per_line*y;        \
            const QRgb* p = (const QRgb *)src;                  \
            body                                                \
                }

        if (dither_tc) {
            switch (mode) {
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<8,<<3,>>3,0xf800,0x7e0,0x1f,5,6,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC_OPT(<<7,<<2,>>3,0x7c00,0x3e0,0x1f,5,5,5)
                            *dst16++ = pixel;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL_DITHER_TC
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            default:
                qFatal("Logic error");
            }
        } else {
            switch (mode) {
            case BPP8:                        // 8 bit
                CYCLE(
                    Q_UNUSED(p);
                    for (int x=0; x<w; x++)
                        *dst++ = pix[*src++];
                    )
                    break;
            case BPP16_565:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x = 0; x < w; x++) {
                        *dst16++ = ((*p >> 8) & 0xf800)
                                   | ((*p >> 5) & 0x7e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_555:
                CYCLE(
                    quint16* dst16 = (quint16*)dst;
                    for (int x=0; x<w; x++) {
                        *dst16++ = ((*p >> 9) & 0x7c00)
                                   | ((*p >> 6) & 0x3e0)
                                   | ((*p >> 3) & 0x1f);
                        ++p;
                    }
                    )
                    break;
            case BPP16_MSB:                        // 16 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = (pixel >> 8);
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP16_LSB:                        // 16 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                    }
                    )
                    break;
            case BPP24_888:
                CYCLE(
                    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                        for (int x=0; x<w; x++) {
                            *dst++ = qRed  (*p);
                            *dst++ = qGreen(*p);
                            *dst++ = qBlue (*p++);
                        }
                    } else {
                        for (int x=0; x<w; x++) {
                            *dst++ = qBlue (*p);
                            *dst++ = qGreen(*p);
                            *dst++ = qRed  (*p++);
                        }
                    }
                    )
                    break;
            case BPP24_MSB:                        // 24 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP24_LSB:                        // 24 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                    }
                    )
                    break;
            case BPP32_8888:
                CYCLE(
                    memcpy(dst, p, w * 4);
                    )
                    break;
            case BPP32_MSB:                        // 32 bit MSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel >> 24;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel;
                    }
                    )
                    break;
            case BPP32_LSB:                        // 32 bit LSB
                CYCLE(
                    for (int x=0; x<w; x++) {
                        GET_PIXEL
                            *dst++ = pixel;
                        *dst++ = pixel >> 8;
                        *dst++ = pixel >> 16;
                        *dst++ = pixel >> 24;
                    }
                    )
                    break;
            default:
                qFatal("Logic error 2");
            }
        }
        xi->data = (char *)newbits;
    }

    if (d == 8 && !trucol) {                        // 8 bit pixmap
        int  pop[256];                                // pixel popularity

        if (image.colorCount() == 0)
            image.setColorCount(1);

        const QImage &cimage = image;
        memset(pop, 0, sizeof(int)*256);        // reset popularity array
        for (int i = 0; i < h; i++) {                        // for each scanline...
            const uchar* p = cimage.scanLine(i);
            const uchar *end = p + w;
            while (p < end)                        // compute popularity
                pop[*p++]++;
        }

        newbits = (uchar *)malloc(nbytes);        // copy image into newbits
        Q_CHECK_PTR(newbits);
        if (!newbits)                                // no memory
            return;
        uchar* p = newbits;
        memcpy(p, cimage.bits(), nbytes);        // copy image data into newbits

        /*
         * The code below picks the most important colors. It is based on the
         * diversity algorithm, implemented in XV 3.10. XV is (C) by John Bradley.
         */

        struct PIX {                                // pixel sort element
            uchar r,g,b,n;                        // color + pad
            int          use;                                // popularity
            int          index;                        // index in colormap
            int          mindist;
        };
        int ncols = 0;
        for (int i=0; i< cimage.colorCount(); i++) { // compute number of colors
            if (pop[i] > 0)
                ncols++;
        }
        for (int i = cimage.colorCount(); i < 256; i++) // ignore out-of-range pixels
            pop[i] = 0;

        // works since we make sure above to have at least
        // one color in the image
        if (ncols == 0)
            ncols = 1;

        PIX pixarr[256];                        // pixel array
        PIX pixarr_sorted[256];                        // pixel array (sorted)
        memset(pixarr, 0, ncols*sizeof(PIX));
        PIX *px                   = &pixarr[0];
        int  maxpop = 0;
        int  maxpix = 0;
        uint j = 0;
        QVector<QRgb> ctable = cimage.colorTable();
        for (int i = 0; i < 256; i++) {                // init pixel array
            if (pop[i] > 0) {
                px->r = qRed  (ctable[i]);
                px->g = qGreen(ctable[i]);
                px->b = qBlue (ctable[i]);
                px->n = 0;
                px->use = pop[i];
                if (pop[i] > maxpop) {        // select most popular entry
                    maxpop = pop[i];
                    maxpix = j;
                }
                px->index = i;
                px->mindist = 1000000;
                px++;
                j++;
            }
        }
        pixarr_sorted[0] = pixarr[maxpix];
        pixarr[maxpix].use = 0;

        for (int i = 1; i < ncols; i++) {                // sort pixels
            int minpix = -1, mindist = -1;
            px = &pixarr_sorted[i-1];
            int r = px->r;
            int g = px->g;
            int b = px->b;
            int dist;
            if ((i & 1) || i<10) {                // sort on max distance
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->mindist > mindist) {
                            mindist = px->mindist;
                            minpix = j;
                        }
                    }
                }
            } else {                                // sort on max popularity
                for (int j=0; j<ncols; j++) {
                    px = &pixarr[j];
                    if (px->use) {
                        dist = (px->r - r)*(px->r - r) +
                               (px->g - g)*(px->g - g) +
                               (px->b - b)*(px->b - b);
                        if (px->mindist > dist)
                            px->mindist = dist;
                        if (px->use > mindist) {
                            mindist = px->use;
                            minpix = j;
                        }
                    }
                }
            }
            pixarr_sorted[i] = pixarr[minpix];
            pixarr[minpix].use = 0;
        }

        QColormap cmap = QColormap::instance(xinfo.screen());
        uint pix[256];                                // pixel translation table
        px = &pixarr_sorted[0];
        for (int i = 0; i < ncols; i++) {                // allocate colors
            QColor c(px->r, px->g, px->b);
            pix[px->index] = cmap.pixel(c);
            px++;
        }

        p = newbits;
        for (int i = 0; i < nbytes; i++) {                // translate pixels
            *p = pix[*p];
            p++;
        }
    }

    if (!xi) {                                // X image not created
        xi = XCreateImage(dpy, visual, dd, ZPixmap, 0, 0, w, h, 32, 0);
        if (xi->bits_per_pixel == 16) {        // convert 8 bpp ==> 16 bpp
            ushort *p2;
            int            p2inc = xi->bytes_per_line/sizeof(ushort);
            ushort *newerbits = (ushort *)malloc(xi->bytes_per_line * h);
            Q_CHECK_PTR(newerbits);
            if (!newerbits)                                // no memory
                return;
            uchar* p = newbits;
            for (int y = 0; y < h; y++) {                // OOPS: Do right byte order!!
                p2 = newerbits + p2inc*y;
                for (int x = 0; x < w; x++)
                    *p2++ = *p++;
            }
            free(newbits);
            newbits = (uchar *)newerbits;
        } else if (xi->bits_per_pixel != 8) {
            qWarning("QPixmap::fromImage: Display not supported "
                     "(bpp=%d)", xi->bits_per_pixel);
        }
        xi->data = (char *)newbits;
    }

    hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                   RootWindow(X11->display, xinfo.screen()),
                                   w, h, dd);

    GC gc = XCreateGC(dpy, hd, 0, 0);
    XPutImage(dpy, hd, gc, xi, 0, 0, 0, 0, w, h);
    XFreeGC(dpy, gc);

    qSafeXDestroyImage(xi);
    d = dd;

#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 1
                                    ? XRenderFindStandardFormat(X11->display, PictStandardA1)
                                    : XRenderFindVisualFormat(X11->display, (Visual *)xinfo.visual());
        picture = XRenderCreatePicture(X11->display, hd, format, 0, 0);
    }
#endif

    if (alphaCheck.hasAlpha()) {
        QBitmap m = QBitmap::fromImage(image.createAlphaMask(flags));
        setMask(m);
    }
}

Qt::HANDLE QX11PixmapData::createBitmapFromImage(const QImage &image)
{
    QImage img = image.convertToFormat(QImage::Format_MonoLSB);
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (img.color(0) == c0 && img.color(1) == c1) {
        img.invertPixels();
        img.setColor(0, c1);
        img.setColor(1, c0);
    }

    char  *bits;
    uchar *tmp_bits;
    int w = img.width();
    int h = img.height();
    int bpl = (w + 7) / 8;
    int ibpl = img.bytesPerLine();
    if (bpl != ibpl) {
        tmp_bits = new uchar[bpl*h];
        bits = (char *)tmp_bits;
        uchar *p, *b;
        int y;
        b = tmp_bits;
        p = img.scanLine(0);
        for (y = 0; y < h; y++) {
            memcpy(b, p, bpl);
            b += bpl;
            p += ibpl;
        }
    } else {
        bits = (char *)img.bits();
        tmp_bits = 0;
    }
    Qt::HANDLE hd = (Qt::HANDLE)XCreateBitmapFromData(X11->display,
                                           QX11Info::appRootWindow(),
                                           bits, w, h);
    if (tmp_bits)                                // Avoid purify complaint
        delete [] tmp_bits;
    return hd;
}

void QX11PixmapData::bitmapFromImage(const QImage &image)
{
    w = image.width();
    h = image.height();
    d = 1;
    is_null = (w <= 0 || h <= 0);
    hd = createBitmapFromImage(image);
#ifndef QT_NO_XRENDER
    if (X11->use_xrender)
        picture = XRenderCreatePicture(X11->display, hd,
                                       XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
#endif // QT_NO_XRENDER
}

void QX11PixmapData::fill(const QColor &fillColor)
{
    if (fillColor.alpha() != 255) {
#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            if (!picture || d != 32)
                convertToARGB32(/*preserveContents = */false);

            ::Picture src  = X11->getSolidFill(xinfo.screen(), fillColor);
            XRenderComposite(X11->display, PictOpSrc, src, 0, picture,
                             0, 0, width(), height(),
                             0, 0, width(), height());
        } else
#endif
        {
            QImage im(width(), height(), QImage::Format_ARGB32_Premultiplied);
            im.fill(PREMUL(fillColor.rgba()));
            release();
            fromImage(im, Qt::AutoColor | Qt::OrderedAlphaDither);
        }
        return;
    }

    GC gc = XCreateGC(X11->display, hd, 0, 0);
    if (depth() == 1) {
        XSetForeground(X11->display, gc, qGray(fillColor.rgb()) > 127 ? 0 : 1);
    } else if (X11->use_xrender && d >= 24) {
        XSetForeground(X11->display, gc, fillColor.rgba());
    } else {
        XSetForeground(X11->display, gc,
                       QColormap::instance(xinfo.screen()).pixel(fillColor));
    }
    XFillRectangle(X11->display, hd, gc, 0, 0, width(), height());
    XFreeGC(X11->display, gc);
}

QX11PixmapData::~QX11PixmapData()
{
    // Cleanup hooks have to be called before the handles are freed
    if (is_cached) {
        QImagePixmapCleanupHooks::executePixmapDataDestructionHooks(this);
        is_cached = false;
    }

    release();
}

void QX11PixmapData::release()
{
    delete pengine;
    pengine = 0;

    if (!X11) {
        // At this point, the X server will already have freed our resources,
        // so there is nothing to do.
        return;
    }

    if (x11_mask) {
#ifndef QT_NO_XRENDER
        if (mask_picture)
            XRenderFreePicture(X11->display, mask_picture);
        mask_picture = 0;
#endif
        XFreePixmap(X11->display, x11_mask);
        x11_mask = 0;
    }

    if (hd) {
#ifndef QT_NO_XRENDER
        if (picture) {
            XRenderFreePicture(X11->display, picture);
            picture = 0;
        }
#endif // QT_NO_XRENDER

        if (hd2) {
            XFreePixmap(xinfo.display(), hd2);
            hd2 = 0;
        }
        if (!(flags & Readonly))
            XFreePixmap(xinfo.display(), hd);
        hd = 0;
    }
}

QPixmap QX11PixmapData::alphaChannel() const
{
    if (!hasAlphaChannel()) {
        QPixmap pm(w, h);
        pm.fill(Qt::white);
        return pm;
    }
    QImage im(toImage());
    return QPixmap::fromImage(im.alphaChannel(), Qt::OrderedDither);
}

void QX11PixmapData::setAlphaChannel(const QPixmap &alpha)
{
    QImage image(toImage());
    image.setAlphaChannel(alpha.toImage());
    release();
    fromImage(image, Qt::OrderedDither | Qt::OrderedAlphaDither);
}


QBitmap QX11PixmapData::mask() const
{
    QBitmap mask;
#ifndef QT_NO_XRENDER
    if (picture && d == 32) {
        // #### slow - there must be a better way..
        mask = QBitmap::fromImage(toImage().createAlphaMask());
    } else
#endif
    if (d == 1) {
        QX11PixmapData *that = const_cast<QX11PixmapData*>(this);
        mask = QPixmap(that);
    } else {
        mask = mask_to_bitmap(xinfo.screen());
    }
    return mask;
}

/*!
    Sets a mask bitmap.

    The \a newmask bitmap defines the clip mask for this pixmap. Every
    pixel in \a newmask corresponds to a pixel in this pixmap. Pixel
    value 1 means opaque and pixel value 0 means transparent. The mask
    must have the same size as this pixmap.

    \warning Setting the mask on a pixmap will cause any alpha channel
    data to be cleared. For example:
    \snippet doc/src/snippets/image/image.cpp 2
    Now, alpha and alphacopy are visually different.

    Setting a null mask resets the mask.

    The effect of this function is undefined when the pixmap is being
    painted on.

    \sa mask(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}, QBitmap
*/
void QX11PixmapData::setMask(const QBitmap &newmask)
{
    if (newmask.isNull()) { // clear mask
#ifndef QT_NO_XRENDER
        if (picture && d == 32) {
            QX11PixmapData newData(pixelType());
            newData.resize(w, h);
            newData.fill(Qt::black);
            XRenderComposite(X11->display, PictOpOver,
                             picture, 0, newData.picture,
                             0, 0, 0, 0, 0, 0, w, h);
            release();
            *this = newData;
            // the new QX11PixmapData object isn't referenced yet, so
            // ref it
            ref.ref();

            // the below is to make sure the QX11PixmapData destructor
            // doesn't delete our newly created render picture
            newData.hd = 0;
            newData.x11_mask = 0;
            newData.picture = 0;
            newData.mask_picture = 0;
            newData.hd2 = 0;
        } else
#endif
            if (x11_mask) {
#ifndef QT_NO_XRENDER
                if (picture) {
                    XRenderPictureAttributes attrs;
                    attrs.alpha_map = 0;
                    XRenderChangePicture(X11->display, picture, CPAlphaMap,
                                         &attrs);
                }
                if (mask_picture)
                    XRenderFreePicture(X11->display, mask_picture);
                mask_picture = 0;
#endif
                XFreePixmap(X11->display, x11_mask);
                x11_mask = 0;
            }
        return;
    }

#ifndef QT_NO_XRENDER
    if (picture && d == 32) {
        XRenderComposite(X11->display, PictOpSrc,
                         picture, newmask.x11PictureHandle(),
                         picture, 0, 0, 0, 0, 0, 0, w, h);
    } else
#endif
        if (depth() == 1) {
            XGCValues vals;
            vals.function = GXand;
            GC gc = XCreateGC(X11->display, hd, GCFunction, &vals);
            XCopyArea(X11->display, newmask.handle(), hd, gc, 0, 0,
                      width(), height(), 0, 0);
            XFreeGC(X11->display, gc);
        } else {
            // ##### should or the masks together
            if (x11_mask) {
                XFreePixmap(X11->display, x11_mask);
#ifndef QT_NO_XRENDER
                if (mask_picture)
                    XRenderFreePicture(X11->display, mask_picture);
#endif
            }
            x11_mask = QX11PixmapData::bitmap_to_mask(newmask, xinfo.screen());
#ifndef QT_NO_XRENDER
            if (picture) {
                mask_picture = XRenderCreatePicture(X11->display, x11_mask,
                                                    XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
                XRenderPictureAttributes attrs;
                attrs.alpha_map = mask_picture;
                XRenderChangePicture(X11->display, picture, CPAlphaMap, &attrs);
            }
#endif
        }
}

int QX11PixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmNumColors:
        return 1 << d;
    case QPaintDevice::PdmDepth:
        return d;
    case QPaintDevice::PdmWidthMM: {
        const int screen = xinfo.screen();
        const int mm = DisplayWidthMM(X11->display, screen) * w
                       / DisplayWidth(X11->display, screen);
        return mm;
    }
    case QPaintDevice::PdmHeightMM: {
        const int screen = xinfo.screen();
        const int mm = (DisplayHeightMM(X11->display, screen) * h)
                       / DisplayHeight(X11->display, screen);
        return mm;
    }
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
        return QX11Info::appDpiX(xinfo.screen());
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
        return QX11Info::appDpiY(xinfo.screen());
    default:
        qWarning("QX11PixmapData::metric(): Invalid metric");
        return 0;
    }
}

struct QXImageWrapper
{
    XImage *xi;
};

bool QX11PixmapData::canTakeQImageFromXImage(const QXImageWrapper &xiWrapper) const
{
    XImage *xi = xiWrapper.xi;

    // ARGB32_Premultiplied
    if (picture && depth() == 32)
        return true;

    Visual *visual = (Visual *)xinfo.visual();

    // RGB32
    if (depth() == 24 && xi->bits_per_pixel == 32 && visual->red_mask == 0xff0000
        && visual->green_mask == 0xff00 && visual->blue_mask == 0xff)
        return true;

    // RGB16
    if (depth() == 16 && xi->bits_per_pixel == 16 && visual->red_mask == 0xf800
        && visual->green_mask == 0x7e0 && visual->blue_mask == 0x1f)
        return true;

    return false;
}

QImage QX11PixmapData::takeQImageFromXImage(const QXImageWrapper &xiWrapper) const
{
    XImage *xi = xiWrapper.xi;

    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    if (depth() == 24)
        format = QImage::Format_RGB32;
    else if (depth() == 16)
        format = QImage::Format_RGB16;

    QImage image((uchar *)xi->data, xi->width, xi->height, xi->bytes_per_line, format);
    // take ownership
    image.data_ptr()->own_data = true;
    xi->data = 0;

    // we may have to swap the byte order
    if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && xi->byte_order == MSBFirst)
        || (QSysInfo::ByteOrder == QSysInfo::BigEndian && xi->byte_order == LSBFirst))
    {
        for (int i=0; i < image.height(); i++) {
            if (depth() == 16) {
                ushort *p = (ushort*)image.scanLine(i);
                ushort *end = p + image.width();
                while (p < end) {
                    *p = ((*p << 8) & 0xff00) | ((*p >> 8) & 0x00ff);
                    p++;
                }
            } else {
                uint *p = (uint*)image.scanLine(i);
                uint *end = p + image.width();
                while (p < end) {
                    *p = ((*p << 24) & 0xff000000) | ((*p << 8) & 0x00ff0000)
                         | ((*p >> 8) & 0x0000ff00) | ((*p >> 24) & 0x000000ff);
                    p++;
                }
            }
        }
    }

    // fix-up alpha channel
    if (format == QImage::Format_RGB32) {
        QRgb *p = (QRgb *)image.bits();
        for (int y = 0; y < xi->height; ++y) {
            for (int x = 0; x < xi->width; ++x)
                p[x] |= 0xff000000;
            p += xi->bytes_per_line / 4;
        }
    }

    XDestroyImage(xi);
    return image;
}

QImage QX11PixmapData::toImage(const QRect &rect) const
{
    QXImageWrapper xiWrapper;
    xiWrapper.xi = XGetImage(X11->display, hd, rect.x(), rect.y(), rect.width(), rect.height(),
                             AllPlanes, (depth() == 1) ? XYPixmap : ZPixmap);

    Q_CHECK_PTR(xiWrapper.xi);
    if (!xiWrapper.xi)
        return QImage();

    if (!x11_mask && canTakeQImageFromXImage(xiWrapper))
        return takeQImageFromXImage(xiWrapper);

    QImage image = toImage(xiWrapper, rect);
    qSafeXDestroyImage(xiWrapper.xi);
    return image;
}

/*!
    Converts the pixmap to a QImage. Returns a null image if the
    conversion fails.

    If the pixmap has 1-bit depth, the returned image will also be 1
    bit deep. If the pixmap has 2- to 8-bit depth, the returned image
    has 8-bit depth. If the pixmap has greater than 8-bit depth, the
    returned image has 32-bit depth.

    Note that for the moment, alpha masks on monochrome images are
    ignored.

    \sa fromImage(), {QImage#Image Formats}{Image Formats}
*/

QImage QX11PixmapData::toImage() const
{
    return toImage(QRect(0, 0, w, h));
}

QImage QX11PixmapData::toImage(const QXImageWrapper &xiWrapper, const QRect &rect) const
{
    XImage *xi = xiWrapper.xi;

    int d = depth();
    Visual *visual = (Visual *)xinfo.visual();
    bool trucol = (visual->c_class >= TrueColor) && d > 1;

    QImage::Format format = QImage::Format_Mono;
    if (d > 1 && d <= 8) {
        d = 8;
        format = QImage::Format_Indexed8;
    }
    // we could run into the situation where d == 8 AND trucol is true, which can
    // cause problems when converting to and from images.  in this case, always treat
    // the depth as 32...
    if (d > 8 || trucol) {
        d = 32;
        format = QImage::Format_RGB32;
    }

    if (d == 1 && xi->bitmap_bit_order == LSBFirst)
        format = QImage::Format_MonoLSB;
    if (x11_mask && format == QImage::Format_RGB32)
        format = QImage::Format_ARGB32;

    QImage image(xi->width, xi->height, format);
    if (image.isNull())                        // could not create image
        return image;

    QImage alpha;
    if (x11_mask) {
        if (rect.contains(QRect(0, 0, w, h)))
            alpha = mask().toImage();
        else
            alpha = mask().toImage().copy(rect);
    }
    bool ale = alpha.format() == QImage::Format_MonoLSB;

    if (trucol) {                                // truecolor
        const uint red_mask = (uint)visual->red_mask;
        const uint green_mask = (uint)visual->green_mask;
        const uint blue_mask = (uint)visual->blue_mask;
        const int  red_shift = highest_bit(red_mask) - 7;
        const int  green_shift = highest_bit(green_mask) - 7;
        const int  blue_shift = highest_bit(blue_mask) - 7;

        const uint red_bits = n_bits(red_mask);
        const uint green_bits = n_bits(green_mask);
        const uint blue_bits = n_bits(blue_mask);

        static uint red_table_bits = 0;
        static uint green_table_bits = 0;
        static uint blue_table_bits = 0;

        if (red_bits < 8 && red_table_bits != red_bits) {
            build_scale_table(&red_scale_table, red_bits);
            red_table_bits = red_bits;
        }
        if (blue_bits < 8 && blue_table_bits != blue_bits) {
            build_scale_table(&blue_scale_table, blue_bits);
            blue_table_bits = blue_bits;
        }
        if (green_bits < 8 && green_table_bits != green_bits) {
            build_scale_table(&green_scale_table, green_bits);
            green_table_bits = green_bits;
        }

        int  r, g, b;

        QRgb *dst;
        uchar *src;
        uint pixel;
        int bppc = xi->bits_per_pixel;

        if (bppc > 8 && xi->byte_order == LSBFirst)
            bppc++;

        for (int y = 0; y < xi->height; ++y) {
            uchar* asrc = x11_mask ? alpha.scanLine(y) : 0;
            dst = (QRgb *)image.scanLine(y);
            src = (uchar *)xi->data + xi->bytes_per_line*y;
            for (int x = 0; x < xi->width; x++) {
                switch (bppc) {
                case 8:
                    pixel = *src++;
                    break;
                case 16:                        // 16 bit MSB
                    pixel = src[1] | (uint)src[0] << 8;
                    src += 2;
                    break;
                case 17:                        // 16 bit LSB
                    pixel = src[0] | (uint)src[1] << 8;
                    src += 2;
                    break;
                case 24:                        // 24 bit MSB
                    pixel = src[2] | (uint)src[1] << 8 | (uint)src[0] << 16;
                    src += 3;
                    break;
                case 25:                        // 24 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16;
                    src += 3;
                    break;
                case 32:                        // 32 bit MSB
                    pixel = src[3] | (uint)src[2] << 8 | (uint)src[1] << 16 | (uint)src[0] << 24;
                    src += 4;
                    break;
                case 33:                        // 32 bit LSB
                    pixel = src[0] | (uint)src[1] << 8 | (uint)src[2] << 16 | (uint)src[3] << 24;
                    src += 4;
                    break;
                default:                        // should not really happen
                    x = xi->width;                        // leave loop
                    y = xi->height;
                    pixel = 0;                // eliminate compiler warning
                    qWarning("QPixmap::convertToImage: Invalid depth %d", bppc);
                }
                if (red_shift > 0)
                    r = (pixel & red_mask) >> red_shift;
                else
                    r = (pixel & red_mask) << -red_shift;
                if (green_shift > 0)
                    g = (pixel & green_mask) >> green_shift;
                else
                    g = (pixel & green_mask) << -green_shift;
                if (blue_shift > 0)
                    b = (pixel & blue_mask) >> blue_shift;
                else
                    b = (pixel & blue_mask) << -blue_shift;

                if (red_bits < 8)
                    r = red_scale_table[r];
                if (green_bits < 8)
                    g = green_scale_table[g];
                if (blue_bits < 8)
                    b = blue_scale_table[b];

                if (x11_mask) {
                    if (ale) {
                        *dst++ = (asrc[x >> 3] & (1 << (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    } else {
                        *dst++ = (asrc[x >> 3] & (0x80 >> (x & 7))) ? qRgba(r, g, b, 0xff) : 0;
                    }
                } else {
                    *dst++ = qRgb(r, g, b);
                }
            }
        }
    } else if (xi->bits_per_pixel == d) {        // compatible depth
        char *xidata = xi->data;                // copy each scanline
        int bpl = qMin(image.bytesPerLine(),xi->bytes_per_line);
        for (int y=0; y<xi->height; y++) {
            memcpy(image.scanLine(y), xidata, bpl);
            xidata += xi->bytes_per_line;
        }
    } else {
        /* Typically 2 or 4 bits display depth */
        qWarning("QPixmap::convertToImage: Display not supported (bpp=%d)",
                 xi->bits_per_pixel);
        return QImage();
    }

    if (d == 1) {                                // bitmap
        image.setColorCount(2);
        image.setColor(0, qRgb(255,255,255));
        image.setColor(1, qRgb(0,0,0));
    } else if (!trucol) {                        // pixmap with colormap
        register uchar *p;
        uchar *end;
        uchar  use[256];                        // pixel-in-use table
        uchar  pix[256];                        // pixel translation table
        int    ncols, bpl;
        memset(use, 0, 256);
        memset(pix, 0, 256);
        bpl = image.bytesPerLine();

        if (x11_mask) {                         // which pixels are used?
            for (int i = 0; i < xi->height; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < xi->width; x++) {
                        if (asrc[x >> 3] & (1 << (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < xi->width; x++) {
                        if (asrc[x >> 3] & (0x80 >> (x & 7)))
                            use[*p] = 1;
                        ++p;
                    }
                }
            }
        } else {
            for (int i = 0; i < xi->height; i++) {
                p = image.scanLine(i);
                end = p + bpl;
                while (p < end)
                    use[*p++] = 1;
            }
        }
        ncols = 0;
        for (int i = 0; i < 256; i++) {                // build translation table
            if (use[i])
                pix[i] = ncols++;
        }
        for (int i = 0; i < xi->height; i++) {                        // translate pixels
            p = image.scanLine(i);
            end = p + bpl;
            while (p < end) {
                *p = pix[*p];
                p++;
            }
        }
        if (x11_mask) {
            int trans;
            if (ncols < 256) {
                trans = ncols++;
                image.setColorCount(ncols);        // create color table
                image.setColor(trans, 0x00000000);
            } else {
                image.setColorCount(ncols);        // create color table
                // oh dear... no spare "transparent" pixel.
                // use first pixel in image (as good as any).
                trans = image.scanLine(0)[0];
            }
            for (int i = 0; i < xi->height; i++) {
                uchar* asrc = alpha.scanLine(i);
                p = image.scanLine(i);
                if (ale) {
                    for (int x = 0; x < xi->width; x++) {
                        if (!(asrc[x >> 3] & (1 << (x & 7))))
                            *p = trans;
                        ++p;
                    }
                } else {
                    for (int x = 0; x < xi->width; x++) {
                        if (!(asrc[x >> 3] & (1 << (7 -(x & 7)))))
                            *p = trans;
                        ++p;
                    }
                }
            }
        } else {
            image.setColorCount(ncols);        // create color table
        }
        QVector<QColor> colors = QColormap::instance(xinfo.screen()).colormap();
        int j = 0;
        for (int i=0; i<colors.size(); i++) {                // translate pixels
            if (use[i])
                image.setColor(j++, 0xff000000 | colors.at(i).rgb());
        }
    }

    return image;
}

/*!
    Returns a copy of the pixmap that is transformed using the given
    transformation \a matrix and transformation \a mode. The original
    pixmap is not changed.

    The transformation \a matrix is internally adjusted to compensate
    for unwanted translation; i.e. the pixmap produced is the smallest
    pixmap that contains all the transformed points of the original
    pixmap. Use the trueMatrix() function to retrieve the actual
    matrix used for transforming the pixmap.

    This function is slow because it involves transformation to a
    QImage, non-trivial computations and a transformation back to a
    QPixmap.

    \sa trueMatrix(), {QPixmap#Pixmap Transformations}{Pixmap
    Transformations}
*/
QPixmap QX11PixmapData::transformed(const QTransform &transform,
                                    Qt::TransformationMode mode ) const
{
    if (mode == Qt::SmoothTransformation || transform.type() >= QTransform::TxProject) {
        QImage image = toImage();
        return QPixmap::fromImage(image.transformed(transform, mode));
    }

    uint   w = 0;
    uint   h = 0;                               // size of target pixmap
    uint   ws, hs;                              // size of source pixmap
    uchar *dptr;                                // data in target pixmap
    uint   dbpl, dbytes;                        // bytes per line/bytes total
    uchar *sptr;                                // data in original pixmap
    int    sbpl;                                // bytes per line in original
    int    bpp;                                 // bits per pixel
    bool   depth1 = depth() == 1;
    Display *dpy = X11->display;

    ws = width();
    hs = height();

    QTransform mat(transform.m11(), transform.m12(), transform.m13(),
                   transform.m21(), transform.m22(), transform.m23(),
                   0., 0., 1);
    bool complex_xform = false;
    qreal scaledWidth;
    qreal scaledHeight;

    if (mat.type() <= QTransform::TxScale) {
        scaledHeight = qAbs(mat.m22()) * hs + 0.9999;
        scaledWidth = qAbs(mat.m11()) * ws + 0.9999;
        h = qAbs(int(scaledHeight));
        w = qAbs(int(scaledWidth));
    } else {                                        // rotation or shearing
        QPolygonF a(QRectF(0, 0, ws, hs));
        a = mat.map(a);
        QRect r = a.boundingRect().toAlignedRect();
        w = r.width();
        h = r.height();
        scaledWidth = w;
        scaledHeight = h;
        complex_xform = true;
    }
    mat = QPixmap::trueMatrix(mat, ws, hs); // true matrix

    bool invertible;
    mat = mat.inverted(&invertible);  // invert matrix

    if (h == 0 || w == 0 || !invertible
        || qAbs(scaledWidth) >= 32768 || qAbs(scaledHeight) >= 32768 )
	// error, return null pixmap
        return QPixmap();

#if defined(QT_MITSHM)
    static bool try_once = true;
    if (try_once) {
        try_once = false;
        if (!xshminit)
            qt_create_mitshm_buffer(this, 800, 600);
    }

    bool use_mitshm = xshmimg && !depth1 &&
                      xshmimg->width >= w && xshmimg->height >= h;
#endif
    XImage *xi = XGetImage(X11->display, handle(), 0, 0, ws, hs, AllPlanes,
                           depth1 ? XYPixmap : ZPixmap);

    if (!xi)
        return QPixmap();

    sbpl = xi->bytes_per_line;
    sptr = (uchar *)xi->data;
    bpp         = xi->bits_per_pixel;

    if (depth1)
        dbpl = (w+7)/8;
    else
        dbpl = ((w*bpp+31)/32)*4;
    dbytes = dbpl*h;

#if defined(QT_MITSHM)
    if (use_mitshm) {
        dptr = (uchar *)xshmimg->data;
        uchar fillbyte = bpp == 8 ? white.pixel() : 0xff;
        for (int y=0; y<h; y++)
            memset(dptr + y*xshmimg->bytes_per_line, fillbyte, dbpl);
    } else {
#endif
        dptr = (uchar *)malloc(dbytes);        // create buffer for bits
        Q_CHECK_PTR(dptr);
        if (depth1)                                // fill with zeros
            memset(dptr, 0, dbytes);
        else if (bpp == 8)                        // fill with background color
            memset(dptr, WhitePixel(X11->display, xinfo.screen()), dbytes);
        else
            memset(dptr, 0, dbytes);
#if defined(QT_MITSHM)
    }
#endif

    // #define QT_DEBUG_XIMAGE
#if defined(QT_DEBUG_XIMAGE)
    qDebug("----IMAGE--INFO--------------");
    qDebug("width............. %d", xi->width);
    qDebug("height............ %d", xi->height);
    qDebug("xoffset........... %d", xi->xoffset);
    qDebug("format............ %d", xi->format);
    qDebug("byte order........ %d", xi->byte_order);
    qDebug("bitmap unit....... %d", xi->bitmap_unit);
    qDebug("bitmap bit order.. %d", xi->bitmap_bit_order);
    qDebug("depth............. %d", xi->depth);
    qDebug("bytes per line.... %d", xi->bytes_per_line);
    qDebug("bits per pixel.... %d", xi->bits_per_pixel);
#endif

    int type;
    if (xi->bitmap_bit_order == MSBFirst)
        type = QT_XFORM_TYPE_MSBFIRST;
    else
        type = QT_XFORM_TYPE_LSBFIRST;
    int        xbpl, p_inc;
    if (depth1) {
        xbpl  = (w+7)/8;
        p_inc = dbpl - xbpl;
    } else {
        xbpl  = (w*bpp)/8;
        p_inc = dbpl - xbpl;
#if defined(QT_MITSHM)
        if (use_mitshm)
            p_inc = xshmimg->bytes_per_line - xbpl;
#endif
    }

    if (!qt_xForm_helper(mat, xi->xoffset, type, bpp, dptr, xbpl, p_inc, h, sptr, sbpl, ws, hs)){
        qWarning("QPixmap::transform: display not supported (bpp=%d)",bpp);
        QPixmap pm;
        return pm;
    }

    qSafeXDestroyImage(xi);

    if (depth1) {                                // mono bitmap
        QBitmap bm = QBitmap::fromData(QSize(w, h), dptr,
                                       BitmapBitOrder(X11->display) == MSBFirst
                                       ? QImage::Format_Mono
                                       : QImage::Format_MonoLSB);
        free(dptr);
        return bm;
    } else {                                        // color pixmap
        QX11PixmapData *x11Data = new QX11PixmapData(QPixmapData::PixmapType);
        QPixmap pm(x11Data);
        x11Data->flags &= ~QX11PixmapData::Uninitialized;
        x11Data->xinfo = xinfo;
        x11Data->d = d;
        x11Data->w = w;
        x11Data->h = h;
        x11Data->is_null = (w <= 0 || h <= 0);
        x11Data->hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                                RootWindow(X11->display, xinfo.screen()),
                                                w, h, d);
        x11Data->setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            XRenderPictFormat *format = x11Data->d == 32
                                        ? XRenderFindStandardFormat(X11->display, PictStandardARGB32)
                                        : XRenderFindVisualFormat(X11->display, (Visual *) x11Data->xinfo.visual());
            x11Data->picture = XRenderCreatePicture(X11->display, x11Data->hd, format, 0, 0);
        }
#endif // QT_NO_XRENDER

        GC gc = XCreateGC(X11->display, x11Data->hd, 0, 0);
#if defined(QT_MITSHM)
        if (use_mitshm) {
            XCopyArea(dpy, xshmpm, x11Data->hd, gc, 0, 0, w, h, 0, 0);
        } else
#endif
        {
            xi = XCreateImage(dpy, (Visual*)x11Data->xinfo.visual(),
                              x11Data->d,
                              ZPixmap, 0, (char *)dptr, w, h, 32, 0);
            XPutImage(dpy, pm.handle(), gc, xi, 0, 0, 0, 0, w, h);
            qSafeXDestroyImage(xi);
        }
        XFreeGC(X11->display, gc);

        if (x11_mask) { // xform mask, too
            pm.setMask(mask_to_bitmap(xinfo.screen()).transformed(transform));
        } else if (d != 32 && complex_xform) { // need a mask!
            QBitmap mask(ws, hs);
            mask.fill(Qt::color1);
            pm.setMask(mask.transformed(transform));
        }
        return pm;
    }
}

int QPixmap::x11SetDefaultScreen(int screen)
{
    int old = defaultScreen;
    defaultScreen = screen;
    return old;
}

void QPixmap::x11SetScreen(int screen)
{
    if (paintingActive()) {
        qWarning("QPixmap::x11SetScreen(): Cannot change screens during painting");
        return;
    }

    if (isNull())
        return;

    if (data->classId() != QPixmapData::X11Class)
        return;

    if (screen < 0)
        screen = QX11Info::appScreen();

    QX11PixmapData *x11Data = static_cast<QX11PixmapData*>(data.data());
    if (screen == x11Data->xinfo.screen())
        return; // nothing to do

    if (isNull()) {
        QX11InfoData* xd = x11Data->xinfo.getX11Data(true);
        xd->screen = screen;
        xd->depth = QX11Info::appDepth(screen);
        xd->cells = QX11Info::appCells(screen);
        xd->colormap = QX11Info::appColormap(screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(screen);
        xd->visual = (Visual *)QX11Info::appVisual(screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(screen);
        x11Data->xinfo.setX11Data(xd);
        return;
    }
#if 0
    qDebug("QPixmap::x11SetScreen for %p from %d to %d. Size is %d/%d", x11Data, x11Data->xinfo.screen(), screen, width(), height());
#endif

    x11SetDefaultScreen(screen);
    *this = qt_toX11Pixmap(toImage());
}

QPixmap QPixmap::grabWindow(WId window, int x, int y, int w, int h)
{
    if (w == 0 || h == 0)
        return QPixmap();

    Display *dpy = X11->display;
    XWindowAttributes window_attr;
    if (!XGetWindowAttributes(dpy, window, &window_attr))
        return QPixmap();

    if (w < 0)
        w = window_attr.width - x;
    if (h < 0)
        h = window_attr.height - y;

    // determine the screen
    int scr;
    for (scr = 0; scr < ScreenCount(dpy); ++scr) {
        if (window_attr.root == RootWindow(dpy, scr))        // found it
            break;
    }
    if (scr >= ScreenCount(dpy))                // sanity check
        return QPixmap();


    // get the depth of the root window
    XWindowAttributes root_attr;
    if (!XGetWindowAttributes(dpy, window_attr.root, &root_attr))
        return QPixmap();

    if (window_attr.depth == root_attr.depth) {
        // if the depth of the specified window and the root window are the
        // same, grab pixels from the root window (so that we get the any
        // overlapping windows and window manager frames)

        // map x and y to the root window
        WId unused;
        if (!XTranslateCoordinates(dpy, window, window_attr.root, x, y,
                                   &x, &y, &unused))
            return QPixmap();

        window = window_attr.root;
        window_attr = root_attr;
    }

    QX11PixmapData *data = new QX11PixmapData(QPixmapData::PixmapType);

    void qt_x11_getX11InfoForWindow(QX11Info * xinfo, const XWindowAttributes &a);
    qt_x11_getX11InfoForWindow(&data->xinfo,window_attr);

    data->resize(w, h);

    QPixmap pm(data);

    data->flags &= ~QX11PixmapData::Uninitialized;
    pm.x11SetScreen(scr);

    GC gc = XCreateGC(dpy, pm.handle(), 0, 0);
    XSetSubwindowMode(dpy, gc, IncludeInferiors);
    XCopyArea(dpy, window, pm.handle(), gc, x, y, w, h, 0, 0);
    XFreeGC(dpy, gc);

    return pm;
}

bool QX11PixmapData::hasAlphaChannel() const
{
    return d == 32;
}

const QX11Info &QPixmap::x11Info() const
{
    if (data && data->classId() == QPixmapData::X11Class)
        return static_cast<QX11PixmapData*>(data.data())->xinfo;
    else {
        static QX11Info nullX11Info;
        return nullX11Info;
    }
}

#if !defined(QT_NO_XRENDER)
static XRenderPictFormat *qt_renderformat_for_depth(const QX11Info &xinfo, int depth)
{
    if (depth == 1)
        return XRenderFindStandardFormat(X11->display, PictStandardA1);
    else if (depth == 32)
        return XRenderFindStandardFormat(X11->display, PictStandardARGB32);
    else
        return XRenderFindVisualFormat(X11->display, (Visual *)xinfo.visual());
}
#endif

QPaintEngine* QX11PixmapData::paintEngine() const
{
    QX11PixmapData *that = const_cast<QX11PixmapData*>(this);

    if ((flags & Readonly) && share_mode == QPixmap::ImplicitlyShared) {
        // if someone wants to draw onto us, copy the shared contents
        // and turn it into a fully fledged QPixmap
        ::Pixmap hd_copy = XCreatePixmap(X11->display, RootWindow(X11->display, xinfo.screen()),
                                         w, h, d);
#if !defined(QT_NO_XRENDER)
        XRenderPictFormat *format = qt_renderformat_for_depth(xinfo, d);
        ::Picture picture_copy = XRenderCreatePicture(X11->display, hd_copy, format, 0, 0);

        if (picture && d == 32) {
            XRenderComposite(X11->display, PictOpSrc, picture, 0, picture_copy,
                             0, 0, 0, 0, 0, 0, w, h);
            XRenderFreePicture(X11->display, picture);
            that->picture = picture_copy;
        } else
#endif
        {
            GC gc = XCreateGC(X11->display, hd_copy, 0, 0);
            XCopyArea(X11->display, hd, hd_copy, gc, 0, 0, w, h, 0, 0);
            XFreeGC(X11->display, gc);
        }
        that->hd = hd_copy;
        that->flags &= ~QX11PixmapData::Readonly;
    }

    if (!that->pengine)
        that->pengine = new QX11PaintEngine;
    return that->pengine;
}

Qt::HANDLE QPixmap::x11PictureHandle() const
{
#ifndef QT_NO_XRENDER
    if (data && data->classId() == QPixmapData::X11Class)
        return static_cast<const QX11PixmapData*>(data.data())->picture;
    else
        return 0;
#else
    return 0;
#endif // QT_NO_XRENDER
}

Qt::HANDLE QX11PixmapData::x11ConvertToDefaultDepth()
{
#ifndef QT_NO_XRENDER
    if (d == QX11Info::appDepth() || !X11->use_xrender)
        return hd;
    if (!hd2) {
        hd2 = XCreatePixmap(xinfo.display(), hd, w, h, QX11Info::appDepth());
        XRenderPictFormat *format = XRenderFindVisualFormat(xinfo.display(),
                                                            (Visual*) xinfo.visual());
        Picture pic = XRenderCreatePicture(xinfo.display(), hd2, format, 0, 0);
        XRenderComposite(xinfo.display(), PictOpSrc, picture,
                         XNone, pic, 0, 0, 0, 0, 0, 0, w, h);
        XRenderFreePicture(xinfo.display(), pic);
    }
    return hd2;
#else
    return hd;
#endif
}

void QX11PixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    if (data->pixelType() == BitmapType) {
        fromImage(data->toImage().copy(rect), Qt::AutoColor);
        return;
    }

    const QX11PixmapData *x11Data = static_cast<const QX11PixmapData*>(data);

    setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));

    flags &= ~Uninitialized;
    xinfo = x11Data->xinfo;
    d = x11Data->d;
    w = rect.width();
    h = rect.height();
    is_null = (w <= 0 || h <= 0);
    hd = (Qt::HANDLE)XCreatePixmap(X11->display,
                                   RootWindow(X11->display, x11Data->xinfo.screen()),
                                   w, h, d);
#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = d == 32
                                    ? XRenderFindStandardFormat(X11->display, PictStandardARGB32)
                                    : XRenderFindVisualFormat(X11->display, (Visual *)xinfo.visual());
        picture = XRenderCreatePicture(X11->display, hd, format, 0, 0);
    }
#endif // QT_NO_XRENDER
    if (x11Data->x11_mask) {
        x11_mask = XCreatePixmap(X11->display, hd, w, h, 1);
#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            mask_picture = XRenderCreatePicture(X11->display, x11_mask,
                                                XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
            XRenderPictureAttributes attrs;
            attrs.alpha_map = x11Data->mask_picture;
            XRenderChangePicture(X11->display, x11Data->picture, CPAlphaMap, &attrs);
        }
#endif
    }

#if !defined(QT_NO_XRENDER)
    if (x11Data->picture && x11Data->d == 32) {
        XRenderComposite(X11->display, PictOpSrc,
                         x11Data->picture, 0, picture,
                         rect.x(), rect.y(), 0, 0, 0, 0, w, h);
    } else
#endif
    {
        GC gc = XCreateGC(X11->display, hd, 0, 0);
        XCopyArea(X11->display, x11Data->hd, hd, gc,
                  rect.x(), rect.y(), w, h, 0, 0);
        if (x11Data->x11_mask) {
            GC monogc = XCreateGC(X11->display, x11_mask, 0, 0);
            XCopyArea(X11->display, x11Data->x11_mask, x11_mask, monogc,
                      rect.x(), rect.y(), w, h, 0, 0);
            XFreeGC(X11->display, monogc);
        }
        XFreeGC(X11->display, gc);
    }
}

bool QX11PixmapData::scroll(int dx, int dy, const QRect &rect)
{
    GC gc = XCreateGC(X11->display, hd, 0, 0);
    XCopyArea(X11->display, hd, hd, gc,
              rect.left(), rect.top(), rect.width(), rect.height(),
              rect.left() + dx, rect.top() + dy);
    XFreeGC(X11->display, gc);
    return true;
}

#if !defined(QT_NO_XRENDER)
void QX11PixmapData::convertToARGB32(bool preserveContents)
{
    if (!X11->use_xrender)
        return;

    // Q_ASSERT(count == 1);
    if ((flags & Readonly) && share_mode == QPixmap::ExplicitlyShared)
        return;

    Pixmap pm = XCreatePixmap(X11->display, RootWindow(X11->display, xinfo.screen()),
                              w, h, 32);
    Picture p = XRenderCreatePicture(X11->display, pm,
                                     XRenderFindStandardFormat(X11->display, PictStandardARGB32), 0, 0);
    if (picture) {
        if (preserveContents)
            XRenderComposite(X11->display, PictOpSrc, picture, 0, p, 0, 0, 0, 0, 0, 0, w, h);
        if (!(flags & Readonly))
            XRenderFreePicture(X11->display, picture);
    }
    if (hd && !(flags & Readonly))
        XFreePixmap(X11->display, hd);
    if (x11_mask) {
        XFreePixmap(X11->display, x11_mask);
        if (mask_picture)
            XRenderFreePicture(X11->display, mask_picture);
        x11_mask = 0;
        mask_picture = 0;
    }
    hd = pm;
    picture = p;
    d = 32;
}
#endif

QPixmap QPixmap::fromX11Pixmap(Qt::HANDLE pixmap, QPixmap::ShareMode mode)
{
    Window root;
    int x;
    int y;
    uint width;
    uint height;
    uint border_width;
    uint depth;
    XWindowAttributes win_attribs;
    int num_screens = ScreenCount(X11->display);
    int screen = 0;

    XGetGeometry(X11->display, pixmap, &root, &x, &y, &width, &height, &border_width, &depth);
    XGetWindowAttributes(X11->display, root, &win_attribs);

    for (; screen < num_screens; ++screen) {
        if (win_attribs.screen == ScreenOfDisplay(X11->display, screen))
            break;
    }

    QX11PixmapData *data = new QX11PixmapData(depth == 1 ? QPixmapData::BitmapType : QPixmapData::PixmapType);
    data->setSerialNumber(qt_pixmap_serial.fetchAndAddRelaxed(1));
    data->flags = QX11PixmapData::Readonly;
    data->share_mode = mode;
    data->w = width;
    data->h = height;
    data->is_null = (width <= 0 || height <= 0);
    data->d = depth;
    data->hd = pixmap;

    if (defaultScreen >= 0 && defaultScreen != screen) {
        QX11InfoData* xd = data->xinfo.getX11Data(true);
        xd->screen = defaultScreen;
        xd->depth = QX11Info::appDepth(xd->screen);
        xd->cells = QX11Info::appCells(xd->screen);
        xd->colormap = QX11Info::appColormap(xd->screen);
        xd->defaultColormap = QX11Info::appDefaultColormap(xd->screen);
        xd->visual = (Visual *)QX11Info::appVisual(xd->screen);
        xd->defaultVisual = QX11Info::appDefaultVisual(xd->screen);
        data->xinfo.setX11Data(xd);
    }

#ifndef QT_NO_XRENDER
    if (X11->use_xrender) {
        XRenderPictFormat *format = qt_renderformat_for_depth(data->xinfo, depth);
        data->picture = XRenderCreatePicture(X11->display, data->hd, format, 0, 0);
    }
#endif // QT_NO_XRENDER

    return QPixmap(data);
}


QT_END_NAMESPACE
