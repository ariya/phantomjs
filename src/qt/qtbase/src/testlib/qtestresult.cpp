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
#include <QtCore/qglobal.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qtestassert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *currentAppName = 0;

QT_BEGIN_NAMESPACE

namespace QTest
{
    static QTestData *currentTestData = 0;
    static QTestData *currentGlobalTestData = 0;
    static const char *currentTestFunc = 0;
    static const char *currentTestObjectName = 0;
    static bool failed = false;
    static bool skipCurrentTest = false;
    static bool blacklistCurrentTest = false;

    static const char *expectFailComment = 0;
    static int expectFailMode = 0;
}

void QTestResult::reset()
{
    QTest::currentTestData = 0;
    QTest::currentGlobalTestData = 0;
    QTest::currentTestFunc = 0;
    QTest::currentTestObjectName = 0;
    QTest::failed = false;

    QTest::expectFailComment = 0;
    QTest::expectFailMode = 0;
    QTest::blacklistCurrentTest = false;

    QTestLog::resetCounters();
}

void QTestResult::setBlacklistCurrentTest(bool b)
{
    QTest::blacklistCurrentTest = b;
}

bool QTestResult::currentTestFailed()
{
    return QTest::failed;
}

QTestData *QTestResult::currentGlobalTestData()
{
    return QTest::currentGlobalTestData;
}

QTestData *QTestResult::currentTestData()
{
    return QTest::currentTestData;
}

void QTestResult::setCurrentGlobalTestData(QTestData *data)
{
    QTest::currentGlobalTestData = data;
}

void QTestResult::setCurrentTestData(QTestData *data)
{
    QTest::currentTestData = data;
    QTest::failed = false;
}

void QTestResult::setCurrentTestFunction(const char *func)
{
    QTest::currentTestFunc = func;
    QTest::failed = false;
    if (func)
        QTestLog::enterTestFunction(func);
}

static void clearExpectFail()
{
    QTest::expectFailMode = 0;
    delete [] const_cast<char *>(QTest::expectFailComment);
    QTest::expectFailComment = 0;
}

void QTestResult::finishedCurrentTestData()
{
    if (QTest::expectFailMode)
        addFailure("QEXPECT_FAIL was called without any subsequent verification statements", 0, 0);
    clearExpectFail();

    if (!QTest::failed && QTestLog::unhandledIgnoreMessages()) {
        QTestLog::printUnhandledIgnoreMessages();
        addFailure("Not all expected messages were received", 0, 0);
    }
    QTestLog::clearIgnoreMessages();
}

void QTestResult::finishedCurrentTestDataCleanup()
{
    // If the current test hasn't failed or been skipped, then it passes.
    if (!QTest::failed && !QTest::skipCurrentTest) {
        if (QTest::blacklistCurrentTest)
            QTestLog::addBPass("");
        else
            QTestLog::addPass("");
    }

    QTest::failed = false;
}

void QTestResult::finishedCurrentTestFunction()
{
    QTest::currentTestFunc = 0;
    QTest::failed = false;

    QTestLog::leaveTestFunction();
}

const char *QTestResult::currentTestFunction()
{
    return QTest::currentTestFunc;
}

const char *QTestResult::currentDataTag()
{
    return QTest::currentTestData ? QTest::currentTestData->dataTag()
                                   : static_cast<const char *>(0);
}

const char *QTestResult::currentGlobalDataTag()
{
    return QTest::currentGlobalTestData ? QTest::currentGlobalTestData->dataTag()
                                         : static_cast<const char *>(0);
}

static bool isExpectFailData(const char *dataIndex)
{
    if (!dataIndex || dataIndex[0] == '\0')
        return true;
    if (!QTest::currentTestData)
        return false;
    if (strcmp(dataIndex, QTest::currentTestData->dataTag()) == 0)
        return true;
    return false;
}

bool QTestResult::expectFail(const char *dataIndex, const char *comment,
                             QTest::TestFailMode mode, const char *file, int line)
{
    QTEST_ASSERT(comment);
    QTEST_ASSERT(mode > 0);

    if (!isExpectFailData(dataIndex)) {
        delete[] comment;
        return true; // we don't care
    }

    if (QTest::expectFailMode) {
        delete[] comment;
        clearExpectFail();
        addFailure("Already expecting a fail", file, line);
        return false;
    }

    QTest::expectFailMode = mode;
    QTest::expectFailComment = comment;
    return true;
}

