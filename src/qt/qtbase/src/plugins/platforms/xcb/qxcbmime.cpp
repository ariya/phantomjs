/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbmime.h"

#include <QtCore/QTextCodec>
#include <QtGui/QImageWriter>
#include <QtCore/QBuffer>
#include <qdebug.h>

#include <X11/Xutil.h>

#undef XCB_ATOM_STRING
#undef XCB_ATOM_PIXMAP
#undef XCB_ATOM_BITMAP

QT_BEGIN_NAMESPACE

#if !(defined(QT_NO_DRAGANDDROP) && defined(QT_NO_CLIPBOARD))

QXcbMime::QXcbMime()
    : QInternalMimeData()
{ }

QXcbMime::~QXcbMime()
{}



QString QXcbMime::mimeAtomToString(QXcbConnection *connection, xcb_atom_t a)
{
    if (a == XCB_NONE)
        return QString();

    // special cases for string type
    if (a == XCB_ATOM_STRING
        || a == connection->atom(QXcbAtom::UTF8_STRING)
        || a == connection->atom(QXcbAtom::TEXT))
        return QLatin1String("text/plain");

    // special case for images
    if (a == XCB_ATOM_PIXMAP)
        return QLatin1String("image/ppm");

    QByteArray atomName = connection->atomName(a);

    // special cases for uris
    if (atomName == "text/x-moz-url")
        atomName = "text/uri-list";

    return QString::fromLatin1(atomName.constData());
}

bool QXcbMime::mimeDataForAtom(QXcbConnection *connection, xcb_atom_t a, QMimeData *mimeData, QByteArray *data,
                               xcb_atom_t *atomFormat, int *dataFormat)
{
    if (!data)
        return false;

    bool ret = false;
    *atomFormat = a;
    *dataFormat = 8;

    if ((a == connection->atom(QXcbAtom::UTF8_STRING)
         || a == XCB_ATOM_STRING
         || a == connection->atom(QXcbAtom::TEXT))
        && QInternalMimeData::hasFormatHelper(QLatin1String("text/plain"), mimeData)) {
        if (a == connection->atom(QXcbAtom::UTF8_STRING)) {
            *data = QInternalMimeData::renderDataHelper(QLatin1String("text/plain"), mimeData);
            ret = true;
        } else if (a == XCB_ATOM_STRING ||
                   a == connection->atom(QXcbAtom::TEXT)) {
            // ICCCM says STRING is latin1
            *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        QLatin1String("text/plain"), mimeData)).toLatin1();
            ret = true;
        }
        return ret;
    }

    QString atomName = mimeAtomToString(connection, a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        if (atomName == QLatin1String("application/x-color"))
            *dataFormat = 16;
        ret = true;
    } else if (atomName == QLatin1String("text/x-moz-url") &&
               QInternalMimeData::hasFormatHelper(QLatin1String("text/uri-list"), mimeData)) {
        QByteArray uri = QInternalMimeData::renderDataHelper(
                         QLatin1String("text/uri-list"), mimeData).split('\n').first();
        QString mozUri = QString::fromLatin1(uri, uri.size());
        mozUri += QLatin1Char('\n');
        *data = QByteArray(reinterpret_cast<const char *>(mozUri.utf16()), mozUri.length() * 2);
        ret = true;
    } else if ((a == XCB_ATOM_PIXMAP || a == XCB_ATOM_BITMAP) && mimeData->hasImage()) {
        ret = true;
    }
    return ret;
}

QList<xcb_atom_t> QXcbMime::mimeAtomsForFormat(QXcbConnection *connection, const QString &format)
{
    QList<xcb_atom_t> atoms;
    atoms.append(connection->internAtom(format.toLatin1()));

    // special cases for strings
    if (format == QLatin1String("text/plain")) {
        atoms.append(connection->atom(QXcbAtom::UTF8_STRING));
        atoms.append(XCB_ATOM_STRING);
        atoms.append(connection->atom(QXcbAtom::TEXT));
    }

    // special cases for uris
    if (format == QLatin1String("text/uri-list"))
        atoms.append(connection->internAtom("text/x-moz-url"));

    //special cases for images
    if (format == QLatin1String("image/ppm"))
        atoms.append(XCB_ATOM_PIXMAP);
    if (format == QLatin1String("image/pbm"))
        atoms.append(XCB_ATOM_BITMAP);

    return atoms;
}

