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

#include "qwindowsprintdevice.h"

#include <qdebug.h>

#ifndef DC_COLLATE
#  define DC_COLLATE 22
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

extern qreal qt_pointMultiplier(QPageLayout::Unit unit);

static inline uint qwcsnlen(const wchar_t *str, uint maxlen)
{
    uint length = 0;
    if (str) {
        while (length < maxlen && *str++)
            length++;
    }
    return length;
}

static QPrint::InputSlot paperBinToInputSlot(int windowsId, const QString &name)
{
    QPrint::InputSlot slot;
    slot.name = name;
    int i;
    for (i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
        if (inputSlotMap[i].windowsId == windowsId) {
            slot.key = inputSlotMap[i].key;
            slot.id = inputSlotMap[i].id;
            slot.windowsId = inputSlotMap[i].windowsId;
            return slot;
        }
    }
    slot.key = inputSlotMap[i].key;
    slot.id = inputSlotMap[i].id;
    return slot;
}


QWindowsPrintDevice::QWindowsPrintDevice()
    : QPlatformPrintDevice(),
      m_hPrinter(0)
{
}

QWindowsPrintDevice::QWindowsPrintDevice(const QString &id)
    : QPlatformPrintDevice(id),
      m_hPrinter(0)
{
    // First do a fast lookup to see if printer exists, if it does then open it
    if (!id.isEmpty() && QWindowsPrintDevice::availablePrintDeviceIds().contains(id)) {
        if (OpenPrinter((LPWSTR)m_id.utf16(), &m_hPrinter, NULL)) {
            DWORD needed = 0;
            GetPrinter(m_hPrinter, 2, 0, 0, &needed);
            QScopedArrayPointer<BYTE> buffer(new BYTE[needed]);
            if (GetPrinter(m_hPrinter, 2, buffer.data(), needed, &needed)) {
                PPRINTER_INFO_2 info = reinterpret_cast<PPRINTER_INFO_2>(buffer.data());
                m_name = QString::fromWCharArray(info->pPrinterName);
                m_location = QString::fromWCharArray(info->pLocation);
                m_makeAndModel = QString::fromWCharArray(info->pDriverName); // TODO Check is not available elsewhere
                m_isRemote = info->Attributes & PRINTER_ATTRIBUTE_NETWORK;
            }
            m_supportsMultipleCopies = (DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_COPIES, NULL, NULL) > 1);
            m_supportsCollateCopies = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_COLLATE, NULL, NULL);
            // Min/Max custom size is in tenths of a millimeter
            const qreal multiplier = qt_pointMultiplier(QPageLayout::Millimeter);
            DWORD min = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_MINEXTENT, NULL, NULL);
            m_minimumPhysicalPageSize = QSize((LOWORD(min) / 10.0) * multiplier, (HIWORD(min) / 10.0) * multiplier);
            DWORD max = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_MAXEXTENT, NULL, NULL);
            m_maximumPhysicalPageSize = QSize((LOWORD(max) / 10.0) * multiplier, (HIWORD(max) / 10.0) * multiplier);
            m_supportsCustomPageSizes = (m_maximumPhysicalPageSize.width() > 0 && m_maximumPhysicalPageSize.height() > 0);
        }
    }
}

QWindowsPrintDevice::QWindowsPrintDevice(const QWindowsPrintDevice &other)
    : QPlatformPrintDevice(other)
{
    OpenPrinter((LPWSTR)other.m_id.utf16(), &m_hPrinter, NULL);
}

QWindowsPrintDevice::~QWindowsPrintDevice()
{
    ClosePrinter(m_hPrinter);
}

QWindowsPrintDevice &QWindowsPrintDevice::operator=(const QWindowsPrintDevice &other)
{
    OpenPrinter((LPWSTR)other.m_id.utf16(), &m_hPrinter, NULL);
    return *this;
}

bool QWindowsPrintDevice::operator==(const QWindowsPrintDevice &other) const
{
    return (m_id == other.m_id);
}

bool QWindowsPrintDevice::isValid() const
{
    return m_hPrinter;
}

bool QWindowsPrintDevice::isDefault() const
{
    return m_id == defaultPrintDeviceId();
}

