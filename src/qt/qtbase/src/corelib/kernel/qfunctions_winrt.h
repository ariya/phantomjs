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

#ifndef QFUNCTIONS_WINRT_H
#define QFUNCTIONS_WINRT_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_WINRT

QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_CORE_LIB
#endif

QT_END_NAMESPACE

// Environment ------------------------------------------------------
errno_t qt_winrt_getenv_s(size_t*, char*, size_t, const char*);
errno_t qt_winrt__putenv_s(const char*, const char*);
void qt_winrt_tzset();
void qt_winrt__tzset();

// As Windows Runtime lacks some standard functions used in Qt, these got
// reimplemented. Other projects do this as well. Inline functions are used
// that there is a central place to disable functions for newer versions if
// they get available. There are no defines used anymore, because this
// will break member functions of classes which are called like these
// functions.
// The other declarations available in this file are being used per
// define inside qplatformdefs.h of the corresponding WinRT mkspec.

#define generate_inline_return_func0(funcname, returntype) \
        inline returntype funcname() \
        { \
            return qt_winrt_##funcname(); \
        }
#define generate_inline_return_func1(funcname, returntype, param1) \
        inline returntype funcname(param1 p1) \
        { \
            return qt_winrt_##funcname(p1); \
        }
#define generate_inline_return_func2(funcname, returntype, param1, param2) \
        inline returntype funcname(param1 p1, param2 p2) \
        { \
            return qt_winrt_##funcname(p1,  p2); \
        }
#define generate_inline_return_func3(funcname, returntype, param1, param2, param3) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3) \
        { \
            return qt_winrt_##funcname(p1,  p2, p3); \
        }
#define generate_inline_return_func4(funcname, returntype, param1, param2, param3, param4) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4) \
        { \
            return qt_winrt_##funcname(p1,  p2, p3, p4); \
        }
#define generate_inline_return_func5(funcname, returntype, param1, param2, param3, param4, param5) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5) \
        { \
            return qt_winrt_##funcname(p1,  p2, p3, p4, p5); \
        }
#define generate_inline_return_func6(funcname, returntype, param1, param2, param3, param4, param5, param6) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6) \
        { \
            return qt_winrt_##funcname(p1,  p2, p3, p4, p5, p6); \
        }
#define generate_inline_return_func7(funcname, returntype, param1, param2, param3, param4, param5, param6, param7) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7) \
        { \
            return qt_winrt_##funcname(p1,  p2, p3, p4, p5, p6, p7); \
        }

typedef unsigned (__stdcall *StartAdressExFunc)(void *);
typedef void(*StartAdressFunc)(void *);
typedef int ( __cdecl *CompareFunc ) (const void *, const void *) ;

generate_inline_return_func4(getenv_s, errno_t, size_t *, char *, size_t, const char *)
generate_inline_return_func2(_putenv_s, errno_t, const char *, const char *)
generate_inline_return_func0(tzset, void)
generate_inline_return_func0(_tzset, void)

#endif // Q_OS_WINRT
#endif // QFUNCTIONS_WINRT_H
