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
#include <exception>
#include <w32std.h>
#include <fbs.h>

#include <private/qapplication_p.h>
#include <private/qgraphicssystem_p.h>
#include <private/qt_s60_p.h>
#include <private/qfont_p.h>
#include <private/qpaintengine_raster_symbian_p.h>

#include "qpixmap.h"
#include "qpixmap_raster_p.h"
#include <qwidget.h>
#include "qpixmap_raster_symbian_p.h"
#include "qnativeimage_p.h"
#include "qbitmap.h"
#include "qimage.h"
#include "qimage_p.h"

#include <fbs.h>

QT_BEGIN_NAMESPACE

const uchar qt_pixmap_bit_mask[] = { 0x01, 0x02, 0x04, 0x08,
                                     0x10, 0x20, 0x40, 0x80 };

static bool cleanup_function_registered = false;
static QSymbianRasterPixmapData *firstPixmap = 0;

// static
void QSymbianRasterPixmapData::qt_symbian_register_pixmap(QSymbianRasterPixmapData *pd)
{
    if (!cleanup_function_registered) {
        qAddPostRoutine(qt_symbian_release_pixmaps);
        cleanup_function_registered = true;
    }

    pd->next = firstPixmap;
    pd->prev = 0;
    if (firstPixmap)
        firstPixmap->prev = pd;
    firstPixmap = pd;
}

// static
void QSymbianRasterPixmapData::qt_symbian_unregister_pixmap(QSymbianRasterPixmapData *pd)
{
    if (pd->next)
        pd->next->prev = pd->prev;
    if (pd->prev)
        pd->prev->next = pd->next;
    else
        firstPixmap = pd->next;
}

// static
void QSymbianRasterPixmapData::qt_symbian_release_pixmaps()
{
    // Scan all QSymbianRasterPixmapData objects in the system and destroy them.
    QSymbianRasterPixmapData *pd = firstPixmap;
    while (pd != 0) {
        pd->release();
        pd = pd->next;
    }
}

/*
    \class QSymbianFbsClient
    \since 4.6
    \internal

    Symbian Font And Bitmap server client that is
    used to lock the global bitmap heap. Only used in
    S60 v3.1 and S60 v3.2.
*/
_LIT(KFBSERVLargeBitmapAccessName,"FbsLargeBitmapAccess");
class QSymbianFbsClient
{
public:

    QSymbianFbsClient() : heapLocked(false)
    {
        heapLock.OpenGlobal(KFBSERVLargeBitmapAccessName);
    }

    ~QSymbianFbsClient()
    {
        heapLock.Close();
    }

    bool lockHeap()
    {
        bool wasLocked = heapLocked;

        if (heapLock.Handle() && !heapLocked) {
            heapLock.Wait();
            heapLocked = true;
        }

        return wasLocked;
    }

    bool unlockHeap()
    {
        bool wasLocked = heapLocked;

        if (heapLock.Handle() && heapLocked) {
            heapLock.Signal();
            heapLocked = false;
        }

        return wasLocked;
    }


private:

    RMutex heapLock;
    bool heapLocked;
};

Q_GLOBAL_STATIC(QSymbianFbsClient, qt_symbianFbsClient);



// QSymbianFbsHeapLock

QSymbianFbsHeapLock::QSymbianFbsHeapLock(LockAction a)
: action(a), wasLocked(false)
{
    QSysInfo::SymbianVersion symbianVersion = QSysInfo::symbianVersion();
    if (symbianVersion == QSysInfo::SV_9_2 || symbianVersion == QSysInfo::SV_9_3)
        wasLocked = qt_symbianFbsClient()->unlockHeap();
}

QSymbianFbsHeapLock::~QSymbianFbsHeapLock()
{
    // Do nothing
}

void QSymbianFbsHeapLock::relock()
{
    QSysInfo::SymbianVersion symbianVersion = QSysInfo::symbianVersion();
    if (wasLocked && (symbianVersion == QSysInfo::SV_9_2 || symbianVersion == QSysInfo::SV_9_3))
        qt_symbianFbsClient()->lockHeap();
}

