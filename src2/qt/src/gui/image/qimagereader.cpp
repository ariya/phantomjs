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

//#define QIMAGEREADER_DEBUG

/*!
    \class QImageReader
    \brief The QImageReader class provides a format independent interface
    for reading images from files or other devices.

    \reentrant
    \ingroup painting
    \ingroup io

    The most common way to read images is through QImage and QPixmap's
    constructors, or by calling QImage::load() and
    QPixmap::load(). QImageReader is a specialized class which gives
    you more control when reading images. For example, you can read an
    image into a specific size by calling setScaledSize(), and you can
    select a clip rect, effectively loading only parts of an image, by
    calling setClipRect(). Depending on the underlying support in the
    image format, this can save memory and speed up loading of images.

    To read an image, you start by constructing a QImageReader object.
    Pass either a file name or a device pointer, and the image format
    to QImageReader's constructor. You can then set several options,
    such as the clip rect (by calling setClipRect()) and scaled size
    (by calling setScaledSize()). canRead() returns the image if the
    QImageReader can read the image (i.e., the image format is
    supported and the device is open for reading). Call read() to read
    the image.

    If any error occurs when reading the image, read() will return a
    null QImage. You can then call error() to find the type of error
    that occurred, or errorString() to get a human readable
    description of what went wrong.

    Call supportedImageFormats() for a list of formats that
    QImageReader can read. QImageReader supports all built-in image
    formats, in addition to any image format plugins that support
    reading.

    QImageReader autodetects the image format by default, by looking at the
    provided (optional) format string, the file name suffix, and the data
    stream contents. You can enable or disable this feature, by calling
    setAutoDetectImageFormat().

    \sa QImageWriter, QImageIOHandler, QImageIOPlugin
*/

/*!
    \enum QImageReader::ImageReaderError

    This enum describes the different types of errors that can occur
    when reading images with QImageReader.

    \value FileNotFoundError QImageReader was used with a file name,
    but not file was found with that name. This can also happen if the
    file name contained no extension, and the file with the correct
    extension is not supported by Qt.

    \value DeviceError QImageReader encountered a device error when
    reading the image. You can consult your particular device for more
    details on what went wrong.

    \value UnsupportedFormatError Qt does not support the requested
    image format.

    \value InvalidDataError The image data was invalid, and
    QImageReader was unable to read an image from it. The can happen
    if the image file is damaged.

    \value UnknownError An unknown error occurred. If you get this
    value after calling read(), it is most likely caused by a bug in
    QImageReader.
*/
#include "qimagereader.h"

#include <qbytearray.h>
#ifdef QIMAGEREADER_DEBUG
#include <qdebug.h>
#endif
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qimageiohandler.h>
#include <qlist.h>
#include <qrect.h>
#include <qset.h>
#include <qsize.h>
#include <qcolor.h>
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

enum _qt_BuiltInFormatType {
#ifndef QT_NO_IMAGEFORMAT_PNG
    _qt_PngFormat,
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
    _qt_JpgFormat,
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
    _qt_MngFormat,
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
    _qt_TifFormat,
#endif
#ifdef QT_BUILTIN_GIF_READER
    _qt_GifFormat,
#endif
    _qt_BmpFormat,
#ifndef QT_NO_IMAGEFORMAT_PPM
    _qt_PpmFormat,
    _qt_PgmFormat,
    _qt_PbmFormat,
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
    _qt_XbmFormat,
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
    _qt_XpmFormat,
#endif
    _qt_NumFormats,
    _qt_NoFormat = -1
};

struct _qt_BuiltInFormatStruct
{
    _qt_BuiltInFormatType type;
    const char *extension;
};

static const _qt_BuiltInFormatStruct _qt_BuiltInFormats[] = {
#ifndef QT_NO_IMAGEFORMAT_PNG
    {_qt_PngFormat, "png"},
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
    {_qt_JpgFormat, "jpg"},
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
    {_qt_MngFormat, "mng"},
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
    {_qt_TifFormat, "tif"},
#endif
#ifdef QT_BUILTIN_GIF_READER
    {_qt_GifFormat, "gif"},
#endif
    {_qt_BmpFormat, "bmp"},
#ifndef QT_NO_IMAGEFORMAT_PPM
    {_qt_PpmFormat, "ppm"},
    {_qt_PgmFormat, "pgm"},
    {_qt_PbmFormat, "pbm"},
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
    {_qt_XbmFormat, "xbm"},
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
    {_qt_XpmFormat, "xpm"},
#endif
    {_qt_NoFormat, ""}
};

