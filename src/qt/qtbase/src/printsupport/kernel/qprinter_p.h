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

#ifndef QPRINTER_P_H
#define QPRINTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include "QtCore/qglobal.h"

#ifndef QT_NO_PRINTER

#include "QtPrintSupport/qprinter.h"
#include "QtPrintSupport/qprinterinfo.h"
#include "QtPrintSupport/qprintengine.h"
#include "QtCore/qpointer.h"
#include "QtCore/qset.h"

#include <limits.h>

QT_BEGIN_NAMESPACE

class QPrintEngine;
class QPreviewPaintEngine;
class QPicture;

class Q_PRINTSUPPORT_EXPORT QPrinterPrivate
{
    Q_DECLARE_PUBLIC(QPrinter)
public:
    QPrinterPrivate(QPrinter *printer)
        : printEngine(0),
          paintEngine(0),
          realPrintEngine(0),
          realPaintEngine(0),
#ifndef QT_NO_PRINTPREVIEWWIDGET
          previewEngine(0),
#endif
          q_ptr(printer),
          printRange(QPrinter::AllPages),
          use_default_engine(true),
          validPrinter(false)
    {
    }

    ~QPrinterPrivate() {

    }

    void init(const QPrinterInfo &printer, QPrinter::PrinterMode mode);

    QPrinterInfo findValidPrinter(const QPrinterInfo &printer = QPrinterInfo());
    void initEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer);
    void changeEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer);
#ifndef QT_NO_PRINTPREVIEWWIDGET
    QList<const QPicture *> previewPages() const;
    void setPreviewMode(bool);
#endif

    void setProperty(QPrintEngine::PrintEnginePropertyKey key, const QVariant &value);

    QPrinter::PrinterMode printerMode;
    QPrinter::OutputFormat outputFormat;
    QPrintEngine *printEngine;
    QPaintEngine *paintEngine;

    QPrintEngine *realPrintEngine;
    QPaintEngine *realPaintEngine;
#ifndef QT_NO_PRINTPREVIEWWIDGET
    QPreviewPaintEngine *previewEngine;
#endif

    QPrinter *q_ptr;

    QPrinter::PrintRange printRange;

    uint use_default_engine : 1;
    uint had_default_engines : 1;

    uint validPrinter : 1;
    uint hasCustomPageMargins : 1;

    // Used to remember which properties have been manually set by the user.
    QSet<QPrintEngine::PrintEnginePropertyKey> m_properties;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTER_P_H
