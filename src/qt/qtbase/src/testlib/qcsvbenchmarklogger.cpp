/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include "qcsvbenchmarklogger_p.h"
#include "qtestresult_p.h"
#include "qbenchmark_p.h"

QCsvBenchmarkLogger::QCsvBenchmarkLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QCsvBenchmarkLogger::~QCsvBenchmarkLogger()
{
}

void QCsvBenchmarkLogger::startLogging()
{
    // don't print anything
}

void QCsvBenchmarkLogger::stopLogging()
{
    // don't print anything
}

void QCsvBenchmarkLogger::enterTestFunction(const char *)
{
    // don't print anything
}

void QCsvBenchmarkLogger::leaveTestFunction()
{
    // don't print anything
}

void QCsvBenchmarkLogger::addIncident(QAbstractTestLogger::IncidentTypes, const char *, const char *, int)
{
    // don't print anything
}

void QCsvBenchmarkLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    const char *fn = QTestResult::currentTestFunction() ? QTestResult::currentTestFunction()
        : "UnknownTestFunc";
    const char *tag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
    const char *gtag = QTestResult::currentGlobalDataTag()
                     ? QTestResult::currentGlobalDataTag()
                     : "";
    const char *filler = (tag[0] && gtag[0]) ? ":" : "";

    const char *metric = QTest::benchmarkMetricName(result.metric);

    char buf[1024];
    // "function","[globaltag:]tag","metric",value_per_iteration,total,iterations
    qsnprintf(buf, sizeof(buf), "\"%s\",\"%s%s%s\",\"%s\",%.13g,%.13g,%u\n",
              fn, gtag, filler, tag, metric,
              result.value / result.iterations, result.value, result.iterations);
    outputString(buf);
}

void QCsvBenchmarkLogger::addMessage(QAbstractTestLogger::MessageTypes, const QString &, const char *, int)
{
    // don't print anything
}