static QImageIOHandler *createReadHandlerHelper(QIODevice *device,
                                                const QByteArray &format,
                                                bool autoDetectImageFormat,
                                                bool ignoresFormatAndExtension)
{
    if (!autoDetectImageFormat && format.isEmpty())
        return 0;

    QByteArray form = format.toLower();
    QImageIOHandler *handler = 0;

#ifndef QT_NO_LIBRARY
    // check if we have plugins that support the image format
    QFactoryLoader *l = loader();
    QStringList keys = l->keys();
#endif
    QByteArray suffix;

#ifdef QIMAGEREADER_DEBUG
    qDebug() << "QImageReader::createReadHandler( device =" << (void *)device << ", format =" << format << "),"
             << keys.size() << "plugins available: " << keys;
#endif

#ifndef QT_NO_LIBRARY
    int suffixPluginIndex = -1;
    if (device && format.isEmpty() && autoDetectImageFormat && !ignoresFormatAndExtension) {
        // if there's no format, see if \a device is a file, and if so, find
        // the file suffix and find support for that format among our plugins.
        // this allows plugins to override our built-in handlers.
        if (QFile *file = qobject_cast<QFile *>(device)) {
#ifdef QIMAGEREADER_DEBUG
            qDebug() << "QImageReader::createReadHandler: device is a file:" << file->fileName();
#endif
            if (!(suffix = QFileInfo(file->fileName()).suffix().toLower().toLatin1()).isEmpty()) {
                int index = keys.indexOf(QString::fromLatin1(suffix));
                if (index != -1) {
#ifdef QIMAGEREADER_DEBUG
                    qDebug() << "QImageReader::createReadHandler: suffix recognized; the"
                             << suffix << "plugin might be able to read this";
#endif
                    suffixPluginIndex = index;
                }
            }
        }
    }
#endif // QT_NO_LIBRARY

    QByteArray testFormat = !form.isEmpty() ? form : suffix;

    if (ignoresFormatAndExtension)
        testFormat = QByteArray();

#ifndef QT_NO_LIBRARY
    if (suffixPluginIndex != -1) {
        // check if the plugin that claims support for this format can load
        // from this device with this format.
        const qint64 pos = device ? device->pos() : 0;
        QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(QString::fromLatin1(suffix)));
        if (plugin && plugin->capabilities(device, testFormat) & QImageIOPlugin::CanRead) {
            handler = plugin->create(device, testFormat);
#ifdef QIMAGEREADER_DEBUG
            qDebug() << "QImageReader::createReadHandler: using the" << suffix
                     << "plugin";
#endif
        }
        if (device && !device->isSequential())
            device->seek(pos);
    }

    if (!handler && !testFormat.isEmpty() && !ignoresFormatAndExtension) {
        // check if any plugin supports the format (they are not allowed to
        // read from the device yet).
        const qint64 pos = device ? device->pos() : 0;

        if (autoDetectImageFormat) {
            for (int i = 0; i < keys.size(); ++i) {
                if (i != suffixPluginIndex) {
                    QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(keys.at(i)));
                    if (plugin && plugin->capabilities(device, testFormat) & QImageIOPlugin::CanRead) {
#ifdef QIMAGEREADER_DEBUG
                        qDebug() << "QImageReader::createReadHandler: the" << keys.at(i) << "plugin can read this format";
#endif
                        handler = plugin->create(device, testFormat);
                        break;
                    }
                }
            }
        } else {
            QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(QLatin1String(testFormat)));
            if (plugin && plugin->capabilities(device, testFormat) & QImageIOPlugin::CanRead) {
#ifdef QIMAGEREADER_DEBUG
                qDebug() << "QImageReader::createReadHandler: the" << testFormat << "plugin can read this format";
#endif
                handler = plugin->create(device, testFormat);
            }
        }
        if (device && !device->isSequential())
            device->seek(pos);
    }

#endif // QT_NO_LIBRARY

    // if we don't have a handler yet, check if we have built-in support for
    // the format
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

#ifdef QIMAGEREADER_DEBUG
        if (handler)
            qDebug() << "QImageReader::createReadHandler: using the built-in handler for" << testFormat;
#endif
    }

#ifndef QT_NO_LIBRARY
    if (!handler && (autoDetectImageFormat || ignoresFormatAndExtension)) {
        // check if any of our plugins recognize the file from its contents.
        const qint64 pos = device ? device->pos() : 0;
        for (int i = 0; i < keys.size(); ++i) {
            if (i != suffixPluginIndex) {
                QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(keys.at(i)));
                if (plugin && plugin->capabilities(device, QByteArray()) & QImageIOPlugin::CanRead) {
                    handler = plugin->create(device, testFormat);
#ifdef QIMAGEREADER_DEBUG
                    qDebug() << "QImageReader::createReadHandler: the" << keys.at(i) << "plugin can read this data";
#endif
                    break;
                }
            }
        }
        if (device && !device->isSequential())
            device->seek(pos);
    }
