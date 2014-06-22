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

#include <qglobal.h>
#include "qsystemerror_p.h"
#if !defined(Q_OS_WINCE)
#  include <errno.h>
#  if defined(Q_CC_MSVC)
#    include <crtdbg.h>
#  endif
#else
#  if (_WIN32_WCE >= 0x700)
#    include <errno.h>
#  endif
#endif
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WIN) && !defined(QT_NO_THREAD) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX) && \
    defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L
namespace {
    // There are two incompatible versions of strerror_r:
    // a) the XSI/POSIX.1 version, which returns an int,
    //    indicating success or not
    // b) the GNU version, which returns a char*, which may or may not
    //    be the beginning of the buffer we used
    // The GNU libc manpage for strerror_r says you should use the XSI
    // version in portable code. However, it's impossible to do that if
    // _GNU_SOURCE is defined so we use C++ overloading to decide what to do
    // depending on the return type
    static inline QString fromstrerror_helper(int, const QByteArray &buf)
    {
        return QString::fromLocal8Bit(buf);
    }
    static inline QString fromstrerror_helper(const char *str, const QByteArray &)
    {
        return QString::fromLocal8Bit(str);
    }
}
#endif

#ifdef Q_OS_WIN
static QString windowsErrorString(int errorCode)
{
    QString ret;
#ifndef Q_OS_WINRT
    wchar_t *string = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);
#else
    wchar_t errorString[1024];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  errorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&errorString,
                  sizeof(errorString)/sizeof(wchar_t),
                  NULL);
    ret = QString::fromWCharArray(errorString);
#endif  // Q_OS_WINRT

    if (ret.isEmpty() && errorCode == ERROR_MOD_NOT_FOUND)
        ret = QString::fromLatin1("The specified module could not be found.");
    return ret;
}
#endif

static QString standardLibraryErrorString(int errorCode)
{
    const char *s = 0;
    QString ret;
    switch (errorCode) {
    case 0:
        break;
    case EACCES:
        s = QT_TRANSLATE_NOOP("QIODevice", "Permission denied");
        break;
    case EMFILE:
        s = QT_TRANSLATE_NOOP("QIODevice", "Too many open files");
        break;
    case ENOENT:
        s = QT_TRANSLATE_NOOP("QIODevice", "No such file or directory");
        break;
    case ENOSPC:
        s = QT_TRANSLATE_NOOP("QIODevice", "No space left on device");
        break;
    default: {
    #ifdef Q_OS_WINCE
        ret = windowsErrorString(errorCode);
    #else
        #if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_QNX)
            QByteArray buf(1024, '\0');
            ret = fromstrerror_helper(strerror_r(errorCode, buf.data(), buf.size()), buf);
        #else
            ret = QString::fromLocal8Bit(strerror(errorCode));
        #endif
    #endif
    break; }
    }
    if (s) {
        // ######## this breaks moc build currently
        // ret = QCoreApplication::translate("QIODevice", s);
        ret = QString::fromLatin1(s);
    }
    return ret.trimmed();
}

QString QSystemError::toString()
{
    switch(errorScope) {
    case NativeError:
#if defined (Q_OS_WIN)
        return windowsErrorString(errorCode);
#else
        //unix: fall through as native and standard library are the same
#endif
    case StandardLibraryError:
        return standardLibraryErrorString(errorCode);
    default:
        qWarning("invalid error scope");
        //fall through
    case NoError:
        return QLatin1String("No error");
    }
}

QT_END_NAMESPACE

