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

/*!
    \class QImageWriter
    \brief The QImageWriter class provides a format independent interface
    for writing images to files or other devices.

    \reentrant
    \ingroup painting
    \ingroup io

    QImageWriter supports setting format specific options, such as the
    gamma level, compression level and quality, prior to storing the
    image. If you do not need such options, you can use QImage::save()
    or QPixmap::save() instead.

    To store an image, you start by constructing a QImageWriter
    object.  Pass either a file name or a device pointer, and the
    image format to QImageWriter's constructor. You can then set
    several options, such as the gamma level (by calling setGamma())
    and quality (by calling setQuality()). canWrite() returns true if
    QImageWriter can write the image (i.e., the image format is
    supported and the device is open for writing). Call write() to
    write the image to the device.

    If any error occurs when writing the image, write() will return
    false. You can then call error() to find the type of error that
    occurred, or errorString() to get a human readable description of
    what went wrong.

    Call supportedImageFormats() for a list of formats that
    QImageWriter can write. QImageWriter supports all built-in image
    formats, in addition to any image format plugins that support
    writing.

    \sa QImageReader, QImageIOHandler, QImageIOPlugin
*/

/*!
    \enum QImageWriter::ImageWriterError

    This enum describes errors that can occur when writing images with
    QImageWriter.

    \value DeviceError QImageWriter encountered a device error when
    writing the image data. Consult your device for more details on
    what went wrong.

    \value UnsupportedFormatError Qt does not support the requested
    image format.

    \value UnknownError An unknown error occurred. If you get this
    value after calling write(), it is most likely caused by a bug in
    QImageWriter.
*/

#include "qimagewriter.h"

#include <qbytearray.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimageiohandler.h>
#include <qset.h>
#include <qvariant.h>

// factory loader
#include <qcoreapplication.h>
#include <private/qfactoryloader_p.h>

// image handlers
#include <private/qbmphandler_p.h>
#include <private/qppmhandler_p.h>
#include <private/qxbmhandler_p.h>
#include <private/qxpmhandler_p.h>
#ifndef QT_NO_IMAGEFORMAT_PNG
#include <private/qpnghandler_p.h>
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
#include <private/qjpeghandler_p.h>
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
#include <private/qmnghandler_p.h>
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
#include <private/qtiffhandler_p.h>
#endif
#ifdef QT_BUILTIN_GIF_READER
#include <private/qgifhandler_p.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QImageIOHandlerFactoryInterface_iid, QLatin1String("/imageformats")))
#endif

static QImageIOHandler *createWriteHandlerHelper(QIODevice *device,
    const QByteArray &format)
{
    QByteArray form = format.toLower();
    QByteArray suffix;
    QImageIOHandler *handler = 0;

#ifndef QT_NO_LIBRARY
    // check if any plugins can write the image
    QFactoryLoader *l = loader();
    QStringList keys = l->keys();
    int suffixPluginIndex = -1;
#endif

    if (device && format.isEmpty()) {
        // if there's no format, see if \a device is a file, and if so, find
        // the file suffix and find support for that format among our plugins.
        // this allows plugins to override our built-in handlers.
        if (QFile *file = qobject_cast<QFile *>(device)) {
            if (!(suffix = QFileInfo(file->fileName()).suffix().toLower().toLatin1()).isEmpty()) {
#ifndef QT_NO_LIBRARY
                int index = keys.indexOf(QString::fromLatin1(suffix));
                if (index != -1)
                    suffixPluginIndex = index;
#endif
            }
        }
    }

    QByteArray testFormat = !form.isEmpty() ? form : suffix;

#ifndef QT_NO_LIBRARY
    if (suffixPluginIndex != -1) {
        // when format is missing, check if we can find a plugin for the
        // suffix.
        QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(QString::fromLatin1(suffix)));
        if (plugin && (plugin->capabilities(device, suffix) & QImageIOPlugin::CanWrite))
            handler = plugin->create(device, suffix);
    }
#endif // QT_NO_LIBRARY

    // check if any built-in handlers can write the image
    if (!handler && !testFormat.isEmpty()) {
        if (false) {
#ifndef QT_NO_IMAGEFORMAT_PNG
        } else if (testFormat == "png") {
            handler = new QPngHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
        } else if (testFormat == "jpg" || testFormat == "jpeg") {
            handler = new QJpegHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
        } else if (testFormat == "mng") {
            handler = new QMngHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
        } else if (testFormat == "tif" || testFormat == "tiff") {
            handler = new QTiffHandler;
#endif
#ifdef QT_BUILTIN_GIF_READER
        } else if (testFormat == "gif") {
            handler = new QGifHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_BMP
        } else if (testFormat == "bmp") {
            handler = new QBmpHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
        } else if (testFormat == "xpm") {
            handler = new QXpmHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
        } else if (testFormat == "xbm") {
            handler = new QXbmHandler;
            handler->setOption(QImageIOHandler::SubType, testFormat);
#endif
#ifndef QT_NO_IMAGEFORMAT_PPM
        } else if (testFormat == "pbm" || testFormat == "pbmraw" || testFormat == "pgm"
                 || testFormat == "pgmraw" || testFormat == "ppm" || testFormat == "ppmraw") {
            handler = new QPpmHandler;
            handler->setOption(QImageIOHandler::SubType, testFormat);
#endif
        }
    }

