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

#include "private/qpnghandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_PNG
#include <qcoreapplication.h>
#include <qiodevice.h>
#include <qimage.h>
#include <qlist.h>
#include <qtextcodec.h>
#include <qvariant.h>
#include <qvector.h>

#ifdef QT_USE_BUNDLED_LIBPNG
#include <../../3rdparty/libpng/png.h>
#include <../../3rdparty/libpng/pngconf.h>
#else
#include <png.h>
#include <pngconf.h>
#endif

#if PNG_LIBPNG_VER >= 10400 && PNG_LIBPNG_VER <= 10502 \
        && defined(PNG_PEDANTIC_WARNINGS_SUPPORTED)
/*
    Versions 1.4.0 to 1.5.2 of libpng declare png_longjmp_ptr to
    have a noreturn attribute if PNG_PEDANTIC_WARNINGS_SUPPORTED
    is enabled, but most declarations of longjmp in the wild do
    not add this attribute. This causes problems when the png_jmpbuf
    macro expands to calling png_set_longjmp_fn with a mismatched
    longjmp, as compilers such as Clang will treat this as an error.

    To work around this we override the png_jmpbuf macro to cast
    longjmp to a png_longjmp_ptr.
*/
#   undef png_jmpbuf
#   ifdef PNG_SETJMP_SUPPORTED
#       define png_jmpbuf(png_ptr) \
            (*png_set_longjmp_fn((png_ptr), (png_longjmp_ptr)longjmp, sizeof(jmp_buf)))
#   else
#       define png_jmpbuf(png_ptr) \
            (LIBPNG_WAS_COMPILED_WITH__PNG_NO_SETJMP)
#   endif
#endif

#ifdef Q_OS_WINCE
#define CALLBACK_CALL_TYPE        __cdecl
#else
#define CALLBACK_CALL_TYPE
#endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WINCE) && defined(STANDARDSHELL_UI_MODEL)
#  define Q_INTERNAL_WIN_NO_THROW __declspec(nothrow)
#else
#  define Q_INTERNAL_WIN_NO_THROW
#endif

// avoid going through QImage::scanLine() which calls detach
#define FAST_SCAN_LINE(data, bpl, y) (data + (y) * bpl)

/*
  All PNG files load to the minimal QImage equivalent.

  All QImage formats output to reasonably efficient PNG equivalents.
  Never to grayscale.
*/

class QPngHandlerPrivate
{
public:
    enum State {
        Ready,
        ReadHeader,
        ReadingEnd,
        Error
    };

    QPngHandlerPrivate(QPngHandler *qq)
        : gamma(0.0), quality(2), png_ptr(0), info_ptr(0),
          end_info(0), row_pointers(0), state(Ready), q(qq)
    { }

    float gamma;
    int quality;
    QString description;
    QStringList readTexts;

    png_struct *png_ptr;
    png_info *info_ptr;
    png_info *end_info;
    png_byte **row_pointers;

    bool readPngHeader();
    bool readPngImage(QImage *image);
    void readPngTexts(png_info *info);

    QImage::Format readImageFormat();

    State state;

    QPngHandler *q;
};


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

class QPNGImageWriter {
public:
    explicit QPNGImageWriter(QIODevice*);
    ~QPNGImageWriter();

    enum DisposalMethod { Unspecified, NoDisposal, RestoreBackground, RestoreImage };
    void setDisposalMethod(DisposalMethod);
    void setLooping(int loops=0); // 0 == infinity
    void setFrameDelay(int msecs);
    void setGamma(float);

    bool writeImage(const QImage& img, int x, int y);
    bool writeImage(const QImage& img, int quality, const QString &description, int x, int y);
    bool writeImage(const QImage& img)
        { return writeImage(img, 0, 0); }
    bool writeImage(const QImage& img, int quality, const QString &description)
        { return writeImage(img, quality, description, 0, 0); }

