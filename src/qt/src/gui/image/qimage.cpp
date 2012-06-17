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

#include "qimage.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qmap.h"
#include "qmatrix.h"
#include "qtransform.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qstringlist.h"
#include "qvariant.h"
#include "qimagepixmapcleanuphooks_p.h"
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <private/qdrawhelper_p.h>
#include <private/qmemrotate_p.h>
#include <private/qpixmapdata_p.h>
#include <private/qimagescale_p.h>
#include <private/qsimd_p.h>

#include <qhash.h>

#if defined(Q_OS_SYMBIAN)
#include <private/qpaintengine_raster_symbian_p.h>
#else
#include <private/qpaintengine_raster_p.h>
#endif

#include <private/qimage_p.h>
#include <private/qfont_p.h>

QT_BEGIN_NAMESPACE

static inline bool checkPixelSize(const QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB8565_Premultiplied:
        return (sizeof(qargb8565) == 3);
    case QImage::Format_RGB666:
        return (sizeof(qrgb666) == 3);
    case QImage::Format_ARGB6666_Premultiplied:
        return (sizeof(qargb6666) == 3);
    case QImage::Format_RGB555:
        return (sizeof(qrgb555) == 2);
    case QImage::Format_ARGB8555_Premultiplied:
        return (sizeof(qargb8555) == 3);
    case QImage::Format_RGB888:
        return (sizeof(qrgb888) == 3);
    case QImage::Format_RGB444:
        return (sizeof(qrgb444) == 2);
    case QImage::Format_ARGB4444_Premultiplied:
        return (sizeof(qargb4444) == 2);
    default:
        return true;
    }
}

#if defined(Q_CC_DEC) && defined(__alpha) && (__DECCXX_VER-0 >= 50190001)
#pragma message disable narrowptr
#endif


#define QIMAGE_SANITYCHECK_MEMORY(image) \
    if ((image).isNull()) { \
        qWarning("QImage: out of memory, returning null image"); \
        return QImage(); \
    }


static QImage rotated90(const QImage &src);
static QImage rotated180(const QImage &src);
static QImage rotated270(const QImage &src);

// ### Qt 5: remove
Q_GUI_EXPORT qint64 qt_image_id(const QImage &image)
{
    return image.cacheKey();
}

const QVector<QRgb> *qt_image_colortable(const QImage &image)
{
    return &image.d->colortable;
}

QBasicAtomicInt qimage_serial_number = Q_BASIC_ATOMIC_INITIALIZER(1);

QImageData::QImageData()
    : ref(0), width(0), height(0), depth(0), nbytes(0), data(0),
#ifdef QT3_SUPPORT
      jumptable(0),
#endif
      format(QImage::Format_ARGB32), bytes_per_line(0),
      ser_no(qimage_serial_number.fetchAndAddRelaxed(1)),
      detach_no(0),
      dpmx(qt_defaultDpiX() * 100 / qreal(2.54)),
      dpmy(qt_defaultDpiY() * 100 / qreal(2.54)),
      offset(0, 0), own_data(true), ro_data(false), has_alpha_clut(false),
      is_cached(false), paintEngine(0)
{
}

/*! \fn QImageData * QImageData::create(const QSize &size, QImage::Format format, int numColors)

    \internal

    Creates a new image data.
    Returns 0 if invalid parameters are give or anything else failed.
*/
QImageData * QImageData::create(const QSize &size, QImage::Format format, int numColors)
{
    if (!size.isValid() || numColors < 0 || format == QImage::Format_Invalid)
        return 0;                                // invalid parameter(s)

    if (!checkPixelSize(format)) {
        qWarning("QImageData::create(): Invalid pixel size for format %i",
                 format);
        return 0;
    }

    uint width = size.width();
    uint height = size.height();
    uint depth = qt_depthForFormat(format);

    switch (format) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        numColors = 2;
        break;
    case QImage::Format_Indexed8:
        numColors = qBound(0, numColors, 256);
        break;
    default:
        numColors = 0;
        break;
    }

    const int bytes_per_line = ((width * depth + 31) >> 5) << 2; // bytes per scanline (must be multiple of 4)

    // sanity check for potential overflows
    if (INT_MAX/depth < width
        || bytes_per_line <= 0
        || height <= 0
        || INT_MAX/uint(bytes_per_line) < height
        || INT_MAX/sizeof(uchar *) < uint(height))
        return 0;

    QScopedPointer<QImageData> d(new QImageData);
    d->colortable.resize(numColors);
    if (depth == 1) {
        d->colortable[0] = QColor(Qt::black).rgba();
        d->colortable[1] = QColor(Qt::white).rgba();
    } else {
        for (int i = 0; i < numColors; ++i)
            d->colortable[i] = 0;
    }

    d->width = width;
    d->height = height;
    d->depth = depth;
    d->format = format;
    d->has_alpha_clut = false;
    d->is_cached = false;

    d->bytes_per_line = bytes_per_line;

    d->nbytes = d->bytes_per_line*height;
    d->data  = (uchar *)malloc(d->nbytes);

    if (!d->data) {
        return 0;
    }

    d->ref.ref();
    return d.take();

}

QImageData::~QImageData()
{
    if (is_cached)
        QImagePixmapCleanupHooks::executeImageHooks((((qint64) ser_no) << 32) | ((qint64) detach_no));
    delete paintEngine;
    if (data && own_data)
        free(data);
#ifdef QT3_SUPPORT
    if (jumptable)
        free(jumptable);
    jumptable = 0;
#endif
    data = 0;
}


