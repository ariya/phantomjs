/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
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

#include "qplatformprintdevice.h"

#include "qprintdevice_p.h"
#include "qprintdialog.h"

#include <QtGui/qpagelayout.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

QPlatformPrintDevice::QPlatformPrintDevice()
    : m_isRemote(false),
      m_supportsMultipleCopies(false),
      m_supportsCollateCopies(false),
      m_havePageSizes(false),
      m_supportsCustomPageSizes(false),
      m_haveResolutions(false),
      m_haveInputSlots(false),
      m_haveOutputBins(false),
      m_haveDuplexModes(false),
      m_haveColorModes(false)
{
}

QPlatformPrintDevice::QPlatformPrintDevice(const QString &id)
    : m_id(id),
      m_isRemote(false),
      m_supportsMultipleCopies(false),
      m_supportsCollateCopies(false),
      m_havePageSizes(false),
      m_supportsCustomPageSizes(false),
      m_haveResolutions(false),
      m_haveInputSlots(false),
      m_haveOutputBins(false),
      m_haveDuplexModes(false),
      m_haveColorModes(false)
{
}

QPlatformPrintDevice::~QPlatformPrintDevice()
{
}

bool QPlatformPrintDevice::operator==(const QPlatformPrintDevice &other) const
{
    return m_id == other.m_id;
}

QString QPlatformPrintDevice::id() const
{
    return m_id;
}

QString QPlatformPrintDevice::name() const
{
    return m_name;
}

QString QPlatformPrintDevice::location() const
{
    return m_location;
}

QString QPlatformPrintDevice::makeAndModel() const
{
    return m_makeAndModel;
}

bool QPlatformPrintDevice::isValid() const
{
    return false;
}

bool QPlatformPrintDevice::isDefault() const
{
    return false;
}

bool QPlatformPrintDevice::isRemote() const
{
    return m_isRemote;
}

bool QPlatformPrintDevice::isValidPageLayout(const QPageLayout &layout, int resolution) const
{
    // Check the page size is supported
    if (!supportedPageSize(layout.pageSize()).isValid())
        return false;

    // Check the margins are valid
    QMarginsF pointMargins = layout.margins(QPageLayout::Point);
    QMarginsF printMargins = printableMargins(layout.pageSize(), layout.orientation(), resolution);
    return pointMargins.left() >= printMargins.left()
           && pointMargins.right() >= printMargins.right()
           && pointMargins.top() >= printMargins.top()
           && pointMargins.bottom() >= printMargins.bottom();
}

QPrint::DeviceState QPlatformPrintDevice::state() const
{
    return QPrint::Error;
}

bool QPlatformPrintDevice::supportsMultipleCopies() const
{
    return m_supportsMultipleCopies;
}

bool QPlatformPrintDevice::supportsCollateCopies() const
{
    return m_supportsCollateCopies;
}

void QPlatformPrintDevice::loadPageSizes() const
{
}

QPageSize QPlatformPrintDevice::defaultPageSize() const
{
    return QPageSize();
}

QList<QPageSize> QPlatformPrintDevice::supportedPageSizes() const
{
    if (!m_havePageSizes)
        loadPageSizes();
    return m_pageSizes.toList();
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QPageSize &pageSize) const
{
    if (!pageSize.isValid())
        return QPageSize();

    if (!m_havePageSizes)
        loadPageSizes();

    // First try match on name and id for case where printer defines same size twice with different names
    // e.g. Windows defines DMPAPER_11X17 and DMPAPER_TABLOID with names "11x17" and "Tabloid", but both
    // map to QPageSize::Tabloid / PPD Key "Tabloid" / ANSI B Tabloid
    if (pageSize.id() != QPageSize::Custom) {
        foreach (const QPageSize &ps, m_pageSizes) {
            if (ps.id() == pageSize.id() && ps.name() == pageSize.name())
                return ps;
        }
    }

    // Next try match on id only if not custom
    if (pageSize.id() != QPageSize::Custom) {
        foreach (const QPageSize &ps, m_pageSizes) {
            if (ps.id() == pageSize.id())
                return ps;
        }
    }

    // Next try a match on size, in case it's a custom with a different name
    return supportedPageSizeMatch(pageSize);
}

