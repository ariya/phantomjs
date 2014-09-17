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

#include "private/qppmhandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_PPM

#include <qimage.h>
#include <qvariant.h>
#include <qvector.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  PBM/PGM/PPM (ASCII and RAW) image read/write functions
 *****************************************************************************/

static int read_pbm_int(QIODevice *d)
{
    char c;
    int          val = -1;
    bool  digit;
    const int buflen = 100;
    char  buf[buflen];
    for (;;) {
        if (!d->getChar(&c))                // end of file
            break;
        digit = isdigit((uchar) c);
        if (val != -1) {
            if (digit) {
                val = 10*val + c - '0';
                continue;
            } else {
                if (c == '#')                        // comment
                    d->readLine(buf, buflen);
                break;
            }
        }
        if (digit)                                // first digit
            val = c - '0';
        else if (isspace((uchar) c))
            continue;
        else if (c == '#')
            (void)d->readLine(buf, buflen);
        else
            break;
    }
    return val;
}

static bool read_pbm_header(QIODevice *device, char& type, int& w, int& h, int& mcc)
{
    char buf[3];
    if (device->read(buf, 3) != 3)                        // read P[1-6]<white-space>
        return false;

    if (!(buf[0] == 'P' && isdigit((uchar) buf[1]) && isspace((uchar) buf[2])))
        return false;

    type = buf[1];
    if (type < '1' || type > '6')
        return false;

    w = read_pbm_int(device);                        // get image width
    h = read_pbm_int(device);                        // get image height

    if (type == '1' || type == '4')
        mcc = 1;                                  // ignore max color component
    else
        mcc = read_pbm_int(device);               // get max color component

    if (w <= 0 || w > 32767 || h <= 0 || h > 32767 || mcc <= 0)
        return false;                                        // weird P.M image

    return true;
}

static bool read_pbm_body(QIODevice *device, char type, int w, int h, int mcc, QImage *outImage)
{
    int nbits, y;
    int pbm_bpl;
    bool raw;

    QImage::Format format;
    switch (type) {
        case '1':                                // ascii PBM
        case '4':                                // raw PBM
            nbits = 1;
            format = QImage::Format_Mono;
            break;
        case '2':                                // ascii PGM
        case '5':                                // raw PGM
            nbits = 8;
            format = QImage::Format_Indexed8;
            break;
        case '3':                                // ascii PPM
        case '6':                                // raw PPM
            nbits = 32;
            format = QImage::Format_RGB32;
            break;
        default:
            return false;
    }
    raw = type >= '4';

    int maxc = mcc;
    if (maxc > 255)
        maxc = 255;
    if (outImage->size() != QSize(w, h) || outImage->format() != format) {
        *outImage = QImage(w, h, format);
        if (outImage->isNull())
            return false;
    }

    pbm_bpl = (nbits*w+7)/8;                        // bytes per scanline in PBM

    if (raw) {                                // read raw data
        if (nbits == 32) {                        // type 6
            pbm_bpl = mcc < 256 ? 3*w : 6*w;
            uchar *buf24 = new uchar[pbm_bpl], *b;
            QRgb  *p;
            QRgb  *end;
            for (y=0; y<h; y++) {
                if (device->read((char *)buf24, pbm_bpl) != pbm_bpl) {
                    delete[] buf24;
                    return false;
                }
                p = (QRgb *)outImage->scanLine(y);
                end = p + w;
                b = buf24;
                while (p < end) {
                    if (mcc < 256) {
                        *p++ = qRgb(b[0],b[1],b[2]);
                        b += 3;
                    } else {
                        *p++ = qRgb(((int(b[0]) * 256 + int(b[1]) + 1) * 256) / (mcc + 1) - 1,
                                    ((int(b[2]) * 256 + int(b[3]) + 1) * 256) / (mcc + 1) - 1,
                                    ((int(b[4]) * 256 + int(b[5]) + 1) * 256) / (mcc + 1) - 1);
                        b += 6;
                    }
                }
            }
            delete[] buf24;
        } else {                                // type 4,5
            for (y=0; y<h; y++) {
                if (device->read((char *)outImage->scanLine(y), pbm_bpl)
                        != pbm_bpl)
                    return false;
            }
        }
    } else {                                        // read ascii data
        register uchar *p;
        int n;
        for (y=0; y<h; y++) {
            p = outImage->scanLine(y);
            n = pbm_bpl;
            if (nbits == 1) {
                int b;
                int bitsLeft = w;
                while (n--) {
                    b = 0;
                    for (int i=0; i<8; i++) {
                        if (i < bitsLeft)
                            b = (b << 1) | (read_pbm_int(device) & 1);
                        else
                            b = (b << 1) | (0 & 1); // pad it our self if we need to
                    }
                    bitsLeft -= 8;
                    *p++ = b;
                }
            } else if (nbits == 8) {
                if (mcc == maxc) {
                    while (n--) {
                        *p++ = read_pbm_int(device);
                    }
                } else {
                    while (n--) {
                        *p++ = read_pbm_int(device) * maxc / mcc;
                    }
                }
            } else {                                // 32 bits
                n /= 4;
                int r, g, b;
                if (mcc == maxc) {
                    while (n--) {
                        r = read_pbm_int(device);
                        g = read_pbm_int(device);
                        b = read_pbm_int(device);
                        *((QRgb*)p) = qRgb(r, g, b);
                        p += 4;
                    }
                } else {
                    while (n--) {
                        r = read_pbm_int(device) * maxc / mcc;
                        g = read_pbm_int(device) * maxc / mcc;
                        b = read_pbm_int(device) * maxc / mcc;
                        *((QRgb*)p) = qRgb(r, g, b);
                        p += 4;
                    }
                }
            }
        }
    }

    if (nbits == 1) {                                // bitmap
        outImage->setColorCount(2);
        outImage->setColor(0, qRgb(255,255,255)); // white
        outImage->setColor(1, qRgb(0,0,0));        // black
    } else if (nbits == 8) {                        // graymap
        outImage->setColorCount(maxc+1);
        for (int i=0; i<=maxc; i++)
            outImage->setColor(i, qRgb(i*255/maxc,i*255/maxc,i*255/maxc));
    }

    return true;
}

