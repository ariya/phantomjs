/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QPLATFORMFUNCTIONS_WCE_H
#define QPLATFORMFUNCTIONS_WCE_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#ifdef Q_OS_WINCE
#include <QtCore/qfunctions_wince.h>
#define UNDER_NT
#include <wingdi.h>
#include <objidl.h>

#ifndef WM_MOUSELEAVE
# define WM_MOUSELEAVE                   0x02A3
#endif

#ifndef WM_TOUCH
# define WM_TOUCH 0x0240
#endif

#ifndef WM_GETOBJECT
#define WM_GETOBJECT                    0x003D
#endif

#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#define GWLP_USERDATA GWL_USERDATA

#ifndef CWP_SKIPINVISIBLE
#define CWP_SKIPINVISIBLE   0x0001
#define CWP_SKIPTRANSPARENT 0x0004
#define findPlatformWindowAt(a, b, c) findPlatformWindowAt(a, b)
#endif

#ifndef CS_OWNDC
#define CS_OWNDC  0x0020
#endif

#ifndef HWND_MESSAGE
#define HWND_MESSAGE 0
#endif

#ifndef CAPTUREBLT
#define CAPTUREBLT                   (DWORD)0x40000000
#endif

#define SW_SHOWMINIMIZED SW_MINIMIZE
#define SW_SHOWMINNOACTIVE SW_MINIMIZE

#ifndef ChildWindowFromPointEx
#define ChildWindowFromPointEx(a, b, c) ChildWindowFromPoint(a, b)
#endif

#ifndef CF_DIBV5
#define CF_DIBV5            17
#endif

#ifndef WM_MOUSEACTIVATE
#define WM_MOUSEACTIVATE 0x0021
#endif

#ifndef WM_CHILDACTIVATE
#define WM_CHILDACTIVATE 0x0022
#endif

#ifndef WM_PARENTNOTIFY
#define WM_PARENTNOTIFY 0x0210
#endif

#ifndef WM_ENTERIDLE
#define WM_ENTERIDLE 0x0121
#endif

#ifndef WM_GETMINMAXINFO
#define WM_GETMINMAXINFO 0x0024
#endif

#ifndef WM_WINDOWPOSCHANGING
#define WM_WINDOWPOSCHANGING 0x0046
#endif

#ifndef WM_NCMOUSEMOVE
#define WM_NCMOUSEMOVE 0x00A0
#endif

#ifndef WM_NCMBUTTONDBLCLK
#define WM_NCMBUTTONDBLCLK 0x00A
#endif

#ifndef WM_NCCREATE
#define WM_NCCREATE 0x0081
#endif

#ifndef WM_NCCALCSIZE
#define WM_NCCALCSIZE 0x0083
#endif

#ifndef WM_NCACTIVATE
#define WM_NCACTIVATE 0x0086
#endif

#ifndef WM_NCMOUSELEAVE
#define WM_NCMOUSELEAVE 0x02A2
#endif

#ifndef WM_NCLBUTTONDOWN
#define WM_NCLBUTTONDOWN 0x00A1
#endif

#ifndef WM_NCLBUTTONUP
#define WM_NCLBUTTONUP 0x00A2
#endif

#ifndef WM_NCPAINT
#define WM_NCPAINT 0x0085
#endif

#ifndef WM_NCHITTEST
#define WM_NCHITTEST 0x0084
#endif

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED 0x031A
#endif

#ifndef WM_DISPLAYCHANGE
#define WM_DISPLAYCHANGE 0x007E
#endif

#ifndef VREFRESH
#define VREFRESH 116
#endif

#ifndef SM_SWAPBUTTON
#define SM_SWAPBUTTON 23
#endif

// application defines
#define SPI_SETNONCLIENTMETRICS 72
#define SPI_SETICONTITLELOGFONT 0x0022
#define WM_ACTIVATEAPP 0x001c
#define SW_PARENTCLOSING    1
#define SW_OTHERMAXIMIZED   2
#define SW_PARENTOPENING    3
#define SW_OTHERRESTORED    4
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))

