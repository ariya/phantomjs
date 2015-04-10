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

#include "qprintengine_pdf_p.h"

#ifndef QT_NO_PRINTER

#include <qiodevice.h>
#include <qfile.h>
#include <qdebug.h>
#include <qbuffer.h>
#include "qprinterinfo.h"
#include <QtGui/qpagelayout.h>

#include <limits.h>
#include <math.h>


#ifdef Q_OS_UNIX
#include "private/qcore_unix_p.h" // overrides QT_OPEN
#endif

#ifdef Q_OS_WIN
#include <io.h> // _close.
#endif

QT_BEGIN_NAMESPACE

QPdfPrintEngine::QPdfPrintEngine(QPrinter::PrinterMode m)
    : QPdfEngine(*new QPdfPrintEnginePrivate(m))
{
    state = QPrinter::Idle;
}

QPdfPrintEngine::QPdfPrintEngine(QPdfPrintEnginePrivate &p)
    : QPdfEngine(p)
{
    state = QPrinter::Idle;
}

QPdfPrintEngine::~QPdfPrintEngine()
{
}

bool QPdfPrintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QPdfPrintEngine);

    if (!d->openPrintDevice()) {
        state = QPrinter::Error;
        return false;
    }
    state = QPrinter::Active;

    return QPdfEngine::begin(pdev);
}

bool QPdfPrintEngine::end()
{
    Q_D(QPdfPrintEngine);

    QPdfEngine::end();

    d->closePrintDevice();
    state = QPrinter::Idle;

    return true;
}

bool QPdfPrintEngine::newPage()
{
    return QPdfEngine::newPage();
}

int QPdfPrintEngine::metric(QPaintDevice::PaintDeviceMetric m) const
{
    return QPdfEngine::metric(m);
}

void QPdfPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QPdfPrintEngine);

    switch (int(key)) {

    // The following keys are properties or derived values and so cannot be set
    case PPK_PageRect:
        break;
    case PPK_PaperRect:
        break;
    case PPK_PaperSources:
        break;
    case PPK_SupportsMultipleCopies:
        break;
    case PPK_SupportedResolutions:
        break;

    // The following keys are settings that are unsupported by the PDF PrintEngine
    case PPK_CustomBase:
        break;

    // The following keys are properties and settings that are supported by the PDF PrintEngine
    case PPK_CollateCopies:
        d->collate = value.toBool();
        break;
    case PPK_ColorMode:
        d->grayscale = (QPrinter::ColorMode(value.toInt()) == QPrinter::GrayScale);
        break;
    case PPK_Creator:
        d->creator = value.toString();
        break;
    case PPK_DocumentName:
        d->title = value.toString();
        break;
    case PPK_FullPage:
        if (value.toBool())
            d->m_pageLayout.setMode(QPageLayout::FullPageMode);
        else
            d->m_pageLayout.setMode(QPageLayout::StandardMode);
        break;
    case PPK_CopyCount: // fallthrough
    case PPK_NumberOfCopies:
        d->copies = value.toInt();
        break;
    case PPK_Orientation:
        d->m_pageLayout.setOrientation(QPageLayout::Orientation(value.toInt()));
        break;
    case PPK_OutputFileName:
        d->outputFileName = value.toString();
        break;
    case PPK_PageOrder:
        d->pageOrder = QPrinter::PageOrder(value.toInt());
        break;
    case PPK_PageSize: {
        QPageSize pageSize = QPageSize(QPageSize::PageSizeId(value.toInt()));
        if (pageSize.isValid())
            d->m_pageLayout.setPageSize(pageSize);
        break;
    }
    case PPK_PaperName: {
        QString name = value.toString();
        for (int i = 0; i <= QPageSize::LastPageSize; ++i) {
            QPageSize pageSize = QPageSize(QPageSize::PageSizeId(i));
            if (name == pageSize.name()) {
                d->m_pageLayout.setPageSize(pageSize);
                break;
            }
        }
        break;
    }
    case PPK_WindowsPageSize:
        d->m_pageLayout.setPageSize(QPageSize(QPageSize::id(value.toInt())));
        break;
    case PPK_PaperSource:
        d->paperSource = QPrinter::PaperSource(value.toInt());
        break;
    case PPK_PrinterName:
        d->printerName = value.toString();
        break;
    case PPK_PrinterProgram:
        d->printProgram = value.toString();
        break;
    case PPK_Resolution:
        d->resolution = value.toInt();
        break;
    case PPK_SelectionOption:
        d->selectionOption = value.toString();
        break;
    case PPK_FontEmbedding:
        d->embedFonts = value.toBool();
        break;
    case PPK_Duplex:
        d->duplex = static_cast<QPrint::DuplexMode>(value.toInt());
        break;
    case PPK_CustomPaperSize:
        d->m_pageLayout.setPageSize(QPageSize(value.toSizeF(), QPageSize::Point));
        break;
    case PPK_PageMargins: {
        QList<QVariant> margins(value.toList());
        Q_ASSERT(margins.size() == 4);
        d->m_pageLayout.setUnits(QPageLayout::Point);
        d->m_pageLayout.setMargins(QMarginsF(margins.at(0).toReal(), margins.at(1).toReal(),
                                             margins.at(2).toReal(), margins.at(3).toReal()));
        break;
    }
    case PPK_QPageSize: {
        QPageSize pageSize = value.value<QPageSize>();
        if (pageSize.isValid())
            d->m_pageLayout.setPageSize(pageSize);
        break;
    }
    case PPK_QPageMargins: {
        QPair<QMarginsF, QPageLayout::Unit> pair = value.value<QPair<QMarginsF, QPageLayout::Unit> >();
        d->m_pageLayout.setUnits(pair.second);
        d->m_pageLayout.setMargins(pair.first);
        break;
    }
    case PPK_QPageLayout: {
        QPageLayout pageLayout = value.value<QPageLayout>();
        if (pageLayout.isValid())
            d->m_pageLayout = pageLayout;
        break;
    }
    // No default so that compiler will complain if new keys added and not handled in this engine
    }
}

