/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qmimedata.h"

#include "private/qobject_p.h"
#include "qurl.h"
#include "qstringlist.h"
#include "qtextcodec.h"

QT_BEGIN_NAMESPACE

struct QMimeDataStruct
{
    QString format;
    QVariant data;
};

class QMimeDataPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QMimeData)
public:
    void removeData(const QString &format);
    void setData(const QString &format, const QVariant &data);
    QVariant getData(const QString &format) const;

    QVariant retrieveTypedData(const QString &format, QVariant::Type type) const;

    QList<QMimeDataStruct> dataList;
};

void QMimeDataPrivate::removeData(const QString &format)
{
    for (int i=0; i<dataList.size(); i++) {
        if (dataList.at(i).format == format) {
            dataList.removeAt(i);
            return;
        }
    }
}

void QMimeDataPrivate::setData(const QString &format, const QVariant &data)
{
    // remove it first if the format is already here.
    removeData(format);
    QMimeDataStruct mimeData;
    mimeData.format = format;
    mimeData.data = data;
    dataList += mimeData;
}


QVariant QMimeDataPrivate::getData(const QString &format) const
{
    QVariant data;
    for (int i=0; i<dataList.size(); i++) {
        if (dataList.at(i).format == format) {
            data = dataList.at(i).data;
            break;
        }
    }
    return data;
}

QVariant QMimeDataPrivate::retrieveTypedData(const QString &format, QVariant::Type type) const
{
    Q_Q(const QMimeData);

    QVariant data = q->retrieveData(format, type);
    if (data.type() == type || !data.isValid())
        return data;

    // provide more conversion possiblities than just what QVariant provides

    // URLs can be lists as well...
    if ((type == QVariant::Url && data.type() == QVariant::List)
        || (type == QVariant::List && data.type() == QVariant::Url))
        return data;

    // images and pixmaps are interchangeable
    if ((type == QVariant::Pixmap && data.type() == QVariant::Image)
        || (type == QVariant::Image && data.type() == QVariant::Pixmap))
        return data;

    if (data.type() == QVariant::ByteArray) {
        // see if we can convert to the requested type
        switch(type) {
#ifndef QT_NO_TEXTCODEC
        case QVariant::String: {
            const QByteArray ba = data.toByteArray();
            QTextCodec *codec = QTextCodec::codecForName("utf-8");
            if (format == QLatin1String("text/html"))
                codec = QTextCodec::codecForHtml(ba, codec);
            return codec->toUnicode(ba);
        }
#endif // QT_NO_TEXTCODEC
        case QVariant::Color: {
            QVariant newData = data;
            newData.convert(QVariant::Color);
            return newData;
        }
        case QVariant::List: {
            if (format != QLatin1String("text/uri-list"))
                break;
            // fall through
        }
        case QVariant::Url: {
            QByteArray ba = data.toByteArray();
            // Qt 3.x will send text/uri-list with a trailing
            // null-terminator (that is *not* sent for any other
            // text/* mime-type), so chop it off
            if (ba.endsWith('\0'))
                ba.chop(1);

            QList<QByteArray> urls = ba.split('\n');
            QList<QVariant> list;
            for (int i = 0; i < urls.size(); ++i) {
                QByteArray ba = urls.at(i).trimmed();
                if (!ba.isEmpty())
                    list.append(QUrl::fromEncoded(ba));
            }
            return list;
        }
        default:
            break;
        }

    } else if (type == QVariant::ByteArray) {

        // try to convert to bytearray
        switch(data.type()) {
        case QVariant::ByteArray:
        case QVariant::Color:
            return data.toByteArray();
            break;
        case QVariant::String:
            return data.toString().toUtf8();
            break;
        case QVariant::Url:
            return data.toUrl().toEncoded();
            break;
        case QVariant::List: {
            // has to be list of URLs
            QByteArray result;
            QList<QVariant> list = data.toList();
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i).type() == QVariant::Url) {
                    result += list.at(i).toUrl().toEncoded();
                    result += "\r\n";
                }
            }
            if (!result.isEmpty())
                return result;
            break;
        }
        default:
            break;
        }
    }
    return data;
}

