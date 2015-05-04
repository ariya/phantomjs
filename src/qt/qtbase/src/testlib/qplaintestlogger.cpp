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

#include <QtTest/private/qtestresult_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef Q_OS_WINCE
#include <QtCore/QString>
#endif

#ifdef min // windows.h without NOMINMAX is included by the benchmark headers.
#  undef min
#endif
#ifdef max
#  undef max
#endif

#include <QtCore/QByteArray>
#include <QtCore/qmath.h>
#include <QtCore/QLibraryInfo>

#ifdef Q_OS_ANDROID
#  include <android/log.h>
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

namespace QTest {

    static const char *incidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Pass:
            return "PASS   ";
        case QAbstractTestLogger::XFail:
            return "XFAIL  ";
        case QAbstractTestLogger::Fail:
            return "FAIL!  ";
        case QAbstractTestLogger::XPass:
            return "XPASS  ";
        case QAbstractTestLogger::BlacklistedPass:
            return "BPASS  ";
        case QAbstractTestLogger::BlacklistedFail:
            return "BFAIL  ";
        }
        return "??????";
    }

    static const char *benchmarkResult2String()
    {
        return "RESULT ";
    }

    static const char *messageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "SKIP   ";
        case QAbstractTestLogger::Warn:
            return "WARNING";
        case QAbstractTestLogger::QWarning:
            return "QWARN  ";
        case QAbstractTestLogger::QDebug:
            return "QDEBUG ";
        case QAbstractTestLogger::QSystem:
            return "QSYSTEM";
        case QAbstractTestLogger::QFatal:
            return "QFATAL ";
        case QAbstractTestLogger::Info:
            return "INFO   ";
        }
        return "??????";
    }

    template <typename T>
    static int countSignificantDigits(T num)
    {
        if (num <= 0)
            return 0;

        int digits = 0;
        qreal divisor = 1;

        while (num / divisor >= 1) {
            divisor *= 10;
            ++digits;
        }

        return digits;
    }

    // Pretty-prints a benchmark result using the given number of digits.
    template <typename T> QString formatResult(T number, int significantDigits)
    {
        if (number < T(0))
            return QLatin1String("NAN");
        if (number == T(0))
            return QLatin1String("0");

        QString beforeDecimalPoint = QString::number(qint64(number), 'f', 0);
        QString afterDecimalPoint = QString::number(number, 'f', 20);
        afterDecimalPoint.remove(0, beforeDecimalPoint.count() + 1);

        int beforeUse = qMin(beforeDecimalPoint.count(), significantDigits);
        int beforeRemove = beforeDecimalPoint.count() - beforeUse;

        // Replace insignificant digits before the decimal point with zeros.
        beforeDecimalPoint.chop(beforeRemove);
        for (int i = 0; i < beforeRemove; ++i) {
            beforeDecimalPoint.append(QLatin1Char('0'));
        }

        int afterUse = significantDigits - beforeUse;

        // leading zeroes after the decimal point does not count towards the digit use.
        if (beforeDecimalPoint == QLatin1String("0") && afterDecimalPoint.isEmpty() == false) {
            ++afterUse;

            int i = 0;
            while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == QLatin1Char('0')) {
                ++i;
            }

            afterUse += i;
        }

        int afterRemove = afterDecimalPoint.count() - afterUse;
        afterDecimalPoint.chop(afterRemove);

        QChar separator = QLatin1Char(',');
        QChar decimalPoint = QLatin1Char('.');

        // insert thousands separators
        int length = beforeDecimalPoint.length();
        for (int i = beforeDecimalPoint.length() -1; i >= 1; --i) {
            if ((length - i) % 3 == 0)
                beforeDecimalPoint.insert(i, separator);
        }

        QString print;
        print = beforeDecimalPoint;
        if (afterUse > 0)
            print.append(decimalPoint);

        print += afterDecimalPoint;


        return print;
    }

    template <typename T>
    int formatResult(char * buffer, int bufferSize, T number, int significantDigits)
    {
        QString result = formatResult(number, significantDigits);
        qstrncpy(buffer, result.toLatin1().constData(), bufferSize);
        int size = result.count();
        return size;
    }
}

#if defined(Q_OS_WIN)
Q_CORE_EXPORT bool qt_logging_to_console(); // defined in qlogging.cpp
#endif

void QPlainTestLogger::outputMessage(const char *str)
{
#if defined(Q_OS_WINCE)
    QString strUtf16 = QString::fromLocal8Bit(str);
    const int maxOutputLength = 255;
    do {
        QString tmp = strUtf16.left(maxOutputLength);
        OutputDebugString((wchar_t*)tmp.utf16());
        strUtf16.remove(0, maxOutputLength);
    } while (!strUtf16.isEmpty());
    if (stream != stdout)
#elif defined(Q_OS_WIN)
    // log to system log only if output is not redirected, and no console is attached
    if (!qt_logging_to_console() && stream == stdout) {
        OutputDebugStringA(str);
        return;
    }
#elif defined(Q_OS_ANDROID)
    __android_log_write(ANDROID_LOG_INFO, "QTestLib", str);
#endif
    outputString(str);
}

