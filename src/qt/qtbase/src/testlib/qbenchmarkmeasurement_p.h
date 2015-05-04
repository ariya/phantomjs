/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QBENCHMARKMEASUREMENT_P_H
#define QBENCHMARKMEASUREMENT_P_H

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

#include <QtCore/qelapsedtimer.h>
#include <QtTest/private/cycle_p.h>
#include <QtTest/qbenchmark.h>

QT_BEGIN_NAMESPACE

class QBenchmarkMeasurerBase
{
public:
    virtual ~QBenchmarkMeasurerBase() {}
    virtual void init() {}
    virtual void start() = 0;
    virtual qint64 checkpoint() = 0;
    virtual qint64 stop() = 0;
    virtual bool isMeasurementAccepted(qint64 measurement) = 0;
    virtual int adjustIterationCount(int suggestion) = 0;
    virtual int adjustMedianCount(int suggestion) = 0;
    virtual bool repeatCount() { return 1; }
    virtual bool needsWarmupIteration() { return false; }
    virtual QTest::QBenchmarkMetric metricType() = 0;
};

class QBenchmarkTimeMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start();
    qint64 checkpoint();
    qint64 stop();
    bool isMeasurementAccepted(qint64 measurement);
    int adjustIterationCount(int sugestion);
    int adjustMedianCount(int suggestion);
    bool needsWarmupIteration();
    QTest::QBenchmarkMetric metricType();
private:
    QElapsedTimer time;
};

#ifdef HAVE_TICK_COUNTER // defined in 3rdparty/cycle_p.h

class QBenchmarkTickMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start();
    qint64 checkpoint();
    qint64 stop();
    bool isMeasurementAccepted(qint64 measurement);
    int adjustIterationCount(int);
    int adjustMedianCount(int suggestion);
    bool needsWarmupIteration();
    QTest::QBenchmarkMetric metricType();
private:
    CycleCounterTicks startTicks;
};

#endif

QT_END_NAMESPACE

#endif // QBENCHMARKMEASUREMENT_P_H
