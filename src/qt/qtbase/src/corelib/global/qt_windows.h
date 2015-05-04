/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QT_WINDOWS_H
#define QT_WINDOWS_H

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#if defined(Q_CC_BOR)
// Borland's windows.h does not set these correctly, resulting in
// unusable WinSDK standard dialogs
#ifndef WINVER
#  define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501
#endif
#endif

#if defined(Q_CC_MINGW)
// mingw's windows.h does not set _WIN32_WINNT, resulting breaking compilation
#ifndef WINVER
#  define WINVER 0x501
#endif
#endif

#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#if defined(_WIN32_IE) && _WIN32_IE < 0x0501
#  undef _WIN32_IE
#endif
#if !defined(_WIN32_IE)
#  define _WIN32_IE 0x0501
#endif

#ifdef _WIN32_WCE
#include <ceconfig.h>
#endif

// already defined when compiled with WINVER >= 0x0500
#ifndef SPI_SETMENUANIMATION
#define SPI_SETMENUANIMATION 0x1003
#endif
#ifndef SPI_SETMENUFADE
#define SPI_SETMENUFADE 0x1013
#endif
#ifndef SPI_SETCOMBOBOXANIMATION
#define SPI_SETCOMBOBOXANIMATION 0x1005
#endif
#ifndef SPI_SETTOOLTIPANIMATION
#define SPI_SETTOOLTIPANIMATION 0x1017
#endif
#ifndef SPI_SETTOOLTIPFADE
#define SPI_SETTOOLTIPFADE 0x1019
#endif
#ifndef SPI_SETUIEFFECTS
#define SPI_SETUIEFFECTS 0x103F
#endif
#ifndef SPI_GETMENUANIMATION
#define SPI_GETMENUANIMATION 0x1002
#endif
#ifndef SPI_GETMENUFADE
#define SPI_GETMENUFADE 0x1012
#endif
#ifndef SPI_GETCOMBOBOXANIMATION
#define SPI_GETCOMBOBOXANIMATION 0x1004
#endif
#ifndef SPI_GETTOOLTIPANIMATION
#define SPI_GETTOOLTIPANIMATION 0x1016
#endif
#ifndef SPI_GETTOOLTIPFADE
#define SPI_GETTOOLTIPFADE 0x1018
#endif
#ifndef SPI_GETUIEFFECTS
#define SPI_GETUIEFFECTS 0x103E
#endif
#ifndef SPI_GETKEYBOARDCUES
#define SPI_GETKEYBOARDCUES 0x100A
#endif
#ifndef SPI_GETGRADIENTCAPTIONS
#define SPI_GETGRADIENTCAPTIONS 0x1008
#endif
#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef ETO_PDY
#define ETO_PDY 0x2000
#endif
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION 27
#endif
#ifndef COLOR_GRADIENTINACTIVECAPTION
#define COLOR_GRADIENTINACTIVECAPTION 28
#endif

// already defined when compiled with WINVER >= 0x0600
#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU 0x1022
#endif
#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif

#ifdef Q_OS_WINCE
#ifndef LR_DEFAULTSIZE
#define LR_DEFAULTSIZE 0
#endif
#ifndef LR_SHARED
#define LR_SHARED 0
#endif
#endif // Q_OS_WINCE

#endif // QT_WINDOWS_H
