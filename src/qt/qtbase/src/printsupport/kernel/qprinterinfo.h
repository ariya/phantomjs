/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPRINTERINFO_H
#define QPRINTERINFO_H

#include <QtPrintSupport/qprinter.h>

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtGui/qpagesize.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_PRINTER
class QPrinterInfoPrivate;
class QPrinterInfoPrivateDeleter;
class Q_PRINTSUPPORT_EXPORT QPrinterInfo
{
public:
    QPrinterInfo();
    QPrinterInfo(const QPrinterInfo &other);
    explicit QPrinterInfo(const QPrinter &printer);
    ~QPrinterInfo();

    QPrinterInfo &operator=(const QPrinterInfo &other);

    QString printerName() const;
    QString description() const;
    QString location() const;
    QString makeAndModel() const;

    bool isNull() const;
    bool isDefault() const;
    bool isRemote() const;

    QPrinter::PrinterState state() const;

    QList<QPageSize> supportedPageSizes() const;
    QPageSize defaultPageSize() const;

    bool supportsCustomPageSizes() const;

    QPageSize minimumPhysicalPageSize() const;
    QPageSize maximumPhysicalPageSize() const;

#if QT_DEPRECATED_SINCE(5,3)
    QT_DEPRECATED QList<QPrinter::PaperSize> supportedPaperSizes() const;
    QT_DEPRECATED QList<QPair<QString, QSizeF> > supportedSizesWithNames() const;
#endif // QT_DEPRECATED_SINCE(5,3)

    QList<int> supportedResolutions() const;

    QPrinter::DuplexMode defaultDuplexMode() const;
    QList<QPrinter::DuplexMode> supportedDuplexModes() const;

    static QStringList availablePrinterNames();
    static QList<QPrinterInfo> availablePrinters();

    static QString defaultPrinterName();
    static QPrinterInfo defaultPrinter();

    static QPrinterInfo printerInfo(const QString &printerName);

private:
    explicit QPrinterInfo(const QString &name);

private:
    friend class QPlatformPrinterSupport;
    Q_DECLARE_PRIVATE(QPrinterInfo)
    QScopedPointer<QPrinterInfoPrivate, QPrinterInfoPrivateDeleter> d_ptr;
};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPRINTERINFO_H
