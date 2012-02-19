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

#include "system.h"

#include "../env.h"

#include <QFile>

namespace commonjs {

System::System(QObject *parent) :
    QObject(parent),
    m_stderr((TextStream *)NULL),
    m_stdin((TextStream *)NULL),
    m_stdout((TextStream *)NULL)
{
}

// public:

void System::setArgs(const QStringList &args)
{
    m_args.clear();
    m_args.append(args);
}

QStringList System::args() const
{
    return m_args;
}

QVariantMap System::env() const
{
    return Env::instance()->asVariantMap();
}

QString System::platform() const
{
    return "phantomjs";
}

QFile *openFd(FILE *fd, QIODevice::OpenMode flags)
{
    QFile *f = new QFile();
    // TODO: Check success
    f->open(fd, flags);
    return f;
}

QObject *System::_stderr()
{
    if ((TextStream *)NULL == m_stderr) {
        m_stderr = new TextStream(openFd(stderr, QIODevice::WriteOnly), this);
    }
    return m_stderr;
}

QObject *System::_stdin()
{
    if ((TextStream *)NULL == m_stdin) {
        m_stdin = new TextStream(openFd(stdin, QIODevice::ReadOnly), this);
    }
    return m_stdin;
}

QObject *System::_stdout()
{
    if ((TextStream *)NULL == m_stdout) {
        m_stdout = new TextStream(openFd(stdout, QIODevice::WriteOnly), this);
    }
    return m_stdout;
}

} // namespace commonjs
