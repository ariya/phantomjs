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

#include "qjpeghandler_p.h"

#include <qimage.h>
#include <qvariant.h>
#include <qvector.h>
#include <qbuffer.h>
#include <private/qsimd_p.h>

#include <stdio.h>      // jpeglib needs this to be pre-included
#include <setjmp.h>

#ifdef FAR
#undef FAR
#endif

// including jpeglib.h seems to be a little messy
extern "C" {
// mingw includes rpcndr.h but does not define boolean
#if defined(Q_OS_WIN) && defined(Q_CC_GNU)
#   if defined(__RPCNDR_H__) && !defined(boolean)
        typedef unsigned char boolean;
#       define HAVE_BOOLEAN
#   endif
#endif

#define XMD_H           // shut JPEGlib up
#if defined(Q_OS_UNIXWARE)
#  define HAVE_BOOLEAN  // libjpeg under Unixware seems to need this
#endif
#include <jpeglib.h>
#ifdef const
#  undef const          // remove crazy C hackery in jconfig.h
#endif
}

#if defined(JPEG_TRUE) && !defined(HAVE_BOOLEAN)
// this jpeglib.h uses JPEG_boolean
typedef JPEG_boolean boolean;
#endif

QT_BEGIN_NAMESPACE

void QT_FASTCALL convert_rgb888_to_rgb32_C(quint32 *dst, const uchar *src, int len)
{
    // Expand 24->32 bpp.
    for (int i = 0; i < len; ++i) {
        *dst++ = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

typedef void (QT_FASTCALL *Rgb888ToRgb32Converter)(quint32 *dst, const uchar *src, int len);

static Rgb888ToRgb32Converter rgb888ToRgb32ConverterPtr = convert_rgb888_to_rgb32_C;

struct my_error_mgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void my_error_exit (j_common_ptr cinfo)
{
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    qWarning("%s", buffer);
    longjmp(myerr->setjmp_buffer, 1);
}

#if defined(Q_C_CALLBACKS)
}
#endif


static const int max_buf = 4096;

struct my_jpeg_source_mgr : public jpeg_source_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    QIODevice *device;
    JOCTET buffer[max_buf];
    const QBuffer *memDevice;

public:
    my_jpeg_source_mgr(QIODevice *device);
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void qt_init_source(j_decompress_ptr)
{
}

static boolean qt_fill_input_buffer(j_decompress_ptr cinfo)
{
    my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;
    qint64 num_read = 0;
    if (src->memDevice) {
        src->next_input_byte = (const JOCTET *)(src->memDevice->data().constData() + src->memDevice->pos());
        num_read = src->memDevice->data().size() - src->memDevice->pos();
        src->device->seek(src->memDevice->data().size());
    } else {
        src->next_input_byte = src->buffer;
        num_read = src->device->read((char*)src->buffer, max_buf);
    }
    if (num_read <= 0) {
        // Insert a fake EOI marker - as per jpeglib recommendation
        src->next_input_byte = src->buffer;
        src->buffer[0] = (JOCTET) 0xFF;
        src->buffer[1] = (JOCTET) JPEG_EOI;
        src->bytes_in_buffer = 2;
    } else {
        src->bytes_in_buffer = num_read;
    }
#if defined(Q_OS_UNIXWARE)
    return B_TRUE;
#else
    return true;
#endif
}

static void qt_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;

    // `dumb' implementation from jpeglib

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0) {
        while (num_bytes > (long) src->bytes_in_buffer) {  // Should not happen in case of memDevice
            num_bytes -= (long) src->bytes_in_buffer;
            (void) qt_fill_input_buffer(cinfo);
            /* note we assume that qt_fill_input_buffer will never return false,
            * so suspension need not be handled.
            */
        }
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

static void qt_term_source(j_decompress_ptr cinfo)
{
    my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;
    if (!src->device->isSequential())
        src->device->seek(src->device->pos() - src->bytes_in_buffer);
}

#if defined(Q_C_CALLBACKS)
}
#endif

