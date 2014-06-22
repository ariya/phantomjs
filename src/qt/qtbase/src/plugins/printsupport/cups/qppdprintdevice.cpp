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

#include "qppdprintdevice.h"

#include <QtCore/QMimeDatabase>
#include <qdebug.h>

#ifndef QT_LINUXBASE // LSB merges everything into cups.h
#include <cups/language.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

QPpdPrintDevice::QPpdPrintDevice()
    : QPlatformPrintDevice(),
      m_cupsDest(0),
      m_ppd(0)
{
}

QPpdPrintDevice::QPpdPrintDevice(const QString &id)
    : QPlatformPrintDevice(id),
      m_cupsDest(0),
      m_ppd(0)
{
    if (!id.isEmpty()) {

        // TODO For now each dest is an individual device
        QStringList parts = id.split(QLatin1Char('/'));
        m_cupsName = parts.at(0).toUtf8();
        if (parts.size() > 1)
            m_cupsInstance = parts.at(1).toUtf8();
        loadPrinter();

        if (m_cupsDest && m_ppd) {
            m_name = printerOption("printer-info");
            m_location = printerOption("printer-location");
            m_makeAndModel = printerOption("printer-make-and-model");
            cups_ptype_e type = printerTypeFlags();
            m_isRemote = type & CUPS_PRINTER_REMOTE;
            // Note this is if the hardware does multiple copies, not if Cups can
            m_supportsMultipleCopies = type & CUPS_PRINTER_COPIES;
            // Note this is if the hardware does collation, not if Cups can
            m_supportsCollateCopies = type & CUPS_PRINTER_COLLATE;

            // Custom Page Size support
            // Cups cups_ptype_e CUPS_PRINTER_VARIABLE
            // Cups ppd_file_t variable_sizes custom_min custom_max
            // PPD MaxMediaWidth MaxMediaHeight
            m_supportsCustomPageSizes = type & CUPS_PRINTER_VARIABLE;
            m_minimumPhysicalPageSize = QSize(m_ppd->custom_min[0], m_ppd->custom_min[1]);
            m_maximumPhysicalPageSize = QSize(m_ppd->custom_max[0], m_ppd->custom_max[1]);
            m_customMargins = QMarginsF(m_ppd->custom_margins[0], m_ppd->custom_margins[3],
                                        m_ppd->custom_margins[2], m_ppd->custom_margins[1]);
        }
    }
}

QPpdPrintDevice::QPpdPrintDevice(const QPpdPrintDevice &other)
    : QPlatformPrintDevice(other),
      m_cupsDest(0),
      m_ppd(0)
{
    m_cupsName = other.m_cupsName;
    m_cupsInstance = other.m_cupsInstance;
    loadPrinter();
}

QPpdPrintDevice::~QPpdPrintDevice()
{
    if (m_ppd)
        ppdClose(m_ppd);
    if (m_cupsDest)
        cupsFreeDests(1, m_cupsDest);
    m_cupsDest = 0;
    m_ppd = 0;
}

QPpdPrintDevice &QPpdPrintDevice::operator=(const QPpdPrintDevice &other)
{
    m_cupsName = other.m_cupsName;
    m_cupsInstance = other.m_cupsInstance;
    if (other.m_cupsDest && other.m_ppd)
        loadPrinter();
    return *this;
}

bool QPpdPrintDevice::operator==(const QPpdPrintDevice &other) const
{
    return (m_id == other.m_id);
}

bool QPpdPrintDevice::isValid() const
{
    return m_cupsDest && m_ppd;
}

bool QPpdPrintDevice::isDefault() const
{
    return printerTypeFlags() & CUPS_PRINTER_DEFAULT;
}

QPrint::DeviceState QPpdPrintDevice::state() const
{
    // 3 = idle, 4 = printing, 5 = stopped
    // More details available from printer-state-message and printer-state-reasons
    int state =  printerOption(QStringLiteral("printer-state")).toInt();
    if (state == 3)
        return QPrint::Idle;
    else if (state == 4)
        return QPrint::Active;
    else
        return QPrint::Error;
}

