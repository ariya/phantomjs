/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QWEBKITGLOBAL_H
#define QWEBKITGLOBAL_H

#include <QtCore/qglobal.h>

#define QTWEBKIT_VERSION_STR "2.2.3"
// QTWEBKIT_VERSION is (major << 16) + (minor << 8) + patch. Similar to Qt.
#define QTWEBKIT_VERSION 0x020203
// Use: #if (QTWEBKIT_VERSION >= QTWEBKIT_VERSION_CHECK(2, 0, 0)). Similar to Qt.
#define QTWEBKIT_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#if defined(QT_MAKEDLL)        /* create a Qt DLL library */
#  if defined(BUILDING_WEBKIT)
#      define QWEBKIT_EXPORT Q_DECL_EXPORT
#  else
#      define QWEBKIT_EXPORT Q_DECL_IMPORT
#  endif
#elif defined(QT_DLL) /* use a Qt DLL library */
#  define QWEBKIT_EXPORT Q_DECL_IMPORT
#endif

#if !defined(QWEBKIT_EXPORT)
#  if defined(QT_SHARED)
#    define QWEBKIT_EXPORT Q_DECL_EXPORT
#  else
#    define QWEBKIT_EXPORT
#  endif
#endif

#endif // QWEBKITGLOBAL_H