QPrint::DeviceState QWindowsPrintDevice::state() const
{
    DWORD needed = 0;
    GetPrinter(m_hPrinter, 6, 0, 0, &needed);
    QScopedArrayPointer<BYTE> buffer(new BYTE[needed]);

    if (GetPrinter(m_hPrinter, 6, buffer.data(), needed, &needed)) {
        PPRINTER_INFO_6 info = reinterpret_cast<PPRINTER_INFO_6>(buffer.data());
        // TODO Check mapping
        if (info->dwStatus == 0
            || (info->dwStatus & PRINTER_STATUS_WAITING) == PRINTER_STATUS_WAITING
            || (info->dwStatus & PRINTER_STATUS_POWER_SAVE) == PRINTER_STATUS_POWER_SAVE) {
            return QPrint::Idle;
        } else if ((info->dwStatus & PRINTER_STATUS_PRINTING) == PRINTER_STATUS_PRINTING
                   || (info->dwStatus & PRINTER_STATUS_BUSY) == PRINTER_STATUS_BUSY
                   || (info->dwStatus & PRINTER_STATUS_INITIALIZING) == PRINTER_STATUS_INITIALIZING
                   || (info->dwStatus & PRINTER_STATUS_IO_ACTIVE) == PRINTER_STATUS_IO_ACTIVE
                   || (info->dwStatus & PRINTER_STATUS_PROCESSING) == PRINTER_STATUS_PROCESSING
                   || (info->dwStatus & PRINTER_STATUS_WARMING_UP) == PRINTER_STATUS_WARMING_UP) {
            return QPrint::Active;
        }
    }

    return QPrint::Error;
}

void QWindowsPrintDevice::loadPageSizes() const
{
    // Get the number of paper sizes and check all 3 attributes have same count
    DWORD paperCount = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERNAMES, NULL, NULL);
    if (int(paperCount) > 0
        && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERSIZE, NULL, NULL) == paperCount
        && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERS, NULL, NULL) == paperCount) {

        QScopedArrayPointer<wchar_t> paperNames(new wchar_t[paperCount*64]);
        QScopedArrayPointer<POINT> winSizes(new POINT[paperCount*sizeof(POINT)]);
        QScopedArrayPointer<wchar_t> papers(new wchar_t[paperCount]);

        // Get the details and match the default paper size
        if (DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERNAMES, paperNames.data(), NULL) == paperCount
            && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERSIZE, (wchar_t *)winSizes.data(), NULL) == paperCount
            && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_PAPERS, papers.data(), NULL) == paperCount) {

            // Returned size is in tenths of a millimeter
            const qreal multiplier = qt_pointMultiplier(QPageLayout::Millimeter);
            for (int i = 0; i < int(paperCount); ++i) {
                QSize size = QSize(qRound((winSizes[i].x / 10.0) * multiplier), qRound((winSizes[i].y / 10.0) * multiplier));
                wchar_t *paper = paperNames.data() + (i * 64);
                QString name = QString::fromWCharArray(paper, qwcsnlen(paper, 64));
                m_pageSizes.append(createPageSize(papers[i], size, name));
            }

        }
    }

    m_havePageSizes = true;
}

QPageSize QWindowsPrintDevice::defaultPageSize() const
{
    if (!m_havePageSizes)
        loadPageSizes();

    QPageSize pageSize;

    // Allocate the required DEVMODE buffer
    DWORD dmSize = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), NULL, NULL, 0);
    LPDEVMODE pDevMode = (LPDEVMODE)malloc(dmSize);

     // Get the default DevMode
    DWORD result = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), pDevMode, NULL, DM_OUT_BUFFER);

    // Get the default paper size
    if (result == IDOK && pDevMode->dmFields & DM_PAPERSIZE) {
        // Find the supported page size that matches, in theory default should be one of them
        foreach (const QPageSize &ps, m_pageSizes) {
            if (ps.windowsId() == pDevMode->dmPaperSize) {
                pageSize = ps;
                break;
            }
        }
    }

    // Clean-up
    free(pDevMode);
    return pageSize;
}