#endif // QT_NO_LIBRARY

    if (!handler && (autoDetectImageFormat || ignoresFormatAndExtension)) {
        // check if any of our built-in handlers recognize the file from its
        // contents.
        int currentFormat = 0;
        if (!suffix.isEmpty()) {
            // If reading from a file with a suffix, start testing our
            // built-in handler for that suffix first.
            for (int i = 0; i < _qt_NumFormats; ++i) {
                if (_qt_BuiltInFormats[i].extension == suffix) {
                    currentFormat = i;
                    break;
                }
            }
        }

        QByteArray subType;
        int numFormats = _qt_NumFormats;
        while (device && numFormats >= 0) {
            const _qt_BuiltInFormatStruct *formatStruct = &_qt_BuiltInFormats[currentFormat];

            const qint64 pos = device->pos();
            switch (formatStruct->type) {
#ifndef QT_NO_IMAGEFORMAT_PNG
            case _qt_PngFormat:
                if (QPngHandler::canRead(device))
                    handler = new QPngHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_JPEG
            case _qt_JpgFormat:
                if (QJpegHandler::canRead(device))
                    handler = new QJpegHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_MNG
            case _qt_MngFormat:
                if (QMngHandler::canRead(device))
                    handler = new QMngHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_TIFF
            case _qt_TifFormat:
                if (QTiffHandler::canRead(device))
                    handler = new QTiffHandler;
                break;
#endif
#ifdef QT_BUILTIN_GIF_READER
            case _qt_GifFormat:
                if (QGifHandler::canRead(device))
                    handler = new QGifHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_BMP
            case _qt_BmpFormat:
                if (QBmpHandler::canRead(device))
                    handler = new QBmpHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
            case _qt_XpmFormat:
                if (QXpmHandler::canRead(device))
                    handler = new QXpmHandler;
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_PPM
            case _qt_PbmFormat:
            case _qt_PgmFormat:
            case _qt_PpmFormat:
                if (QPpmHandler::canRead(device, &subType)) {
                    handler = new QPpmHandler;
                    handler->setOption(QImageIOHandler::SubType, subType);
                }
                break;
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
            case _qt_XbmFormat:
                if (QXbmHandler::canRead(device))
                    handler = new QXbmHandler;
                break;
#endif
            default:
                break;
            }
            if (!device->isSequential())
                device->seek(pos);

            if (handler) {
#ifdef QIMAGEREADER_DEBUG
                qDebug() << "QImageReader::createReadHandler: the" << formatStruct->extension
                         << "built-in handler can read this data";
#endif
                break;
            }

            --numFormats;
            ++currentFormat;
            currentFormat %= _qt_NumFormats;
        }
    }

    if (!handler) {
#ifdef QIMAGEREADER_DEBUG
        qDebug() << "QImageReader::createReadHandler: no handlers found. giving up.";
#endif
        // no handler: give up.
        return 0;
    }

    handler->setDevice(device);
    if (!form.isEmpty())
        handler->setFormat(form);
    return handler;
}

class QImageReaderPrivate
{
public:
    QImageReaderPrivate(QImageReader *qq);
    ~QImageReaderPrivate();

    // device
    QByteArray format;
    bool autoDetectImageFormat;
    bool ignoresFormatAndExtension;
    QIODevice *device;
    bool deleteDevice;
    QImageIOHandler *handler;
    bool initHandler();

    // image options
    QRect clipRect;
    QSize scaledSize;
    QRect scaledClipRect;
    int quality;
    QMap<QString, QString> text;
    void getText();

    // error
    QImageReader::ImageReaderError imageReaderError;
    QString errorString;

    QImageReader *q;
};

/*!
    \internal
*/
QImageReaderPrivate::QImageReaderPrivate(QImageReader *qq)
    : autoDetectImageFormat(true), ignoresFormatAndExtension(false)
{
    device = 0;
    deleteDevice = false;
    handler = 0;
    quality = -1;
    imageReaderError = QImageReader::UnknownError;

    q = qq;
}

/*!
    \internal
*/
QImageReaderPrivate::~QImageReaderPrivate()
{
    if (deleteDevice)
        delete device;
    delete handler;
}

/*!
    \internal
*/
bool QImageReaderPrivate::initHandler()
{
    // check some preconditions
    if (!device || (!deleteDevice && !device->isOpen() && !device->open(QIODevice::ReadOnly))) {
        imageReaderError = QImageReader::DeviceError;
        errorString = QLatin1String(QT_TRANSLATE_NOOP(QImageReader, "Invalid device"));
        return false;
    }

    // probe the file extension
    if (deleteDevice && !device->isOpen() && !device->open(QIODevice::ReadOnly) && autoDetectImageFormat) {
        QList<QByteArray> extensions = QImageReader::supportedImageFormats();
        if (!format.isEmpty()) {
            // Try the most probable extension first
            int currentFormatIndex = extensions.indexOf(format.toLower());
            if (currentFormatIndex > 0)
                extensions.swap(0, currentFormatIndex);
        }

        int currentExtension = 0;

        QFile *file = static_cast<QFile *>(device);
        QString fileName = file->fileName();

        do {
            file->setFileName(fileName + QLatin1Char('.')
                    + QString::fromLatin1(extensions.at(currentExtension++).constData()));
            file->open(QIODevice::ReadOnly);
        } while (!file->isOpen() && currentExtension < extensions.size());

        if (!device->isOpen()) {
            imageReaderError = QImageReader::FileNotFoundError;
            errorString = QLatin1String(QT_TRANSLATE_NOOP(QImageReader, "File not found"));
            file->setFileName(fileName); // restore the old file name
            return false;
        }
    }

    // assign a handler
    if (!handler && (handler = createReadHandlerHelper(device, format, autoDetectImageFormat, ignoresFormatAndExtension)) == 0) {
        imageReaderError = QImageReader::UnsupportedFormatError;
        errorString = QLatin1String(QT_TRANSLATE_NOOP(QImageReader, "Unsupported image format"));
        return false;
    }
    return true;
}