bool QImageData::checkForAlphaPixels() const
{
    bool has_alpha_pixels = false;

    switch (format) {

    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
        has_alpha_pixels = has_alpha_clut;
        break;

    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied: {
        uchar *bits = data;
        for (int y=0; y<height && !has_alpha_pixels; ++y) {
            for (int x=0; x<width; ++x)
                has_alpha_pixels |= (((uint *)bits)[x] & 0xff000000) != 0xff000000;
            bits += bytes_per_line;
        }
    } break;

    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied: {
        uchar *bits = data;
        uchar *end_bits = data + bytes_per_line;

        for (int y=0; y<height && !has_alpha_pixels; ++y) {
            while (bits < end_bits) {
                has_alpha_pixels |= bits[0] != 0;
                bits += 3;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
    } break;

    case QImage::Format_ARGB6666_Premultiplied: {
        uchar *bits = data;
        uchar *end_bits = data + bytes_per_line;

        for (int y=0; y<height && !has_alpha_pixels; ++y) {
            while (bits < end_bits) {
                has_alpha_pixels |= (bits[0] & 0xfc) != 0;
                bits += 3;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
    } break;

    case QImage::Format_ARGB4444_Premultiplied: {
        uchar *bits = data;
        uchar *end_bits = data + bytes_per_line;

        for (int y=0; y<height && !has_alpha_pixels; ++y) {
            while (bits < end_bits) {
                has_alpha_pixels |= (bits[0] & 0xf0) != 0;
                bits += 2;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
    } break;

    default:
        break;
    }

    return has_alpha_pixels;
}

/*!
    \class QImage

    \ingroup painting
    \ingroup shared

    \reentrant

    \brief The QImage class provides a hardware-independent image
    representation that allows direct access to the pixel data, and
    can be used as a paint device.

    Qt provides four classes for handling image data: QImage, QPixmap,
    QBitmap and QPicture.  QImage is designed and optimized for I/O,
    and for direct pixel access and manipulation, while QPixmap is
    designed and optimized for showing images on screen. QBitmap is
    only a convenience class that inherits QPixmap, ensuring a
    depth of 1. Finally, the QPicture class is a paint device that
    records and replays QPainter commands.

    Because QImage is a QPaintDevice subclass, QPainter can be used to
    draw directly onto images.  When using QPainter on a QImage, the
    painting can be performed in another thread than the current GUI
    thread.

    The QImage class supports several image formats described by the
    \l Format enum. These include monochrome, 8-bit, 32-bit and
    alpha-blended images which are available in all versions of Qt
    4.x.

    QImage provides a collection of functions that can be used to
    obtain a variety of information about the image. There are also
    several functions that enables transformation of the image.

    QImage objects can be passed around by value since the QImage
    class uses \l{Implicit Data Sharing}{implicit data
    sharing}. QImage objects can also be streamed and compared.

    \note If you would like to load QImage objects in a static build of Qt,
    refer to the \l{How To Create Qt Plugins#Static Plugins}{Plugin HowTo}.

    \warning Painting on a QImage with the format
    QImage::Format_Indexed8 is not supported.

    \tableofcontents

    \section1 Reading and Writing Image Files

    QImage provides several ways of loading an image file: The file
    can be loaded when constructing the QImage object, or by using the
    load() or loadFromData() functions later on. QImage also provides
    the static fromData() function, constructing a QImage from the
    given data.  When loading an image, the file name can either refer
    to an actual file on disk or to one of the application's embedded
    resources. See \l{The Qt Resource System} overview for details
    on how to embed images and other resource files in the
    application's executable.

    Simply call the save() function to save a QImage object.

    The complete list of supported file formats are available through
    the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions. New file formats
    can be added as plugins. By default, Qt supports the following
    formats:

    \table
    \header \o Format \o Description                      \o Qt's support
    \row    \o BMP    \o Windows Bitmap                   \o Read/write
    \row    \o GIF    \o Graphic Interchange Format (optional) \o Read
    \row    \o JPG    \o Joint Photographic Experts Group \o Read/write
    \row    \o JPEG   \o Joint Photographic Experts Group \o Read/write
    \row    \o PNG    \o Portable Network Graphics        \o Read/write
    \row    \o PBM    \o Portable Bitmap                  \o Read
    \row    \o PGM    \o Portable Graymap                 \o Read
    \row    \o PPM    \o Portable Pixmap                  \o Read/write
    \row    \o TIFF   \o Tagged Image File Format         \o Read/write
    \row    \o XBM    \o X11 Bitmap                       \o Read/write
    \row    \o XPM    \o X11 Pixmap                       \o Read/write
    \endtable

    \section1 Image Information

    QImage provides a collection of functions that can be used to
    obtain a variety of information about the image:

    \table
    \header
    \o \o Available Functions

    \row
    \o Geometry
    \o

    The size(), width(), height(), dotsPerMeterX(), and
    dotsPerMeterY() functions provide information about the image size
    and aspect ratio.

    The rect() function returns the image's enclosing rectangle. The
    valid() function tells if a given pair of coordinates is within
    this rectangle. The offset() function returns the number of pixels
    by which the image is intended to be offset by when positioned
    relative to other images, which also can be manipulated using the
    setOffset() function.

    \row
    \o Colors
    \o

    The color of a pixel can be retrieved by passing its coordinates
    to the pixel() function.  The pixel() function returns the color
    as a QRgb value indepedent of the image's format.

    In case of monochrome and 8-bit images, the colorCount() and
    colorTable() functions provide information about the color
    components used to store the image data: The colorTable() function
    returns the image's entire color table. To obtain a single entry,
    use the pixelIndex() function to retrieve the pixel index for a
    given pair of coordinates, then use the color() function to
    retrieve the color. Note that if you create an 8-bit image
    manually, you have to set a valid color table on the image as
    well.

    The hasAlphaChannel() function tells if the image's format
    respects the alpha channel, or not. The allGray() and
    isGrayscale() functions tell whether an image's colors are all
    shades of gray.

    See also the \l {QImage#Pixel Manipulation}{Pixel Manipulation}
    and \l {QImage#Image Transformations}{Image Transformations}
    sections.

    \row
    \o Text
    \o

    The text() function returns the image text associated with the
    given text key. An image's text keys can be retrieved using the
    textKeys() function. Use the setText() function to alter an
    image's text.

    \row
    \o Low-level information
    \o

    The depth() function returns the depth of the image. The supported
    depths are 1 (monochrome), 8, 16, 24 and 32 bits. The
    bitPlaneCount() function tells how many of those bits that are
    used. For more information see the
    \l {QImage#Image Formats}{Image Formats} section.

    The format(), bytesPerLine(), and byteCount() functions provide
    low-level information about the data stored in the image.

    The cacheKey() function returns a number that uniquely
    identifies the contents of this QImage object.
    \endtable

    \section1 Pixel Manipulation

    The functions used to manipulate an image's pixels depend on the
    image format. The reason is that monochrome and 8-bit images are
    index-based and use a color lookup table, while 32-bit images
    store ARGB values directly. For more information on image formats,
    see the \l {Image Formats} section.

    In case of a 32-bit image, the setPixel() function can be used to
    alter the color of the pixel at the given coordinates to any other
    color specified as an ARGB quadruplet. To make a suitable QRgb
    value, use the qRgb() (adding a default alpha component to the
    given RGB values, i.e. creating an opaque color) or qRgba()
    function. For example:

    \table
    \header
    \o {2,1}32-bit
    \row
    \o \inlineimage qimage-32bit_scaled.png
    \o
    \snippet doc/src/snippets/code/src_gui_image_qimage.cpp 0
    \endtable

    In case of a 8-bit and monchrome images, the pixel value is only
    an index from the image's color table. So the setPixel() function
    can only be used to alter the color of the pixel at the given
    coordinates to a predefined color from the image's color table,
    i.e. it can only change the pixel's index value. To alter or add a
    color to an image's color table, use the setColor() function.

    An entry in the color table is an ARGB quadruplet encoded as an
    QRgb value. Use the qRgb() and qRgba() functions to make a
    suitable QRgb value for use with the setColor() function. For
    example:

    \table
    \header
    \o {2,1} 8-bit
    \row
    \o \inlineimage qimage-8bit_scaled.png
    \o
    \snippet doc/src/snippets/code/src_gui_image_qimage.cpp 1
    \endtable

    QImage also provide the scanLine() function which returns a
    pointer to the pixel data at the scanline with the given index,
    and the bits() function which returns a pointer to the first pixel
    data (this is equivalent to \c scanLine(0)).

    \section1 Image Formats

    Each pixel stored in a QImage is represented by an integer. The
    size of the integer varies depending on the format. QImage
    supports several image formats described by the \l Format
    enum.

    Monochrome images are stored using 1-bit indexes into a color table
    with at most two colors. There are two different types of
    monochrome images: big endian (MSB first) or little endian (LSB
    first) bit order.

    8-bit images are stored using 8-bit indexes into a color table,
    i.e.  they have a single byte per pixel. The color table is a
    QVector<QRgb>, and the QRgb typedef is equivalent to an unsigned
    int containing an ARGB quadruplet on the format 0xAARRGGBB.

    32-bit images have no color table; instead, each pixel contains an
    QRgb value. There are three different types of 32-bit images
    storing RGB (i.e. 0xffRRGGBB), ARGB and premultiplied ARGB
    values respectively. In the premultiplied format the red, green,
    and blue channels are multiplied by the alpha component divided by
    255.

    An image's format can be retrieved using the format()
    function. Use the convertToFormat() functions to convert an image
    into another format. The allGray() and isGrayscale() functions
    tell whether a color image can safely be converted to a grayscale
    image.

    \section1 Image Transformations

    QImage supports a number of functions for creating a new image
    that is a transformed version of the original: The
    createAlphaMask() function builds and returns a 1-bpp mask from
    the alpha buffer in this image, and the createHeuristicMask()
    function creates and returns a 1-bpp heuristic mask for this
    image. The latter function works by selecting a color from one of
    the corners, then chipping away pixels of that color starting at
    all the edges.

    The mirrored() function returns a mirror of the image in the
    desired direction, the scaled() returns a copy of the image scaled
    to a rectangle of the desired measures, and the rgbSwapped() function
    constructs a BGR image from a RGB image.

    The scaledToWidth() and scaledToHeight() functions return scaled
    copies of the image.

    The transformed() function returns a copy of the image that is
    transformed with the given transformation matrix and
    transformation mode: Internally, the transformation matrix is
    adjusted to compensate for unwanted translation,
    i.e. transformed() returns the smallest image containing all
    transformed points of the original image. The static trueMatrix()
    function returns the actual matrix used for transforming the
    image.

    There are also functions for changing attributes of an image
    in-place:

    \table
    \header \o Function \o Description
    \row
    \o setDotsPerMeterX()
    \o Defines the aspect ratio by setting the number of pixels that fit
    horizontally in a physical meter.
    \row
    \o setDotsPerMeterY()
    \o Defines the aspect ratio by setting the number of pixels that fit
    vertically in a physical meter.
    \row
    \o fill()
    \o Fills the entire image with the given pixel value.
    \row
    \o invertPixels()
    \o Inverts all pixel values in the image using the given InvertMode value.
    \row
    \o setColorTable()
    \o Sets the color table used to translate color indexes. Only
    monochrome and 8-bit formats.
    \row
    \o setColorCount()
    \o Resizes the color table. Only monochrome and 8-bit formats.

    \endtable

    \section1 Legal Information

    For smooth scaling, the transformed() functions use code based on
    smooth scaling algorithm by Daniel M. Duley.

    \legalese
     Copyright (C) 2004, 2005 Daniel M. Duley

     Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions
        are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
     IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
     OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
     IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
     THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    \endlegalese

    \sa QImageReader, QImageWriter, QPixmap, QSvgRenderer, {Image Composition Example},
        {Image Viewer Example}, {Scribble Example}, {Pixelator Example}
*/

/*!
    \enum QImage::Endian
    \compat

    This enum type is used to describe the endianness of the CPU and
    graphics hardware. It is provided here for compatibility with earlier versions of Qt.

    Use the \l Format enum instead. The \l Format enum specify the
    endianess for monchrome formats, but for other formats the
    endianess is not relevant.

    \value IgnoreEndian  Endianness does not matter. Useful for some
                         operations that are independent of endianness.
    \value BigEndian     Most significant bit first or network byte order, as on SPARC, PowerPC, and Motorola CPUs.
    \value LittleEndian  Least significant bit first or little endian byte order, as on Intel x86.
*/

/*!
    \enum QImage::InvertMode

    This enum type is used to describe how pixel values should be
    inverted in the invertPixels() function.

    \value InvertRgb    Invert only the RGB values and leave the alpha
                        channel unchanged.

    \value InvertRgba   Invert all channels, including the alpha channel.

    \sa invertPixels()
*/

/*!
    \enum QImage::Format

    The following image formats are available in Qt. Values greater
    than QImage::Format_RGB16 were added in Qt 4.4. See the notes
    after the table.

    \value Format_Invalid   The image is invalid.
    \value Format_Mono      The image is stored using 1-bit per pixel. Bytes are
                            packed with the most significant bit (MSB) first.
    \value Format_MonoLSB   The image is stored using 1-bit per pixel. Bytes are
                            packed with the less significant bit (LSB) first.

    \value Format_Indexed8  The image is stored using 8-bit indexes
                            into a colormap.

    \value Format_RGB32     The image is stored using a 32-bit RGB format (0xffRRGGBB).

    \value Format_ARGB32    The image is stored using a 32-bit ARGB
                            format (0xAARRGGBB).

    \value Format_ARGB32_Premultiplied  The image is stored using a premultiplied 32-bit
                            ARGB format (0xAARRGGBB), i.e. the red,
                            green, and blue channels are multiplied
                            by the alpha component divided by 255. (If RR, GG, or BB
                            has a higher value than the alpha channel, the results are
                            undefined.) Certain operations (such as image composition
                            using alpha blending) are faster using premultiplied ARGB32
                            than with plain ARGB32.

    \value Format_RGB16     The image is stored using a 16-bit RGB format (5-6-5).

    \value Format_ARGB8565_Premultiplied  The image is stored using a
                            premultiplied 24-bit ARGB format (8-5-6-5).
    \value Format_RGB666    The image is stored using a 24-bit RGB format (6-6-6).
                            The unused most significant bits is always zero.
    \value Format_ARGB6666_Premultiplied  The image is stored using a
                            premultiplied 24-bit ARGB format (6-6-6-6).
    \value Format_RGB555    The image is stored using a 16-bit RGB format (5-5-5).
                            The unused most significant bit is always zero.
    \value Format_ARGB8555_Premultiplied  The image is stored using a
                            premultiplied 24-bit ARGB format (8-5-5-5).
    \value Format_RGB888    The image is stored using a 24-bit RGB format (8-8-8).
    \value Format_RGB444    The image is stored using a 16-bit RGB format (4-4-4).
                            The unused bits are always zero.
    \value Format_ARGB4444_Premultiplied  The image is stored using a
                            premultiplied 16-bit ARGB format (4-4-4-4).

    \note Drawing into a QImage with QImage::Format_Indexed8 is not
    supported.

    \note Do not render into ARGB32 images using QPainter.  Using
    QImage::Format_ARGB32_Premultiplied is significantly faster.

    \sa format(), convertToFormat()
*/

/*****************************************************************************
  QImage member functions
 *****************************************************************************/

// table to flip bits
static const uchar bitflip[256] = {
    /*
        open OUT, "| fmt";
        for $i (0..255) {
            print OUT (($i >> 7) & 0x01) | (($i >> 5) & 0x02) |
                      (($i >> 3) & 0x04) | (($i >> 1) & 0x08) |
                      (($i << 7) & 0x80) | (($i << 5) & 0x40) |
                      (($i << 3) & 0x20) | (($i << 1) & 0x10), ", ";
        }
        close OUT;
    */
    0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

const uchar *qt_get_bitflip_array()                        // called from QPixmap code
{
    return bitflip;
}

#if defined(QT3_SUPPORT)
static QImage::Format formatFor(int depth, QImage::Endian bitOrder)
{
    QImage::Format format;
    if (depth == 1) {
        format = bitOrder == QImage::BigEndian ? QImage::Format_Mono : QImage::Format_MonoLSB;
    } else if (depth == 8) {
        format = QImage::Format_Indexed8;
    } else if (depth == 32) {
        format = QImage::Format_RGB32;
    } else if (depth == 24) {
        format = QImage::Format_RGB888;
    } else if (depth == 16) {
        format = QImage::Format_RGB16;
    } else {
        qWarning("QImage: Depth %d not supported", depth);
        format = QImage::Format_Invalid;
    }
    return format;
}
#endif

/*!
    Constructs a null image.

    \sa isNull()
*/

QImage::QImage()
    : QPaintDevice()
{
    d = 0;
}

/*!
    Constructs an image with the given \a width, \a height and \a
    format.

    A \l{isNull()}{null} image will be returned if memory cannot be allocated.

    \warning This will create a QImage with uninitialized data. Call
    fill() to fill the image with an appropriate pixel value before
    drawing onto it with QPainter.
*/
QImage::QImage(int width, int height, Format format)
    : QPaintDevice()
{
    d = QImageData::create(QSize(width, height), format, 0);
}

/*!
    Constructs an image with the given \a size and \a format.

    A \l{isNull()}{null} image is returned if memory cannot be allocated.

    \warning This will create a QImage with uninitialized data. Call
    fill() to fill the image with an appropriate pixel value before
    drawing onto it with QPainter.
*/
QImage::QImage(const QSize &size, Format format)
    : QPaintDevice()
{
    d = QImageData::create(size, format, 0);
}



QImageData *QImageData::create(uchar *data, int width, int height,  int bpl, QImage::Format format, bool readOnly)
{
    QImageData *d = 0;

    if (format == QImage::Format_Invalid)
        return d;

    if (!checkPixelSize(format)) {
        qWarning("QImageData::create(): Invalid pixel size for format %i",
                 format);
        return 0;
    }

    const int depth = qt_depthForFormat(format);
    const int calc_bytes_per_line = ((width * depth + 31)/32) * 4;
    const int min_bytes_per_line = (width * depth + 7)/8;

    if (bpl <= 0)
        bpl = calc_bytes_per_line;

    if (width <= 0 || height <= 0 || !data
        || INT_MAX/sizeof(uchar *) < uint(height)
        || INT_MAX/uint(depth) < uint(width)
        || bpl <= 0
        || height <= 0
        || bpl < min_bytes_per_line
        || INT_MAX/uint(bpl) < uint(height))
        return d;                                        // invalid parameter(s)

    d = new QImageData;
    d->ref.ref();

    d->own_data = false;
    d->ro_data = readOnly;
    d->data = data;
    d->width = width;
    d->height = height;
    d->depth = depth;
    d->format = format;

    d->bytes_per_line = bpl;
    d->nbytes = d->bytes_per_line * height;

    return d;
}

/*!
    Constructs an image with the given \a width, \a height and \a
    format, that uses an existing memory buffer, \a data. The \a width
    and \a height must be specified in pixels, \a data must be 32-bit aligned,
    and each scanline of data in the image must also be 32-bit aligned.

    The buffer must remain valid throughout the life of the
    QImage. The image does not delete the buffer at destruction.

    If \a format is an indexed color format, the image color table is
    initially empty and must be sufficiently expanded with
    setColorCount() or setColorTable() before the image is used.
*/
QImage::QImage(uchar* data, int width, int height, Format format)
    : QPaintDevice()
{
    d = QImageData::create(data, width, height, 0, format, false);
}

/*!
    Constructs an image with the given \a width, \a height and \a
    format, that uses an existing read-only memory buffer, \a
    data. The \a width and \a height must be specified in pixels, \a
    data must be 32-bit aligned, and each scanline of data in the
    image must also be 32-bit aligned.

    The buffer must remain valid throughout the life of the QImage and
    all copies that have not been modified or otherwise detached from
    the original buffer. The image does not delete the buffer at
    destruction.

    If \a format is an indexed color format, the image color table is
    initially empty and must be sufficiently expanded with
    setColorCount() or setColorTable() before the image is used.

    Unlike the similar QImage constructor that takes a non-const data buffer,
    this version will never alter the contents of the buffer.  For example,
    calling QImage::bits() will return a deep copy of the image, rather than
    the buffer passed to the constructor.  This allows for the efficiency of
    constructing a QImage from raw data, without the possibility of the raw
    data being changed.
*/
QImage::QImage(const uchar* data, int width, int height, Format format)
    : QPaintDevice()
{
    d = QImageData::create(const_cast<uchar*>(data), width, height, 0, format, true);
}

/*!
    Constructs an image with the given \a width, \a height and \a
    format, that uses an existing memory buffer, \a data. The \a width
    and \a height must be specified in pixels. \a bytesPerLine
    specifies the number of bytes per line (stride).

    The buffer must remain valid throughout the life of the
    QImage. The image does not delete the buffer at destruction.

    If \a format is an indexed color format, the image color table is
    initially empty and must be sufficiently expanded with
    setColorCount() or setColorTable() before the image is used.
*/
QImage::QImage(uchar *data, int width, int height, int bytesPerLine, Format format)
    :QPaintDevice()
{
    d = QImageData::create(data, width, height, bytesPerLine, format, false);
}


/*!
    Constructs an image with the given \a width, \a height and \a
    format, that uses an existing memory buffer, \a data. The \a width
    and \a height must be specified in pixels. \a bytesPerLine
    specifies the number of bytes per line (stride).

    The buffer must remain valid throughout the life of the
    QImage. The image does not delete the buffer at destruction.

    If \a format is an indexed color format, the image color table is
    initially empty and must be sufficiently expanded with
    setColorCount() or setColorTable() before the image is used.

    Unlike the similar QImage constructor that takes a non-const data buffer,
    this version will never alter the contents of the buffer.  For example,
    calling QImage::bits() will return a deep copy of the image, rather than
    the buffer passed to the constructor.  This allows for the efficiency of
    constructing a QImage from raw data, without the possibility of the raw
    data being changed.
*/

QImage::QImage(const uchar *data, int width, int height, int bytesPerLine, Format format)
    :QPaintDevice()
{
    d = QImageData::create(const_cast<uchar*>(data), width, height, bytesPerLine, format, true);
}

/*!
    Constructs an image and tries to load the image from the file with
    the given \a fileName.

    The loader attempts to read the image using the specified \a
    format. If the \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.

    If the loading of the image failed, this object is a null image.

    The file name can either refer to an actual file on disk or to one
    of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    \sa isNull(), {QImage#Reading and Writing Image Files}{Reading and Writing Image Files}
*/

QImage::QImage(const QString &fileName, const char *format)
    : QPaintDevice()
{
    d = 0;
    load(fileName, format);
}

/*!
    Constructs an image and tries to load the image from the file with
    the given \a fileName.

    The loader attempts to read the image using the specified \a
    format. If the \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.

    If the loading of the image failed, this object is a null image.

    The file name can either refer to an actual file on disk or to one
    of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This can
    be useful, for example, if you want to ensure that all
    user-visible strings go through QObject::tr().

    \sa QString::fromAscii(), isNull(), {QImage#Reading and Writing
    Image Files}{Reading and Writing Image Files}
*/
#ifndef QT_NO_CAST_FROM_ASCII
QImage::QImage(const char *fileName, const char *format)
    : QPaintDevice()
{
    // ### Qt 5: if you remove the QImage(const QByteArray &) QT3_SUPPORT
    // constructor, remove this constructor as well. The constructor here
    // exists so that QImage("foo.png") compiles without ambiguity.
    d = 0;
    load(QString::fromAscii(fileName), format);
}
#endif

#ifndef QT_NO_IMAGEFORMAT_XPM
extern bool qt_read_xpm_image_or_array(QIODevice *device, const char * const *source, QImage &image);

/*!
    Constructs an image from the given \a xpm image.

    Make sure that the image is a valid XPM image. Errors are silently
    ignored.

    Note that it's possible to squeeze the XPM variable a little bit
    by using an unusual declaration:

    \snippet doc/src/snippets/code/src_gui_image_qimage.cpp 2

    The extra \c const makes the entire definition read-only, which is
    slightly more efficient (e.g., when the code is in a shared
    library) and able to be stored in ROM with the application.
*/

QImage::QImage(const char * const xpm[])
    : QPaintDevice()
{
    d = 0;
    if (!xpm)
        return;
    if (!qt_read_xpm_image_or_array(0, xpm, *this))
        // Issue: Warning because the constructor may be ambigious
        qWarning("QImage::QImage(), XPM is not supported");
}
#endif // QT_NO_IMAGEFORMAT_XPM

/*!
    \fn QImage::QImage(const QByteArray &data)

    Use the static fromData() function instead.

    \oldcode
        QByteArray data;
        ...
        QImage image(data);
    \newcode
        QByteArray data;
        ...
        QImage image = QImage::fromData(data);
    \endcode
*/


/*!
    Constructs a shallow copy of the given \a image.

    For more information about shallow copies, see the \l {Implicit
    Data Sharing} documentation.

    \sa copy()
*/

QImage::QImage(const QImage &image)
    : QPaintDevice()
{
    if (image.paintingActive()) {
        d = 0;
        operator=(image.copy());
    } else {
        d = image.d;
        if (d)
            d->ref.ref();
    }
}

#ifdef QT3_SUPPORT
/*!
    \fn QImage::QImage(int width, int height, int depth, int numColors, Endian bitOrder)

    Constructs an image with the given \a width, \a height, \a depth,
    \a numColors colors and \a bitOrder.

    Use the constructor that accepts a width, a height and a format
    (i.e. specifying the depth and bit order), in combination with the
    setColorCount() function, instead.

    \oldcode
        QImage image(width, height, depth, numColors);
    \newcode
        QImage image(width, height, format);

        // For 8 bit images the default number of colors is 256. If
        // another number of colors is required it can be specified
        // using the setColorCount() function.
        image.setColorCount(numColors);
    \endcode
*/

QImage::QImage(int w, int h, int depth, int colorCount, Endian bitOrder)
    : QPaintDevice()
{
    d = QImageData::create(QSize(w, h), formatFor(depth, bitOrder), colorCount);
}

/*!
    Constructs an image with the given \a size, \a depth, \a numColors
    and \a bitOrder.

    Use the constructor that accepts a size and a format
    (i.e. specifying the depth and bit order), in combination with the
    setColorCount() function, instead.

    \oldcode
        QSize mySize(width, height);
        QImage image(mySize, depth, numColors);
    \newcode
        QSize mySize(width, height);
        QImage image(mySize, format);

        // For 8 bit images the default number of colors is 256. If
        // another number of colors is required it can be specified
        // using the setColorCount() function.
        image.setColorCount(numColors);
    \endcode
*/
QImage::QImage(const QSize& size, int depth, int numColors, Endian bitOrder)
    : QPaintDevice()
{
    d = QImageData::create(size, formatFor(depth, bitOrder), numColors);
}

/*!
    \fn QImage::QImage(uchar* data, int width, int height, int depth, const QRgb* colortable, int numColors, Endian bitOrder)

    Constructs an image with the given \a width, \a height, depth, \a
    colortable, \a numColors and \a bitOrder, that uses an existing
    memory buffer, \a data.

    Use the constructor that accepts a uchar pointer, a width, a
    height and a format (i.e. specifying the depth and bit order), in
    combination with the setColorTable() function, instead.

    \oldcode
        uchar *myData;
        QRgb *myColorTable;

        QImage image(myData, width, height, depth,
                               myColorTable, numColors, IgnoreEndian);
    \newcode
        uchar *myData;
        QVector<QRgb> myColorTable;

        QImage image(myData, width, height, format);
        image.setColorTable(myColorTable);
    \endcode
*/
QImage::QImage(uchar* data, int w, int h, int depth, const QRgb* colortable, int numColors, Endian bitOrder)
    : QPaintDevice()
{
    d = 0;
    Format f = formatFor(depth, bitOrder);
    if (f == Format_Invalid)
        return;

    const int bytes_per_line = ((w*depth+31)/32)*4;        // bytes per scanline
    if (w <= 0 || h <= 0 || numColors < 0 || !data
        || INT_MAX/sizeof(uchar *) < uint(h)
        || INT_MAX/uint(depth) < uint(w)
        || bytes_per_line <= 0
        || INT_MAX/uint(bytes_per_line) < uint(h))
        return;                                        // invalid parameter(s)
    d = new QImageData;
    d->ref.ref();

    d->own_data = false;
    d->data = data;
    d->width = w;
    d->height = h;
    d->depth = depth;
    d->format = f;
    if (depth == 32)
        numColors = 0;

    d->bytes_per_line = bytes_per_line;
    d->nbytes = d->bytes_per_line * h;
    if (colortable) {
        d->colortable.resize(numColors);
        for (int i = 0; i < numColors; ++i)
            d->colortable[i] = colortable[i];
    } else if (numColors) {
        setColorCount(numColors);
    }
}

#ifdef Q_WS_QWS

/*!
    \fn QImage::QImage(uchar* data, int width, int height, int depth, int bytesPerLine, const QRgb* colortable, int numColors, Endian bitOrder)

    Constructs an image with the given \a width, \a height, \a depth,
    \a bytesPerLine, \a colortable, \a numColors and \a bitOrder, that
    uses an existing memory buffer, \a data. The image does not delete
    the buffer at destruction.

    \warning This constructor is only available in Qt for Embedded Linux.

    The data has to be 32-bit aligned, and each scanline of data in the image
    must also be 32-bit aligned, so it's no longer possible to specify a custom
    \a bytesPerLine value.
*/
QImage::QImage(uchar* data, int w, int h, int depth, int bpl, const QRgb* colortable, int numColors, Endian bitOrder)
    : QPaintDevice()
{
    d = 0;
    Format f = formatFor(depth, bitOrder);
    if (f == Format_Invalid)
        return;
    if (!data || w <= 0 || h <= 0 || depth <= 0 || numColors < 0
        || INT_MAX/sizeof(uchar *) < uint(h)
        || INT_MAX/uint(depth) < uint(w)
        || bpl <= 0
        || INT_MAX/uint(bpl) < uint(h))
        return;                                        // invalid parameter(s)

    d = new QImageData;
    d->ref.ref();
    d->own_data = false;
    d->data = data;
    d->width = w;
    d->height = h;
    d->depth = depth;
    d->format = f;
    if (depth == 32)
        numColors = 0;
    d->bytes_per_line = bpl;
    d->nbytes = d->bytes_per_line * h;
    if (colortable) {
        d->colortable.resize(numColors);
        for (int i = 0; i < numColors; ++i)
            d->colortable[i] = colortable[i];
    } else if (numColors) {
        setColorCount(numColors);
    }
}
#endif // Q_WS_QWS
#endif // QT3_SUPPORT

/*!
    Destroys the image and cleans up.
*/

QImage::~QImage()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Assigns a shallow copy of the given \a image to this image and
    returns a reference to this image.

    For more information about shallow copies, see the \l {Implicit
    Data Sharing} documentation.

    \sa copy(), QImage()
*/

QImage &QImage::operator=(const QImage &image)
{
    if (image.paintingActive()) {
        operator=(image.copy());
    } else {
        if (image.d)
            image.d->ref.ref();
        if (d && !d->ref.deref())
            delete d;
        d = image.d;
    }
    return *this;
}

/*!
    \fn void QImage::swap(QImage &other)
    \since 4.8

    Swaps image \a other with this image. This operation is very
    fast and never fails.
*/

/*!
  \internal
*/
int QImage::devType() const
{
    return QInternal::Image;
}

/*!
   Returns the image as a QVariant.
*/
QImage::operator QVariant() const
{
    return QVariant(QVariant::Image, this);
}

/*!
    \internal

    If multiple images share common data, this image makes a copy of
    the data and detaches itself from the sharing mechanism, making
    sure that this image is the only one referring to the data.

    Nothing is done if there is just a single reference.

    \sa copy(), isDetached(), {Implicit Data Sharing}
*/
void QImage::detach()
{
    if (d) {
        if (d->is_cached && d->ref == 1)
            QImagePixmapCleanupHooks::executeImageHooks(cacheKey());

        if (d->ref != 1 || d->ro_data)
            *this = copy();

        if (d)
            ++d->detach_no;
    }
}


/*!
    \fn QImage QImage::copy(int x, int y, int width, int height) const
    \overload

    The returned image is copied from the position (\a x, \a y) in
    this image, and will always have the given \a width and \a height.
    In areas beyond this image, pixels are set to 0.

*/

/*!
    \fn QImage QImage::copy(const QRect& rectangle) const

    Returns a sub-area of the image as a new image.

    The returned image is copied from the position (\a
    {rectangle}.x(), \a{rectangle}.y()) in this image, and will always
    have the size of the given \a rectangle.

    In areas beyond this image, pixels are set to 0. For 32-bit RGB
    images, this means black; for 32-bit ARGB images, this means
    transparent black; for 8-bit images, this means the color with
    index 0 in the color table which can be anything; for 1-bit
    images, this means Qt::color0.

    If the given \a rectangle is a null rectangle the entire image is
    copied.

    \sa QImage()
*/
QImage QImage::copy(const QRect& r) const
{
    if (!d)
        return QImage();

    if (r.isNull()) {
        QImage image(d->width, d->height, d->format);
        if (image.isNull())
            return image;

        // Qt for Embedded Linux can create images with non-default bpl
        // make sure we don't crash.
        if (image.d->nbytes != d->nbytes) {
            int bpl = image.bytesPerLine();
            for (int i = 0; i < height(); i++)
                memcpy(image.scanLine(i), scanLine(i), bpl);
        } else
            memcpy(image.bits(), bits(), d->nbytes);
        image.d->colortable = d->colortable;
        image.d->dpmx = d->dpmx;
        image.d->dpmy = d->dpmy;
        image.d->offset = d->offset;
        image.d->has_alpha_clut = d->has_alpha_clut;
#ifndef QT_NO_IMAGE_TEXT
        image.d->text = d->text;
#endif
        return image;
    }

    int x = r.x();
    int y = r.y();
    int w = r.width();
    int h = r.height();

    int dx = 0;
    int dy = 0;
    if (w <= 0 || h <= 0)
        return QImage();

    QImage image(w, h, d->format);
    if (image.isNull())
        return image;

    if (x < 0 || y < 0 || x + w > d->width || y + h > d->height) {
        // bitBlt will not cover entire image - clear it.
        image.fill(0);
        if (x < 0) {
            dx = -x;
            x = 0;
        }
        if (y < 0) {
            dy = -y;
            y = 0;
        }
    }

    image.d->colortable = d->colortable;

    int pixels_to_copy = qMax(w - dx, 0);
    if (x > d->width)
        pixels_to_copy = 0;
    else if (pixels_to_copy > d->width - x)
        pixels_to_copy = d->width - x;
    int lines_to_copy = qMax(h - dy, 0);
    if (y > d->height)
        lines_to_copy = 0;
    else if (lines_to_copy > d->height - y)
        lines_to_copy = d->height - y;

    bool byteAligned = true;
    if (d->format == Format_Mono || d->format == Format_MonoLSB)
        byteAligned = !(dx & 7) && !(x & 7) && !(pixels_to_copy & 7);

    if (byteAligned) {
        const uchar *src = d->data + ((x * d->depth) >> 3) + y * d->bytes_per_line;
        uchar *dest = image.d->data + ((dx * d->depth) >> 3) + dy * image.d->bytes_per_line;
        const int bytes_to_copy = (pixels_to_copy * d->depth) >> 3;
        for (int i = 0; i < lines_to_copy; ++i) {
            memcpy(dest, src, bytes_to_copy);
            src += d->bytes_per_line;
            dest += image.d->bytes_per_line;
        }
    } else if (d->format == Format_Mono) {
        const uchar *src = d->data + y * d->bytes_per_line;
        uchar *dest = image.d->data + dy * image.d->bytes_per_line;
        for (int i = 0; i < lines_to_copy; ++i) {
            for (int j = 0; j < pixels_to_copy; ++j) {
                if (src[(x + j) >> 3] & (0x80 >> ((x + j) & 7)))
                    dest[(dx + j) >> 3] |= (0x80 >> ((dx + j) & 7));
                else
                    dest[(dx + j) >> 3] &= ~(0x80 >> ((dx + j) & 7));
            }
            src += d->bytes_per_line;
            dest += image.d->bytes_per_line;
        }
    } else { // Format_MonoLSB
        Q_ASSERT(d->format == Format_MonoLSB);
        const uchar *src = d->data + y * d->bytes_per_line;
        uchar *dest = image.d->data + dy * image.d->bytes_per_line;
        for (int i = 0; i < lines_to_copy; ++i) {
            for (int j = 0; j < pixels_to_copy; ++j) {
                if (src[(x + j) >> 3] & (0x1 << ((x + j) & 7)))
                    dest[(dx + j) >> 3] |= (0x1 << ((dx + j) & 7));
                else
                    dest[(dx + j) >> 3] &= ~(0x1 << ((dx + j) & 7));
            }
            src += d->bytes_per_line;
            dest += image.d->bytes_per_line;
        }
    }

    image.d->dpmx = dotsPerMeterX();
    image.d->dpmy = dotsPerMeterY();
    image.d->offset = offset();
    image.d->has_alpha_clut = d->has_alpha_clut;
#ifndef QT_NO_IMAGE_TEXT
    image.d->text = d->text;
#endif
    return image;
}


/*!
    \fn bool QImage::isNull() const

    Returns true if it is a null image, otherwise returns false.

    A null image has all parameters set to zero and no allocated data.
*/
bool QImage::isNull() const
{
    return !d;
}

/*!
    \fn int QImage::width() const

    Returns the width of the image.

    \sa {QImage#Image Information}{Image Information}
*/
int QImage::width() const
{
    return d ? d->width : 0;
}

/*!
    \fn int QImage::height() const

    Returns the height of the image.

    \sa {QImage#Image Information}{Image Information}
*/
int QImage::height() const
{
    return d ? d->height : 0;
}

/*!
    \fn QSize QImage::size() const

    Returns the size of the image, i.e. its width() and height().

    \sa {QImage#Image Information}{Image Information}
*/
QSize QImage::size() const
{
    return d ? QSize(d->width, d->height) : QSize(0, 0);
}

/*!
    \fn QRect QImage::rect() const

    Returns the enclosing rectangle (0, 0, width(), height()) of the
    image.

    \sa {QImage#Image Information}{Image Information}
*/
QRect QImage::rect() const
{
    return d ? QRect(0, 0, d->width, d->height) : QRect();
}

/*!
    Returns the depth of the image.

    The image depth is the number of bits used to store a single
    pixel, also called bits per pixel (bpp).

    The supported depths are 1, 8, 16, 24 and 32.

    \sa bitPlaneCount(), convertToFormat(), {QImage#Image Formats}{Image Formats},
    {QImage#Image Information}{Image Information}

*/
int QImage::depth() const
{
    return d ? d->depth : 0;
}

/*!
    \obsolete
    \fn int QImage::numColors() const

    Returns the size of the color table for the image.

    \sa setColorCount()
*/
int QImage::numColors() const
{
    return d ? d->colortable.size() : 0;
}

/*!
    \since 4.6
    \fn int QImage::colorCount() const

    Returns the size of the color table for the image.

    Notice that colorCount() returns 0 for 32-bpp images because these
    images do not use color tables, but instead encode pixel values as
    ARGB quadruplets.

    \sa setColorCount(), {QImage#Image Information}{Image Information}
*/
int QImage::colorCount() const
{
    return d ? d->colortable.size() : 0;
}


#ifdef QT3_SUPPORT
/*!
    \fn QImage::Endian QImage::bitOrder() const

    Returns the bit order for the image. If it is a 1-bpp image, this
    function returns either QImage::BigEndian or
    QImage::LittleEndian. Otherwise, this function returns
    QImage::IgnoreEndian.

    Use the format() function instead for the monochrome formats. For
    non-monochrome formats the bit order is irrelevant.
*/

/*!
    Returns a pointer to the scanline pointer table. This is the
    beginning of the data block for the image.
    Returns 0 in case of an error.

    Use the bits() or scanLine() function instead.
*/
uchar **QImage::jumpTable()
{
    if (!d)
        return 0;
    detach();

    // in case detach() ran out of memory..
    if (!d)
        return 0;

    if (!d->jumptable) {
        d->jumptable = (uchar **)malloc(d->height*sizeof(uchar *));
        if (!d->jumptable)
            return 0;
        uchar *data = d->data;
        int height = d->height;
        uchar **p = d->jumptable;
        while (height--) {
            *p++ = data;
            data += d->bytes_per_line;
        }
    }
    return d->jumptable;
}

/*!
    \overload
*/
const uchar * const *QImage::jumpTable() const
{
    if (!d)
        return 0;
    if (!d->jumptable) {
        d->jumptable = (uchar **)malloc(d->height*sizeof(uchar *));
        if (!d->jumptable)
            return 0;
        uchar *data = d->data;
        int height = d->height;
        uchar **p = d->jumptable;
        while (height--) {
            *p++ = data;
            data += d->bytes_per_line;
        }
    }
    return d->jumptable;
}
#endif

/*!
    Sets the color table used to translate color indexes to QRgb
    values, to the specified \a colors.

    When the image is used, the color table must be large enough to
    have entries for all the pixel/index values present in the image,
    otherwise the results are undefined.

    \sa colorTable(), setColor(), {QImage#Image Transformations}{Image
    Transformations}
*/
void QImage::setColorTable(const QVector<QRgb> colors)
{
    if (!d)
        return;
    detach();

    // In case detach() ran out of memory
    if (!d)
        return;

    d->colortable = colors;
    d->has_alpha_clut = false;
    for (int i = 0; i < d->colortable.size(); ++i) {
        if (qAlpha(d->colortable.at(i)) != 255) {
            d->has_alpha_clut = true;
            break;
        }
    }
}

/*!
    Returns a list of the colors contained in the image's color table,
    or an empty list if the image does not have a color table

    \sa setColorTable(), colorCount(), color()
*/
QVector<QRgb> QImage::colorTable() const
{
    return d ? d->colortable : QVector<QRgb>();
}


/*!
    \obsolete
    Returns the number of bytes occupied by the image data.

    \sa byteCount()
*/
int QImage::numBytes() const
{
    return d ? d->nbytes : 0;
}

/*!
    \since 4.6
    Returns the number of bytes occupied by the image data.

    \sa bytesPerLine(), bits(), {QImage#Image Information}{Image
    Information}
*/
int QImage::byteCount() const
{
    return d ? d->nbytes : 0;
}

/*!
    Returns the number of bytes per image scanline.

    This is equivalent to byteCount() / height().

    \sa scanLine()
*/
int QImage::bytesPerLine() const
{
    return (d && d->height) ? d->nbytes / d->height : 0;
}


/*!
    Returns the color in the color table at index \a i. The first
    color is at index 0.

    The colors in an image's color table are specified as ARGB
    quadruplets (QRgb). Use the qAlpha(), qRed(), qGreen(), and
    qBlue() functions to get the color value components.

    \sa setColor(), pixelIndex(), {QImage#Pixel Manipulation}{Pixel
    Manipulation}
*/
QRgb QImage::color(int i) const
{
    Q_ASSERT(i < colorCount());
    return d ? d->colortable.at(i) : QRgb(uint(-1));
}

/*!
    \fn void QImage::setColor(int index, QRgb colorValue)

    Sets the color at the given \a index in the color table, to the
    given to \a colorValue. The color value is an ARGB quadruplet.

    If \a index is outside the current size of the color table, it is
    expanded with setColorCount().

    \sa color(), colorCount(), setColorTable(), {QImage#Pixel Manipulation}{Pixel
    Manipulation}
*/
void QImage::setColor(int i, QRgb c)
{
    if (!d)
        return;
    if (i < 0 || d->depth > 8 || i >= 1<<d->depth) {
        qWarning("QImage::setColor: Index out of bound %d", i);
        return;
    }
    detach();

    // In case detach() run out of memory
    if (!d)
        return;

    if (i >= d->colortable.size())
        setColorCount(i+1);
    d->colortable[i] = c;
    d->has_alpha_clut |= (qAlpha(c) != 255);
}

/*!
    Returns a pointer to the pixel data at the scanline with index \a
    i. The first scanline is at index 0.

    The scanline data is aligned on a 32-bit boundary.

    \warning If you are accessing 32-bpp image data, cast the returned
    pointer to \c{QRgb*} (QRgb has a 32-bit size) and use it to
    read/write the pixel value. You cannot use the \c{uchar*} pointer
    directly, because the pixel format depends on the byte order on
    the underlying platform. Use qRed(), qGreen(), qBlue(), and
    qAlpha() to access the pixels.

    \sa bytesPerLine(), bits(), {QImage#Pixel Manipulation}{Pixel
    Manipulation}, constScanLine()
*/
uchar *QImage::scanLine(int i)
{
    if (!d)
        return 0;

    detach();

    // In case detach() ran out of memory
    if (!d)
        return 0;

    return d->data + i * d->bytes_per_line;
}

/*!
    \overload
*/
const uchar *QImage::scanLine(int i) const
{
    if (!d)
        return 0;

    Q_ASSERT(i >= 0 && i < height());
    return d->data + i * d->bytes_per_line;
}


/*!
    Returns a pointer to the pixel data at the scanline with index \a
    i. The first scanline is at index 0.

    The scanline data is aligned on a 32-bit boundary.

    Note that QImage uses \l{Implicit Data Sharing} {implicit data
    sharing}, but this function does \e not perform a deep copy of the
    shared pixel data, because the returned data is const.

    \sa scanLine(), constBits()
    \since 4.7
*/
const uchar *QImage::constScanLine(int i) const
{
    if (!d)
        return 0;

    Q_ASSERT(i >= 0 && i < height());
    return d->data + i * d->bytes_per_line;
}

/*!
    Returns a pointer to the first pixel data. This is equivalent to
    scanLine(0).

    Note that QImage uses \l{Implicit Data Sharing} {implicit data
    sharing}. This function performs a deep copy of the shared pixel
    data, thus ensuring that this QImage is the only one using the
    current return value.

    \sa scanLine(), byteCount(), constBits()
*/
uchar *QImage::bits()
{
    if (!d)
        return 0;
    detach();

    // In case detach ran out of memory...
    if (!d)
        return 0;

    return d->data;
}

/*!
    \overload

    Note that QImage uses \l{Implicit Data Sharing} {implicit data
    sharing}, but this function does \e not perform a deep copy of the
    shared pixel data, because the returned data is const.
*/
const uchar *QImage::bits() const
{
    return d ? d->data : 0;
}


/*!
    Returns a pointer to the first pixel data.

    Note that QImage uses \l{Implicit Data Sharing} {implicit data
    sharing}, but this function does \e not perform a deep copy of the
    shared pixel data, because the returned data is const.

    \sa bits(), constScanLine()
    \since 4.7
*/
const uchar *QImage::constBits() const
{
    return d ? d->data : 0;
}

/*!
    \fn void QImage::reset()

    Resets all image parameters and deallocates the image data.

    Assign a null image instead.

    \oldcode
        QImage image;
        image.reset();
    \newcode
        QImage image;
        image = QImage();
    \endcode
*/

/*!
    \fn void QImage::fill(uint pixelValue)

    Fills the entire image with the given \a pixelValue.

    If the depth of this image is 1, only the lowest bit is used. If
    you say fill(0), fill(2), etc., the image is filled with 0s. If
    you say fill(1), fill(3), etc., the image is filled with 1s. If
    the depth is 8, the lowest 8 bits are used and if the depth is 16
    the lowest 16 bits are used.

    Note: QImage::pixel() returns the color of the pixel at the given
    coordinates while QColor::pixel() returns the pixel value of the
    underlying window system (essentially an index value), so normally
    you will want to use QImage::pixel() to use a color from an
    existing image or QColor::rgb() to use a specific color.

    \sa depth(), {QImage#Image Transformations}{Image Transformations}
*/

void QImage::fill(uint pixel)
{
    if (!d)
        return;

    detach();

    // In case detach() ran out of memory
    if (!d)
        return;

    if (d->depth == 1 || d->depth == 8) {
        int w = d->width;
        if (d->depth == 1) {
            if (pixel & 1)
                pixel = 0xffffffff;
            else
                pixel = 0;
            w = (w + 7) / 8;
        } else {
            pixel &= 0xff;
        }
        qt_rectfill<quint8>(d->data, pixel, 0, 0,
                            w, d->height, d->bytes_per_line);
        return;
    } else if (d->depth == 16) {
        qt_rectfill<quint16>(reinterpret_cast<quint16*>(d->data), pixel,
                             0, 0, d->width, d->height, d->bytes_per_line);
        return;
    } else if (d->depth == 24) {
        qt_rectfill<quint24>(reinterpret_cast<quint24*>(d->data), pixel,
                             0, 0, d->width, d->height, d->bytes_per_line);
        return;
    }

    if (d->format == Format_RGB32)
        pixel |= 0xff000000;

    qt_rectfill<uint>(reinterpret_cast<uint*>(d->data), pixel,
                      0, 0, d->width, d->height, d->bytes_per_line);
}


/*!
    \fn void QImage::fill(Qt::GlobalColor color)
    \overload
    \since 4.8

    Fills the image with the given \a color, described as a standard global
    color.
 */

void QImage::fill(Qt::GlobalColor color)
{
    fill(QColor(color));
}



/*!
    \fn void QImage::fill(const QColor &color)

    \overload

    Fills the entire image with the given \a color.

    If the depth of the image is 1, the image will be filled with 1 if
    \a color equals Qt::color1; it will otherwise be filled with 0.

    If the depth of the image is 8, the image will be filled with the
    index corresponding the \a color in the color table if present; it
    will otherwise be filled with 0.

    \since 4.8
*/

void QImage::fill(const QColor &color)
{
    if (!d)
        return;
    detach();

    // In case we run out of memory
    if (!d)
        return;

    if (d->depth == 32) {
        uint pixel = color.rgba();
        if (d->format == QImage::Format_ARGB32_Premultiplied)
            pixel = PREMUL(pixel);
        fill((uint) pixel);

    } else if (d->depth == 16 && d->format == QImage::Format_RGB16) {
        qrgb565 p(color.rgba());
        fill((uint) p.rawValue());

    } else if (d->depth == 1) {
        if (color == Qt::color1)
            fill((uint) 1);
        else
            fill((uint) 0);

    } else if (d->depth == 8) {
        uint pixel = 0;
        for (int i=0; i<d->colortable.size(); ++i) {
            if (color.rgba() == d->colortable.at(i)) {
                pixel = i;
                break;
            }
        }
        fill(pixel);

    } else {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(rect(), color);
    }

}






/*!
    Inverts all pixel values in the image.

    The given invert \a mode only have a meaning when the image's
    depth is 32. The default \a mode is InvertRgb, which leaves the
    alpha channel unchanged. If the \a mode is InvertRgba, the alpha
    bits are also inverted.

    Inverting an 8-bit image means to replace all pixels using color
    index \e i with a pixel using color index 255 minus \e i. The same
    is the case for a 1-bit image. Note that the color table is \e not
    changed.

    \sa {QImage#Image Transformations}{Image Transformations}
*/

void QImage::invertPixels(InvertMode mode)
{
    if (!d)
        return;

    detach();

    // In case detach() ran out of memory
    if (!d)
        return;

    if (depth() != 32) {
        // number of used bytes pr line
        int bpl = (d->width * d->depth + 7) / 8;
        int pad = d->bytes_per_line - bpl;
        uchar *sl = d->data;
        for (int y=0; y<d->height; ++y) {
            for (int x=0; x<bpl; ++x)
                *sl++ ^= 0xff;
            sl += pad;
        }
    } else {
        quint32 *p = (quint32*)d->data;
        quint32 *end = (quint32*)(d->data + d->nbytes);
        uint xorbits = (mode == InvertRgba) ? 0xffffffff : 0x00ffffff;
        while (p < end)
            *p++ ^= xorbits;
    }
}

/*!
    \fn void QImage::invertPixels(bool invertAlpha)

    Use the invertPixels() function that takes a QImage::InvertMode
    parameter instead.
*/

/*! \fn QImage::Endian QImage::systemByteOrder()

    Determines the host computer byte order. Returns
    QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).

    This function is no longer relevant for QImage. Use QSysInfo
    instead.
*/

// Windows defines these
#if defined(write)
# undef write
#endif
#if defined(close)
# undef close
#endif
#if defined(read)
# undef read
#endif

/*!
    \obsolete
    Resizes the color table to contain \a numColors entries.

    \sa setColorCount()
*/

void QImage::setNumColors(int numColors)
{
    setColorCount(numColors);
}

/*!
    \since 4.6
    Resizes the color table to contain \a colorCount entries.

    If the color table is expanded, all the extra colors will be set to
    transparent (i.e qRgba(0, 0, 0, 0)).

    When the image is used, the color table must be large enough to
    have entries for all the pixel/index values present in the image,
    otherwise the results are undefined.

    \sa colorCount(), colorTable(), setColor(), {QImage#Image
    Transformations}{Image Transformations}
*/

void QImage::setColorCount(int colorCount)
{
    if (!d) {
        qWarning("QImage::setColorCount: null image");
        return;
    }

    detach();

    // In case detach() ran out of memory
    if (!d)
        return;

    if (colorCount == d->colortable.size())
        return;
    if (colorCount <= 0) {                        // use no color table
        d->colortable = QVector<QRgb>();
        return;
    }
    int nc = d->colortable.size();
    d->colortable.resize(colorCount);
    for (int i = nc; i < colorCount; ++i)
        d->colortable[i] = 0;
}

/*!
    Returns the format of the image.

    \sa {QImage#Image Formats}{Image Formats}
*/
QImage::Format QImage::format() const
{
    return d ? d->format : Format_Invalid;
}


#ifdef QT3_SUPPORT
/*!
    Returns true if alpha buffer mode is enabled; otherwise returns
    false.

    Use the hasAlphaChannel() function instead.

*/
bool QImage::hasAlphaBuffer() const
{
    if (!d)
        return false;

    switch (d->format) {
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
    case Format_ARGB8565_Premultiplied:
    case Format_ARGB8555_Premultiplied:
    case Format_ARGB6666_Premultiplied:
    case Format_ARGB4444_Premultiplied:
        return true;
    default:
        return false;
    }
}

/*!
    Enables alpha buffer mode if \a enable is true, otherwise disables
    it. The alpha buffer is used to set a mask when a QImage is
    translated to a QPixmap.

    If a monochrome or indexed 8-bit image has alpha channels in their
    color tables they will automatically detect that they have an
    alpha channel, so this function is not required.  To force alpha
    channels on 32-bit images, use the convertToFormat() function.
*/

void QImage::setAlphaBuffer(bool enable)
{
    if (!d
        || d->format == Format_Mono
        || d->format == Format_MonoLSB
        || d->format == Format_Indexed8)
        return;
    if (enable && (d->format == Format_ARGB32 ||
                   d->format == Format_ARGB32_Premultiplied ||
                   d->format == Format_ARGB8565_Premultiplied ||
                   d->format == Format_ARGB6666_Premultiplied ||
                   d->format == Format_ARGB8555_Premultiplied ||
                   d->format == Format_ARGB4444_Premultiplied))
    {
        return;
    }
    if (!enable && (d->format == Format_RGB32 ||
                    d->format == Format_RGB555 ||
                    d->format == Format_RGB666 ||
                    d->format == Format_RGB888 ||
                    d->format == Format_RGB444))
    {
        return;
    }
    detach();
    d->format = (enable ? Format_ARGB32 : Format_RGB32);
}


/*!
  \fn bool QImage::create(int width, int height, int depth, int numColors, Endian bitOrder)

    Sets the image \a width, \a height, \a depth, its number of colors
    (in \a numColors), and bit order. Returns true if successful, or
    false if the parameters are incorrect or if memory cannot be
    allocated.

    The \a width and \a height is limited to 32767. \a depth must be
    1, 8, or 32. If \a depth is 1, \a bitOrder must be set to
    either QImage::LittleEndian or QImage::BigEndian. For other depths
    \a bitOrder must be QImage::IgnoreEndian.

    This function allocates a color table and a buffer for the image
    data. The image data is not initialized. The image buffer is
    allocated as a single block that consists of a table of scanLine()
    pointers (jumpTable()) and the image data (bits()).

    Use a QImage constructor instead.
*/
bool QImage::create(int width, int height, int depth, int numColors, Endian bitOrder)
{
    if (d && !d->ref.deref())
        delete d;
    d = QImageData::create(QSize(width, height), formatFor(depth, bitOrder), numColors);
    return true;
}

/*!
    \fn bool QImage::create(const QSize& size, int depth, int numColors, Endian bitOrder)
    \overload

    The width and height are specified in the \a size argument.

    Use a QImage constructor instead.
*/
bool QImage::create(const QSize& size, int depth, int numColors, QImage::Endian bitOrder)
{
    if (d && !d->ref.deref())
        delete d;
    d = QImageData::create(size, formatFor(depth, bitOrder), numColors);
    return true;
}
#endif // QT3_SUPPORT

/*****************************************************************************
  Internal routines for converting image depth.
 *****************************************************************************/

typedef void (*Image_Converter)(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);

typedef bool (*InPlace_Image_Converter)(QImageData *data, Qt::ImageConversionFlags);

static void convert_ARGB_to_ARGB_PM(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32);
    Q_ASSERT(dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = PREMUL(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static bool convert_ARGB_to_ARGB_PM_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_ARGB32);

    const int pad = (data->bytes_per_line >> 2) - data->width;
    QRgb *rgb_data = (QRgb *) data->data;

    for (int i = 0; i < data->height; ++i) {
        const QRgb *end = rgb_data + data->width;
        while (rgb_data < end) {
            *rgb_data = PREMUL(*rgb_data);
            ++rgb_data;
        }
        rgb_data += pad;
    }
    data->format = QImage::Format_ARGB32_Premultiplied;
    return true;
}

static bool convert_indexed8_to_ARGB_PM_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 32;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes; // end of src
    quint32 *dest_data = (quint32 *) (newData + nbytes); // end of dest > end of src
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 2) - width;
    if (data->colortable.size() == 0) {
        data->colortable.resize(256);
        for (int i = 0; i < 256; ++i)
            data->colortable[i] = qRgb(i, i, i);
    } else {
        for (int i = 0; i < data->colortable.size(); ++i)
            data->colortable[i] = PREMUL(data->colortable.at(i));

        // Fill the rest of the table in case src_data > colortable.size()
        const int oldSize = data->colortable.size();
        const QRgb lastColor = data->colortable.at(oldSize - 1);
        data->colortable.insert(oldSize, 256 - oldSize, lastColor);
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = data->colortable.at(*src_data);
        }
    }

    data->colortable = QVector<QRgb>();
    data->format = QImage::Format_ARGB32_Premultiplied;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_indexed8_to_RGB_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 32;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes;
    quint32 *dest_data = (quint32 *) (newData + nbytes);
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 2) - width;
    if (data->colortable.size() == 0) {
        data->colortable.resize(256);
        for (int i = 0; i < 256; ++i)
            data->colortable[i] = qRgb(i, i, i);
    } else {
        // Fill the rest of the table in case src_data > colortable.size()
        const int oldSize = data->colortable.size();
        const QRgb lastColor = data->colortable.at(oldSize - 1);
        data->colortable.insert(oldSize, 256 - oldSize, lastColor);
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = (quint32) data->colortable.at(*src_data);
        }
    }

    data->colortable = QVector<QRgb>();
    data->format = QImage::Format_RGB32;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_indexed8_to_RGB16_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 16;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes;
    quint16 *dest_data = (quint16 *) (newData + nbytes);
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 1) - width;

    quint16 colorTableRGB16[256];
    if (data->colortable.isEmpty()) {
        for (int i = 0; i < 256; ++i)
            colorTableRGB16[i] = qt_colorConvert<quint16, quint32>(qRgb(i, i, i), 0);
    } else {
        // 1) convert the existing colors to RGB16
        const int tableSize = data->colortable.size();
        for (int i = 0; i < tableSize; ++i)
            colorTableRGB16[i] = qt_colorConvert<quint16, quint32>(data->colortable.at(i), 0);
        data->colortable = QVector<QRgb>();

        // 2) fill the rest of the table in case src_data > colortable.size()
        const quint16 lastColor = colorTableRGB16[tableSize - 1];
        for (int i = tableSize; i < 256; ++i)
            colorTableRGB16[i] = lastColor;
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = colorTableRGB16[*src_data];
        }
    }

    data->format = QImage::Format_RGB16;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_RGB_to_RGB16_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_RGB32);
    const int depth = 16;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int src_bytes_per_line = data->bytes_per_line;
    quint32 *src_data = (quint32 *) data->data;
    quint16 *dst_data = (quint16 *) data->data;

    for (int i = 0; i < data->height; ++i) {
        qt_memconvert(dst_data, src_data, data->width);
        src_data = (quint32 *) (((char*)src_data) + src_bytes_per_line);
        dst_data = (quint16 *) (((char*)dst_data) + dst_bytes_per_line);
    }
    data->format = QImage::Format_RGB16;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, data->nbytes);
    if (newData) {
        data->data = newData;
        return true;
    } else {
        return false;
    }
}