inline my_jpeg_source_mgr::my_jpeg_source_mgr(QIODevice *device)
{
    jpeg_source_mgr::init_source = qt_init_source;
    jpeg_source_mgr::fill_input_buffer = qt_fill_input_buffer;
    jpeg_source_mgr::skip_input_data = qt_skip_input_data;
    jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
    jpeg_source_mgr::term_source = qt_term_source;
    this->device = device;
    memDevice = qobject_cast<QBuffer *>(device);
    bytes_in_buffer = 0;
    next_input_byte = buffer;
}


inline static bool read_jpeg_size(int &w, int &h, j_decompress_ptr cinfo)
{
    (void) jpeg_calc_output_dimensions(cinfo);

    w = cinfo->output_width;
    h = cinfo->output_height;
    return true;
}

#define HIGH_QUALITY_THRESHOLD 50

inline static bool read_jpeg_format(QImage::Format &format, j_decompress_ptr cinfo)
{

    bool result = true;
    switch (cinfo->output_components) {
    case 1:
        format = QImage::Format_Indexed8;
        break;
    case 3:
    case 4:
        format = QImage::Format_RGB32;
        break;
    default:
        result = false;
        break;
    }
    cinfo->output_scanline = cinfo->output_height;
    return result;
}

static bool ensureValidImage(QImage *dest, struct jpeg_decompress_struct *info,
                             const QSize& size)
{
    QImage::Format format;
    switch (info->output_components) {
    case 1:
        format = QImage::Format_Indexed8;
        break;
    case 3:
    case 4:
        format = QImage::Format_RGB32;
        break;
    default:
        return false; // unsupported format
    }

    if (dest->size() != size || dest->format() != format) {
        *dest = QImage(size, format);

        if (format == QImage::Format_Indexed8) {
            dest->setColorCount(256);
            for (int i = 0; i < 256; i++)
                dest->setColor(i, qRgb(i,i,i));
        }
    }

    return !dest->isNull();
}

