/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
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

#include "qcocoaprintdevice.h"

#include <QtCore/qmimedatabase.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

static QPrint::DuplexMode macToDuplexMode(const PMDuplexMode &mode)
{
    if (mode == kPMDuplexTumble)
        return QPrint::DuplexShortSide;
    else if (mode == kPMDuplexNoTumble)
        return QPrint::DuplexLongSide;
    else // kPMDuplexNone or kPMSimplexTumble
        return QPrint::DuplexNone;
}

QCocoaPrintDevice::QCocoaPrintDevice()
    : QPlatformPrintDevice(),
      m_printer(0),
      m_session(0),
      m_ppd(0)
{
}

QCocoaPrintDevice::QCocoaPrintDevice(const QString &id)
    : QPlatformPrintDevice(id),
      m_printer(0),
      m_session(0),
      m_ppd(0)
{
    if (!id.isEmpty()) {
        m_printer = PMPrinterCreateFromPrinterID(QCFString::toCFStringRef(id));
        if (m_printer) {
            m_name = QCFString::toQString(PMPrinterGetName(m_printer));
            m_location = QCFString::toQString(PMPrinterGetLocation(m_printer));
            CFStringRef cfMakeAndModel;
            if (PMPrinterGetMakeAndModelName(m_printer, &cfMakeAndModel) == noErr)
                m_makeAndModel = QCFString::toQString(cfMakeAndModel);
            Boolean isRemote;
            if (PMPrinterIsRemote(m_printer, &isRemote) == noErr)
                m_isRemote = isRemote;
            if (PMCreateSession(&m_session) == noErr)
                PMSessionSetCurrentPMPrinter(m_session, m_printer);

            // No native api to query these options, need to use PPD directly, note is deprecated from 1.6 onwards
            if (openPpdFile()) {
                // Note this is if the hardware does multiple copies, not if Cups can
                m_supportsMultipleCopies = !m_ppd->manual_copies;
                // Note this is if the hardware does collation, not if Cups can
                ppd_option_t *collate = ppdFindOption(m_ppd, "Collate");
                if (collate)
                    m_supportsCollateCopies = true;
                m_supportsCustomPageSizes = m_ppd->custom_max[0] > 0 && m_ppd->custom_max[1] > 0;
                m_minimumPhysicalPageSize = QSize(m_ppd->custom_min[0], m_ppd->custom_min[1]);
                m_maximumPhysicalPageSize = QSize(m_ppd->custom_max[0], m_ppd->custom_max[1]);
                m_customMargins = QMarginsF(m_ppd->custom_margins[0], m_ppd->custom_margins[3],
                                            m_ppd->custom_margins[2], m_ppd->custom_margins[1]);
            }
        }
    }
}

QCocoaPrintDevice::QCocoaPrintDevice(const QCocoaPrintDevice &other)
    : QPlatformPrintDevice(other),
      m_printer(0),
      m_session(0),
      m_ppd(0)
{
    m_printer = other.m_printer;
    PMRetain(m_printer);
    m_session = other.m_session;
    PMRetain(m_session);
    m_macPapers = other.m_macPapers;
    foreach (PMPaper paper, m_macPapers.values())
        PMRetain(paper);
    openPpdFile();
    m_customMargins = other.m_customMargins;
    m_printableMargins = other.m_printableMargins;
}

QCocoaPrintDevice::~QCocoaPrintDevice()
{
    if (m_ppd)
        ppdClose(m_ppd);
    foreach (PMPaper paper, m_macPapers.values())
        PMRelease(paper);
    // Releasing the session appears to also release the printer
    if (m_session)
        PMRelease(m_session);
    else if (m_printer)
        PMRelease(m_printer);
}

QCocoaPrintDevice *QCocoaPrintDevice::clone()
{
    return new QCocoaPrintDevice(*this);
}

bool QCocoaPrintDevice::operator==(const QCocoaPrintDevice &other) const
{
    return (m_id == other.m_id);
}

bool QCocoaPrintDevice::isValid() const
{
    return m_printer ? true : false;
}

bool QCocoaPrintDevice::isDefault() const
{
    return PMPrinterIsDefault(m_printer);
}

QPrint::DeviceState QCocoaPrintDevice::state() const
{
    PMPrinterState state;
    if (PMPrinterGetState(m_printer, &state) == noErr) {
        if (state == kPMPrinterIdle)
            return QPrint::Idle;
        else if (state == kPMPrinterProcessing)
            return QPrint::Active;
        else if (state == kPMPrinterStopped)
            return QPrint::Error;
    }
    return QPrint::Error;
}

