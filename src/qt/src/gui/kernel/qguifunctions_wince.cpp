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
#include "qguifunctions_wince.h"
#include <shellapi.h>
#include <QtCore/qlibrary.h>

QT_USE_NAMESPACE

struct AygSHINITDLGINFO
{
    DWORD dwMask;
    HWND  hDlg;
    DWORD dwFlags;
};

struct AygSIPINFO
{
    DWORD   cbSize;
    DWORD   fdwFlags;
    RECT    rcVisibleDesktop;
    RECT    rcSipRect;
    DWORD   dwImDataSize;
    void   *pvImData;
};

#ifndef SHIDIF_CANCELBUTTON
#define SHIDIF_CANCELBUTTON 0x0080
#endif

#ifndef SHIDIM_FLAGS
#define SHIDIM_FLAGS 0x0001
#endif

#ifndef SHIDIF_DONEBUTTON
#define SHIDIF_DONEBUTTON 0x0001
#endif
#ifndef SHIDIF_SIZEDLGFULLSCREEN
#define SHIDIF_SIZEDLGFULLSCREEN 0x0004
#endif

#ifndef SHDB_HIDE
#define SHDB_HIDE 0x0002
#endif

#ifndef SHFS_SHOWTASKBAR
#define SHFS_SHOWTASKBAR 0x0001
#endif
#ifndef SHFS_HIDETASKBAR
#define SHFS_HIDETASKBAR 0x0002
#endif
#ifndef SHFS_SHOWSIPBUTTON
#define SHFS_SHOWSIPBUTTON 0x0004
#endif
#ifndef SHFS_HIDESIPBUTTON
#define SHFS_HIDESIPBUTTON 0x0008
#endif
#ifndef SHFS_SHOWSTARTICON
#define SHFS_SHOWSTARTICON 0x0010
#endif
#ifndef SHFS_HIDESTARTICON
#define SHFS_HIDESTARTICON 0x0020
#endif

#ifndef SIPF_OFF
#define SIPF_OFF 0x00000000
#endif
#ifndef SIPF_ON
#define SIPF_ON 0x00000001
#endif

#ifndef SPI_SETSIPINFO
#define SPI_SETSIPINFO 224
#endif
#ifndef SPI_GETSIPINFO
#define SPI_GETSIPINFO 225
#endif
#ifndef SPI_GETPLATFORMTYPE
#define SPI_GETPLATFORMTYPE 257
#endif

typedef BOOL (*AygInitDialog)(AygSHINITDLGINFO*);
typedef BOOL (*AygFullScreen)(HWND, DWORD);
typedef BOOL (*AygSHSipInfo)(UINT, UINT, PVOID, UINT);
typedef BOOL (*AygSHDoneButton)(HWND, DWORD);

static AygInitDialog ptrAygInitDialog = 0;
static AygFullScreen ptrAygFullScreen = 0;
static AygSHSipInfo  ptrAygSHSipInfo  = 0;
static AygSHDoneButton ptrAygSHDoneButton = 0;
static void resolveAygLibs()
{
    static bool aygResolved = false;
    if (!aygResolved) {
        QLibrary ayglib(QLatin1String("aygshell"));
        ptrAygInitDialog = (AygInitDialog) ayglib.resolve("SHInitDialog");
        ptrAygFullScreen = (AygFullScreen) ayglib.resolve("SHFullScreen");
        ptrAygSHSipInfo  = (AygSHSipInfo)  ayglib.resolve("SHSipInfo");
        ptrAygSHDoneButton = (AygSHDoneButton) ayglib.resolve("SHDoneButton");
        aygResolved = true;
    }
}

