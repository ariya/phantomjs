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
#include "qwindowspipereader_p.h"
#include "qwindowspipewriter_p.h"

#include <qdatetime.h>
#include <qdir.h>
#include <qelapsedtimer.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qwineventnotifier.h>
#include <private/qthread_p.h>
#include <qdebug.h>

#include "private/qfsfileengine_p.h" // for longFileName

#ifndef PIPE_REJECT_REMOTE_CLIENTS
#define PIPE_REJECT_REMOTE_CLIENTS 0x08
#endif

#ifndef QT_NO_PROCESS

QT_BEGIN_NAMESPACE

//#define QPROCESS_DEBUG

#define NOTIFYTIMEOUT 100

static void qt_create_pipe(Q_PIPE *pipe, bool isInputPipe)
{
    // Anomymous pipes do not support asynchronous I/O. Thus we
    // create named pipes for redirecting stdout, stderr and stdin.

    // The write handle must be non-inheritable for input pipes.
    // The read handle must be non-inheritable for output pipes.
    SECURITY_ATTRIBUTES secAtt = { sizeof(SECURITY_ATTRIBUTES), 0, false };

    HANDLE hServer;
    wchar_t pipeName[256];
    unsigned int attempts = 1000;
    forever {
        // ### The user must make sure to call qsrand() to make the pipe names less predictable.
        // ### Replace the call to qrand() with a secure version, once we have it in Qt.
        _snwprintf(pipeName, sizeof(pipeName) / sizeof(pipeName[0]),
                L"\\\\.\\pipe\\qt-%X", qrand());

        DWORD dwOpenMode = FILE_FLAG_OVERLAPPED;
        DWORD dwOutputBufferSize = 0;
        DWORD dwInputBufferSize = 0;
        const DWORD dwPipeBufferSize = 1024 * 1024;
        if (isInputPipe) {
            dwOpenMode |= PIPE_ACCESS_OUTBOUND;
            dwOutputBufferSize = dwPipeBufferSize;
        } else {
            dwOpenMode |= PIPE_ACCESS_INBOUND;
            dwInputBufferSize = dwPipeBufferSize;
        }
        DWORD dwPipeFlags = PIPE_TYPE_BYTE | PIPE_WAIT;
        if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
            dwPipeFlags |= PIPE_REJECT_REMOTE_CLIENTS;
        hServer = CreateNamedPipe(pipeName,
                                  dwOpenMode,
                                  dwPipeFlags,
                                  1,                      // only one pipe instance
                                  dwOutputBufferSize,
                                  dwInputBufferSize,
                                  0,
                                  &secAtt);
        if (hServer != INVALID_HANDLE_VALUE)
            break;
        DWORD dwError = GetLastError();
        if (dwError != ERROR_PIPE_BUSY || !--attempts) {
            qErrnoWarning(dwError, "QProcess: CreateNamedPipe failed.");
            return;
        }
    }

    secAtt.bInheritHandle = TRUE;
    const HANDLE hClient = CreateFile(pipeName,
                                      (isInputPipe ? (GENERIC_READ | FILE_WRITE_ATTRIBUTES)
                                                   : GENERIC_WRITE),
                                      0,
                                      &secAtt,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);
    if (hClient == INVALID_HANDLE_VALUE) {
        qErrnoWarning("QProcess: CreateFile failed.");
        CloseHandle(hServer);
        return;
    }

    // Wait until connection is in place.
    ConnectNamedPipe(hServer, NULL);

    if (isInputPipe) {
        pipe[0] = hClient;
        pipe[1] = hServer;
    } else {
        pipe[0] = hServer;
        pipe[1] = hClient;
    }
}

static void duplicateStdWriteChannel(Q_PIPE *pipe, DWORD nStdHandle)
{
    pipe[0] = INVALID_Q_PIPE;
    HANDLE hStdWriteChannel = GetStdHandle(nStdHandle);
    HANDLE hCurrentProcess = GetCurrentProcess();
    DuplicateHandle(hCurrentProcess, hStdWriteChannel, hCurrentProcess,
                    &pipe[1], 0, TRUE, DUPLICATE_SAME_ACCESS);
}