static bool read_jpeg_image(QImage *outImage,
                            QSize scaledSize, QRect scaledClipRect,
                            QRect clipRect, int inQuality, j_decompress_ptr info, struct my_error_mgr* err  )
{
    if (!setjmp(err->setjmp_buffer)) {
        // -1 means default quality.
        int quality = inQuality;
        if (quality < 0)
            quality = 75;

        // If possible, merge the scaledClipRect into either scaledSize
        // or clipRect to avoid doing a separate scaled clipping pass.
        // Best results are achieved by clipping before scaling, not after.
        if (!scaledClipRect.isEmpty()) {
            if (scaledSize.isEmpty() && clipRect.isEmpty()) {
                // No clipping or scaling before final clip.
                clipRect = scaledClipRect;
                scaledClipRect = QRect();
            } else if (scaledSize.isEmpty()) {
                // Clipping, but no scaling: combine the clip regions.
                scaledClipRect.translate(clipRect.topLeft());
                clipRect = scaledClipRect.intersected(clipRect);
                scaledClipRect = QRect();
            } else if (clipRect.isEmpty()) {
                // No clipping, but scaling: if we can map back to an
                // integer pixel boundary, then clip before scaling.
                if ((info->image_width % scaledSize.width()) == 0 &&
                        (info->image_height % scaledSize.height()) == 0) {
                    int x = scaledClipRect.x() * info->image_width /
                            scaledSize.width();
                    int y = scaledClipRect.y() * info->image_height /
                            scaledSize.height();
                    int width = (scaledClipRect.right() + 1) *
                                info->image_width / scaledSize.width() - x;
                    int height = (scaledClipRect.bottom() + 1) *
                                 info->image_height / scaledSize.height() - y;
                    clipRect = QRect(x, y, width, height);
                    scaledSize = scaledClipRect.size();
                    scaledClipRect = QRect();
                }
            } else {
                // Clipping and scaling: too difficult to figure out,
                // and not a likely use case, so do it the long way.
            }
        }

        // Determine the scale factor to pass to libjpeg for quick downscaling.
        if (!scaledSize.isEmpty()) {
            if (clipRect.isEmpty()) {
                info->scale_denom =
                    qMin(info->image_width / scaledSize.width(),
                         info->image_height / scaledSize.height());
            } else {
                info->scale_denom =
                    qMin(clipRect.width() / scaledSize.width(),
                         clipRect.height() / scaledSize.height());
            }
            if (info->scale_denom < 2) {
                info->scale_denom = 1;
            } else if (info->scale_denom < 4) {
                info->scale_denom = 2;
            } else if (info->scale_denom < 8) {
                info->scale_denom = 4;
            } else {
                info->scale_denom = 8;
            }
            info->scale_num = 1;
            if (!clipRect.isEmpty()) {
                // Correct the scale factor so that we clip accurately.
                // It is recommended that the clip rectangle be aligned
                // on an 8-pixel boundary for best performance.
                while (info->scale_denom > 1 &&
                       ((clipRect.x() % info->scale_denom) != 0 ||
                        (clipRect.y() % info->scale_denom) != 0 ||
                        (clipRect.width() % info->scale_denom) != 0 ||
                        (clipRect.height() % info->scale_denom) != 0)) {
                    info->scale_denom /= 2;
                }
            }
        }

        // If high quality not required, use fast decompression
        if( quality < HIGH_QUALITY_THRESHOLD ) {
            info->dct_method = JDCT_IFAST;
            info->do_fancy_upsampling = FALSE;
        }

        (void) jpeg_calc_output_dimensions(info);

        // Determine the clip region to extract.
        QRect imageRect(0, 0, info->output_width, info->output_height);
        QRect clip;
        if (clipRect.isEmpty()) {
            clip = imageRect;
        } else if (info->scale_denom == info->scale_num) {
            clip = clipRect.intersected(imageRect);
        } else {
            // The scale factor was corrected above to ensure that
            // we don't miss pixels when we scale the clip rectangle.
            clip = QRect(clipRect.x() / int(info->scale_denom),
                         clipRect.y() / int(info->scale_denom),
                         clipRect.width() / int(info->scale_denom),
                         clipRect.height() / int(info->scale_denom));
            clip = clip.intersected(imageRect);
        }

        // Allocate memory for the clipped QImage.
        if (!ensureValidImage(outImage, info, clip.size()))
            longjmp(err->setjmp_buffer, 1);

        // Avoid memcpy() overhead if grayscale with no clipping.
        bool quickGray = (info->output_components == 1 &&
                          clip == imageRect);
        if (!quickGray) {
            // Ask the jpeg library to allocate a temporary row.
            // The library will automatically delete it for us later.
            // The libjpeg docs say we should do this before calling
            // jpeg_start_decompress().  We can't use "new" here
            // because we are inside the setjmp() block and an error
            // in the jpeg input stream would cause a memory leak.
            JSAMPARRAY rows = (info->mem->alloc_sarray)
                              ((j_common_ptr)info, JPOOL_IMAGE,
                               info->output_width * info->output_components, 1);

            (void) jpeg_start_decompress(info);

            while (info->output_scanline < info->output_height) {
                int y = int(info->output_scanline) - clip.y();
                if (y >= clip.height())
                    break;      // We've read the entire clip region, so abort.

                (void) jpeg_read_scanlines(info, rows, 1);

                if (y < 0)
                    continue;   // Haven't reached the starting line yet.

                if (info->output_components == 3) {
                    uchar *in = rows[0] + clip.x() * 3;
                    QRgb *out = (QRgb*)outImage->scanLine(y);
                    rgb888ToRgb32ConverterPtr(out, in, clip.width());
                } else if (info->out_color_space == JCS_CMYK) {
                    // Convert CMYK->RGB.
                    uchar *in = rows[0] + clip.x() * 4;
                    QRgb *out = (QRgb*)outImage->scanLine(y);
                    for (int i = 0; i < clip.width(); ++i) {
                        int k = in[3];
                        *out++ = qRgb(k * in[0] / 255, k * in[1] / 255,
                                      k * in[2] / 255);
                        in += 4;
                    }
                } else if (info->output_components == 1) {
                    // Grayscale.
                    memcpy(outImage->scanLine(y),
                           rows[0] + clip.x(), clip.width());
                }
            }
        } else {
            // Load unclipped grayscale data directly into the QImage.
            (void) jpeg_start_decompress(info);
            while (info->output_scanline < info->output_height) {
                uchar *row = outImage->scanLine(info->output_scanline);
                (void) jpeg_read_scanlines(info, &row, 1);
            }
        }

        if (info->output_scanline == info->output_height)
            (void) jpeg_finish_decompress(info);

        if (info->density_unit == 1) {
            outImage->setDotsPerMeterX(int(100. * info->X_density / 2.54));
            outImage->setDotsPerMeterY(int(100. * info->Y_density / 2.54));
        } else if (info->density_unit == 2) {
            outImage->setDotsPerMeterX(int(100. * info->X_density));
            outImage->setDotsPerMeterY(int(100. * info->Y_density));
        }

        if (scaledSize.isValid() && scaledSize != clip.size()) {
            *outImage = outImage->scaled(scaledSize, Qt::IgnoreAspectRatio, quality >= HIGH_QUALITY_THRESHOLD ? Qt::SmoothTransformation : Qt::FastTransformation);
        }

        if (!scaledClipRect.isEmpty())
            *outImage = outImage->copy(scaledClipRect);
        return !outImage->isNull();
    }
    else
        return false;
}

