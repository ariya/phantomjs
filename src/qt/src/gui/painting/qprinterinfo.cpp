/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** GNU Free Documentation License
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms
** and conditions contained in a signed written agreement between you
** and Nokia.
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qprinterinfo.h"
#include "qprinterinfo_p.h"

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE

QPrinterInfoPrivate QPrinterInfoPrivate::shared_null;


/*!
    \class QPrinterInfo

    \brief The QPrinterInfo class gives access to information about
    existing printers.
    
    \ingroup printing

    Use the static functions to generate a list of QPrinterInfo
    objects. Each QPrinterInfo object in the list represents a single
    printer and can be queried for name, supported paper sizes, and
    whether or not it is the default printer.

    \since 4.4
*/

/*!
    \fn QList<QPrinterInfo> QPrinterInfo::availablePrinters()

    Returns a list of available printers on the system.
*/

/*!
    \fn QPrinterInfo QPrinterInfo::defaultPrinter()

    Returns the default printer on the system.

    The return value should be checked using isNull() before being
    used, in case there is no default printer.

    \sa isNull()
*/

/*!
    Constructs an empty QPrinterInfo object.

    \sa isNull()
*/
QPrinterInfo::QPrinterInfo()
    : d_ptr(&QPrinterInfoPrivate::shared_null)
{
}

/*!
    \since 4.8

    Constructs a copy of \a other.
*/
QPrinterInfo::QPrinterInfo(const QPrinterInfo &other)
    : d_ptr(new QPrinterInfoPrivate(*other.d_ptr))
{
}

/*!
    Constructs a QPrinterInfo object from \a printer.
*/
QPrinterInfo::QPrinterInfo(const QPrinter &printer)
    : d_ptr(&QPrinterInfoPrivate::shared_null)
{
    foreach (const QPrinterInfo &printerInfo, availablePrinters()) {
        if (printerInfo.printerName() == printer.printerName()) {
            d_ptr.reset(new QPrinterInfoPrivate(*printerInfo.d_ptr));
            break;
        }
    }
}

/*!
    \internal
*/
QPrinterInfo::QPrinterInfo(const QString &name)
    : d_ptr(new QPrinterInfoPrivate(name))
{
}

/*!
    Destroys the QPrinterInfo object. References to the values in the
    object become invalid.
*/
QPrinterInfo::~QPrinterInfo()
{
}

/*!
    \since 4.8

    Sets the QPrinterInfo object to be equal to \a other.
*/
QPrinterInfo &QPrinterInfo::operator=(const QPrinterInfo &other)
{
    Q_ASSERT(d_ptr);
    d_ptr.reset(new QPrinterInfoPrivate(*other.d_ptr));
    return *this;
}

/*!
    Returns the name of the printer.

    \sa QPrinter::setPrinterName()
*/
QString QPrinterInfo::printerName() const
{
    const Q_D(QPrinterInfo);
    return d->name;
}

/*!
    Returns whether this QPrinterInfo object holds a printer definition.

    An empty QPrinterInfo object could result for example from calling
    defaultPrinter() when there are no printers on the system.
*/
bool QPrinterInfo::isNull() const
{
    const Q_D(QPrinterInfo);
    return d == &QPrinterInfoPrivate::shared_null;
}

/*!
    Returns whether this printer is the default printer.
*/
bool QPrinterInfo::isDefault() const
{
    const Q_D(QPrinterInfo);
    return d->isDefault;
}

/*!
    \fn QList< QPrinter::PaperSize> QPrinterInfo::supportedPaperSizes() const
    \since 4.4

    Returns a list of supported paper sizes by the printer.

    Not all printer drivers support this query, so the list may be empty.
    On Mac OS X 10.3, this function always returns an empty list.
*/

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