/*!
    \internal
*/
void QImageReaderPrivate::getText()
{
    if (!text.isEmpty() || (!handler && !initHandler()) || !handler->supportsOption(QImageIOHandler::Description))
        return;
    foreach (QString pair, handler->option(QImageIOHandler::Description).toString().split(
                QLatin1String("\n\n"))) {
        int index = pair.indexOf(QLatin1Char(':'));
        if (index >= 0 && pair.indexOf(QLatin1Char(' ')) < index) {
            text.insert(QLatin1String("Description"), pair.simplified());
        } else {
            QString key = pair.left(index);
            text.insert(key, pair.mid(index + 2).simplified());
        }
    }
}

/*!
    Constructs an empty QImageReader object. Before reading an image,
    call setDevice() or setFileName().
*/
QImageReader::QImageReader()
    : d(new QImageReaderPrivate(this))
{
}

/*!
    Constructs a QImageReader object with the device \a device and the
    image format \a format.
*/
QImageReader::QImageReader(QIODevice *device, const QByteArray &format)
    : d(new QImageReaderPrivate(this))
{
    d->device = device;
    d->format = format;
}

/*!
    Constructs a QImageReader object with the file name \a fileName
    and the image format \a format.

    \sa setFileName()
*/
QImageReader::QImageReader(const QString &fileName, const QByteArray &format)
    : d(new QImageReaderPrivate(this))
{
    QFile *file = new QFile(fileName);
    d->device = file;
    d->deleteDevice = true;
    d->format = format;
}

/*!
    Destructs the QImageReader object.
*/
QImageReader::~QImageReader()
{
    delete d;
}

/*!
    Sets the format QImageReader will use when reading images, to \a
    format. \a format is a case insensitive text string. Example:

    \snippet doc/src/snippets/code/src_gui_image_qimagereader.cpp 0

    You can call supportedImageFormats() for the full list of formats
    QImageReader supports.

    \sa format()
*/
void QImageReader::setFormat(const QByteArray &format)
{
    d->format = format;
}

/*!
    Returns the format QImageReader uses for reading images.

    You can call this function after assigning a device to the
    reader to determine the format of the device. For example:

    \snippet doc/src/snippets/code/src_gui_image_qimagereader.cpp 1

    If the reader cannot read any image from the device (e.g., there is no
    image there, or the image has already been read), or if the format is
    unsupported, this function returns an empty QByteArray().

    \sa setFormat(), supportedImageFormats()
*/
QByteArray QImageReader::format() const
{
    if (d->format.isEmpty()) {
        if (!d->initHandler())
            return QByteArray();
        return d->handler->canRead() ? d->handler->format() : QByteArray();
    }

    return d->format;
}

/*!
    If \a enabled is true, image format autodetection is enabled; otherwise,
    it is disabled. By default, autodetection is enabled.

    QImageReader uses an extensive approach to detecting the image format;
    firstly, if you pass a file name to QImageReader, it will attempt to
    detect the file extension if the given file name does not point to an
    existing file, by appending supported default extensions to the given file
    name, one at a time. It then uses the following approach to detect the
    image format:

    \list

    \o Image plugins are queried first, based on either the optional format
    string, or the file name suffix (if the source device is a file). No
    content detection is done at this stage. QImageReader will choose the
    first plugin that supports reading for this format.

    \o If no plugin supports the image format, Qt's built-in handlers are
    checked based on either the optional format string, or the file name
    suffix.

    \o If no capable plugins or built-in handlers are found, each plugin is
    tested by inspecting the content of the data stream.

    \o If no plugins could detect the image format based on data contents,
    each built-in image handler is tested by inspecting the contents.

    \o Finally, if all above approaches fail, QImageReader will report failure
    when trying to read the image.

    \endlist

    By disabling image format autodetection, QImageReader will only query the
    plugins and built-in handlers based on the format string (i.e., no file
    name extensions are tested).

    \sa QImageIOHandler::canRead(), QImageIOPlugin::capabilities()
*/
void QImageReader::setAutoDetectImageFormat(bool enabled)
{
    d->autoDetectImageFormat = enabled;
}

/*!
    Returns true if image format autodetection is enabled on this image
    reader; otherwise returns false. By default, autodetection is enabled.

    \sa setAutoDetectImageFormat()
*/
bool QImageReader::autoDetectImageFormat() const
{
    return d->autoDetectImageFormat;
}


/*!
    If \a ignored is set to true, then the image reader will ignore
    specified formats or file extensions and decide which plugin to
    use only based on the contents in the datastream.

    Setting this flag means that all image plugins gets loaded. Each
    plugin will read the first bytes in the image data and decide if
    the plugin is compatible or not.

    This also disables auto detecting the image format.

    \sa decideFormatFromContent()
*/

