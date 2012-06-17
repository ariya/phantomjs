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

#ifndef QMIME_H
#define QMIME_H

#include <QtCore/qmimedata.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QMimeSource
{
public:
    virtual ~QMimeSource();
    virtual const char* format(int n = 0) const = 0;
    virtual bool provides(const char*) const;
    virtual QByteArray encodedData(const char*) const = 0;
};


#if defined(Q_WS_WIN)

QT_BEGIN_INCLUDE_NAMESPACE
typedef struct tagFORMATETC FORMATETC;
typedef struct tagSTGMEDIUM STGMEDIUM;
struct IDataObject;

#include <QtCore/qvariant.h>
QT_END_INCLUDE_NAMESPACE

/*
  Encapsulation of conversion between MIME and Windows CLIPFORMAT.
  Not need on X11, as the underlying protocol uses the MIME standard
  directly.
*/

class Q_GUI_EXPORT QWindowsMime
{
public:
    QWindowsMime();
    virtual ~QWindowsMime();

    // for converting from Qt
    virtual bool canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const = 0;
    virtual bool convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData, STGMEDIUM * pmedium) const = 0;
    virtual QVector<FORMATETC> formatsForMime(const QString &mimeType, const QMimeData *mimeData) const = 0;

    // for converting to Qt
    virtual bool canConvertToMime(const QString &mimeType, IDataObject *pDataObj) const = 0;
    virtual QVariant convertToMime(const QString &mimeType, IDataObject *pDataObj, QVariant::Type preferredType) const = 0;
    virtual QString mimeForFormat(const FORMATETC &formatetc) const = 0;

    static int registerMimeType(const QString &mime);

private:
    friend class QClipboardWatcher;
    friend class QDragManager;
    friend class QDropData;
    friend class QOleDataObject;

    static QWindowsMime *converterToMime(const QString &mimeType, IDataObject *pDataObj);
    static QStringList allMimesForFormats(IDataObject *pDataObj);
    static QWindowsMime *converterFromMime(const FORMATETC &formatetc, const QMimeData *mimeData);
    static QVector<FORMATETC> allFormatsForMime(const QMimeData *mimeData);
};

#endif
#if defined(Q_WS_MAC)

/*
  Encapsulation of conversion between MIME and Mac flavor.
  Not needed on X11, as the underlying protocol uses the MIME standard
  directly.
*/

class Q_GUI_EXPORT QMacMime { //Obsolete
    char type;
public:
    enum QMacMimeType { MIME_DND=0x01, MIME_CLIP=0x02, MIME_QT_CONVERTOR=0x04, MIME_ALL=MIME_DND|MIME_CLIP };
    explicit QMacMime(char) { }
    virtual ~QMacMime() { }

    static void initialize() { }

    static QList<QMacMime*> all(QMacMimeType) { return QList<QMacMime*>(); }
    static QMacMime *convertor(QMacMimeType, const QString &, int) { return 0; }
    static QString flavorToMime(QMacMimeType, int) { return QString(); }

    virtual QString convertorName()=0;
    virtual int countFlavors()=0;
    virtual int flavor(int index)=0;
    virtual bool canConvert(const QString &mime, int flav)=0;
    virtual QString mimeFor(int flav)=0;
    virtual int flavorFor(const QString &mime)=0;
    virtual QVariant convertToMime(const QString &mime, QList<QByteArray> data, int flav)=0;
    virtual QList<QByteArray> convertFromMime(const QString &mime, QVariant data, int flav)=0;
};

class Q_GUI_EXPORT QMacPasteboardMime {
    char type;
public:
    enum QMacPasteboardMimeType { MIME_DND=0x01,
                                  MIME_CLIP=0x02,
                                  MIME_QT_CONVERTOR=0x04,
                                  MIME_QT3_CONVERTOR=0x08,
                                  MIME_ALL=MIME_DND|MIME_CLIP
    };
    explicit QMacPasteboardMime(char);
    virtual ~QMacPasteboardMime();

    static void initialize();

    static QList<QMacPasteboardMime*> all(uchar);
    static QMacPasteboardMime *convertor(uchar, const QString &mime, QString flav);
    static QString flavorToMime(uchar, QString flav);

    virtual QString convertorName() = 0;

    virtual bool canConvert(const QString &mime, QString flav) = 0;
    virtual QString mimeFor(QString flav) = 0;
    virtual QString flavorFor(const QString &mime) = 0;
    virtual QVariant convertToMime(const QString &mime, QList<QByteArray> data, QString flav) = 0;
    virtual QList<QByteArray> convertFromMime(const QString &mime, QVariant data, QString flav) = 0;
};

// ### Qt 5: Add const QStringList& QMacPasteboardMime::supportedFlavours()
Q_GUI_EXPORT void qRegisterDraggedTypes(const QStringList &types);
#endif // Q_WS_MAC

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMIME_H