QVariant QXcbMime::mimeConvertToFormat(QXcbConnection *connection, xcb_atom_t a, const QByteArray &data, const QString &format,
                                       QVariant::Type requestedType, const QByteArray &encoding)
{
    QString atomName = mimeAtomToString(connection, a);
//    qDebug() << "mimeConvertDataToFormat" << format << atomName << data;

    if (!encoding.isEmpty()
        && atomName == format + QLatin1String(";charset=") + QString::fromLatin1(encoding)) {

#ifndef QT_NO_TEXTCODEC
        if (requestedType == QVariant::String) {
            QTextCodec *codec = QTextCodec::codecForName(encoding);
            if (codec)
                return codec->toUnicode(data);
        }
#endif

        return data;
    }

    // special cases for string types
    if (format == QLatin1String("text/plain")) {
        if (a == connection->atom(QXcbAtom::UTF8_STRING))
            return QString::fromUtf8(data);
        if (a == XCB_ATOM_STRING ||
            a == connection->atom(QXcbAtom::TEXT))
            return QString::fromLatin1(data);
    }

    // special case for uri types
    if (format == QLatin1String("text/uri-list")) {
        if (atomName == QLatin1String("text/x-moz-url")) {
            // we expect this as utf16 <url><space><title>
            // the first part is a url that should only contain ascci char
            // so it should be safe to check that the second char is 0
            // to verify that it is utf16
            if (data.size() > 1 && data.at(1) == 0)
                return QString::fromRawData((const QChar *)data.constData(),
                                data.size() / 2).split(QLatin1Char('\n')).first().toLatin1();
        }
    }

    if (atomName == format)
        return data;

#if 0 // ###
    // special case for images
    if (format == QLatin1String("image/ppm")) {
        if (a == XCB_ATOM_PIXMAP && data.size() == sizeof(Pixmap)) {
            Pixmap xpm = *((Pixmap*)data.data());
            if (!xpm)
                return QByteArray();
            Window root;
            int x;
            int y;
            uint width;
            uint height;
            uint border_width;
            uint depth;

            XGetGeometry(display, xpm, &root, &x, &y, &width, &height, &border_width, &depth);
            XImage *ximg = XGetImage(display,xpm,x,y,width,height,AllPlanes,depth==1 ? XYPixmap : ZPixmap);
            QImage qimg = QXlibStatic::qimageFromXImage(ximg);
            XDestroyImage(ximg);

            QImageWriter imageWriter;
            imageWriter.setFormat("PPMRAW");
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            imageWriter.setDevice(&buf);
            imageWriter.write(qimg);
            return buf.buffer();
        }
    }
#endif
    return QVariant();
}

xcb_atom_t QXcbMime::mimeAtomForFormat(QXcbConnection *connection, const QString &format, QVariant::Type requestedType,
                                 const QList<xcb_atom_t> &atoms, QByteArray *requestedEncoding)
{
    requestedEncoding->clear();

    // find matches for string types
    if (format == QLatin1String("text/plain")) {
        if (atoms.contains(connection->atom(QXcbAtom::UTF8_STRING)))
            return connection->atom(QXcbAtom::UTF8_STRING);
        if (atoms.contains(XCB_ATOM_STRING))
            return XCB_ATOM_STRING;
        if (atoms.contains(connection->atom(QXcbAtom::TEXT)))
            return connection->atom(QXcbAtom::TEXT);
    }

    // find matches for uri types
    if (format == QLatin1String("text/uri-list")) {
        xcb_atom_t a = connection->internAtom(format.toLatin1());
        if (a && atoms.contains(a))
            return a;
        a = connection->internAtom("text/x-moz-url");
        if (a && atoms.contains(a))
            return a;
    }

    // find match for image
    if (format == QLatin1String("image/ppm")) {
        if (atoms.contains(XCB_ATOM_PIXMAP))
            return XCB_ATOM_PIXMAP;
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType == QVariant::String
        && format.startsWith(QLatin1String("text/"))
        && !format.contains(QLatin1String("charset="))) {

        QString formatWithCharset = format;
        formatWithCharset.append(QLatin1String(";charset=utf-8"));

        xcb_atom_t a = connection->internAtom(formatWithCharset.toLatin1());
        if (a && atoms.contains(a)) {
            *requestedEncoding = "utf-8";
            return a;
        }
    }

    xcb_atom_t a = connection->internAtom(format.toLatin1());
    if (a && atoms.contains(a))
        return a;

    return 0;
}

#endif // !(defined(QT_NO_DRAGANDDROP) && defined(QT_NO_CLIPBOARD))

QT_END_NAMESPACE
