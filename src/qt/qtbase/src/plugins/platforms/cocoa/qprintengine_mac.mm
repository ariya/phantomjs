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

#include "qprintengine_mac_p.h"
#include "qcocoaprintersupport.h"
#include <quuid.h>
#include <QtGui/qpagelayout.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

#include "qcocoaautoreleasepool.h"

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE

extern QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits);

QMacPrintEngine::QMacPrintEngine(QPrinter::PrinterMode mode) : QPaintEngine(*(new QMacPrintEnginePrivate))
{
    Q_D(QMacPrintEngine);
    d->mode = mode;
    d->m_printDevice = new QCocoaPrintDevice(QCocoaPrinterSupport().defaultPrintDeviceId());
    d->m_pageLayout.setPageSize(d->m_printDevice->defaultPageSize());
    d->initialize();
}

bool QMacPrintEngine::begin(QPaintDevice *dev)
{
    Q_D(QMacPrintEngine);

    Q_ASSERT(dev && dev->devType() == QInternal::Printer);
    if (!static_cast<QPrinter *>(dev)->isValid())
        return false;

    if (d->state == QPrinter::Idle && !d->isPrintSessionInitialized()) // Need to reinitialize
        d->initialize();

    d->paintEngine->state = state;
    d->paintEngine->begin(dev);
    Q_ASSERT_X(d->state == QPrinter::Idle, "QMacPrintEngine", "printer already active");

    if (PMSessionValidatePrintSettings(d->session(), d->settings(), kPMDontWantBoolean) != noErr
        || PMSessionValidatePageFormat(d->session(), d->format(), kPMDontWantBoolean) != noErr) {
        d->state = QPrinter::Error;
        return false;
    }

    if (!d->outputFilename.isEmpty()) {
        QCFType<CFURLRef> outFile = CFURLCreateWithFileSystemPath(kCFAllocatorSystemDefault,
                                                                  QCFString(d->outputFilename),
                                                                  kCFURLPOSIXPathStyle,
                                                                  false);
        if (PMSessionSetDestination(d->session(), d->settings(), kPMDestinationFile,
                                    kPMDocumentFormatPDF, outFile) != noErr) {
            qWarning("QMacPrintEngine::begin: Problem setting file [%s]", d->outputFilename.toUtf8().constData());
            return false;
        }
    }

    OSStatus status = PMSessionBeginCGDocumentNoDialog(d->session(), d->settings(), d->format());
    if (status != noErr) {
        d->state = QPrinter::Error;
        return false;
    }

    d->state = QPrinter::Active;
    setActive(true);
    d->newPage_helper();
    return true;
}

bool QMacPrintEngine::end()
{
    Q_D(QMacPrintEngine);
    if (d->state == QPrinter::Aborted)
        return true;  // I was just here a function call ago :)
    if (d->paintEngine->type() == QPaintEngine::CoreGraphics) {
        // We don't need the paint engine to call restoreGraphicsState()
        static_cast<QCoreGraphicsPaintEngine*>(d->paintEngine)->d_func()->stackCount = 0;
        static_cast<QCoreGraphicsPaintEngine*>(d->paintEngine)->d_func()->hd = 0;
    }
    d->paintEngine->end();
    if (d->state != QPrinter::Idle)
        d->releaseSession();
    d->state  = QPrinter::Idle;
    return true;
}

QPaintEngine *
QMacPrintEngine::paintEngine() const
{
    return d_func()->paintEngine;
}

Qt::HANDLE QMacPrintEngine::handle() const
{
    QCoreGraphicsPaintEngine *cgEngine = static_cast<QCoreGraphicsPaintEngine*>(paintEngine());
    return cgEngine->d_func()->hd;
}

QMacPrintEnginePrivate::~QMacPrintEnginePrivate()
{
    [printInfo release];
    delete paintEngine;
}

QPrinter::PrinterState QMacPrintEngine::printerState() const
{
    return d_func()->state;
}

bool QMacPrintEngine::newPage()
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    OSStatus err = PMSessionEndPageNoDialog(d->session());
    if (err != noErr)  {
        if (err == kPMCancel) {
            // User canceled, we need to abort!
            abort();
        } else {
            // Not sure what the problem is...
            qWarning("QMacPrintEngine::newPage: Cannot end current page. %ld", long(err));
            d->state = QPrinter::Error;
        }
        return false;
    }
    return d->newPage_helper();
}

