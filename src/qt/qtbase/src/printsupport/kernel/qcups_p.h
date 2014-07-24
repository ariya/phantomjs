/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QCUPS_P_H
#define QCUPS_P_H

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
#include "QtCore/qstring.h"
#include "QtCore/qstringlist.h"
#include "QtPrintSupport/qprinter.h"
#include "QtCore/qdatetime.h"

#ifndef QT_NO_CUPS

QT_BEGIN_NAMESPACE

// HACK! Define these here temporarily so they can be used in the dialogs
// without a circular reference to QCupsPrintEngine in the plugin.
// Move back to qcupsprintengine_p.h in the plugin once all usage
// removed from the dialogs.
#define PPK_CupsOptions QPrintEngine::PrintEnginePropertyKey(0xfe00)

class Q_PRINTSUPPORT_EXPORT QCUPSSupport
{
public:
    // Enum for values of job-hold-until option
    enum JobHoldUntil {
        NoHold = 0,  //CUPS Default
        Indefinite,
        DayTime,
        Night,
        SecondShift,
        ThirdShift,
        Weekend,
        SpecificTime
    };

    // Enum for valid banner pages
    enum BannerPage {
        NoBanner = 0,  //CUPS Default 'none'
        Standard,
        Unclassified,
        Confidential,
        Classified,
        Secret,
        TopSecret
    };

    // Enum for valid page set
    enum PageSet {
        AllPages = 0,  //CUPS Default
        OddPages,
        EvenPages
    };

    // Enum for valid number of pages per sheet
    enum PagesPerSheet {
        OnePagePerSheet = 0,
        TwoPagesPerSheet,
        FourPagesPerSheet,
        SixPagesPerSheet,
        NinePagesPerSheet,
        SixteenPagesPerSheet
    };

    // Enum for valid layouts of pages per sheet
    enum PagesPerSheetLayout {
        LeftToRightTopToBottom = 0,
        LeftToRightBottomToTop,
        RightToLeftTopToBottom,
        RightToLeftBottomToTop,
        BottomToTopLeftToRight,
        BottomToTopRightToLeft,
        TopToBottomLeftToRight,
        TopToBottomRightToLeft
    };

    static QStringList cupsOptionsList(QPrinter *printer);
    static void setCupsOptions(QPrinter *printer, const QStringList &cupsOptions);
    static void setCupsOption(QStringList &cupsOptions, const QString &option, const QString &value);

    static void setJobHold(QPrinter *printer, const JobHoldUntil jobHold = NoHold, const QTime &holdUntilTime = QTime());
    static void setJobBilling(QPrinter *printer, const QString &jobBilling = QString());
    static void setJobPriority(QPrinter *printer, int priority = 50);
    static void setBannerPages(QPrinter *printer, const BannerPage startBannerPage, const BannerPage endBannerPage);
    static void setPageSet(QPrinter *printer, const PageSet pageSet);
    static void setPagesPerSheetLayout(QPrinter *printer, const PagesPerSheet pagesPerSheet,
                                       const PagesPerSheetLayout pagesPerSheetLayout);
    static void setPageRange(QPrinter *printer, int pageFrom, int pageTo);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCUPSSupport::JobHoldUntil)
Q_DECLARE_METATYPE(QCUPSSupport::BannerPage)
Q_DECLARE_METATYPE(QCUPSSupport::PageSet)
Q_DECLARE_METATYPE(QCUPSSupport::PagesPerSheetLayout)
Q_DECLARE_METATYPE(QCUPSSupport::PagesPerSheet)

#endif // QT_NO_CUPS

#endif
