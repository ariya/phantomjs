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

#ifndef QPPDPRINTDEVICE_H
#define QPPDPRINTDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformprintdevice.h>

#ifndef QT_NO_PRINTER

#include <cups/cups.h>
#include <cups/ppd.h>

QT_BEGIN_NAMESPACE

class QPpdPrintDevice : public QPlatformPrintDevice
{
public:
    QPpdPrintDevice();
    explicit QPpdPrintDevice(const QString &id);
    QPpdPrintDevice(const QPpdPrintDevice &other);
    virtual ~QPpdPrintDevice();

    QPpdPrintDevice &operator=(const QPpdPrintDevice &other);

    QPpdPrintDevice *clone();

    bool operator==(const QPpdPrintDevice &other) const;

    bool isValid() const Q_DECL_OVERRIDE;
    bool isDefault() const Q_DECL_OVERRIDE;

    QPrint::DeviceState state() const Q_DECL_OVERRIDE;

    QPageSize defaultPageSize() const Q_DECL_OVERRIDE;

    QMarginsF printableMargins(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                               int resolution) const Q_DECL_OVERRIDE;

    int defaultResolution() const Q_DECL_OVERRIDE;

    QPrint::InputSlot defaultInputSlot() const Q_DECL_OVERRIDE;

    QPrint::OutputBin defaultOutputBin() const Q_DECL_OVERRIDE;

    QPrint::DuplexMode defaultDuplexMode() const Q_DECL_OVERRIDE;

    QPrint::ColorMode defaultColorMode() const Q_DECL_OVERRIDE;

protected:
    void loadPageSizes() const Q_DECL_OVERRIDE;
    void loadResolutions() const Q_DECL_OVERRIDE;
    void loadInputSlots() const Q_DECL_OVERRIDE;
    void loadOutputBins() const Q_DECL_OVERRIDE;
    void loadDuplexModes() const Q_DECL_OVERRIDE;
    void loadColorModes() const Q_DECL_OVERRIDE;
    void loadMimeTypes() const Q_DECL_OVERRIDE;

private:
    void loadPrinter();
    QString printerOption(const QString &key) const;
    cups_ptype_e printerTypeFlags() const;

    cups_dest_t *m_cupsDest;
    ppd_file_t *m_ppd;
    QByteArray m_cupsName;
    QByteArray m_cupsInstance;
    QMarginsF m_customMargins;
    mutable QHash<QString, QMarginsF> m_printableMargins;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
#endif // QPPDPRINTDEVICE_H