static void convert_ARGB_PM_to_ARGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_ARGB32);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = INV_PREMUL(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_ARGB_PM_to_RGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGB32);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = 0xff000000 | INV_PREMUL(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void swap_bit_order(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_Mono || dest->format == QImage::Format_MonoLSB);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);
    Q_ASSERT(src->nbytes == dest->nbytes);
    Q_ASSERT(src->bytes_per_line == dest->bytes_per_line);

    dest->colortable = src->colortable;

    const uchar *src_data = src->data;
    const uchar *end = src->data + src->nbytes;
    uchar *dest_data = dest->data;
    while (src_data < end) {
        *dest_data = bitflip[*src_data];
        ++src_data;
        ++dest_data;
    }
}

static void mask_alpha_converter(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const uint *src_data = (const uint *)src->data;
    uint *dest_data = (uint *)dest->data;

    for (int i = 0; i < src->height; ++i) {
        const uint *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = *src_data | 0xff000000;
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static QVector<QRgb> fix_color_table(const QVector<QRgb> &ctbl, QImage::Format format)
{
    QVector<QRgb> colorTable = ctbl;
    if (format == QImage::Format_RGB32) {
        // check if the color table has alpha
        for (int i = 0; i < colorTable.size(); ++i)
            if (qAlpha(colorTable.at(i) != 0xff))
                colorTable[i] = colorTable.at(i) | 0xff000000;
    } else if (format == QImage::Format_ARGB32_Premultiplied) {
        // check if the color table has alpha
        for (int i = 0; i < colorTable.size(); ++i)
            colorTable[i] = PREMUL(colorTable.at(i));
    }
    return colorTable;
}

//
// dither_to_1:  Uses selected dithering algorithm.
//

static void dither_to_Mono(QImageData *dst, const QImageData *src,
                           Qt::ImageConversionFlags flags, bool fromalpha)
{
    Q_ASSERT(src->width == dst->width);
    Q_ASSERT(src->height == dst->height);
    Q_ASSERT(dst->format == QImage::Format_Mono || dst->format == QImage::Format_MonoLSB);

    dst->colortable.clear();
    dst->colortable.append(0xffffffff);
    dst->colortable.append(0xff000000);

    enum { Threshold, Ordered, Diffuse } dithermode;

    if (fromalpha) {
        if ((flags & Qt::AlphaDither_Mask) == Qt::DiffuseAlphaDither)
            dithermode = Diffuse;
        else if ((flags & Qt::AlphaDither_Mask) == Qt::OrderedAlphaDither)
            dithermode = Ordered;
        else
            dithermode = Threshold;
    } else {
        if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither)
            dithermode = Threshold;
        else if ((flags & Qt::Dither_Mask) == Qt::OrderedDither)
            dithermode = Ordered;
        else
            dithermode = Diffuse;
    }

    int          w = src->width;
    int          h = src->height;
    int          d = src->depth;
    uchar gray[256];                                // gray map for 8 bit images
    bool  use_gray = (d == 8);
    if (use_gray) {                                // make gray map
        if (fromalpha) {
            // Alpha 0x00 -> 0 pixels (white)
            // Alpha 0xFF -> 1 pixels (black)
            for (int i = 0; i < src->colortable.size(); i++)
                gray[i] = (255 - (src->colortable.at(i) >> 24));
        } else {
            // Pixel 0x00 -> 1 pixels (black)
            // Pixel 0xFF -> 0 pixels (white)
            for (int i = 0; i < src->colortable.size(); i++)
                gray[i] = qGray(src->colortable.at(i));
        }
    }

    uchar *dst_data = dst->data;
    int dst_bpl = dst->bytes_per_line;
    const uchar *src_data = src->data;
    int src_bpl = src->bytes_per_line;

    switch (dithermode) {
    case Diffuse: {
        QScopedArrayPointer<int> lineBuffer(new int[w * 2]);
        int *line1 = lineBuffer.data();
        int *line2 = lineBuffer.data() + w;
        int bmwidth = (w+7)/8;

        int *b1, *b2;
        int wbytes = w * (d/8);
        register const uchar *p = src->data;
        const uchar *end = p + wbytes;
        b2 = line2;
        if (use_gray) {                        // 8 bit image
            while (p < end)
                *b2++ = gray[*p++];
        } else {                                // 32 bit image
            if (fromalpha) {
                while (p < end) {
                    *b2++ = 255 - (*(uint*)p >> 24);
                    p += 4;
                }
            } else {
                while (p < end) {
                    *b2++ = qGray(*(uint*)p);
                    p += 4;
                }
            }
        }
        for (int y=0; y<h; y++) {                        // for each scan line...
            int *tmp = line1; line1 = line2; line2 = tmp;
            bool not_last_line = y < h - 1;
            if (not_last_line) {                // calc. grayvals for next line
                p = src->data + (y+1)*src->bytes_per_line;
                end = p + wbytes;
                b2 = line2;
                if (use_gray) {                // 8 bit image
                    while (p < end)
                        *b2++ = gray[*p++];
                } else {                        // 24 bit image
                    if (fromalpha) {
                        while (p < end) {
                            *b2++ = 255 - (*(uint*)p >> 24);
                            p += 4;
                        }
                    } else {
                        while (p < end) {
                            *b2++ = qGray(*(uint*)p);
                            p += 4;
                        }
                    }
                }
            }

            int err;
            uchar *p = dst->data + y*dst->bytes_per_line;
            memset(p, 0, bmwidth);
            b1 = line1;
            b2 = line2;
            int bit = 7;
            for (int x=1; x<=w; x++) {
                if (*b1 < 128) {                // black pixel
                    err = *b1++;
                    *p |= 1 << bit;
                } else {                        // white pixel
                    err = *b1++ - 255;
                }
                if (bit == 0) {
                    p++;
                    bit = 7;
                } else {
                    bit--;
                }
                if (x < w)
                    *b1 += (err*7)>>4;                // spread error to right pixel
                if (not_last_line) {
                    b2[0] += (err*5)>>4;        // pixel below
                    if (x > 1)
                        b2[-1] += (err*3)>>4;        // pixel below left
                    if (x < w)
                        b2[1] += err>>4;        // pixel below right
                }
                b2++;
            }
        }
    } break;
    case Ordered: {

        memset(dst->data, 0, dst->nbytes);
        if (d == 32) {
            for (int i=0; i<h; i++) {
                const uint *p = (const uint *)src_data;
                const uint *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                int j = 0;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= qt_bayer_matrix[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if ((uint)qGray(*p++) < qt_bayer_matrix[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        } else
            /* (d == 8) */ {
            for (int i=0; i<h; i++) {
                const uchar *p = src_data;
                const uchar *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                int j = 0;
                while (p < end) {
                    if ((uint)gray[*p++] < qt_bayer_matrix[j++&15][i&15])
                        *m |= 1 << bit;
                    if (bit == 0) {
                        m++;
                        bit = 7;
                    } else {
                        bit--;
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        }
    } break;
    default: { // Threshold:
        memset(dst->data, 0, dst->nbytes);
        if (d == 32) {
            for (int i=0; i<h; i++) {
                const uint *p = (const uint *)src_data;
                const uint *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= 128)
                            *m |= 1 << bit;        // Set mask "on"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if (qGray(*p++) < 128)
                            *m |= 1 << bit;        // Set pixel "black"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        } else
            if (d == 8) {
                for (int i=0; i<h; i++) {
                    const uchar *p = src_data;
                    const uchar *end = p + w;
                    uchar *m = dst_data;
                    int bit = 7;
                    while (p < end) {
                        if (gray[*p++] < 128)
                            *m |= 1 << bit;                // Set mask "on"/ pixel "black"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                    dst_data += dst_bpl;
                    src_data += src_bpl;
                }
            }
        }
    }

    if (dst->format == QImage::Format_MonoLSB) {
        // need to swap bit order
        uchar *sl = dst->data;
        int bpl = (dst->width + 7) * dst->depth / 8;
        int pad = dst->bytes_per_line - bpl;
        for (int y=0; y<dst->height; ++y) {
            for (int x=0; x<bpl; ++x) {
                *sl = bitflip[*sl];
                ++sl;
            }
            sl += pad;
        }
    }
}

static void convert_X_to_Mono(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    dither_to_Mono(dst, src, flags, false);
}

static void convert_ARGB_PM_to_Mono(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    QScopedPointer<QImageData> tmp(QImageData::create(QSize(src->width, src->height), QImage::Format_ARGB32));
    convert_ARGB_PM_to_ARGB(tmp.data(), src, flags);
    dither_to_Mono(dst, tmp.data(), flags, false);
}

//
// convert_32_to_8:  Converts a 32 bits depth (true color) to an 8 bit
// image with a colormap. If the 32 bit image has more than 256 colors,
// we convert the red,green and blue bytes into a single byte encoded
// as 6 shades of each of red, green and blue.
//
// if dithering is needed, only 1 color at most is available for alpha.
//
struct QRgbMap {
    inline QRgbMap() : used(0) { }
    uchar  pix;
    uchar used;
    QRgb  rgb;
};

static void convert_RGB_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    Q_ASSERT(src->format == QImage::Format_RGB32 || src->format == QImage::Format_ARGB32);
    Q_ASSERT(dst->format == QImage::Format_Indexed8);
    Q_ASSERT(src->width == dst->width);
    Q_ASSERT(src->height == dst->height);

    bool    do_quant = (flags & Qt::DitherMode_Mask) == Qt::PreferDither
                       || src->format == QImage::Format_ARGB32;
    uint alpha_mask = src->format == QImage::Format_RGB32 ? 0xff000000 : 0;

    const int tablesize = 997; // prime
    QRgbMap table[tablesize];
    int   pix=0;

    if (!dst->colortable.isEmpty()) {
        QVector<QRgb> ctbl = dst->colortable;
        dst->colortable.resize(256);
        // Preload palette into table.
        // Almost same code as pixel insertion below
        for (int i = 0; i < dst->colortable.size(); ++i) {
            // Find in table...
            QRgb p = ctbl.at(i) | alpha_mask;
            int hash = p % tablesize;
            for (;;) {
                if (table[hash].used) {
                    if (table[hash].rgb == p) {
                        // Found previous insertion - use it
                        break;
                    } else {
                        // Keep searching...
                        if (++hash == tablesize) hash = 0;
                    }
                } else {
                    // Cannot be in table
                    Q_ASSERT (pix != 256);        // too many colors
                    // Insert into table at this unused position
                    dst->colortable[pix] = p;
                    table[hash].pix = pix++;
                    table[hash].rgb = p;
                    table[hash].used = 1;
                    break;
                }
            }
        }
    }

    if ((flags & Qt::DitherMode_Mask) != Qt::PreferDither) {
        dst->colortable.resize(256);
        const uchar *src_data = src->data;
        uchar *dest_data = dst->data;
        for (int y = 0; y < src->height; y++) {        // check if <= 256 colors
            const QRgb *s = (const QRgb *)src_data;
            uchar *b = dest_data;
            for (int x = 0; x < src->width; ++x) {
                QRgb p = s[x] | alpha_mask;
                int hash = p % tablesize;
                for (;;) {
                    if (table[hash].used) {
                        if (table[hash].rgb == (p)) {
                            // Found previous insertion - use it
                            break;
                        } else {
                            // Keep searching...
                            if (++hash == tablesize) hash = 0;
                        }
                    } else {
                        // Cannot be in table
                        if (pix == 256) {        // too many colors
                            do_quant = true;
                            // Break right out
                            x = src->width;
                            y = src->height;
                        } else {
                            // Insert into table at this unused position
                            dst->colortable[pix] = p;
                            table[hash].pix = pix++;
                            table[hash].rgb = p;
                            table[hash].used = 1;
                        }
                        break;
                    }
                }
                *b++ = table[hash].pix;                // May occur once incorrectly
            }
            src_data += src->bytes_per_line;
            dest_data += dst->bytes_per_line;
        }
    }
    int numColors = do_quant ? 256 : pix;

    dst->colortable.resize(numColors);

    if (do_quant) {                                // quantization needed

#define MAX_R 5
#define MAX_G 5
#define MAX_B 5
#define INDEXOF(r,g,b) (((r)*(MAX_G+1)+(g))*(MAX_B+1)+(b))

        for (int rc=0; rc<=MAX_R; rc++)                // build 6x6x6 color cube
            for (int gc=0; gc<=MAX_G; gc++)
                for (int bc=0; bc<=MAX_B; bc++)
                    dst->colortable[INDEXOF(rc,gc,bc)] = 0xff000000 | qRgb(rc*255/MAX_R, gc*255/MAX_G, bc*255/MAX_B);

        const uchar *src_data = src->data;
        uchar *dest_data = dst->data;
        if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither) {
            for (int y = 0; y < src->height; y++) {
                const QRgb *p = (const QRgb *)src_data;
                const QRgb *end = p + src->width;
                uchar *b = dest_data;

                while (p < end) {
#define DITHER(p,m) ((uchar) ((p * (m) + 127) / 255))
                    *b++ =
                        INDEXOF(
                            DITHER(qRed(*p), MAX_R),
                            DITHER(qGreen(*p), MAX_G),
                            DITHER(qBlue(*p), MAX_B)
                            );
#undef DITHER
                    p++;
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        } else if ((flags & Qt::Dither_Mask) == Qt::DiffuseDither) {
            int* line1[3];
            int* line2[3];
            int* pv[3];
            QScopedArrayPointer<int> lineBuffer(new int[src->width * 9]);
            line1[0] = lineBuffer.data();
            line2[0] = lineBuffer.data() + src->width;
            line1[1] = lineBuffer.data() + src->width * 2;
            line2[1] = lineBuffer.data() + src->width * 3;
            line1[2] = lineBuffer.data() + src->width * 4;
            line2[2] = lineBuffer.data() + src->width * 5;
            pv[0] = lineBuffer.data() + src->width * 6;
            pv[1] = lineBuffer.data() + src->width * 7;
            pv[2] = lineBuffer.data() + src->width * 8;

            int endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
            for (int y = 0; y < src->height; y++) {
                const uchar* q = src_data;
                const uchar* q2 = y < src->height - 1 ? q + src->bytes_per_line : src->data;
                uchar *b = dest_data;
                for (int chan = 0; chan < 3; chan++) {
                    int *l1 = (y&1) ? line2[chan] : line1[chan];
                    int *l2 = (y&1) ? line1[chan] : line2[chan];
                    if (y == 0) {
                        for (int i = 0; i < src->width; i++)
                            l1[i] = q[i*4+chan+endian];
                    }
                    if (y+1 < src->height) {
                        for (int i = 0; i < src->width; i++)
                            l2[i] = q2[i*4+chan+endian];
                    }
                    // Bi-directional error diffusion
                    if (y&1) {
                        for (int x = 0; x < src->width; x++) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x + 1< src->width) {
                                l1[x+1] += (err*7)>>4;
                                l2[x+1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x>1)
                                l2[x-1]+=(err*3)>>4;
                        }
                    } else {
                        for (int x = src->width; x-- > 0;) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x > 0) {
                                l1[x-1] += (err*7)>>4;
                                l2[x-1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x + 1 < src->width)
                                l2[x+1]+=(err*3)>>4;
                        }
                    }
                }
                if (endian) {
                    for (int x = 0; x < src->width; x++) {
                        *b++ = INDEXOF(pv[0][x],pv[1][x],pv[2][x]);
                    }
                } else {
                    for (int x = 0; x < src->width; x++) {
                        *b++ = INDEXOF(pv[2][x],pv[1][x],pv[0][x]);
                    }
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        } else { // OrderedDither
            for (int y = 0; y < src->height; y++) {
                const QRgb *p = (const QRgb *)src_data;
                const QRgb *end = p + src->width;
                uchar *b = dest_data;

                int x = 0;
                while (p < end) {
                    uint d = qt_bayer_matrix[y & 15][x & 15] << 8;

#define DITHER(p, d, m) ((uchar) ((((256 * (m) + (m) + 1)) * (p) + (d)) >> 16))
                    *b++ =
                        INDEXOF(
                            DITHER(qRed(*p), d, MAX_R),
                            DITHER(qGreen(*p), d, MAX_G),
                            DITHER(qBlue(*p), d, MAX_B)
                            );
#undef DITHER

                    p++;
                    x++;
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        }

        if (src->format != QImage::Format_RGB32
            && src->format != QImage::Format_RGB16) {
            const int trans = 216;
            Q_ASSERT(dst->colortable.size() > trans);
            dst->colortable[trans] = 0;
            QScopedPointer<QImageData> mask(QImageData::create(QSize(src->width, src->height), QImage::Format_Mono));
            dither_to_Mono(mask.data(), src, flags, true);
            uchar *dst_data = dst->data;
            const uchar *mask_data = mask->data;
            for (int y = 0; y < src->height; y++) {
                for (int x = 0; x < src->width ; x++) {
                    if (!(mask_data[x>>3] & (0x80 >> (x & 7))))
                        dst_data[x] = trans;
                }
                mask_data += mask->bytes_per_line;
                dst_data += dst->bytes_per_line;
            }
            dst->has_alpha_clut = true;
        }

#undef MAX_R
#undef MAX_G
#undef MAX_B
#undef INDEXOF

    }
}

static void convert_ARGB_PM_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    QScopedPointer<QImageData> tmp(QImageData::create(QSize(src->width, src->height), QImage::Format_ARGB32));
    convert_ARGB_PM_to_ARGB(tmp.data(), src, flags);
    convert_RGB_to_Indexed8(dst, tmp.data(), flags);
}

static void convert_ARGB_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    convert_RGB_to_Indexed8(dst, src, flags);
}

static void convert_Indexed8_to_X32(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Indexed8);
    Q_ASSERT(dest->format == QImage::Format_RGB32
             || dest->format == QImage::Format_ARGB32
             || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> colorTable = fix_color_table(src->colortable, dest->format);
    if (colorTable.size() == 0) {
        colorTable.resize(256);
        for (int i=0; i<256; ++i)
            colorTable[i] = qRgb(i, i, i);
    }

    int w = src->width;
    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    int tableSize = colorTable.size() - 1;
    for (int y = 0; y < src->height; y++) {
        uint *p = (uint *)dest_data;
        const uchar *b = src_data;
        uint *end = p + w;

        while (p < end)
            *p++ = colorTable.at(qMin<int>(tableSize, *b++));

        src_data += src->bytes_per_line;
        dest_data += dest->bytes_per_line;
    }
}

static void convert_Mono_to_X32(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_RGB32
             || dest->format == QImage::Format_ARGB32
             || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> colorTable = fix_color_table(src->colortable, dest->format);

    // Default to black / white colors
    if (colorTable.size() < 2) {
        if (colorTable.size() == 0)
            colorTable << 0xff000000;
        colorTable << 0xffffffff;
    }

    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    if (src->format == QImage::Format_Mono) {
        for (int y = 0; y < dest->height; y++) {
            register uint *p = (uint *)dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = colorTable.at((src_data[x>>3] >> (7 - (x & 7))) & 1);

            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    } else {
        for (int y = 0; y < dest->height; y++) {
            register uint *p = (uint *)dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = colorTable.at((src_data[x>>3] >> (x & 7)) & 1);

            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    }
}


static void convert_Mono_to_Indexed8(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_Indexed8);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> ctbl = src->colortable;
    if (ctbl.size() > 2) {
        ctbl.resize(2);
    } else if (ctbl.size() < 2) {
        if (ctbl.size() == 0)
            ctbl << 0xff000000;
        ctbl << 0xffffffff;
    }
    dest->colortable = ctbl;
    dest->has_alpha_clut = src->has_alpha_clut;


    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    if (src->format == QImage::Format_Mono) {
        for (int y = 0; y < dest->height; y++) {
            register uchar *p = dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = (src_data[x>>3] >> (7 - (x & 7))) & 1;
            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    } else {
        for (int y = 0; y < dest->height; y++) {
            register uchar *p = dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = (src_data[x>>3] >> (x & 7)) & 1;
            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    }
}

#define CONVERT_DECL(DST, SRC)                                          \
    static void convert_##SRC##_to_##DST(QImageData *dest,              \
                                         const QImageData *src,         \
                                         Qt::ImageConversionFlags)      \
    {                                                                   \
        qt_rectconvert<DST, SRC>(reinterpret_cast<DST*>(dest->data),    \
                                 reinterpret_cast<const SRC*>(src->data), \
                                 0, 0, src->width, src->height,         \
                                 dest->bytes_per_line, src->bytes_per_line); \
    }

CONVERT_DECL(quint32, quint16)
CONVERT_DECL(quint16, quint32)
CONVERT_DECL(quint32, qargb8565)
CONVERT_DECL(qargb8565, quint32)
CONVERT_DECL(quint32, qrgb555)
CONVERT_DECL(qrgb666, quint32)
CONVERT_DECL(quint32, qrgb666)
CONVERT_DECL(qargb6666, quint32)
CONVERT_DECL(quint32, qargb6666)
CONVERT_DECL(qrgb555, quint32)
#if !defined(Q_WS_QWS) || (defined(QT_QWS_DEPTH_15) && defined(QT_QWS_DEPTH_16))
CONVERT_DECL(quint16, qrgb555)
CONVERT_DECL(qrgb555, quint16)
#endif
CONVERT_DECL(quint32, qrgb888)
CONVERT_DECL(qrgb888, quint32)
CONVERT_DECL(quint32, qargb8555)
CONVERT_DECL(qargb8555, quint32)
CONVERT_DECL(quint32, qrgb444)
CONVERT_DECL(qrgb444, quint32)
CONVERT_DECL(quint32, qargb4444)
CONVERT_DECL(qargb4444, quint32)
#undef CONVERT_DECL
#define CONVERT_PTR(DST, SRC) convert_##SRC##_to_##DST

/*
        Format_Invalid,
        Format_Mono,
        Format_MonoLSB,
        Format_Indexed8,
        Format_RGB32,
        Format_ARGB32,
        Format_ARGB32_Premultiplied,
        Format_RGB16,
        Format_ARGB8565_Premultiplied,
        Format_RGB666,
        Format_ARGB6666_Premultiplied,
        Format_RGB555,
        Format_ARGB8555_Premultiplied,
        Format_RGB888
        Format_RGB444
        Format_ARGB4444_Premultiplied
*/


// first index source, second dest
static Image_Converter converter_map[QImage::NImageFormats][QImage::NImageFormats] =
{
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0,
        0,
        swap_bit_order,
        convert_Mono_to_Indexed8,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_Mono

    {
        0,
        swap_bit_order,
        0,
        convert_Mono_to_Indexed8,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_MonoLSB

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        0,
        convert_Indexed8_to_X32,
        convert_Indexed8_to_X32,
        convert_Indexed8_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_Indexed8

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        convert_RGB_to_Indexed8,
        0,
        mask_alpha_converter,
        mask_alpha_converter,
        CONVERT_PTR(quint16, quint32),
        CONVERT_PTR(qargb8565, quint32),
        CONVERT_PTR(qrgb666, quint32),
        CONVERT_PTR(qargb6666, quint32),
        CONVERT_PTR(qrgb555, quint32),
        CONVERT_PTR(qargb8555, quint32),
        CONVERT_PTR(qrgb888, quint32),
        CONVERT_PTR(qrgb444, quint32),
        CONVERT_PTR(qargb4444, quint32)
    }, // Format_RGB32

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        convert_ARGB_to_Indexed8,
        mask_alpha_converter,
        0,
        convert_ARGB_to_ARGB_PM,
        CONVERT_PTR(quint16, quint32),
        CONVERT_PTR(qargb8565, quint32),
        CONVERT_PTR(qrgb666, quint32),
        CONVERT_PTR(qargb6666, quint32),
        CONVERT_PTR(qrgb555, quint32),
        CONVERT_PTR(qargb8555, quint32),
        CONVERT_PTR(qrgb888, quint32),
        CONVERT_PTR(qrgb444, quint32),
        CONVERT_PTR(qargb4444, quint32)
    }, // Format_ARGB32

    {
        0,
        convert_ARGB_PM_to_Mono,
        convert_ARGB_PM_to_Mono,
        convert_ARGB_PM_to_Indexed8,
        convert_ARGB_PM_to_RGB,
        convert_ARGB_PM_to_ARGB,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    },  // Format_ARGB32_Premultiplied

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, quint16),
        CONVERT_PTR(quint32, quint16),
        CONVERT_PTR(quint32, quint16),
        0,
        0,
        0,
        0,
#if !defined(Q_WS_QWS) || (defined(QT_QWS_DEPTH_15) && defined(QT_QWS_DEPTH_16))
        CONVERT_PTR(qrgb555, quint16),
#else
        0,
#endif
        0,
        0,
        0,
        0
    }, // Format_RGB16

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qargb8565),
        CONVERT_PTR(quint32, qargb8565),
        CONVERT_PTR(quint32, qargb8565),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB8565_Premultiplied

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qrgb666),
        CONVERT_PTR(quint32, qrgb666),
        CONVERT_PTR(quint32, qrgb666),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB666

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qargb6666),
        CONVERT_PTR(quint32, qargb6666),
        CONVERT_PTR(quint32, qargb6666),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB6666_Premultiplied

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qrgb555),
        CONVERT_PTR(quint32, qrgb555),
        CONVERT_PTR(quint32, qrgb555),
#if !defined(Q_WS_QWS) || (defined(QT_QWS_DEPTH_15) && defined(QT_QWS_DEPTH_16))
        CONVERT_PTR(quint16, qrgb555),
#else
        0,
#endif
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB555

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qargb8555),
        CONVERT_PTR(quint32, qargb8555),
        CONVERT_PTR(quint32, qargb8555),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB8555_Premultiplied

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qrgb888),
        CONVERT_PTR(quint32, qrgb888),
        CONVERT_PTR(quint32, qrgb888),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB888

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qrgb444),
        CONVERT_PTR(quint32, qrgb444),
        CONVERT_PTR(quint32, qrgb444),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB444

    {
        0,
        0,
        0,
        0,
        CONVERT_PTR(quint32, qargb4444),
        CONVERT_PTR(quint32, qargb4444),
        CONVERT_PTR(quint32, qargb4444),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    } // Format_ARGB4444_Premultiplied
};

static InPlace_Image_Converter inplace_converter_map[QImage::NImageFormats][QImage::NImageFormats] =
{
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_Mono
    {
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_MonoLSB
    {
        0,
        0,
        0,
        0,
        0,
        convert_indexed8_to_RGB_inplace,
        convert_indexed8_to_ARGB_PM_inplace,
        convert_indexed8_to_RGB16_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_Indexed8
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_RGB_to_RGB16_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_ARGB32
    {
        0,
        0,
        0,
        0,
        0,
        0,
        convert_ARGB_to_ARGB_PM_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_ARGB32
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },  // Format_ARGB32_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB16
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB8565_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB666
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB6666_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB555
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB8555_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB888
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB444
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    } // Format_ARGB4444_Premultiplied
};

void qInitImageConversions()
{
    const uint features = qDetectCPUFeatures();
    Q_UNUSED(features);

#ifdef QT_HAVE_SSE2
    if (features & SSE2) {
        extern bool convert_ARGB_to_ARGB_PM_inplace_sse2(QImageData *data, Qt::ImageConversionFlags);
        inplace_converter_map[QImage::Format_ARGB32][QImage::Format_ARGB32_Premultiplied] = convert_ARGB_to_ARGB_PM_inplace_sse2;
    }
#endif
#ifdef QT_HAVE_SSSE3
    if (features & SSSE3) {
        extern void convert_RGB888_to_RGB32_ssse3(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
        converter_map[QImage::Format_RGB888][QImage::Format_RGB32] = convert_RGB888_to_RGB32_ssse3;
        converter_map[QImage::Format_RGB888][QImage::Format_ARGB32] = convert_RGB888_to_RGB32_ssse3;
        converter_map[QImage::Format_RGB888][QImage::Format_ARGB32_Premultiplied] = convert_RGB888_to_RGB32_ssse3;
    }
#endif
#ifdef QT_HAVE_NEON
    if (features & NEON) {
        extern void convert_RGB888_to_RGB32_neon(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
        converter_map[QImage::Format_RGB888][QImage::Format_RGB32] = convert_RGB888_to_RGB32_neon;
        converter_map[QImage::Format_RGB888][QImage::Format_ARGB32] = convert_RGB888_to_RGB32_neon;
        converter_map[QImage::Format_RGB888][QImage::Format_ARGB32_Premultiplied] = convert_RGB888_to_RGB32_neon;
    }
#endif
}

void qGamma_correct_back_to_linear_cs(QImage *image)
{
    extern uchar qt_pow_rgb_gamma[256];

    // gamma correct the pixels back to linear color space...
    int h = image->height();
    int w = image->width();

    for (int y=0; y<h; ++y) {
        uint *pixels = (uint *) image->scanLine(y);
        for (int x=0; x<w; ++x) {
            uint p = pixels[x];
            uint r = qt_pow_rgb_gamma[qRed(p)];
            uint g = qt_pow_rgb_gamma[qGreen(p)];
            uint b = qt_pow_rgb_gamma[qBlue(p)];
            pixels[x] = (r << 16) | (g << 8) | b | 0xff000000;
        }
    }
}

/*!
    Returns a copy of the image in the given \a format.

    The specified image conversion \a flags control how the image data
    is handled during the conversion process.

    \sa {QImage#Image Format}{Image Format}
*/
QImage QImage::convertToFormat(Format format, Qt::ImageConversionFlags flags) const
{
    if (!d || d->format == format)
        return *this;

    if (format == Format_Invalid || d->format == Format_Invalid)
        return QImage();

    const Image_Converter *converterPtr = &converter_map[d->format][format];
    Image_Converter converter = *converterPtr;
    if (converter) {
        QImage image(d->width, d->height, format);

        QIMAGE_SANITYCHECK_MEMORY(image);

        image.setDotsPerMeterY(dotsPerMeterY());
        image.setDotsPerMeterX(dotsPerMeterX());

#if !defined(QT_NO_IMAGE_TEXT)
        image.d->text = d->text;
#endif // !QT_NO_IMAGE_TEXT

        converter(image.d, d, flags);
        return image;
    }

    Q_ASSERT(format != QImage::Format_ARGB32);
    Q_ASSERT(d->format != QImage::Format_ARGB32);

    QImage image = convertToFormat(Format_ARGB32, flags);
    return image.convertToFormat(format, flags);
}



static inline int pixel_distance(QRgb p1, QRgb p2) {
    int r1 = qRed(p1);
    int g1 = qGreen(p1);
    int b1 = qBlue(p1);
    int a1 = qAlpha(p1);

    int r2 = qRed(p2);
    int g2 = qGreen(p2);
    int b2 = qBlue(p2);
    int a2 = qAlpha(p2);

    return abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2) + abs(a1 - a2);
}

static inline int closestMatch(QRgb pixel, const QVector<QRgb> &clut) {
    int idx = 0;
    int current_distance = INT_MAX;
    for (int i=0; i<clut.size(); ++i) {
        int dist = pixel_distance(pixel, clut.at(i));
        if (dist < current_distance) {
            current_distance = dist;
            idx = i;
        }
    }
    return idx;
}

static QImage convertWithPalette(const QImage &src, QImage::Format format,
                                 const QVector<QRgb> &clut) {
    QImage dest(src.size(), format);
    QIMAGE_SANITYCHECK_MEMORY(dest);
    dest.setColorTable(clut);

#if !defined(QT_NO_IMAGE_TEXT)
    QString textsKeys = src.text();
    QStringList textKeyList = textsKeys.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    foreach (const QString &textKey, textKeyList) {
        QStringList textKeySplitted = textKey.split(QLatin1String(": "));
        dest.setText(textKeySplitted[0], textKeySplitted[1]);
    }
#endif // !QT_NO_IMAGE_TEXT

    int h = src.height();
    int w = src.width();

    QHash<QRgb, int> cache;

    if (format == QImage::Format_Indexed8) {
        for (int y=0; y<h; ++y) {
            QRgb *src_pixels = (QRgb *) src.scanLine(y);
            uchar *dest_pixels = (uchar *) dest.scanLine(y);
            for (int x=0; x<w; ++x) {
                int src_pixel = src_pixels[x];
                int value = cache.value(src_pixel, -1);
                if (value == -1) {
                    value = closestMatch(src_pixel, clut);
                    cache.insert(src_pixel, value);
                }
                dest_pixels[x] = (uchar) value;
            }
        }
    } else {
        QVector<QRgb> table = clut;
        table.resize(2);
        for (int y=0; y<h; ++y) {
            QRgb *src_pixels = (QRgb *) src.scanLine(y);
            for (int x=0; x<w; ++x) {
                int src_pixel = src_pixels[x];
                int value = cache.value(src_pixel, -1);
                if (value == -1) {
                    value = closestMatch(src_pixel, table);
                    cache.insert(src_pixel, value);
                }
                dest.setPixel(x, y, value);
            }
        }
    }

    return dest;
}

/*!
    \overload

    Returns a copy of the image converted to the given \a format,
    using the specified \a colorTable.

    Conversion from 32 bit to 8 bit indexed is a slow operation and
    will use a straightforward nearest color approach, with no
    dithering.
*/
QImage QImage::convertToFormat(Format format, const QVector<QRgb> &colorTable, Qt::ImageConversionFlags flags) const
{
    if (d->format == format)
        return *this;

    if (format <= QImage::Format_Indexed8 && depth() == 32) {
        return convertWithPalette(*this, format, colorTable);
    }

    const Image_Converter *converterPtr = &converter_map[d->format][format];
    Image_Converter converter = *converterPtr;
    if (!converter)
        return QImage();

    QImage image(d->width, d->height, format);
    QIMAGE_SANITYCHECK_MEMORY(image);

#if !defined(QT_NO_IMAGE_TEXT)
        image.d->text = d->text;
#endif // !QT_NO_IMAGE_TEXT

    converter(image.d, d, flags);
    return image;
}

#ifdef QT3_SUPPORT
/*!
    Converts the depth (bpp) of the image to the given \a depth and
    returns the converted image. The original image is not changed.
    Returns this image if \a depth is equal to the image depth, or a
    null image if this image cannot be converted. The \a depth
    argument must be 1, 8 or 32. If the image needs to be modified to
    fit in a lower-resolution result (e.g. converting from 32-bit to
    8-bit), use the \a flags to specify how you'd prefer this to
    happen.

    Use the convertToFormat() function instead.
*/

QImage QImage::convertDepth(int depth, Qt::ImageConversionFlags flags) const
{
    if (!d || d->depth == depth)
        return *this;

    Format format = formatFor (depth, QImage::LittleEndian);
    return convertToFormat(format, flags);
}
#endif

/*!
    \fn bool QImage::valid(const QPoint &pos) const

    Returns true if \a pos is a valid coordinate pair within the
    image; otherwise returns false.

    \sa rect(), QRect::contains()
*/

/*!
    \overload

    Returns true if QPoint(\a x, \a y) is a valid coordinate pair
    within the image; otherwise returns false.
*/
bool QImage::valid(int x, int y) const
{
    return d
        && x >= 0 && x < d->width
        && y >= 0 && y < d->height;
}

/*!
    \fn int QImage::pixelIndex(const QPoint &position) const

    Returns the pixel index at the given \a position.

    If \a position is not valid, or if the image is not a paletted
    image (depth() > 8), the results are undefined.

    \sa valid(), depth(), {QImage#Pixel Manipulation}{Pixel Manipulation}
*/

/*!
    \overload

    Returns the pixel index at (\a x, \a y).
*/
int QImage::pixelIndex(int x, int y) const
{
    if (!d || x < 0 || x >= d->width || y < 0 || y >= height()) {
        qWarning("QImage::pixelIndex: coordinate (%d,%d) out of range", x, y);
        return -12345;
    }
    const uchar * s = scanLine(y);
    switch(d->format) {
    case Format_Mono:
        return (*(s + (x >> 3)) >> (7- (x & 7))) & 1;
    case Format_MonoLSB:
        return (*(s + (x >> 3)) >> (x & 7)) & 1;
    case Format_Indexed8:
        return (int)s[x];
    default:
        qWarning("QImage::pixelIndex: Not applicable for %d-bpp images (no palette)", d->depth);
    }
    return 0;
}


/*!
    \fn QRgb QImage::pixel(const QPoint &position) const

    Returns the color of the pixel at the given \a position.

    If the \a position is not valid, the results are undefined.

    \warning This function is expensive when used for massive pixel
    manipulations.

    \sa setPixel(), valid(), {QImage#Pixel Manipulation}{Pixel
    Manipulation}
*/

/*!
    \overload

    Returns the color of the pixel at coordinates (\a x, \a y).
*/
QRgb QImage::pixel(int x, int y) const
{
    if (!d || x < 0 || x >= d->width || y < 0 || y >= height()) {
        qWarning("QImage::pixel: coordinate (%d,%d) out of range", x, y);
        return 12345;
    }
    const uchar * s = scanLine(y);
    switch(d->format) {
    case Format_Mono:
        return d->colortable.at((*(s + (x >> 3)) >> (7- (x & 7))) & 1);
    case Format_MonoLSB:
        return d->colortable.at((*(s + (x >> 3)) >> (x & 7)) & 1);
    case Format_Indexed8:
        return d->colortable.at((int)s[x]);
    case Format_ARGB8565_Premultiplied:
        return qt_colorConvert<quint32, qargb8565>(reinterpret_cast<const qargb8565*>(s)[x], 0);
    case Format_RGB666:
        return qt_colorConvert<quint32, qrgb666>(reinterpret_cast<const qrgb666*>(s)[x], 0);
    case Format_ARGB6666_Premultiplied:
        return qt_colorConvert<quint32, qargb6666>(reinterpret_cast<const qargb6666*>(s)[x], 0);
    case Format_RGB555:
        return qt_colorConvert<quint32, qrgb555>(reinterpret_cast<const qrgb555*>(s)[x], 0);
    case Format_ARGB8555_Premultiplied:
        return qt_colorConvert<quint32, qargb8555>(reinterpret_cast<const qargb8555*>(s)[x], 0);
    case Format_RGB888:
        return qt_colorConvert<quint32, qrgb888>(reinterpret_cast<const qrgb888*>(s)[x], 0);
    case Format_RGB444:
        return qt_colorConvert<quint32, qrgb444>(reinterpret_cast<const qrgb444*>(s)[x], 0);
    case Format_ARGB4444_Premultiplied:
        return qt_colorConvert<quint32, qargb4444>(reinterpret_cast<const qargb4444*>(s)[x], 0);
    case Format_RGB16:
        return qt_colorConvert<quint32, quint16>(reinterpret_cast<const quint16*>(s)[x], 0);
    default:
        return ((QRgb*)s)[x];
    }
}


/*!
    \fn void QImage::setPixel(const QPoint &position, uint index_or_rgb)

    Sets the pixel index or color at the given \a position to \a
    index_or_rgb.

    If the image's format is either monochrome or 8-bit, the given \a
    index_or_rgb value must be an index in the image's color table,
    otherwise the parameter must be a QRgb value.

    If \a position is not a valid coordinate pair in the image, or if
    \a index_or_rgb >= colorCount() in the case of monochrome and
    8-bit images, the result is undefined.

    \warning This function is expensive due to the call of the internal
    \c{detach()} function called within; if performance is a concern, we
    recommend the use of \l{QImage::}{scanLine()} to access pixel data
    directly.

    \sa pixel(), {QImage#Pixel Manipulation}{Pixel Manipulation}
*/

/*!
    \overload

    Sets the pixel index or color at (\a x, \a y) to \a index_or_rgb.
*/
void QImage::setPixel(int x, int y, uint index_or_rgb)
{
    if (!d || x < 0 || x >= width() || y < 0 || y >= height()) {
        qWarning("QImage::setPixel: coordinate (%d,%d) out of range", x, y);
        return;
    }
    // detach is called from within scanLine
    uchar * s = scanLine(y);
    if (!s) {
        qWarning("setPixel: Out of memory");
        return;
    }
    const quint32p p = quint32p::fromRawData(index_or_rgb);
    switch(d->format) {
    case Format_Mono:
    case Format_MonoLSB:
        if (index_or_rgb > 1) {
            qWarning("QImage::setPixel: Index %d out of range", index_or_rgb);
        } else if (format() == Format_MonoLSB) {
            if (index_or_rgb==0)
                *(s + (x >> 3)) &= ~(1 << (x & 7));
            else
                *(s + (x >> 3)) |= (1 << (x & 7));
        } else {
            if (index_or_rgb==0)
                *(s + (x >> 3)) &= ~(1 << (7-(x & 7)));
            else
                *(s + (x >> 3)) |= (1 << (7-(x & 7)));
        }
        break;
    case Format_Indexed8:
        if (index_or_rgb >= (uint)d->colortable.size()) {
            qWarning("QImage::setPixel: Index %d out of range", index_or_rgb);
            return;
        }
        s[x] = index_or_rgb;
        break;
    case Format_RGB32:
        //make sure alpha is 255, we depend on it in qdrawhelper for cases
        // when image is set as a texture pattern on a qbrush
        ((uint *)s)[x] = uint(255 << 24) | index_or_rgb;
        break;
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
        ((uint *)s)[x] = index_or_rgb;
        break;
    case Format_RGB16:
        ((quint16 *)s)[x] = qt_colorConvert<quint16, quint32p>(p, 0);
        break;
    case Format_ARGB8565_Premultiplied:
        ((qargb8565*)s)[x] = qt_colorConvert<qargb8565, quint32p>(p, 0);
        break;
    case Format_RGB666:
        ((qrgb666*)s)[x] = qt_colorConvert<qrgb666, quint32p>(p, 0);
        break;
    case Format_ARGB6666_Premultiplied:
        ((qargb6666*)s)[x] = qt_colorConvert<qargb6666, quint32p>(p, 0);
        break;
    case Format_RGB555:
        ((qrgb555*)s)[x] = qt_colorConvert<qrgb555, quint32p>(p, 0);
        break;
    case Format_ARGB8555_Premultiplied:
        ((qargb8555*)s)[x] = qt_colorConvert<qargb8555, quint32p>(p, 0);
        break;
    case Format_RGB888:
        ((qrgb888*)s)[x] = qt_colorConvert<qrgb888, quint32p>(p, 0);
        break;
    case Format_RGB444:
        ((qrgb444*)s)[x] = qt_colorConvert<qrgb444, quint32p>(p, 0);
        break;
    case Format_ARGB4444_Premultiplied:
        ((qargb4444*)s)[x] = qt_colorConvert<qargb4444, quint32p>(p, 0);
        break;
    case Format_Invalid:
    case NImageFormats:
        Q_ASSERT(false);
    }
}

#ifdef QT3_SUPPORT
/*!
    Converts the bit order of the image to the given \a bitOrder and
    returns the converted image. The original image is not changed.
    Returns this image if the given \a bitOrder is equal to the image
    current bit order, or a null image if this image cannot be
    converted.

    Use convertToFormat() instead.
*/

QImage QImage::convertBitOrder(Endian bitOrder) const
{
    if (!d || isNull() || d->depth != 1 || !(bitOrder == BigEndian || bitOrder == LittleEndian))
        return QImage();

    if ((d->format == Format_Mono && bitOrder == BigEndian)
        || (d->format == Format_MonoLSB && bitOrder == LittleEndian))
        return *this;

    QImage image(d->width, d->height, d->format == Format_Mono ? Format_MonoLSB : Format_Mono);
    QIMAGE_SANITYCHECK_MEMORY(image);

    const uchar *data = d->data;
    const uchar *end = data + d->nbytes;
    uchar *ndata = image.d->data;
    while (data < end)
        *ndata++ = bitflip[*data++];

    image.setDotsPerMeterX(dotsPerMeterX());
    image.setDotsPerMeterY(dotsPerMeterY());

    image.d->colortable = d->colortable;
    return image;
}
#endif
/*!
    Returns true if all the colors in the image are shades of gray
    (i.e. their red, green and blue components are equal); otherwise
    false.

    Note that this function is slow for images without color table.

    \sa isGrayscale()
*/
bool QImage::allGray() const
{
    if (!d)
        return true;

    if (d->depth == 32) {
        int p = width()*height();
        const QRgb* b = (const QRgb*)bits();
        while (p--)
            if (!qIsGray(*b++))
                return false;
    } else if (d->depth == 16) {
        int p = width()*height();
        const ushort* b = (const ushort *)bits();
        while (p--)
            if (!qIsGray(qt_colorConvert<quint32, quint16>(*b++, 0)))
                return false;
    } else if (d->format == QImage::Format_RGB888) {
        int p = width()*height();
        const qrgb888* b = (const qrgb888 *)bits();
        while (p--)
            if (!qIsGray(qt_colorConvert<quint32, qrgb888>(*b++, 0)))
                return false;
    } else {
        if (d->colortable.isEmpty())
            return true;
        for (int i = 0; i < colorCount(); i++)
            if (!qIsGray(d->colortable.at(i)))
                return false;
    }
    return true;
}

/*!
    For 32-bit images, this function is equivalent to allGray().

    For 8-bpp images, this function returns true if color(i) is
    QRgb(i, i, i) for all indexes of the color table; otherwise
    returns false.

    \sa allGray(), {QImage#Image Formats}{Image Formats}
*/
bool QImage::isGrayscale() const
{
    if (!d)
        return false;

    switch (depth()) {
    case 32:
    case 24:
    case 16:
        return allGray();
    case 8: {
        for (int i = 0; i < colorCount(); i++)
            if (d->colortable.at(i) != qRgb(i,i,i))
                return false;
        return true;
        }
    }
    return false;
}


/*!
    \fn QImage QImage::smoothScale(int width, int height, Qt::AspectRatioMode mode) const

    Use scaled() instead.

    \oldcode
        QImage image;
        image.smoothScale(width, height, mode);
    \newcode
        QImage image;
        image.scaled(width, height, mode, Qt::SmoothTransformation);
    \endcode
*/

/*!
    \fn QImage QImage::smoothScale(const QSize &size, Qt::AspectRatioMode mode) const
    \overload

    Use scaled() instead.

    \oldcode
        QImage image;
        image.smoothScale(size, mode);
    \newcode
        QImage image;
        image.scaled(size, mode, Qt::SmoothTransformation);
    \endcode
*/

/*!
    \fn QImage QImage::scaled(int width, int height, Qt::AspectRatioMode aspectRatioMode,
                             Qt::TransformationMode transformMode) const
    \overload

    Returns a copy of the image scaled to a rectangle with the given
    \a width and \a height according to the given \a aspectRatioMode
    and \a transformMode.

    If either the \a width or the \a height is zero or negative, this
    function returns a null image.
*/

/*!
    \fn QImage QImage::scaled(const QSize &size, Qt::AspectRatioMode aspectRatioMode,
                             Qt::TransformationMode transformMode) const

    Returns a copy of the image scaled to a rectangle defined by the
    given \a size according to the given \a aspectRatioMode and \a
    transformMode.

    \image qimage-scaling.png

    \list
    \i If \a aspectRatioMode is Qt::IgnoreAspectRatio, the image
       is scaled to \a size.
    \i If \a aspectRatioMode is Qt::KeepAspectRatio, the image is
       scaled to a rectangle as large as possible inside \a size, preserving the aspect ratio.
    \i If \a aspectRatioMode is Qt::KeepAspectRatioByExpanding,
       the image is scaled to a rectangle as small as possible
       outside \a size, preserving the aspect ratio.
    \endlist

    If the given \a size is empty, this function returns a null image.

    \sa isNull(), {QImage#Image Transformations}{Image
    Transformations}
*/
QImage QImage::scaled(const QSize& s, Qt::AspectRatioMode aspectMode, Qt::TransformationMode mode) const
{
    if (!d) {
        qWarning("QImage::scaled: Image is a null image");
        return QImage();
    }
    if (s.isEmpty())
        return QImage();

    QSize newSize = size();
    newSize.scale(s, aspectMode);
    newSize.rwidth() = qMax(newSize.width(), 1);
    newSize.rheight() = qMax(newSize.height(), 1);
    if (newSize == size())
        return *this;

    QTransform wm = QTransform::fromScale((qreal)newSize.width() / width(), (qreal)newSize.height() / height());
    QImage img = transformed(wm, mode);
    return img;
}

/*!
    \fn QImage QImage::scaledToWidth(int width, Qt::TransformationMode mode) const

    Returns a scaled copy of the image. The returned image is scaled
    to the given \a width using the specified transformation \a
    mode.

    This function automatically calculates the height of the image so
    that its aspect ratio is preserved.

    If the given \a width is 0 or negative, a null image is returned.

    \sa {QImage#Image Transformations}{Image Transformations}
*/
QImage QImage::scaledToWidth(int w, Qt::TransformationMode mode) const
{
    if (!d) {
        qWarning("QImage::scaleWidth: Image is a null image");
        return QImage();
    }
    if (w <= 0)
        return QImage();

    qreal factor = (qreal) w / width();
    QTransform wm = QTransform::fromScale(factor, factor);
    return transformed(wm, mode);
}

/*!
    \fn QImage QImage::scaledToHeight(int height, Qt::TransformationMode mode) const

    Returns a scaled copy of the image. The returned image is scaled
    to the given \a height using the specified transformation \a
    mode.

    This function automatically calculates the width of the image so that
    the ratio of the image is preserved.

    If the given \a height is 0 or negative, a null image is returned.

    \sa {QImage#Image Transformations}{Image Transformations}
*/
QImage QImage::scaledToHeight(int h, Qt::TransformationMode mode) const
{
    if (!d) {
        qWarning("QImage::scaleHeight: Image is a null image");
        return QImage();
    }
    if (h <= 0)
        return QImage();

    qreal factor = (qreal) h / height();
    QTransform wm = QTransform::fromScale(factor, factor);
    return transformed(wm, mode);
}


/*!
    \fn QMatrix QImage::trueMatrix(const QMatrix &matrix, int width, int height)

    Returns the actual matrix used for transforming an image with the
    given \a width, \a height and \a matrix.

    When transforming an image using the transformed() function, the
    transformation matrix is internally adjusted to compensate for
    unwanted translation, i.e. transformed() returns the smallest
    image containing all transformed points of the original image.
    This function returns the modified matrix, which maps points
    correctly from the original image into the new image.

    \sa transformed(), {QImage#Image Transformations}{Image
    Transformations}
*/
QMatrix QImage::trueMatrix(const QMatrix &matrix, int w, int h)
{
    return trueMatrix(QTransform(matrix), w, h).toAffine();
}

/*!
    Returns a copy of the image that is transformed using the given
    transformation \a matrix and transformation \a mode.

    The transformation \a matrix is internally adjusted to compensate
    for unwanted translation; i.e. the image produced is the smallest
    image that contains all the transformed points of the original
    image. Use the trueMatrix() function to retrieve the actual matrix
    used for transforming an image.

    \sa trueMatrix(), {QImage#Image Transformations}{Image
    Transformations}
*/
QImage QImage::transformed(const QMatrix &matrix, Qt::TransformationMode mode) const
{
    return transformed(QTransform(matrix), mode);
}

/*!
    Builds and returns a 1-bpp mask from the alpha buffer in this
    image. Returns a null image if the image's format is
    QImage::Format_RGB32.

    The \a flags argument is a bitwise-OR of the
    Qt::ImageConversionFlags, and controls the conversion
    process. Passing 0 for flags sets all the default options.

    The returned image has little-endian bit order (i.e. the image's
    format is QImage::Format_MonoLSB), which you can convert to
    big-endian (QImage::Format_Mono) using the convertToFormat()
    function.

    \sa createHeuristicMask(), {QImage#Image Transformations}{Image
    Transformations}
*/
QImage QImage::createAlphaMask(Qt::ImageConversionFlags flags) const
{
    if (!d || d->format == QImage::Format_RGB32)
        return QImage();

    if (d->depth == 1) {
        // A monochrome pixmap, with alpha channels on those two colors.
        // Pretty unlikely, so use less efficient solution.
        return convertToFormat(Format_Indexed8, flags).createAlphaMask(flags);
    }

    QImage mask(d->width, d->height, Format_MonoLSB);
    if (!mask.isNull())
        dither_to_Mono(mask.d, d, flags, true);
    return mask;
}

#ifndef QT_NO_IMAGE_HEURISTIC_MASK
/*!
    Creates and returns a 1-bpp heuristic mask for this image.

    The function works by selecting a color from one of the corners,
    then chipping away pixels of that color starting at all the edges.
    The four corners vote for which color is to be masked away. In
    case of a draw (this generally means that this function is not
    applicable to the image), the result is arbitrary.

    The returned image has little-endian bit order (i.e. the image's
    format is QImage::Format_MonoLSB), which you can convert to
    big-endian (QImage::Format_Mono) using the convertToFormat()
    function.

    If \a clipTight is true (the default) the mask is just large
    enough to cover the pixels; otherwise, the mask is larger than the
    data pixels.

    Note that this function disregards the alpha buffer.

    \sa createAlphaMask(), {QImage#Image Transformations}{Image
    Transformations}
*/

QImage QImage::createHeuristicMask(bool clipTight) const
{
    if (!d)
        return QImage();

    if (d->depth != 32) {
        QImage img32 = convertToFormat(Format_RGB32);
        return img32.createHeuristicMask(clipTight);
    }

#define PIX(x,y)  (*((QRgb*)scanLine(y)+x) & 0x00ffffff)

    int w = width();
    int h = height();
    QImage m(w, h, Format_MonoLSB);
    QIMAGE_SANITYCHECK_MEMORY(m);
    m.setColorCount(2);
    m.setColor(0, QColor(Qt::color0).rgba());
    m.setColor(1, QColor(Qt::color1).rgba());
    m.fill(0xff);

    QRgb background = PIX(0,0);
    if (background != PIX(w-1,0) &&
         background != PIX(0,h-1) &&
         background != PIX(w-1,h-1)) {
        background = PIX(w-1,0);
        if (background != PIX(w-1,h-1) &&
             background != PIX(0,h-1) &&
             PIX(0,h-1) == PIX(w-1,h-1)) {
            background = PIX(w-1,h-1);
        }
    }

    int x,y;
    bool done = false;
    uchar *ypp, *ypc, *ypn;
    while(!done) {
        done = true;
        ypn = m.scanLine(0);
        ypc = 0;
        for (y = 0; y < h; y++) {
            ypp = ypc;
            ypc = ypn;
            ypn = (y == h-1) ? 0 : m.scanLine(y+1);
            QRgb *p = (QRgb *)scanLine(y);
            for (x = 0; x < w; x++) {
                // slowness here - it's possible to do six of these tests
                // together in one go. oh well.
                if ((x == 0 || y == 0 || x == w-1 || y == h-1 ||
                       !(*(ypc + ((x-1) >> 3)) & (1 << ((x-1) & 7))) ||
                       !(*(ypc + ((x+1) >> 3)) & (1 << ((x+1) & 7))) ||
                       !(*(ypp + (x     >> 3)) & (1 << (x     & 7))) ||
                       !(*(ypn + (x     >> 3)) & (1 << (x     & 7)))) &&
                     (       (*(ypc + (x     >> 3)) & (1 << (x     & 7)))) &&
                     ((*p & 0x00ffffff) == background)) {
                    done = false;
                    *(ypc + (x >> 3)) &= ~(1 << (x & 7));
                }
                p++;
            }
        }
    }

    if (!clipTight) {
        ypn = m.scanLine(0);
        ypc = 0;
        for (y = 0; y < h; y++) {
            ypp = ypc;
            ypc = ypn;
            ypn = (y == h-1) ? 0 : m.scanLine(y+1);
            QRgb *p = (QRgb *)scanLine(y);
            for (x = 0; x < w; x++) {
                if ((*p & 0x00ffffff) != background) {
                    if (x > 0)
                        *(ypc + ((x-1) >> 3)) |= (1 << ((x-1) & 7));
                    if (x < w-1)
                        *(ypc + ((x+1) >> 3)) |= (1 << ((x+1) & 7));
                    if (y > 0)
                        *(ypp + (x >> 3)) |= (1 << (x & 7));
                    if (y < h-1)
                        *(ypn + (x >> 3)) |= (1 << (x & 7));
                }
                p++;
            }
        }
    }

#undef PIX

    return m;
}
#endif //QT_NO_IMAGE_HEURISTIC_MASK

/*!
    Creates and returns a mask for this image based on the given \a
    color value. If the \a mode is MaskInColor (the default value),
    all pixels matching \a color will be opaque pixels in the mask. If
    \a mode is MaskOutColor, all pixels matching the given color will
    be transparent.

    \sa createAlphaMask(), createHeuristicMask()
*/

QImage QImage::createMaskFromColor(QRgb color, Qt::MaskMode mode) const
{
    if (!d)
        return QImage();
    QImage maskImage(size(), QImage::Format_MonoLSB);
    QIMAGE_SANITYCHECK_MEMORY(maskImage);
    maskImage.fill(0);
    uchar *s = maskImage.bits();

    if (depth() == 32) {
        for (int h = 0; h < d->height; h++) {
            const uint *sl = (uint *) scanLine(h);
            for (int w = 0; w < d->width; w++) {
                if (sl[w] == color)
                    *(s + (w >> 3)) |= (1 << (w & 7));
            }
            s += maskImage.bytesPerLine();
        }
    } else {
        for (int h = 0; h < d->height; h++) {
            for (int w = 0; w < d->width; w++) {
                if ((uint) pixel(w, h) == color)
                    *(s + (w >> 3)) |= (1 << (w & 7));
            }
            s += maskImage.bytesPerLine();
        }
    }
    if  (mode == Qt::MaskOutColor)
        maskImage.invertPixels();
    return maskImage;
}


/*
  This code is contributed by Philipp Lang,
  GeneriCom Software Germany (www.generi.com)
  under the terms of the QPL, Version 1.0
*/

/*!
    \fn QImage QImage::mirror(bool horizontal, bool vertical) const

    Use mirrored() instead.
*/

/*!
    Returns a mirror of the image, mirrored in the horizontal and/or
    the vertical direction depending on whether \a horizontal and \a
    vertical are set to true or false.

    Note that the original image is not changed.

    \sa {QImage#Image Transformations}{Image Transformations}
*/
QImage QImage::mirrored(bool horizontal, bool vertical) const
{
    if (!d)
        return QImage();

    if ((d->width <= 1 && d->height <= 1) || (!horizontal && !vertical))
        return *this;

    int w = d->width;
    int h = d->height;
    // Create result image, copy colormap
    QImage result(d->width, d->height, d->format);
    QIMAGE_SANITYCHECK_MEMORY(result);

    // check if we ran out of of memory..
    if (!result.d)
        return QImage();

    result.d->colortable = d->colortable;
    result.d->has_alpha_clut = d->has_alpha_clut;

    if (depth() == 1)
        w = (w+7)/8;
    int dxi = horizontal ? -1 : 1;
    int dxs = horizontal ? w-1 : 0;
    int dyi = vertical ? -1 : 1;
    int dy = vertical ? h-1: 0;

    // 1 bit, 8 bit
    if (d->depth == 1 || d->depth == 8) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint8* ssl = (quint8*)(d->data + sy*d->bytes_per_line);
            quint8* dsl = (quint8*)(result.d->data + dy*result.d->bytes_per_line);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
    // 16 bit
    else if (d->depth == 16) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint16* ssl = (quint16*)(d->data + sy*d->bytes_per_line);
            quint16* dsl = (quint16*)(result.d->data + dy*result.d->bytes_per_line);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
    // 24 bit
    else if (d->depth == 24) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint24* ssl = (quint24*)(d->data + sy*d->bytes_per_line);
            quint24* dsl = (quint24*)(result.d->data + dy*result.d->bytes_per_line);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
    // 32 bit
    else if (d->depth == 32) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint32* ssl = (quint32*)(d->data + sy*d->bytes_per_line);
            quint32* dsl = (quint32*)(result.d->data + dy*result.d->bytes_per_line);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }

    // special handling of 1 bit images for horizontal mirroring
    if (horizontal && d->depth == 1) {
        int shift = width() % 8;
        for (int y = h-1; y >= 0; y--) {
            quint8* a0 = (quint8*)(result.d->data + y*d->bytes_per_line);
            // Swap bytes
            quint8* a = a0+dxs;
            while (a >= a0) {
                *a = bitflip[*a];
                a--;
            }
            // Shift bits if unaligned
            if (shift != 0) {
                a = a0+dxs;
                quint8 c = 0;
                if (format() == Format_MonoLSB) {
                    while (a >= a0) {
                        quint8 nc = *a << shift;
                        *a = (*a >> (8-shift)) | c;
                        --a;
                        c = nc;
                    }
                } else {
                    while (a >= a0) {
                        quint8 nc = *a >> shift;
                        *a = (*a << (8-shift)) | c;
                        --a;
                        c = nc;
                    }
                }
            }
        }
    }

    return result;
}

/*!
    \fn QImage QImage::swapRGB() const

    Use rgbSwapped() instead.

    \omit
    Returns a QImage in which the values of the red and blue
    components of all pixels have been swapped, effectively converting
    an RGB image to an BGR image. The original QImage is not changed.
    \endomit
*/

/*!
    Returns a QImage in which the values of the red and blue
    components of all pixels have been swapped, effectively converting
    an RGB image to an BGR image.

    The original QImage is not changed.

    \sa {QImage#Image Transformations}{Image Transformations}
*/
QImage QImage::rgbSwapped() const
{
    if (isNull())
        return *this;
    QImage res;
    switch (d->format) {
    case Format_Invalid:
    case NImageFormats:
        Q_ASSERT(false);
        break;
    case Format_Mono:
    case Format_MonoLSB:
    case Format_Indexed8:
        res = copy();
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < res.d->colortable.size(); i++) {
            QRgb c = res.d->colortable.at(i);
            res.d->colortable[i] = QRgb(((c << 16) & 0xff0000) | ((c >> 16) & 0xff) | (c & 0xff00ff00));
        }
        break;
    case Format_RGB32:
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            uint *q = (uint*)res.scanLine(i);
            uint *p = (uint*)constScanLine(i);
            uint *end = p + d->width;
            while (p < end) {
                *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) | (*p & 0xff00ff00);
                p++;
                q++;
            }
        }
        break;
    case Format_RGB16:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            ushort *q = (ushort*)res.scanLine(i);
            const ushort *p = (const ushort*)constScanLine(i);
            const ushort *end = p + d->width;
            while (p < end) {
                *q = ((*p << 11) & 0xf800) | ((*p >> 11) & 0x1f) | (*p & 0x07e0);
                p++;
                q++;
            }
        }
        break;
    case Format_ARGB8565_Premultiplied:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            const quint8 *p = constScanLine(i);
            quint8 *q = res.scanLine(i);
            const quint8 *end = p + d->width * sizeof(qargb8565);
            while (p < end) {
                q[0] = p[0];
                q[1] = (p[1] & 0xe0) | (p[2] >> 3);
                q[2] = (p[2] & 0x07) | (p[1] << 3);
                p += sizeof(qargb8565);
                q += sizeof(qargb8565);
            }
        }
        break;
    case Format_RGB666:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            qrgb666 *q = reinterpret_cast<qrgb666*>(res.scanLine(i));
            const qrgb666 *p = reinterpret_cast<const qrgb666*>(constScanLine(i));
            const qrgb666 *end = p + d->width;
            while (p < end) {
                const QRgb rgb = quint32(*p++);
                *q++ = qRgb(qBlue(rgb), qGreen(rgb), qRed(rgb));
            }
        }
        break;
    case Format_ARGB6666_Premultiplied:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            const quint8 *p = constScanLine(i);
            const quint8 *end = p + d->width * sizeof(qargb6666);
            quint8 *q = res.scanLine(i);
            while (p < end) {
                q[0] = (p[1] >> 4) | ((p[2] & 0x3) << 4) | (p[0] & 0xc0);
                q[1] = (p[1] & 0xf) | (p[0] << 4);
                q[2] = (p[2] & 0xfc) | ((p[0] >> 4) & 0x3);
                p += sizeof(qargb6666);
                q += sizeof(qargb6666);
            }
        }
        break;
    case Format_RGB555:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            quint16 *q = (quint16*)res.scanLine(i);
            const quint16 *p = (const quint16*)constScanLine(i);
            const quint16 *end = p + d->width;
            while (p < end) {
                *q = ((*p << 10) & 0x7c00) | ((*p >> 10) & 0x1f) | (*p & 0x3e0);
                p++;
                q++;
            }
        }
        break;
    case Format_ARGB8555_Premultiplied:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            const quint8 *p = constScanLine(i);
            quint8 *q = res.scanLine(i);
            const quint8 *end = p + d->width * sizeof(qargb8555);
            while (p < end) {
                q[0] = p[0];
                q[1] = (p[1] & 0xe0) | (p[2] >> 2);
                q[2] = (p[2] & 0x03) | ((p[1] << 2) & 0x7f);
                p += sizeof(qargb8555);
                q += sizeof(qargb8555);
            }
        }
        break;
    case Format_RGB888:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            quint8 *q = res.scanLine(i);
            const quint8 *p = constScanLine(i);
            const quint8 *end = p + d->width * sizeof(qrgb888);
            while (p < end) {
                q[0] = p[2];
                q[1] = p[1];
                q[2] = p[0];
                q += sizeof(qrgb888);
                p += sizeof(qrgb888);
            }
        }
        break;
    case Format_RGB444:
    case Format_ARGB4444_Premultiplied:
        res = QImage(d->width, d->height, d->format);
        QIMAGE_SANITYCHECK_MEMORY(res);
        for (int i = 0; i < d->height; i++) {
            quint16 *q = reinterpret_cast<quint16*>(res.scanLine(i));
            const quint16 *p = reinterpret_cast<const quint16*>(constScanLine(i));
            const quint16 *end = p + d->width;
            while (p < end) {
                *q = (*p & 0xf0f0) | ((*p & 0x0f) << 8) | ((*p & 0xf00) >> 8);
                p++;
                q++;
            }
        }
        break;
    }
    return res;
}

