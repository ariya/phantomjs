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

/*! 
    \class QtIcoHandler
    \since 4.4
    \brief The QtIcoHandler class provides support for the ICO image format.
    \internal
*/



#include "qicohandler.h"
#include <QtCore/qendian.h>
#include <QtGui/QImage>
#include <QtCore/QFile>
#include <QtCore/QBuffer>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
    quint8  bWidth;               // Width of the image
    quint8  bHeight;              // Height of the image (times 2)
    quint8  bColorCount;          // Number of colors in image (0 if >=8bpp) [ not ture ]
    quint8  bReserved;            // Reserved
    quint16 wPlanes;              // Color Planes
    quint16 wBitCount;            // Bits per pixel
    quint32 dwBytesInRes;         // how many bytes in this resource?
    quint32 dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
#define ICONDIRENTRY_SIZE 16

typedef struct
{
    quint16 idReserved;   // Reserved
    quint16 idType;       // resource type (1 for icons)
    quint16 idCount;      // how many images?
    ICONDIRENTRY    idEntries[1]; // the entries for each image
} ICONDIR, *LPICONDIR;
#define ICONDIR_SIZE    6       // Exclude the idEntries field

typedef struct {                    // BMP information header
    quint32 biSize;                // size of this struct
    quint32 biWidth;               // pixmap width
    quint32 biHeight;              // pixmap height     (specifies the combined height of the XOR and AND masks)
    quint16 biPlanes;              // should be 1
    quint16 biBitCount;            // number of bits per pixel
    quint32 biCompression;         // compression method
    quint32 biSizeImage;           // size of image
    quint32 biXPelsPerMeter;       // horizontal resolution
    quint32 biYPelsPerMeter;       // vertical resolution
    quint32 biClrUsed;             // number of colors used
    quint32 biClrImportant;        // number of important colors
} BMP_INFOHDR ,*LPBMP_INFOHDR;
#define BMP_INFOHDR_SIZE 40

class ICOReader
{
public:
    ICOReader(QIODevice * iodevice);
    int count();
    QImage iconAt(int index);
    static bool canRead(QIODevice *iodev);

    static QList<QImage> read(QIODevice * device);

    static bool write(QIODevice * device, const QList<QImage> & images);

private:
    bool readHeader();
    bool readIconEntry(int index, ICONDIRENTRY * iconEntry);

    bool readBMPHeader(quint32 imageOffset, BMP_INFOHDR * header);
    void findColorInfo(QImage & image);
    void readColorTable(QImage & image);

    void readBMP(QImage & image);
    void read1BitBMP(QImage & image);
    void read4BitBMP(QImage & image);
    void read8BitBMP(QImage & image);
    void read16_24_32BMP(QImage & image);

    struct IcoAttrib
    {
        int nbits;
        int ncolors;
        int h;
        int w;
        int depth;
    } icoAttrib;

    QIODevice * iod;
    qint64 startpos;
    bool headerRead;
    ICONDIR iconDir;

};

// Data readers and writers that takes care of alignment and endian stuff.
static bool readIconDirEntry(QIODevice *iodev, ICONDIRENTRY *iconDirEntry)
{
    if (iodev) {
        uchar tmp[ICONDIRENTRY_SIZE];
        if (iodev->read((char*)tmp, ICONDIRENTRY_SIZE) == ICONDIRENTRY_SIZE) {
            iconDirEntry->bWidth = tmp[0];
            iconDirEntry->bHeight = tmp[1];
            iconDirEntry->bColorCount = tmp[2];
            iconDirEntry->bReserved = tmp[3];

            iconDirEntry->wPlanes = qFromLittleEndian<quint16>(&tmp[4]);
            iconDirEntry->wBitCount = qFromLittleEndian<quint16>(&tmp[6]);
            iconDirEntry->dwBytesInRes = qFromLittleEndian<quint32>(&tmp[8]);
            iconDirEntry->dwImageOffset = qFromLittleEndian<quint32>(&tmp[12]);
            return true;
        }
    }
    return false;
}

