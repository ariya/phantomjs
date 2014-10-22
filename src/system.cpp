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

#include "system.h"

#include <QApplication>
#include <QSslSocket>
#include <QSysInfo>
#include <QVariantMap>
#include <QTextCodec>

#include "../env.h"

System::System(QObject *parent) :
    QObject(parent)
  , m_stdout((File *)NULL)
  , m_stderr((File *)NULL)
  , m_stdin((File *)NULL)
{
    // Populate "env"
    m_env = Env::instance()->asVariantMap();

    // Populate "os"
    // "osarchitecture" word size
    m_os.insert("architecture", QString("%1bit").arg(QSysInfo::WordSize));

    // "os.name" and "os.version"
#if defined(Q_OS_WIN32)
    m_os.insert("name", "windows");
    switch (QSysInfo::WindowsVersion) {
    case QSysInfo::WV_32s:
        m_os.insert("version", "3.1");
        break;
    case QSysInfo::WV_95:
        m_os.insert("version", "95");
        break;
    case QSysInfo::WV_98:
        m_os.insert("version", "98");
        break;
    case QSysInfo::WV_Me:
        m_os.insert("version", "Me");
        break;
    case QSysInfo::WV_NT:
        m_os.insert("version", "NT");
        break;
    case QSysInfo::WV_2000:
        m_os.insert("version", "2000");
        break;
    case QSysInfo::WV_XP:
        m_os.insert("version", "XP");
        break;
    case QSysInfo::WV_2003:
        m_os.insert("version", "2003");
        break;
    case QSysInfo::WV_VISTA:
        m_os.insert("version", "Vista");
        break;
    case QSysInfo::WV_WINDOWS7:
        m_os.insert("version", "7");
        break;
    case QSysInfo::WV_WINDOWS8:
        m_os.insert("version", "8");
        break;
    default:
        m_os.insert("version", "unknown");
        break;
    }
#elif defined(Q_OS_MAC)
    m_os.insert("name", "mac");
    switch (QSysInfo::MacintoshVersion) {
    case QSysInfo::MV_10_3:
        m_os.insert("version", "10.3 (Panther)");
        break;
    case QSysInfo::MV_10_4:
        m_os.insert("version", "10.4 (Tiger)");
        break;
    case QSysInfo::MV_10_5:
        m_os.insert("version", "10.5 (Leopard)");
        break;
    case QSysInfo::MV_10_6:
        m_os.insert("version", "10.6 (Snow Leopard)");
        break;
    case QSysInfo::MV_10_7:
        m_os.insert("version", "10.7 (Lion)");
        break;
    case QSysInfo::MV_10_8:
        m_os.insert("version", "10.8 (Mountain Lion)");
        break;
    case QSysInfo::MV_10_9:
        m_os.insert("version", "10.9 (Mavericks)");
        break;
    case QSysInfo::MV_10_10:
        m_os.insert("version", "10.10 (Yosemite)");
        break;
    default:
        m_os.insert("version", "unknown");
        break;
    }
#elif defined(Q_OS_LINUX)
    m_os.insert("name", "linux");
    m_os.insert("version", "unknown");
#else
    m_os.insert("name", "unknown");
    m_os.insert("version", "unknown");
#endif
}

System::~System()
{
    // Clean-up standard streams
    if ((File *)NULL != m_stdout) {
        delete m_stdout;
        m_stdout = (File *)NULL;
    }
    if ((File *)NULL != m_stderr) {
        delete m_stderr;
        m_stderr = (File *)NULL;
    }
    if ((File *)NULL != m_stdin) {
        delete m_stdin;
        m_stdin = (File *)NULL;
    }
}

qint64 System::pid() const
{
    return QApplication::applicationPid();
}

void System::setArgs(const QStringList &args)
{
    m_args = args;
}

QStringList System::args() const
{
    return m_args;
}

QVariant System::env() const
{
    return m_env;
}

QVariant System::os() const
{
    return m_os;
}

bool System::isSSLSupported() const
{
    return QSslSocket::supportsSsl();
}

QObject *System::_stdout() {
    if ((File *)NULL == m_stdout) {
        QFile *f = new QFile();
        f->open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered);
        m_stdout = createFileInstance(f);
    }

    return m_stdout;
}

QObject *System::_stderr() {
    if ((File *)NULL == m_stderr) {
        QFile *f = new QFile();
        f->open(stderr, QIODevice::WriteOnly | QIODevice::Unbuffered);
        m_stderr = createFileInstance(f);
    }

    return m_stderr;
}

QObject *System::_stdin() {
    if ((File *)NULL == m_stdin) {
        QFile *f = new QFile();
        f->open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered);
        m_stdin = createFileInstance(f);
    }

    return m_stdin;
}

// private:

File *System::createFileInstance(QFile *f)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    return new File(f, codec, this);
}