/*!
    Loads an image from the file with the given \a fileName. Returns true if
    the image was successfully loaded; otherwise returns false.

    The loader attempts to read the image using the specified \a format, e.g.,
    PNG or JPG. If \a format is not specified (which is the default), the
    loader probes the file for a header to guess the file format.

    The file name can either refer to an actual file on disk or to one
    of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    \sa {QImage#Reading and Writing Image Files}{Reading and Writing Image Files}
*/

bool QImage::load(const QString &fileName, const char* format)
{
    if (fileName.isEmpty())
        return false;

    QImage image = QImageReader(fileName, format).read();
    if (!image.isNull()) {
        operator=(image);
        return true;
    }
    return false;
}

/*!
    \overload

    This function reads a QImage from the given \a device. This can,
    for example, be used to load an image directly into a QByteArray.
*/

bool QImage::load(QIODevice* device, const char* format)
{
    QImage image = QImageReader(device, format).read();
    if(!image.isNull()) {
        operator=(image);
        return true;
    }
    return false;
}

/*!
    \fn bool QImage::loadFromData(const uchar *data, int len, const char *format)

    Loads an image from the first \a len bytes of the given binary \a
    data. Returns true if the image was successfully loaded; otherwise
    returns false.

    The loader attempts to read the image using the specified \a format, e.g.,
    PNG or JPG. If \a format is not specified (which is the default), the
    loader probes the file for a header to guess the file format.

    \sa {QImage#Reading and Writing Image Files}{Reading and Writing Image Files}
*/