#ifndef QT_NO_LIBRARY
    if (!testFormat.isEmpty()) {
        for (int i = 0; i < keys.size(); ++i) {
            QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(keys.at(i)));
            if (plugin && (plugin->capabilities(device, testFormat) & QImageIOPlugin::CanWrite)) {
                delete handler;
                handler = plugin->create(device, testFormat);
                break;
            }
        }
    }
#endif // QT_NO_LIBRARY

    if (!handler)
        return 0;

    handler->setDevice(device);
    if (!testFormat.isEmpty())
        handler->setFormat(testFormat);
    return handler;
}

class QImageWriterPrivate
{
public:
    QImageWriterPrivate(QImageWriter *qq);

    // device
    QByteArray format;
    QIODevice *device;
    bool deleteDevice;
    QImageIOHandler *handler;

    // image options
    int quality;
    int compression;
    float gamma;
    QString description;
    QString text;

    // error
    QImageWriter::ImageWriterError imageWriterError;
    QString errorString;

    QImageWriter *q;
};

/*!
    \internal
*/
QImageWriterPrivate::QImageWriterPrivate(QImageWriter *qq)
{
    device = 0;
    deleteDevice = false;
    handler = 0;
    quality = -1;
    compression = 0;
    gamma = 0.0;
    imageWriterError = QImageWriter::UnknownError;
    errorString = QT_TRANSLATE_NOOP(QImageWriter, QLatin1String("Unknown error"));

    q = qq;
}

/*!
    Constructs an empty QImageWriter object. Before writing, you must
    call setFormat() to set an image format, then setDevice() or
    setFileName().
*/
QImageWriter::QImageWriter()
    : d(new QImageWriterPrivate(this))
{
}

/*!
    Constructs a QImageWriter object using the device \a device and
    image format \a format.
*/
QImageWriter::QImageWriter(QIODevice *device, const QByteArray &format)
    : d(new QImageWriterPrivate(this))
{
    d->device = device;
    d->format = format;
}

/*!
    Constructs a QImageWriter objects that will write to a file with
    the name \a fileName, using the image format \a format. If \a
    format is not provided, QImageWriter will detect the image format
    by inspecting the extension of \a fileName.
*/
QImageWriter::QImageWriter(const QString &fileName, const QByteArray &format)
    : d(new QImageWriterPrivate(this))
{
    QFile *file = new QFile(fileName);
    d->device = file;
    d->deleteDevice = true;
    d->format = format;
}

/*!
    Destructs the QImageWriter object.
*/
QImageWriter::~QImageWriter()
{
    if (d->deleteDevice)
        delete d->device;
    delete d->handler;
    delete d;
}

/*!
    Sets the format QImageWriter will use when writing images, to \a
    format. \a format is a case insensitive text string. Example:

    \snippet doc/src/snippets/code/src_gui_image_qimagewriter.cpp 0

    You can call supportedImageFormats() for the full list of formats
    QImageWriter supports.

    \sa format()
*/
void QImageWriter::setFormat(const QByteArray &format)
{
    d->format = format;
}

/*!
    Returns the format QImageWriter uses for writing images.

    \sa setFormat()
*/
QByteArray QImageWriter::format() const
{
    return d->format;
}

/*!
    Sets QImageWriter's device to \a device. If a device has already
    been set, the old device is removed from QImageWriter and is
    otherwise left unchanged.

    If the device is not already open, QImageWriter will attempt to
    open the device in \l QIODevice::WriteOnly mode by calling
    open(). Note that this does not work for certain devices, such as
    QProcess, QTcpSocket and QUdpSocket, where more logic is required
    to open the device.

    \sa device(), setFileName()
*/
void QImageWriter::setDevice(QIODevice *device)
{
    if (d->device && d->deleteDevice)
        delete d->device;

    d->device = device;
    d->deleteDevice = false;
    delete d->handler;
    d->handler = 0;
}