/*
    Create the pipes to a QProcessPrivate::Channel.

    This function must be called in order: stdin, stdout, stderr
*/
bool QProcessPrivate::createChannel(Channel &channel)
{
    Q_Q(QProcess);

    if (&channel == &stderrChannel && processChannelMode == QProcess::MergedChannels) {
        return DuplicateHandle(GetCurrentProcess(), stdoutChannel.pipe[1], GetCurrentProcess(),
                               &stderrChannel.pipe[1], 0, TRUE, DUPLICATE_SAME_ACCESS);
    }

    if (channel.type == Channel::Normal) {
        // we're piping this channel to our own process
        if (&channel == &stdinChannel) {
            if (inputChannelMode != QProcess::ForwardedInputChannel) {
                qt_create_pipe(channel.pipe, true);
            } else {
                channel.pipe[1] = INVALID_Q_PIPE;
                HANDLE hStdReadChannel = GetStdHandle(STD_INPUT_HANDLE);
                HANDLE hCurrentProcess = GetCurrentProcess();
                DuplicateHandle(hCurrentProcess, hStdReadChannel, hCurrentProcess,
                                &channel.pipe[0], 0, TRUE, DUPLICATE_SAME_ACCESS);
            }
        } else {
            QWindowsPipeReader *pipeReader = 0;
            if (&channel == &stdoutChannel) {
                if (processChannelMode != QProcess::ForwardedChannels
                        && processChannelMode != QProcess::ForwardedOutputChannel) {
                    if (!stdoutReader) {
                        stdoutReader = new QWindowsPipeReader(q);
                        q->connect(stdoutReader, SIGNAL(readyRead()), SLOT(_q_canReadStandardOutput()));
                    }
                    pipeReader = stdoutReader;
                } else {
                    duplicateStdWriteChannel(channel.pipe, STD_OUTPUT_HANDLE);
                }
            } else /* if (&channel == &stderrChannel) */ {
                if (processChannelMode != QProcess::ForwardedChannels
                        && processChannelMode != QProcess::ForwardedErrorChannel) {
                    if (!stderrReader) {
                        stderrReader = new QWindowsPipeReader(q);
                        q->connect(stderrReader, SIGNAL(readyRead()), SLOT(_q_canReadStandardError()));
                    }
                    pipeReader = stderrReader;
                } else {
                    duplicateStdWriteChannel(channel.pipe, STD_ERROR_HANDLE);
                }
            }
            if (pipeReader) {
                qt_create_pipe(channel.pipe, false);
                pipeReader->setHandle(channel.pipe[0]);
                pipeReader->startAsyncRead();
            }
        }

        return true;
    } else if (channel.type == Channel::Redirect) {
        // we're redirecting the channel to/from a file
        SECURITY_ATTRIBUTES secAtt = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

        if (&channel == &stdinChannel) {
            // try to open in read-only mode
            channel.pipe[1] = INVALID_Q_PIPE;
            channel.pipe[0] =
                CreateFile((const wchar_t*)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &secAtt,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

            if (channel.pipe[0] != INVALID_Q_PIPE)
                return true;

            q->setErrorString(QProcess::tr("Could not open input redirection for reading"));
        } else {
            // open in write mode
            channel.pipe[0] = INVALID_Q_PIPE;
            channel.pipe[1] =
                CreateFile((const wchar_t *)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &secAtt,
                           channel.append ? OPEN_ALWAYS : CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

            if (channel.pipe[1] != INVALID_Q_PIPE) {
                if (channel.append) {
                    SetFilePointer(channel.pipe[1], 0, NULL, FILE_END);
                }
                return true;
            }

            q->setErrorString(QProcess::tr("Could not open output redirection for writing"));
        }

        // could not open file
        processError = QProcess::FailedToStart;
        emit q->error(processError);
        cleanup();
        return false;
    } else {
        Q_ASSERT_X(channel.process, "QProcess::start", "Internal error");

        Channel *source;
        Channel *sink;

        if (channel.type == Channel::PipeSource) {
            // we are the source
            source = &channel;
            sink = &channel.process->stdinChannel;

            if (source->pipe[1] != INVALID_Q_PIPE) {
                // already constructed by the sink
                // make it inheritable
                HANDLE tmpHandle = source->pipe[1];
                if (!DuplicateHandle(GetCurrentProcess(), tmpHandle,
                                     GetCurrentProcess(), &source->pipe[1],
                                     0, TRUE, DUPLICATE_SAME_ACCESS))
                    return false;

                CloseHandle(tmpHandle);
                return true;
            }

            Q_ASSERT(source == &stdoutChannel);
            Q_ASSERT(sink->process == this && sink->type == Channel::PipeSink);

            qt_create_pipe(source->pipe, /* in = */ false); // source is stdout
            sink->pipe[0] = source->pipe[0];
            source->pipe[0] = INVALID_Q_PIPE;

            return true;
        } else {
            // we are the sink;
            source = &channel.process->stdoutChannel;
            sink = &channel;

            if (sink->pipe[0] != INVALID_Q_PIPE) {
                // already constructed by the source
                // make it inheritable
                HANDLE tmpHandle = sink->pipe[0];
                if (!DuplicateHandle(GetCurrentProcess(), tmpHandle,
                                     GetCurrentProcess(), &sink->pipe[0],
                                     0, TRUE, DUPLICATE_SAME_ACCESS))
                    return false;

                CloseHandle(tmpHandle);
                return true;
            }
            Q_ASSERT(sink == &stdinChannel);
            Q_ASSERT(source->process == this && source->type == Channel::PipeSource);

            qt_create_pipe(sink->pipe, /* in = */ true); // sink is stdin
            source->pipe[1] = sink->pipe[1];
            sink->pipe[1] = INVALID_Q_PIPE;

            return true;
        }
    }
}

void QProcessPrivate::destroyPipe(Q_PIPE pipe[2])
{
    if (pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(pipe[0]);
        pipe[0] = INVALID_Q_PIPE;
    }
    if (pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(pipe[1]);
        pipe[1] = INVALID_Q_PIPE;
    }
}

void QProcessPrivate::destroyChannel(Channel *channel)
{
    if (channel == &stdinChannel) {
        if (pipeWriter) {
            delete pipeWriter;
            pipeWriter = 0;
        }
    } else if (channel == &stdoutChannel) {
        if (stdoutReader) {
            stdoutReader->stop();
            stdoutReader->deleteLater();
            stdoutReader = 0;
        }
    } else if (channel == &stderrChannel) {
        if (stderrReader) {
            stderrReader->stop();
            stderrReader->deleteLater();
            stderrReader = 0;
        }
    }
    destroyPipe(channel->pipe);
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
    // Calls to setenv() affect the low-level environment as well.
    // This is not the case the other way round.
    if (wchar_t *envStrings = GetEnvironmentStringsW()) {
        for (const wchar_t *entry = envStrings; *entry; ) {
            const int entryLen = int(wcslen(entry));
            // + 1 to permit magic cmd variable names starting with =
            if (const wchar_t *equal = wcschr(entry + 1, L'=')) {
                int nameLen = equal - entry;
                QString name = QString::fromWCharArray(entry, nameLen);
                QString value = QString::fromWCharArray(equal + 1, entryLen - nameLen - 1);
                env.d->hash.insert(QProcessEnvironmentPrivate::Key(name), value);
            }
            entry += entryLen + 1;
        }
        FreeEnvironmentStringsW(envStrings);
    }
    return env;
}

static QByteArray qt_create_environment(const QProcessEnvironmentPrivate::Hash &environment)
{
    QByteArray envlist;
    if (!environment.isEmpty()) {
        QProcessEnvironmentPrivate::Hash copy = environment;

        // add PATH if necessary (for DLL loading)
        QProcessEnvironmentPrivate::Key pathKey(QLatin1String("PATH"));
        if (!copy.contains(pathKey)) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                copy.insert(pathKey, QString::fromLocal8Bit(path));
        }

        // add systemroot if needed
        QProcessEnvironmentPrivate::Key rootKey(QLatin1String("SystemRoot"));
        if (!copy.contains(rootKey)) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                copy.insert(rootKey, QString::fromLocal8Bit(systemRoot));
        }

        int pos = 0;
        QProcessEnvironmentPrivate::Hash::ConstIterator it = copy.constBegin(),
                                                       end = copy.constEnd();

        static const wchar_t equal = L'=';
        static const wchar_t nul = L'\0';

        for ( ; it != end; ++it) {
            uint tmpSize = sizeof(wchar_t) * (it.key().length() + it.value().length() + 2);
            // ignore empty strings
            if (tmpSize == sizeof(wchar_t) * 2)
                continue;
            envlist.resize(envlist.size() + tmpSize);

            tmpSize = it.key().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.key().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &equal, sizeof(wchar_t));
            pos += sizeof(wchar_t);

            tmpSize = it.value().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.value().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &nul, sizeof(wchar_t));
            pos += sizeof(wchar_t);
        }
        // add the 2 terminating 0 (actually 4, just to be on the safe side)
        envlist.resize( envlist.size()+4 );
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
    }
    return envlist;
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

    if (!createChannel(stdinChannel) ||
        !createChannel(stdoutChannel) ||
        !createChannel(stderrChannel))
        return;

    QString args = qt_create_commandline(program, arguments);
    QByteArray envlist;
    if (environment.d.constData())
        envlist = qt_create_environment(environment.d.constData()->hash);
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

    // We cannot unconditionally set the CREATE_NO_WINDOW flag, because this
    // will render the stdout/stderr handles connected to a console useless
    // (this typically affects ForwardedChannels mode).
    // However, we also do not want console tools launched from a GUI app to
    // create new console windows (behavior consistent with UNIX).
    DWORD dwCreationFlags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
    dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 0, 0, 0,
                                 STARTF_USESTDHANDLES,
                                 0, 0, 0,
                                 stdinChannel.pipe[0], stdoutChannel.pipe[1], stderrChannel.pipe[1]
    };
    success = CreateProcess(0, (wchar_t*)args.utf16(),
                            0, 0, TRUE, dwCreationFlags,
                            environment.isEmpty() ? 0 : envlist.data(),
                            workingDirectory.isEmpty() ? 0 : (wchar_t*)QDir::toNativeSeparators(workingDirectory).utf16(),
                            &startupInfo, pid);
    if (!success) {
        // Capture the error string before we do CloseHandle below
        q->setErrorString(QProcess::tr("Process failed to start: %1").arg(qt_error_string()));
    }

    if (stdinChannel.pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(stdinChannel.pipe[0]);
        stdinChannel.pipe[0] = INVALID_Q_PIPE;
    }
    if (stdoutChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stdoutChannel.pipe[1]);
        stdoutChannel.pipe[1] = INVALID_Q_PIPE;
    }
    if (stderrChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stderrChannel.pipe[1]);
        stderrChannel.pipe[1] = INVALID_Q_PIPE;
    }

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
        notifier = new QTimer(q);
        QObject::connect(notifier, SIGNAL(timeout()), q, SLOT(_q_notified()));
        notifier->start(NOTIFYTIMEOUT);
    }

    _q_startupNotification();
}