void QImageReader::setDecideFormatFromContent(bool ignored)
{
    d->ignoresFormatAndExtension = ignored;
}


/*!
    Returns whether the image reader should decide which plugin to use
    only based on the contents of the datastream rather than on the file
    extension.

    \sa setDecideFormatFromContent()
*/

bool QImageReader::decideFormatFromContent() const
{
    return d->ignoresFormatAndExtension;
}


/*!
    Sets QImageReader's device to \a device. If a device has already
    been set, the old device is removed from QImageReader and is
    otherwise left unchanged.

    If the device is not already open, QImageReader will attempt to
    open the device in \l QIODevice::ReadOnly mode by calling
    open(). Note that this does not work for certain devices, such as
    QProcess, QTcpSocket and QUdpSocket, where more logic is required
    to open the device.

    \sa device(), setFileName()
*/
void QImageReader::setDevice(QIODevice *device)
{
    if (d->device && d->deleteDevice)
        delete d->device;
    d->device = device;
    d->deleteDevice = false;
    delete d->handler;
    d->handler = 0;
    d->text.clear();
}

/*!
    Returns the device currently assigned to QImageReader, or 0 if no
    device has been assigned.
*/
QIODevice *QImageReader::device() const
{
    return d->device;
}

/*!
    Sets the file name of QImageReader to \a fileName. Internally,
    QImageReader will create a QFile object and open it in \l
    QIODevice::ReadOnly mode, and use this when reading images.

    If \a fileName does not include a file extension (e.g., .png or .bmp),
    QImageReader will cycle through all supported extensions until it finds
    a matching file.

    \sa fileName(), setDevice(), supportedImageFormats()
*/
void QImageReader::setFileName(const QString &fileName)
{
    setDevice(new QFile(fileName));
    d->deleteDevice = true;
}

/*!
    If the currently assigned device is a QFile, or if setFileName()
    has been called, this function returns the name of the file
    QImageReader reads from. Otherwise (i.e., if no device has been
    assigned or the device is not a QFile), an empty QString is
    returned.

    \sa setFileName(), setDevice()
*/
QString QImageReader::fileName() const
{
    QFile *file = qobject_cast<QFile *>(d->device);
    return file ? file->fileName() : QString();
}

/*!
    \since 4.2

    This is an image format specific function that sets the quality
    level of the image to \a quality. For image formats that do not
    support setting the quality, this value is ignored.

    The value range of \a quality depends on the image format. For
    example, the "jpeg" format supports a quality range from 0 (low
    quality, high compression) to 100 (high quality, low compression).

    \sa quality()
*/
void QImageReader::setQuality(int quality)
{
    d->quality = quality;
}

/*!
    \since 4.2

    Returns the quality level of the image.

    \sa setQuality()
*/
int QImageReader::quality() const
{
    return d->quality;
}


/*!
    Returns the size of the image, without actually reading the image
    contents.

    If the image format does not support this feature, this function returns
    an invalid size. Qt's built-in image handlers all support this feature,
    but custom image format plugins are not required to do so.

    \sa QImageIOHandler::ImageOption, QImageIOHandler::option(), QImageIOHandler::supportsOption()
*/
QSize QImageReader::size() const
{
    if (!d->initHandler())
        return QSize();

    if (d->handler->supportsOption(QImageIOHandler::Size))
        return d->handler->option(QImageIOHandler::Size).toSize();

    return QSize();
}

/*!
    \since 4.5

    Returns the format of the image, without actually reading the image
    contents. The format describes the image format \l QImageReader::read()
    returns, not the format of the actual image.

    If the image format does not support this feature, this function returns
    an invalid format.

    \sa QImageIOHandler::ImageOption, QImageIOHandler::option(), QImageIOHandler::supportsOption()
*/
QImage::Format QImageReader::imageFormat() const
{
    if (!d->initHandler())
        return QImage::Format_Invalid;

    if (d->handler->supportsOption(QImageIOHandler::ImageFormat))
        return (QImage::Format)d->handler->option(QImageIOHandler::ImageFormat).toInt();

    return QImage::Format_Invalid;
}

/*!
    \since 4.1

    Returns the text keys for this image. You can use
    these keys with text() to list the image text for
    a certain key.

    Support for this option is implemented through
    QImageIOHandler::Description.

    \sa text(), QImageWriter::setText(), QImage::textKeys()
*/
QStringList QImageReader::textKeys() const
{
    d->getText();
    return d->text.keys();
}

/*!
    \since 4.1

    Returns the image text associated with \a key.

    Support for this option is implemented through
    QImageIOHandler::Description.

    \sa textKeys(), QImageWriter::setText()
*/
QString QImageReader::text(const QString &key) const
{
    d->getText();
    return d->text.value(key);
}

/*!
    Sets the image clip rect (also known as the ROI, or Region Of
    Interest) to \a rect. The coordinates of \a rect are relative to
    the untransformed image size, as returned by size().

    \sa clipRect(), setScaledSize(), setScaledClipRect()
*/
void QImageReader::setClipRect(const QRect &rect)
{
    d->clipRect = rect;
}

