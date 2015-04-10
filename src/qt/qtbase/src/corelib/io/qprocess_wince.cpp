/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qprocess.h"
#include "qprocess_p.h"
#include "qwindowspipewriter_p.h"

#include <qdir.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qwineventnotifier.h>
#include <qdebug.h>
#include <private/qthread_p.h>

#ifndef QT_NO_PROCESS

QT_BEGIN_NAMESPACE

//#define QPROCESS_DEBUG

void QProcessPrivate::destroyPipe(Q_PIPE pipe[2])
{
    Q_UNUSED(pipe);
}

void QProcessPrivate::destroyChannel(Channel *channel)
{
    Q_UNUSED(channel);
}

static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
    QString args;
    if (!program.isEmpty()) {
        QString programName = program;
        if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(QLatin1Char(' ')))
            programName = QLatin1Char('\"') + programName + QLatin1Char('\"');
        programName.replace(QLatin1Char('/'), QLatin1Char('\\'));

        // add the prgram as the first arg ... it works better
        args = programName + QLatin1Char(' ');
    }

    for (int i=0; i<arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // Quotes are escaped and their preceding backslashes are doubled.
        tmp.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\\1\\1\\\""));
        if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            int i = tmp.length();
            while (i > 0 && tmp.at(i - 1) == QLatin1Char('\\'))
                --i;
            tmp.insert(i, QLatin1Char('"'));
            tmp.prepend(QLatin1Char('"'));
        }
        args += QLatin1Char(' ') + tmp;
    }
    return args;
}

QProcessEnvironment QProcessEnvironment::systemEnvironment()
{
    QProcessEnvironment env;
    return env;
}

void QProcessPrivate::startProcess()
{
    Q_Q(QProcess);

    bool success = false;

    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    pid = new PROCESS_INFORMATION;
    memset(pid, 0, sizeof(PROCESS_INFORMATION));

    q->setProcessState(QProcess::Starting);

    QString args = qt_create_commandline(QString(), arguments);
    if (!nativeArguments.isEmpty()) {
        if (!args.isEmpty())
             args += QLatin1Char(' ');
        args += nativeArguments;
    }

#if defined QPROCESS_DEBUG
    qDebug("Creating process");
    qDebug("   program : [%s]", program.toLatin1().constData());
    qDebug("   args : %s", args.toLatin1().constData());
    qDebug("   pass environment : %s", environment.isEmpty() ? "no" : "yes");
#endif

    QString fullPathProgram = program;
    if (!QDir::isAbsolutePath(fullPathProgram))
        fullPathProgram = QFileInfo(fullPathProgram).absoluteFilePath();
    fullPathProgram.replace(QLatin1Char('/'), QLatin1Char('\\'));
    success = CreateProcess((wchar_t*)fullPathProgram.utf16(),
                            (wchar_t*)args.utf16(),
                            0, 0, false, 0, 0, 0, 0, pid);

    if (!success) {
        cleanup();
        processError = QProcess::FailedToStart;
        emit q->error(processError);
        q->setProcessState(QProcess::NotRunning);
        return;
    }

    q->setProcessState(QProcess::Running);
    // User can call kill()/terminate() from the stateChanged() slot
    // so check before proceeding
    if (!pid)
        return;

    if (threadData->hasEventDispatcher()) {
        processFinishedNotifier = new QWinEventNotifier(pid->hProcess, q);
        QObject::connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_processDied()));
        processFinishedNotifier->setEnabled(true);
    }

    // give the process a chance to start ...
    Sleep(SLEEPMIN * 2);
    _q_startupNotification();
}

bool QProcessPrivate::processStarted()
{
    return processState == QProcess::Running;
}

qint64 QProcessPrivate::bytesAvailableFromStdout() const
{
    return 0;
}

qint64 QProcessPrivate::bytesAvailableFromStderr() const
{
    return 0;
}

qint64 QProcessPrivate::readFromStdout(char *data, qint64 maxlen)
{
    return -1;
}

qint64 QProcessPrivate::readFromStderr(char *data, qint64 maxlen)
{
    return -1;
}

static BOOL QT_WIN_CALLBACK qt_terminateApp(HWND hwnd, LPARAM procId)
{
    DWORD currentProcId = 0;
    GetWindowThreadProcessId(hwnd, &currentProcId);
    if (currentProcId == (DWORD)procId)
        PostMessage(hwnd, WM_CLOSE, 0, 0);

    return TRUE;
}

void QProcessPrivate::terminateProcess()
{
    if (pid) {
        EnumWindows(qt_terminateApp, (LPARAM)pid->dwProcessId);
        PostThreadMessage(pid->dwThreadId, WM_CLOSE, 0, 0);
    }
}

void QProcessPrivate::killProcess()
{
    if (pid)
        TerminateProcess(pid->hProcess, 0xf291);
}

bool QProcessPrivate::waitForStarted(int)
{
    Q_Q(QProcess);

    if (processStarted())
        return true;

    if (processError == QProcess::FailedToStart)
        return false;

    processError = QProcess::Timedout;
    q->setErrorString(QProcess::tr("Process operation timed out"));
    return false;
}

bool QProcessPrivate::drainOutputPipes()
{
    return true;
}

bool QProcessPrivate::waitForReadyRead(int msecs)
{
    return false;
}

bool QProcessPrivate::waitForBytesWritten(int msecs)
{
    return false;
}

bool QProcessPrivate::waitForFinished(int msecs)
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::waitForFinished(%d)", msecs);
#endif

    QIncrementalSleepTimer timer(msecs);

    forever {
        if (!pid)
            return true;

        if (WaitForSingleObject(pid->hProcess, timer.nextSleepTime()) == WAIT_OBJECT_0) {
            _q_processDied();
            return true;
        }

        if (timer.hasTimedOut())
            break;
    }
    processError = QProcess::Timedout;
    q->setErrorString(QProcess::tr("Process operation timed out"));
    return false;
}

void QProcessPrivate::findExitCode()
{
    DWORD theExitCode;
    if (GetExitCodeProcess(pid->hProcess, &theExitCode)) {
        exitCode = theExitCode;
        //### for now we assume a crash if exit code is less than -1 or the magic number
        crashed = (exitCode == 0xf291 || (int)exitCode < 0);
    }
}

void QProcessPrivate::flushPipeWriter()
{
}

qint64 QProcessPrivate::pipeWriterBytesToWrite() const
{
    return 0;
}

qint64 QProcessPrivate::writeToStdin(const char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return -1;
}

bool QProcessPrivate::waitForWrite(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}

void QProcessPrivate::_q_notified()
{
}

bool QProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDir, qint64 *pid)
{
    Q_UNUSED(workingDir);
    QString args = qt_create_commandline(QString(), arguments);

    bool success = false;

    PROCESS_INFORMATION pinfo;

    QString fullPathProgram = program;
    if (!QDir::isAbsolutePath(fullPathProgram))
        fullPathProgram.prepend(QDir::currentPath().append(QLatin1Char('/')));
    fullPathProgram.replace(QLatin1Char('/'), QLatin1Char('\\'));
    success = CreateProcess((wchar_t*)fullPathProgram.utf16(),
                            (wchar_t*)args.utf16(),
                            0, 0, false, CREATE_NEW_CONSOLE, 0, 0, 0, &pinfo);

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
        if (pid)
            *pid = pinfo.dwProcessId;
    }

    return success;
}

QT_END_NAMESPACE

#endif // QT_NO_PROCESS