/*!
    Returns the device currently assigned to QImageWriter, or 0 if no
    device has been assigned.
*/
QIODevice *QImageWriter::device() const
{
    return d->device;
}

/*!
    Sets the file name of QImageWriter to \a fileName. Internally,
    QImageWriter will create a QFile and open it in \l
    QIODevice::WriteOnly mode, and use this file when writing images.

    \sa fileName(), setDevice()
*/
void QImageWriter::setFileName(const QString &fileName)
{
    setDevice(new QFile(fileName));
    d->deleteDevice = true;
}

/*!
    If the currently assigned device is a QFile, or if setFileName()
    has been called, this function returns the name of the file
    QImageWriter writes to. Otherwise (i.e., if no device has been
    assigned or the device is not a QFile), an empty QString is
    returned.

    \sa setFileName(), setDevice()
*/
QString QImageWriter::fileName() const
{
    QFile *file = qobject_cast<QFile *>(d->device);
    return file ? file->fileName() : QString();
}

/*!
    This is an image format specific function that sets the quality
    level of the image to \a quality. For image formats that do not
    support setting the quality, this value is ignored.

    The value range of \a quality depends on the image format. For
    example, the "jpeg" format supports a quality range from 0 (low
    quality, high compression) to 100 (high quality, low compression).

    \sa quality()
*/
void QImageWriter::setQuality(int quality)
{
    d->quality = quality;
}

/*!
    Returns the quality level of the image.

    \sa setQuality()
*/
int QImageWriter::quality() const
{
    return d->quality;
}

/*!
    This is an image format specific function that set the compression
    of an image. For image formats that do not support setting the
    compression, this value is ignored.

    The value range of \a compression depends on the image format. For
    example, the "tiff" format supports two values, 0(no compression) and
    1(LZW-compression).

    \sa compression()
*/
void QImageWriter::setCompression(int compression)
{
    d->compression = compression;
}

/*!
    Returns the compression of the image.

    \sa setCompression()
*/
int QImageWriter::compression() const
{
    return d->compression;
}

/*!
    This is an image format specific function that sets the gamma
    level of the image to \a gamma. For image formats that do not
    support setting the gamma level, this value is ignored.

    The value range of \a gamma depends on the image format. For
    example, the "png" format supports a gamma range from 0.0 to 1.0.

    \sa quality()
*/
void QImageWriter::setGamma(float gamma)
{
    d->gamma = gamma;
}

/*!
    Returns the gamma level of the image.

    \sa setGamma()
*/
float QImageWriter::gamma() const
{
    return d->gamma;
}

/*!
    \obsolete

    Use setText() instead.

    This is an image format specific function that sets the
    description of the image to \a description. For image formats that
    do not support setting the description, this value is ignored.

    The contents of \a description depends on the image format.

    \sa description()
*/
void QImageWriter::setDescription(const QString &description)
{
    d->description = description;
}

/*!
    \obsolete

    Use QImageReader::text() instead.

    Returns the description of the image.

    \sa setDescription()
*/
QString QImageWriter::description() const
{
    return d->description;
}

/*!
    \since 4.1

    Sets the image text associated with the key \a key to
    \a text. This is useful for storing copyright information
    or other information about the image. Example:

    \snippet doc/src/snippets/code/src_gui_image_qimagewriter.cpp 1

    If you want to store a single block of data
    (e.g., a comment), you can pass an empty key, or use
    a generic key like "Description".

    The key and text will be embedded into the
    image data after calling write().

    Support for this option is implemented through
    QImageIOHandler::Description.

    \sa QImage::setText(), QImageReader::text()
*/
void QImageWriter::setText(const QString &key, const QString &text)
{
    if (!d->description.isEmpty())
        d->description += QLatin1String("\n\n");
    d->description += key.simplified() + QLatin1String(": ") + text.simplified();
}

/*!
    Returns true if QImageWriter can write the image; i.e., the image
    format is supported and the assigned device is open for reading.

    \sa write(), setDevice(), setFormat()
*/
bool QImageWriter::canWrite() const
{
    if (d->device && !d->handler && (d->handler = createWriteHandlerHelper(d->device, d->format)) == 0) {
        d->imageWriterError = QImageWriter::UnsupportedFormatError;
        d->errorString = QT_TRANSLATE_NOOP(QImageWriter,
                                           QLatin1String("Unsupported image format"));
        return false;
    }
    if (d->device && !d->device->isOpen())
        d->device->open(QIODevice::WriteOnly);
    if (!d->device || !d->device->isWritable()) {
        d->imageWriterError = QImageWriter::DeviceError;
        d->errorString = QT_TRANSLATE_NOOP(QImageWriter,
                                           QLatin1String("Device not writable"));
        return false;
    }
    return true;
}

