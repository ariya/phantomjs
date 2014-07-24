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

#include "qprintdevice_p.h"
#include "qplatformprintdevice.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

QPrintDevice::QPrintDevice()
    : d(new QPlatformPrintDevice())
{
}

QPrintDevice::QPrintDevice(const QString &id)
    : d(new QPlatformPrintDevice(id))
{
}

QPrintDevice::QPrintDevice(QPlatformPrintDevice *dd)
    : d(dd)
{
}

QPrintDevice::QPrintDevice(const QPrintDevice &other)
    : d(other.d)
{
}

QPrintDevice::~QPrintDevice()
{
}

QPrintDevice &QPrintDevice::operator=(const QPrintDevice &other)
{
    d = other.d;
    return *this;
}

bool QPrintDevice::operator==(const QPrintDevice &other) const
{
    if (d && other.d)
        return *d == *other.d;
    return d == other.d;
}

QString QPrintDevice::id() const
{
    return isValid() ? d->id() : QString();
}

QString QPrintDevice::name() const
{
    return isValid() ? d->name() : QString();
}

QString QPrintDevice::location() const
{
    return isValid() ? d->location() : QString();
}

QString QPrintDevice::makeAndModel() const
{
    return isValid() ? d->makeAndModel() : QString();
}

bool QPrintDevice::isValid() const
{
    return d && d->isValid();
}

bool QPrintDevice::isDefault() const
{
    return isValid() && d->isDefault();
}

bool QPrintDevice::isRemote() const
{
    return isValid() && d->isRemote();
}

QPrint::DeviceState QPrintDevice::state() const
{
    return isValid() ? d->state() : QPrint::Error;
}

bool QPrintDevice::isValidPageLayout(const QPageLayout &layout, int resolution) const
{
    return isValid() && d->isValidPageLayout(layout, resolution);
}

bool QPrintDevice::supportsMultipleCopies() const
{
    return isValid() && d->supportsMultipleCopies();
}

bool QPrintDevice::supportsCollateCopies() const
{
    return isValid() && d->supportsCollateCopies();
}

QPageSize QPrintDevice::defaultPageSize() const
{
    return isValid() ? d->defaultPageSize() : QPageSize();
}

QList<QPageSize> QPrintDevice::supportedPageSizes() const
{
    return isValid() ? d->supportedPageSizes() : QList<QPageSize>();
}

QPageSize QPrintDevice::supportedPageSize(const QPageSize &pageSize) const
{
    return isValid() ? d->supportedPageSize(pageSize) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(QPageSize::PageSizeId pageSizeId) const
{
    return isValid() ? d->supportedPageSize(pageSizeId) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QString &pageName) const
{
    return isValid() ? d->supportedPageSize(pageName) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QSize &pointSize) const
{
    return isValid() ? d->supportedPageSize(pointSize) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QSizeF &size, QPageSize::Unit units) const
{
    return isValid() ? d->supportedPageSize(size, units) : QPageSize();
}

bool QPrintDevice::supportsCustomPageSizes() const
{
    return isValid() && d->supportsCustomPageSizes();
}

QSize QPrintDevice::minimumPhysicalPageSize() const
{
    return isValid() ? d->minimumPhysicalPageSize() : QSize();
}

QSize QPrintDevice::maximumPhysicalPageSize() const
{
    return isValid() ? d->maximumPhysicalPageSize() : QSize();
}

QMarginsF QPrintDevice::printableMargins(const QPageSize &pageSize,
                                         QPageLayout::Orientation orientation,
                                         int resolution) const
{
    return isValid() ? d->printableMargins(pageSize, orientation, resolution) : QMarginsF();
}

int QPrintDevice::defaultResolution() const
{
    return isValid() ? d->defaultResolution() : 0;
}

QList<int> QPrintDevice::supportedResolutions() const
{
    return isValid() ? d->supportedResolutions() : QList<int>();
}

QPrint::InputSlot QPrintDevice::defaultInputSlot() const
{
    return isValid() ? d->defaultInputSlot() : QPrint::InputSlot();
}

QList<QPrint::InputSlot> QPrintDevice::supportedInputSlots() const
{
    return isValid() ? d->supportedInputSlots() : QList<QPrint::InputSlot>();
}

QPrint::OutputBin QPrintDevice::defaultOutputBin() const
{
    return isValid() ? d->defaultOutputBin() : QPrint::OutputBin();
}

QList<QPrint::OutputBin> QPrintDevice::supportedOutputBins() const
{
    return isValid() ? d->supportedOutputBins() : QList<QPrint::OutputBin>();
}

QPrint::DuplexMode QPrintDevice::defaultDuplexMode() const
{
    return isValid() ? d->defaultDuplexMode() : QPrint::DuplexNone;
}

QList<QPrint::DuplexMode> QPrintDevice::supportedDuplexModes() const
{
    return isValid() ? d->supportedDuplexModes() : QList<QPrint::DuplexMode>();
}

QPrint::ColorMode QPrintDevice::defaultColorMode() const
{
    return isValid() ? d->defaultColorMode() : QPrint::GrayScale;
}

QList<QPrint::ColorMode> QPrintDevice::supportedColorModes() const
{
    return isValid() ? d->supportedColorModes() : QList<QPrint::ColorMode>();
}

QList<QMimeType> QPrintDevice::supportedMimeTypes() const
{
    return isValid() ? d->supportedMimeTypes() : QList<QMimeType>();
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