bool QImage::loadFromData(const uchar *data, int len, const char *format)
{
    QImage image = fromData(data, len, format);
    if (!image.isNull()) {
        operator=(image);
        return true;
    }
    return false;
}

/*!
    \fn bool QImage::loadFromData(const QByteArray &data, const char *format)

    \overload

    Loads an image from the given QByteArray \a data.
*/

/*!
    \fn QImage QImage::fromData(const uchar *data, int size, const char *format)

    Constructs a QImage from the first \a size bytes of the given
    binary \a data. The loader attempts to read the image using the
    specified \a format. If \a format is not specified (which is the default),
    the loader probes the file for a header to guess the file format.
    binary \a data. The loader attempts to read the image, either using the
    optional image \a format specified or by determining the image format from
    the data.

    If \a format is not specified (which is the default), the loader probes the
    file for a header to determine the file format. If \a format is specified,
    it must be one of the values returned by QImageReader::supportedImageFormats().

    If the loading of the image fails, the image returned will be a null image.

    \sa load(), save(), {QImage#Reading and Writing Image Files}{Reading and Writing Image Files}
 */

QImage QImage::fromData(const uchar *data, int size, const char *format)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(data), size);
    QBuffer b;
    b.setData(a);
    b.open(QIODevice::ReadOnly);
    return QImageReader(&b, format).read();
}