void QPpdPrintDevice::loadPageSizes() const
{
    m_pageSizes.clear();
    m_printableMargins.clear();

    ppd_option_t *pageSizes = ppdFindOption(m_ppd, "PageSize");
    if (pageSizes) {
        for (int i = 0; i < pageSizes->num_choices; ++i) {
            const ppd_size_t *ppdSize = ppdPageSize(m_ppd, pageSizes->choices[i].choice);
            if (ppdSize) {
                // Returned size is in points
                QString key = QString::fromUtf8(ppdSize->name);
                QSize size = QSize(qRound(ppdSize->width), qRound(ppdSize->length));
                QString name = QString::fromUtf8(pageSizes->choices[i].text);
                if (!size.isEmpty()) {
                    QPageSize ps = createPageSize(key, size, name);
                    if (ps.isValid()) {
                        m_pageSizes.append(ps);
                        m_printableMargins.insert(key, QMarginsF(ppdSize->left, ppdSize->length - ppdSize->top,
                                                                 ppdSize->width - ppdSize->right, ppdSize->bottom));
                    }
                }
            }
        }
        m_havePageSizes = true;
    }
}

QPageSize QPpdPrintDevice::defaultPageSize() const
{
    ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "PageSize");
    if (defaultChoice) {
        ppd_size_t *ppdSize = ppdPageSize(m_ppd, defaultChoice->choice);
        if (ppdSize) {
            // Returned size is in points
            QString key = QString::fromUtf8(ppdSize->name);
            QSize size = QSize(qRound(ppdSize->width), qRound(ppdSize->length));
            QString name = QString::fromUtf8(defaultChoice->text);
            return createPageSize(key, size, name);
        }
    }
    return QPageSize();
}

QMarginsF QPpdPrintDevice::printableMargins(const QPageSize &pageSize,
                                            QPageLayout::Orientation orientation,
                                            int resolution) const
{
    Q_UNUSED(orientation)
    Q_UNUSED(resolution)
    if (!m_havePageSizes)
        loadPageSizes();
    // TODO Orientation?
    if (m_printableMargins.contains(pageSize.key()))
        return m_printableMargins.value(pageSize.key());
    return m_customMargins;
}

void QPpdPrintDevice::loadResolutions() const
{
    m_resolutions.clear();

    // Try load standard PPD options first
    ppd_option_t *resolutions = ppdFindOption(m_ppd, "Resolution");
    if (resolutions) {
        for (int i = 0; i < resolutions->num_choices; ++i) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[i].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    // If no result, try just the default
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "DefaultResolution");
        if (resolutions) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[0].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    // If still no result, then try HP's custom options
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "HPPrintQuality");
        if (resolutions) {
            for (int i = 0; i < resolutions->num_choices; ++i) {
                int res = QPrintUtils::parsePpdResolution(resolutions->choices[i].choice);
                if (res > 0)
                    m_resolutions.append(res);
            }
        }
    }
    if (m_resolutions.size() == 0) {
        resolutions = ppdFindOption(m_ppd, "DefaultHPPrintQuality");
        if (resolutions) {
            int res = QPrintUtils::parsePpdResolution(resolutions->choices[0].choice);
            if (res > 0)
                m_resolutions.append(res);
        }
    }
    m_haveResolutions = true;
}

int QPpdPrintDevice::defaultResolution() const
{
    // Try load standard PPD option first
    ppd_option_t *resolution = ppdFindOption(m_ppd, "DefaultResolution");
    if (resolution) {
        int res = QPrintUtils::parsePpdResolution(resolution->choices[0].choice);
        if (res > 0)
            return res;
    }
    // If no result, then try a marked option
    ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "Resolution");
    if (defaultChoice) {
        int res = QPrintUtils::parsePpdResolution(defaultChoice->choice);
        if (res > 0)
            return res;
    }
    // If still no result, then try HP's custom options
    resolution = ppdFindOption(m_ppd, "DefaultHPPrintQuality");
    if (resolution) {
        int res = QPrintUtils::parsePpdResolution(resolution->choices[0].choice);
        if (res > 0)
            return res;
    }
    defaultChoice = ppdFindMarkedChoice(m_ppd, "HPPrintQuality");
    if (defaultChoice) {
        int res = QPrintUtils::parsePpdResolution(defaultChoice->choice);
        if (res > 0)
            return res;
    }
    // Otherwise return a sensible default.
    // TODO What is sensible? 150? 300?
    return 72;
}

