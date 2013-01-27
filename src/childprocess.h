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

#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include <QObject>
#include <QProcess>

#ifdef Q_OS_WIN32
#include <QtCore/qt_windows.h>
#endif

#include "encoding.h"

/**
 * This class wraps a QProcess and facilitates emulation of node.js's ChildProcess
 */
class ChildProcessContext : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 pid READ pid)

public:
    explicit ChildProcessContext(QObject *parent = 0);
    virtual ~ChildProcessContext();

    qint64 pid() const;
    Q_INVOKABLE void kill(const QString &signal = "SIGTERM");

    Q_INVOKABLE void _setEncoding(const QString &encoding);
    Q_INVOKABLE bool _start(const QString &cmd, const QStringList &args);

signals:
    void exit(const int code) const;

    /**
     * For emulating `child.stdout.on("data", function (data) {})`
     */
    void stdoutData(const QString &data) const;
    /**
     * For emulating `child.stderr.on("data", function (data) {})`
     */
    void stderrData(const QString &data) const;

private slots:
    void _readyReadStandardOutput();
    void _readyReadStandardError();
    void _error(const QProcess::ProcessError error);
    void _finished(const int exitCode, const QProcess::ExitStatus exitStatus);

private:
    QProcess m_proc;
    Encoding m_encoding;
};

/**
 * Helper class for child_process module
 */
class ChildProcess : public QObject
{
    Q_OBJECT

public:
    explicit ChildProcess(QObject *parent = 0);
    virtual ~ChildProcess();

    Q_INVOKABLE QObject *_createChildProcessContext();
};

#endif // CHILDPROCESS_H
