/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWDEFS_WIN_H
#define QWINDOWDEFS_WIN_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE


QT_END_NAMESPACE

#if !defined(Q_NOWINSTRICT)
#define Q_WINSTRICT
#endif

#if defined(Q_WINSTRICT)

#if !defined(STRICT)
#define STRICT
#endif
#undef NO_STRICT
#define Q_DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name

#else

#if !defined(NO_STRICT)
#define NO_STRICT
#endif
#undef  STRICT
#define Q_DECLARE_HANDLE(name) typedef HANDLE name

#endif

#ifndef HINSTANCE
Q_DECLARE_HANDLE(HINSTANCE);
#endif
#ifndef HDC
Q_DECLARE_HANDLE(HDC);
#endif
#ifndef HWND
Q_DECLARE_HANDLE(HWND);
#endif
#ifndef HFONT
Q_DECLARE_HANDLE(HFONT);
#endif
#ifndef HPEN
Q_DECLARE_HANDLE(HPEN);
#endif
#ifndef HBRUSH
Q_DECLARE_HANDLE(HBRUSH);
#endif
#ifndef HBITMAP
Q_DECLARE_HANDLE(HBITMAP);
#endif
#ifndef HICON
Q_DECLARE_HANDLE(HICON);
#endif
#ifndef HCURSOR
typedef HICON HCURSOR;
#endif
#ifndef HPALETTE
Q_DECLARE_HANDLE(HPALETTE);
#endif
#ifndef HRGN
Q_DECLARE_HANDLE(HRGN);
#endif
#ifndef HMONITOR
Q_DECLARE_HANDLE(HMONITOR);
#endif
#ifndef _HRESULT_DEFINED
typedef long HRESULT;
#endif

typedef struct tagMSG MSG;

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT HINSTANCE qWinAppInst();
Q_CORE_EXPORT HINSTANCE qWinAppPrevInst();
Q_CORE_EXPORT int       qWinAppCmdShow();
Q_WIDGETS_EXPORT HDC           qt_win_display_dc();

QT_END_NAMESPACE

#endif // QWINDOWDEFS_WIN_H