int qt_wince_GetDIBits(HDC /*hdc*/ , HBITMAP hSourceBitmap, uint, uint, LPVOID lpvBits, LPBITMAPINFO, uint)
{
    if (!lpvBits) {
        qWarning("::GetDIBits(), lpvBits NULL");
        return 0;
    }
    BITMAP bm;
    GetObject(hSourceBitmap, sizeof(BITMAP), &bm);
    bm.bmHeight = qAbs(bm.bmHeight);

    HBITMAP hTargetBitmap;
    void *pixels;

    BITMAPINFO dibInfo;
    memset(&dibInfo, 0, sizeof(dibInfo));
    dibInfo.bmiHeader.biBitCount = 32;
    dibInfo.bmiHeader.biClrImportant = 0;
    dibInfo.bmiHeader.biClrUsed = 0;
    dibInfo.bmiHeader.biCompression = BI_RGB;;
    dibInfo.bmiHeader.biHeight = -bm.bmHeight;
    dibInfo.bmiHeader.biWidth = bm.bmWidth;
    dibInfo.bmiHeader.biPlanes = 1;
    dibInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dibInfo.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;

    HDC displayDC = GetDC(NULL);
    if (!displayDC) {
        qWarning("::GetDIBits(), failed to GetDC");
        return 0;
    }

    int ret = bm.bmHeight;

    hTargetBitmap = CreateDIBSection(displayDC, (const BITMAPINFO*) &dibInfo, DIB_RGB_COLORS,
                                    (void**)&pixels, NULL, 0);
    if (!hTargetBitmap) {
        qWarning("::GetDIBits(), failed to CreateDIBSection");
        return 0;
    }

    HDC hdcSrc = CreateCompatibleDC(displayDC);
    HDC hdcDst = CreateCompatibleDC(displayDC);

    if (!(hdcDst && hdcSrc)) {
        qWarning("::GetDIBits(), failed to CreateCompatibleDC");
        ret = 0;
    }

    HBITMAP hOldBitmap1 = (HBITMAP) SelectObject(hdcSrc, hSourceBitmap);
    HBITMAP hOldBitmap2 = (HBITMAP) SelectObject(hdcDst, hTargetBitmap);

    if (!(hOldBitmap1 && hOldBitmap2)) {
        qWarning("::GetDIBits(), failed to SelectObject for bitmaps");
        ret = 0;
    }

    if (!BitBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY)) {
        qWarning("::GetDIBits(), BitBlt failed");
        ret = 0;
    }

    SelectObject(hdcSrc, hOldBitmap1);
    SelectObject(hdcDst, hOldBitmap2);

    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);

    ReleaseDC(NULL, displayDC);

    memcpy(lpvBits, pixels, dibInfo.bmiHeader.biSizeImage);

    DeleteObject(hTargetBitmap);
    return ret;
}

HINSTANCE qt_wince_ShellExecute(HWND hwnd, LPCWSTR, LPCWSTR file, LPCWSTR params, LPCWSTR dir, int showCmd)
{
    SHELLEXECUTEINFO info;
    info.hwnd = hwnd;
    info.lpVerb = L"Open";
    info.lpFile = file;
    info.lpParameters = params;
    info.lpDirectory = dir;
    info.nShow = showCmd;
    info.cbSize = sizeof(info);
    ShellExecuteEx(&info);
    return info.hInstApp;
}

// Clipboard --------------------------------------------------------
BOOL qt_wince_ChangeClipboardChain( HWND /*hWndRemove*/, HWND /*hWndNewNext*/ )
{
    return FALSE;
}

HWND qt_wince_SetClipboardViewer( HWND /*hWndNewViewer*/ )
{
    return NULL;
}


// Graphics ---------------------------------------------------------
COLORREF qt_wince_PALETTEINDEX( WORD /*wPaletteIndex*/)
{
    return 0;
}

// Internal Qt -----------------------------------------------------
bool qt_wince_is_platform(const QString &platformString) {
    wchar_t tszPlatform[64];
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(tszPlatform) / sizeof(wchar_t), tszPlatform, 0))
        if (0 == _tcsicmp(reinterpret_cast<const wchar_t *> (platformString.utf16()), tszPlatform))
            return true;
    return false;
}

int qt_wince_get_build()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi)) {
        return osvi.dwBuildNumber;
    } 
    return 0;
}

int qt_wince_get_version()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi)) {
        return (osvi.dwMajorVersion * 10 + osvi.dwMinorVersion);
    } 
    return 0;
}

bool qt_wince_is_windows_mobile_65()
{
    const DWORD dwFirstWM65BuildNumber = 21139;
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
        return false;
    return osvi.dwMajorVersion > 5
        || (osvi.dwMajorVersion == 5 && (osvi.dwMinorVersion > 2 ||
            (osvi.dwMinorVersion == 2 && osvi.dwBuildNumber >= dwFirstWM65BuildNumber)));
}

