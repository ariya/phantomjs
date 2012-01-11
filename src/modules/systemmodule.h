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

#ifndef SYSTEMMODULE_H
#define SYSTEMMODULE_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QTextStream>

#include "../textstream.h"

// This class implements the CommonJS System/1.0 spec.
// See: http://wiki.commonjs.org/wiki/System/1.0
class SystemModule : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QVariantMap env READ env)
    Q_PROPERTY(QString platform READ platform)
    Q_PROPERTY(QObject *stderr READ _stderr)
    Q_PROPERTY(QObject *stdin READ _stdin)
    Q_PROPERTY(QObject *stdout READ _stdout)

public:
    explicit SystemModule(QObject *parent = 0);

    void setArgs(const QStringList &args);
    // system.args
    QStringList args() const;

    // system.env
    QVariantMap env() const;

    // system.platform
    QString platform() const;

    // system.stderr
    QObject *_stderr();

    // system.stdin
    QObject *_stdin();

    // system.stdout
    QObject *_stdout();

private:
    QStringList m_args;
    QTextStream m_stderr;
    QTextStream m_stdin;
    QTextStream m_stdout;
    TextStream m_stream_stderr;
    TextStream m_stream_stdin;
    TextStream m_stream_stdout;
};

#endif // SYSTEMMODULE_H