bool QMacPrintEngine::abort()
{
    Q_D(QMacPrintEngine);
    if (d->state != QPrinter::Active)
        return false;
    bool ret = end();
    d->state = QPrinter::Aborted;
    return ret;
}

int QMacPrintEngine::metric(QPaintDevice::PaintDeviceMetric m) const
{
    Q_D(const QMacPrintEngine);
    int val = 1;
    switch (m) {
    case QPaintDevice::PdmWidth:
        val = d->m_pageLayout.paintRectPixels(d->resolution.hRes).width();
        break;
    case QPaintDevice::PdmHeight:
        val = d->m_pageLayout.paintRectPixels(d->resolution.hRes).height();
        break;
    case QPaintDevice::PdmWidthMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).width());
        break;
    case QPaintDevice::PdmHeightMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).height());
        break;
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmPhysicalDpiY: {
        PMPrinter printer;
        if (PMSessionGetCurrentPrinter(d->session(), &printer) == noErr) {
            PMResolution resolution;
            PMPrinterGetOutputResolution(printer, d->settings(), &resolution);
            val = (int)resolution.vRes;
            break;
        }
        //otherwise fall through
    }
    case QPaintDevice::PdmDpiY:
        val = (int)d->resolution.vRes;
        break;
    case QPaintDevice::PdmDpiX:
        val = (int)d->resolution.hRes;
        break;
    case QPaintDevice::PdmNumColors:
        val = (1 << metric(QPaintDevice::PdmDepth));
        break;
    case QPaintDevice::PdmDepth:
        val = 24;
        break;
    case QPaintDevice::PdmDevicePixelRatio:
        val = 1;
        break;
    default:
        val = 0;
        qWarning("QPrinter::metric: Invalid metric command");
    }
    return val;
}

void QMacPrintEnginePrivate::initialize()
{
    Q_Q(QMacPrintEngine);

    Q_ASSERT(!printInfo);

    if (!paintEngine)
        paintEngine = new QCoreGraphicsPaintEngine();

    q->gccaps = paintEngine->gccaps;

    QCocoaAutoReleasePool pool;
    printInfo = [[NSPrintInfo alloc] initWithDictionary:[NSDictionary dictionary]];

    QList<int> resolutions = m_printDevice->supportedResolutions();
    if (!resolutions.isEmpty() && mode != QPrinter::ScreenResolution) {
        qSort(resolutions);
        if (resolutions.count() > 1 && mode == QPrinter::HighResolution)
            resolution.hRes = resolution.vRes = resolutions.last();
        else
            resolution.hRes = resolution.vRes = resolutions.first();
        if (resolution.hRes == 0)
            resolution.hRes = resolution.vRes = 600;
    } else {
        resolution.hRes = resolution.vRes = qt_defaultDpi();
    }

    setPageSize(m_pageLayout.pageSize());

    QHash<QMacPrintEngine::PrintEnginePropertyKey, QVariant>::const_iterator propC;
    for (propC = valueCache.constBegin(); propC != valueCache.constEnd(); propC++) {
        q->setProperty(propC.key(), propC.value());
    }
}

void QMacPrintEnginePrivate::releaseSession()
{
    PMSessionEndPageNoDialog(session());
    PMSessionEndDocumentNoDialog(session());
    [printInfo release];
    printInfo = 0;
}

