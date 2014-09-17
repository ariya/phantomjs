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

#include "qclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include "qapplication.h"
#include "qapplication_p.h"
#include "qpixmap.h"
#include "qclipboard_p.h"
#include "qvariant.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qtextcodec.h"

QT_BEGIN_NAMESPACE

/*!
    \class QClipboard
    \brief The QClipboard class provides access to the window system clipboard.

    The clipboard offers a simple mechanism to copy and paste data
    between applications.

    QClipboard supports the same data types that QDrag does, and uses
    similar mechanisms. For advanced clipboard usage read \l{Drag and
    Drop}.

    There is a single QClipboard object in an application, accessible
    as QApplication::clipboard().

    Example:
    \snippet doc/src/snippets/code/src_gui_kernel_qclipboard.cpp 0

    QClipboard features some convenience functions to access common
    data types: setText() allows the exchange of Unicode text and
    setPixmap() and setImage() allows the exchange of QPixmaps and
    QImages between applications. The setMimeData() function is the
    ultimate in flexibility: it allows you to add any QMimeData into
    the clipboard. There are corresponding getters for each of these,
    e.g. text(), image() and pixmap(). You can clear the clipboard by
    calling clear().

    A typical example of the use of these functions follows:

    \snippet doc/src/snippets/droparea.cpp 0

    \section1 Notes for X11 Users

    \list

    \i The X11 Window System has the concept of a separate selection
    and clipboard.  When text is selected, it is immediately available
    as the global mouse selection.  The global mouse selection may
    later be copied to the clipboard.  By convention, the middle mouse
    button is used to paste the global mouse selection.

    \i X11 also has the concept of ownership; if you change the
    selection within a window, X11 will only notify the owner and the
    previous owner of the change, i.e. it will not notify all
    applications that the selection or clipboard data changed.

    \i Lastly, the X11 clipboard is event driven, i.e. the clipboard
    will not function properly if the event loop is not running.
    Similarly, it is recommended that the contents of the clipboard
    are stored or retrieved in direct response to user-input events,
    e.g. mouse button or key presses and releases.  You should not
    store or retrieve the clipboard contents in response to timer or
    non-user-input events.

    \i Since there is no standard way to copy and paste files between
    applications on X11, various MIME types and conventions are currently
    in use. For instance, Nautilus expects files to be supplied with a
    \c{x-special/gnome-copied-files} MIME type with data beginning with
    the cut/copy action, a newline character, and the URL of the file.

    \endlist

    \section1 Notes for Mac OS X Users

    Mac OS X supports a separate find buffer that holds the current
    search string in Find operations. This find clipboard can be accessed
    by specifying the FindBuffer mode.

    \section1 Notes for Windows and Mac OS X Users

    \list

    \i Windows and Mac OS X do not support the global mouse
    selection; they only supports the global clipboard, i.e. they
    only add text to the clipboard when an explicit copy or cut is
    made.

    \i Windows and Mac OS X does not have the concept of ownership;
    the clipboard is a fully global resource so all applications are
    notified of changes.

    \endlist

    \sa QApplication
*/

#ifndef Q_WS_X11
// for X11 there is a separate implementation of a constructor.
/*!
    \internal

    Constructs a clipboard object.

    Do not call this function.

    Call QApplication::clipboard() instead to get a pointer to the
    application's global clipboard object.

    There is only one clipboard in the window system, and creating
    more than one object to represent it is almost certainly an error.
*/

QClipboard::QClipboard(QObject *parent)
    : QObject(*new QClipboardPrivate, parent)
{
    // nothing
}
#endif

#ifndef Q_WS_WIN32
/*!
    \internal

    Destroys the clipboard.

    You should never delete the clipboard. QApplication will do this
    when the application terminates.
*/
QClipboard::~QClipboard()
{
}
#endif

/*!
    \fn void QClipboard::changed(QClipboard::Mode mode)
    \since 4.2

    This signal is emitted when the data for the given clipboard \a
    mode is changed.

    \sa dataChanged(), selectionChanged(), findBufferChanged()
*/

/*!
    \fn void QClipboard::dataChanged()

    This signal is emitted when the clipboard data is changed.

    On Mac OS X and with Qt version 4.3 or higher, clipboard
    changes made by other applications will only be detected
    when the application is activated.

    \sa findBufferChanged(), selectionChanged(), changed()
*/

/*!
    \fn void QClipboard::selectionChanged()

    This signal is emitted when the selection is changed. This only
    applies to windowing systems that support selections, e.g. X11.
    Windows and Mac OS X don't support selections.

    \sa dataChanged(), findBufferChanged(), changed()
*/

/*!
    \fn void QClipboard::findBufferChanged()
    \since 4.2

    This signal is emitted when the find buffer is changed. This only
    applies to Mac OS X.

    With Qt version 4.3 or higher, clipboard changes made by other
    applications will only be detected when the application is activated.

    \sa dataChanged(), selectionChanged(), changed()
*/


