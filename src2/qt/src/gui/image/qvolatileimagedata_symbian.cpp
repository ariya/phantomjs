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

#include "qvolatileimagedata_p.h"
#include <fbs.h>
#include <QtGui/private/qt_s60_p.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/private/qimage_p.h>

QT_BEGIN_NAMESPACE

static CFbsBitmap *rasterizeBitmap(CFbsBitmap *bitmap, TDisplayMode newMode)
{
    if (!bitmap) {
        return 0;
    }
    QScopedPointer<CFbsBitmap> newBitmap(new CFbsBitmap);
    const TSize size = bitmap->SizeInPixels();
    TInt err = newBitmap->Create(size, newMode);
    if (err != KErrNone) {
        qWarning("QVolatileImage: Failed to create new bitmap (w %d h %d dispmode %d err %d)",
                 size.iWidth, size.iHeight, newMode, err);
        return 0;
    }
    CFbsBitmapDevice *bitmapDevice = 0;
    CFbsBitGc *bitmapGc = 0;
    QT_TRAP_THROWING(bitmapDevice = CFbsBitmapDevice::NewL(newBitmap.data()));
    QScopedPointer<CFbsBitmapDevice> bitmapDevicePtr(bitmapDevice);
    QT_TRAP_THROWING(bitmapGc = CFbsBitGc::NewL());
    bitmapGc->Activate(bitmapDevice);
    bitmapGc->BitBlt(TPoint(), bitmap);
    delete bitmapGc;
    return newBitmap.take();
}

static inline TDisplayMode format2TDisplayMode(QImage::Format format)
{
    TDisplayMode mode;
    switch (format) {
    case QImage::Format_MonoLSB:
        mode = EGray2;
        break;
    case QImage::Format_Indexed8:
        mode = EColor256;
        break;
    case QImage::Format_RGB444:
        mode = EColor4K;
        break;
    case QImage::Format_RGB16:
        mode = EColor64K;
        break;
    case QImage::Format_RGB888:
        mode = EColor16M;
        break;
    case QImage::Format_RGB32:
        mode = EColor16MU;
        break;
    case QImage::Format_ARGB32:
        mode = EColor16MA;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        mode = Q_SYMBIAN_ECOLOR16MAP;
        break;
    default:
        qWarning("QVolatileImage: Unknown image format %d", format);
        mode = ENone;
        break;
    }
    return mode;
}

static CFbsBitmap *imageToBitmap(const QImage &image)
{
    if (image.isNull()) {
        return 0;
    }
    CFbsBitmap *bitmap = new CFbsBitmap;
    TInt err = bitmap->Create(TSize(image.width(), image.height()),
                              format2TDisplayMode(image.format()));
    if (err == KErrNone) {
        bitmap->BeginDataAccess();
        uchar *dptr = reinterpret_cast<uchar *>(bitmap->DataAddress());
        int bmpLineLen = bitmap->DataStride();
        int imgLineLen = image.bytesPerLine();
        if (bmpLineLen == imgLineLen) {
            qMemCopy(dptr, image.constBits(), image.byteCount());
        } else {
            int len = qMin(bmpLineLen, imgLineLen);
            const uchar *sptr = image.constBits();
            for (int y = 0; y < image.height(); ++y) {
                qMemCopy(dptr, sptr, len);
                dptr += bmpLineLen;
                sptr += imgLineLen;
            }
        }
        bitmap->EndDataAccess();
    } else {
        qWarning("QVolatileImage: Failed to create source bitmap (w %d h %d fmt %d err %d)",
                 image.width(), image.height(), image.format(), err);
        delete bitmap;
        bitmap = 0;
    }
    return bitmap;
}

static CFbsBitmap *copyData(const QVolatileImageData &source)
{
    source.beginDataAccess();
    CFbsBitmap *bmp = imageToBitmap(source.image);
    source.endDataAccess();
    return bmp;
}

static CFbsBitmap *convertData(const QVolatileImageData &source, QImage::Format newFormat)
{
    source.beginDataAccess();
    QImage img = source.image.convertToFormat(newFormat);
    CFbsBitmap *bmp = imageToBitmap(img);
    source.endDataAccess();
    return bmp;
}

static CFbsBitmap *duplicateBitmap(const CFbsBitmap &sourceBitmap)
{
    CFbsBitmap *bitmap = new CFbsBitmap;
    TInt err = bitmap->Duplicate(sourceBitmap.Handle());
    if (err != KErrNone) {
        qWarning("QVolatileImage: Failed to duplicate source bitmap (%d)", err);
        delete bitmap;
        bitmap = 0;
    }
    return bitmap;
}

