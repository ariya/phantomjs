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

#ifndef QCOCOAPRINTDEVICE_H
#define QCOCOAPRINTDEVICE_H

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

#include "qt_mac_p.h"

#include <cups/ppd.h>

QT_BEGIN_NAMESPACE

class QCocoaPrintDevice : public QPlatformPrintDevice
{
public:
    QCocoaPrintDevice();
    explicit QCocoaPrintDevice(const QString &id);
    QCocoaPrintDevice(const QCocoaPrintDevice &other);
    virtual ~QCocoaPrintDevice();

    QCocoaPrintDevice *clone();

    bool operator==(const QCocoaPrintDevice &other) const;

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

    PMPrinter macPrinter() const;
    PMPaper macPaper(const QPageSize &pageSize) const;

protected:
    void loadPageSizes() const Q_DECL_OVERRIDE;
    void loadResolutions() const Q_DECL_OVERRIDE;
    void loadInputSlots() const Q_DECL_OVERRIDE;
    void loadOutputBins() const Q_DECL_OVERRIDE;
    void loadDuplexModes() const Q_DECL_OVERRIDE;
    void loadColorModes() const Q_DECL_OVERRIDE;
    void loadMimeTypes() const Q_DECL_OVERRIDE;

private:
    QPageSize createPageSize(const PMPaper &paper) const;
    bool openPpdFile();

    // Mac Core Printing
    PMPrinter m_printer;
    PMPrintSession m_session;
    mutable QHash<QString, PMPaper> m_macPapers;

    // PPD File
    ppd_file_t *m_ppd;

    QMarginsF m_customMargins;
    mutable QHash<QString, QMarginsF> m_printableMargins;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
#endif // QCOCOAPRINTDEVICE_H