static bool writeIconDirEntry(QIODevice *iodev, const ICONDIRENTRY &iconEntry)
{
    if (iodev) {
        uchar tmp[ICONDIRENTRY_SIZE];
        tmp[0] = iconEntry.bWidth;
        tmp[1] = iconEntry.bHeight;
        tmp[2] = iconEntry.bColorCount;
        tmp[3] = iconEntry.bReserved;
        qToLittleEndian<quint16>(iconEntry.wPlanes, &tmp[4]);
        qToLittleEndian<quint16>(iconEntry.wBitCount, &tmp[6]);
        qToLittleEndian<quint32>(iconEntry.dwBytesInRes, &tmp[8]);
        qToLittleEndian<quint32>(iconEntry.dwImageOffset, &tmp[12]);
        return (iodev->write((char*)tmp,  ICONDIRENTRY_SIZE) == ICONDIRENTRY_SIZE) ? true : false;
    }

    return false;
}

static bool readIconDir(QIODevice *iodev, ICONDIR *iconDir)
{
    if (iodev) {
        uchar tmp[ICONDIR_SIZE];
        if (iodev->read((char*)tmp, ICONDIR_SIZE) == ICONDIR_SIZE) {
            iconDir->idReserved = qFromLittleEndian<quint16>(&tmp[0]);
            iconDir->idType = qFromLittleEndian<quint16>(&tmp[2]);
            iconDir->idCount = qFromLittleEndian<quint16>(&tmp[4]);
            return true;
        }
    }
    return false;
}

static bool writeIconDir(QIODevice *iodev, const ICONDIR &iconDir)
{
    if (iodev) {
        uchar tmp[6];
        qToLittleEndian(iconDir.idReserved, tmp);
        qToLittleEndian(iconDir.idType, &tmp[2]);
        qToLittleEndian(iconDir.idCount, &tmp[4]);
        return (iodev->write((char*)tmp,  6) == 6) ? true : false;
    }
    return false;
}

static bool readBMPInfoHeader(QIODevice *iodev, BMP_INFOHDR *pHeader)
{
    if (iodev) {
        uchar header[BMP_INFOHDR_SIZE];
        if (iodev->read((char*)header, BMP_INFOHDR_SIZE) == BMP_INFOHDR_SIZE) {
            pHeader->biSize = qFromLittleEndian<quint32>(&header[0]);
            pHeader->biWidth = qFromLittleEndian<quint32>(&header[4]);
            pHeader->biHeight = qFromLittleEndian<quint32>(&header[8]);
            pHeader->biPlanes = qFromLittleEndian<quint16>(&header[12]);
            pHeader->biBitCount = qFromLittleEndian<quint16>(&header[14]);
            pHeader->biCompression = qFromLittleEndian<quint32>(&header[16]);
            pHeader->biSizeImage = qFromLittleEndian<quint32>(&header[20]);
            pHeader->biXPelsPerMeter = qFromLittleEndian<quint32>(&header[24]);
            pHeader->biYPelsPerMeter = qFromLittleEndian<quint32>(&header[28]);
            pHeader->biClrUsed = qFromLittleEndian<quint32>(&header[32]);
            pHeader->biClrImportant = qFromLittleEndian<quint32>(&header[36]);
            return true;
        }
    }
    return false;
}

static bool writeBMPInfoHeader(QIODevice *iodev, const BMP_INFOHDR &header)
{
    if (iodev) {
        uchar tmp[BMP_INFOHDR_SIZE];
        qToLittleEndian<quint32>(header.biSize, &tmp[0]);
        qToLittleEndian<quint32>(header.biWidth, &tmp[4]);
        qToLittleEndian<quint32>(header.biHeight, &tmp[8]);
        qToLittleEndian<quint16>(header.biPlanes, &tmp[12]);
        qToLittleEndian<quint16>(header.biBitCount, &tmp[14]);
        qToLittleEndian<quint32>(header.biCompression, &tmp[16]);
        qToLittleEndian<quint32>(header.biSizeImage, &tmp[20]);
        qToLittleEndian<quint32>(header.biXPelsPerMeter, &tmp[24]);
        qToLittleEndian<quint32>(header.biYPelsPerMeter, &tmp[28]);
        qToLittleEndian<quint32>(header.biClrUsed, &tmp[32]);
        qToLittleEndian<quint32>(header.biClrImportant, &tmp[36]);

        return (iodev->write((char*)tmp, BMP_INFOHDR_SIZE) == BMP_INFOHDR_SIZE) ? true : false;
    }
    return false;
}