/*! \enum QClipboard::Mode
    \keyword clipboard mode

    This enum type is used to control which part of the system clipboard is
    used by QClipboard::mimeData(), QClipboard::setMimeData() and related functions.

    \value Clipboard  indicates that data should be stored and retrieved from
    the global clipboard.

    \value Selection  indicates that data should be stored and retrieved from
    the global mouse selection. Support for \c Selection is provided only on 
    systems with a global mouse selection (e.g. X11).

    \value FindBuffer indicates that data should be stored and retrieved from
    the Find buffer. This mode is used for holding search strings on Mac OS X.

    \omitvalue LastMode

    \sa QClipboard::supportsSelection()
*/


/*****************************************************************************
  QApplication member functions related to QClipboard.
 *****************************************************************************/

// text handling is done directly in qclipboard_qws, for now

/*!
    \fn bool QClipboard::event(QEvent *e)
    \reimp
*/

/*!
    \overload

    Returns the clipboard text in subtype \a subtype, or an empty string
    if the clipboard does not contain any text. If \a subtype is null,
    any subtype is acceptable, and \a subtype is set to the chosen
    subtype.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the text is retrieved from the global
    mouse selection.

    Common values for \a subtype are "plain" and "html".

    Note that calling this function repeatedly, for instance from a
    key event handler, may be slow. In such cases, you should use the
    \c dataChanged() signal instead.

    \sa setText(), mimeData()
*/
QString QClipboard::text(QString &subtype, Mode mode) const
{
    const QMimeData *const data = mimeData(mode);
    if (!data)
        return QString();

    const QStringList formats = data->formats();
    if (subtype.isEmpty()) {
        if (formats.contains(QLatin1String("text/plain")))
            subtype = QLatin1String("plain");
        else {
            for (int i = 0; i < formats.size(); ++i)
                if (formats.at(i).startsWith(QLatin1String("text/"))) {
                    subtype = formats.at(i).mid(5);
                    break;
                }
            if (subtype.isEmpty())
                return QString();
        }
    } else if (!formats.contains(QLatin1String("text/") + subtype)) {
        return QString();
    }

    const QByteArray rawData = data->data(QLatin1String("text/") + subtype);

#ifndef QT_NO_TEXTCODEC
    QTextCodec* codec = QTextCodec::codecForMib(106); // utf-8 is default
    if (subtype == QLatin1String("html"))
        codec = QTextCodec::codecForHtml(rawData, codec);
    else
        codec = QTextCodec::codecForUtfText(rawData, codec);
    return codec->toUnicode(rawData);
#else //QT_NO_TEXTCODEC
    return rawData;
#endif //QT_NO_TEXTCODEC
}

/*!
    Returns the clipboard text as plain text, or an empty string if the
    clipboard does not contain any text.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the text is retrieved from the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    text is retrieved from the search string buffer.

    \sa setText(), mimeData()
*/
QString QClipboard::text(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    return data ? data->text() : QString();
}

/*!
    Copies \a text into the clipboard as plain text.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the text is stored in the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    text is stored in the search string buffer.

    \sa text(), setMimeData()
*/
void QClipboard::setText(const QString &text, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setText(text);
    setMimeData(data, mode);
}

/*!
    Returns the clipboard image, or returns a null image if the
    clipboard does not contain an image or if it contains an image in
    an unsupported image format.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    image is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the image is retrieved from the global
    mouse selection. 

    \sa setImage() pixmap() mimeData(), QImage::isNull()
*/
QImage QClipboard::image(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    if (!data)
        return QImage();
    return qvariant_cast<QImage>(data->imageData());
}

/*!
    Copies the \a image into the clipboard.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    image is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the data is stored in the global
    mouse selection.

    This is shorthand for:

    \snippet doc/src/snippets/code/src_gui_kernel_qclipboard.cpp 1

    \sa image(), setPixmap() setMimeData()
*/
void QClipboard::setImage(const QImage &image, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setImageData(image);
    setMimeData(data, mode);
}

/*!
    Returns the clipboard pixmap, or null if the clipboard does not
    contain a pixmap. Note that this can lose information. For
    example, if the image is 24-bit and the display is 8-bit, the
    result is converted to 8 bits, and if the image has an alpha
    channel, the result just has a mask.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    pixmap is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the pixmap is retrieved from the global
    mouse selection.

    \sa setPixmap() image() mimeData() QPixmap::convertFromImage()
*/
QPixmap QClipboard::pixmap(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    return data ? qvariant_cast<QPixmap>(data->imageData()) : QPixmap();
}

/*!
    Copies \a pixmap into the clipboard. Note that this is slower
    than setImage() because it needs to convert the QPixmap to a
    QImage first.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    pixmap is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the pixmap is stored in the global
    mouse selection.

    \sa pixmap() setImage() setMimeData()
*/
void QClipboard::setPixmap(const QPixmap &pixmap, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setImageData(pixmap);
    setMimeData(data, mode);
}


/*!
    \fn QMimeData *QClipboard::mimeData(Mode mode) const

    Returns a reference to a QMimeData representation of the current
    clipboard data.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    data is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the data is retrieved from the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    data is retrieved from the search string buffer.

    The text(), image(), and pixmap() functions are simpler
    wrappers for retrieving text, image, and pixmap data.

    \sa setMimeData()
*/