    QIODevice* device() { return dev; }

private:
    QIODevice* dev;
    int frames_written;
    DisposalMethod disposal;
    int looping;
    int ms_delay;
    float gamma;
};

static
void CALLBACK_CALL_TYPE iod_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QPngHandlerPrivate *d = (QPngHandlerPrivate *)png_get_io_ptr(png_ptr);
    QIODevice *in = d->q->device();

    if (d->state == QPngHandlerPrivate::ReadingEnd && !in->isSequential() && (in->size() - in->pos()) < 4 && length == 4) {
        // Workaround for certain malformed PNGs that lack the final crc bytes
        uchar endcrc[4] = { 0xae, 0x42, 0x60, 0x82 };
        qMemCopy(data, endcrc, 4);
        in->seek(in->size());
        return;
    }

    while (length) {
        int nr = in->read((char*)data, length);
        if (nr <= 0) {
            png_error(png_ptr, "Read Error");
            return;
        }
        length -= nr;
    }
}


static
void CALLBACK_CALL_TYPE qpiw_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QPNGImageWriter* qpiw = (QPNGImageWriter*)png_get_io_ptr(png_ptr);
    QIODevice* out = qpiw->device();

    uint nr = out->write((char*)data, length);
    if (nr != length) {
        png_error(png_ptr, "Write Error");
        return;
    }
}


static
void CALLBACK_CALL_TYPE qpiw_flush_fn(png_structp /* png_ptr */)
{
}

#if defined(Q_C_CALLBACKS)
}
#endif

static
void setup_qt(QImage& image, png_structp png_ptr, png_infop info_ptr, float screen_gamma=0.0)
{
    if (screen_gamma != 0.0 && png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)) {
        double file_gamma;
        png_get_gAMA(png_ptr, info_ptr, &file_gamma);
        png_set_gamma(png_ptr, screen_gamma, file_gamma);
    }

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_bytep trans_alpha = 0;
    png_color_16p trans_color_p = 0;
    int num_trans;
    png_colorp palette = 0;
    int num_palette;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
    png_set_interlace_handling(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY) {
        // Black & White or 8-bit grayscale
        if (bit_depth == 1 && png_get_channels(png_ptr, info_ptr) == 1) {
            png_set_invert_mono(png_ptr);
            png_read_update_info(png_ptr, info_ptr);
            if (image.size() != QSize(width, height) || image.format() != QImage::Format_Mono) {
                image = QImage(width, height, QImage::Format_Mono);
                if (image.isNull())
                    return;
            }
            image.setColorCount(2);
            image.setColor(1, qRgb(0,0,0));
            image.setColor(0, qRgb(255,255,255));
        } else if (bit_depth == 16 && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_expand(png_ptr);
            png_set_strip_16(png_ptr);
            png_set_gray_to_rgb(png_ptr);
            if (image.size() != QSize(width, height) || image.format() != QImage::Format_ARGB32) {
                image = QImage(width, height, QImage::Format_ARGB32);
                if (image.isNull())
                    return;
            }
            if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
                png_set_swap_alpha(png_ptr);

            png_read_update_info(png_ptr, info_ptr);
        } else {
            if (bit_depth == 16)
                png_set_strip_16(png_ptr);
            else if (bit_depth < 8)
                png_set_packing(png_ptr);
            int ncols = bit_depth < 8 ? 1 << bit_depth : 256;
            png_read_update_info(png_ptr, info_ptr);
            if (image.size() != QSize(width, height) || image.format() != QImage::Format_Indexed8) {
                image = QImage(width, height, QImage::Format_Indexed8);
                if (image.isNull())
                    return;
            }
            image.setColorCount(ncols);
            for (int i=0; i<ncols; i++) {
                int c = i*255/(ncols-1);
                image.setColor(i, qRgba(c,c,c,0xff));
            }
            if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, &trans_color_p) && trans_color_p) {
                const int g = trans_color_p->gray;
                if (g < ncols) {
                    image.setColor(g, 0);
                }
            }
        }
    } else if (color_type == PNG_COLOR_TYPE_PALETTE
               && png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)
               && num_palette <= 256)
    {
        // 1-bit and 8-bit color
        if (bit_depth != 1)
            png_set_packing(png_ptr);
        png_read_update_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
        QImage::Format format = bit_depth == 1 ? QImage::Format_Mono : QImage::Format_Indexed8;
        if (image.size() != QSize(width, height) || image.format() != format) {
            image = QImage(width, height, format);
            if (image.isNull())
                return;
        }
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
        image.setColorCount(num_palette);
        int i = 0;
        if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, &trans_color_p) && trans_alpha) {
            while (i < num_trans) {
                image.setColor(i, qRgba(
                    palette[i].red,
                    palette[i].green,
                    palette[i].blue,
                    trans_alpha[i]
                   )
               );
                i++;
            }
        }
        while (i < num_palette) {
            image.setColor(i, qRgba(
                palette[i].red,
                palette[i].green,
                palette[i].blue,
                0xff
               )
           );
            i++;
        }
    } else {
        // 32-bit
        if (bit_depth == 16)
            png_set_strip_16(png_ptr);

        png_set_expand(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        QImage::Format format = QImage::Format_ARGB32;
        // Only add filler if no alpha, or we can get 5 channel data.
        if (!(color_type & PNG_COLOR_MASK_ALPHA)
            && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_filler(png_ptr, 0xff, QSysInfo::ByteOrder == QSysInfo::BigEndian ?
                           PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
            // We want 4 bytes, but it isn't an alpha channel
            format = QImage::Format_RGB32;
        }
        if (image.size() != QSize(width, height) || image.format() != format) {
            image = QImage(width, height, format);
            if (image.isNull())
                return;
        }

        if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
            png_set_swap_alpha(png_ptr);

        png_read_update_info(png_ptr, info_ptr);
    }

    // Qt==ARGB==Big(ARGB)==Little(BGRA)
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        png_set_bgr(png_ptr);
    }
}


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif
static void CALLBACK_CALL_TYPE qt_png_warning(png_structp /*png_ptr*/, png_const_charp message)
{
    qWarning("libpng warning: %s", message);
}