/*!
    \fn QImage QImage::fromData(const QByteArray &data, const char *format)

    \overload

    Loads an image from the given QByteArray \a data.
*/

/*!
    Saves the image to the file with the given \a fileName, using the
    given image file \a format and \a quality factor. If \a format is
    0, QImage will attempt to guess the format by looking at \a fileName's
    suffix.

    The \a quality factor must be in the range 0 to 100 or -1. Specify
    0 to obtain small compressed files, 100 for large uncompressed
    files, and -1 (the default) to use the default settings.

    Returns true if the image was successfully saved; otherwise
    returns false.

    \sa {QImage#Reading and Writing Image Files}{Reading and Writing
    Image Files}
*/
bool QImage::save(const QString &fileName, const char *format, int quality) const
{
    if (isNull())
        return false;
    QImageWriter writer(fileName, format);
    return d->doImageIO(this, &writer, quality);
}

/*!
    \overload

    This function writes a QImage to the given \a device.

    This can, for example, be used to save an image directly into a
    QByteArray:

    \snippet doc/src/snippets/image/image.cpp 0
*/

bool QImage::save(QIODevice* device, const char* format, int quality) const
{
    if (isNull())
        return false;                                // nothing to save
    QImageWriter writer(device, format);
    return d->doImageIO(this, &writer, quality);
}