/*!
    \fn void QClipboard::setMimeData(QMimeData *src, Mode mode)

    Sets the clipboard data to \a src. Ownership of the data is
    transferred to the clipboard. If you want to remove the data
    either call clear() or call setMimeData() again with new data.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    data is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the data is stored in the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    data is stored in the search string buffer.

    The setText(), setImage() and setPixmap() functions are simpler
    wrappers for setting text, image and pixmap data respectively.

    \sa mimeData()
*/

/*! 
    \fn void QClipboard::clear(Mode mode)
    Clear the clipboard contents.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, this
    function clears the global clipboard contents.  If \a mode is
    QClipboard::Selection, this function clears the global mouse
    selection contents. If \a mode is QClipboard::FindBuffer, this 
    function clears the search string buffer.

    \sa QClipboard::Mode, supportsSelection()
*/

#ifdef QT3_SUPPORT
/*!
    \fn QMimeSource *QClipboard::data(Mode mode) const
    \compat

    Use mimeData() instead.
*/
QMimeSource *QClipboard::data(Mode mode) const
{
    Q_D(const QClipboard);

    if (supportsMode(mode) == false)
        return 0;

    if (d->compat_data[mode])
        return d->compat_data[mode];

    d->wrapper[mode]->data = mimeData(mode);
    return d->wrapper[mode];
}


/*!
    \fn void QClipboard::setData(QMimeSource *src, Mode mode)
    \compat

    Use setMimeData() instead.
*/
void QClipboard::setData(QMimeSource *source, Mode mode)
{
    Q_D(QClipboard);

    if (supportsMode(mode) == false)
        return;

    d->compat_data[mode] = source;
    setMimeData(new QMimeSourceWrapper(d, mode), mode);
}
#endif // QT3_SUPPORT

/*!
    Returns true if the clipboard supports mouse selection; otherwise
    returns false.
*/
bool QClipboard::supportsSelection() const
{
    return supportsMode(Selection);
}

/*!
    Returns true if the clipboard supports a separate search buffer; otherwise
    returns false.
*/
bool QClipboard::supportsFindBuffer() const
{
    return supportsMode(FindBuffer);
}

/*!
    Returns true if this clipboard object owns the clipboard data;
    otherwise returns false.
*/
bool QClipboard::ownsClipboard() const
{
    return ownsMode(Clipboard);
}

/*!
    Returns true if this clipboard object owns the mouse selection
    data; otherwise returns false.
*/
bool QClipboard::ownsSelection() const
{
    return ownsMode(Selection);
}

/*!
    \since 4.2

    Returns true if this clipboard object owns the find buffer data;
    otherwise returns false.
*/
bool QClipboard::ownsFindBuffer() const
{
    return ownsMode(FindBuffer);
}

/*! 
    \internal
    \fn bool QClipboard::supportsMode(Mode mode) const;
    Returns true if the clipboard supports the clipboard mode speacified by \a mode;
    otherwise returns false.
*/

/*! 
    \internal
    \fn bool QClipboard::ownsMode(Mode mode) const;
    Returns true if the clipboard supports the clipboard data speacified by \a mode;
    otherwise returns false.
*/

/*! 
    \internal
    Emits the appropriate changed signal for \a mode.
*/
void QClipboard::emitChanged(Mode mode)
{
    switch (mode) {
        case Clipboard:
            emit dataChanged();
        break;
        case Selection:
            emit selectionChanged();
        break;
        case FindBuffer:
            emit findBufferChanged();
        break;
        default:
        break;
    }
    emit changed(mode);
}

const char* QMimeDataWrapper::format(int n) const
{
    if (formats.isEmpty()) {
        QStringList fmts = data->formats();
        for (int i = 0; i < fmts.size(); ++i)
            formats.append(fmts.at(i).toLatin1());
    }
    if (n < 0 || n >= formats.size())
        return 0;
    return formats.at(n).data();
}

QByteArray QMimeDataWrapper::encodedData(const char *format) const
{
    if (QLatin1String(format) != QLatin1String("application/x-qt-image")){
        return data->data(QLatin1String(format));
    } else{
        QVariant variant = data->imageData();
        QImage img = qvariant_cast<QImage>(variant);
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        return ba;
    }
}

QVariant QMimeSourceWrapper::retrieveData(const QString &mimetype, QVariant::Type) const
{
    return source->encodedData(mimetype.toLatin1());
}

bool QMimeSourceWrapper::hasFormat(const QString &mimetype) const
{
    return source->provides(mimetype.toLatin1());
}

QStringList QMimeSourceWrapper::formats() const
{
    QStringList fmts;
    int i = 0;
    const char *fmt;
    while ((fmt = source->format(i))) {
        fmts.append(QLatin1String(fmt));
        ++i;
    }
    return fmts;
}

#endif // QT_NO_CLIPBOARD

QT_END_NAMESPACE