#if defined(Q_C_CALLBACKS)
}
#endif


/*!
    \internal
*/
void Q_INTERNAL_WIN_NO_THROW QPngHandlerPrivate::readPngTexts(png_info *info)
{
#ifndef QT_NO_IMAGE_TEXT
    png_textp text_ptr;
    int num_text=0;
    png_get_text(png_ptr, info, &text_ptr, &num_text);

    while (num_text--) {
        QString key, value;
        key = QString::fromLatin1(text_ptr->key);
#if defined(PNG_iTXt_SUPPORTED)
        if (text_ptr->itxt_length) {
            value = QString::fromUtf8(text_ptr->text, int(text_ptr->itxt_length));
        } else
#endif
        {
            value = QString::fromLatin1(text_ptr->text, int(text_ptr->text_length));
        }
        if (!description.isEmpty())
            description += QLatin1String("\n\n");
        description += key + QLatin1String(": ") + value.simplified();
        readTexts.append(key);
        readTexts.append(value);
        text_ptr++;
    }
#endif
}


/*!
    \internal
*/
bool Q_INTERNAL_WIN_NO_THROW QPngHandlerPrivate::readPngHeader()
{
    state = Error;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if (!png_ptr)
        return false;

    png_set_error_fn(png_ptr, 0, 0, qt_png_warning);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        png_ptr = 0;
        return false;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        png_ptr = 0;
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        png_ptr = 0;
        return false;
    }

    png_set_read_fn(png_ptr, this, iod_read_fn);
    png_read_info(png_ptr, info_ptr);

    readPngTexts(info_ptr);

    state = ReadHeader;
    return true;
}