QPageSize QCocoaPrintDevice::createPageSize(const PMPaper &paper) const
{
    CFStringRef key;
    double width;
    double height;
    CFStringRef localizedName;
    if (PMPaperGetPPDPaperName(paper, &key) == noErr
        && PMPaperGetWidth(paper, &width) == noErr
        && PMPaperGetHeight(paper, &height) == noErr
        && PMPaperCreateLocalizedName(paper, m_printer, &localizedName) == noErr) {
        QPageSize pageSize = QPlatformPrintDevice::createPageSize(QString::fromCFString(key),QSize(width, height),
                                                                  QString::fromCFString(localizedName));
        CFRelease(localizedName);
        return pageSize;
    }
    return QPageSize();
}

void QCocoaPrintDevice::loadPageSizes() const
{
    m_pageSizes.clear();
    foreach (PMPaper paper, m_macPapers.values())
        PMRelease(paper);
    m_macPapers.clear();
    m_printableMargins.clear();
    CFArrayRef paperSizes;
    if (PMPrinterGetPaperList(m_printer, &paperSizes) == noErr) {
        int count = CFArrayGetCount(paperSizes);
        for (int i = 0; i < count; ++i) {
            PMPaper paper = static_cast<PMPaper>(const_cast<void *>(CFArrayGetValueAtIndex(paperSizes, i)));
            QPageSize pageSize = createPageSize(paper);
            if (pageSize.isValid()) {
                m_pageSizes.append(pageSize);
                PMRetain(paper);
                m_macPapers.insert(pageSize.key(), paper);
                PMPaperMargins printMargins;
                PMPaperGetMargins(paper, &printMargins);
                m_printableMargins.insert(pageSize.key(), QMarginsF(printMargins.left, printMargins.top,
                                                                    printMargins.right, printMargins.bottom));
            }
        }
    }
    m_havePageSizes = true;
}

QPageSize QCocoaPrintDevice::defaultPageSize() const
{
    QPageSize pageSize;
    PMPageFormat pageFormat;
    PMPaper paper;
    if (PMCreatePageFormat(&pageFormat) == noErr) {
        if (PMSessionDefaultPageFormat(m_session, pageFormat) == noErr
            && PMGetPageFormatPaper(pageFormat, &paper) == noErr) {
            pageSize = createPageSize(paper);
        }
        PMRelease(pageFormat);
    }
    return pageSize;
}

QMarginsF QCocoaPrintDevice::printableMargins(const QPageSize &pageSize,
                                              QPageLayout::Orientation orientation,
                                              int resolution) const
{
    Q_UNUSED(orientation)
    Q_UNUSED(resolution)
    if (!m_havePageSizes)
        loadPageSizes();
    if (m_printableMargins.contains(pageSize.key()))
        return m_printableMargins.value(pageSize.key());
    return m_customMargins;
}

void QCocoaPrintDevice::loadResolutions() const
{
    m_resolutions.clear();
    UInt32 count;
    if (PMPrinterGetPrinterResolutionCount(m_printer, &count) == noErr) {
        // 1-based index
        for (UInt32 i = 1; i <= count; ++i) {
            PMResolution resolution;
            if (PMPrinterGetIndexedPrinterResolution(m_printer, i, &resolution) == noErr)
                m_resolutions.append(int(resolution.hRes));
        }
    }
    m_haveResolutions = true;
}

int QCocoaPrintDevice::defaultResolution() const
{
    int defaultResolution = 72;
    PMPrintSettings settings;
    if (PMCreatePrintSettings(&settings) == noErr) {
        PMResolution resolution;
        if (PMSessionDefaultPrintSettings(m_session, settings) == noErr
            && PMPrinterGetOutputResolution(m_printer, settings, &resolution) == noErr) {
            // PMPrinterGetOutputResolution usually fails with -9589 kPMKeyNotFound as not set in PPD
            defaultResolution = int(resolution.hRes);
        }
        PMRelease(settings);
    }
    // If no value returned (usually means not set in PPD) then use supported resolutions which
    // OSX will have populated with at least one default value (but why not returned by call?)
    if (defaultResolution <= 0) {
        if (!m_haveResolutions)
            loadResolutions();
        if (m_resolutions.count() > 0)
            return m_resolutions.at(0);  // First value or highest? Only likely to be one anyway.
        return 72; // TDOD More sensible default value???
    }
    return defaultResolution;
}