static CFbsBitmap *createBitmap(int w, int h, QImage::Format format)
{
    CFbsBitmap *bitmap = new CFbsBitmap;
    TInt err = bitmap->Create(TSize(w, h), format2TDisplayMode(format));
    if (err != KErrNone) {
        qWarning("QVolatileImage: Failed to create source bitmap (w %d h %d fmt %d err %d)",
                 w, h, format, err);
        delete bitmap;
        bitmap = 0;
    }
    return bitmap;
}

static inline bool bitmapNeedsCopy(CFbsBitmap *bitmap)
{
    bool needsCopy = bitmap->IsCompressedInRAM();
#ifdef Q_SYMBIAN_HAS_EXTENDED_BITMAP_TYPE
    needsCopy |= (bitmap->ExtendedBitmapType() != KNullUid);
#endif
    return needsCopy;
}

static bool cleanup_function_registered = false;
static QVolatileImageData *firstImageData = 0;

static void cleanup()
{
    if (RFbsSession::GetSession()) {
        QVolatileImageData *imageData = firstImageData;
        while (imageData) {
            imageData->release();
            imageData = imageData->next;
        }
    }
}

static void ensureCleanup()
{
    // Destroy all underlying bitmaps in a post routine to prevent panics.
    // This is a must because CFbsBitmap destructor needs the fbs session,
    // that was used to create the bitmap, to be open still.
    if (!cleanup_function_registered) {
        qAddPostRoutine(cleanup);
        cleanup_function_registered = true;
    }
}

static void registerImageData(QVolatileImageData *imageData)
{
    ensureCleanup();
    imageData->next = firstImageData;
    if (firstImageData) {
        firstImageData->prev = imageData;
    }
    firstImageData = imageData;
}

static void unregisterImageData(QVolatileImageData *imageData)
{
    if (imageData->prev) {
        imageData->prev->next = imageData->next;
    } else {
        firstImageData = imageData->next;
    }
    if (imageData->next) {
        imageData->next->prev = imageData->prev;
    }
}

QVolatileImageData::QVolatileImageData()
    : next(0), prev(0), bitmap(0), pengine(0)
{
    registerImageData(this);
}

QVolatileImageData::QVolatileImageData(int w, int h, QImage::Format format)
    : next(0), prev(0), bitmap(0), pengine(0)
{
    registerImageData(this);
    bitmap = createBitmap(w, h, format);
    updateImage();
}

QVolatileImageData::QVolatileImageData(const QImage &sourceImage)
    : next(0), prev(0), bitmap(0), pengine(0)
{
    registerImageData(this);
    image = sourceImage;
    // The following is not mandatory, but we do it here to have a bitmap
    // created always in order to reduce local heap usage.
    ensureBitmap();
}

QVolatileImageData::QVolatileImageData(void *nativeImage, void *nativeMask)
    : next(0), prev(0), bitmap(0), pengine(0)
{
    registerImageData(this);
    if (nativeImage) {
        CFbsBitmap *source = static_cast<CFbsBitmap *>(nativeImage);
        CFbsBitmap *mask = static_cast<CFbsBitmap *>(nativeMask);
        initWithBitmap(source);
        if (mask) {
            applyMask(mask);
        }
    }
}

QVolatileImageData::QVolatileImageData(const QVolatileImageData &other)
{
    bitmap = 0;
    pengine = 0;
    next = prev = 0;
    registerImageData(this);
    if (!other.image.isNull()) {
        bitmap = copyData(other);
        updateImage();
    }
}

QVolatileImageData::~QVolatileImageData()
{
    release();
    unregisterImageData(this);
}

void QVolatileImageData::release()
{
    delete bitmap;
    bitmap = 0;
    delete pengine;
    pengine = 0;
}

void QVolatileImageData::beginDataAccess() const
{
    if (bitmap) {
        bitmap->BeginDataAccess();
    }
}

void QVolatileImageData::endDataAccess(bool readOnly) const
{
    if (bitmap) {
        bitmap->EndDataAccess(readOnly);
    }
}

bool QVolatileImageData::ensureFormat(QImage::Format format)
{
    if (image.isNull()) {
        return false;
    }
    if (image.format() != format) {
        CFbsBitmap *newBitmap = convertData(*this, format);
        if (newBitmap && newBitmap != bitmap) {
            delete bitmap;
            bitmap = newBitmap;
            updateImage();
        } else {
            return false;
        }
    }
    return true;
}

void *QVolatileImageData::duplicateNativeImage() const
{
    const_cast<QVolatileImageData *>(this)->ensureBitmap();
    if (bitmap) {
        if (bitmap->DisplayMode() == EColor16M) {
            // slow path: needs rgb swapping
            beginDataAccess();
            QImage tmp = image.rgbSwapped();
            endDataAccess(true);
            return imageToBitmap(tmp);
        } else if (bitmap->DisplayMode() == EGray2) {
            // slow path: needs inverting pixels
            beginDataAccess();
            QImage tmp = image.copy();
            endDataAccess(true);
            tmp.invertPixels();
            return imageToBitmap(tmp);
        } else {
            // fast path: just duplicate the bitmap
            return duplicateBitmap(*bitmap);
        }
    }
    return 0;
}