bool QProcessPrivate::processStarted()
{
    return processState == QProcess::Running;
}

qint64 QProcessPrivate::bytesAvailableFromStdout() const
{
    if (stdoutChannel.pipe[0] == INVALID_Q_PIPE)
        return 0;

    if (!stdoutReader)
        return 0;

    DWORD bytesAvail = stdoutReader->bytesAvailable();
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::bytesAvailableFromStdout() == %d", bytesAvail);
#endif
    return bytesAvail;
}

qint64 QProcessPrivate::bytesAvailableFromStderr() const
{
    if (stderrChannel.pipe[0] == INVALID_Q_PIPE)
        return 0;

    if (!stderrReader)
        return 0;

    DWORD bytesAvail = stderrReader->bytesAvailable();
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::bytesAvailableFromStderr() == %d", bytesAvail);
#endif
    return bytesAvail;
}

qint64 QProcessPrivate::readFromStdout(char *data, qint64 maxlen)
{
    return stdoutReader ? stdoutReader->read(data, maxlen) : 0;
}

qint64 QProcessPrivate::readFromStderr(char *data, qint64 maxlen)
{
    return stderrReader ? stderrReader->read(data, maxlen) : 0;
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
    if (!stdoutReader && !stderrReader)
        return false;

    bool someReadyReadEmitted = false;
    forever {
        bool readyReadEmitted = false;
        bool readOperationActive = false;
        if (stdoutReader) {
            readyReadEmitted |= stdoutReader->waitForReadyRead(0);
            readOperationActive = stdoutReader && stdoutReader->isReadOperationActive();
        }
        if (stderrReader) {
            readyReadEmitted |= stderrReader->waitForReadyRead(0);
            readOperationActive |= stderrReader && stderrReader->isReadOperationActive();
        }
        someReadyReadEmitted |= readyReadEmitted;
        if (!readOperationActive || !readyReadEmitted)
            break;
        Sleep(100);
    }

    return someReadyReadEmitted;
}