bool QMacPrintEnginePrivate::newPage_helper()
{
    Q_Q(QMacPrintEngine);
    Q_ASSERT(state == QPrinter::Active);

    if (PMSessionError(session()) != noErr) {
        q->abort();
        return false;
    }

    // pop the stack of saved graphic states, in case we get the same
    // context back - either way, the stack count should be 0 when we
    // get the new one
    QCoreGraphicsPaintEngine *cgEngine = static_cast<QCoreGraphicsPaintEngine*>(paintEngine);
    while (cgEngine->d_func()->stackCount > 0)
        cgEngine->d_func()->restoreGraphicsState();

    OSStatus status = PMSessionBeginPageNoDialog(session(), format(), 0);
    if (status != noErr) {
        state = QPrinter::Error;
        return false;
    }

    QRect page = m_pageLayout.paintRectPixels(resolution.hRes);
    QRect paper = m_pageLayout.fullRectPixels(resolution.hRes);

    CGContextRef cgContext;
    OSStatus err = noErr;
    err = PMSessionGetCGGraphicsContext(session(), &cgContext);
    if (err != noErr) {
        qWarning("QMacPrintEngine::newPage: Cannot retrieve CoreGraphics context: %ld", long(err));
        state = QPrinter::Error;
        return false;
    }
    cgEngine->d_func()->hd = cgContext;

    // Set the resolution as a scaling ration of 72 (the default).
    CGContextScaleCTM(cgContext, 72 / resolution.hRes, 72 / resolution.vRes);

    CGContextScaleCTM(cgContext, 1, -1);
    CGContextTranslateCTM(cgContext, 0, -paper.height());
    if (m_pageLayout.mode() != QPageLayout::FullPageMode)
        CGContextTranslateCTM(cgContext, page.x() - paper.x(), page.y() - paper.y());
    cgEngine->d_func()->orig_xform = CGContextGetCTM(cgContext);
    cgEngine->d_func()->setClip(0);
    cgEngine->state->dirtyFlags = QPaintEngine::DirtyFlag(QPaintEngine::AllDirty
                                                          & ~(QPaintEngine::DirtyClipEnabled
                                                              | QPaintEngine::DirtyClipRegion
                                                              | QPaintEngine::DirtyClipPath));
    if (cgEngine->painter()->hasClipping())
        cgEngine->state->dirtyFlags |= QPaintEngine::DirtyClipEnabled;
    cgEngine->syncState();
    return true;
}

void QMacPrintEnginePrivate::setPageSize(const QPageSize &pageSize)
{
    if (!pageSize.isValid())
        return;

    // Get the matching printer paper
    QPageSize printerPageSize = m_printDevice->supportedPageSize(pageSize);
    QPageSize usePageSize = printerPageSize.isValid() ? printerPageSize : pageSize;

    // Get the PMPaper and check it is valid
    PMPaper macPaper = m_printDevice->macPaper(usePageSize);
    if (macPaper == 0) {
        qWarning() << "QMacPrintEngine: Invalid PMPaper returned for " << pageSize;
        return;
    }

    QMarginsF printable = m_printDevice->printableMargins(usePageSize, m_pageLayout.orientation(), resolution.hRes);
    m_pageLayout.setPageSize(usePageSize, qt_convertMargins(printable, QPageLayout::Point, m_pageLayout.units()));

    // You cannot set the page size on a PMPageFormat, you must create a new PMPageFormat
    PMPageFormat pageFormat;
    PMCreatePageFormatWithPMPaper(&pageFormat, macPaper);
    PMSetOrientation(pageFormat, m_pageLayout.orientation() == QPageLayout::Landscape ? kPMLandscape : kPMPortrait, kPMUnlocked);
    PMCopyPageFormat(pageFormat, format());
    if (PMSessionValidatePageFormat(session(), format(), kPMDontWantBoolean) != noErr)
        qWarning("QMacPrintEngine: Invalid page format");
    PMRelease(pageFormat);
}

void QMacPrintEngine::updateState(const QPaintEngineState &state)
{
    d_func()->paintEngine->updateState(state);
}

void QMacPrintEngine::drawRects(const QRectF *r, int num)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawRects(r, num);
}

void QMacPrintEngine::drawPoints(const QPointF *points, int pointCount)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawPoints(points, pointCount);
}

void QMacPrintEngine::drawEllipse(const QRectF &r)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawEllipse(r);
}

void QMacPrintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawLines(lines, lineCount);
}

void QMacPrintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawPolygon(points, pointCount, mode);
}

void QMacPrintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawPixmap(r, pm, sr);
}

void QMacPrintEngine::drawImage(const QRectF &r, const QImage &pm, const QRectF &sr, Qt::ImageConversionFlags flags)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawImage(r, pm, sr, flags);
}

void QMacPrintEngine::drawTextItem(const QPointF &p, const QTextItem &ti)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawTextItem(p, ti);
}

void QMacPrintEngine::drawTiledPixmap(const QRectF &dr, const QPixmap &pixmap, const QPointF &sr)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawTiledPixmap(dr, pixmap, sr);
}

void QMacPrintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QMacPrintEngine);
    Q_ASSERT(d->state == QPrinter::Active);
    d->paintEngine->drawPath(path);
}


void QMacPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QMacPrintEngine);

    d->valueCache.insert(key, value);
    if (!d->printInfo)
        return;

    switch (key) {

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

    // The following keys are settings that are unsupported by the Mac PrintEngine
    case PPK_ColorMode:
        break;
    case PPK_CustomBase:
        break;
    case PPK_Duplex:
        // TODO Add support using PMSetDuplex / PMGetDuplex
        break;
    case PPK_FontEmbedding:
        break;
    case PPK_PageOrder:
        // TODO Check if can be supported via Cups Options
        break;
    case PPK_PaperSource:
        // TODO Check if can be supported via Cups Options
        break;
    case PPK_PrinterProgram:
        break;
    case PPK_SelectionOption:
        break;

    // The following keys are properties and settings that are supported by the Mac PrintEngine
    case PPK_Resolution:  {
        // TODO It appears the old code didn't actually set the resolution???  Can we delete all this???
        int bestResolution = 0;
        int dpi = value.toInt();
        int bestDistance = INT_MAX;
        foreach (int resolution, d->m_printDevice->supportedResolutions()) {
            if (dpi == resolution) {
                bestResolution = resolution;
                break;
            } else {
                int distance = qAbs(dpi - resolution);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestResolution = resolution;
                }
            }
        }
        PMSessionValidatePageFormat(d->session(), d->format(), kPMDontWantBoolean);
        break;
    }
    case PPK_CollateCopies:
        PMSetCollate(d->settings(), value.toBool());
        break;
    case PPK_Creator:
        d->m_creator = value.toString();
        break;
    case PPK_DocumentName:
        PMPrintSettingsSetJobName(d->settings(), QCFString(value.toString()));
        break;
    case PPK_FullPage:
        if (value.toBool())
            d->m_pageLayout.setMode(QPageLayout::FullPageMode);
        else
            d->m_pageLayout.setMode(QPageLayout::StandardMode);
        break;
    case PPK_CopyCount: // fallthrough
    case PPK_NumberOfCopies:
        PMSetCopies(d->settings(), value.toInt(), false);
        break;
    case PPK_Orientation: {
        // First try set the Mac format orientation, then set our orientation to match result
        QPageLayout::Orientation newOrientation = QPageLayout::Orientation(value.toInt());
        PMOrientation macOrientation = (newOrientation == QPageLayout::Landscape) ? kPMLandscape : kPMPortrait;
        PMSetOrientation(d->format(), macOrientation, kPMUnlocked);
        PMSessionValidatePageFormat(d->session(), d->format(), kPMDontWantBoolean);
        PMGetOrientation(d->format(), &macOrientation);
        d->m_pageLayout.setOrientation(macOrientation == kPMLandscape ? QPageLayout::Landscape : QPageLayout::Portrait);
        break;
    }
    case PPK_OutputFileName:
        d->outputFilename = value.toString();
        break;
    case PPK_PageSize:
        d->setPageSize(QPageSize(QPageSize::PageSizeId(value.toInt())));
        break;
    case PPK_PaperName:
        // Get the named page size from the printer if supported
        d->setPageSize(d->m_printDevice->supportedPageSize(value.toString()));
        break;
    case PPK_WindowsPageSize:
        d->setPageSize(QPageSize(QPageSize::id(value.toInt())));
        break;
    case PPK_PrinterName: {
        QString id = value.toString();
        if (id.isEmpty())
            id = QCocoaPrinterSupport().defaultPrintDeviceId();
        else if (!QCocoaPrinterSupport().availablePrintDeviceIds().contains(id))
            break;
        d->m_printDevice = new QCocoaPrintDevice(id);
        PMPrinter printer = d->m_printDevice->macPrinter();
        PMRetain(printer);
        PMSessionSetCurrentPMPrinter(d->session(), printer);
        // TODO Do we need to check if the page size, etc, are valid on new printer?
        break;
    }
    case PPK_CustomPaperSize:
        d->setPageSize(QPageSize(value.toSizeF(), QPageSize::Point));
        break;
    case PPK_PageMargins:
    {
        QList<QVariant> margins(value.toList());
        Q_ASSERT(margins.size() == 4);
        d->m_pageLayout.setMargins(QMarginsF(margins.at(0).toReal(), margins.at(1).toReal(),
                                             margins.at(2).toReal(), margins.at(3).toReal()));
        break;
    }
    case PPK_QPageSize:
        d->setPageSize(value.value<QPageSize>());
        break;
    case PPK_QPageMargins: {
        QPair<QMarginsF, QPageLayout::Unit> pair = value.value<QPair<QMarginsF, QPageLayout::Unit> >();
        d->m_pageLayout.setUnits(pair.second);
        d->m_pageLayout.setMargins(pair.first);
        break;
    }
    case PPK_QPageLayout: {
        QPageLayout pageLayout = value.value<QPageLayout>();
        if (pageLayout.isValid() && d->m_printDevice->isValidPageLayout(pageLayout, d->resolution.hRes)) {
            setProperty(PPK_QPageSize, QVariant::fromValue(pageLayout.pageSize()));
            setProperty(PPK_FullPage, pageLayout.mode() == QPageLayout::FullPageMode);
            setProperty(PPK_Orientation, QVariant::fromValue(pageLayout.orientation()));
            d->m_pageLayout.setUnits(pageLayout.units());
            d->m_pageLayout.setMargins(pageLayout.margins());
        }
        break;
    }
    // No default so that compiler will complain if new keys added and not handled in this engine
    }
}