ICOReader::ICOReader(QIODevice * iodevice)
: iod(iodevice)
, startpos(0)
, headerRead(false)
{
}


int ICOReader::count()
{
    if (readHeader())
        return iconDir.idCount;
    return 0;
}

bool ICOReader::canRead(QIODevice *iodev)
{
    bool isProbablyICO = false;
    if (iodev) {
        qint64 oldPos = iodev->pos();

        ICONDIR ikonDir;
        if (readIconDir(iodev, &ikonDir)) {
            qint64 readBytes = ICONDIR_SIZE;
            if (readIconDirEntry(iodev, &ikonDir.idEntries[0])) {
                readBytes += ICONDIRENTRY_SIZE;
                // ICO format does not have a magic identifier, so we read 6 different values, which will hopefully be enough to identify the file.
                if (   ikonDir.idReserved == 0
                    && ikonDir.idType == 1
                    && ikonDir.idEntries[0].bReserved == 0
                    && ikonDir.idEntries[0].wPlanes <= 1
                    && ikonDir.idEntries[0].wBitCount <= 32     // Bits per pixel
                    && ikonDir.idEntries[0].dwBytesInRes >= 40  // Must be over 40, since sizeof (infoheader) == 40
                    ) {
                    isProbablyICO = true;
                }

                if (iodev->isSequential()) {
                    // Our structs might be padded due to alignment, so we need to fetch each member before we ungetChar() !
                    quint32 tmp = ikonDir.idEntries[0].dwImageOffset;
                    iodev->ungetChar((tmp >> 24) & 0xff);
                    iodev->ungetChar((tmp >> 16) & 0xff);
                    iodev->ungetChar((tmp >>  8) & 0xff);
                    iodev->ungetChar(tmp & 0xff);

                    tmp = ikonDir.idEntries[0].dwBytesInRes;
                    iodev->ungetChar((tmp >> 24) & 0xff);
                    iodev->ungetChar((tmp >> 16) & 0xff);
                    iodev->ungetChar((tmp >>  8) & 0xff);
                    iodev->ungetChar(tmp & 0xff);

                    tmp = ikonDir.idEntries[0].wBitCount;
                    iodev->ungetChar((tmp >>  8) & 0xff);
                    iodev->ungetChar(tmp & 0xff);

                    tmp = ikonDir.idEntries[0].wPlanes;
                    iodev->ungetChar((tmp >>  8) & 0xff);
                    iodev->ungetChar(tmp & 0xff);

                    iodev->ungetChar(ikonDir.idEntries[0].bReserved);
                    iodev->ungetChar(ikonDir.idEntries[0].bColorCount);
                    iodev->ungetChar(ikonDir.idEntries[0].bHeight);
                    iodev->ungetChar(ikonDir.idEntries[0].bWidth);
                }
            }

            if (iodev->isSequential()) {
                // Our structs might be padded due to alignment, so we need to fetch each member before we ungetChar() !
                quint32 tmp = ikonDir.idCount;
                iodev->ungetChar((tmp >>  8) & 0xff);
                iodev->ungetChar(tmp & 0xff);

                tmp = ikonDir.idType;
                iodev->ungetChar((tmp >>  8) & 0xff);
                iodev->ungetChar(tmp & 0xff);

                tmp = ikonDir.idReserved;
                iodev->ungetChar((tmp >>  8) & 0xff);
                iodev->ungetChar(tmp & 0xff);
            }
        }
        if (!iodev->isSequential()) iodev->seek(oldPos);
    }

    return isProbablyICO;
}

bool ICOReader::readHeader()
{
    if (iod && !headerRead) {
        startpos = iod->pos();
        if (readIconDir(iod, &iconDir)) {
            if (iconDir.idReserved == 0 || iconDir.idType == 1)
            headerRead = true;
        }
    }

    return headerRead;
}

