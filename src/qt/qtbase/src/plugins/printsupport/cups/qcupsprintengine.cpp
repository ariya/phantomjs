/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qcupsprintengine_p.h"

#ifndef QT_NO_PRINTER

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "private/qcups_p.h" // Only needed for PPK_CupsOptions
#include <QtGui/qpagelayout.h>

#include <cups/cups.h>
#include <limits.h>
#include <math.h>

#include "private/qcore_unix_p.h" // overrides QT_OPEN

QT_BEGIN_NAMESPACE

extern QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits);

QCupsPrintEngine::QCupsPrintEngine(QPrinter::PrinterMode m)
    : QPdfPrintEngine(*new QCupsPrintEnginePrivate(m))
{
    Q_D(QCupsPrintEngine);
    d->setupDefaultPrinter();
    state = QPrinter::Idle;
}

QCupsPrintEngine::~QCupsPrintEngine()
{
}

void QCupsPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QCupsPrintEngine);

    switch (int(key)) {
    case PPK_PageSize:
        d->setPageSize(QPageSize(QPageSize::PageSizeId(value.toInt())));
        break;
    case PPK_WindowsPageSize:
        d->setPageSize(QPageSize(QPageSize::id(value.toInt())));
        break;
    case PPK_CustomPaperSize:
        d->setPageSize(QPageSize(value.toSizeF(), QPageSize::Point));
        break;
    case PPK_PaperName:
        // Get the named page size from the printer if supported
        d->setPageSize(d->m_printDevice.supportedPageSize(value.toString()));
        break;
    case PPK_PrinterName:
        d->changePrinter(value.toString());
        break;
    case PPK_CupsOptions:
        d->cupsOptions = value.toStringList();
        break;
    case PPK_QPageSize:
        d->setPageSize(value.value<QPageSize>());
        break;
    case PPK_QPageLayout: {
        QPageLayout pageLayout = value.value<QPageLayout>();
        if (pageLayout.isValid() && d->m_printDevice.isValidPageLayout(pageLayout, d->resolution)) {
            d->m_pageLayout = pageLayout;
            // Replace the page size with the CUPS page size
            d->setPageSize(d->m_printDevice.supportedPageSize(pageLayout.pageSize()));
        }
        break;
    }
    default:
        QPdfPrintEngine::setProperty(key, value);
        break;
    }
}

QVariant QCupsPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QCupsPrintEngine);

    QVariant ret;
    switch (int(key)) {
    case PPK_SupportsMultipleCopies:
        // CUPS server always supports multiple copies, even if individual m_printDevice doesn't
        ret = true;
        break;
    case PPK_NumberOfCopies:
        ret = 1;
        break;
    case PPK_CupsOptions:
        ret = d->cupsOptions;
        break;
    default:
        ret = QPdfPrintEngine::property(key);
        break;
    }
    return ret;
}


QCupsPrintEnginePrivate::QCupsPrintEnginePrivate(QPrinter::PrinterMode m) : QPdfPrintEnginePrivate(m)
{
}

QCupsPrintEnginePrivate::~QCupsPrintEnginePrivate()
{
}

bool QCupsPrintEnginePrivate::openPrintDevice()
{
    if (outDevice)
        return false;

    if (!outputFileName.isEmpty()) {
        QFile *file = new QFile(outputFileName);
        if (! file->open(QFile::WriteOnly|QFile::Truncate)) {
            delete file;
            return false;
        }
        outDevice = file;
    } else {
        char filename[512];
        fd = cupsTempFd(filename, 512);
        if (fd < 0) {
            qWarning("QPdfPrinter: Could not open temporary file to print");
            return false;
        }
        cupsTempFile = QString::fromLocal8Bit(filename);
        outDevice = new QFile();
        static_cast<QFile *>(outDevice)->open(fd, QIODevice::WriteOnly);
    }

    return true;
}

