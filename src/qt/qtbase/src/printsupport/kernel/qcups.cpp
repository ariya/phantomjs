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

#include "qcups_p.h"

#include "qprintengine.h"

#ifndef QT_NO_CUPS

QT_BEGIN_NAMESPACE

QStringList QCUPSSupport::cupsOptionsList(QPrinter *printer)
{
    return printer->printEngine()->property(PPK_CupsOptions).toStringList();
}

void QCUPSSupport::setCupsOptions(QPrinter *printer, const QStringList &cupsOptions)
{
    printer->printEngine()->setProperty(PPK_CupsOptions, QVariant(cupsOptions));
}

void QCUPSSupport::setCupsOption(QStringList &cupsOptions, const QString &option, const QString &value)
{
    if (cupsOptions.contains(option)) {
        cupsOptions.replace(cupsOptions.indexOf(option) + 1, value);
    } else {
        cupsOptions.append(option);
        cupsOptions.append(value);
    }
}

void QCUPSSupport::setJobHold(QPrinter *printer, const JobHoldUntil jobHold, const QTime &holdUntilTime)
{
    QStringList cupsOptions = cupsOptionsList(printer);

    switch (jobHold) {
    case NoHold: //default
        break;
    case Indefinite:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("indefinite"));
        break;
    case DayTime:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("day-time"));
        break;
    case Night:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("night"));
        break;
    case SecondShift:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("second-shift"));
        break;
    case ThirdShift:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("third-shift"));
        break;
    case Weekend:
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      QStringLiteral("weekend"));
        break;
    case SpecificTime:
        if (holdUntilTime.isNull()) {
            setJobHold(printer, NoHold);
            return;
        }
        // CUPS expects the time in UTC, user has entered in local time, so get the UTS equivalent
        QDateTime localDateTime = QDateTime::currentDateTime();
        // Check if time is for tomorrow in case of DST change overnight
        if (holdUntilTime < localDateTime.time())
            localDateTime = localDateTime.addDays(1);
        localDateTime.setTime(holdUntilTime);
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      localDateTime.toUTC().time().toString(QStringLiteral("HH:mm")));
        break;
    }

    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setJobBilling(QPrinter *printer, const QString &jobBilling)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("job-billing"), jobBilling);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setJobPriority(QPrinter *printer, int priority)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("job-priority"), QString::number(priority));
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setBannerPages(QPrinter *printer, const BannerPage startBannerPage, const BannerPage endBannerPage)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    QString startBanner, endBanner;

    switch (startBannerPage) {
    case NoBanner:
        startBanner = QStringLiteral("none");
        break;
    case Standard:
        startBanner = QStringLiteral("standard");
        break;
    case Unclassified:
        startBanner = QStringLiteral("unclassified");
        break;
    case Confidential:
        startBanner = QStringLiteral("confidential");
        break;
    case Classified:
        startBanner = QStringLiteral("classified");
        break;
    case Secret:
        startBanner = QStringLiteral("secret");
        break;
    case TopSecret:
        startBanner = QStringLiteral("topsecret");
        break;
    }

    switch (endBannerPage) {
    case NoBanner:
        endBanner = QStringLiteral("none");
        break;
    case Standard:
        endBanner = QStringLiteral("standard");
        break;
    case Unclassified:
        endBanner = QStringLiteral("unclassified");
        break;
    case Confidential:
        endBanner = QStringLiteral("confidential");
        break;
    case Classified:
        endBanner = QStringLiteral("classified");
        break;
    case Secret:
        endBanner = QStringLiteral("secret");
        break;
    case TopSecret:
        endBanner = QStringLiteral("topsecret");
        break;
    }

    setCupsOption(cupsOptions, QStringLiteral("job-sheets"), startBanner + QLatin1Char(',') + endBanner);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPageSet(QPrinter *printer, const PageSet pageSet)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    QString pageSetString;

    switch (pageSet) {
    case OddPages:
        pageSetString = QStringLiteral("odd");
        break;
    case EvenPages:
        pageSetString = QStringLiteral("even");
        break;
    case AllPages:
        pageSetString = QStringLiteral("all");
        break;
    }

    setCupsOption(cupsOptions, QStringLiteral("page-set"), pageSetString);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPagesPerSheetLayout(QPrinter *printer,  const PagesPerSheet pagesPerSheet,
                                          const PagesPerSheetLayout pagesPerSheetLayout)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    static const char *pagesPerSheetData[] = { "1", "2", "4", "6", "9", "16", 0 };
    static const char *pageLayoutData[] = {"lrtb", "lrbt", "rlbt", "rltb", "btlr", "btrl", "tblr", "tbrl", 0};
    setCupsOption(cupsOptions, QStringLiteral("number-up"), QLatin1String(pagesPerSheetData[pagesPerSheet]));
    setCupsOption(cupsOptions, QStringLiteral("number-up-layout"), QLatin1String(pageLayoutData[pagesPerSheetLayout]));
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPageRange(QPrinter *printer, int pageFrom, int pageTo)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("page-ranges"), QStringLiteral("%1-%2").arg(pageFrom).arg(pageTo));
    setCupsOptions(printer, cupsOptions);
}

QT_END_NAMESPACE

#endif // QT_NO_CUPS
