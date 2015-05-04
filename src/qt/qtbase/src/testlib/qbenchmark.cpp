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

#include <QtTest/qbenchmark.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

#include <QtCore/qprocess.h>
#include <QtCore/qdir.h>
#include <QtCore/qset.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QBenchmarkGlobalData *QBenchmarkGlobalData::current;

QBenchmarkGlobalData::QBenchmarkGlobalData()
    : measurer(0)
    , walltimeMinimum(-1)
    , iterationCount(-1)
    , medianIterationCount(-1)
    , createChart(false)
    , verboseOutput(false)
    , minimumTotal(-1)
    , mode_(WallTime)
{
    setMode(mode_);
}

QBenchmarkGlobalData::~QBenchmarkGlobalData()
{
    delete measurer;
    QBenchmarkGlobalData::current = 0;
}

void QBenchmarkGlobalData::setMode(Mode mode)
{
    mode_ = mode;

    if (measurer)
        delete measurer;
    measurer = createMeasurer();
}

QBenchmarkMeasurerBase * QBenchmarkGlobalData::createMeasurer()
{
    QBenchmarkMeasurerBase *measurer = 0;
    if (0) {
#ifdef QTESTLIB_USE_VALGRIND
    } else if (mode_ == CallgrindChildProcess || mode_ == CallgrindParentProcess) {
        measurer = new QBenchmarkCallgrindMeasurer;
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
    } else if (mode_ == PerfCounter) {
        measurer = new QBenchmarkPerfEventsMeasurer;
#endif
#ifdef HAVE_TICK_COUNTER
    } else if (mode_ == TickCounter) {
        measurer = new QBenchmarkTickMeasurer;
#endif
    } else if (mode_ == EventCounter) {
        measurer = new QBenchmarkEvent;
    } else {
        measurer =  new QBenchmarkTimeMeasurer;
    }
    measurer->init();
    return measurer;
}

int QBenchmarkGlobalData::adjustMedianIterationCount()
{
    if (medianIterationCount != -1) {
        return medianIterationCount;
    } else {
        return measurer->adjustMedianCount(1);
    }
}


QBenchmarkTestMethodData *QBenchmarkTestMethodData::current;

QBenchmarkTestMethodData::QBenchmarkTestMethodData()
:resultAccepted(false), runOnce(false), iterationCount(-1)
{
}

QBenchmarkTestMethodData::~QBenchmarkTestMethodData()
{
    QBenchmarkTestMethodData::current = 0;
}

void QBenchmarkTestMethodData::beginDataRun()
{
    iterationCount = adjustIterationCount(1);
}

void QBenchmarkTestMethodData::endDataRun()
{
}

int QBenchmarkTestMethodData::adjustIterationCount(int suggestion)
{
    // Let the -iterations option override the measurer.
    if (QBenchmarkGlobalData::current->iterationCount != -1) {
        iterationCount = QBenchmarkGlobalData::current->iterationCount;
    } else {
        iterationCount = QBenchmarkGlobalData::current->measurer->adjustIterationCount(suggestion);
    }

    return iterationCount;
}

void QBenchmarkTestMethodData::setResult(
    qreal value, QTest::QBenchmarkMetric metric, bool setByMacro)
{
    bool accepted = false;

    // Always accept the result if the iteration count has been
    // specified on the command line with -iterations.
    if (QBenchmarkGlobalData::current->iterationCount != -1)
        accepted = true;

    else if (QBenchmarkTestMethodData::current->runOnce || !setByMacro) {
        iterationCount = 1;
        accepted = true;
    }

    // Test the result directly without calling the measurer if the minimum time
    // has been specified on the command line with -minimumvalue.
    else if (QBenchmarkGlobalData::current->walltimeMinimum != -1)
        accepted = (value > QBenchmarkGlobalData::current->walltimeMinimum);
    else
        accepted = QBenchmarkGlobalData::current->measurer->isMeasurementAccepted(value);

    // Accept the result or double the number of iterations.
    if (accepted)
        resultAccepted = true;
    else
        iterationCount *= 2;

    this->result = QBenchmarkResult(
        QBenchmarkGlobalData::current->context, value, iterationCount, metric, setByMacro);
}

/*!
    \class QTest::QBenchmarkIterationController
    \internal

    The QBenchmarkIterationController class is used by the QBENCHMARK macro to
    drive the benchmarking loop. It is repsonsible for starting and stopping
    the timing measurements as well as calling the result reporting functions.
*/

/*! \internal
*/
QTest::QBenchmarkIterationController::QBenchmarkIterationController(RunMode runMode)
{
    i = 0;
    if (runMode == RunOnce)
        QBenchmarkTestMethodData::current->runOnce = true;
    QTest::beginBenchmarkMeasurement();
}

QTest::QBenchmarkIterationController::QBenchmarkIterationController()
{
    i = 0;
    QTest::beginBenchmarkMeasurement();
}

/*! \internal
*/
QTest::QBenchmarkIterationController::~QBenchmarkIterationController()
{
    const qreal result = QTest::endBenchmarkMeasurement();
    QBenchmarkTestMethodData::current->setResult(result, QBenchmarkGlobalData::current->measurer->metricType());
}

/*! \internal
*/
bool QTest::QBenchmarkIterationController::isDone()
{
    if (QBenchmarkTestMethodData::current->runOnce)
        return i > 0;
    return i >= QTest::iterationCount();
}

/*! \internal
*/
void QTest::QBenchmarkIterationController::next()
{
    ++i;
}

/*! \internal
*/
int QTest::iterationCount()
{
    return QBenchmarkTestMethodData::current->iterationCount;
}

/*! \internal
*/
void QTest::setIterationCountHint(int count)
{
    QBenchmarkTestMethodData::current->adjustIterationCount(count);
}

/*! \internal
*/
void QTest::setIterationCount(int count)
{
    QBenchmarkTestMethodData::current->iterationCount = count;
    QBenchmarkTestMethodData::current->resultAccepted = true;
}

/*! \internal
*/
void QTest::beginBenchmarkMeasurement()
{
    QBenchmarkGlobalData::current->measurer->start();
    // the clock is ticking after the line above, don't add code here.
}

/*! \internal
*/
quint64 QTest::endBenchmarkMeasurement()
{
    // the clock is ticking before the line below, don't add code here.
    return QBenchmarkGlobalData::current->measurer->stop();
}

/*!
    Sets the benchmark result for this test function to \a result.

    Use this function if you want to report benchmark results without
    using the QBENCHMARK macro. Use \a metric to specify how Qt Test
    should interpret the results.

    The context for the result will be the test function name and any
    data tag from the _data function. This function can only be called
    once in each test function, subsequent calls will replace the
    earlier reported results.

    Note that the -iterations command line argument has no effect
    on test functions without the QBENCHMARK macro.

    \since 4.7
*/
void QTest::setBenchmarkResult(qreal result, QTest::QBenchmarkMetric metric)
{
    QBenchmarkTestMethodData::current->setResult(result, metric, false);
}

template <typename T>
typename T::value_type qAverage(const T &container)
{
    typename T::const_iterator it = container.constBegin();
    typename T::const_iterator end = container.constEnd();
    typename T::value_type acc = typename T::value_type();
    int count = 0;
    while (it != end) {
        acc += *it;
        ++it;
        ++count;
    }
    return acc / count;
}

QT_END_NAMESPACE