struct my_jpeg_destination_mgr : public jpeg_destination_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    QIODevice *device;
    JOCTET buffer[max_buf];

public:
    my_jpeg_destination_mgr(QIODevice *);
};


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void qt_init_destination(j_compress_ptr)
{
}

static boolean qt_empty_output_buffer(j_compress_ptr cinfo)
{
    my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;

    int written = dest->device->write((char*)dest->buffer, max_buf);
    if (written == -1)
        (*cinfo->err->error_exit)((j_common_ptr)cinfo);

    dest->next_output_byte = dest->buffer;
    dest->free_in_buffer = max_buf;

#if defined(Q_OS_UNIXWARE)
    return B_TRUE;
#else
    return true;
#endif
}

static void qt_term_destination(j_compress_ptr cinfo)
{
    my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
    qint64 n = max_buf - dest->free_in_buffer;

    qint64 written = dest->device->write((char*)dest->buffer, n);
    if (written == -1)
        (*cinfo->err->error_exit)((j_common_ptr)cinfo);
}

#if defined(Q_C_CALLBACKS)
}
#endif

inline my_jpeg_destination_mgr::my_jpeg_destination_mgr(QIODevice *device)
{
    jpeg_destination_mgr::init_destination = qt_init_destination;
    jpeg_destination_mgr::empty_output_buffer = qt_empty_output_buffer;
    jpeg_destination_mgr::term_destination = qt_term_destination;
    this->device = device;
    next_output_byte = buffer;
    free_in_buffer = max_buf;
}