void QCocoaPrintDevice::loadInputSlots() const
{
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // TODO Deal with concatenated names like Tray1Manual or Tray1_Man,
    //      will currently show as CustomInputSlot
    // TODO Deal with separate ManualFeed key
    // Try load standard PPD options first
    m_inputSlots.clear();
    if (m_ppd) {
        ppd_option_t *inputSlots = ppdFindOption(m_ppd, "InputSlot");
        if (inputSlots) {
            for (int i = 0; i < inputSlots->num_choices; ++i)
                m_inputSlots.append(QPrintUtils::ppdChoiceToInputSlot(inputSlots->choices[i]));
        }
        // If no result, try just the default
        if (m_inputSlots.size() == 0) {
            inputSlots = ppdFindOption(m_ppd, "DefaultInputSlot");
            if (inputSlots)
                m_inputSlots.append(QPrintUtils::ppdChoiceToInputSlot(inputSlots->choices[0]));
        }
    }
    // If still no result, just use Auto
    if (m_inputSlots.size() == 0)
        m_inputSlots.append(QPlatformPrintDevice::defaultInputSlot());
    m_haveInputSlots = true;
}

QPrint::InputSlot QCocoaPrintDevice::defaultInputSlot() const
{
    // No native api to query, use PPD directly
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *inputSlot = ppdFindOption(m_ppd, "DefaultInputSlot");
        if (inputSlot)
            return QPrintUtils::ppdChoiceToInputSlot(inputSlot->choices[0]);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "InputSlot");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToInputSlot(*defaultChoice);
    }
    // Otherwise return Auto
    return QPlatformPrintDevice::defaultInputSlot();
}

void QCocoaPrintDevice::loadOutputBins() const
{
    // No native api to query, use PPD directly
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    m_outputBins.clear();
    if (m_ppd) {
        ppd_option_t *outputBins = ppdFindOption(m_ppd, "OutputBin");
        if (outputBins) {
            for (int i = 0; i < outputBins->num_choices; ++i)
                m_outputBins.append(QPrintUtils::ppdChoiceToOutputBin(outputBins->choices[i]));
        }
        // If no result, try just the default
        if (m_outputBins.size() == 0) {
            outputBins = ppdFindOption(m_ppd, "DefaultOutputBin");
            if (outputBins)
                m_outputBins.append(QPrintUtils::ppdChoiceToOutputBin(outputBins->choices[0]));
        }
    }
    // If still no result, just use Auto
    if (m_outputBins.size() == 0)
        m_outputBins.append(QPlatformPrintDevice::defaultOutputBin());
    m_haveOutputBins = true;
}

QPrint::OutputBin QCocoaPrintDevice::defaultOutputBin() const
{
    // No native api to query, use PPD directly
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *outputBin = ppdFindOption(m_ppd, "DefaultOutputBin");
        if (outputBin)
            return QPrintUtils::ppdChoiceToOutputBin(outputBin->choices[0]);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "OutputBin");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToOutputBin(*defaultChoice);
    }
    // Otherwise return AutoBin
    return QPlatformPrintDevice::defaultOutputBin();
}

void QCocoaPrintDevice::loadDuplexModes() const
{
    // No native api to query, use PPD directly
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Try load standard PPD options first
    m_duplexModes.clear();
    if (m_ppd) {
        ppd_option_t *duplexModes = ppdFindOption(m_ppd, "Duplex");
        if (duplexModes) {
            for (int i = 0; i < duplexModes->num_choices; ++i)
                m_duplexModes.append(QPrintUtils::ppdChoiceToDuplexMode(duplexModes->choices[i].choice));
        }
        // If no result, try just the default
        if (m_duplexModes.size() == 0) {
            duplexModes = ppdFindOption(m_ppd, "DefaultDuplex");
            if (duplexModes)
                m_duplexModes.append(QPrintUtils::ppdChoiceToDuplexMode(duplexModes->choices[0].choice));
        }
    }
    // If still no result, or not added in PPD, then add None
    if (m_duplexModes.size() == 0 || !m_duplexModes.contains(QPrint::DuplexNone))
        m_duplexModes.append(QPrint::DuplexNone);
    m_haveDuplexModes = true;
}