/*!
    \class QMimeData
    \brief The QMimeData class provides a container for data that records information
    about its MIME type.

    QMimeData is used to describe information that can be stored in
    the \l{QClipboard}{clipboard}, and transferred via the \l{drag
    and drop} mechanism. QMimeData objects associate the data that
    they hold with the corresponding MIME types to ensure that
    information can be safely transferred between applications, and
    copied around within the same application.

    QMimeData objects are usually created using \c new and supplied
    to QDrag or QClipboard objects. This is to enable Qt to manage
    the memory that they use.

    A single QMimeData object can store the same data using several
    different formats at the same time. The formats() function
    returns a list of the available formats in order of preference.
    The data() function returns the raw data associated with a MIME
    type, and setData() allows you to set the data for a MIME type.

    For the most common MIME types, QMimeData provides convenience
    functions to access the data:

    \table
    \header \o Tester       \o Getter       \o Setter           \o MIME Types
    \row    \o hasText()    \o text()       \o setText()        \o \c text/plain
    \row    \o hasHtml()    \o html()       \o setHtml()        \o \c text/html
    \row    \o hasUrls()    \o urls()       \o setUrls()        \o \c text/uri-list
    \row    \o hasImage()   \o imageData()  \o setImageData()   \o \c image/ *
    \row    \o hasColor()   \o colorData()  \o setColorData()   \o \c application/x-color
    \endtable

    For example, if your write a widget that accepts URL drags, you
    would end up writing code like this:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 0

    There are three approaches for storing custom data in a QMimeData
    object:

    \list 1
    \o  Custom data can be stored directly in a QMimeData object as a
        QByteArray using setData(). For example:

        \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 1

    \o  We can subclass QMimeData and reimplement hasFormat(),
        formats(), and retrieveData().

    \o  If the drag and drop operation occurs within a single
        application, we can subclass QMimeData and add extra data in
        it, and use a qobject_cast() in the receiver's drop event
        handler. For example:

        \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 2
    \endlist

    \section1 Platform-Specific MIME Types

    On Windows, formats() will also return custom formats available
    in the MIME data, using the \c{x-qt-windows-mime} subtype to
    indicate that they represent data in non-standard formats.
    The formats will take the following form:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 3

    The following are examples of custom MIME types:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 4

    The \c value declaration of each format describes the way in which the
    data is encoded.

    On Windows, the MIME format does not always map directly to the
    clipboard formats. Qt provides QWindowsMime to map clipboard
    formats to open-standard MIME formats. Similarly, the
    QMacPasteboardMime maps MIME to Mac flavors.

    \sa QClipboard, QDragEnterEvent, QDragMoveEvent, QDropEvent, QDrag,
        QWindowsMime, QMacPasteboardMime, {Drag and Drop}
*/

/*!
    Constructs a new MIME data object with no data in it.
*/
QMimeData::QMimeData()
    : QObject(*new QMimeDataPrivate, 0)
{
}

/*!
    Destroys the MIME data object.
*/
QMimeData::~QMimeData()
{
}

/*!
    Returns a list of URLs contained within the MIME data object.

    URLs correspond to the MIME type \c text/uri-list.

    \sa hasUrls(), data()
*/
QList<QUrl> QMimeData::urls() const
{
    Q_D(const QMimeData);
    QVariant data = d->retrieveTypedData(QLatin1String("text/uri-list"), QVariant::List);
    QList<QUrl> urls;
    if (data.type() == QVariant::Url)
        urls.append(data.toUrl());
    else if (data.type() == QVariant::List) {
        QList<QVariant> list = data.toList();
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).type() == QVariant::Url)
                urls.append(list.at(i).toUrl());
        }
    }
    return urls;
}