static bool checkStatement(bool statement, const char *msg, const char *file, int line)
{
    if (statement) {
        if (QTest::expectFailMode) {
            QTestLog::addXPass(msg, file, line);
            bool doContinue = (QTest::expectFailMode == QTest::Continue);
            clearExpectFail();
            QTest::failed = true;
            return doContinue;
        }
        return true;
    }

    if (QTest::expectFailMode) {
        QTestLog::addXFail(QTest::expectFailComment, file, line);
        bool doContinue = (QTest::expectFailMode == QTest::Continue);
        clearExpectFail();
        return doContinue;
    }

    QTestResult::addFailure(msg, file, line);
    return false;
}

bool QTestResult::verify(bool statement, const char *statementStr,
                         const char *description, const char *file, int line)
{
    QTEST_ASSERT(statementStr);

    char msg[1024];

    if (QTestLog::verboseLevel() >= 2) {
        qsnprintf(msg, 1024, "QVERIFY(%s)", statementStr);
        QTestLog::info(msg, file, line);
    }

    const char * format = QTest::expectFailMode
        ? "'%s' returned TRUE unexpectedly. (%s)"
        : "'%s' returned FALSE. (%s)";
    qsnprintf(msg, 1024, format, statementStr, description ? description : "");

    return checkStatement(statement, msg, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          char *val1, char *val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    QTEST_ASSERT(expected);
    QTEST_ASSERT(actual);

    const size_t maxMsgLen = 1024;
    char msg[maxMsgLen];

    if (QTestLog::verboseLevel() >= 2) {
        qsnprintf(msg, maxMsgLen, "QCOMPARE(%s, %s)", actual, expected);
        QTestLog::info(msg, file, line);
    }

    if (!failureMsg)
        failureMsg = "Compared values are not the same";

    if (success && QTest::expectFailMode) {
        qsnprintf(msg, maxMsgLen,
                  "QCOMPARE(%s, %s) returned TRUE unexpectedly.", actual, expected);
    } else if (val1 || val2) {
        size_t len1 = mbstowcs(NULL, actual, maxMsgLen);    // Last parameter is not ignored on QNX
        size_t len2 = mbstowcs(NULL, expected, maxMsgLen);  // (result is never larger than this).
        qsnprintf(msg, maxMsgLen, "%s\n   Actual   (%s)%*s %s\n   Expected (%s)%*s %s",
                  failureMsg,
                  actual, qMax(len1, len2) - len1 + 1, ":", val1 ? val1 : "<null>",
                  expected, qMax(len1, len2) - len2 + 1, ":", val2 ? val2 : "<null>");
    } else
        qsnprintf(msg, maxMsgLen, "%s", failureMsg);

    delete [] val1;
    delete [] val2;

    return checkStatement(success, msg, file, line);
}

void QTestResult::addFailure(const char *message, const char *file, int line)
{
    clearExpectFail();

    if (QTest::blacklistCurrentTest)
        QTestLog::addBFail(message, file, line);
    else
        QTestLog::addFail(message, file, line);
    QTest::failed = true;
}

void QTestResult::addSkip(const char *message, const char *file, int line)
{
    clearExpectFail();

    QTestLog::addSkip(message, file, line);
}

void QTestResult::setCurrentTestObject(const char *name)
{
    QTest::currentTestObjectName = name;
}

const char *QTestResult::currentTestObjectName()
{
    return QTest::currentTestObjectName ? QTest::currentTestObjectName : "";
}

void QTestResult::setSkipCurrentTest(bool value)
{
    QTest::skipCurrentTest = value;
}

bool QTestResult::skipCurrentTest()
{
    return QTest::skipCurrentTest;
}

void QTestResult::setCurrentAppName(const char *appName)
{
    ::currentAppName = appName;
}

const char *QTestResult::currentAppName()
{
    return ::currentAppName;
}

QT_END_NAMESPACE