static bool write_pbm_image(QIODevice *out, const QImage &sourceImage, const QByteArray &sourceFormat)
{
    QByteArray str;
    QImage image = sourceImage;
    QByteArray format = sourceFormat;

    format = format.left(3);                        // ignore RAW part
    bool gray = format == "pgm";

    if (format == "pbm") {
        image = image.convertToFormat(QImage::Format_Mono);
    } else if (image.depth() == 1) {
        image = image.convertToFormat(QImage::Format_Indexed8);
    } else {
        switch (image.format()) {
        case QImage::Format_RGB16:
        case QImage::Format_RGB666:
        case QImage::Format_RGB555:
        case QImage::Format_RGB888:
        case QImage::Format_RGB444:
            image = image.convertToFormat(QImage::Format_RGB32);
            break;
        case QImage::Format_ARGB8565_Premultiplied:
        case QImage::Format_ARGB6666_Premultiplied:
        case QImage::Format_ARGB8555_Premultiplied:
        case QImage::Format_ARGB4444_Premultiplied:
            image = image.convertToFormat(QImage::Format_ARGB32);
            break;
        default:
            break;
        }
    }

    if (image.depth() == 1 && image.colorCount() == 2) {
        if (qGray(image.color(0)) < qGray(image.color(1))) {
            // 0=dark/black, 1=light/white - invert
            image.detach();
            for (int y=0; y<image.height(); y++) {
                uchar *p = image.scanLine(y);
                uchar *end = p + image.bytesPerLine();
                while (p < end)
                    *p++ ^= 0xff;
            }
        }
    }

    uint w = image.width();
    uint h = image.height();

    str = "P\n";
    str += QByteArray::number(w);
    str += ' ';
    str += QByteArray::number(h);
    str += '\n';

    switch (image.depth()) {
        case 1: {
            str.insert(1, '4');
            if (out->write(str, str.length()) != str.length())
                return false;
            w = (w+7)/8;
            for (uint y=0; y<h; y++) {
                uchar* line = image.scanLine(y);
                if (w != (uint)out->write((char*)line, w))
                    return false;
            }
            }
            break;

        case 8: {
            str.insert(1, gray ? '5' : '6');
            str.append("255\n");
            if (out->write(str, str.length()) != str.length())
                return false;
            QVector<QRgb> color = image.colorTable();
            uint bpl = w*(gray ? 1 : 3);
            uchar *buf   = new uchar[bpl];
            for (uint y=0; y<h; y++) {
                uchar *b = image.scanLine(y);
                uchar *p = buf;
                uchar *end = buf+bpl;
                if (gray) {
                    while (p < end) {
                        uchar g = (uchar)qGray(color[*b++]);
                        *p++ = g;
                    }
                } else {
                    while (p < end) {
                        QRgb rgb = color[*b++];
                        *p++ = qRed(rgb);
                        *p++ = qGreen(rgb);
                        *p++ = qBlue(rgb);
                    }
                }
                if (bpl != (uint)out->write((char*)buf, bpl))
                    return false;
            }
            delete [] buf;
            }
            break;

        case 32: {
            str.insert(1, gray ? '5' : '6');
            str.append("255\n");
            if (out->write(str, str.length()) != str.length())
                return false;
            uint bpl = w*(gray ? 1 : 3);
            uchar *buf = new uchar[bpl];
            for (uint y=0; y<h; y++) {
                QRgb  *b = (QRgb*)image.scanLine(y);
                uchar *p = buf;
                uchar *end = buf+bpl;
                if (gray) {
                    while (p < end) {
                        uchar g = (uchar)qGray(*b++);
                        *p++ = g;
                    }
                } else {
                    while (p < end) {
                        QRgb rgb = *b++;
                        *p++ = qRed(rgb);
                        *p++ = qGreen(rgb);
                        *p++ = qBlue(rgb);
                    }
                }
                if (bpl != (uint)out->write((char*)buf, bpl))
                    return false;
            }
            delete [] buf;
            }
            break;

    default:
        return false;
    }

    return true;
}

