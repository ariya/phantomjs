/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qcocoaprintersupport.h"

#ifndef QT_NO_PRINTER

#include "qcocoaprintdevice.h"
#include "qprintengine_mac_p.h"

#include <private/qprinterinfo_p.h>

QCocoaPrinterSupport::QCocoaPrinterSupport()
{ }

QCocoaPrinterSupport::~QCocoaPrinterSupport()
{ }

QPrintEngine *QCocoaPrinterSupport::createNativePrintEngine(QPrinter::PrinterMode printerMode)
{
    return new QMacPrintEngine(printerMode);
}

QPaintEngine *QCocoaPrinterSupport::createPaintEngine(QPrintEngine *printEngine, QPrinter::PrinterMode printerMode)
{
    Q_UNUSED(printerMode);
    /*
        QMacPrintEngine multiply inherits from QPrintEngine and QPaintEngine,
        the cast here allows conversion of QMacPrintEngine* to QPaintEngine*
    */
    return static_cast<QMacPrintEngine *>(printEngine);
}

QPrintDevice QCocoaPrinterSupport::createPrintDevice(const QString &id)
{
    return QPlatformPrinterSupport::createPrintDevice(new QCocoaPrintDevice(id));
}

QStringList QCocoaPrinterSupport::availablePrintDeviceIds() const
{
    QStringList list;
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            list.append(QCFString::toQString(PMPrinterGetID(printer)));
        }
    }
    return list;
}

QString QCocoaPrinterSupport::defaultPrintDeviceId() const
{
    QCFType<CFArrayRef> printerList;
    if (PMServerCreatePrinterList(kPMServerLocal, &printerList) == noErr) {
        CFIndex count = CFArrayGetCount(printerList);
        for (CFIndex i = 0; i < count; ++i) {
            PMPrinter printer = static_cast<PMPrinter>(const_cast<void *>(CFArrayGetValueAtIndex(printerList, i)));
            if (PMPrinterIsDefault(printer))
                return QCFString::toQString(PMPrinterGetID(printer));
        }
    }
    return QString();
}

#endif  //QT_NO_PRINTER