/*
    \class QSymbianBitmapDataAccess
    \since 4.6
    \internal

    Data access class that is used to locks/unlocks pixel data
    when drawing or modifying CFbsBitmap pixel data.
*/
class QSymbianBitmapDataAccess
{
public:

    static int heapRefCount;
    QSysInfo::SymbianVersion symbianVersion;

    explicit QSymbianBitmapDataAccess()
    {
        symbianVersion = QSysInfo::symbianVersion();
    };

    ~QSymbianBitmapDataAccess() {};

    inline void beginDataAccess(CFbsBitmap *bitmap)
    {
        if (symbianVersion == QSysInfo::SV_9_2) {
            if (heapRefCount == 0)
                qt_symbianFbsClient()->lockHeap();
        } else {
            bitmap->LockHeap(ETrue);
        }

        heapRefCount++;
    }

    inline void endDataAccess(CFbsBitmap *bitmap)
    {
        heapRefCount--;

        if (symbianVersion == QSysInfo::SV_9_2) {
            if (heapRefCount == 0)
                qt_symbianFbsClient()->unlockHeap();
        } else {
            bitmap->UnlockHeap(ETrue);
        }
    }
};

int QSymbianBitmapDataAccess::heapRefCount = 0;


#define UPDATE_BUFFER()     \
    {                       \
    beginDataAccess();      \
    endDataAccess();        \
}


static CFbsBitmap* createSymbianCFbsBitmap(const TSize& size, TDisplayMode mode)
{
    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);

    CFbsBitmap* bitmap = 0;
    QT_TRAP_THROWING(bitmap = new (ELeave) CFbsBitmap);

    if (bitmap->Create(size, mode) != KErrNone) {
        delete bitmap;
        bitmap = 0;
    }

    lock.relock();

    return bitmap;
}

static CFbsBitmap* uncompress(CFbsBitmap* bitmap)
{
    if(bitmap->IsCompressedInRAM()) {
        QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);

        CFbsBitmap *uncompressed = 0;
        QT_TRAP_THROWING(uncompressed = new (ELeave) CFbsBitmap);

        if (uncompressed->Create(bitmap->SizeInPixels(), bitmap->DisplayMode()) != KErrNone) {
            delete bitmap;
            bitmap = 0;
            lock.relock();

            return bitmap;
        }

        lock.relock();

        CFbsBitmapDevice* bitmapDevice = 0;
        CFbsBitGc *bitmapGc = 0;
        QT_TRAP_THROWING(bitmapDevice = CFbsBitmapDevice::NewL(uncompressed));
        QT_TRAP_THROWING(bitmapGc = CFbsBitGc::NewL());
        bitmapGc->Activate(bitmapDevice);

        bitmapGc->BitBlt(TPoint(), bitmap);

        delete bitmapGc;
        delete bitmapDevice;

        return uncompressed;
    } else {
        return bitmap;
    }
}

QPixmap QPixmap::grabWindow(WId winId, int x, int y, int w, int h)
{
    CWsScreenDevice* screenDevice = S60->screenDevice();
    TSize screenSize = screenDevice->SizeInPixels();

    TSize srcSize;
    // Find out if this is one of our windows.
    QSymbianControl *sControl;
    sControl = winId->MopGetObject(sControl);
    if (sControl && sControl->widget()->windowType() == Qt::Desktop) {
        // Grabbing desktop widget
        srcSize = screenSize;
    } else {
        TPoint relativePos = winId->PositionRelativeToScreen();
        x += relativePos.iX;
        y += relativePos.iY;
        srcSize = winId->Size();
    }

    TRect srcRect(TPoint(x, y), srcSize);
    // Clip to the screen
    srcRect.Intersection(TRect(screenSize));

    if (w > 0 && h > 0) {
        TRect subRect(TPoint(x, y), TSize(w, h));
        // Clip to the subRect
        srcRect.Intersection(subRect);
    }

    if (srcRect.IsEmpty())
        return QPixmap();

    CFbsBitmap* temporary = createSymbianCFbsBitmap(srcRect.Size(), screenDevice->DisplayMode());

    QPixmap pix;

    if (temporary && screenDevice->CopyScreenToBitmap(temporary, srcRect) == KErrNone) {
        pix = QPixmap::fromSymbianCFbsBitmap(temporary);
    }

    delete temporary;
    return pix;
}