bool QProcessPrivate::waitForReadyRead(int msecs)
{
    Q_Q(QProcess);

    QIncrementalSleepTimer timer(msecs);

    forever {
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;
        if (pipeWriter && pipeWriter->waitForWrite(0))
            timer.resetIncrements();

        if ((stdoutReader && stdoutReader->waitForReadyRead(0))
            || (stderrReader && stderrReader->waitForReadyRead(0)))
            return true;

        if (!pid)
            return false;
        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            bool readyReadEmitted = drainOutputPipes();
            _q_processDied();
            return readyReadEmitted;
        }

        Sleep(timer.nextSleepTime());
        if (timer.hasTimedOut())
            break;
    }

    processError = QProcess::Timedout;
    q->setErrorString(QProcess::tr("Process operation timed out"));
    return false;
}

bool QProcessPrivate::waitForBytesWritten(int msecs)
{
    Q_Q(QProcess);

    QIncrementalSleepTimer timer(msecs);

    forever {
        // Check if we have any data pending: the pipe writer has
        // bytes waiting to written, or it has written data since the
        // last time we called pipeWriter->waitForWrite().
        bool pendingDataInPipe = pipeWriter && (pipeWriter->bytesToWrite() || pipeWriter->hadWritten());

        // If we don't have pending data, and our write buffer is
        // empty, we fail.
        if (!pendingDataInPipe && writeBuffer.isEmpty())
            return false;

        // If we don't have pending data and we do have data in our
        // write buffer, try to flush that data over to the pipe
        // writer.  Fail on error.
        if (!pendingDataInPipe) {
            if (!_q_canWrite())
                return false;
        }

        // Wait for the pipe writer to acknowledge that it has
        // written. This will succeed if either the pipe writer has
        // already written the data, or if it manages to write data
        // within the given timeout. If the write buffer was non-empty
        // and the pipeWriter is now dead, that means _q_canWrite()
        // destroyed the writer after it successfully wrote the last
        // batch.
        if (!pipeWriter || pipeWriter->waitForWrite(0))
            return true;

        // If we wouldn't write anything, check if we can read stdout.
        if (bytesAvailableFromStdout() != 0) {
            _q_canReadStandardOutput();
            timer.resetIncrements();
        }

        // Check if we can read stderr.
        if (bytesAvailableFromStderr() != 0) {
            _q_canReadStandardError();
            timer.resetIncrements();
        }

        // Check if the process died while reading.
        if (!pid)
            return false;

        // Wait for the process to signal any change in its state,
        // such as incoming data, or if the process died.
        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            _q_processDied();
            return false;
        }

        // Only wait for as long as we've been asked.
        if (timer.hasTimedOut())
            break;
    }

    processError = QProcess::Timedout;
    q->setErrorString(QProcess::tr("Process operation timed out"));
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
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;
        if (pipeWriter && pipeWriter->waitForWrite(0))
            timer.resetIncrements();
        if (stdoutReader && stdoutReader->waitForReadyRead(0))
            timer.resetIncrements();
        if (stderrReader && stderrReader->waitForReadyRead(0))
            timer.resetIncrements();

        if (!pid) {
            drainOutputPipes();
            return true;
        }

        if (WaitForSingleObject(pid->hProcess, timer.nextSleepTime()) == WAIT_OBJECT_0) {
            drainOutputPipes();
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
        crashed = (exitCode == 0xf291   // our magic number, see killProcess
                   || (theExitCode >= 0x80000000 && theExitCode < 0xD0000000));
    }
}