QMarginsF QWindowsPrintDevice::printableMargins(const QPageSize &pageSize,
                                                QPageLayout::Orientation orientation,
                                                int resolution) const
{
    // TODO This is slow, need to cache values or find better way!
    // Modify the DevMode to get the DC printable margins in device pixels
    QMarginsF margins = QMarginsF(0, 0, 0, 0);
    DWORD needed = 0;
    GetPrinter(m_hPrinter, 2, 0, 0, &needed);
    QScopedArrayPointer<BYTE> buffer(new BYTE[needed]);
    if (GetPrinter(m_hPrinter, 2, buffer.data(), needed, &needed)) {
        PPRINTER_INFO_2 info = reinterpret_cast<PPRINTER_INFO_2>(buffer.data());
        DEVMODE *devMode = info->pDevMode;
        HDC pDC = CreateDC(NULL, (LPWSTR)m_id.utf16(), NULL, devMode);
        if (pageSize.id() == QPageSize::Custom || pageSize.windowsId() <= 0 || pageSize.windowsId() > DMPAPER_LAST) {
            devMode->dmPaperSize =  0;
            devMode->dmPaperWidth = pageSize.size(QPageSize::Millimeter).width() * 10.0;
            devMode->dmPaperLength = pageSize.size(QPageSize::Millimeter).height() * 10.0;
        } else {
            devMode->dmPaperSize =  pageSize.windowsId();
        }
        devMode->dmPrintQuality = resolution;
        devMode->dmOrientation = orientation == QPageLayout::Portrait ? DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE;
        ResetDC(pDC, devMode);
        const int dpiWidth = GetDeviceCaps(pDC, LOGPIXELSX);
        const int dpiHeight = GetDeviceCaps(pDC, LOGPIXELSY);
        const qreal wMult = 72.0 / dpiWidth;
        const qreal hMult = 72.0 / dpiHeight;
        const qreal physicalWidth = GetDeviceCaps(pDC, PHYSICALWIDTH) * wMult;
        const qreal physicalHeight = GetDeviceCaps(pDC, PHYSICALHEIGHT) * hMult;
        const qreal printableWidth = GetDeviceCaps(pDC, HORZRES) * wMult;
        const qreal printableHeight = GetDeviceCaps(pDC, VERTRES) * hMult;
        const qreal leftMargin = GetDeviceCaps(pDC, PHYSICALOFFSETX)* wMult;
        const qreal topMargin = GetDeviceCaps(pDC, PHYSICALOFFSETY) * hMult;
        const qreal rightMargin = physicalWidth - leftMargin - printableWidth;
        const qreal bottomMargin = physicalHeight - topMargin - printableHeight;
        margins = QMarginsF(leftMargin, topMargin, rightMargin, bottomMargin);
        ReleaseDC(NULL, pDC);
    }
    return margins;
}

void QWindowsPrintDevice::loadResolutions() const
{
    DWORD resCount = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_ENUMRESOLUTIONS, NULL, NULL);
    if (int(resCount) > 0) {
        QScopedArrayPointer<LONG> resolutions(new LONG[resCount*sizeof(LONG)]);
        // Get the details and match the default paper size
        if (DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_ENUMRESOLUTIONS, (LPWSTR)resolutions.data(), NULL) == resCount) {
            for (int i = 0; i < int(resCount); ++i)
                m_resolutions.append(resolutions[i]);
        }
    }
    m_haveResolutions = true;
}

int QWindowsPrintDevice::defaultResolution() const
{
    int resolution = 72;  // TODO Set a sensible default?

    // Allocate the required DEVMODE buffer
    DWORD dmSize = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), NULL, NULL, 0);
    LPDEVMODE pDevMode = (LPDEVMODE)malloc(dmSize);

     // Get the default DevMode
    DWORD result = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), pDevMode, NULL, DM_OUT_BUFFER);

    // Get the default resolution
    if (result == IDOK && pDevMode->dmFields & DM_YRESOLUTION) {
        if (pDevMode->dmPrintQuality > 0)
            resolution = pDevMode->dmPrintQuality;
        else
            resolution = pDevMode->dmYResolution;
    }

    // Clean-up
    free(pDevMode);
    return resolution;
}

void QWindowsPrintDevice::loadInputSlots() const
{
    DWORD binCount = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_BINS, NULL, NULL);
    if (int(binCount) > 0
        && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_BINNAMES, NULL, NULL) == binCount) {

        QScopedArrayPointer<WORD> bins(new WORD[binCount*sizeof(WORD)]);
        QScopedArrayPointer<wchar_t> binNames(new wchar_t[binCount*24]);

        // Get the details and match the default paper size
        if (DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_BINS, (LPWSTR)bins.data(), NULL) == binCount
            && DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_BINNAMES, binNames.data(), NULL) == binCount) {

            for (int i = 0; i < int(binCount); ++i) {
                wchar_t *binName = binNames.data() + (i * 24);
                QString name = QString::fromWCharArray(binName, qwcsnlen(binName, 24));
                m_inputSlots.append(paperBinToInputSlot(bins[i], name));
            }

        }
    }

    m_haveInputSlots = true;
}