/* \internal
*/

bool QImageData::doImageIO(const QImage *image, QImageWriter *writer, int quality) const
{
    if (quality > 100  || quality < -1)
        qWarning("QPixmap::save: Quality out of range [-1, 100]");
    if (quality >= 0)
        writer->setQuality(qMin(quality,100));
    return writer->write(*image);
}

/*****************************************************************************
  QImage stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QImage &image)
    \relates QImage

    Writes the given \a image to the given \a stream as a PNG image,
    or as a BMP image if the stream's version is 1. Note that writing
    the stream to a file will not produce a valid image file.

    \sa QImage::save(), {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QImage &image)
{
    if (s.version() >= 5) {
        if (image.isNull()) {
            s << (qint32) 0; // null image marker
            return s;
        } else {
            s << (qint32) 1;
            // continue ...
        }
    }
    QImageWriter writer(s.device(), s.version() == 1 ? "bmp" : "png");
    writer.write(image);
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QImage &image)
    \relates QImage

    Reads an image from the given \a stream and stores it in the given
    \a image.

    \sa QImage::load(), {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QImage &image)
{
    if (s.version() >= 5) {
        qint32 nullMarker;
        s >> nullMarker;
        if (!nullMarker) {
            image = QImage(); // null image
            return s;
        }
    }
    image = QImageReader(s.device(), 0).read();
    return s;
}
#endif // QT_NO_DATASTREAM


#ifdef QT3_SUPPORT
/*!
    \fn QImage QImage::convertDepthWithPalette(int depth, QRgb* palette, int palette_count, Qt::ImageConversionFlags flags) const

    Returns an image with the given \a depth, using the \a
    palette_count colors pointed to by \a palette. If \a depth is 1 or
    8, the returned image will have its color table ordered in the
    same way as \a palette.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a flags to
    specify how you'd prefer this to happen.

    Note: currently no closest-color search is made. If colors are
    found that are not in the palette, the palette may not be used at
    all. This result should not be considered valid because it may
    change in future implementations.

    Currently inefficient for non-32-bit images.

    Use the convertToFormat() function in combination with the
    setColorTable() function instead.
*/
QImage QImage::convertDepthWithPalette(int d, QRgb* palette, int palette_count, Qt::ImageConversionFlags flags) const
{
    Format f = formatFor(d, QImage::LittleEndian);
    QVector<QRgb> colortable;
    for (int i = 0; i < palette_count; ++i)
        colortable.append(palette[i]);
    return convertToFormat(f, colortable, flags);
}

/*!
    \relates QImage

    Copies a block of pixels from \a src to \a dst. The pixels
    copied from source (src) are converted according to
    \a flags if it is incompatible with the destination
    (\a dst).

    \a sx, \a sy is the top-left pixel in \a src, \a dx, \a dy is the
    top-left position in \a dst and \a sw, \a sh is the size of the
    copied block. The copying is clipped if areas outside \a src or \a
    dst are specified. If \a sw is -1, it is adjusted to
    src->width(). Similarly, if \a sh is -1, it is adjusted to
    src->height().

    Currently inefficient for non 32-bit images.

    Use copy() or QPainter::drawImage() instead.
*/
void bitBlt(QImage *dst, int dx, int dy, const QImage *src, int sx, int sy, int sw, int sh,
            Qt::ImageConversionFlags flags)
{
    if (dst->isNull() || src->isNull())
        return;
    QPainter p(dst);
    p.drawImage(QPoint(dx, dy), *src, QRect(sx, sy, sw, sh), flags);
}
#endif

/*!
    \fn bool QImage::operator==(const QImage & image) const

    Returns true if this image and the given \a image have the same
    contents; otherwise returns false.

    The comparison can be slow, unless there is some obvious
    difference (e.g. different size or format), in which case the
    function will return quickly.

    \sa operator=()
*/

bool QImage::operator==(const QImage & i) const
{
    // same object, or shared?
    if (i.d == d)
        return true;
    if (!i.d || !d)
        return false;

    // obviously different stuff?
    if (i.d->height != d->height || i.d->width != d->width || i.d->format != d->format)
        return false;

    if (d->format != Format_RGB32) {
        if (d->format >= Format_ARGB32) { // all bits defined
            const int n = d->width * d->depth / 8;
            if (n == d->bytes_per_line && n == i.d->bytes_per_line) {
                if (memcmp(bits(), i.bits(), d->nbytes))
                    return false;
            } else {
                for (int y = 0; y < d->height; ++y) {
                    if (memcmp(scanLine(y), i.scanLine(y), n))
                        return false;
                }
            }
        } else {
            const int w = width();
            const int h = height();
            const QVector<QRgb> &colortable = d->colortable;
            const QVector<QRgb> &icolortable = i.d->colortable;
            for (int y=0; y<h; ++y) {
                for (int x=0; x<w; ++x) {
                    if (colortable[pixelIndex(x, y)] != icolortable[i.pixelIndex(x, y)])
                        return false;
                }
            }
        }
    } else {
        //alpha channel undefined, so we must mask it out
        for(int l = 0; l < d->height; l++) {
            int w = d->width;
            const uint *p1 = reinterpret_cast<const uint*>(scanLine(l));
            const uint *p2 = reinterpret_cast<const uint*>(i.scanLine(l));
            while (w--) {
                if ((*p1++ & 0x00ffffff) != (*p2++ & 0x00ffffff))
                    return false;
            }
        }
    }
    return true;
}


/*!
    \fn bool QImage::operator!=(const QImage & image) const

    Returns true if this image and the given \a image have different
    contents; otherwise returns false.

    The comparison can be slow, unless there is some obvious
    difference, such as different widths, in which case the function
    will return quickly.

    \sa operator=()
*/

bool QImage::operator!=(const QImage & i) const
{
    return !(*this == i);
}




/*!
    Returns the number of pixels that fit horizontally in a physical
    meter. Together with dotsPerMeterY(), this number defines the
    intended scale and aspect ratio of the image.

    \sa setDotsPerMeterX(), {QImage#Image Information}{Image
    Information}
*/
int QImage::dotsPerMeterX() const
{
    return d ? qRound(d->dpmx) : 0;
}

/*!
    Returns the number of pixels that fit vertically in a physical
    meter. Together with dotsPerMeterX(), this number defines the
    intended scale and aspect ratio of the image.

    \sa setDotsPerMeterY(), {QImage#Image Information}{Image
    Information}
*/
int QImage::dotsPerMeterY() const
{
    return d ? qRound(d->dpmy) : 0;
}

/*!
    Sets the number of pixels that fit horizontally in a physical
    meter, to \a x.

    Together with dotsPerMeterY(), this number defines the intended
    scale and aspect ratio of the image, and determines the scale
    at which QPainter will draw graphics on the image. It does not
    change the scale or aspect ratio of the image when it is rendered
    on other paint devices.

    \sa dotsPerMeterX(), {QImage#Image Information}{Image Information}
*/
void QImage::setDotsPerMeterX(int x)
{
    if (!d || !x)
        return;
    detach();

    if (d)
        d->dpmx = x;
}

/*!
    Sets the number of pixels that fit vertically in a physical meter,
    to \a y.

    Together with dotsPerMeterX(), this number defines the intended
    scale and aspect ratio of the image, and determines the scale
    at which QPainter will draw graphics on the image. It does not
    change the scale or aspect ratio of the image when it is rendered
    on other paint devices.

    \sa dotsPerMeterY(), {QImage#Image Information}{Image Information}
*/
void QImage::setDotsPerMeterY(int y)
{
    if (!d || !y)
        return;
    detach();

    if (d)
        d->dpmy = y;
}

/*!
    \fn QPoint QImage::offset() const

    Returns the number of pixels by which the image is intended to be
    offset by when positioning relative to other images.

    \sa setOffset(), {QImage#Image Information}{Image Information}
*/
QPoint QImage::offset() const
{
    return d ? d->offset : QPoint();
}


/*!
    \fn void QImage::setOffset(const QPoint& offset)

    Sets the number of pixels by which the image is intended to be
    offset by when positioning relative to other images, to \a offset.

    \sa offset(), {QImage#Image Information}{Image Information}
*/
void QImage::setOffset(const QPoint& p)
{
    if (!d)
        return;
    detach();

    if (d)
        d->offset = p;
}
#ifndef QT_NO_IMAGE_TEXT

/*!
    Returns the text keys for this image.

    You can use these keys with text() to list the image text for a
    certain key.

    \sa text()
*/
QStringList QImage::textKeys() const
{
    return d ? QStringList(d->text.keys()) : QStringList();
}

/*!
    Returns the image text associated with the given \a key. If the
    specified \a key is an empty string, the whole image text is
    returned, with each key-text pair separated by a newline.

    \sa setText(), textKeys()
*/
QString QImage::text(const QString &key) const
{
    if (!d)
        return QString();

    if (!key.isEmpty())
        return d->text.value(key);

    QString tmp;
    foreach (const QString &key, d->text.keys()) {
        if (!tmp.isEmpty())
            tmp += QLatin1String("\n\n");
        tmp += key + QLatin1String(": ") + d->text.value(key).simplified();
    }
    return tmp;
}

/*!
    \fn void QImage::setText(const QString &key, const QString &text)

    Sets the image text to the given \a text and associate it with the
    given \a key.

    If you just want to store a single text block (i.e., a "comment"
    or just a description), you can either pass an empty key, or use a
    generic key like "Description".

    The image text is embedded into the image data when you
    call save() or QImageWriter::write().

    Not all image formats support embedded text. You can find out
    if a specific image or format supports embedding text
    by using QImageWriter::supportsOption(). We give an example:

    \snippet doc/src/snippets/image/supportedformat.cpp 0

    You can use QImageWriter::supportedImageFormats() to find out
    which image formats are available to you.

    \sa text(), textKeys()
*/
void QImage::setText(const QString &key, const QString &value)
{
    if (!d)
        return;
    detach();

    if (d)
        d->text.insert(key, value);
}

/*!
    \fn QString QImage::text(const char* key, const char* language) const
    \obsolete

    Returns the text recorded for the given \a key in the given \a
    language, or in a default language if \a language is 0.

    Use text() instead.

    The language the text is recorded in is no longer relevant since
    the text is always set using QString and UTF-8 representation.
*/
QString QImage::text(const char* key, const char* lang) const
{
    if (!d)
        return QString();
    QString k = QString::fromAscii(key);
    if (lang && *lang)
        k += QLatin1Char('/') + QString::fromAscii(lang);
    return d->text.value(k);
}

/*!
    \fn QString QImage::text(const QImageTextKeyLang& keywordAndLanguage) const
    \overload
    \obsolete

    Returns the text recorded for the given \a keywordAndLanguage.

    Use text() instead.

    The language the text is recorded in is no longer relevant since
    the text is always set using QString and UTF-8 representation.
*/
QString QImage::text(const QImageTextKeyLang& kl) const
{
    if (!d)
        return QString();
    QString k = QString::fromAscii(kl.key);
    if (!kl.lang.isEmpty())
        k += QLatin1Char('/') + QString::fromAscii(kl.lang);
    return d->text.value(k);
}

/*!
    \obsolete

    Returns the language identifiers for which some texts are
    recorded. Note that if you want to iterate over the list, you
    should iterate over a copy.

    The language the text is recorded in is no longer relevant since
    the text is always set using QString and UTF-8 representation.
*/
QStringList QImage::textLanguages() const
{
    if (!d)
        return QStringList();
    QStringList keys = textKeys();
    QStringList languages;
    for (int i = 0; i < keys.size(); ++i) {
        int index = keys.at(i).indexOf(QLatin1Char('/'));
        if (index > 0)
            languages += keys.at(i).mid(index+1);
    }

    return languages;
}

/*!
    \obsolete

    Returns a list of QImageTextKeyLang objects that enumerate all the
    texts key/language pairs set for this image.

    Use textKeys() instead.

    The language the text is recorded in is no longer relevant since
    the text is always set using QString and UTF-8 representation.
*/
QList<QImageTextKeyLang> QImage::textList() const
{
    QList<QImageTextKeyLang> imageTextKeys;
    if (!d)
        return imageTextKeys;
    QStringList keys = textKeys();
    for (int i = 0; i < keys.size(); ++i) {
        int index = keys.at(i).indexOf(QLatin1Char('/'));
        if (index > 0) {
            QImageTextKeyLang tkl;
            tkl.key = keys.at(i).left(index).toAscii();
            tkl.lang = keys.at(i).mid(index+1).toAscii();
            imageTextKeys += tkl;
        }
    }

    return imageTextKeys;
}

/*!
    \fn void QImage::setText(const char* key, const char* language, const QString& text)
    \obsolete

    Sets the image text to the given \a text and associate it with the
    given \a key. The text is recorded in the specified \a language,
    or in a default language if \a language is 0.

    Use setText() instead.

    The language the text is recorded in is no longer relevant since
    the text is always set using QString and UTF-8 representation.

    \omit
    Records string \a  for the keyword \a key. The \a key should be
    a portable keyword recognizable by other software - some suggested
    values can be found in
    \l{http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#C.Anc-text}
    {the PNG specification}. \a s can be any text. \a lang should
    specify the language code (see
    \l{http://www.rfc-editor.org/rfc/rfc1766.txt}{RFC 1766}) or 0.
    \endomit
*/
void QImage::setText(const char* key, const char* lang, const QString& s)
{
    if (!d)
        return;
    detach();

    // In case detach() ran out of memory
    if (!d)
        return;

    QString k = QString::fromAscii(key);
    if (lang && *lang)
        k += QLatin1Char('/') + QString::fromAscii(lang);
    d->text.insert(k, s);
}

#endif // QT_NO_IMAGE_TEXT

/*
    Sets the image bits to the \a pixmap contents and returns a
    reference to the image.

    If the image shares data with other images, it will first
    dereference the shared data.

    Makes a call to QPixmap::convertToImage().
*/

/*! \fn QImage::Endian QImage::systemBitOrder()

    Determines the bit order of the display hardware. Returns
    QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).

    This function is no longer relevant for QImage. Use QSysInfo
    instead.
*/


/*!
    \internal

    Used by QPainter to retrieve a paint engine for the image.
*/

QPaintEngine *QImage::paintEngine() const
{
    if (!d)
        return 0;

    if (!d->paintEngine) {
#ifdef Q_OS_SYMBIAN
        d->paintEngine = new QSymbianRasterPaintEngine(const_cast<QImage *>(this));
#else
        d->paintEngine = new QRasterPaintEngine(const_cast<QImage *>(this));
#endif
    }

    return d->paintEngine;
}


/*!
    \internal

    Returns the size for the specified \a metric on the device.
*/
int QImage::metric(PaintDeviceMetric metric) const
{
    if (!d)
        return 0;

    switch (metric) {
    case PdmWidth:
        return d->width;
        break;

    case PdmHeight:
        return d->height;
        break;

    case PdmWidthMM:
        return qRound(d->width * 1000 / d->dpmx);
        break;

    case PdmHeightMM:
        return qRound(d->height * 1000 / d->dpmy);
        break;

    case PdmNumColors:
        return d->colortable.size();
        break;

    case PdmDepth:
        return d->depth;
        break;

    case PdmDpiX:
        return qRound(d->dpmx * 0.0254);
        break;

    case PdmDpiY:
        return qRound(d->dpmy * 0.0254);
        break;

    case PdmPhysicalDpiX:
        return qRound(d->dpmx * 0.0254);
        break;

    case PdmPhysicalDpiY:
        return qRound(d->dpmy * 0.0254);
        break;

    default:
        qWarning("QImage::metric(): Unhandled metric type %d", metric);
        break;
    }
    return 0;
}



/*****************************************************************************
  QPixmap (and QImage) helper functions
 *****************************************************************************/
/*
  This internal function contains the common (i.e. platform independent) code
  to do a transformation of pixel data. It is used by QPixmap::transform() and by
  QImage::transform().

  \a trueMat is the true transformation matrix (see QPixmap::trueMatrix()) and
  \a xoffset is an offset to the matrix.

  \a msbfirst specifies for 1bpp images, if the MSB or LSB comes first and \a
  depth specifies the colordepth of the data.

  \a dptr is a pointer to the destination data, \a dbpl specifies the bits per
  line for the destination data, \a p_inc is the offset that we advance for
  every scanline and \a dHeight is the height of the destination image.

  \a sprt is the pointer to the source data, \a sbpl specifies the bits per
  line of the source data, \a sWidth and \a sHeight are the width and height of
  the source data.
*/

#undef IWX_MSB
#define IWX_MSB(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if (*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &                      \
                                 (1 << (7-((trigx>>12)&7))))                              \
                                *dptr |= b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
#undef IWX_LSB
#define IWX_LSB(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if (*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &                      \
                                 (1 << ((trigx>>12)&7)))                              \
                                *dptr |= b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
#undef IWX_PIX
#define IWX_PIX(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if ((*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &              \
                                 (1 << (7-((trigx>>12)&7)))) == 0)                      \
                                *dptr &= ~b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
