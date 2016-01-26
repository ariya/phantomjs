/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 execjosh, http://execjosh.blogspot.com

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "childprocess.h"

//
// ChildProcessContext
//

ChildProcessContext::ChildProcessContext(QObject* parent)
    : QObject(parent)
    , m_proc(this)
{
    connect(&m_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(_readyReadStandardOutput()));
    connect(&m_proc, SIGNAL(readyReadStandardError()), this, SLOT(_readyReadStandardError()));
    connect(&m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(_finished(int, QProcess::ExitStatus)));
    connect(&m_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(_error(QProcess::ProcessError)));
}

ChildProcessContext::~ChildProcessContext()
{
}

// public:

qint64 ChildProcessContext::pid() const
{
    Q_PID pid = m_proc.pid();

#if !defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    return pid;
#else
    return pid->dwProcessId;
#endif
}

void ChildProcessContext::kill(const QString& signal)
{
    // TODO: it would be nice to be able to handle more signals
    if ("SIGKILL" == signal) {
        m_proc.kill();
    } else {
        // Default to "SIGTERM"
        m_proc.terminate();
    }
}

void ChildProcessContext::_setEncoding(const QString& encoding)
{
    m_encoding.setEncoding(encoding);
}

// This is affected by [QTBUG-5990](https://bugreports.qt-project.org/browse/QTBUG-5990).
// `QProcess` doesn't properly handle the situations of `cmd` not existing or
// failing to start...
bool ChildProcessContext::_start(const QString& cmd, const QStringList& args)
{
    m_proc.start(cmd, args);
    // TODO: Is there a better way to do this???
    return m_proc.waitForStarted(1000);
}

qint64 ChildProcessContext::_write(const QString &chunk, const QString &encoding)
{
    // Try to get codec for encoding
    QTextCodec *codec = QTextCodec::codecForName(encoding.toLatin1());

    // If unavailable, attempt UTF-8 codec
    if ((QTextCodec *)NULL == codec) {
        codec = QTextCodec::codecForName("UTF-8");

        // Don't even try to write if UTF-8 codec is unavailable
        if ((QTextCodec *)NULL == codec) {
            return -1;
        }
    }

    qint64 bytesWritten = m_proc.write(codec->fromUnicode(chunk));

    return bytesWritten;
}

void ChildProcessContext::_close()
{
    m_proc.closeWriteChannel();
}

// private slots:

void ChildProcessContext::_readyReadStandardOutput()
{
    QByteArray bytes = m_proc.readAllStandardOutput();
    emit stdoutData(m_encoding.decode(bytes));
}

void ChildProcessContext::_readyReadStandardError()
{
    QByteArray bytes = m_proc.readAllStandardError();
    emit stderrData(m_encoding.decode(bytes));
}

void ChildProcessContext::_finished(const int exitCode, const QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    emit exit(exitCode);
}

void ChildProcessContext::_error(const QProcess::ProcessError error)
{
    Q_UNUSED(error)

    emit exit(m_proc.exitCode());
}


//
// ChildProcess
//

ChildProcess::ChildProcess(QObject* parent)
    : QObject(parent)
{
}

ChildProcess::~ChildProcess()
{
}

// public:

QObject* ChildProcess::_createChildProcessContext()
{
    return new ChildProcessContext(this);
}