QVariant QPdfPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QPdfPrintEngine);

    QVariant ret;
    switch (int(key)) {

    // The following keys are settings that are unsupported by the PDF PrintEngine
    // Return sensible default values to ensure consistent behavior across platforms
    case PPK_CustomBase:
        // Special case, leave null
        break;

    // The following keys are properties and settings that are supported by the PDF PrintEngine
    case PPK_CollateCopies:
        ret = d->collate;
        break;
    case PPK_ColorMode:
        ret = d->grayscale ? QPrinter::GrayScale : QPrinter::Color;
        break;
    case PPK_Creator:
        ret = d->creator;
        break;
    case PPK_DocumentName:
        ret = d->title;
        break;
    case PPK_FullPage:
        ret = d->m_pageLayout.mode() == QPageLayout::FullPageMode;
        break;
    case PPK_CopyCount:
        ret = d->copies;
        break;
    case PPK_SupportsMultipleCopies:
        ret = false;
        break;
    case PPK_NumberOfCopies:
        ret = d->copies;
        break;
    case PPK_Orientation:
        ret = d->m_pageLayout.orientation();
        break;
    case PPK_OutputFileName:
        ret = d->outputFileName;
        break;
    case PPK_PageOrder:
        ret = d->pageOrder;
        break;
    case PPK_PageSize:
        ret = d->m_pageLayout.pageSize().id();
        break;
    case PPK_PaperName:
        ret = d->m_pageLayout.pageSize().name();
        break;
    case PPK_WindowsPageSize:
        ret = d->m_pageLayout.pageSize().windowsId();
        break;
    case PPK_PaperSource:
        ret = d->paperSource;
        break;
    case PPK_PrinterName:
        ret = d->printerName;
        break;
    case PPK_PrinterProgram:
        ret = d->printProgram;
        break;
    case PPK_Resolution:
        ret = d->resolution;
        break;
    case PPK_SupportedResolutions:
        ret = QList<QVariant>() << 72;
        break;
    case PPK_PaperRect:
        ret = d->m_pageLayout.fullRectPixels(d->resolution);
        break;
    case PPK_PageRect:
        ret = d->m_pageLayout.paintRectPixels(d->resolution);
        break;
    case PPK_SelectionOption:
        ret = d->selectionOption;
        break;
    case PPK_FontEmbedding:
        ret = d->embedFonts;
        break;
    case PPK_Duplex:
        ret = d->duplex;
        break;
    case PPK_CustomPaperSize:
        ret = d->m_pageLayout.fullRectPoints().size();
        break;
    case PPK_PageMargins: {
        QList<QVariant> list;
        QMarginsF margins = d->m_pageLayout.margins(QPageLayout::Point);
        list << margins.left() << margins.top() << margins.right() << margins.bottom();
        ret = list;
        break;
    }
    case PPK_QPageSize:
        ret.setValue(d->m_pageLayout.pageSize());
        break;
    case PPK_QPageMargins: {
        QPair<QMarginsF, QPageLayout::Unit> pair = qMakePair(d->m_pageLayout.margins(), d->m_pageLayout.units());
        ret.setValue(pair);
        break;
    }
    case PPK_QPageLayout:
        ret.setValue(d->m_pageLayout);
        break;
    // No default so that compiler will complain if new keys added and not handled in this engine
    }
    return ret;
}


bool QPdfPrintEnginePrivate::openPrintDevice()
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
    }

    return true;
}

void QPdfPrintEnginePrivate::closePrintDevice()
{
    if (outDevice) {
        outDevice->close();
        if (fd >= 0)
    #if defined(Q_OS_WIN) && defined(_MSC_VER) && _MSC_VER >= 1400
            ::_close(fd);
    #else
            ::close(fd);
    #endif
        fd = -1;
        delete outDevice;
        outDevice = 0;
    }
}



QPdfPrintEnginePrivate::QPdfPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfEnginePrivate(),
      duplex(QPrint::DuplexNone),
      collate(true),
      copies(1),
      pageOrder(QPrinter::FirstPageFirst),
      paperSource(QPrinter::Auto),
      fd(-1)
{
    resolution = 72;
    if (m == QPrinter::HighResolution)
        resolution = 1200;
    else if (m == QPrinter::ScreenResolution)
        resolution = qt_defaultDpi();
}

QPdfPrintEnginePrivate::~QPdfPrintEnginePrivate()
{
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