void QPpdPrintDevice::loadInputSlots() const
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

QPrint::InputSlot QPpdPrintDevice::defaultInputSlot() const
{
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

void QPpdPrintDevice::loadOutputBins() const
{
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

QPrint::OutputBin QPpdPrintDevice::defaultOutputBin() const
{
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

void QPpdPrintDevice::loadDuplexModes() const
{
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

QPrint::DuplexMode QPpdPrintDevice::defaultDuplexMode() const
{
    // Try load standard PPD option first
    if (m_ppd) {
        ppd_option_t *inputSlot = ppdFindOption(m_ppd, "DefaultDuplex");
        if (inputSlot)
            return QPrintUtils::ppdChoiceToDuplexMode(inputSlot->choices[0].choice);
        // If no result, then try a marked option
        ppd_choice_t *defaultChoice = ppdFindMarkedChoice(m_ppd, "Duplex");
        if (defaultChoice)
            return QPrintUtils::ppdChoiceToDuplexMode(defaultChoice->choice);
    }
    // Otherwise return None
    return QPrint::DuplexNone;
}

void QPpdPrintDevice::loadColorModes() const
{
    // Cups cups_ptype_e CUPS_PRINTER_BW CUPS_PRINTER_COLOR
    // Cups ppd_file_t color_device
    // PPD ColorDevice
    m_colorModes.clear();
    cups_ptype_e type = printerTypeFlags();
    if (type & CUPS_PRINTER_BW)
        m_colorModes.append(QPrint::GrayScale);
    if (type & CUPS_PRINTER_COLOR)
        m_colorModes.append(QPrint::Color);
    m_haveColorModes = true;
}

QPrint::ColorMode QPpdPrintDevice::defaultColorMode() const
{
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

void QPpdPrintDevice::loadMimeTypes() const
{
    // TODO No CUPS api? Need to manually load CUPS mime.types file?
    //      For now hard-code most common support types
    QMimeDatabase db;
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("application/pdf")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("application/postscript")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/gif")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/png")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/jpeg")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("image/tiff")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("text/html")));
    m_mimeTypes.append(db.mimeTypeForName(QStringLiteral("text/plain")));
    m_haveMimeTypes = true;
}

void QPpdPrintDevice::loadPrinter()
{
    // Just to be safe, check if existing printer needs closing
    if (m_ppd) {
        ppdClose(m_ppd);
        m_ppd = 0;
    }
    if (m_cupsDest) {
        cupsFreeDests(1, m_cupsDest);
        m_cupsDest = 0;
    }

    // Get the print instance and PPD file
    m_cupsDest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, m_cupsName, m_cupsInstance);
    if (m_cupsDest) {
        const char *ppdFile = cupsGetPPD(m_cupsName);
        if (ppdFile)
            m_ppd = ppdOpenFile(ppdFile);
        unlink(ppdFile);
        if (m_ppd) {
            ppdMarkDefaults(m_ppd);
        } else {
            cupsFreeDests(1, m_cupsDest);
            m_cupsDest = 0;
            m_ppd = 0;
        }
    }
}

QString QPpdPrintDevice::printerOption(const QString &key) const
{
    return cupsGetOption(key.toUtf8(), m_cupsDest->num_options, m_cupsDest->options);
}

cups_ptype_e QPpdPrintDevice::printerTypeFlags() const
{
    return static_cast<cups_ptype_e>(printerOption("printer-type").toUInt());
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
