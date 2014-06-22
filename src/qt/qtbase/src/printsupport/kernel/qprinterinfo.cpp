/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file.  Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: http://www.gnu.org/copyleft/fdl.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qprinterinfo.h"
#include "qprinterinfo_p.h"
#include "qprintdevice_p.h"

#ifndef QT_NO_PRINTER

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QPrinterInfoPrivate, shared_null);

class QPrinterInfoPrivateDeleter
{
public:
    static inline void cleanup(QPrinterInfoPrivate *d)
    {
        if (d != shared_null)
            delete d;
    }
};

QPrinterInfoPrivate::QPrinterInfoPrivate(const QString &id)
{
    if (!id.isEmpty()) {
        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps)
            m_printDevice = ps->createPrintDevice(id);
    }
}

QPrinterInfoPrivate::~QPrinterInfoPrivate()
{
}

/*!
    \class QPrinterInfo

    \brief The QPrinterInfo class gives access to information about
    existing printers.

    \ingroup printing
    \inmodule QtPrintSupport

    Use the static functions to generate a list of QPrinterInfo
    objects. Each QPrinterInfo object in the list represents a single
    printer and can be queried for name, supported paper sizes, and
    whether or not it is the default printer.

    \since 4.4
*/

/*!
    Constructs an empty QPrinterInfo object.

    \sa isNull()
*/
QPrinterInfo::QPrinterInfo()
    : d_ptr(shared_null)
{
}

/*!
    Constructs a copy of \a other.
*/
QPrinterInfo::QPrinterInfo(const QPrinterInfo &other)
    : d_ptr((other.d_ptr.data() == shared_null) ? shared_null : new QPrinterInfoPrivate(*other.d_ptr))
{
}

/*!
    Constructs a QPrinterInfo object from \a printer.
*/
QPrinterInfo::QPrinterInfo(const QPrinter &printer)
    : d_ptr(shared_null)
{
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps) {
        QPrinterInfo pi(printer.printerName());
        if (pi.d_ptr.data() == shared_null)
            d_ptr.reset(shared_null);
        else
            d_ptr.reset(new QPrinterInfoPrivate(*pi.d_ptr));
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
    Sets the QPrinterInfo object to be equal to \a other.
*/
QPrinterInfo &QPrinterInfo::operator=(const QPrinterInfo &other)
{
    Q_ASSERT(d_ptr);
    if (other.d_ptr.data() == shared_null)
        d_ptr.reset(shared_null);
    else
        d_ptr.reset(new QPrinterInfoPrivate(*other.d_ptr));
    return *this;
}

/*!
    Returns the name of the printer.

    This is a unique id to identify the printer and may not be human-readable.

    \sa QPrinterInfo::description()
    \sa QPrinter::setPrinterName()
*/
QString QPrinterInfo::printerName() const
{
    const Q_D(QPrinterInfo);
    return d->m_printDevice.id();
}

/*!
    Returns the human-readable description of the printer.

    \since 5.0
    \sa QPrinterInfo::printerName()
*/
QString QPrinterInfo::description() const
{
    const Q_D(QPrinterInfo);
    return d->m_printDevice.name();
}

/*!
    Returns the human-readable location of the printer.

    \since 5.0
*/
QString QPrinterInfo::location() const
{
    const Q_D(QPrinterInfo);
    return d->m_printDevice.location();
}

/*!
    Returns the human-readable make and model of the printer.

    \since 5.0
*/
QString QPrinterInfo::makeAndModel() const
{
    const Q_D(QPrinterInfo);
    return d->m_printDevice.makeAndModel();
}

/*!
    Returns whether this QPrinterInfo object holds a printer definition.

    An empty QPrinterInfo object could result for example from calling
    defaultPrinter() when there are no printers on the system.
*/
bool QPrinterInfo::isNull() const
{
    Q_D(const QPrinterInfo);
    return d == shared_null || !d->m_printDevice.isValid();
}

/*!
    Returns whether this printer is currently the default printer.
*/
bool QPrinterInfo::isDefault() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.isDefault();
}

/*!
    Returns whether this printer is a remote network printer.

    \since 5.3
*/
bool QPrinterInfo::isRemote() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.isRemote();
}

/*!
    Returns the current state of this printer.

    This state may not always be accurate, depending on the platform, printer
    driver, or printer itself.

    \since 5.3
*/
QPrinter::PrinterState QPrinterInfo::state() const
{
    Q_D(const QPrinterInfo);
    return QPrinter::PrinterState(d->m_printDevice.state());
}

/*!
    Returns a list of Page Sizes supported by this printer.

    \since 5.3
*/

QList<QPageSize> QPrinterInfo::supportedPageSizes() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.supportedPageSizes();
}