/*!
    Sets the URLs stored in the MIME data object to those specified by \a urls.

    URLs correspond to the MIME type \c text/uri-list.

    \sa hasUrls(), setData()
*/
void QMimeData::setUrls(const QList<QUrl> &urls)
{
    Q_D(QMimeData);
    QList<QVariant> list;
    for (int i = 0; i < urls.size(); ++i)
        list.append(urls.at(i));

    d->setData(QLatin1String("text/uri-list"), list);
}

/*!
    Returns true if the object can return a list of urls; otherwise
    returns false.

    URLs correspond to the MIME type \c text/uri-list.

    \sa setUrls(), urls(), hasFormat()
*/
bool QMimeData::hasUrls() const
{
    return hasFormat(QLatin1String("text/uri-list"));
}


/*!
    Returns a plain text (MIME type \c text/plain) representation of
    the data.

    \sa hasText(), html(), data()
*/
QString QMimeData::text() const
{
    Q_D(const QMimeData);
    QVariant data = d->retrieveTypedData(QLatin1String("text/plain"), QVariant::String);
    return data.toString();
}

/*!
    Sets \a text as the plain text (MIME type \c text/plain) used to
    represent the data.

    \sa hasText(), setHtml(), setData()
*/
void QMimeData::setText(const QString &text)
{
    Q_D(QMimeData);
    d->setData(QLatin1String("text/plain"), text);
}

/*!
    Returns true if the object can return plain text (MIME type \c
    text/plain); otherwise returns false.

    \sa setText(), text(), hasHtml(), hasFormat()
*/
bool QMimeData::hasText() const
{
    return hasFormat(QLatin1String("text/plain"));
}

/*!
    Returns a string if the data stored in the object is HTML (MIME
    type \c text/html); otherwise returns an empty string.

    \sa hasHtml(), setData()
*/
QString QMimeData::html() const
{
    Q_D(const QMimeData);
    QVariant data = d->retrieveTypedData(QLatin1String("text/html"), QVariant::String);
    return data.toString();
}

/*!
    Sets \a html as the HTML (MIME type \c text/html) used to
    represent the data.

    \sa hasHtml(), setText(), setData()
*/
void QMimeData::setHtml(const QString &html)
{
    Q_D(QMimeData);
    d->setData(QLatin1String("text/html"), html);
}

/*!
    Returns true if the object can return HTML (MIME type \c
    text/html); otherwise returns false.

    \sa setHtml(), html(), hasFormat()
*/
bool QMimeData::hasHtml() const
{
    return hasFormat(QLatin1String("text/html"));
}

/*!
    Returns a QVariant storing a QImage if the object can return an
    image; otherwise returns a null variant.

    A QVariant is used because QMimeData belongs to the \l QtCore
    library, whereas QImage belongs to \l QtGui. To convert the
    QVariant to a QImage, simply use qvariant_cast(). For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 5

    \sa hasImage()
*/
QVariant QMimeData::imageData() const
{
    Q_D(const QMimeData);
    return d->retrieveTypedData(QLatin1String("application/x-qt-image"), QVariant::Image);
}

/*!
    Sets the data in the object to the given \a image.

    A QVariant is used because QMimeData belongs to the \l QtCore
    library, whereas QImage belongs to \l QtGui. The conversion
    from QImage to QVariant is implicit. For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 6

    \sa hasImage(), setData()
*/
void QMimeData::setImageData(const QVariant &image)
{
    Q_D(QMimeData);
    d->setData(QLatin1String("application/x-qt-image"), image);
}

/*!
    Returns true if the object can return an image; otherwise returns
    false.

    \sa setImageData(), imageData(), hasFormat()
*/
bool QMimeData::hasImage() const
{
    return hasFormat(QLatin1String("application/x-qt-image"));
}

/*!
    Returns a color if the data stored in the object represents a
    color (MIME type \c application/x-color); otherwise returns a
    null variant.

    A QVariant is used because QMimeData belongs to the \l QtCore
    library, whereas QColor belongs to \l QtGui. To convert the
    QVariant to a QColor, simply use qvariant_cast(). For example:

    \snippet doc/src/snippets/code/src_corelib_kernel_qmimedata.cpp 7

    \sa hasColor(), setColorData(), data()
*/
QVariant QMimeData::colorData() const
{
    Q_D(const QMimeData);
    return d->retrieveTypedData(QLatin1String("application/x-color"), QVariant::Color);
}