/*!
    \internal
*/
bool Q_INTERNAL_WIN_NO_THROW QPngHandlerPrivate::readPngImage(QImage *outImage)
{
    if (state == Error)
        return false;

    if (state == Ready && !readPngHeader()) {
        state = Error;
        return false;
    }

    row_pointers = 0;
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete [] row_pointers;
        png_ptr = 0;
        state = Error;
        return false;
    }

    setup_qt(*outImage, png_ptr, info_ptr, gamma);

    if (outImage->isNull()) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete [] row_pointers;
        png_ptr = 0;
        state = Error;
        return false;
    }

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 0, 0, 0);

    uchar *data = outImage->bits();
    int bpl = outImage->bytesPerLine();
    row_pointers = new png_bytep[height];

    for (uint y = 0; y < height; y++)
        row_pointers[y] = data + y * bpl;

    png_read_image(png_ptr, row_pointers);

#if 0 // libpng takes care of this.
    png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
        if (outImage->depth()==32 && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            QRgb trans = 0xFF000000 | qRgb(
                (info_ptr->trans_values.red << 8 >> bit_depth)&0xff,
                (info_ptr->trans_values.green << 8 >> bit_depth)&0xff,
                (info_ptr->trans_values.blue << 8 >> bit_depth)&0xff);
            for (uint y=0; y<height; y++) {
                for (uint x=0; x<info_ptr->width; x++) {
                    if (((uint**)jt)[y][x] == trans) {
                        ((uint**)jt)[y][x] &= 0x00FFFFFF;
                    } else {
                    }
                }
            }
        }
#endif

    outImage->setDotsPerMeterX(png_get_x_pixels_per_meter(png_ptr,info_ptr));
    outImage->setDotsPerMeterY(png_get_y_pixels_per_meter(png_ptr,info_ptr));

    state = ReadingEnd;
    png_read_end(png_ptr, end_info);

#ifndef QT_NO_IMAGE_TEXT
    readPngTexts(end_info);
    for (int i = 0; i < readTexts.size()-1; i+=2)
        outImage->setText(readTexts.at(i), readTexts.at(i+1));
#endif

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    delete [] row_pointers;
    png_ptr = 0;
    state = Ready;

    // sanity check palette entries
    if (color_type == PNG_COLOR_TYPE_PALETTE
        && outImage->format() == QImage::Format_Indexed8) {
        int color_table_size = outImage->colorCount();
        for (int y=0; y<(int)height; ++y) {
            uchar *p = FAST_SCAN_LINE(data, bpl, y);
            uchar *end = p + width;
            while (p < end) {
                if (*p >= color_table_size)
                    *p = 0;
                ++p;
            }
        }
    }

    return true;
}

QImage::Format QPngHandlerPrivate::readImageFormat()
{
        QImage::Format format = QImage::Format_Invalid;
        png_uint_32 width, height;
        int bit_depth, color_type;
        png_colorp palette;
        int num_palette;
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
        if (color_type == PNG_COLOR_TYPE_GRAY) {
            // Black & White or 8-bit grayscale
            if (bit_depth == 1 && png_get_channels(png_ptr, info_ptr) == 1) {
                format = QImage::Format_Mono;
            } else if (bit_depth == 16 && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                format = QImage::Format_ARGB32;
            } else {
                format = QImage::Format_Indexed8;
            }
        } else if (color_type == PNG_COLOR_TYPE_PALETTE
                   && png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)
                   && num_palette <= 256)
        {
            // 1-bit and 8-bit color
            if (bit_depth != 1)
                png_set_packing(png_ptr);
            png_read_update_info(png_ptr, info_ptr);
            png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
            format = bit_depth == 1 ? QImage::Format_Mono : QImage::Format_Indexed8;
        } else {
            // 32-bit
            if (bit_depth == 16)
                png_set_strip_16(png_ptr);

            format = QImage::Format_ARGB32;
            // Only add filler if no alpha, or we can get 5 channel data.
            if (!(color_type & PNG_COLOR_MASK_ALPHA)
                && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                // We want 4 bytes, but it isn't an alpha channel
                format = QImage::Format_RGB32;
            }
        }

        return format;
}

