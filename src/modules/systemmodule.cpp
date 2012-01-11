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

#include "systemmodule.h"

#include "../env.h"

SystemModule::SystemModule(QObject *parent) :
    QObject(parent),
    m_stderr(stderr, QIODevice::WriteOnly),
    m_stdin(stdin, QIODevice::ReadOnly),
    m_stdout(stdout, QIODevice::WriteOnly),
    m_stream_stderr(&m_stderr, this),
    m_stream_stdin(&m_stdin, this),
    m_stream_stdout(&m_stdout, this)
{
}

// public:

void SystemModule::setArgs(const QStringList &args)
{
    m_args.clear();
    m_args.append(args);
}

QStringList SystemModule::args() const
{
    return m_args;
}

QVariantMap SystemModule::env() const
{
    return Env::instance()->asVariantMap();
}

QString SystemModule::platform() const
{
    return "phantomjs";
}

QObject *SystemModule::_stderr()
{
    return &m_stream_stderr;
}

QObject *SystemModule::_stdin()
{
    return &m_stream_stdin;
}

QObject *SystemModule::_stdout()
{
    return &m_stream_stdout;
}
