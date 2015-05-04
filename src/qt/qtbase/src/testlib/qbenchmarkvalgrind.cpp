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

#include <QtTest/private/qbenchmark_p.h>

#ifdef QTESTLIB_USE_VALGRIND

#include <QtTest/private/qbenchmarkvalgrind_p.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdir.h>
#include <QtCore/qset.h>
#include <QtTest/private/callgrind_p.h>

QT_BEGIN_NAMESPACE

// Returns \c true if valgrind is available.
bool QBenchmarkValgrindUtils::haveValgrind()
{
#ifdef NVALGRIND
    return false;
#else
    QProcess process;
    process.start(QLatin1String("valgrind"), QStringList(QLatin1String("--version")));
    return process.waitForStarted() && process.waitForFinished(-1);
#endif
}

// Reruns this program through callgrind.
// Returns \c true upon success, otherwise false.
bool QBenchmarkValgrindUtils::rerunThroughCallgrind(const QStringList &origAppArgs, int &exitCode)
{
    if (!QBenchmarkValgrindUtils::runCallgrindSubProcess(origAppArgs, exitCode)) {
        qWarning("failed to run callgrind subprocess");
        return false;
    }
    return true;
}

static void dumpOutput(const QByteArray &data, FILE *fh)
{
    QFile file;
    file.open(fh, QIODevice::WriteOnly);
    file.write(data);
}

qint64 QBenchmarkValgrindUtils::extractResult(const QString &fileName)
{
    QFile file(fileName);
    const bool openOk = file.open(QIODevice::ReadOnly | QIODevice::Text);
    Q_ASSERT(openOk);
    Q_UNUSED(openOk);

    qint64 val = -1;
    bool valSeen = false;
    QRegExp rxValue(QLatin1String("^summary: (\\d+)"));
    while (!file.atEnd()) {
        const QString line(QLatin1String(file.readLine()));
        if (rxValue.indexIn(line) != -1) {
            Q_ASSERT(rxValue.captureCount() == 1);
            bool ok;
            val = rxValue.cap(1).toLongLong(&ok);
            Q_ASSERT(ok);
            valSeen = true;
            break;
        }
    }
    if (!valSeen)
        qFatal("Failed to extract result");
    return val;
}

// Gets the newest file name (i.e. the one with the highest integer suffix).
QString QBenchmarkValgrindUtils::getNewestFileName()
{
    QStringList nameFilters;
    QString base = QBenchmarkGlobalData::current->callgrindOutFileBase;
    Q_ASSERT(!base.isEmpty());

    nameFilters << QString::fromLatin1("%1.*").arg(base);
    QFileInfoList fiList = QDir().entryInfoList(nameFilters, QDir::Files | QDir::Readable);
    Q_ASSERT(!fiList.empty());
    int hiSuffix = -1;
    QFileInfo lastFileInfo;
    const QString pattern = QString::fromLatin1("%1.(\\d+)").arg(base);
    QRegExp rx(pattern);
    foreach (const QFileInfo &fileInfo, fiList) {
        const int index = rx.indexIn(fileInfo.fileName());
        Q_ASSERT(index == 0);
        Q_UNUSED(index);
        bool ok;
        const int suffix = rx.cap(1).toInt(&ok);
        Q_ASSERT(ok);
        Q_ASSERT(suffix >= 0);
        if (suffix > hiSuffix) {
            lastFileInfo = fileInfo;
            hiSuffix = suffix;
        }
    }

    return lastFileInfo.fileName();
}

qint64 QBenchmarkValgrindUtils::extractLastResult()
{
    return extractResult(getNewestFileName());
}

void QBenchmarkValgrindUtils::cleanup()
{
    QStringList nameFilters;
    QString base = QBenchmarkGlobalData::current->callgrindOutFileBase;
    Q_ASSERT(!base.isEmpty());
    nameFilters
        << base // overall summary
        << QString::fromLatin1("%1.*").arg(base); // individual dumps
    QFileInfoList fiList = QDir().entryInfoList(nameFilters, QDir::Files | QDir::Readable);
    foreach (const QFileInfo &fileInfo, fiList) {
        const bool removeOk = QFile::remove(fileInfo.fileName());
        Q_ASSERT(removeOk);
        Q_UNUSED(removeOk);
    }
}

QString QBenchmarkValgrindUtils::outFileBase(qint64 pid)
{
    return QString::fromLatin1("callgrind.out.%1").arg(
        pid != -1 ? pid : QCoreApplication::applicationPid());
}

// Reruns this program through callgrind, storing callgrind result files in the
// current directory.
// Returns \c true upon success, otherwise false.
bool QBenchmarkValgrindUtils::runCallgrindSubProcess(const QStringList &origAppArgs, int &exitCode)
{
    const QString execFile(origAppArgs.at(0));
    QStringList args;
    args << QLatin1String("--tool=callgrind") << QLatin1String("--instr-atstart=yes")
         << QLatin1String("--quiet")
         << execFile << QLatin1String("-callgrindchild");

    // pass on original arguments that make sense (e.g. avoid wasting time producing output
    // that will be ignored anyway) ...
    for (int i = 1; i < origAppArgs.size(); ++i) {
        const QString arg(origAppArgs.at(i));
        if (arg == QLatin1String("-callgrind"))
            continue;
        args << arg; // ok to pass on
    }

    QProcess process;
    process.start(QLatin1String("valgrind"), args);
    process.waitForStarted(-1);
    QBenchmarkGlobalData::current->callgrindOutFileBase =
        QBenchmarkValgrindUtils::outFileBase(process.pid());
    const bool finishedOk = process.waitForFinished(-1);
    exitCode = process.exitCode();

    dumpOutput(process.readAllStandardOutput(), stdout);
    dumpOutput(process.readAllStandardError(), stderr);

    return finishedOk;
}

void QBenchmarkCallgrindMeasurer::start()
{
    CALLGRIND_ZERO_STATS;
}

qint64 QBenchmarkCallgrindMeasurer::checkpoint()
{
    CALLGRIND_DUMP_STATS;
    const qint64 result = QBenchmarkValgrindUtils::extractLastResult();
    return result;
}

qint64 QBenchmarkCallgrindMeasurer::stop()
{
    return checkpoint();
}

bool QBenchmarkCallgrindMeasurer::isMeasurementAccepted(qint64 measurement)
{
    Q_UNUSED(measurement);
    return true;
}

int QBenchmarkCallgrindMeasurer::adjustIterationCount(int)
{
    return 1;
}

int QBenchmarkCallgrindMeasurer::adjustMedianCount(int)
{
    return 1;
}

bool QBenchmarkCallgrindMeasurer::needsWarmupIteration()
{
    return true;
}

QTest::QBenchmarkMetric QBenchmarkCallgrindMeasurer::metricType()
{
    return QTest::InstructionReads;
}

QT_END_NAMESPACE

#endif // QTESTLIB_USE_VALGRIND