/*!
    Writes the image \a image to the assigned device or file
    name. Returns true on success; otherwise returns false. If the
    operation fails, you can call error() to find the type of error
    that occurred, or errorString() to get a human readable
    description of the error.

    \sa canWrite(), error(), errorString()
*/
bool QImageWriter::write(const QImage &image)
{
    if (!canWrite())
        return false;

    if (d->handler->supportsOption(QImageIOHandler::Quality))
        d->handler->setOption(QImageIOHandler::Quality, d->quality);
    if (d->handler->supportsOption(QImageIOHandler::CompressionRatio))
        d->handler->setOption(QImageIOHandler::CompressionRatio, d->compression);
    if (d->handler->supportsOption(QImageIOHandler::Gamma))
        d->handler->setOption(QImageIOHandler::Gamma, d->gamma);
    if (!d->description.isEmpty() && d->handler->supportsOption(QImageIOHandler::Description))
        d->handler->setOption(QImageIOHandler::Description, d->description);

    if (!d->handler->write(image))
        return false;
    if (QFile *file = qobject_cast<QFile *>(d->device))
        file->flush();
    return true;
}

/*!
    Returns the type of error that last occurred.

    \sa ImageWriterError, errorString()
*/
QImageWriter::ImageWriterError QImageWriter::error() const
{
    return d->imageWriterError;
}

/*!
    Returns a human readable description of the last error that occurred.

    \sa error()
*/
QString QImageWriter::errorString() const
{
    return d->errorString;
}

/*!
    \since 4.2

    Returns true if the writer supports \a option; otherwise returns
    false.

    Different image formats support different options. Call this function to
    determine whether a certain option is supported by the current format. For
    example, the PNG format allows you to embed text into the image's metadata
    (see text()).

    \snippet doc/src/snippets/code/src_gui_image_qimagewriter.cpp 2

    Options can be tested after the writer has been associated with a format.

    \sa QImageReader::supportsOption(), setFormat()
*/
bool QImageWriter::supportsOption(QImageIOHandler::ImageOption option) const
{
    if (!d->handler && (d->handler = createWriteHandlerHelper(d->device, d->format)) == 0) {
        d->imageWriterError = QImageWriter::UnsupportedFormatError;
        d->errorString = QT_TRANSLATE_NOOP(QImageWriter,
                                           QLatin1String("Unsupported image format"));
        return false;
    }

    return d->handler->supportsOption(option);
}

/*!
    Returns the list of image formats supported by QImageWriter.

    By default, Qt can write the following formats:

    \table
    \header \o Format \o Description
    \row    \o BMP    \o Windows Bitmap
    \row    \o JPG    \o Joint Photographic Experts Group
    \row    \o JPEG   \o Joint Photographic Experts Group
    \row    \o PNG    \o Portable Network Graphics
    \row    \o PPM    \o Portable Pixmap
    \row    \o TIFF   \o Tagged Image File Format
    \row    \o XBM    \o X11 Bitmap
    \row    \o XPM    \o X11 Pixmap
    \endtable

    Reading and writing SVG files is supported through Qt's
    \l{QtSvg Module}{SVG Module}.

    Note that the QApplication instance must be created before this function is
    called.

    \sa setFormat(), QImageReader::supportedImageFormats(), QImageIOPlugin
*/
QList<QByteArray> QImageWriter::supportedImageFormats()
{
    QSet<QByteArray> formats;
    formats << "bmp";
#ifndef QT_NO_IMAGEFORMAT_PPM
    formats << "ppm";
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
    formats << "xbm";
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
    formats << "xpm";
#endif
#ifndef QT_NO_IMAGEFORMAT_PNG
    formats << "png";
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
    formats << "jpg" << "jpeg";
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
    formats << "mng";
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
    formats << "tif" << "tiff";
#endif
#ifdef QT_BUILTIN_GIF_READER
    formats << "gif";
#endif

#ifndef QT_NO_LIBRARY
    QFactoryLoader *l = loader();
    QStringList keys = l->keys();
    for (int i = 0; i < keys.count(); ++i) {
        QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(keys.at(i)));
        if (plugin && (plugin->capabilities(0, keys.at(i).toLatin1()) & QImageIOPlugin::CanWrite) != 0)
            formats << keys.at(i).toLatin1();
    }
#endif // QT_NO_LIBRARY

    QList<QByteArray> sortedFormats;
    for (QSet<QByteArray>::ConstIterator it = formats.constBegin(); it != formats.constEnd(); ++it)
        sortedFormats << *it;

    qSort(sortedFormats);
    return sortedFormats;
}

QT_END_NAMESPACE