/*!
    \fn CFbsBitmap *QPixmap::toSymbianCFbsBitmap() const
    \since 4.6

    Creates a \c CFbsBitmap that is equivalent to the QPixmap. Internally this
    function will try to duplicate the handle instead of copying the data,
    however in scenarios where this is not possible the data will be copied.
    If the creation fails or the pixmap is null, then this function returns 0.

    It is the caller's responsibility to release the \c CFbsBitmap data
    after use either by deleting the bitmap or calling \c Reset().

    \warning On S60 3.1 and S60 3.2, semi-transparent pixmaps are always copied
             and not duplicated.
    \warning This function is only available on Symbian OS.

    \sa fromSymbianCFbsBitmap()
*/
CFbsBitmap *QPixmap::toSymbianCFbsBitmap() const
{
    QPixmapData *data = pixmapData();
    if (!data || data->isNull())
        return 0;

    return reinterpret_cast<CFbsBitmap*>(data->toNativeType(QPixmapData::FbsBitmap));
}

/*!
    \fn QPixmap QPixmap::fromSymbianCFbsBitmap(CFbsBitmap *bitmap)
    \since 4.6

    Creates a QPixmap from a \c CFbsBitmap \a bitmap. Internally this function
    will try to duplicate the bitmap handle instead of copying the data, however
    in scenarios where this is not possible the data will be copied.
    To be sure that QPixmap does not modify your original instance, you should
    make a copy of your \c CFbsBitmap before calling this function.
    If the CFbsBitmap is not valid this function will return a null QPixmap.
    For performance reasons it is recommended to use a \a bitmap with a display
    mode of EColor16MAP or EColor16MU whenever possible.

    \warning This function is only available on Symbian OS.

    \sa toSymbianCFbsBitmap(), {QPixmap#Pixmap Conversion}{Pixmap Conversion}
*/
QPixmap QPixmap::fromSymbianCFbsBitmap(CFbsBitmap *bitmap)
{
    if (!bitmap)
        return QPixmap();

    QScopedPointer<QPixmapData> data(QPixmapData::create(0,0, QPixmapData::PixmapType));
    data->fromNativeType(reinterpret_cast<void*>(bitmap), QPixmapData::FbsBitmap);
    QPixmap pixmap(data.take());
    return pixmap;
}

QSymbianRasterPixmapData::QSymbianRasterPixmapData(PixelType type) : QRasterPixmapData(type),
    symbianBitmapDataAccess(new QSymbianBitmapDataAccess),
    cfbsBitmap(0),
    pengine(0),
    bytes(0),
    formatLocked(false),
    next(0),
    prev(0)
{
    qt_symbian_register_pixmap(this);
}

QSymbianRasterPixmapData::~QSymbianRasterPixmapData()
{
    release();
    delete symbianBitmapDataAccess;
    qt_symbian_unregister_pixmap(this);
}

void QSymbianRasterPixmapData::resize(int width, int height)
{
    if (width <= 0 || height <= 0) {
        w = width;
        h = height;
        is_null = true;

        release();
        return;
    } else if (!cfbsBitmap) {
        TDisplayMode mode;
        if (pixelType() == BitmapType)
            mode = EGray2;
        else
            mode = EColor16MU;

        CFbsBitmap* bitmap = createSymbianCFbsBitmap(TSize(width, height), mode);
        fromSymbianBitmap(bitmap);
    } else {

        TSize newSize(width, height);

        if(cfbsBitmap->SizeInPixels() != newSize) {
            cfbsBitmap->Resize(TSize(width, height));
            if(pengine) {
                delete pengine;
                pengine = 0;
            }
        }

        UPDATE_BUFFER();
    }
}