/*!
    Returns the current default Page Size for this printer.

    \since 5.3
*/

QPageSize QPrinterInfo::defaultPageSize() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.defaultPageSize();
}

/*!
    Returns whether this printer supports custom page sizes.

    \since 5.3
*/

bool QPrinterInfo::supportsCustomPageSizes() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.supportsCustomPageSizes();
}

/*!
    Returns the minimum physical page size supported by this printer.

    \sa maximumPhysicalPageSize()

    \since 5.3
*/

QPageSize QPrinterInfo::minimumPhysicalPageSize() const
{
    Q_D(const QPrinterInfo);
    return QPageSize(d->m_printDevice.minimumPhysicalPageSize(), QString(), QPageSize::ExactMatch);
}

/*!
    Returns the maximum physical page size supported by this printer.

    \sa minimumPhysicalPageSize()

    \since 5.3
*/

QPageSize QPrinterInfo::maximumPhysicalPageSize() const
{
    Q_D(const QPrinterInfo);
    return QPageSize(d->m_printDevice.maximumPhysicalPageSize(), QString(), QPageSize::ExactMatch);
}

#if QT_DEPRECATED_SINCE(5,3)
/*!
    \obsolete Use supportedPageSizes() instead.

    Returns a list of supported paper sizes by the printer.

    Not all printer drivers support this query, so the list may be empty.

    \since 4.4
*/

QList<QPrinter::PaperSize> QPrinterInfo::supportedPaperSizes() const
{
    Q_D(const QPrinterInfo);
    QList<QPrinter::PaperSize> list;
    foreach (const QPageSize &pageSize, d->m_printDevice.supportedPageSizes())
        list.append(QPrinter::PaperSize(pageSize.id()));
    return list;
}

/*!
    \obsolete Use supportedPageSizes() instead.

    Returns a list of all the paper names supported by the driver with the
    corresponding size in millimeters.

    Not all printer drivers support this query, so the list may be empty.

    \since 5.1
*/

QList<QPair<QString, QSizeF> > QPrinterInfo::supportedSizesWithNames() const
{
    Q_D(const QPrinterInfo);
    QList<QPair<QString, QSizeF> > list;
    foreach (const QPageSize &pageSize, d->m_printDevice.supportedPageSizes())
        list.append(qMakePair(pageSize.name(), pageSize.size(QPageSize::Millimeter)));
    return list;
}
#endif // QT_DEPRECATED_SINCE(5,3)

/*!
    Returns a list of resolutions supported by this printer.

    \since 5.3
*/

QList<int> QPrinterInfo::supportedResolutions() const
{
    Q_D(const QPrinterInfo);
    return d->m_printDevice.supportedResolutions();
}

/*!
    Returns a list of all the available Printer Names on this system.

    It is recommended to use this instead of availablePrinters() as
    it will be faster on most systems.

    Note that the list may become outdated if changes are made on the local
    system or remote print server. Only instantiate required QPrinterInfo
    instances when needed, and always check for validity before calling.

    \since 5.3
*/
QStringList QPrinterInfo::availablePrinterNames()
{
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps)
        return ps->availablePrintDeviceIds();
    return QStringList();
}

/*!
    Returns a list of QPrinterInfo objects for all the available printers
    on this system.

    It is NOT recommended to use this as creating each printer instance may
    take a long time, especially if there are remote networked printers, and
    retained instances may become outdated if changes are made on the local
    system or remote print server. Use availablePrinterNames() instead and
    only instantiate printer instances as you need them.
*/
QList<QPrinterInfo> QPrinterInfo::availablePrinters()
{
    QList<QPrinterInfo> list;
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps) {
        foreach (const QString &id, ps->availablePrintDeviceIds())
            list.append(QPrinterInfo(id));
    }
    return list;
}

/*!
    Returns the current default printer name.

    \since 5.3
*/
QString QPrinterInfo::defaultPrinterName()
{
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps)
        return ps->defaultPrintDeviceId();
    return QString();
}

/*!
    Returns the default printer on the system.

    The return value should be checked using isNull() before being
    used, in case there is no default printer.

    On some systems it is possible for there to be available printers
    but none of them set to be the default printer.

    \sa isNull()
    \sa isDefault()
    \sa availablePrinters()
*/

QPrinterInfo QPrinterInfo::defaultPrinter()
{
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps)
        return QPrinterInfo(ps->defaultPrintDeviceId());
    return QPrinterInfo();
}

/*!
    Returns the printer \a printerName.

    The return value should be checked using isNull() before being
    used, in case the named printer does not exist.

    \since 5.0
    \sa isNull()
*/
QPrinterInfo QPrinterInfo::printerInfo(const QString &printerName)
{
    return QPrinterInfo(printerName);
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
