/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
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
#include "qtestblacklist_p.h"
#include "qtestresult_p.h"

#include <QtTest/qtestcase.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>

#include <set>

QT_BEGIN_NAMESPACE

/*
 The file format is simply a grouped listing of keywords
 Ungrouped entries at the beginning apply to the whole testcase
 Groups define testfunctions or specific test data to ignore.
 After the groups come a list of entries (one per line) that define
 for which platform/os combination to ignore the test result.
 All keys in a single line have to match to blacklist the test.

 mac
 [testFunction]
 linux
 windows 64bit
 [testfunction2:testData]
 msvc

 The known keys are listed below:
*/

// this table can be extended with new keywords as required
const char *matchedConditions[] =
{
    "*",
#ifdef Q_OS_LINUX
    "linux",
#endif
#ifdef Q_OS_OSX
    "osx",
#endif
#ifdef Q_OS_WIN
    "windows",
#endif
#ifdef Q_OS_IOS
    "ios",
#endif
#ifdef Q_OS_ANDROID
    "android",
#endif
#ifdef Q_OS_QNX
    "qnx",
#endif
#ifdef Q_OS_WINRT
    "winrt",
#endif
#ifdef Q_OS_WINCE
    "wince",
#endif

#if QT_POINTER_SIZE == 8
    "64bit",
#else
    "32bit",
#endif

#ifdef Q_CC_GNU
    "gcc",
#endif
#ifdef Q_CC_CLANG
    "clang",
#endif
#ifdef Q_CC_MSVC
    "msvc",
#endif

#ifdef Q_AUTOTEST_EXPORT
    "developer-build",
#endif
    0
};


static bool checkCondition(const QByteArray &condition)
{
    QList<QByteArray> conds = condition.split(' ');
    std::set<QByteArray> matches;
    const char **m = matchedConditions;
    while (*m) {
        matches.insert(*m);
        ++m;
    }

    for (int i = 0; i < conds.size(); ++i) {
        QByteArray c = conds.at(i);
        bool result = c.startsWith('!');
        if (result)
            c = c.mid(1);

        result ^= (matches.find(c) != matches.end());
        if (!result)
            return false;
    }
    return true;
}

static bool ignoreAll = false;
static std::set<QByteArray> *ignoredTests = 0;

namespace QTestPrivate {

void parseBlackList()
{
    QString filename = QTest::qFindTestData(QStringLiteral("BLACKLIST"));
    if (filename.isEmpty())
        return;
    QFile ignored(filename);
    if (!ignored.open(QIODevice::ReadOnly))
        return;

    QByteArray function;

    while (!ignored.atEnd()) {
        QByteArray line = ignored.readLine().simplified();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        if (line.startsWith('[')) {
            function = line.mid(1, line.length() - 2);
            continue;
        }
        bool condition = checkCondition(line);
        if (condition) {
            if (!function.size()) {
                ignoreAll = true;
            } else {
                if (!ignoredTests)
                    ignoredTests = new std::set<QByteArray>;
                ignoredTests->insert(function);
            }
        }
    }
}

void checkBlackList(const char *slot, const char *data)
{
    bool ignore = ignoreAll;

    if (!ignore && ignoredTests) {
        QByteArray s = slot;
        ignore = (ignoredTests->find(s) != ignoredTests->end());
        if (!ignore && data) {
            s += ':';
            s += data;
            ignore = (ignoredTests->find(s) != ignoredTests->end());
        }
    }

    QTestResult::setBlacklistCurrentTest(ignore);
}

}


QT_END_NAMESPACE
