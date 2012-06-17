/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qtiffhandler_p.h"
#include <qvariant.h>
#include <qdebug.h>
#include <qimage.h>
#include <qglobal.h>
extern "C" {
#include "tiffio.h"
}

QT_BEGIN_NAMESPACE

tsize_t qtiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    QIODevice* device = static_cast<QTiffHandler*>(fd)->device();
    return device->isReadable() ? device->read(static_cast<char *>(buf), size) : -1;
}

tsize_t qtiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return static_cast<QTiffHandler*>(fd)->device()->write(static_cast<char *>(buf), size);
}

toff_t qtiffSeekProc(thandle_t fd, toff_t off, int whence)
{
    QIODevice *device = static_cast<QTiffHandler*>(fd)->device();
    switch (whence) {
    case SEEK_SET:
        device->seek(off);
        break;
    case SEEK_CUR:
        device->seek(device->pos() + off);
        break;
    case SEEK_END:
        device->seek(device->size() + off);
        break;
    }

    return device->pos();
}

int qtiffCloseProc(thandle_t /*fd*/)
{
    return 0;
}

toff_t qtiffSizeProc(thandle_t fd)
{
    return static_cast<QTiffHandler*>(fd)->device()->size();
}

int qtiffMapProc(thandle_t /*fd*/, tdata_t* /*pbase*/, toff_t* /*psize*/)
{
    return 0;
}

void qtiffUnmapProc(thandle_t /*fd*/, tdata_t /*base*/, toff_t /*size*/)
{
}

// for 32 bits images
inline void rotate_right_mirror_horizontal(QImage *const image)// rotate right->mirrored horizontal
{
    const int height = image->height();
    const int width = image->width();
    QImage generated(/* width = */ height, /* height = */ width, image->format());
    const uint32 *originalPixel = reinterpret_cast<const uint32*>(image->bits());
    uint32 *const generatedPixels = reinterpret_cast<uint32*>(generated.bits());
    for (int row=0; row < height; ++row) {
        for (int col=0; col < width; ++col) {
            int idx = col * height + row;
            generatedPixels[idx] = *originalPixel;
            ++originalPixel;
        }
    }
    *image = generated;
}

inline void rotate_right_mirror_vertical(QImage *const image) // rotate right->mirrored vertical
{
    const int height = image->height();
    const int width = image->width();
    QImage generated(/* width = */ height, /* height = */ width, image->format());
    const int lastCol = width - 1;
    const int lastRow = height - 1;
    const uint32 *pixel = reinterpret_cast<const uint32*>(image->bits());
    uint32 *const generatedBits = reinterpret_cast<uint32*>(generated.bits());
    for (int row=0; row < height; ++row) {
        for (int col=0; col < width; ++col) {
            int idx = (lastCol - col) * height + (lastRow - row);
            generatedBits[idx] = *pixel;
            ++pixel;
        }
    }
    *image = generated;
}

QTiffHandler::QTiffHandler() : QImageIOHandler()
{
    compression = NoCompression;
}

bool QTiffHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("tiff");
        return true;
    }
    return false;
}

bool QTiffHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QTiffHandler::canRead() called with no device");
        return false;
    }

    // current implementation uses TIFFClientOpen which needs to be
    // able to seek, so sequential devices are not supported
    int pos = device->pos();
    if (pos != 0)
        device->seek(0);  // need the magic from the beginning
    QByteArray header = device->peek(4);
    if (pos != 0)
        device->seek(pos);  // put it back where we found it

    return header == QByteArray::fromRawData("\x49\x49\x2A\x00", 4)
           || header == QByteArray::fromRawData("\x4D\x4D\x00\x2A", 4);
}