bool ICOReader::readIconEntry(int index, ICONDIRENTRY *iconEntry)
{
    if (iod) {
        if (iod->seek(startpos + ICONDIR_SIZE + (index * ICONDIRENTRY_SIZE))) {
            return readIconDirEntry(iod, iconEntry);
        }
    }
    return false;
}



bool ICOReader::readBMPHeader(quint32 imageOffset, BMP_INFOHDR * header)
{
    if (iod) {
        if (iod->seek(startpos + imageOffset)) {
            if (readBMPInfoHeader(iod, header)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void ICOReader::findColorInfo(QImage & image)
{
    if (icoAttrib.ncolors > 0) {                // set color table
        readColorTable(image);
    } else if (icoAttrib.nbits == 16) { // don't support RGB values for 15/16 bpp
        image = QImage();
    }
}

void ICOReader::readColorTable(QImage & image)
{
    if (iod) {
        image.setColorCount(icoAttrib.ncolors);
        uchar rgb[4];
        for (int i=0; i<icoAttrib.ncolors; i++) {
            if (iod->read((char*)rgb, 4) != 4) {
            image = QImage();
            break;
            }
            image.setColor(i, qRgb(rgb[2],rgb[1],rgb[0]));
        }
    } else {
        image = QImage();
    }
}

void ICOReader::readBMP(QImage & image)
{
    if (icoAttrib.nbits == 1) {                // 1 bit BMP image
        read1BitBMP(image);
    } else if (icoAttrib.nbits == 4) {            // 4 bit BMP image
        read4BitBMP(image);
    } else if (icoAttrib.nbits == 8) {
        read8BitBMP(image);
    } else if (icoAttrib.nbits == 16 || icoAttrib.nbits == 24 || icoAttrib.nbits == 32 ) { // 16,24,32 bit BMP image
        read16_24_32BMP(image);
    }
}


/**
 * NOTE: A 1 bit BMP is only flipped vertically, and not horizontally like all other color depths!
 * (This is the same with the bitmask)
 *
 */
void ICOReader::read1BitBMP(QImage & image)
{
    if (iod) {

        int h = image.height();
        int bpl = image.bytesPerLine();

        while (--h >= 0) {
            if (iod->read((char*)image.scanLine(h),bpl) != bpl) {
                image = QImage();
                break;
            }
        }
    } else {
        image = QImage();
    }
}

void ICOReader::read4BitBMP(QImage & image)
{
    if (iod) {

        int h = icoAttrib.h;
        int buflen = ((icoAttrib.w+7)/8)*4;
        uchar *buf = new uchar[buflen];
        Q_CHECK_PTR(buf);

        while (--h >= 0) {
            if (iod->read((char*)buf,buflen) != buflen) {
                image = QImage();
                break;
            }
            register uchar *p = image.scanLine(h);
            uchar *b = buf;
            for (int i=0; i<icoAttrib.w/2; i++) {   // convert nibbles to bytes
                *p++ = *b >> 4;
                *p++ = *b++ & 0x0f;
            }
            if (icoAttrib.w & 1)                    // the last nibble
                *p = *b >> 4;
        }

        delete [] buf;

    } else {
        image = QImage();
    }
}

void ICOReader::read8BitBMP(QImage & image)
{
    if (iod) {

        int h = icoAttrib.h;
        int bpl = image.bytesPerLine();

        while (--h >= 0) {
            if (iod->read((char *)image.scanLine(h), bpl) != bpl) {
                image = QImage();
                break;
            }
        }
    } else {
        image = QImage();
    }
}

void ICOReader::read16_24_32BMP(QImage & image)
{
    if (iod) {
        int h = icoAttrib.h;
        register QRgb *p;
        QRgb  *end;
        uchar *buf = new uchar[image.bytesPerLine()];
        int    bpl = ((icoAttrib.w*icoAttrib.nbits+31)/32)*4;
        uchar *b;

        while (--h >= 0) {
            p = (QRgb *)image.scanLine(h);
            end = p + icoAttrib.w;
            if (iod->read((char *)buf, bpl) != bpl) {
                image = QImage();
                break;
            }
            b = buf;
            while (p < end) {
                if (icoAttrib.nbits == 24)
                    *p++ = qRgb(*(b+2), *(b+1), *b);
                else if (icoAttrib.nbits == 32)
                    *p++ = qRgba(*(b+2), *(b+1), *b, *(b+3));
                b += icoAttrib.nbits/8;
            }
        }

        delete[] buf;

    } else {
        image = QImage();
    }
}

QImage ICOReader::iconAt(int index)
{
    QImage img;

    if (count() > index) { // forces header to be read

        ICONDIRENTRY iconEntry;
        if (readIconEntry(index, &iconEntry)) {

            static const uchar pngMagicData[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

            iod->seek(iconEntry.dwImageOffset);

            const QByteArray pngMagic = QByteArray::fromRawData((char*)pngMagicData, sizeof(pngMagicData));
            const bool isPngImage = (iod->read(pngMagic.size()) == pngMagic);

            if (isPngImage) {
                iod->seek(iconEntry.dwImageOffset);
                return QImage::fromData(iod->read(iconEntry.dwBytesInRes), "png");
            }

            BMP_INFOHDR header;
            if (readBMPHeader(iconEntry.dwImageOffset, &header)) {
                icoAttrib.nbits = header.biBitCount ? header.biBitCount : iconEntry.wBitCount;

                switch (icoAttrib.nbits) {
                case 32:
                case 24:
                case 16:
                    icoAttrib.depth = 32;
                    break;
                case 8:
                case 4:
                    icoAttrib.depth = 8;
                    break;
                default:
                    icoAttrib.depth = 1;
                }
                if (icoAttrib.depth == 32)                // there's no colormap
                    icoAttrib.ncolors = 0;
                else                    // # colors used
                    icoAttrib.ncolors = header.biClrUsed ? header.biClrUsed : 1 << icoAttrib.nbits;
                if (icoAttrib.ncolors > 256) //color table can't be more than 256
                    return img;
                icoAttrib.w = iconEntry.bWidth;
                if (icoAttrib.w == 0)
                    icoAttrib.w = header.biWidth;
                icoAttrib.h = iconEntry.bHeight;
                if (icoAttrib.h == 0)
                    icoAttrib.h = header.biHeight/2;

                QImage::Format format = QImage::Format_ARGB32;
                if (icoAttrib.nbits == 24)
                    format = QImage::Format_RGB32;
                else if (icoAttrib.ncolors == 2)
                    format = QImage::Format_Mono;
                else if (icoAttrib.ncolors > 0)
                    format = QImage::Format_Indexed8;

                QImage image(icoAttrib.w, icoAttrib.h, format);
                if (!image.isNull()) {
                    findColorInfo(image);
                    if (!image.isNull()) {
                        readBMP(image);
                        if (!image.isNull()) {
                            QImage mask(image.width(), image.height(), QImage::Format_Mono);
                            if (!mask.isNull()) {
                                mask.setColorCount(2);
                                mask.setColor(0, qRgba(255,255,255,0xff));
                                mask.setColor(1, qRgba(0  ,0  ,0  ,0xff));
                                read1BitBMP(mask);
                                if (!mask.isNull()) {
                                    img = QImage(image.width(), image.height(), QImage::Format_ARGB32 );
                                    img = image;
                                    img.setAlphaChannel(mask);
                                    // (Luckily, it seems that setAlphaChannel() does not ruin the alpha values
                                    // of partially transparent pixels in those icons that have that)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return img;
}


/*!
    Reads all the icons from the given \a device, and returns them as
    a list of QImage objects.

    Each image has an alpha channel that represents the mask from the
    corresponding icon.

    \sa write()
*/
QList<QImage> ICOReader::read(QIODevice * device)
{
    QList<QImage> images;

    ICOReader reader(device);
    for (int i = 0; i < reader.count(); i++)
        images += reader.iconAt(i);

    return images;
}


/*!
    Writes all the QImages in the \a images list to the given \a
    device. Returns true if the images are written successfully;
    otherwise returns false.

    The first image in the list is stored as the first icon in the
    device, and is therefore used as the default icon by applications.
    The alpha channel of each image is converted to a mask for each
    corresponding icon.

    \sa read()
*/
bool ICOReader::write(QIODevice * device, const QList<QImage> & images)
{
    bool retValue = false;

    if (images.count()) {

        qint64 origOffset = device->pos();

        ICONDIR id;
        id.idReserved = 0;
        id.idType = 1;
        id.idCount = images.count();

        ICONDIRENTRY * entries = new ICONDIRENTRY[id.idCount];
        BMP_INFOHDR * bmpHeaders = new BMP_INFOHDR[id.idCount];
        QByteArray * imageData = new QByteArray[id.idCount];

        for (int i=0; i<id.idCount; i++) {

            QImage image = images[i];
            // Scale down the image if it is larger than 128 pixels in either width or height
            if (image.width() > 128 || image.height() > 128)
            {
                image = image.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            QImage maskImage(image.width(), image.height(), QImage::Format_Mono);
            image = image.convertToFormat(QImage::Format_ARGB32);

            if (image.hasAlphaChannel()) {
                maskImage = image.createAlphaMask();
            } else {
                maskImage.fill(0xff);
            }
            maskImage = maskImage.convertToFormat(QImage::Format_Mono);

            int    nbits = 32;
            int    bpl_bmp = ((image.width()*nbits+31)/32)*4;

            entries[i].bColorCount = 0;
            entries[i].bReserved = 0;
            entries[i].wBitCount = nbits;
            entries[i].bHeight = image.height();
            entries[i].bWidth = image.width();
            entries[i].dwBytesInRes = BMP_INFOHDR_SIZE + (bpl_bmp * image.height())
                + (maskImage.bytesPerLine() * maskImage.height());
            entries[i].wPlanes = 1;
            if (i == 0)
                entries[i].dwImageOffset = origOffset + ICONDIR_SIZE
                + (id.idCount * ICONDIRENTRY_SIZE);
            else
                entries[i].dwImageOffset = entries[i-1].dwImageOffset + entries[i-1].dwBytesInRes;

            bmpHeaders[i].biBitCount = entries[i].wBitCount;
            bmpHeaders[i].biClrImportant = 0;
            bmpHeaders[i].biClrUsed = entries[i].bColorCount;
            bmpHeaders[i].biCompression = 0;
            bmpHeaders[i].biHeight = entries[i].bHeight * 2; // 2 is for the mask
            bmpHeaders[i].biPlanes = entries[i].wPlanes;
            bmpHeaders[i].biSize = BMP_INFOHDR_SIZE;
            bmpHeaders[i].biSizeImage = entries[i].dwBytesInRes - BMP_INFOHDR_SIZE;
            bmpHeaders[i].biWidth = entries[i].bWidth;
            bmpHeaders[i].biXPelsPerMeter = 0;
            bmpHeaders[i].biYPelsPerMeter = 0;

            QBuffer buffer(&imageData[i]);
            buffer.open(QIODevice::WriteOnly);

            uchar *buf = new uchar[bpl_bmp];
            uchar *b;
            memset( buf, 0, bpl_bmp );
            int y;
            for (y = image.height() - 1; y >= 0; y--) {    // write the image bits
                // 32 bits
                QRgb *p   = (QRgb *)image.scanLine(y);
                QRgb *end = p + image.width();
                b = buf;
                int x = 0;
                while (p < end) {
                    *b++ = qBlue(*p);
                    *b++ = qGreen(*p);
                    *b++ = qRed(*p);
                    *b++ = qAlpha(*p);
                    if (qAlpha(*p) > 0)   // Even mostly transparent pixels must not be masked away
                        maskImage.setPixel(x, y, Qt::color1);  // (i.e. createAlphaMask() takes away too much)
                    p++;
                    x++;
                }
                buffer.write((char*)buf, bpl_bmp);
            }
            delete[] buf;

            maskImage.invertPixels();   // seems as though it needs this
            // NOTE! !! The mask is only flipped vertically - not horizontally !!
            for (y = maskImage.height() - 1; y >= 0; y--)
                buffer.write((char*)maskImage.scanLine(y), maskImage.bytesPerLine());
        }

        if (writeIconDir(device, id)) {
            int i;
            bool bOK = true;
            for (i = 0; i < id.idCount && bOK; i++) {
                bOK = writeIconDirEntry(device, entries[i]);
            }
            if (bOK) {
                for (i = 0; i < id.idCount && bOK; i++) {
                    bOK = writeBMPInfoHeader(device, bmpHeaders[i]);
                    bOK &= (device->write(imageData[i]) == (int) imageData[i].size());
                }
                retValue = bOK;
            }
        }

        delete [] entries;
        delete [] bmpHeaders;
        delete [] imageData;

    }
    return retValue;
}

/*!
    Constructs an instance of QtIcoHandler initialized to use \a device.
*/
QtIcoHandler::QtIcoHandler(QIODevice *device)
{
    m_currentIconIndex = 0;
    setDevice(device);
    m_pICOReader = new ICOReader(device);
}

/*!
    Destructor for QtIcoHandler.
*/
QtIcoHandler::~QtIcoHandler()
{
    delete m_pICOReader;
}

QVariant QtIcoHandler::option(ImageOption option) const
{
    if (option == Size) {
        QIODevice *device = QImageIOHandler::device();
        qint64 oldPos = device->pos();
        ICONDIRENTRY iconEntry;
        if (device->seek(oldPos + ICONDIR_SIZE + (m_currentIconIndex * ICONDIRENTRY_SIZE))) {
            if (readIconDirEntry(device, &iconEntry)) {
                device->seek(oldPos);
                return QSize(iconEntry.bWidth, iconEntry.bHeight);
            }
        }
        if (!device->isSequential())
            device->seek(oldPos);
    }
    return QVariant();
}

bool QtIcoHandler::supportsOption(ImageOption option) const
{
    return option == Size;
}

/*!
 * Verifies if some values (magic bytes) are set as expected in the header of the file.
 * If the magic bytes were found, it is assumed that the QtIcoHandler can read the file.
 *
 */
bool QtIcoHandler::canRead() const
{
    bool bCanRead = false;
    QIODevice *device = QImageIOHandler::device();
    if (device) {
        bCanRead = ICOReader::canRead(device);
        if (bCanRead)
            setFormat("ico");
    } else {
        qWarning("QtIcoHandler::canRead() called with no device");
    }
    return bCanRead;
}

/*! This static function is used by the plugin code, and is provided for convenience only.
    \a device must be an opened device with pointing to the start of the header data of the ICO file.
*/
bool QtIcoHandler::canRead(QIODevice *device)
{
    Q_ASSERT(device);
    return ICOReader::canRead(device);
}

/*! \reimp

*/
bool QtIcoHandler::read(QImage *image)
{
    bool bSuccess = false;
    QImage img = m_pICOReader->iconAt(m_currentIconIndex);

    // Make sure we only write to \a image when we succeed.
    if (!img.isNull()) {
        bSuccess = true;
        *image = img;
    }

    return bSuccess;
}


/*! \reimp

*/
bool QtIcoHandler::write(const QImage &image)
{
    QIODevice *device = QImageIOHandler::device();
    QList<QImage> imgs;
    imgs.append(image);
    return ICOReader::write(device, imgs);
}

/*!
 * Return the common identifier of the format.
 * For ICO format this will return "ico".
 */
QByteArray QtIcoHandler::name() const
{
    return "ico";
}


/*! \reimp

*/
int QtIcoHandler::imageCount() const
{
    return m_pICOReader->count();
}

/*! \reimp

*/
bool QtIcoHandler::jumpToImage(int imageNumber)
{
    if (imageNumber < imageCount()) {
        m_currentIconIndex = imageNumber;
    }

    return (imageNumber < imageCount()) ? true : false;
}

/*! \reimp

*/
bool QtIcoHandler::jumpToNextImage()
{
    return jumpToImage(m_currentIconIndex + 1);
}

QT_END_NAMESPACE
