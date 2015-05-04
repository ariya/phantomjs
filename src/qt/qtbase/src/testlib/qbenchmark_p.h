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

#ifndef QBENCHMARK_P_H
#define QBENCHMARK_P_H

#include <stdlib.h>

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

#include <QtCore/qglobal.h>

#if (defined(Q_OS_LINUX) || defined Q_OS_MAC) && !defined(QT_NO_PROCESS)
#define QTESTLIB_USE_VALGRIND
#else
#undef QTESTLIB_USE_VALGRIND
#endif

#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE) && !defined(Q_OS_ANDROID)
#define QTESTLIB_USE_PERF_EVENTS
#else
#undef QTESTLIB_USE_PERF_EVENTS
#endif

#include <QtTest/private/qbenchmarkmeasurement_p.h>
#include <QtCore/QMap>
#include <QtTest/qtest_global.h>
#ifdef QTESTLIB_USE_VALGRIND
#include <QtTest/private/qbenchmarkvalgrind_p.h>
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
#include <QtTest/private/qbenchmarkperfevents_p.h>
#endif
#include <QtTest/private/qbenchmarkevent_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

QT_BEGIN_NAMESPACE

struct QBenchmarkContext
{
    // None of the strings below are assumed to contain commas (see toString() below)
    QString slotName;
    QString tag; // from _data() function

    int checkpointIndex;

    QString toString() const
    {
        QString s = QString::fromLatin1("%1,%2,%3").arg(slotName).arg(tag).arg(checkpointIndex);
        return s;
    }

    QBenchmarkContext() : checkpointIndex(-1) {}
};

class QBenchmarkResult
{
public:
    QBenchmarkContext context;
    qreal value;
    int iterations;
    QTest::QBenchmarkMetric metric;
    bool setByMacro;
    bool valid;

    QBenchmarkResult()
    : value(-1)
    , iterations(-1)
    , setByMacro(true)
    , valid(false)
    { }

    QBenchmarkResult(
        const QBenchmarkContext &context, const qreal value, const int iterations,
        QTest::QBenchmarkMetric metric, bool setByMacro)
        : context(context)
        , value(value)
        , iterations(iterations)
        , metric(metric)
        , setByMacro(setByMacro)
        , valid(true)
    { }

    bool operator<(const QBenchmarkResult &other) const
    {
        return (value / iterations) < (other.value / other.iterations);
    }
};

/*
    The QBenchmarkGlobalData class stores global benchmark-related data.
    QBenchmarkGlobalData:current is created at the beginning of qExec()
    and cleared at the end.
*/
class Q_TESTLIB_EXPORT QBenchmarkGlobalData
{
public:
    static QBenchmarkGlobalData *current;

    QBenchmarkGlobalData();
    ~QBenchmarkGlobalData();
    enum Mode { WallTime, CallgrindParentProcess, CallgrindChildProcess, PerfCounter, TickCounter, EventCounter };
    void setMode(Mode mode);
    Mode mode() const { return mode_; }
    QBenchmarkMeasurerBase *createMeasurer();
    int adjustMedianIterationCount();

    QBenchmarkMeasurerBase *measurer;
    QBenchmarkContext context;
    int walltimeMinimum;
    int iterationCount;
    int medianIterationCount;
    bool createChart;
    bool verboseOutput;
    QString callgrindOutFileBase;
    int minimumTotal;
private:
    Mode mode_;
};

/*
    The QBenchmarkTestMethodData class stores all benchmark-related data
    for the current test case. QBenchmarkTestMethodData:current is
    created at the beginning of qInvokeTestMethod() and cleared at
    the end.
*/
class Q_TESTLIB_EXPORT QBenchmarkTestMethodData
{
public:
    static QBenchmarkTestMethodData *current;
    QBenchmarkTestMethodData();
    ~QBenchmarkTestMethodData();

    // Called once for each data row created by the _data function,
    // before and after calling the test function itself.
    void beginDataRun();
    void endDataRun();

    bool isBenchmark() const { return result.valid; }
    bool resultsAccepted() const { return resultAccepted; }
    int adjustIterationCount(int suggestion);
    void setResult(qreal value, QTest::QBenchmarkMetric metric, bool setByMacro = true);

    QBenchmarkResult result;
    bool resultAccepted;
    bool runOnce;
    int iterationCount;
};

// low-level API:
namespace QTest
{
    int iterationCount();
    void setIterationCountHint(int count);
    void setIterationCount(int count);

    Q_TESTLIB_EXPORT void beginBenchmarkMeasurement();
    Q_TESTLIB_EXPORT quint64 endBenchmarkMeasurement();
}

QT_END_NAMESPACE

#endif // QBENCHMARK_H