QPNGImageWriter::QPNGImageWriter(QIODevice* iod) :
    dev(iod),
    frames_written(0),
    disposal(Unspecified),
    looping(-1),
    ms_delay(-1),
    gamma(0.0)
{
}

QPNGImageWriter::~QPNGImageWriter()
{
}

void QPNGImageWriter::setDisposalMethod(DisposalMethod dm)
{
    disposal = dm;
}

void QPNGImageWriter::setLooping(int loops)
{
    looping = loops;
}

void QPNGImageWriter::setFrameDelay(int msecs)
{
    ms_delay = msecs;
}

void QPNGImageWriter::setGamma(float g)
{
    gamma = g;
}


#ifndef QT_NO_IMAGE_TEXT
static void set_text(const QImage &image, png_structp png_ptr, png_infop info_ptr,
                     const QString &description)
{
    QMap<QString, QString> text;
    foreach (const QString &key, image.textKeys()) {
        if (!key.isEmpty())
            text.insert(key, image.text(key));
    }
    foreach (const QString &pair, description.split(QLatin1String("\n\n"))) {
        int index = pair.indexOf(QLatin1Char(':'));
        if (index >= 0 && pair.indexOf(QLatin1Char(' ')) < index) {
            QString s = pair.simplified();
            if (!s.isEmpty())
                text.insert(QLatin1String("Description"), s);
        } else {
            QString key = pair.left(index);
            if (!key.simplified().isEmpty())
                text.insert(key, pair.mid(index + 2).simplified());
        }
    }

    if (text.isEmpty())
        return;

    png_textp text_ptr = new png_text[text.size()];
    qMemSet(text_ptr, 0, text.size() * sizeof(png_text));

    QMap<QString, QString>::ConstIterator it = text.constBegin();
    int i = 0;
    while (it != text.constEnd()) {
        text_ptr[i].key = qstrdup(it.key().left(79).toLatin1().constData());
        bool noCompress = (it.value().length() < 40);

#ifdef PNG_iTXt_SUPPORTED
        bool needsItxt = false;
        foreach(const QChar c, it.value()) {
            uchar ch = c.cell();
            if (c.row() || (ch < 0x20 && ch != '\n') || (ch > 0x7e && ch < 0xa0)) {
                needsItxt = true;
                break;
            }
        }

        if (needsItxt) {
            text_ptr[i].compression = noCompress ? PNG_ITXT_COMPRESSION_NONE : PNG_ITXT_COMPRESSION_zTXt;
            QByteArray value = it.value().toUtf8();
            text_ptr[i].text = qstrdup(value.constData());
            text_ptr[i].itxt_length = value.size();
            text_ptr[i].lang = const_cast<char*>("UTF-8");
            text_ptr[i].lang_key = qstrdup(it.key().toUtf8().constData());
        }
        else
#endif
        {
            text_ptr[i].compression = noCompress ? PNG_TEXT_COMPRESSION_NONE : PNG_TEXT_COMPRESSION_zTXt;
            QByteArray value = it.value().toLatin1();
            text_ptr[i].text = qstrdup(value.constData());
            text_ptr[i].text_length = value.size();
        }
        ++i;
        ++it;
    }

    png_set_text(png_ptr, info_ptr, text_ptr, i);
    for (i = 0; i < text.size(); ++i) {
        delete [] text_ptr[i].key;
        delete [] text_ptr[i].text;
#ifdef PNG_iTXt_SUPPORTED
        delete [] text_ptr[i].lang_key;
#endif
    }
    delete [] text_ptr;
}
#endif

bool QPNGImageWriter::writeImage(const QImage& image, int off_x, int off_y)
{
    return writeImage(image, -1, QString(), off_x, off_y);
}