bool QTiffHandler::read(QImage *image)
{
    if (!canRead())
        return false;

    TIFF *const tiff = TIFFClientOpen("foo",
                                      "r",
                                      this,
                                      qtiffReadProc,
                                      qtiffWriteProc,
                                      qtiffSeekProc,
                                      qtiffCloseProc,
                                      qtiffSizeProc,
                                      qtiffMapProc,
                                      qtiffUnmapProc);

    if (!tiff) {
        return false;
    }
    uint32 width;
    uint32 height;
    uint16 photometric;
    if (!TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width)
        || !TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height)
        || !TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric)) {
        TIFFClose(tiff);
        return false;
    }

    // BitsPerSample defaults to 1 according to the TIFF spec.
    uint16 bitPerSample;
    if (!TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitPerSample))
        bitPerSample = 1;
    uint16 samplesPerPixel; // they may be e.g. grayscale with 2 samples per pixel
    if (!TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel))
        samplesPerPixel = 1;

    bool grayscale = photometric == PHOTOMETRIC_MINISBLACK || photometric == PHOTOMETRIC_MINISWHITE;
    if (grayscale && bitPerSample == 1 && samplesPerPixel == 1) {
        if (image->size() != QSize(width, height) || image->format() != QImage::Format_Mono)
            *image = QImage(width, height, QImage::Format_Mono);
        QVector<QRgb> colortable(2);
        if (photometric == PHOTOMETRIC_MINISBLACK) {
            colortable[0] = 0xff000000;
            colortable[1] = 0xffffffff;
        } else {
            colortable[0] = 0xffffffff;
            colortable[1] = 0xff000000;
        }
        image->setColorTable(colortable);

        if (!image->isNull()) {
            for (uint32 y=0; y<height; ++y) {
                if (TIFFReadScanline(tiff, image->scanLine(y), y, 0) < 0) {
                    TIFFClose(tiff);
                    return false;
                }
            }
        }
    } else {
        if ((grayscale || photometric == PHOTOMETRIC_PALETTE) && bitPerSample == 8 && samplesPerPixel == 1) {
            if (image->size() != QSize(width, height) || image->format() != QImage::Format_Indexed8)
                *image = QImage(width, height, QImage::Format_Indexed8);
            if (!image->isNull()) {
                const uint16 tableSize = 256;
                QVector<QRgb> qtColorTable(tableSize);
                if (grayscale) {
                    for (int i = 0; i<tableSize; ++i) {
                        const int c = (photometric == PHOTOMETRIC_MINISBLACK) ? i : (255 - i);
                        qtColorTable[i] = qRgb(c, c, c);
                    }
                } else {
                    // create the color table
                    uint16 *redTable = 0;
                    uint16 *greenTable = 0;
                    uint16 *blueTable = 0;
                    if (!TIFFGetField(tiff, TIFFTAG_COLORMAP, &redTable, &greenTable, &blueTable)) {
                        TIFFClose(tiff);
                        return false;
                    }
                    if (!redTable || !greenTable || !blueTable) {
                        TIFFClose(tiff);
                        return false;
                    }

                    for (int i = 0; i<tableSize ;++i) {
                        const int red = redTable[i] / 257;
                        const int green = greenTable[i] / 257;
                        const int blue = blueTable[i] / 257;
                        qtColorTable[i] = qRgb(red, green, blue);
                    }
                }

                image->setColorTable(qtColorTable);
                for (uint32 y=0; y<height; ++y) {
                    if (TIFFReadScanline(tiff, image->scanLine(y), y, 0) < 0) {
                        TIFFClose(tiff);
                        return false;
                    }
                }

                // free redTable, greenTable and greenTable done by libtiff
            }
        } else {
            if (image->size() != QSize(width, height) || image->format() != QImage::Format_ARGB32)
                *image = QImage(width, height, QImage::Format_ARGB32);
            if (!image->isNull()) {
                const int stopOnError = 1;
                if (TIFFReadRGBAImageOriented(tiff, width, height, reinterpret_cast<uint32 *>(image->bits()), ORIENTATION_TOPLEFT, stopOnError)) {
                    for (uint32 y=0; y<height; ++y)
                        convert32BitOrder(image->scanLine(y), width);
                } else {
                    TIFFClose(tiff);
                    return false;
                }
            }
        }
    }

    if (image->isNull()) {
        TIFFClose(tiff);
        return false;
    }

    float resX = 0;
    float resY = 0;
    uint16 resUnit = RESUNIT_NONE;
    if (TIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &resUnit)
        && TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &resX)
        && TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &resY)) {

        switch(resUnit) {
        case RESUNIT_CENTIMETER:
            image->setDotsPerMeterX(qRound(resX * 100));
            image->setDotsPerMeterY(qRound(resY * 100));
            break;
        case RESUNIT_INCH:
            image->setDotsPerMeterX(qRound(resX * (100 / 2.54)));
            image->setDotsPerMeterY(qRound(resY * (100 / 2.54)));
            break;
        default:
            // do nothing as defaults have already
            // been set within the QImage class
            break;
        }
    }

    // rotate the image if the orientation is defined in the file
    uint16 orientationTag;
    if (TIFFGetField(tiff, TIFFTAG_ORIENTATION, &orientationTag)) {
        if (image->format() == QImage::Format_ARGB32) {
            // TIFFReadRGBAImageOriented() flip the image but does not rotate them
            switch (orientationTag) {
            case 5:
                rotate_right_mirror_horizontal(image);
                break;
            case 6:
                rotate_right_mirror_vertical(image);
                break;
            case 7:
                rotate_right_mirror_horizontal(image);
                break;
            case 8:
                rotate_right_mirror_vertical(image);
                break;
            }
        } else {
            switch (orientationTag) {
            case 1: // default orientation
                break;
            case 2: // mirror horizontal
                *image = image->mirrored(true, false);
                break;
            case 3: // mirror both
                *image = image->mirrored(true, true);
                break;
            case 4: // mirror vertical
                *image = image->mirrored(false, true);
                break;
            case 5: // rotate right mirror horizontal
                {
                    QMatrix transformation;
                    transformation.rotate(90);
                    *image = image->transformed(transformation);
                    *image = image->mirrored(true, false);
                    break;
                }
            case 6: // rotate right
                {
                    QMatrix transformation;
                    transformation.rotate(90);
                    *image = image->transformed(transformation);
                    break;
                }
            case 7: // rotate right, mirror vertical
                {
                    QMatrix transformation;
                    transformation.rotate(90);
                    *image = image->transformed(transformation);
                    *image = image->mirrored(false, true);
                    break;
                }
            case 8: // rotate left
                {
                    QMatrix transformation;
                    transformation.rotate(270);
                    *image = image->transformed(transformation);
                    break;
                }
            }
        }
    }


    TIFFClose(tiff);
    return true;
}