// drag n drop
#ifndef CFSTR_PERFORMEDDROPEFFECT
#define CFSTR_PERFORMEDDROPEFFECT TEXT("Performed DropEffect")
#endif

// QWidget
#define SW_SHOWMINIMIZED SW_MINIMIZE

// QRegion
#define ALTERNATE 0
#define WINDING 1

// QFontEngine
typedef struct _FIXED {
  WORD  fract;
  short value;
} FIXED;

typedef struct tagPOINTFX {
  FIXED x;
  FIXED y;
} POINTFX;

typedef struct _MAT2 {
  FIXED eM11;
  FIXED eM12;
  FIXED eM21;
  FIXED eM22;
} MAT2;

typedef struct _GLYPHMETRICS {
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    short   gmCellIncX;
    short   gmCellIncY;
} GLYPHMETRICS;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER;

typedef struct tagTTPOLYCURVE
{
    WORD    wType;
    WORD    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE;

#define GGO_NATIVE 2
#define GGO_GLYPH_INDEX 0x0080
#define TT_PRIM_LINE 1
#define TT_PRIM_QSPLINE 2
#define TT_PRIM_CSPLINE 3
#define ANSI_VAR_FONT 12

#ifndef OleInitialize
#define OleInitialize(a) 0
#endif

#ifndef SPI_GETSNAPTODEFBUTTON
#define SPI_GETSNAPTODEFBUTTON  95
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif

// Clipboard --------------------------------------------------------
#ifndef WM_CHANGECBCHAIN
#define WM_CHANGECBCHAIN 0x030D
#endif

#ifndef WM_DRAWCLIPBOARD
#define WM_DRAWCLIPBOARD 0x0308
#endif

inline bool IsIconic( HWND /*hWnd*/ )
{
    return false;
}

inline int AddFontResourceExW( LPCWSTR /*name*/, DWORD /*fl*/, PVOID /*res*/)
{
    return 0;
}

inline bool RemoveFontResourceExW( LPCWSTR /*name*/, DWORD /*fl*/, PVOID /*pdv*/)
{
    return 0;
}

inline void OleUninitialize()
{
}

inline DWORD GetGlyphOutline( HDC /*hdc*/, UINT /*uChar*/, INT /*fuFormat*/, GLYPHMETRICS * /*lpgm*/,
                       DWORD /*cjBuffer*/, LPVOID /*pvBuffer*/, CONST MAT2 * /*lpmat2*/ )
{
    qFatal("GetGlyphOutline() not supported under Windows CE. Please try using freetype font-rendering, by "
           "passing the command line argument -platform windows:fontengine=freetype to the application.");
    return GDI_ERROR;
}

inline HWND GetAncestor(HWND hWnd, UINT /*gaFlags*/)
{
    return GetParent(hWnd);
}

#ifndef GA_PARENT
#  define GA_PARENT 1
#endif

#ifndef SPI_SETFONTSMOOTHINGTYPE
#  define SPI_SETFONTSMOOTHINGTYPE 0x200B
#endif
#ifndef SPI_GETFONTSMOOTHINGTYPE
#  define SPI_GETFONTSMOOTHINGTYPE 0x200A
#endif
#ifndef FE_FONTSMOOTHINGCLEARTYPE
#  define FE_FONTSMOOTHINGCLEARTYPE 0x0002
#endif

#ifndef DEVICE_FONTTYPE
#define DEVICE_FONTTYPE 0x0002
#endif

#ifndef RASTER_FONTTYPE
#define RASTER_FONTTYPE 0x0001
#endif

#ifndef WM_DISPLAYCHANGE
#define WM_DISPLAYCHANGE 0x007E
#endif

BOOL qt_wince_ChangeClipboardChain(
    HWND hWndRemove,  // handle to window to remove
    HWND hWndNewNext  // handle to next window
);
#define ChangeClipboardChain(a,b) qt_wince_ChangeClipboardChain(a,b);

HWND qt_wince_SetClipboardViewer(
    HWND hWndNewViewer   // handle to clipboard viewer window
);
#define SetClipboardViewer(a) qt_wince_SetClipboardViewer(a)

#endif // Q_OS_WINCE
#endif // QPLATFORMFUNCTIONS_WCE_H