QPageSize QPlatformPrintDevice::supportedPageSize(QPageSize::PageSizeId pageSizeId) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    foreach (const QPageSize &ps, m_pageSizes) {
        if (ps.id() == pageSizeId)
            return ps;
    }

    // If no supported page size found, try use a custom size instead if supported
    return supportedPageSizeMatch(QPageSize(pageSizeId));
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QString &pageName) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    foreach (const QPageSize &ps, m_pageSizes) {
        if (ps.name() == pageName)
            return ps;
    }

    return QPageSize();
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QSize &sizePoints) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    // Try to find a supported page size based on fuzzy-matched point size
    return supportedPageSizeMatch(QPageSize(sizePoints));
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QSizeF &size, QPageSize::Unit units) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    // Try to find a supported page size based on fuzzy-matched unit size
    return supportedPageSizeMatch(QPageSize(size, units));
}

QPageSize QPlatformPrintDevice::supportedPageSizeMatch(const QPageSize &pageSize) const
{
    // Try to find a supported page size based on point size
    foreach (const QPageSize &ps, m_pageSizes) {
        if (ps.sizePoints() == pageSize.sizePoints())
            return ps;
    }
    return QPageSize();
}

bool QPlatformPrintDevice::supportsCustomPageSizes() const
{
    return m_supportsCustomPageSizes;
}

QSize QPlatformPrintDevice::minimumPhysicalPageSize() const
{
    return m_minimumPhysicalPageSize;
}

QSize QPlatformPrintDevice::maximumPhysicalPageSize() const
{
    return m_maximumPhysicalPageSize;
}

QMarginsF QPlatformPrintDevice::printableMargins(const QPageSize &pageSize,
                                                 QPageLayout::Orientation orientation,
                                                 int resolution) const
{
    Q_UNUSED(pageSize)
    Q_UNUSED(orientation)
    Q_UNUSED(resolution)
    return QMarginsF(0, 0, 0, 0);
}

void QPlatformPrintDevice::loadResolutions() const
{
}

int QPlatformPrintDevice::defaultResolution() const
{
    return 0;
}

QList<int> QPlatformPrintDevice::supportedResolutions() const
{
    if (!m_haveResolutions)
        loadResolutions();
    return m_resolutions.toList();
}

void QPlatformPrintDevice::loadInputSlots() const
{
}

QPrint::InputSlot QPlatformPrintDevice::defaultInputSlot() const
{
    QPrint::InputSlot input;
    input.key = QByteArrayLiteral("Auto");
    input.name = QPrintDialog::tr("Automatic");
    input.id = QPrint::Auto;
    return input;
}

QList<QPrint::InputSlot> QPlatformPrintDevice::supportedInputSlots() const
{
    if (!m_haveInputSlots)
        loadInputSlots();
    return m_inputSlots.toList();
}

void QPlatformPrintDevice::loadOutputBins() const
{
}

QPrint::OutputBin QPlatformPrintDevice::defaultOutputBin() const
{
    QPrint::OutputBin output;
    output.key = QByteArrayLiteral("Auto");
    output.name = QPrintDialog::tr("Automatic");
    output.id = QPrint::AutoOutputBin;
    return output;
}

QList<QPrint::OutputBin> QPlatformPrintDevice::supportedOutputBins() const
{
    if (!m_haveOutputBins)
        loadOutputBins();
    return m_outputBins.toList();
}

void QPlatformPrintDevice::loadDuplexModes() const
{
}

QPrint::DuplexMode QPlatformPrintDevice::defaultDuplexMode() const
{
    return QPrint::DuplexNone;
}

QList<QPrint::DuplexMode> QPlatformPrintDevice::supportedDuplexModes() const
{
    if (!m_haveDuplexModes)
        loadDuplexModes();
    return m_duplexModes.toList();
}

void QPlatformPrintDevice::loadColorModes() const
{
}

QPrint::ColorMode QPlatformPrintDevice::defaultColorMode() const
{
    return QPrint::GrayScale;
}

QList<QPrint::ColorMode> QPlatformPrintDevice::supportedColorModes() const
{
    if (!m_haveColorModes)
        loadColorModes();
    return m_colorModes.toList();
}

void QPlatformPrintDevice::loadMimeTypes() const
{
}

QList<QMimeType> QPlatformPrintDevice::supportedMimeTypes() const
{
    if (!m_haveMimeTypes)
        loadMimeTypes();
    return m_mimeTypes.toList();
}

QPageSize QPlatformPrintDevice::createPageSize(const QString &key, const QSize &size, const QString &localizedName)
{
    return QPageSize(key, size, localizedName);
}

QPageSize QPlatformPrintDevice::createPageSize(int windowsId, const QSize &size, const QString &localizedName)
{
    return QPageSize(windowsId, size, localizedName);
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