void QCupsPrintEnginePrivate::closePrintDevice()
{
    QPdfPrintEnginePrivate::closePrintDevice();

    if (!cupsTempFile.isEmpty()) {
        QString tempFile = cupsTempFile;
        cupsTempFile.clear();

        // Should never have got here without a printer, but check anyway
        if (printerName.isEmpty()) {
            qWarning("Could not determine printer to print to");
            QFile::remove(tempFile);
            return;
        }

        // Set up print options.
        QList<QPair<QByteArray, QByteArray> > options;
        QVector<cups_option_t> cupsOptStruct;

        options.append(QPair<QByteArray, QByteArray>("media", m_pageLayout.pageSize().key().toLocal8Bit()));

        if (copies > 1)
            options.append(QPair<QByteArray, QByteArray>("copies", QString::number(copies).toLocal8Bit()));

        if (copies > 1 && collate)
            options.append(QPair<QByteArray, QByteArray>("Collate", "True"));

        switch (duplex) {
        case QPrint::DuplexNone:
            options.append(QPair<QByteArray, QByteArray>("sides", "one-sided"));
            break;
        case QPrint::DuplexAuto:
            if (m_pageLayout.orientation() == QPageLayout::Portrait)
                options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-long-edge"));
            else
                options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-short-edge"));
            break;
        case QPrint::DuplexLongSide:
            options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-long-edge"));
            break;
        case QPrint::DuplexShortSide:
            options.append(QPair<QByteArray, QByteArray>("sides", "two-sided-short-edge"));
            break;
        }

        if (m_pageLayout.orientation() == QPageLayout::Landscape)
            options.append(QPair<QByteArray, QByteArray>("landscape", ""));

        QStringList::const_iterator it = cupsOptions.constBegin();
        while (it != cupsOptions.constEnd()) {
            options.append(QPair<QByteArray, QByteArray>((*it).toLocal8Bit(), (*(it+1)).toLocal8Bit()));
            it += 2;
        }

        for (int c = 0; c < options.size(); ++c) {
            cups_option_t opt;
            opt.name = options[c].first.data();
            opt.value = options[c].second.data();
            cupsOptStruct.append(opt);
        }

        // Print the file.
        cups_option_t* optPtr = cupsOptStruct.size() ? &cupsOptStruct.first() : 0;
        cupsPrintFile(printerName.toLocal8Bit().constData(), tempFile.toLocal8Bit().constData(),
                      title.toLocal8Bit().constData(), cupsOptStruct.size(), optPtr);

        QFile::remove(tempFile);
    }
}

void QCupsPrintEnginePrivate::setupDefaultPrinter()
{
    // Should never have reached here if no plugin available, but check just in case
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps)
        return;

    // Get default printer id, if no default then use the first available
    // TODO Find way to remove printerName from base class?
    printerName = ps->defaultPrintDeviceId();
    if (printerName.isEmpty()) {
        QStringList list = ps->availablePrintDeviceIds();
        if (list.size() > 0)
            printerName = list.at(0);
    }

    // Should never have reached here if no printers available, but check just in case
    if (printerName.isEmpty())
        return;

    m_printDevice = ps->createPrintDevice(printerName);
    if (!m_printDevice.isValid())
        return;

    // Setup the printer defaults
    duplex = m_printDevice.defaultDuplexMode();
    grayscale = m_printDevice.defaultColorMode() == QPrint::GrayScale;
    // CUPS server always supports collation, even if individual m_printDevice doesn't
    collate = true;
    setPageSize(m_printDevice.defaultPageSize());
}

void QCupsPrintEnginePrivate::changePrinter(const QString &newPrinter)
{
    // Don't waste time if same printer name
    if (newPrinter == printerName)
        return;

    // Should never have reached here if no plugin available, but check just in case
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (!ps)
        return;

    // Try create the printer, only use it if it returns valid
    QPrintDevice printDevice = ps->createPrintDevice(newPrinter);
    if (!m_printDevice.isValid())
        return;
    m_printDevice.swap(printDevice);
    printerName = m_printDevice.id();

    // Check if new printer supports current settings, otherwise us defaults
    if (duplex != QPrint::DuplexAuto && !m_printDevice.supportedDuplexModes().contains(duplex))
        duplex = m_printDevice.defaultDuplexMode();
    QPrint::ColorMode colorMode = grayscale ? QPrint::GrayScale : QPrint::Color;
    if (!m_printDevice.supportedColorModes().contains(colorMode))
        grayscale = m_printDevice.defaultColorMode() == QPrint::GrayScale;

    // Get the equivalent page size for this printer as supported names may be different
    setPageSize(m_pageLayout.pageSize());
}

void QCupsPrintEnginePrivate::setPageSize(const QPageSize &pageSize)
{
    if (pageSize.isValid()) {
        // Find if the requested page size has a matching printer page size, if so use its defined name instead
        QPageSize printerPageSize = m_printDevice.supportedPageSize(pageSize);
        QPageSize usePageSize = printerPageSize.isValid() ? printerPageSize : pageSize;
        QMarginsF printable = m_printDevice.printableMargins(usePageSize, m_pageLayout.orientation(), resolution);
        m_pageLayout.setPageSize(usePageSize, qt_convertMargins(printable, QPageLayout::Point, m_pageLayout.units()));
    }
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