void QProcessPrivate::flushPipeWriter()
{
    if (pipeWriter && pipeWriter->bytesToWrite() > 0) {
        pipeWriter->waitForWrite(ULONG_MAX);
    }
}

qint64 QProcessPrivate::pipeWriterBytesToWrite() const
{
    return pipeWriter ? pipeWriter->bytesToWrite() : qint64(0);
}

qint64 QProcessPrivate::writeToStdin(const char *data, qint64 maxlen)
{
    Q_Q(QProcess);

    if (!pipeWriter) {
        pipeWriter = new QWindowsPipeWriter(stdinChannel.pipe[1], q);
        pipeWriter->start();
    }

    return pipeWriter->write(data, maxlen);
}

bool QProcessPrivate::waitForWrite(int msecs)
{
    Q_Q(QProcess);

    if (!pipeWriter || pipeWriter->waitForWrite(msecs))
        return true;

    processError = QProcess::Timedout;
    q->setErrorString(QProcess::tr("Process operation timed out"));
    return false;
}

void QProcessPrivate::_q_notified()
{
    notifier->stop();

    if (!writeBuffer.isEmpty() && (!pipeWriter || pipeWriter->waitForWrite(0)))
        _q_canWrite();

    if (processState != QProcess::NotRunning)
        notifier->start(NOTIFYTIMEOUT);
}

bool QProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDir, qint64 *pid)
{
    QString args = qt_create_commandline(program, arguments);
    bool success = false;
    PROCESS_INFORMATION pinfo;

    STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                               };
    success = CreateProcess(0, (wchar_t*)args.utf16(),
                            0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE, 0,
                            workingDir.isEmpty() ? 0 : (wchar_t*)workingDir.utf16(),
                            &startupInfo, &pinfo);

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