static bool write_jpeg_image(const QImage &image, QIODevice *device, int sourceQuality)
{
    bool success = false;
    const QVector<QRgb> cmap = image.colorTable();

    struct jpeg_compress_struct cinfo;
    JSAMPROW row_pointer[1];
    row_pointer[0] = 0;

    struct my_jpeg_destination_mgr *iod_dest = new my_jpeg_destination_mgr(device);
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_error_exit;

    if (!setjmp(jerr.setjmp_buffer)) {
        // WARNING:
        // this if loop is inside a setjmp/longjmp branch
        // do not create C++ temporaries here because the destructor may never be called
        // if you allocate memory, make sure that you can free it (row_pointer[0])
        jpeg_create_compress(&cinfo);

        cinfo.dest = iod_dest;

        cinfo.image_width = image.width();
        cinfo.image_height = image.height();

        bool gray=false;
        switch (image.format()) {
        case QImage::Format_Mono:
        case QImage::Format_MonoLSB:
        case QImage::Format_Indexed8:
            gray = true;
            for (int i = image.colorCount(); gray && i--;) {
                gray = gray & (qRed(cmap[i]) == qGreen(cmap[i]) &&
                               qRed(cmap[i]) == qBlue(cmap[i]));
            }
            cinfo.input_components = gray ? 1 : 3;
            cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
            break;
        default:
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
        }

        jpeg_set_defaults(&cinfo);

        qreal diffInch = qAbs(image.dotsPerMeterX()*2.54/100. - qRound(image.dotsPerMeterX()*2.54/100.))
                         + qAbs(image.dotsPerMeterY()*2.54/100. - qRound(image.dotsPerMeterY()*2.54/100.));
        qreal diffCm = (qAbs(image.dotsPerMeterX()/100. - qRound(image.dotsPerMeterX()/100.))
                        + qAbs(image.dotsPerMeterY()/100. - qRound(image.dotsPerMeterY()/100.)))*2.54;
        if (diffInch < diffCm) {
            cinfo.density_unit = 1; // dots/inch
            cinfo.X_density = qRound(image.dotsPerMeterX()*2.54/100.);
            cinfo.Y_density = qRound(image.dotsPerMeterY()*2.54/100.);
        } else {
            cinfo.density_unit = 2; // dots/cm
            cinfo.X_density = (image.dotsPerMeterX()+50) / 100;
            cinfo.Y_density = (image.dotsPerMeterY()+50) / 100;
        }


        int quality = sourceQuality >= 0 ? qMin(sourceQuality,100) : 75;
#if defined(Q_OS_UNIXWARE)
        jpeg_set_quality(&cinfo, quality, B_TRUE /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, B_TRUE);
#else
        jpeg_set_quality(&cinfo, quality, true /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, true);
#endif

        row_pointer[0] = new uchar[cinfo.image_width*cinfo.input_components];
        int w = cinfo.image_width;
        while (cinfo.next_scanline < cinfo.image_height) {
            uchar *row = row_pointer[0];
            switch (image.format()) {
            case QImage::Format_Mono:
            case QImage::Format_MonoLSB:
                if (gray) {
                    const uchar* data = image.constScanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            row[i] = qRed(cmap[bit]);
                        }
                    } else {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                            row[i] = qRed(cmap[bit]);
                        }
                    }
                } else {
                    const uchar* data = image.constScanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    } else {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    }
                }
                break;
            case QImage::Format_Indexed8:
                if (gray) {
                    const uchar* pix = image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row = qRed(cmap[*pix]);
                        ++row; ++pix;
                    }
                } else {
                    const uchar* pix = image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(cmap[*pix]);
                        *row++ = qGreen(cmap[*pix]);
                        *row++ = qBlue(cmap[*pix]);
                        ++pix;
                    }
                }
                break;
            case QImage::Format_RGB888:
                memcpy(row, image.constScanLine(cinfo.next_scanline), w * 3);
                break;
            case QImage::Format_RGB32:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB32_Premultiplied:
                {
                    const QRgb* rgb = (const QRgb*)image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(*rgb);
                        *row++ = qGreen(*rgb);
                        *row++ = qBlue(*rgb);
                        ++rgb;
                    }
                }
                break;
            default:
                {
                    // (Testing shows that this way is actually faster than converting to RGB888 + memcpy)
                    QImage rowImg = image.copy(0, cinfo.next_scanline, w, 1).convertToFormat(QImage::Format_RGB32);
                    const QRgb* rgb = (const QRgb*)rowImg.constScanLine(0);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(*rgb);
                        *row++ = qGreen(*rgb);
                        *row++ = qBlue(*rgb);
                        ++rgb;
                    }
                }
                break;
            }
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        success = true;
    } else {
        jpeg_destroy_compress(&cinfo);
        success = false;
    }

    delete iod_dest;
    delete [] row_pointer[0];
    return success;
}

class QJpegHandlerPrivate
{
public:
    enum State {
        Ready,
        ReadHeader,
        Error
    };

    QJpegHandlerPrivate(QJpegHandler *qq)
        : quality(75), iod_src(0), state(Ready), q(qq)
    {}

    ~QJpegHandlerPrivate()
    {
        if(iod_src)
        {
            jpeg_destroy_decompress(&info);
            delete iod_src;
            iod_src = 0;
        }
    }

    bool readJpegHeader(QIODevice*);
    bool read(QImage *image);

    int quality;
    QVariant size;
    QImage::Format format;
    QSize scaledSize;
    QRect scaledClipRect;
    QRect clipRect;
    struct jpeg_decompress_struct info;
    struct my_jpeg_source_mgr * iod_src;
    struct my_error_mgr err;

    State state;

    QJpegHandler *q;
};