/*!
    Returns the clip rect (also known as the ROI, or Region Of
    Interest) of the image. If no clip rect has been set, an invalid
    QRect is returned.

    \sa setClipRect()
*/
QRect QImageReader::clipRect() const
{
    return d->clipRect;
}

/*!
    Sets the scaled size of the image to \a size. The scaling is
    performed after the initial clip rect, but before the scaled clip
    rect is applied. The algorithm used for scaling depends on the
    image format. By default (i.e., if the image format does not
    support scaling), QImageReader will use QImage::scale() with
    Qt::SmoothScaling.

    \sa scaledSize(), setClipRect(), setScaledClipRect()
*/
void QImageReader::setScaledSize(const QSize &size)
{
    d->scaledSize = size;
}

/*!
    Returns the scaled size of the image.

    \sa setScaledSize()
*/
QSize QImageReader::scaledSize() const
{
    return d->scaledSize;
}

/*!
    Sets the scaled clip rect to \a rect. The scaled clip rect is the
    clip rect (also known as ROI, or Region Of Interest) that is
    applied after the image has been scaled.

    \sa scaledClipRect(), setScaledSize()
*/
void QImageReader::setScaledClipRect(const QRect &rect)
{
    d->scaledClipRect = rect;
}

/*!
    Returns the scaled clip rect of the image.

    \sa setScaledClipRect()
*/
QRect QImageReader::scaledClipRect() const
{
    return d->scaledClipRect;
}

/*!
    \since 4.1

    Sets the background color to \a color.
    Image formats that support this operation are expected to
    initialize the background to \a color before reading an image.

    \sa backgroundColor(), read()
*/
void QImageReader::setBackgroundColor(const QColor &color)
{
    if (!d->initHandler())
        return;
    if (d->handler->supportsOption(QImageIOHandler::BackgroundColor))
        d->handler->setOption(QImageIOHandler::BackgroundColor, color);
}

/*!
    \since 4.1

    Returns the background color that's used when reading an image.
    If the image format does not support setting the background color
    an invalid color is returned.

    \sa setBackgroundColor(), read()
*/
QColor QImageReader::backgroundColor() const
{
    if (!d->initHandler())
        return QColor();
    if (d->handler->supportsOption(QImageIOHandler::BackgroundColor))
        return qvariant_cast<QColor>(d->handler->option(QImageIOHandler::BackgroundColor));
    return QColor();
}

/*!
    \since 4.1

    Returns true if the image format supports animation;
    otherwise, false is returned.

    \sa QMovie::supportedFormats()
*/
bool QImageReader::supportsAnimation() const
{
    if (!d->initHandler())
        return false;
    if (d->handler->supportsOption(QImageIOHandler::Animation))
        return d->handler->option(QImageIOHandler::Animation).toBool();
    return false;
}

/*!
    Returns true if an image can be read for the device (i.e., the
    image format is supported, and the device seems to contain valid
    data); otherwise returns false.

    canRead() is a lightweight function that only does a quick test to
    see if the image data is valid. read() may still return false
    after canRead() returns true, if the image data is corrupt.

    For images that support animation, canRead() returns false when
    all frames have been read.

    \sa read(), supportedImageFormats()
*/
bool QImageReader::canRead() const
{
    if (!d->initHandler())
        return false;

    return d->handler->canRead();
}

/*!
    Reads an image from the device. On success, the image that was
    read is returned; otherwise, a null QImage is returned. You can
    then call error() to find the type of error that occurred, or
    errorString() to get a human readable description of the error.

    For image formats that support animation, calling read()
    repeatedly will return the next frame. When all frames have been
    read, a null image will be returned.

    \sa canRead(), supportedImageFormats(), supportsAnimation(), QMovie
*/
QImage QImageReader::read()
{
    // Because failed image reading might have side effects, we explicitly
    // return a null image instead of the image we've just created.
    QImage image;
    return read(&image) ? image : QImage();
}