bool qt_xForm_helper(const QTransform &trueMat, int xoffset, int type, int depth,
                     uchar *dptr, int dbpl, int p_inc, int dHeight,
                     const uchar *sptr, int sbpl, int sWidth, int sHeight)
{
    int m11 = int(trueMat.m11()*qreal(4096.0));
    int m12 = int(trueMat.m12()*qreal(4096.0));
    int m21 = int(trueMat.m21()*qreal(4096.0));
    int m22 = int(trueMat.m22()*qreal(4096.0));
    int dx  = qRound(trueMat.dx()*qreal(4096.0));
    int dy  = qRound(trueMat.dy()*qreal(4096.0));

    int m21ydx = dx + (xoffset<<16) + (m11 + m21) / 2;
    int m22ydy = dy + (m12 + m22) / 2;
    uint trigx;
    uint trigy;
    uint maxws = sWidth<<12;
    uint maxhs = sHeight<<12;

    for (int y=0; y<dHeight; y++) {                // for each target scanline
        trigx = m21ydx;
        trigy = m22ydy;
        uchar *maxp = dptr + dbpl;
        if (depth != 1) {
            switch (depth) {
                case 8:                                // 8 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *dptr = *(sptr+sbpl*(trigy>>12)+(trigx>>12));
                    trigx += m11;
                    trigy += m12;
                    dptr++;
                }
                break;

                case 16:                        // 16 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *((ushort*)dptr) = *((ushort *)(sptr+sbpl*(trigy>>12) +
                                                     ((trigx>>12)<<1)));
                    trigx += m11;
                    trigy += m12;
                    dptr++;
                    dptr++;
                }
                break;

                case 24:                        // 24 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs) {
                        const uchar *p2 = sptr+sbpl*(trigy>>12) + ((trigx>>12)*3);
                        dptr[0] = p2[0];
                        dptr[1] = p2[1];
                        dptr[2] = p2[2];
                    }
                    trigx += m11;
                    trigy += m12;
                    dptr += 3;
                }
                break;

                case 32:                        // 32 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *((uint*)dptr) = *((uint *)(sptr+sbpl*(trigy>>12) +
                                                   ((trigx>>12)<<2)));
                    trigx += m11;
                    trigy += m12;
                    dptr += 4;
                }
                break;

                default: {
                return false;
                }
            }
        } else  {
            switch (type) {
                case QT_XFORM_TYPE_MSBFIRST:
                    while (dptr < maxp) {
                        IWX_MSB(128);
                        IWX_MSB(64);
                        IWX_MSB(32);
                        IWX_MSB(16);
                        IWX_MSB(8);
                        IWX_MSB(4);
                        IWX_MSB(2);
                        IWX_MSB(1);
                        dptr++;
                    }
                    break;
                case QT_XFORM_TYPE_LSBFIRST:
                    while (dptr < maxp) {
                        IWX_LSB(1);
                        IWX_LSB(2);
                        IWX_LSB(4);
                        IWX_LSB(8);
                        IWX_LSB(16);
                        IWX_LSB(32);
                        IWX_LSB(64);
                        IWX_LSB(128);
                        dptr++;
                    }
                    break;
#  if defined(Q_WS_WIN)
                case QT_XFORM_TYPE_WINDOWSPIXMAP:
                    while (dptr < maxp) {
                        IWX_PIX(128);
                        IWX_PIX(64);
                        IWX_PIX(32);
                        IWX_PIX(16);
                        IWX_PIX(8);
                        IWX_PIX(4);
                        IWX_PIX(2);
                        IWX_PIX(1);
                        dptr++;
                    }
                    break;
#  endif
            }
        }
        m21ydx += m21;
        m22ydy += m22;
        dptr += p_inc;
    }
    return true;
}
#undef IWX_MSB
#undef IWX_LSB
#undef IWX_PIX

/*!
    \fn QImage QImage::xForm(const QMatrix &matrix) const

    Use transformed() instead.

    \oldcode
        QImage image;
        ...
        image.xForm(matrix);
    \newcode
        QImage image;
        ...
        image.transformed(matrix);
    \endcode
*/

/*! \obsolete
    Returns a number that identifies the contents of this
    QImage object. Distinct QImage objects can only have the same
    serial number if they refer to the same contents (but they don't
    have to).

    Use cacheKey() instead.

    \warning The serial number doesn't necessarily change when the
    image is altered. This means that it may be dangerous to use
    it as a cache key.

    \sa operator==()
*/

int QImage::serialNumber() const
{
    if (!d)
        return 0;
    else
        return d->ser_no;
}

/*!
    Returns a number that identifies the contents of this QImage
    object. Distinct QImage objects can only have the same key if they
    refer to the same contents.

    The key will change when the image is altered.
*/
qint64 QImage::cacheKey() const
{
    if (!d)
        return 0;
    else
        return (((qint64) d->ser_no) << 32) | ((qint64) d->detach_no);
}

/*!
    \internal

    Returns true if the image is detached; otherwise returns false.

    \sa detach(), {Implicit Data Sharing}
*/

bool QImage::isDetached() const
{
    return d && d->ref == 1;
}


/*!
    \obsolete
    Sets the alpha channel of this image to the given \a alphaChannel.

    If \a alphaChannel is an 8 bit grayscale image, the intensity values are
    written into this buffer directly. Otherwise, \a alphaChannel is converted
    to 32 bit and the intensity of the RGB pixel values is used.

    Note that the image will be converted to the Format_ARGB32_Premultiplied
    format if the function succeeds.

    Use one of the composition modes in QPainter::CompositionMode instead.

    \warning This function is expensive.

    \sa alphaChannel(), {QImage#Image Transformations}{Image
    Transformations}, {QImage#Image Formats}{Image Formats}
*/

void QImage::setAlphaChannel(const QImage &alphaChannel)
{
    if (!d)
        return;

    int w = d->width;
    int h = d->height;

    if (w != alphaChannel.d->width || h != alphaChannel.d->height) {
        qWarning("QImage::setAlphaChannel: "
                 "Alpha channel must have same dimensions as the target image");
        return;
    }

    if (d->paintEngine && d->paintEngine->isActive()) {
        qWarning("QImage::setAlphaChannel: "
                 "Unable to set alpha channel while image is being painted on");
        return;
    }

    if (d->format == QImage::Format_ARGB32_Premultiplied)
        detach();
    else
        *this = convertToFormat(QImage::Format_ARGB32_Premultiplied);

    if (isNull())
        return;

    // Slight optimization since alphachannels are returned as 8-bit grays.
    if (alphaChannel.d->depth == 8 && alphaChannel.isGrayscale()) {
        const uchar *src_data = alphaChannel.d->data;
        const uchar *dest_data = d->data;
        for (int y=0; y<h; ++y) {
            const uchar *src = src_data;
            QRgb *dest = (QRgb *)dest_data;
            for (int x=0; x<w; ++x) {
                int alpha = *src;
                int destAlpha = qt_div_255(alpha * qAlpha(*dest));
                *dest = ((destAlpha << 24)
                         | (qt_div_255(qRed(*dest) * alpha) << 16)
                         | (qt_div_255(qGreen(*dest) * alpha) << 8)
                         | (qt_div_255(qBlue(*dest) * alpha)));
                ++dest;
                ++src;
            }
            src_data += alphaChannel.d->bytes_per_line;
            dest_data += d->bytes_per_line;
        }

    } else {
        const QImage sourceImage = alphaChannel.convertToFormat(QImage::Format_RGB32);
        if (sourceImage.isNull()) {
            qWarning("QImage::setAlphaChannel: out of memory");
            return;
        }
        const uchar *src_data = sourceImage.d->data;
        const uchar *dest_data = d->data;
        for (int y=0; y<h; ++y) {
            const QRgb *src = (const QRgb *) src_data;
            QRgb *dest = (QRgb *) dest_data;
            for (int x=0; x<w; ++x) {
                int alpha = qGray(*src);
                int destAlpha = qt_div_255(alpha * qAlpha(*dest));
                *dest = ((destAlpha << 24)
                         | (qt_div_255(qRed(*dest) * alpha) << 16)
                         | (qt_div_255(qGreen(*dest) * alpha) << 8)
                         | (qt_div_255(qBlue(*dest) * alpha)));
                ++dest;
                ++src;
            }
            src_data += sourceImage.d->bytes_per_line;
            dest_data += d->bytes_per_line;
        }
    }
}


/*!
    \obsolete

    Returns the alpha channel of the image as a new grayscale QImage in which
    each pixel's red, green, and blue values are given the alpha value of the
    original image. The color depth of the returned image is 8-bit.

    You can see an example of use of this function in QPixmap's
    \l{QPixmap::}{alphaChannel()}, which works in the same way as
    this function on QPixmaps.

    Most usecases for this function can be replaced with QPainter and
    using composition modes.

    \warning This is an expensive function.

    \sa setAlphaChannel(), hasAlphaChannel(),
    {QPixmap#Pixmap Information}{Pixmap},
    {QImage#Image Transformations}{Image Transformations}
*/

QImage QImage::alphaChannel() const
{
    if (!d)
        return QImage();

    int w = d->width;
    int h = d->height;

    QImage image(w, h, Format_Indexed8);
    QIMAGE_SANITYCHECK_MEMORY(image);
    image.setColorCount(256);

    // set up gray scale table.
    for (int i=0; i<256; ++i)
        image.setColor(i, qRgb(i, i, i));

    if (!hasAlphaChannel()) {
        image.fill(255);
        return image;
    }

    if (d->format == Format_Indexed8) {
        const uchar *src_data = d->data;
        uchar *dest_data = image.d->data;
        for (int y=0; y<h; ++y) {
            const uchar *src = src_data;
            uchar *dest = dest_data;
            for (int x=0; x<w; ++x) {
                *dest = qAlpha(d->colortable.at(*src));
                ++dest;
                ++src;
            }
            src_data += d->bytes_per_line;
            dest_data += image.d->bytes_per_line;
        }
    } else {
        QImage alpha32 = *this;
        if (d->format != Format_ARGB32 && d->format != Format_ARGB32_Premultiplied)
            alpha32 = convertToFormat(Format_ARGB32);
        QIMAGE_SANITYCHECK_MEMORY(alpha32);

        const uchar *src_data = alpha32.d->data;
        uchar *dest_data = image.d->data;
        for (int y=0; y<h; ++y) {
            const QRgb *src = (const QRgb *) src_data;
            uchar *dest = dest_data;
            for (int x=0; x<w; ++x) {
                *dest = qAlpha(*src);
                ++dest;
                ++src;
            }
            src_data += alpha32.d->bytes_per_line;
            dest_data += image.d->bytes_per_line;
        }
    }

    return image;
}

/*!
    Returns true if the image has a format that respects the alpha
    channel, otherwise returns false.

    \sa {QImage#Image Information}{Image Information}
*/
bool QImage::hasAlphaChannel() const
{
    return d && (d->format == Format_ARGB32_Premultiplied
                 || d->format == Format_ARGB32
                 || d->format == Format_ARGB8565_Premultiplied
                 || d->format == Format_ARGB8555_Premultiplied
                 || d->format == Format_ARGB6666_Premultiplied
                 || d->format == Format_ARGB4444_Premultiplied
                 || (d->has_alpha_clut && (d->format == Format_Indexed8
                                           || d->format == Format_Mono
                                           || d->format == Format_MonoLSB)));
}


/*!
    \since 4.7
    Returns the number of bit planes in the image.

    The number of bit planes is the number of bits of color and
    transparency information for each pixel. This is different from
    (i.e. smaller than) the depth when the image format contains
    unused bits.

    \sa depth(), format(), {QImage#Image Formats}{Image Formats}
*/
int QImage::bitPlaneCount() const
{
    if (!d)
        return 0;
    int bpc = 0;
    switch (d->format) {
    case QImage::Format_Invalid:
        break;
    case QImage::Format_RGB32:
        bpc = 24;
        break;
    case QImage::Format_RGB666:
        bpc = 18;
        break;
    case QImage::Format_RGB555:
        bpc = 15;
        break;
    case QImage::Format_ARGB8555_Premultiplied:
        bpc = 23;
        break;
    case QImage::Format_RGB444:
        bpc = 12;
        break;
    default:
        bpc = qt_depthForFormat(d->format);
        break;
    }
    return bpc;
}


#ifdef QT3_SUPPORT
#if defined(Q_WS_X11)
QT_BEGIN_INCLUDE_NAMESPACE
#include <private/qt_x11_p.h>
QT_END_INCLUDE_NAMESPACE
#endif

QImage::Endian QImage::systemBitOrder()
{
#if defined(Q_WS_X11)
    return BitmapBitOrder(X11->display) == MSBFirst ? BigEndian : LittleEndian;
#else
    return BigEndian;
#endif
}
#endif

/*!
    \fn QImage QImage::copy(const QRect &rect, Qt::ImageConversionFlags flags) const
    \compat

    Use copy() instead.
*/

/*!
    \fn QImage QImage::copy(int x, int y, int w, int h, Qt::ImageConversionFlags flags) const
    \compat

    Use copy() instead.
*/

/*!
    \fn QImage QImage::scaleWidth(int w) const
    \compat

    Use scaledToWidth() instead.
*/

/*!
    \fn QImage QImage::scaleHeight(int h) const
    \compat

    Use scaledToHeight() instead.
*/

static QImage smoothScaled(const QImage &source, int w, int h) {
    QImage src = source;
    if (src.format() == QImage::Format_ARGB32)
        src = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    else if (src.depth() < 32) {
        if (src.hasAlphaChannel())
            src = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        else
            src = src.convertToFormat(QImage::Format_RGB32);
    }

    return qSmoothScaleImage(src, w, h);
}


static QImage rotated90(const QImage &image) {
    QImage out(image.height(), image.width(), image.format());
    QIMAGE_SANITYCHECK_MEMORY(out);
    if (image.colorCount() > 0)
        out.setColorTable(image.colorTable());
    int w = image.width();
    int h = image.height();
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        qt_memrotate270(reinterpret_cast<const quint32*>(image.bits()),
                        w, h, image.bytesPerLine(),
                        reinterpret_cast<quint32*>(out.bits()),
                        out.bytesPerLine());
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        qt_memrotate270(reinterpret_cast<const quint24*>(image.bits()),
                        w, h, image.bytesPerLine(),
                        reinterpret_cast<quint24*>(out.bits()),
                        out.bytesPerLine());
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_ARGB4444_Premultiplied:
        qt_memrotate270(reinterpret_cast<const quint16*>(image.bits()),
                        w, h, image.bytesPerLine(),
                        reinterpret_cast<quint16*>(out.bits()),
                        out.bytesPerLine());
        break;
    case QImage::Format_Indexed8:
        qt_memrotate270(reinterpret_cast<const quint8*>(image.bits()),
                        w, h, image.bytesPerLine(),
                        reinterpret_cast<quint8*>(out.bits()),
                        out.bytesPerLine());
        break;
    default:
        for (int y=0; y<h; ++y) {
            if (image.colorCount())
                for (int x=0; x<w; ++x)
                    out.setPixel(h-y-1, x, image.pixelIndex(x, y));
            else
                for (int x=0; x<w; ++x)
                    out.setPixel(h-y-1, x, image.pixel(x, y));
        }
        break;
    }
    return out;
}


static QImage rotated180(const QImage &image) {
    return image.mirrored(true, true);
}


static QImage rotated270(const QImage &image) {
    QImage out(image.height(), image.width(), image.format());
    QIMAGE_SANITYCHECK_MEMORY(out);
    if (image.colorCount() > 0)
        out.setColorTable(image.colorTable());
    int w = image.width();
    int h = image.height();
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        qt_memrotate90(reinterpret_cast<const quint32*>(image.bits()),
                       w, h, image.bytesPerLine(),
                       reinterpret_cast<quint32*>(out.bits()),
                       out.bytesPerLine());
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        qt_memrotate90(reinterpret_cast<const quint24*>(image.bits()),
                       w, h, image.bytesPerLine(),
                       reinterpret_cast<quint24*>(out.bits()),
                       out.bytesPerLine());
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_ARGB4444_Premultiplied:
       qt_memrotate90(reinterpret_cast<const quint16*>(image.bits()),
                       w, h, image.bytesPerLine(),
                       reinterpret_cast<quint16*>(out.bits()),
                       out.bytesPerLine());
        break;
    case QImage::Format_Indexed8:
        qt_memrotate90(reinterpret_cast<const quint8*>(image.bits()),
                       w, h, image.bytesPerLine(),
                       reinterpret_cast<quint8*>(out.bits()),
                       out.bytesPerLine());
        break;
    default:
        for (int y=0; y<h; ++y) {
            if (image.colorCount())
                for (int x=0; x<w; ++x)
                    out.setPixel(y, w-x-1, image.pixelIndex(x, y));
            else
                for (int x=0; x<w; ++x)
                    out.setPixel(y, w-x-1, image.pixel(x, y));
        }
        break;
    }
    return out;
}

/*!
    Returns a copy of the image that is transformed using the given
    transformation \a matrix and transformation \a mode.

    The transformation \a matrix is internally adjusted to compensate
    for unwanted translation; i.e. the image produced is the smallest
    image that contains all the transformed points of the original
    image. Use the trueMatrix() function to retrieve the actual matrix
    used for transforming an image.

    Unlike the other overload, this function can be used to perform perspective
    transformations on images.

    \sa trueMatrix(), {QImage#Image Transformations}{Image
    Transformations}
*/

QImage QImage::transformed(const QTransform &matrix, Qt::TransformationMode mode ) const
{
    if (!d)
        return QImage();

    // source image data
    int ws = width();
    int hs = height();

    // target image data
    int wd;
    int hd;

    // compute size of target image
    QTransform mat = trueMatrix(matrix, ws, hs);
    bool complex_xform = false;
    bool scale_xform = false;
    if (mat.type() <= QTransform::TxScale) {
        if (mat.type() == QTransform::TxNone) // identity matrix
            return *this;
        else if (mat.m11() == -1. && mat.m22() == -1.)
            return rotated180(*this);

        if (mode == Qt::FastTransformation) {
            hd = qRound(qAbs(mat.m22()) * hs);
            wd = qRound(qAbs(mat.m11()) * ws);
        } else {
            hd = int(qAbs(mat.m22()) * hs + 0.9999);
            wd = int(qAbs(mat.m11()) * ws + 0.9999);
        }
        scale_xform = true;
    } else {
        if (mat.type() <= QTransform::TxRotate && mat.m11() == 0 && mat.m22() == 0) {
            if (mat.m12() == 1. && mat.m21() == -1.)
                return rotated90(*this);
            else if (mat.m12() == -1. && mat.m21() == 1.)
                return rotated270(*this);
        }

        QPolygonF a(QRectF(0, 0, ws, hs));
        a = mat.map(a);
        QRect r = a.boundingRect().toAlignedRect();
        wd = r.width();
        hd = r.height();
        complex_xform = true;
    }

    if (wd == 0 || hd == 0)
        return QImage();

    // Make use of the optimized algorithm when we're scaling
    if (scale_xform && mode == Qt::SmoothTransformation) {
        if (mat.m11() < 0.0F && mat.m22() < 0.0F) { // horizontal/vertical flip
            return smoothScaled(mirrored(true, true), wd, hd);
        } else if (mat.m11() < 0.0F) { // horizontal flip
            return smoothScaled(mirrored(true, false), wd, hd);
        } else if (mat.m22() < 0.0F) { // vertical flip
            return smoothScaled(mirrored(false, true), wd, hd);
        } else { // no flipping
            return smoothScaled(*this, wd, hd);
        }
    }

    int bpp = depth();

    int sbpl = bytesPerLine();
    const uchar *sptr = bits();

    QImage::Format target_format = d->format;

    if (complex_xform || mode == Qt::SmoothTransformation) {
        if (d->format < QImage::Format_RGB32 || !hasAlphaChannel()) {
            switch(d->format) {
            case QImage::Format_RGB16:
                target_format = Format_ARGB8565_Premultiplied;
                break;
            case QImage::Format_RGB555:
                target_format = Format_ARGB8555_Premultiplied;
                break;
            case QImage::Format_RGB666:
                target_format = Format_ARGB6666_Premultiplied;
                break;
            case QImage::Format_RGB444:
                target_format = Format_ARGB4444_Premultiplied;
                break;
            default:
                target_format = Format_ARGB32_Premultiplied;
                break;
            }
        }
    }

    QImage dImage(wd, hd, target_format);
    QIMAGE_SANITYCHECK_MEMORY(dImage);

    if (target_format == QImage::Format_MonoLSB
        || target_format == QImage::Format_Mono
        || target_format == QImage::Format_Indexed8) {
        dImage.d->colortable = d->colortable;
        dImage.d->has_alpha_clut = d->has_alpha_clut | complex_xform;
    }

    dImage.d->dpmx = dotsPerMeterX();
    dImage.d->dpmy = dotsPerMeterY();

    switch (bpp) {
        // initizialize the data
        case 8:
            if (dImage.d->colortable.size() < 256) {
                // colors are left in the color table, so pick that one as transparent
                dImage.d->colortable.append(0x0);
                memset(dImage.bits(), dImage.d->colortable.size() - 1, dImage.byteCount());
            } else {
                memset(dImage.bits(), 0, dImage.byteCount());
            }
            break;
        case 1:
        case 16:
        case 24:
        case 32:
            memset(dImage.bits(), 0x00, dImage.byteCount());
            break;
    }

    if (target_format >= QImage::Format_RGB32) {
        QPainter p(&dImage);
        if (mode == Qt::SmoothTransformation) {
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
        }
        p.setTransform(mat);
        p.drawImage(QPoint(0, 0), *this);
    } else {
        bool invertible;
        mat = mat.inverted(&invertible);                // invert matrix
        if (!invertible)        // error, return null image
            return QImage();

        // create target image (some of the code is from QImage::copy())
        int type = format() == Format_Mono ? QT_XFORM_TYPE_MSBFIRST : QT_XFORM_TYPE_LSBFIRST;
        int dbpl = dImage.bytesPerLine();
        qt_xForm_helper(mat, 0, type, bpp, dImage.bits(), dbpl, 0, hd, sptr, sbpl, ws, hs);
    }
    return dImage;
}

/*!
    \fn QTransform QImage::trueMatrix(const QTransform &matrix, int width, int height)

    Returns the actual matrix used for transforming an image with the
    given \a width, \a height and \a matrix.

    When transforming an image using the transformed() function, the
    transformation matrix is internally adjusted to compensate for
    unwanted translation, i.e. transformed() returns the smallest
    image containing all transformed points of the original image.
    This function returns the modified matrix, which maps points
    correctly from the original image into the new image.

    Unlike the other overload, this function creates transformation
    matrices that can be used to perform perspective
    transformations on images.

    \sa transformed(), {QImage#Image Transformations}{Image
    Transformations}
*/

QTransform QImage::trueMatrix(const QTransform &matrix, int w, int h)
{
    const QRectF rect(0, 0, w, h);
    const QRect mapped = matrix.mapRect(rect).toAlignedRect();
    const QPoint delta = mapped.topLeft();
    return matrix * QTransform().translate(-delta.x(), -delta.y());
}

bool QImageData::convertInPlace(QImage::Format newFormat, Qt::ImageConversionFlags flags)
{
    if (format == newFormat)
        return true;

    // No in-place conversion if we have to detach
    if (ref > 1)
        return false;

    const InPlace_Image_Converter *const converterPtr = &inplace_converter_map[format][newFormat];
    InPlace_Image_Converter converter = *converterPtr;
    if (converter)
        return converter(this, flags);
    else
        return false;
}

/*!
    \typedef QImage::DataPtr
    \internal
*/

/*!
    \fn DataPtr & QImage::data_ptr()
    \internal
*/

QT_END_NAMESPACE