/*!
    \internal
*/
bool QJpegHandlerPrivate::readJpegHeader(QIODevice *device)
{
    if(state == Ready)
    {
        state = Error;
        iod_src = new my_jpeg_source_mgr(device);

        jpeg_create_decompress(&info);
        info.src = iod_src;
        info.err = jpeg_std_error(&err);
        err.error_exit = my_error_exit;

        if (!setjmp(err.setjmp_buffer)) {
    #if defined(Q_OS_UNIXWARE)
            (void) jpeg_read_header(&info, B_TRUE);
    #else
            (void) jpeg_read_header(&info, true);
    #endif

            int width = 0;
            int height = 0;
            read_jpeg_size(width, height, &info);
            size = QSize(width, height);

            format = QImage::Format_Invalid;
            read_jpeg_format(format, &info);
            state = ReadHeader;
            return true;
        }
        else
        {
            return false;
        }
    }
    else if(state == Error)
        return false;
    return true;
}

bool QJpegHandlerPrivate::read(QImage *image)
{
    if(state == Ready)
        readJpegHeader(q->device());

    if(state == ReadHeader)
    {
        bool success = read_jpeg_image(image, scaledSize, scaledClipRect, clipRect, quality,  &info, &err);
        state = success ? Ready : Error;
        return success;
    }

    return false;

}

QJpegHandler::QJpegHandler()
    : d(new QJpegHandlerPrivate(this))
{
    const uint features = qDetectCPUFeatures();
    Q_UNUSED(features);
#if defined(QT_HAVE_NEON)
    // from qimage_neon.cpp
    Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_neon(quint32 *dst, const uchar *src, int len);

    if (features & NEON)
        rgb888ToRgb32ConverterPtr = qt_convert_rgb888_to_rgb32_neon;
#endif // QT_HAVE_NEON
#if defined(QT_HAVE_SSSE3)
    // from qimage_ssse3.cpp
    Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_ssse3(quint32 *dst, const uchar *src, int len);

    if (features & SSSE3)
        rgb888ToRgb32ConverterPtr = qt_convert_rgb888_to_rgb32_ssse3;
#endif // QT_HAVE_SSSE3
}

QJpegHandler::~QJpegHandler()
{
    delete d;
}

bool QJpegHandler::canRead() const
{
    if(d->state == QJpegHandlerPrivate::Ready && !canRead(device()))
        return false;

    if (d->state != QJpegHandlerPrivate::Error) {
        setFormat("jpeg");
        return true;
    }

    return false;
}

bool QJpegHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QJpegHandler::canRead() called with no device");
        return false;
    }

    char buffer[2];
    if (device->peek(buffer, 2) != 2)
        return false;
    return uchar(buffer[0]) == 0xff && uchar(buffer[1]) == 0xd8;
}

bool QJpegHandler::read(QImage *image)
{
    if (!canRead())
        return false;
    return d->read(image);
}

bool QJpegHandler::write(const QImage &image)
{
    return write_jpeg_image(image, device(), d->quality);
}

bool QJpegHandler::supportsOption(ImageOption option) const
{
    return option == Quality
        || option == ScaledSize
        || option == ScaledClipRect
        || option == ClipRect
        || option == Size
        || option == ImageFormat;
}

QVariant QJpegHandler::option(ImageOption option) const
{
    switch(option) {
    case Quality:
        return d->quality;
    case ScaledSize:
        return d->scaledSize;
    case ScaledClipRect:
        return d->scaledClipRect;
    case ClipRect:
        return d->clipRect;
    case Size:
        d->readJpegHeader(device());
        return d->size;
    case ImageFormat:
        d->readJpegHeader(device());
        return d->format;
    default:
        return QVariant();
    }
}

void QJpegHandler::setOption(ImageOption option, const QVariant &value)
{
    switch(option) {
    case Quality:
        d->quality = value.toInt();
        break;
    case ScaledSize:
        d->scaledSize = value.toSize();
        break;
    case ScaledClipRect:
        d->scaledClipRect = value.toRect();
        break;
    case ClipRect:
        d->clipRect = value.toRect();
        break;
    default:
        break;
    }
}

QByteArray QJpegHandler::name() const
{
    return "jpeg";
}




QT_END_NAMESPACE