void QPlainTestLogger::printMessage(const char *type, const char *msg, const char *file, int line)
{
    QTEST_ASSERT(type);
    QTEST_ASSERT(msg);

    QTestCharBuffer buf;

    const char *fn = QTestResult::currentTestFunction() ? QTestResult::currentTestFunction()
        : "UnknownTestFunc";
    const char *tag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
    const char *gtag = QTestResult::currentGlobalDataTag()
                     ? QTestResult::currentGlobalDataTag()
                     : "";
    const char *filler = (tag[0] && gtag[0]) ? ":" : "";
    if (file) {
        QTest::qt_asprintf(&buf, "%s: %s::%s(%s%s%s)%s%s\n"
#ifdef Q_OS_WIN
                      "%s(%d) : failure location\n"
#else
                      "   Loc: [%s(%d)]\n"
#endif
                      , type, QTestResult::currentTestObjectName(), fn, gtag, filler, tag,
                      msg[0] ? " " : "", msg, file, line);
    } else {
        QTest::qt_asprintf(&buf, "%s: %s::%s(%s%s%s)%s%s\n",
                type, QTestResult::currentTestObjectName(), fn, gtag, filler, tag,
                msg[0] ? " " : "", msg);
    }
    // In colored mode, printf above stripped our nonprintable control characters.
    // Put them back.
    memcpy(buf.data(), type, strlen(type));
    outputMessage(buf.data());
}

void QPlainTestLogger::printBenchmarkResult(const QBenchmarkResult &result)
{
    const char *bmtag = QTest::benchmarkResult2String();

    char buf1[1024];
    qsnprintf(
        buf1, sizeof(buf1), "%s: %s::%s",
        bmtag,
        QTestResult::currentTestObjectName(),
        result.context.slotName.toLatin1().data());

    char bufTag[1024];
    bufTag[0] = 0;
    QByteArray tag = result.context.tag.toLocal8Bit();
    if (tag.isEmpty() == false) {
        qsnprintf(bufTag, sizeof(bufTag), ":\"%s\"", tag.data());
    }


    char fillFormat[8];
    int fillLength = 5;
    qsnprintf(fillFormat, sizeof(fillFormat), ":\n%%%ds", fillLength);
    char fill[1024];
    qsnprintf(fill, sizeof(fill), fillFormat, "");

    const char * unitText = QTest::benchmarkMetricUnit(result.metric);

    qreal valuePerIteration = qreal(result.value) / qreal(result.iterations);
    char resultBuffer[100] = "";
    QTest::formatResult(resultBuffer, 100, valuePerIteration, QTest::countSignificantDigits(result.value));

    char buf2[1024];
    qsnprintf(buf2, sizeof(buf2), "%s %s", resultBuffer, unitText);

    char buf2_[1024];
    QByteArray iterationText = " per iteration";
    Q_ASSERT(result.iterations > 0);
    qsnprintf(buf2_, sizeof(buf2_), "%s", iterationText.data());

    char buf3[1024];
    Q_ASSERT(result.iterations > 0);
    QTest::formatResult(resultBuffer, 100, result.value, QTest::countSignificantDigits(result.value));
    qsnprintf(buf3, sizeof(buf3), " (total: %s, iterations: %d)", resultBuffer, result.iterations);

    char buf[1024];

    if (result.setByMacro) {
        qsnprintf(buf, sizeof(buf), "%s%s%s%s%s%s\n", buf1, bufTag, fill, buf2, buf2_, buf3);
    } else {
        qsnprintf(buf, sizeof(buf), "%s%s%s%s\n", buf1, bufTag, fill, buf2);
    }

    memcpy(buf, bmtag, strlen(bmtag));
    outputMessage(buf);
}

QPlainTestLogger::QPlainTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QPlainTestLogger::~QPlainTestLogger()
{
}

void QPlainTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Testing %s\n", QTestResult::currentTestObjectName());
    } else {
        qsnprintf(buf, sizeof(buf),
                  "********* Start testing of %s *********\n"
                  "Config: Using QtTest library " QTEST_VERSION_STR
                  ", %s\n", QTestResult::currentTestObjectName(), QLibraryInfo::build());
    }
    outputMessage(buf);
}

void QPlainTestLogger::stopLogging()
{
    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Totals: %d passed, %d failed, %d skipped, %d blacklisted\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount());
    } else {
        qsnprintf(buf, sizeof(buf),
                  "Totals: %d passed, %d failed, %d skipped, %d blacklisted\n"
                  "********* Finished testing of %s *********\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount(),
                  QTestResult::currentTestObjectName());
    }
    outputMessage(buf);

    QAbstractTestLogger::stopLogging();
}


void QPlainTestLogger::enterTestFunction(const char * /*function*/)
{
    if (QTestLog::verboseLevel() >= 1)
        printMessage(QTest::messageType2String(Info), "entering");
}

void QPlainTestLogger::leaveTestFunction()
{
}

void QPlainTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    // suppress PASS and XFAIL in silent mode
    if ((type == QAbstractTestLogger::Pass || type == QAbstractTestLogger::XFail)
        && QTestLog::verboseLevel() < 0)
        return;

    printMessage(QTest::incidentType2String(type), description, file, line);
}

void QPlainTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    // suppress benchmark results in silent mode
    if (QTestLog::verboseLevel() < 0)
        return;

    printBenchmarkResult(result);
}

void QPlainTestLogger::addMessage(MessageTypes type, const QString &message,
                                  const char *file, int line)
{
    // suppress non-fatal messages in silent mode
    if (type != QAbstractTestLogger::QFatal && QTestLog::verboseLevel() < 0)
        return;

    printMessage(QTest::messageType2String(type), qPrintable(message), file, line);
}

QT_END_NAMESPACE