void QVolatileImageData::updateImage()
{
    if (bitmap) {
        TSize size = bitmap->SizeInPixels();
        beginDataAccess();
        // Use existing buffer, no copy.  The data address never changes so it
        // is enough to do this whenever we have a new CFbsBitmap.  N.B. never
        // use const uchar* here, that would create a read-only image data which
        // would make a copy in detach() even when refcount is 1.
        image = QImage(reinterpret_cast<uchar *>(bitmap->DataAddress()),
                       size.iWidth, size.iHeight, bitmap->DataStride(),
                       qt_TDisplayMode2Format(bitmap->DisplayMode()));
        endDataAccess(true);
    } else {
        image = QImage();
    }
}

void QVolatileImageData::initWithBitmap(CFbsBitmap *source)
{
    bool needsCopy = bitmapNeedsCopy(source);
    if (source->DisplayMode() == EColor16M) {
        // EColor16M is BGR
        CFbsBitmap *unswappedBmp = source;
        if (needsCopy) {
            unswappedBmp = rasterizeBitmap(source, source->DisplayMode());
        }
        unswappedBmp->BeginDataAccess();
        TSize sourceSize = unswappedBmp->SizeInPixels();
        QImage img((uchar *) unswappedBmp->DataAddress(),
                   sourceSize.iWidth, sourceSize.iHeight, unswappedBmp->DataStride(),
                   qt_TDisplayMode2Format(unswappedBmp->DisplayMode()));
        img = img.rgbSwapped();
        unswappedBmp->EndDataAccess(true);
        bitmap = imageToBitmap(img);
        if (needsCopy) {
            delete unswappedBmp;
        }
    } else if (needsCopy) {
        // Rasterize extended and compressed bitmaps.
        bitmap = rasterizeBitmap(source, EColor16MAP);
    } else if (source->DisplayMode() == EGray2) {
        // The pixels will be inverted, must make a copy.
        bitmap = rasterizeBitmap(source, source->DisplayMode());
    } else {
        // Efficient path: no pixel data copying. Just duplicate. This of course
        // means the original bitmap's data may get modified, but that's fine
        // and is in accordance with the QPixmap::fromSymbianCFbsBitmap() docs.
        bitmap = duplicateBitmap(*source);
    }
    updateImage();
    if (bitmap && bitmap->DisplayMode() == EGray2) {
        // Symbian thinks set pixels are white/transparent, Qt thinks they are
        // foreground/solid. Invert mono bitmaps so that masks work correctly.
        beginDataAccess();
        image.invertPixels();
        endDataAccess();
    }
}

void QVolatileImageData::applyMask(CFbsBitmap *mask)
{
    ensureFormat(QImage::Format_ARGB32_Premultiplied);
    bool destroyMask = false;
    if (bitmapNeedsCopy(mask)) {
        mask = rasterizeBitmap(mask, EColor16MU);
        if (!mask) {
            return;
        }
        destroyMask = true;
    }
    mask->BeginDataAccess();
    TSize maskSize = mask->SizeInPixels();
    QImage maskImg((const uchar *) mask->DataAddress(), maskSize.iWidth, maskSize.iHeight,
                   mask->DataStride(), qt_TDisplayMode2Format(mask->DisplayMode()));
    if (mask->DisplayMode() == EGray2) {
        maskImg = maskImg.copy();
        maskImg.invertPixels();
    }
    beginDataAccess();
    image.setAlphaChannel(maskImg);
    endDataAccess();
    mask->EndDataAccess(true);
    ensureImage();
    if (destroyMask) {
        delete mask;
    }
}

void QVolatileImageData::ensureImage()
{
    if (bitmap && !image.isNull()) {
        QImageData *imaged = image.data_ptr();
        if (imaged->ref != 1 || imaged->ro_data) {
            // This is bad, the imagedata got shared somehow. Detach, in order to
            // have the next check fail and thus have 'image' recreated.
            beginDataAccess();
            image.detach();
            endDataAccess(true);
        }
    }
    if (bitmap && image.constBits() != reinterpret_cast<const uchar *>(bitmap->DataAddress())) {
        // Should not ever get here. If we do it means that either 'image' has
        // been replaced with a copy (e.g. because some QImage API assigned a
        // new, regular QImage to *this) or the bitmap's data address changed
        // unexpectedly.
        qWarning("QVolatileImageData: Ptr mismatch");
        // Recover by recreating the image so that it uses the bitmap as its buffer.
        updateImage();
    }
}

void QVolatileImageData::ensureBitmap()
{
    if (!bitmap && !image.isNull()) {
        bitmap = imageToBitmap(image);
        updateImage();
    }
}

QT_END_NAMESPACE