void QSymbianRasterPixmapData::release()
{
    if (cfbsBitmap) {
        QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
        delete cfbsBitmap;
        lock.relock();
    }

    delete pengine;
    image = QImage();
    cfbsBitmap = 0;
    pengine = 0;
    bytes = 0;
}

/*!
 * Takes ownership of bitmap. Used by window surface
 */
void QSymbianRasterPixmapData::fromSymbianBitmap(CFbsBitmap* bitmap, bool lockFormat)
{
    Q_ASSERT(bitmap);

    release();

    cfbsBitmap = bitmap;
    formatLocked = lockFormat;

    setSerialNumber(cfbsBitmap->Handle());

    UPDATE_BUFFER();

    // Create default palette if needed
    if (cfbsBitmap->DisplayMode() == EGray2) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::color0).rgba());
        image.setColor(1, QColor(Qt::color1).rgba());

        //Symbian thinks set pixels are white/transparent, Qt thinks they are foreground/solid
        //So invert mono bitmaps so that masks work correctly.
        image.invertPixels();
    } else if (cfbsBitmap->DisplayMode() == EGray256) {
        for (int i=0; i < 256; ++i)
            image.setColor(i, qRgb(i, i, i));
    } else if (cfbsBitmap->DisplayMode() == EColor256) {
        const TColor256Util *palette = TColor256Util::Default();
        for (int i=0; i < 256; ++i)
            image.setColor(i, (QRgb)(palette->Color256(i).Value()));
    }
}

QImage QSymbianRasterPixmapData::toImage(const QRect &r) const
{
    QSymbianRasterPixmapData *that = const_cast<QSymbianRasterPixmapData*>(this);
    that->beginDataAccess();
    QImage copy = that->image.copy(r);
    that->endDataAccess();

    return copy;
}

void QSymbianRasterPixmapData::fromImage(const QImage &img, Qt::ImageConversionFlags flags)
{
    release();

    QImage sourceImage;

    if (pixelType() == BitmapType) {
        sourceImage = img.convertToFormat(QImage::Format_MonoLSB);
    } else {
        if (img.depth() == 1) {
            sourceImage = img.hasAlphaChannel()
                        ? img.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                        : img.convertToFormat(QImage::Format_RGB32);
        } else {

            QImage::Format opaqueFormat = QNativeImage::systemFormat();
            QImage::Format alphaFormat = QImage::Format_ARGB32_Premultiplied;

            if (!img.hasAlphaChannel()
                || ((flags & Qt::NoOpaqueDetection) == 0
                    && !const_cast<QImage &>(img).data_ptr()->checkForAlphaPixels())) {
                sourceImage = img.convertToFormat(opaqueFormat);
            } else {
                sourceImage = img.convertToFormat(alphaFormat);
            }
        }
    }


    QImage::Format destFormat = sourceImage.format();
    TDisplayMode mode;
    switch (destFormat) {
    case QImage::Format_MonoLSB:
        mode = EGray2;
        break;
    case QImage::Format_RGB32:
        mode = EColor16MU;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        if (S60->supportsPremultipliedAlpha) {
            mode = Q_SYMBIAN_ECOLOR16MAP;
            break;
        } else {
            destFormat = QImage::Format_ARGB32;
        }
        // Fall through intended
    case QImage::Format_ARGB32:
        mode = EColor16MA;
        break;
    case QImage::Format_Invalid:
        return;
    default:
        qWarning("Image format not supported: %d", image.format());
        return;
    }

    cfbsBitmap = createSymbianCFbsBitmap(TSize(sourceImage.width(), sourceImage.height()), mode);
    if (!cfbsBitmap) {
        qWarning("Could not create CFbsBitmap");
        release();
        return;
    }

    setSerialNumber(cfbsBitmap->Handle());

    const uchar *sptr = const_cast<const QImage &>(sourceImage).bits();
    symbianBitmapDataAccess->beginDataAccess(cfbsBitmap);
    uchar *dptr = (uchar*)cfbsBitmap->DataAddress();
    Mem::Copy(dptr, sptr, sourceImage.byteCount());
    symbianBitmapDataAccess->endDataAccess(cfbsBitmap);

    UPDATE_BUFFER();

    if (destFormat == QImage::Format_MonoLSB) {
		image.setColorCount(2);
		image.setColor(0, QColor(Qt::color0).rgba());
		image.setColor(1, QColor(Qt::color1).rgba());
	} else {
		image.setColorTable(sourceImage.colorTable());
	}
}

void QSymbianRasterPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    const QSymbianRasterPixmapData *s60Data = static_cast<const QSymbianRasterPixmapData*>(data);
    fromImage(s60Data->toImage(rect), Qt::AutoColor | Qt::OrderedAlphaDither);
}

bool QSymbianRasterPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    beginDataAccess();
    bool res = QRasterPixmapData::scroll(dx, dy, rect);
    endDataAccess();
    return res;
}

Q_GUI_EXPORT int qt_defaultDpiX();
Q_GUI_EXPORT int qt_defaultDpiY();

int QSymbianRasterPixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    if (!cfbsBitmap)
        return 0;

    switch (metric) {
    case QPaintDevice::PdmWidth:
        return cfbsBitmap->SizeInPixels().iWidth;
    case QPaintDevice::PdmHeight:
        return cfbsBitmap->SizeInPixels().iHeight;
    case QPaintDevice::PdmWidthMM:
        return qRound(cfbsBitmap->SizeInPixels().iWidth * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(cfbsBitmap->SizeInPixels().iHeight * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmNumColors:
        return TDisplayModeUtils::NumDisplayModeColors(cfbsBitmap->DisplayMode());
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    case QPaintDevice::PdmDepth:
        return TDisplayModeUtils::NumDisplayModeBitsPerPixel(cfbsBitmap->DisplayMode());
    default:
        qWarning("QPixmap::metric: Invalid metric command");
    }
    return 0;

}

void QSymbianRasterPixmapData::fill(const QColor &color)
{
    if (color.alpha() != 255) {
        QImage im(width(), height(), QImage::Format_ARGB32_Premultiplied);
        im.fill(PREMUL(color.rgba()));
        release();
        fromImage(im, Qt::AutoColor | Qt::OrderedAlphaDither);
    } else {
        beginDataAccess();
        QRasterPixmapData::fill(color);
        endDataAccess();
    }
}

void QSymbianRasterPixmapData::setMask(const QBitmap &mask)
{
    if (mask.size().isEmpty()) {
        if (image.depth() != 1) {
            QImage newImage = image.convertToFormat(QImage::Format_RGB32);
            release();
            fromImage(newImage,  Qt::AutoColor | Qt::OrderedAlphaDither);
        }
    } else if (image.depth() == 1) {
        beginDataAccess();
        QRasterPixmapData::setMask(mask);
        endDataAccess();
    } else {
        const int w = image.width();
        const int h = image.height();

        const QImage imageMask = mask.toImage().convertToFormat(QImage::Format_MonoLSB);
        QImage newImage = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        for (int y = 0; y < h; ++y) {
            const uchar *mscan = imageMask.scanLine(y);
            QRgb *tscan = (QRgb *)newImage.scanLine(y);
            for (int x = 0; x < w; ++x) {
                if (!(mscan[x>>3] & qt_pixmap_bit_mask[x&7]))
                    tscan[x] = 0;
            }
        }
        release();
        fromImage(newImage,  Qt::AutoColor | Qt::OrderedAlphaDither);
    }
}

void QSymbianRasterPixmapData::setAlphaChannel(const QPixmap &alphaChannel)
{
    QImage img(toImage());
    img.setAlphaChannel(alphaChannel.toImage());
    release();
    fromImage(img, Qt::OrderedDither | Qt::OrderedAlphaDither);
}

QImage QSymbianRasterPixmapData::toImage() const
{
    return toImage(QRect());
}

QPaintEngine* QSymbianRasterPixmapData::paintEngine() const
{
    if (!pengine) {
        QSymbianRasterPixmapData *that = const_cast<QSymbianRasterPixmapData*>(this);
        that->pengine = new QSymbianRasterPaintEngine(&that->image, that);
    }
    return pengine;
}

void QSymbianRasterPixmapData::beginDataAccess()
{
    if(!cfbsBitmap)
        return;

    symbianBitmapDataAccess->beginDataAccess(cfbsBitmap);

    uchar* newBytes = (uchar*)cfbsBitmap->DataAddress();

    TSize size = cfbsBitmap->SizeInPixels();

    if (newBytes == bytes && image.width() == size.iWidth && image.height() == size.iHeight)
        return;

    bytes = newBytes;
    TDisplayMode mode = cfbsBitmap->DisplayMode();
    QImage::Format format = qt_TDisplayMode2Format(mode);
    // On S60 3.1, premultiplied alpha pixels are stored in a bitmap with 16MA type.
    // S60 window surface needs backing store pixmap for transparent window in ARGB32 format.
    // In that case formatLocked is true.
    if (!formatLocked && format == QImage::Format_ARGB32)
        format = QImage::Format_ARGB32_Premultiplied; // pixel data is actually in premultiplied format

    QVector<QRgb> savedColorTable;
    if (!image.isNull())
        savedColorTable = image.colorTable();

    image = QImage(bytes, size.iWidth, size.iHeight, format);

    // Restore the palette or create a default
    if (!savedColorTable.isEmpty()) {
        image.setColorTable(savedColorTable);
    }

    w = size.iWidth;
    h = size.iHeight;
    d = image.depth();
    is_null = (w <= 0 || h <= 0);

    if (pengine) {
        QSymbianRasterPaintEngine *engine = static_cast<QSymbianRasterPaintEngine *>(pengine);
        engine->prepare(&image);
    }
}

void QSymbianRasterPixmapData::endDataAccess(bool readOnly) const
{
    Q_UNUSED(readOnly);

    if(!cfbsBitmap)
        return;

    symbianBitmapDataAccess->endDataAccess(cfbsBitmap);
}

/*!
  \since 4.6

  Returns a QPixmap that wraps given \a sgImage graphics resource.
  The data should be valid even when original RSgImage handle has been
  closed.

  \warning This function is only available on Symbian OS.

  \sa toSymbianRSgImage(), {QPixmap#Pixmap Conversion}{Pixmap Conversion}
*/

QPixmap QPixmap::fromSymbianRSgImage(RSgImage *sgImage)
{
    // It is expected that RSgImage will
    // CURRENTLY be used in conjuction with
    // OpenVG graphics system
    //
    // Surely things might change in future

    if (!sgImage)
        return QPixmap();

    QScopedPointer<QPixmapData> data(QPixmapData::create(0,0, QPixmapData::PixmapType));
    data->fromNativeType(reinterpret_cast<void*>(sgImage), QPixmapData::SgImage);
    QPixmap pixmap(data.take());
    return pixmap;
}

/*!
\since 4.6

Returns a \c RSgImage that is equivalent to the QPixmap by copying the data.

It is the caller's responsibility to close/delete the \c RSgImage after use.

\warning This function is only available on Symbian OS.

\sa fromSymbianRSgImage()
*/

RSgImage *QPixmap::toSymbianRSgImage() const
{
    // It is expected that RSgImage will
    // CURRENTLY be used in conjuction with
    // OpenVG graphics system
    //
    // Surely things might change in future

    if (isNull())
        return 0;

    RSgImage *sgImage = reinterpret_cast<RSgImage*>(pixmapData()->toNativeType(QPixmapData::SgImage));

    return sgImage;
}

void* QSymbianRasterPixmapData::toNativeType(NativeType type)
{
    if (type == QPixmapData::SgImage) {
        return 0;
    } else if (type == QPixmapData::FbsBitmap) {

        if (isNull() || !cfbsBitmap)
            return 0;

        bool convertToArgb32 = false;
        bool needsCopy = false;

        if (!(S60->supportsPremultipliedAlpha)) {
            // Convert argb32_premultiplied to argb32 since Symbian 9.2 does
            // not support premultipied format.

            if (image.format() == QImage::Format_ARGB32_Premultiplied) {
                needsCopy = true;
                convertToArgb32 = true;
            }
        }

        CFbsBitmap *bitmap = 0;

        TDisplayMode displayMode = cfbsBitmap->DisplayMode();

        if(displayMode == EGray2) {
            //Symbian thinks set pixels are white/transparent, Qt thinks they are foreground/solid
            //So invert mono bitmaps so that masks work correctly.
            beginDataAccess();
            image.invertPixels();
            endDataAccess();
            needsCopy = true;
        }

        if (needsCopy) {
            QImage source;

            if (convertToArgb32) {
                beginDataAccess();
                source = image.convertToFormat(QImage::Format_ARGB32);
                endDataAccess();
                displayMode = EColor16MA;
            } else {
                source = image;
            }

            CFbsBitmap *newBitmap = createSymbianCFbsBitmap(TSize(source.width(), source.height()), displayMode);
            const uchar *sptr = source.bits();
            symbianBitmapDataAccess->beginDataAccess(newBitmap);

            uchar *dptr = (uchar*)newBitmap->DataAddress();
            Mem::Copy(dptr, sptr, source.byteCount());

            symbianBitmapDataAccess->endDataAccess(newBitmap);

            bitmap = newBitmap;
        } else {

            QT_TRAP_THROWING(bitmap = new (ELeave) CFbsBitmap);

            TInt err = bitmap->Duplicate(cfbsBitmap->Handle());
            if (err != KErrNone) {
                qWarning("Could not duplicate CFbsBitmap");
                delete bitmap;
                bitmap = 0;
            }
        }

        if(displayMode == EGray2) {
            // restore pixels
            beginDataAccess();
            image.invertPixels();
            endDataAccess();
        }

        return reinterpret_cast<void*>(bitmap);

    }

    return 0;
}

void QSymbianRasterPixmapData::fromNativeType(void* pixmap, NativeType nativeType)
{
    if (nativeType == QPixmapData::SgImage) {
        return;
    } else if (nativeType == QPixmapData::FbsBitmap && pixmap) {

        CFbsBitmap *bitmap = reinterpret_cast<CFbsBitmap*>(pixmap);

        bool deleteSourceBitmap = false;
        bool needsCopy = false;

#ifdef Q_SYMBIAN_HAS_EXTENDED_BITMAP_TYPE

        // Rasterize extended bitmaps

        TUid extendedBitmapType = bitmap->ExtendedBitmapType();
        if (extendedBitmapType != KNullUid) {
            CFbsBitmap *rasterBitmap = createSymbianCFbsBitmap(bitmap->SizeInPixels(), EColor16MA);

            CFbsBitmapDevice *rasterBitmapDev = 0;
            QT_TRAP_THROWING(rasterBitmapDev = CFbsBitmapDevice::NewL(rasterBitmap));

            CFbsBitGc *rasterBitmapGc = 0;
            TInt err = rasterBitmapDev->CreateContext(rasterBitmapGc);
            if (err != KErrNone) {
                delete rasterBitmap;
                delete rasterBitmapDev;
                rasterBitmapDev = 0;
                return;
            }

            rasterBitmapGc->BitBlt(TPoint( 0, 0), bitmap);

            bitmap = rasterBitmap;

            delete rasterBitmapDev;
            delete rasterBitmapGc;

            rasterBitmapDev = 0;
            rasterBitmapGc = 0;

            deleteSourceBitmap = true;
        }
#endif


        deleteSourceBitmap = bitmap->IsCompressedInRAM();
        CFbsBitmap *sourceBitmap = uncompress(bitmap);

        TDisplayMode displayMode = sourceBitmap->DisplayMode();
        QImage::Format format = qt_TDisplayMode2Format(displayMode);

        QImage::Format opaqueFormat = QNativeImage::systemFormat();
        QImage::Format alphaFormat = QImage::Format_ARGB32_Premultiplied;

        if (format != opaqueFormat && format != alphaFormat && format != QImage::Format_MonoLSB)
            needsCopy = true;


        type = (format != QImage::Format_MonoLSB)
                    ? QPixmapData::PixmapType
                    : QPixmapData::BitmapType;

        if (needsCopy) {

            TSize size = sourceBitmap->SizeInPixels();
            int bytesPerLine = sourceBitmap->ScanLineLength(size.iWidth, displayMode);

            QSymbianBitmapDataAccess da;
            da.beginDataAccess(sourceBitmap);
            uchar *bytes = (uchar*)sourceBitmap->DataAddress();
            QImage img = QImage(bytes, size.iWidth, size.iHeight, bytesPerLine, format);
            img = img.copy();
            da.endDataAccess(sourceBitmap);

            if(displayMode == EGray2) {
                //Symbian thinks set pixels are white/transparent, Qt thinks they are foreground/solid
                //So invert mono bitmaps so that masks work correctly.
                img.invertPixels();
            } else if(displayMode == EColor16M) {
                img = img.rgbSwapped(); // EColor16M is BGR
            }

            fromImage(img, Qt::AutoColor);

            if(deleteSourceBitmap)
                delete sourceBitmap;
        } else {
            CFbsBitmap* duplicate = 0;
            QT_TRAP_THROWING(duplicate = new (ELeave) CFbsBitmap);

            TInt err = duplicate->Duplicate(sourceBitmap->Handle());
            if (err != KErrNone) {
                qWarning("Could not duplicate CFbsBitmap");

                if(deleteSourceBitmap)
                    delete sourceBitmap;

                delete duplicate;
                return;
            }

            fromSymbianBitmap(duplicate);

            if(deleteSourceBitmap)
                delete sourceBitmap;
        }
    }
}

void QSymbianRasterPixmapData::convertToDisplayMode(int mode)
{
    const TDisplayMode displayMode = static_cast<TDisplayMode>(mode);
    if (!cfbsBitmap || cfbsBitmap->DisplayMode() == displayMode)
        return;
    if (image.depth() != TDisplayModeUtils::NumDisplayModeBitsPerPixel(displayMode)) {
        qWarning("Cannot convert display mode due to depth mismatch");
        return;
    }

    const TSize size = cfbsBitmap->SizeInPixels();
    QScopedPointer<CFbsBitmap> newBitmap(createSymbianCFbsBitmap(size, displayMode));

    const uchar *sptr = const_cast<const QImage &>(image).bits();
    symbianBitmapDataAccess->beginDataAccess(newBitmap.data());
    uchar *dptr = (uchar*)newBitmap->DataAddress();
    Mem::Copy(dptr, sptr, image.byteCount());
    symbianBitmapDataAccess->endDataAccess(newBitmap.data());

    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
    delete cfbsBitmap;
    lock.relock();
    cfbsBitmap = newBitmap.take();
    setSerialNumber(cfbsBitmap->Handle());
    UPDATE_BUFFER();
}

QPixmapData *QSymbianRasterPixmapData::createCompatiblePixmapData() const
{
    return new QSymbianRasterPixmapData(pixelType());
}

QT_END_NAMESPACE