static bool checkGrayscale(const QVector<QRgb> &colorTable)
{
    if (colorTable.size() != 256)
        return false;

    const bool increasing = (colorTable.at(0) == 0xff000000);
    for (int i = 0; i < 256; ++i) {
        if ((increasing && colorTable.at(i) != qRgb(i, i, i))
            || (!increasing && colorTable.at(i) != qRgb(255 - i, 255 - i, 255 - i)))
            return false;
    }
    return true;
}

bool QTiffHandler::write(const QImage &image)
{
    if (!device()->isWritable())
        return false;

    TIFF *const tiff = TIFFClientOpen("foo",
                                      "w",
                                      this,
                                      qtiffReadProc,
                                      qtiffWriteProc,
                                      qtiffSeekProc,
                                      qtiffCloseProc,
                                      qtiffSizeProc,
                                      qtiffMapProc,
                                      qtiffUnmapProc);
    if (!tiff)
        return false;

    const int width = image.width();
    const int height = image.height();

    if (!TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width)
        || !TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height)
        || !TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) {
        TIFFClose(tiff);
        return false;
    }

    // set the resolution
    bool  resolutionSet = false;
    const int dotPerMeterX = image.dotsPerMeterX();
    const int dotPerMeterY = image.dotsPerMeterY();
    if ((dotPerMeterX % 100) == 0
        && (dotPerMeterY % 100) == 0) {
        resolutionSet = TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER)
                        && TIFFSetField(tiff, TIFFTAG_XRESOLUTION, dotPerMeterX/100.0)
                        && TIFFSetField(tiff, TIFFTAG_YRESOLUTION, dotPerMeterY/100.0);
    } else {
        resolutionSet = TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH)
                        && TIFFSetField(tiff, TIFFTAG_XRESOLUTION, static_cast<float>(image.logicalDpiX()))
                        && TIFFSetField(tiff, TIFFTAG_YRESOLUTION, static_cast<float>(image.logicalDpiY()));
    }
    if (!resolutionSet) {
        TIFFClose(tiff);
        return false;
    }

    // configure image depth
    const QImage::Format format = image.format();
    if (format == QImage::Format_Mono || format == QImage::Format_MonoLSB) {
        uint16 photometric = PHOTOMETRIC_MINISBLACK;
        if (image.colorTable().at(0) == 0xffffffff)
            photometric = PHOTOMETRIC_MINISWHITE;
        if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric)
            || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_CCITTRLE)
            || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 1)) {
            TIFFClose(tiff);
            return false;
        }

        // try to do the conversion in chunks no greater than 16 MB
        int chunks = (width * height / (1024 * 1024 * 16)) + 1;
        int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(QImage::Format_Mono);

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    } else if (format == QImage::Format_Indexed8) {
        const QVector<QRgb> colorTable = image.colorTable();
        bool isGrayscale = checkGrayscale(colorTable);
        if (isGrayscale) {
            uint16 photometric = PHOTOMETRIC_MINISBLACK;
            if (image.colorTable().at(0) == 0xffffffff)
                photometric = PHOTOMETRIC_MINISWHITE;
            if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric)
                    || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                TIFFClose(tiff);
                return false;
            }
        } else {
            if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE)
                    || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                TIFFClose(tiff);
                return false;
            }
            //// write the color table
            // allocate the color tables
            uint16 *redTable = static_cast<uint16 *>(qMalloc(256 * sizeof(uint16)));
            uint16 *greenTable = static_cast<uint16 *>(qMalloc(256 * sizeof(uint16)));
            uint16 *blueTable = static_cast<uint16 *>(qMalloc(256 * sizeof(uint16)));
            if (!redTable || !greenTable || !blueTable) {
                qFree(redTable);
                qFree(greenTable);
                qFree(blueTable);
                TIFFClose(tiff);
                return false;
            }

            // set the color table
            const int tableSize = colorTable.size();
            Q_ASSERT(tableSize <= 256);
            for (int i = 0; i<tableSize; ++i) {
                const QRgb color = colorTable.at(i);
                redTable[i] = qRed(color) * 257;
                greenTable[i] = qGreen(color) * 257;
                blueTable[i] = qBlue(color) * 257;
            }

            const bool setColorTableSuccess = TIFFSetField(tiff, TIFFTAG_COLORMAP, redTable, greenTable, blueTable);

            qFree(redTable);
            qFree(greenTable);
            qFree(blueTable);

            if (!setColorTableSuccess) {
                TIFFClose(tiff);
                return false;
            }
        }

        //// write the data
        // try to do the conversion in chunks no greater than 16 MB
        int chunks = (width * height/ (1024 * 1024 * 16)) + 1;
        int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y));

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);

    } else {
        if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
            || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_LZW)
            || !TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4)
            || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
            TIFFClose(tiff);
            return false;
        }
        // try to do the ARGB32 conversion in chunks no greater than 16 MB
        int chunks = (width * height * 4 / (1024 * 1024 * 16)) + 1;
        int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(QImage::Format_ARGB32);

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (QSysInfo::ByteOrder == QSysInfo::LittleEndian)
                    convert32BitOrder(chunk.scanLine(y - chunkStart), width);
                else
                    convert32BitOrderBigEndian(chunk.scanLine(y - chunkStart), width);

                if (TIFFWriteScanline(tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    }

    return true;
}

