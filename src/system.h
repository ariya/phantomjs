/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 execjosh, http://execjosh.blogspot.com
  Copyright (C) 2012 James M. Greene <james.m.greene@gmail.com>

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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVariant>

#include "filesystem.h"

// This class implements the CommonJS System/1.0 spec.
// See: http://wiki.commonjs.org/wiki/System/1.0
class System : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 pid READ pid)
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QVariant env READ env)
    Q_PROPERTY(QVariant os READ os)
    Q_PROPERTY(bool isSSLSupported READ isSSLSupported)
    Q_PROPERTY(QObject *stdout READ _stdout)
    Q_PROPERTY(QObject *stderr READ _stderr)
    Q_PROPERTY(QObject *stdin READ _stdin)

public:
    explicit System(QObject *parent = 0);
    virtual ~System();

    qint64 pid() const;

    void setArgs(const QStringList& args);
    QStringList args() const;

    QVariant env() const;

    QVariant os() const;

    bool isSSLSupported() const;

    // system.stdout
    QObject *_stdout();

    // system.stderr
    QObject *_stderr();

    // system.stdin
    QObject *_stdin();

private:
    File *createFileInstance(QFile *f);

    QStringList m_args;
    QVariant m_env;
    QMap<QString, QVariant> m_os;
    File *m_stdout;
    File *m_stderr;
    File *m_stdin;
};

#endif // SYSTEM_H