bool Q_INTERNAL_WIN_NO_THROW QPNGImageWriter::writeImage(const QImage& image, int quality_in, const QString &description,
                                 int off_x_in, int off_y_in)
{
#ifdef QT_NO_IMAGE_TEXT
    Q_UNUSED(description);
#endif

    QPoint offset = image.offset();
    int off_x = off_x_in + offset.x();
    int off_y = off_y_in + offset.y();

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if (!png_ptr) {
        return false;
    }

    png_set_error_fn(png_ptr, 0, 0, qt_png_warning);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, 0);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    int quality = quality_in;
    if (quality >= 0) {
        if (quality > 9) {
            qWarning("PNG: Quality %d out of range", quality);
            quality = 9;
        }
        png_set_compression_level(png_ptr, quality);
    }

    png_set_write_fn(png_ptr, (void*)this, qpiw_write_fn, qpiw_flush_fn);


    int color_type = 0;
    if (image.colorCount())
        color_type = PNG_COLOR_TYPE_PALETTE;
    else if (image.hasAlphaChannel())
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    else
        color_type = PNG_COLOR_TYPE_RGB;

    png_set_IHDR(png_ptr, info_ptr, image.width(), image.height(),
                 image.depth() == 1 ? 1 : 8, // per channel
                 color_type, 0, 0, 0);       // sets #channels

    if (gamma != 0.0) {
        png_set_gAMA(png_ptr, info_ptr, 1.0/gamma);
    }

    png_color_8 sig_bit;
    sig_bit.red = 8;
    sig_bit.green = 8;
    sig_bit.blue = 8;
    sig_bit.alpha = image.hasAlphaChannel() ? 8 : 0;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    if (image.format() == QImage::Format_MonoLSB)
       png_set_packswap(png_ptr);

    if (image.colorCount()) {
        // Paletted
        int num_palette = qMin(256, image.colorCount());
        png_color palette[256];
        png_byte trans[256];
        int num_trans = 0;
        for (int i=0; i<num_palette; i++) {
            QRgb rgba=image.color(i);
            palette[i].red = qRed(rgba);
            palette[i].green = qGreen(rgba);
            palette[i].blue = qBlue(rgba);
            trans[i] = qAlpha(rgba);
            if (trans[i] < 255) {
                num_trans = i+1;
            }
        }
        png_set_PLTE(png_ptr, info_ptr, palette, num_palette);

        if (num_trans) {
            png_set_tRNS(png_ptr, info_ptr, trans, num_trans, 0);
        }
    }

    // Swap ARGB to RGBA (normal PNG format) before saving on
    // BigEndian machines
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        png_set_swap_alpha(png_ptr);
    }

    // Qt==ARGB==Big(ARGB)==Little(BGRA). But RGB888 is RGB regardless
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian
        && image.format() != QImage::Format_RGB888) {
        png_set_bgr(png_ptr);
    }

    if (off_x || off_y) {
        png_set_oFFs(png_ptr, info_ptr, off_x, off_y, PNG_OFFSET_PIXEL);
    }

    if (frames_written > 0)
        png_set_sig_bytes(png_ptr, 8);

    if (image.dotsPerMeterX() > 0 || image.dotsPerMeterY() > 0) {
        png_set_pHYs(png_ptr, info_ptr,
                image.dotsPerMeterX(), image.dotsPerMeterY(),
                PNG_RESOLUTION_METER);
    }

#ifndef QT_NO_IMAGE_TEXT
    set_text(image, png_ptr, info_ptr, description);