QPpmHandler::QPpmHandler()
    : state(Ready)
{
}

bool QPpmHandler::readHeader()
{
    state = Error;
    if (!read_pbm_header(device(), type, width, height, mcc))
        return false;
    state = ReadHeader;
    return true;
}

bool QPpmHandler::canRead() const
{
    if (state == Ready && !canRead(device(), &subType))
        return false;

    if (state != Error) {
        setFormat(subType);
        return true;
    }

    return false;
}

bool QPpmHandler::canRead(QIODevice *device, QByteArray *subType)
{
    if (!device) {
        qWarning("QPpmHandler::canRead() called with no device");
        return false;
    }

    char head[2];
    if (device->peek(head, sizeof(head)) != sizeof(head))
        return false;

    if (head[0] != 'P')
        return false;

    if (head[1] == '1' || head[1] == '4') {
        if (subType)
            *subType = "pbm";
    } else if (head[1] == '2' || head[1] == '5') {
        if (subType)
            *subType = "pgm";
    } else if (head[1] == '3' || head[1] == '6') {
        if (subType)
            *subType = "ppm";
    } else {
        return false;
    }
    return true;
}

bool QPpmHandler::read(QImage *image)
{
    if (state == Error)
        return false;

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    if (!read_pbm_body(device(), type, width, height, mcc, image)) {
        state = Error;
        return false;
    }

    state = Ready;
    return true;
}

bool QPpmHandler::write(const QImage &image)
{
    return write_pbm_image(device(), image, subType);
}

bool QPpmHandler::supportsOption(ImageOption option) const
{
    return option == SubType
        || option == Size
        || option == ImageFormat;
}

QVariant QPpmHandler::option(ImageOption option) const
{
    if (option == SubType) {
        return subType;
    } else if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QPpmHandler*>(this)->readHeader())
            return QVariant();
        return QSize(width, height);
    } else if (option == ImageFormat) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QPpmHandler*>(this)->readHeader())
            return QVariant();
        QImage::Format format = QImage::Format_Invalid;
        switch (type) {
            case '1':                                // ascii PBM
            case '4':                                // raw PBM
            format = QImage::Format_Mono;
            break;
            case '2':                                // ascii PGM
            case '5':                                // raw PGM
                format = QImage::Format_Indexed8;
                break;
            case '3':                                // ascii PPM
            case '6':                                // raw PPM
                format = QImage::Format_RGB32;
                break;
            default:
                break;
        }
        return format;
    }
    return QVariant();
}

void QPpmHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == SubType)
        subType = value.toByteArray().toLower();
}

QByteArray QPpmHandler::name() const
{
    return subType.isEmpty() ? QByteArray("ppm") : subType;
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_PPM