QPrint::InputSlot QWindowsPrintDevice::defaultInputSlot() const
{
    QPrint::InputSlot inputSlot = QPlatformPrintDevice::defaultInputSlot();;

    // Allocate the required DEVMODE buffer
    DWORD dmSize = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), NULL, NULL, 0);
    LPDEVMODE pDevMode = (LPDEVMODE)malloc(dmSize);

     // Get the default DevMode
    DWORD result = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), pDevMode, NULL, DM_OUT_BUFFER);

    // Get the default input slot
    if (result == IDOK && pDevMode->dmFields & DM_DEFAULTSOURCE) {
        QPrint::InputSlot tempSlot = paperBinToInputSlot(pDevMode->dmDefaultSource, QString());
        foreach (const QPrint::InputSlot &slot, supportedInputSlots()) {
            if (slot.key == tempSlot.key) {
                inputSlot = slot;
                break;
            }
        }
    }

    // Clean-up
    free(pDevMode);
    return inputSlot;
}

void QWindowsPrintDevice::loadOutputBins() const
{
    m_outputBins.append(QPlatformPrintDevice::defaultOutputBin());
    m_haveOutputBins = true;
}

void QWindowsPrintDevice::loadDuplexModes() const
{
    m_duplexModes.append(QPrint::DuplexNone);
    DWORD duplex = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_DUPLEX, NULL, NULL);
    if (int(duplex) == 1) {
        // TODO Assume if duplex flag supports both modes
        m_duplexModes.append(QPrint::DuplexLongSide);
        m_duplexModes.append(QPrint::DuplexShortSide);
    }
    m_haveDuplexModes = true;
}

QPrint::DuplexMode QWindowsPrintDevice::defaultDuplexMode() const
{
    QPrint::DuplexMode duplexMode = QPrint::DuplexNone;

    // Allocate the required DEVMODE buffer
    DWORD dmSize = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), NULL, NULL, 0);
    LPDEVMODE pDevMode = (LPDEVMODE)malloc(dmSize);

     // Get the default DevMode
    DWORD result = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), pDevMode, NULL, DM_OUT_BUFFER);

    // Get the default duplex mode
    if (result == IDOK && pDevMode->dmFields & DM_DUPLEX) {
        if (pDevMode->dmDuplex == DMDUP_VERTICAL)
            duplexMode = QPrint::DuplexLongSide;
        else if (pDevMode->dmDuplex == DMDUP_HORIZONTAL)
            duplexMode = QPrint::DuplexShortSide;
    }

    // Clean-up
    free(pDevMode);
    return duplexMode;
}

void QWindowsPrintDevice::loadColorModes() const
{
    m_colorModes.append(QPrint::GrayScale);
    DWORD color = DeviceCapabilities((LPWSTR)m_id.utf16(), NULL, DC_COLORDEVICE, NULL, NULL);
    if (int(color) == 1)
        m_colorModes.append(QPrint::Color);
    m_haveColorModes = true;
}

QPrint::ColorMode QWindowsPrintDevice::defaultColorMode() const
{
    if (!m_haveColorModes)
        loadColorModes();
    if (!m_colorModes.contains(QPrint::Color))
        return QPrint::GrayScale;

    QPrint::ColorMode colorMode = QPrint::GrayScale;

    // Allocate the required DEVMODE buffer
    DWORD dmSize = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), NULL, NULL, 0);
    LPDEVMODE pDevMode = (LPDEVMODE)malloc(dmSize);

     // Get the default DevMode
    DWORD result = DocumentProperties(NULL, m_hPrinter, (LPWSTR)m_id.utf16(), pDevMode, NULL, DM_OUT_BUFFER);

    // Get the default color mode
    if (result == IDOK && pDevMode->dmFields & DM_COLOR) {
        if (pDevMode->dmColor == DMCOLOR_COLOR)
            colorMode = QPrint::Color;
    }

    // Clean-up
    free(pDevMode);
    return colorMode;
}

QStringList QWindowsPrintDevice::availablePrintDeviceIds()
{
    QStringList list;
    DWORD needed = 0;
    DWORD returned = 0;
    if ((!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, 0, 0, &needed, &returned) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        || !needed) {
        return list;
    }
    QScopedArrayPointer<BYTE> buffer(new BYTE[needed]);
    if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, buffer.data(), needed, &needed, &returned))
        return list;
    PPRINTER_INFO_4 infoList = reinterpret_cast<PPRINTER_INFO_4>(buffer.data());
    for (uint i = 0; i < returned; ++i)
        list.append(QString::fromWCharArray(infoList[i].pPrinterName));
    return list;
}

QString QWindowsPrintDevice::defaultPrintDeviceId()
{
    DWORD size = 0;
    GetDefaultPrinter(NULL, &size);
    QScopedArrayPointer<wchar_t> name(new wchar_t[size]);
    GetDefaultPrinter(name.data(), &size);
    return QString::fromWCharArray(name.data());
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