QByteArray QTiffHandler::name() const
{
    return "tiff";
}

QVariant QTiffHandler::option(ImageOption option) const
{
    if (option == Size && canRead()) {
        QSize imageSize;
        qint64 pos = device()->pos();
        TIFF *tiff = TIFFClientOpen("foo",
                                    "r",
                                    const_cast<QTiffHandler*>(this),
                                    qtiffReadProc,
                                    qtiffWriteProc,
                                    qtiffSeekProc,
                                    qtiffCloseProc,
                                    qtiffSizeProc,
                                    qtiffMapProc,
                                    qtiffUnmapProc);

        if (tiff) {
            uint32 width = 0;
            uint32 height = 0;
            TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
            imageSize = QSize(width, height);
            TIFFClose(tiff);
        }
        device()->seek(pos);
        if (imageSize.isValid())
            return imageSize;
    } else if (option == CompressionRatio) {
        return compression;
    } else if (option == ImageFormat) {
        return QImage::Format_ARGB32;
    }
    return QVariant();
}

void QTiffHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == CompressionRatio && value.type() == QVariant::Int)
        compression = value.toInt();
}

bool QTiffHandler::supportsOption(ImageOption option) const
{
    return option == CompressionRatio
            || option == Size
            || option == ImageFormat;
}

void QTiffHandler::convert32BitOrder(void *buffer, int width)
{
    uint32 *target = reinterpret_cast<uint32 *>(buffer);
    for (int32 x=0; x<width; ++x) {
        uint32 p = target[x];
        // convert between ARGB and ABGR
        target[x] = (p & 0xff000000)
                    | ((p & 0x00ff0000) >> 16)
                    | (p & 0x0000ff00)
                    | ((p & 0x000000ff) << 16);
    }
}

void QTiffHandler::convert32BitOrderBigEndian(void *buffer, int width)
{
    uint32 *target = reinterpret_cast<uint32 *>(buffer);
    for (int32 x=0; x<width; ++x) {
        uint32 p = target[x];
        target[x] = (p & 0xff000000) >> 24
                    | (p & 0x00ff0000) << 8
                    | (p & 0x0000ff00) << 8
                    | (p & 0x000000ff) << 8;
    }
}

QT_END_NAMESPACE