QVariant QMacPrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QMacPrintEngine);
    QVariant ret;

    if (!d->printInfo && d->valueCache.contains(key))
        return *d->valueCache.find(key);

    switch (key) {

    // The following keys are settings that are unsupported by the Mac PrintEngine
    // Return sensible default values to ensure consistent behavior across platforms
    case PPK_ColorMode:
        ret = QPrinter::Color;
        break;
    case PPK_CustomBase:
        // Special case, leave null
        break;
    case PPK_Duplex:
        // TODO Add support using PMSetDuplex / PMGetDuplex
        ret = QPrinter::DuplexNone;
        break;
    case PPK_FontEmbedding:
        ret = false;
        break;
    case PPK_PageOrder:
        // TODO Check if can be supported via Cups Options
        ret = QPrinter::FirstPageFirst;
        break;
    case PPK_PaperSource:
        // TODO Check if can be supported via Cups Options
        ret = QPrinter::Auto;
        break;
    case PPK_PaperSources: {
        // TODO Check if can be supported via Cups Options
        QList<QVariant> out;
        out << int(QPrinter::Auto);
        ret = out;
        break;
        }
    case PPK_PrinterProgram:
        ret = QString();
        break;
    case PPK_SelectionOption:
        ret = QString();
        break;

    // The following keys are properties and settings that are supported by the Mac PrintEngine
    case PPK_CollateCopies: {
        Boolean status;
        PMGetCollate(d->settings(), &status);
        ret = bool(status);
        break;
    }
    case PPK_Creator:
        ret = d->m_creator;
        break;
    case PPK_DocumentName: {
        CFStringRef name;
        PMPrintSettingsGetJobName(d->settings(), &name);
        ret = QCFString::toQString(name);
        break;
    }
    case PPK_FullPage:
        ret = d->m_pageLayout.mode() == QPageLayout::FullPageMode;
        break;
    case PPK_NumberOfCopies:
        ret = 1;
        break;
    case PPK_CopyCount: {
        UInt32 copies = 1;
        PMGetCopies(d->settings(), &copies);
        ret = (uint) copies;
        break;
    }
    case PPK_SupportsMultipleCopies:
        ret = true;
        break;
    case PPK_Orientation:
        ret = d->m_pageLayout.orientation();
        break;
    case PPK_OutputFileName:
        ret = d->outputFilename;
        break;
    case PPK_PageRect:
        // PageRect is returned in device pixels
        ret = d->m_pageLayout.paintRectPixels(d->resolution.hRes);
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
    case PPK_PaperRect:
        // PaperRect is returned in device pixels
        ret = d->m_pageLayout.fullRectPixels(d->resolution.hRes);
        break;
    case PPK_PrinterName:
        return d->m_printDevice->id();
        break;
    case PPK_Resolution: {
        ret = d->resolution.hRes;
        break;
    }
    case PPK_SupportedResolutions: {
        QList<QVariant> list;
        foreach (int resolution, d->m_printDevice->supportedResolutions())
            list << resolution;
        ret = list;
        break;
    }
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
    // No default so that compiler will complain if new keys added and not handled in this engine
    }
    return ret;
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
