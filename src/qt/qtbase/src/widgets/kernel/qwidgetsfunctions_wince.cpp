/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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
#include "qwidgetsfunctions_wince.h"
#include <shellapi.h>

QT_USE_NAMESPACE

#ifndef ShellExecute
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
#endif

#ifndef SPI_GETPLATFORMTYPE
#define SPI_GETPLATFORMTYPE 257
#endif

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
    if (GetVersionEx(&osvi))
        return osvi.dwBuildNumber;
    return 0;
}

int qt_wince_get_version()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi))
        return (osvi.dwMajorVersion * 10 + osvi.dwMinorVersion);
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
