/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QGUIFUNCTIONS_WCE_H
#define QGUIFUNCTIONS_WCE_H
#ifdef Q_OS_WINCE
#include <QtCore/qfunctions_wince.h>
#define UNDER_NT
#include <wingdi.h>

#ifdef QT_BUILD_GUI_LIB
QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
QT_MODULE(Gui)
QT_END_NAMESPACE
QT_END_HEADER
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
int qt_wince_GetDIBits(HDC, HBITMAP, uint, uint, void*, LPBITMAPINFO, uint);
#define GetDIBits(a,b,c,d,e,f,g) qt_wince_GetDIBits(a,b,c,d,e,f,g)

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

HINSTANCE qt_wince_ShellExecute(HWND hwnd, LPCWSTR operation, LPCWSTR file, LPCWSTR params, LPCWSTR dir, int showCmd);
#define ShellExecute(a,b,c,d,e,f) qt_wince_ShellExecute(a,b,c,d,e,f)


// Clipboard --------------------------------------------------------
#define WM_CHANGECBCHAIN	1
#define WM_DRAWCLIPBOARD	2

BOOL qt_wince_ChangeClipboardChain(
    HWND hWndRemove,  // handle to window to remove
    HWND hWndNewNext  // handle to next window
);
#define ChangeClipboardChain(a,b) qt_wince_ChangeClipboardChain(a,b);

HWND qt_wince_SetClipboardViewer(
    HWND hWndNewViewer   // handle to clipboard viewer window
);
#define SetClipboardViewer(a) qt_wince_SetClipboardViewer(a)

// Graphics ---------------------------------------------------------
COLORREF qt_wince_PALETTEINDEX( WORD wPaletteIndex );
#define PALETTEINDEX(a) qt_wince_PALETTEINDEX(a)

#endif // Q_OS_WINCE
#endif // QGUIFUNCTIONS_WCE_H