bool qt_wince_is_pocket_pc() {
    return qt_wince_is_platform(QString::fromLatin1("PocketPC"));
}

bool qt_wince_is_smartphone() {
       return qt_wince_is_platform(QString::fromLatin1("Smartphone"));
}
bool qt_wince_is_mobile() {
     return (qt_wince_is_smartphone() || qt_wince_is_pocket_pc());
}

bool qt_wince_is_high_dpi() {
    if (!qt_wince_is_pocket_pc())
        return false;
    HDC deviceContext = GetDC(0);
    int dpi = GetDeviceCaps(deviceContext, LOGPIXELSX);
    ReleaseDC(0, deviceContext);
    if ((dpi < 1000) && (dpi > 0))
        return dpi > 96;
    else
        return false;
}

void qt_wince_maximize(QWidget *widget)
{
    HWND hwnd = widget->winId();
    if (qt_wince_is_mobile()) {
        AygSHINITDLGINFO shidi;
        shidi.dwMask = SHIDIM_FLAGS;
        shidi.hDlg = hwnd;
        shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
        if (widget->windowFlags() & Qt::WindowCancelButtonHint)
            shidi.dwFlags |= SHIDIF_CANCELBUTTON;
        if (widget->windowFlags() & Qt::WindowOkButtonHint)
            shidi.dwFlags |= SHIDIF_DONEBUTTON;
        if (!(widget->windowFlags() & (Qt::WindowCancelButtonHint | Qt::WindowOkButtonHint)))
            shidi.dwFlags |= SHIDIF_CANCELBUTTON;
        resolveAygLibs();
        if (ptrAygInitDialog)
            ptrAygInitDialog(&shidi);
    } else {
        RECT r;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
        MoveWindow(hwnd, r.top, r.left, r.right - r.left, r.bottom - r.top, true);
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong (hwnd, GWL_EXSTYLE) | WS_EX_NODRAG);
    }
}

void qt_wince_unmaximize(QWidget *widget)
{
    if (ptrAygSHDoneButton && qt_wince_is_mobile()
        && !(widget->windowFlags() & (Qt::WindowCancelButtonHint | Qt::WindowOkButtonHint)))
    {
        // Hide the [X] button, we've added in qt_wince_maximize.
        ptrAygSHDoneButton(widget->winId(), SHDB_HIDE);
    }
}

void qt_wince_minimize(HWND hwnd)
{
#ifdef Q_OS_WINCE_WM
    ShowWindow(hwnd, SW_HIDE);
#else
    if (!IsWindowVisible(hwnd)) {
        // Hack for an initial showMinimized.
        // Without it, our widget doesn't appear in the task bar.
        ShowWindow(hwnd, SW_SHOW);
    }
    ShowWindow(hwnd, SW_MINIMIZE);
#endif
}

void qt_wince_hide_taskbar(HWND hwnd) {
    if (ptrAygFullScreen)
        ptrAygFullScreen(hwnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON);
}

void qt_wince_full_screen(HWND hwnd, bool fullScreen, UINT swpf) {
    resolveAygLibs();
    if (fullScreen) {
        QRect r = qApp->desktop()->screenGeometry(QWidget::find(hwnd));
        SetWindowPos(hwnd, HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
        if (ptrAygFullScreen)
            ptrAygFullScreen(hwnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON);
        if (!qt_wince_is_mobile()) {
            HWND handle = FindWindow(L"HHTaskBar", L"");
            if (handle) {
                ShowWindow(handle, 0);
                EnableWindow(handle, false);
            }
        }
    } else {
        if (ptrAygFullScreen)
            ptrAygFullScreen(hwnd, SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON | SHFS_SHOWSTARTICON);
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, swpf);
        if (!qt_wince_is_mobile()) {
            HWND handle = FindWindow(L"HHTaskBar", L"");
            if (handle) {
                ShowWindow(handle, 1);
                EnableWindow(handle, true);
            }
        }
    }
}

void qt_wince_show_SIP(bool show)
{
    resolveAygLibs();
    if (!ptrAygSHSipInfo)
        return;

    AygSIPINFO si;
    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    ptrAygSHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
    si.cbSize = sizeof(si);
    si.fdwFlags = (show ? SIPF_ON : SIPF_OFF);
    ptrAygSHSipInfo(SPI_SETSIPINFO, 0, &si, 0);
}