/*!
    Sets the color data in the object to the given \a color.

    Colors correspond to the MIME type \c application/x-color.

    \sa hasColor(), setData()
*/
void QMimeData::setColorData(const QVariant &color)
{
    Q_D(QMimeData);
    d->setData(QLatin1String("application/x-color"), color);
}


/*!
    Returns true if the object can return a color (MIME type \c
    application/x-color); otherwise returns false.

    \sa setColorData(), colorData(), hasFormat()
*/
bool QMimeData::hasColor() const
{
    return hasFormat(QLatin1String("application/x-color"));
}

/*!
    Returns the data stored in the object in the format described by
    the MIME type specified by \a mimeType.
*/
QByteArray QMimeData::data(const QString &mimeType) const
{
    Q_D(const QMimeData);
    QVariant data = d->retrieveTypedData(mimeType, QVariant::ByteArray);
    return data.toByteArray();
}

/*!
    Sets the data associated with the MIME type given by \a mimeType
    to the specified \a data.

    For the most common types of data, you can call the higher-level
    functions setText(), setHtml(), setUrls(), setImageData(), and
    setColorData() instead.

    Note that if you want to use a custom data type in an item view drag and drop
    operation, you must register it as a Qt \l{QMetaType}{meta type}, using the
    Q_DECLARE_METATYPE() macro, and implement stream operators for it. The stream
    operators must then be registered with the qRegisterMetaTypeStreamOperators()
    function.

    \sa hasFormat(), QMetaType, qRegisterMetaTypeStreamOperators()
*/
void QMimeData::setData(const QString &mimeType, const QByteArray &data)
{
    Q_D(QMimeData);
    d->setData(mimeType, QVariant(data));
}

/*!
    Returns true if the object can return data for the MIME type
    specified by \a mimeType; otherwise returns false.

    For the most common types of data, you can call the higher-level
    functions hasText(), hasHtml(), hasUrls(), hasImage(), and
    hasColor() instead.

    \sa formats(), setData(), data()
*/
bool QMimeData::hasFormat(const QString &mimeType) const
{
    return formats().contains(mimeType);
}

/*!
    Returns a list of formats supported by the object. This is a list
    of MIME types for which the object can return suitable data. The
    formats in the list are in a priority order.

    For the most common types of data, you can call the higher-level
    functions hasText(), hasHtml(), hasUrls(), hasImage(), and
    hasColor() instead.

    \sa hasFormat(), setData(), data()
*/
QStringList QMimeData::formats() const
{
    Q_D(const QMimeData);
    QStringList list;
    for (int i=0; i<d->dataList.size(); i++)
        list += d->dataList.at(i).format;
    return list;
}

/*!
    Returns a variant with the given \a type containing data for the
    MIME type specified by \a mimeType. If the object does not
    support the MIME type or variant type given, a null variant is
    returned instead.

    This function is called by the general data() getter and by the
    convenience getters (text(), html(), urls(), imageData(), and
    colorData()). You can reimplement it if you want to store your
    data using a custom data structure (instead of a QByteArray,
    which is what setData() provides). You would then also need
    to reimplement hasFormat() and formats().

    \sa data()
*/
QVariant QMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    Q_UNUSED(type);
    Q_D(const QMimeData);
    return d->getData(mimeType);
}

/*!
    Removes all the MIME type and data entries in the object.
*/
void QMimeData::clear()
{
    Q_D(QMimeData);
    d->dataList.clear();
}

/*!
    \since 4.4

    Removes the data entry for \a mimeType in the object.
*/
void QMimeData::removeFormat(const QString &mimeType)
{
    Q_D(QMimeData);
    d->removeData(mimeType);
}

QT_END_NAMESPACE