QPrint::DuplexMode QCocoaPrintDevice::defaultDuplexMode() const
{
    QPrint::DuplexMode defaultMode = QPrint::DuplexNone;
    PMPrintSettings settings;
    if (PMCreatePrintSettings(&settings) == noErr) {
        PMDuplexMode duplexMode;
        if (PMSessionDefaultPrintSettings(m_session, settings) == noErr
            && PMGetDuplex(settings, &duplexMode) == noErr) {
                defaultMode = macToDuplexMode(duplexMode);
        }
        PMRelease(settings);
    }
    return defaultMode;
}

void QCocoaPrintDevice::loadColorModes() const
{
    // No native api to query, use PPD directly
    m_colorModes.clear();
    m_colorModes.append(QPrint::GrayScale);
    if (!m_ppd || (m_ppd && m_ppd->color_device))
        m_colorModes.append(QPrint::Color);
    m_haveColorModes = true;
}

QPrint::ColorMode QCocoaPrintDevice::defaultColorMode() const
{
    // No native api to query, use PPD directly
    // NOTE: Implemented in both CUPS and Mac plugins, please keep in sync
    // Not a proper option, usually only know if supports color or not, but some
    // users known to abuse ColorModel to always force GrayScale.
    if (m_ppd && supportedColorModes().contains(QPrint::Color)) {
        ppd_option_t *colorModel = ppdFindOption(m_ppd, "DefaultColorModel");
        if (!colorModel)
            colorModel = ppdFindOption(m_ppd, "ColorModel");
        if (!colorModel || (colorModel && !qstrcmp(colorModel->defchoice, "Gray")))
            return QPrint::Color;
    }
    return QPrint::GrayScale;
}

void QCocoaPrintDevice::loadMimeTypes() const
{
    // TODO Check how settings affect returned list
    m_mimeTypes.clear();
    QMimeDatabase db;
    PMPrintSettings settings;
    if (PMCreatePrintSettings(&settings) == noErr) {
        CFArrayRef mimeTypes;
        if (PMPrinterGetMimeTypes(m_printer, settings, &mimeTypes) == noErr) {
            int count = CFArrayGetCount(mimeTypes);
            for (int i = 0; i < count; ++i) {
                CFStringRef mimeName = static_cast<CFStringRef>(const_cast<void *>(CFArrayGetValueAtIndex(mimeTypes, i)));
                QMimeType mimeType = db.mimeTypeForName(QCFString::toQString(mimeName));
                if (mimeType.isValid())
                    m_mimeTypes.append(mimeType);
            }
        }
        PMRelease(settings);
    }
    m_haveMimeTypes = true;
}

bool QCocoaPrintDevice::openPpdFile()
{
    if (m_ppd)
        ppdClose(m_ppd);
    m_ppd = 0;
    CFURLRef ppdURL = NULL;
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
    char ppdPath[PATH_MAX];
#else
    char ppdPath[MAXPATHLEN];
#endif
    if (PMPrinterCopyDescriptionURL(m_printer, kPMPPDDescriptionType, &ppdURL) == noErr
        && ppdURL != NULL
        && CFURLGetFileSystemRepresentation(ppdURL, true, (UInt8*)ppdPath, sizeof(ppdPath))) {
        m_ppd = ppdOpenFile(ppdPath);
    }
    CFRelease(ppdURL);
    return m_ppd ? true : false;
}

PMPrinter QCocoaPrintDevice::macPrinter() const
{
    return m_printer;
}

// Returns a cached printer PMPaper, or creates and caches a new custom PMPaper
// Caller should never release a cached PMPaper!
PMPaper QCocoaPrintDevice::macPaper(const QPageSize &pageSize) const
{
    if (!m_havePageSizes)
        loadPageSizes();
    // If keys match, then is a supported size or an existing custom size
    if (m_macPapers.contains(pageSize.key()))
        return m_macPapers.value(pageSize.key());
    // For any other page size, whether custom or just unsupported, needs to be a custom PMPaper
    PMPaper paper = 0;
    PMPaperMargins paperMargins;
    paperMargins.left = m_customMargins.left();
    paperMargins.right = m_customMargins.right();
    paperMargins.top = m_customMargins.top();
    paperMargins.bottom = m_customMargins.bottom();
    PMPaperCreateCustom(m_printer, QCFString(pageSize.key()), QCFString(pageSize.name()),
                        pageSize.sizePoints().width(), pageSize.sizePoints().height(),
                        &paperMargins, &paper);
    m_macPapers.insert(pageSize.key(), paper);
    return paper;
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