/*!
    \overload

    Reads an image from the device into \a image, which must point to a
    QImage. Returns true on success; otherwise, returns false.

    If \a image has same format and size as the image data that is about to be
    read, this function may not need to allocate a new image before
    reading. Because of this, it can be faster than the other read() overload,
    which always constructs a new image; especially when reading several
    images with the same format and size.

    \snippet doc/src/snippets/code/src_gui_image_qimagereader.cpp 2

    For image formats that support animation, calling read() repeatedly will
    return the next frame. When all frames have been read, a null image will
    be returned.

    \sa canRead(), supportedImageFormats(), supportsAnimation(), QMovie
*/
bool QImageReader::read(QImage *image)
{
    if (!image) {
        qWarning("QImageReader::read: cannot read into null pointer");
        return false;
    }

    if (!d->handler && !d->initHandler())
        return false;

    // set the handler specific options.
    if (d->handler->supportsOption(QImageIOHandler::ScaledSize) && d->scaledSize.isValid()) {
        if ((d->handler->supportsOption(QImageIOHandler::ClipRect) && !d->clipRect.isNull())
            || d->clipRect.isNull()) {
            // Only enable the ScaledSize option if there is no clip rect, or
            // if the handler also supports ClipRect.
            d->handler->setOption(QImageIOHandler::ScaledSize, d->scaledSize);
        }
    }
    if (d->handler->supportsOption(QImageIOHandler::ClipRect) && !d->clipRect.isNull())
        d->handler->setOption(QImageIOHandler::ClipRect, d->clipRect);
    if (d->handler->supportsOption(QImageIOHandler::ScaledClipRect) && !d->scaledClipRect.isNull())
        d->handler->setOption(QImageIOHandler::ScaledClipRect, d->scaledClipRect);
    if (d->handler->supportsOption(QImageIOHandler::Quality))
        d->handler->setOption(QImageIOHandler::Quality, d->quality);

    // read the image
    if (!d->handler->read(image)) {
        d->imageReaderError = InvalidDataError;
        d->errorString = QLatin1String(QT_TRANSLATE_NOOP(QImageReader, "Unable to read image data"));
        return false;
    }

    // provide default implementations for any unsupported image
    // options
    if (d->handler->supportsOption(QImageIOHandler::ClipRect) && !d->clipRect.isNull()) {
        if (d->handler->supportsOption(QImageIOHandler::ScaledSize) && d->scaledSize.isValid()) {
            if (d->handler->supportsOption(QImageIOHandler::ScaledClipRect) && !d->scaledClipRect.isNull()) {
                // all features are supported by the handler; nothing to do.
            } else {
                // the image is already scaled, so apply scaled clipping.
                if (!d->scaledClipRect.isNull())
                    *image = image->copy(d->scaledClipRect);
            }
        } else {
            if (d->handler->supportsOption(QImageIOHandler::ScaledClipRect) && !d->scaledClipRect.isNull()) {
                // supports scaled clipping but not scaling, most
                // likely a broken handler.
            } else {
                if (d->scaledSize.isValid()) {
                    *image = image->scaled(d->scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                if (d->scaledClipRect.isValid()) {
                    *image = image->copy(d->scaledClipRect);
                }
            }
        }
    } else {
        if (d->handler->supportsOption(QImageIOHandler::ScaledSize) && d->scaledSize.isValid()) {
            // in this case, there's nothing we can do. if the
            // plugin supports scaled size but not ClipRect, then
            // we have to ignore ClipRect."

            if (d->handler->supportsOption(QImageIOHandler::ScaledClipRect) && !d->scaledClipRect.isNull()) {
                // nothing to do (ClipRect is ignored!)
            } else {
                // provide all workarounds.
                if (d->scaledClipRect.isValid()) {
                    *image = image->copy(d->scaledClipRect);
                }
            }
        } else {
            if (d->handler->supportsOption(QImageIOHandler::ScaledClipRect) && !d->scaledClipRect.isNull()) {
                // this makes no sense; a handler that supports
                // ScaledClipRect but not ScaledSize is broken, and we
                // can't work around it.
            } else {
                // provide all workarounds.
                if (d->clipRect.isValid())
                    *image = image->copy(d->clipRect);
                if (d->scaledSize.isValid())
                    *image = image->scaled(d->scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                if (d->scaledClipRect.isValid())
                    *image = image->copy(d->scaledClipRect);
            }
        }
    }

    return true;
}

/*!
   For image formats that support animation, this function steps over the
   current image, returning true if successful or false if there is no
   following image in the animation.

   The default implementation calls read(), then discards the resulting
   image, but the image handler may have a more efficient way of implementing
   this operation.

   \sa jumpToImage(), QImageIOHandler::jumpToNextImage()
*/
bool QImageReader::jumpToNextImage()
{
    if (!d->initHandler())
        return false;
    return d->handler->jumpToNextImage();
}

/*!
   For image formats that support animation, this function skips to the image
   whose sequence number is \a imageNumber, returning true if successful
   or false if the corresponding image cannot be found.

   The next call to read() will attempt to read this image.

   \sa jumpToNextImage(), QImageIOHandler::jumpToImage()
*/
bool QImageReader::jumpToImage(int imageNumber)
{
    if (!d->initHandler())
        return false;
    return d->handler->jumpToImage(imageNumber);
}

/*!
    For image formats that support animation, this function returns the number
    of times the animation should loop. If this function returns -1, it can
    either mean the animation should loop forever, or that an error occurred.
    If an error occurred, canRead() will return false.

    \sa supportsAnimation(), QImageIOHandler::loopCount(), canRead()
*/
int QImageReader::loopCount() const
{
    if (!d->initHandler())
        return -1;
    return d->handler->loopCount();
}

/*!
    For image formats that support animation, this function returns the total
    number of images in the animation. If the format does not support
    animation, 0 is returned.

    This function returns -1 if an error occurred.

    \sa supportsAnimation(), QImageIOHandler::imageCount(), canRead()
*/
int QImageReader::imageCount() const
{
    if (!d->initHandler())
        return -1;
    return d->handler->imageCount();
}

/*!
    For image formats that support animation, this function returns the number
    of milliseconds to wait until displaying the next frame in the animation.
    If the image format doesn't support animation, 0 is returned.

    This function returns -1 if an error occurred.

    \sa supportsAnimation(), QImageIOHandler::nextImageDelay(), canRead()
*/
int QImageReader::nextImageDelay() const
{
    if (!d->initHandler())
        return -1;
    return d->handler->nextImageDelay();
}

/*!
    For image formats that support animation, this function returns the
    sequence number of the current frame. If the image format doesn't support
    animation, 0 is returned.

    This function returns -1 if an error occurred.

    \sa supportsAnimation(), QImageIOHandler::currentImageNumber(), canRead()
*/
int QImageReader::currentImageNumber() const
{
    if (!d->initHandler())
        return -1;
    return d->handler->currentImageNumber();
}

/*!
    For image formats that support animation, this function returns
    the rect for the current frame. Otherwise, a null rect is returned.

    \sa supportsAnimation(), QImageIOHandler::currentImageRect()
*/
QRect QImageReader::currentImageRect() const
{
    if (!d->initHandler())
        return QRect();
    return d->handler->currentImageRect();
}

/*!
    Returns the type of error that occurred last.

    \sa ImageReaderError, errorString()
*/
QImageReader::ImageReaderError QImageReader::error() const
{
    return d->imageReaderError;
}

/*!
    Returns a human readable description of the last error that
    occurred.

    \sa error()
*/
QString QImageReader::errorString() const
{
    if (d->errorString.isEmpty())
        return QLatin1String(QT_TRANSLATE_NOOP(QImageReader, "Unknown error"));
    return d->errorString;
}

/*!
    \since 4.2

    Returns true if the reader supports \a option; otherwise returns
    false.

    Different image formats support different options. Call this function to
    determine whether a certain option is supported by the current format. For
    example, the PNG format allows you to embed text into the image's metadata
    (see text()), and the BMP format allows you to determine the image's size
    without loading the whole image into memory (see size()).

    \snippet doc/src/snippets/code/src_gui_image_qimagereader.cpp 3

    \sa QImageWriter::supportsOption()
*/
bool QImageReader::supportsOption(QImageIOHandler::ImageOption option) const
{
    if (!d->initHandler())
        return false;
    return d->handler->supportsOption(option);
}

/*!
    If supported, this function returns the image format of the file
    \a fileName. Otherwise, an empty string is returned.
*/
QByteArray QImageReader::imageFormat(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return QByteArray();

    return imageFormat(&file);
}

/*!
    If supported, this function returns the image format of the device
    \a device. Otherwise, an empty string is returned.

    \sa QImageReader::autoDetectImageFormat()
*/
QByteArray QImageReader::imageFormat(QIODevice *device)
{
    QByteArray format;
    QImageIOHandler *handler = createReadHandlerHelper(device, format, /* autoDetectImageFormat = */ true, false);
    if (handler) {
        if (handler->canRead())
            format = handler->format();
        delete handler;
    }
    return format;
}

/*!
    Returns the list of image formats supported by QImageReader.

    By default, Qt can read the following formats:

    \table
    \header \o Format \o Description
    \row    \o BMP    \o Windows Bitmap
    \row    \o GIF    \o Graphic Interchange Format (optional)
    \row    \o JPG    \o Joint Photographic Experts Group
    \row    \o JPEG   \o Joint Photographic Experts Group
    \row    \o MNG    \o Multiple-image Network Graphics
    \row    \o PNG    \o Portable Network Graphics
    \row    \o PBM    \o Portable Bitmap
    \row    \o PGM    \o Portable Graymap
    \row    \o PPM    \o Portable Pixmap
    \row    \o TIFF   \o Tagged Image File Format
    \row    \o XBM    \o X11 Bitmap
    \row    \o XPM    \o X11 Pixmap
    \row    \o SVG    \o Scalable Vector Graphics
    \row    \o TGA    \o Targa Image Format
    \endtable

    Reading and writing SVG files is supported through Qt's
    \l{QtSvg Module}{SVG Module}.

    TGA support only extends to reading non-RLE compressed files.  In particular
    calls to \l{http://doc.qt.nokia.com/4.7-snapshot/qimageioplugin.html#capabilities}{capabilities}
    for the tga plugin returns only QImageIOPlugin::CanRead, not QImageIOPlugin::CanWrite.

    To configure Qt with GIF support, pass \c -qt-gif to the \c
    configure script or check the appropriate option in the graphical
    installer.

    Note that the QApplication instance must be created before this function is
    called.

    \sa setFormat(), QImageWriter::supportedImageFormats(), QImageIOPlugin
*/
QList<QByteArray> QImageReader::supportedImageFormats()
{
    QSet<QByteArray> formats;
    for (int i = 0; i < _qt_NumFormats; ++i)
        formats << _qt_BuiltInFormats[i].extension;

#ifndef QT_NO_LIBRARY
    QFactoryLoader *l = loader();
    QStringList keys = l->keys();

    for (int i = 0; i < keys.count(); ++i) {
        QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(keys.at(i)));
        if (plugin && plugin->capabilities(0, keys.at(i).toLatin1()) & QImageIOPlugin::CanRead)
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