#endif
    png_write_info(png_ptr, info_ptr);

    if (image.depth() != 1)
        png_set_packing(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB && image.format() != QImage::Format_RGB888)
        png_set_filler(png_ptr, 0,
            QSysInfo::ByteOrder == QSysInfo::BigEndian ?
                PNG_FILLER_BEFORE : PNG_FILLER_AFTER);

    if (looping >= 0 && frames_written == 0) {
        uchar data[13] = "NETSCAPE2.0";
        //                0123456789aBC
        data[0xB] = looping%0x100;
        data[0xC] = looping/0x100;
        png_write_chunk(png_ptr, (png_byte*)"gIFx", data, 13);
    }
    if (ms_delay >= 0 || disposal!=Unspecified) {
        uchar data[4];
        data[0] = disposal;
        data[1] = 0;
        data[2] = (ms_delay/10)/0x100; // hundredths
        data[3] = (ms_delay/10)%0x100;
        png_write_chunk(png_ptr, (png_byte*)"gIFg", data, 4);
    }

    int height = image.height();
    int width = image.width();
    switch (image.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_RGB888:
        {
            png_bytep* row_pointers = new png_bytep[height];
            for (int y=0; y<height; y++)
                row_pointers[y] = (png_bytep)image.constScanLine(y);
            png_write_image(png_ptr, row_pointers);
            delete [] row_pointers;
        }
        break;
    default:
        {
            QImage::Format fmt = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
            QImage row;
            png_bytep row_pointers[1];
            for (int y=0; y<height; y++) {
                row = image.copy(0, y, width, 1).convertToFormat(fmt);
                row_pointers[0] = png_bytep(row.constScanLine(0));
                png_write_rows(png_ptr, row_pointers, 1);
            }
        }
        break;
    }

    png_write_end(png_ptr, info_ptr);
    frames_written++;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
}

static bool write_png_image(const QImage &image, QIODevice *device,
                            int quality, float gamma, const QString &description)
{
    QPNGImageWriter writer(device);
    if (quality >= 0) {
        quality = qMin(quality, 100);
        quality = (100-quality) * 9 / 91; // map [0,100] -> [9,0]
    }
    writer.setGamma(gamma);
    return writer.writeImage(image, quality, description);
}

QPngHandler::QPngHandler()
    : d(new QPngHandlerPrivate(this))
{
}

QPngHandler::~QPngHandler()
{
    if (d->png_ptr)
        png_destroy_read_struct(&d->png_ptr, &d->info_ptr, &d->end_info);
    delete d;
}

bool QPngHandler::canRead() const
{
    if (d->state == QPngHandlerPrivate::Ready && !canRead(device()))
        return false;

    if (d->state != QPngHandlerPrivate::Error) {
        setFormat("png");
        return true;
    }

    return false;
}

bool QPngHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QPngHandler::canRead() called with no device");
        return false;
    }

    return device->peek(8) == "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";
}

bool QPngHandler::read(QImage *image)
{
    if (!canRead())
        return false;
    return d->readPngImage(image);
}

bool QPngHandler::write(const QImage &image)
{
    return write_png_image(image, device(), d->quality, d->gamma, d->description);
}

bool QPngHandler::supportsOption(ImageOption option) const
{
    return option == Gamma
        || option == Description
        || option == ImageFormat
        || option == Quality
        || option == Size;
}

QVariant QPngHandler::option(ImageOption option) const
{
    if (d->state == QPngHandlerPrivate::Error)
        return QVariant();
    if (d->state == QPngHandlerPrivate::Ready && !d->readPngHeader())
        return QVariant();

    if (option == Gamma)
        return d->gamma;
    else if (option == Quality)
        return d->quality;
    else if (option == Description)
        return d->description;
    else if (option == Size)
        return QSize(png_get_image_width(d->png_ptr, d->info_ptr),
                     png_get_image_height(d->png_ptr, d->info_ptr));
    else if (option == ImageFormat)
        return d->readImageFormat();
    return 0;
}

void QPngHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Gamma)
        d->gamma = value.toFloat();
    else if (option == Quality)
        d->quality = value.toInt();
    else if (option == Description)
        d->description = value.toString();
}

QByteArray QPngHandler::name() const
{
    return "png";
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_PNG
