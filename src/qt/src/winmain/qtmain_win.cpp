/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Windows main function of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt_windows.h"
#include "qbytearray.h"
#include "qstring.h"
#include "qvector.h"

/*
  This file contains the code in the qtmain library for Windows.
  qtmain contains the Windows startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. WinMain calls qWinMain in the Qt DLL/library, which
  initializes Qt.
*/

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WINCE)
extern void __cdecl qWinMain(HINSTANCE, HINSTANCE, LPSTR, int, int &, QVector<char *> &);
#else
extern void qWinMain(HINSTANCE, HINSTANCE, LPSTR, int, int &, QVector<char *> &);
#endif

QT_END_NAMESPACE

QT_USE_NAMESPACE


#if defined(QT_NEEDS_QMAIN)
int qMain(int, char **);
#define main qMain
#else
#ifdef Q_OS_WINCE
extern "C" int __cdecl main(int, char **);
#else
extern "C" int main(int, char **);
#endif
#endif

/*
  WinMain() - Initializes Windows and calls user's startup function main().
  NOTE: WinMain() won't be called if the application was linked as a "console"
  application.
*/

#ifdef Q_OS_WINCE
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR /*wCmdParam*/, int cmdShow)
#else
extern "C"
int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR /*cmdParamarg*/, int cmdShow)
#endif
{
    QByteArray cmdParam = QString::fromWCharArray(GetCommandLine()).toLocal8Bit();

#if defined(Q_OS_WINCE)
    wchar_t appName[MAX_PATH];
    GetModuleFileName(0, appName, MAX_PATH);
    cmdParam.prepend(QString(QLatin1String("\"%1\" ")).arg(QString::fromWCharArray(appName)).toLocal8Bit());
#endif

    int argc = 0;
    QVector<char *> argv(8);
    qWinMain(instance, prevInstance, cmdParam.data(), cmdShow, argc, argv);

#if defined(Q_OS_WINCE)
    wchar_t uniqueAppID[MAX_PATH];
    GetModuleFileName(0, uniqueAppID, MAX_PATH);
    QString uid = QString::fromWCharArray(uniqueAppID).toLower().replace(QLatin1String("\\"), QLatin1String("_"));

    // If there exists an other instance of this application
    // it will be the owner of a mutex with the unique ID.
    HANDLE mutex = CreateMutex(NULL, TRUE, (LPCWSTR)uid.utf16());
    if (mutex && ERROR_ALREADY_EXISTS == GetLastError()) {
        CloseHandle(mutex);

        // The app is already running, so we use the unique
        // ID to create a unique messageNo, which is used
        // as the registered class name for the windows
        // created. Set the first instance's window to the
        // foreground, else just terminate.
        // Use bitwise 0x01 OR to reactivate window state if
        // it was hidden
        UINT msgNo = RegisterWindowMessage((LPCWSTR)uid.utf16());
        HWND aHwnd = FindWindow((LPCWSTR)QString::number(msgNo).utf16(), 0);
        if (aHwnd)
            SetForegroundWindow((HWND)(((ULONG)aHwnd) | 0x01));
        return 0;
    }
#endif // Q_OS_WINCE

    int result = main(argc, argv.data());
#if defined(Q_OS_WINCE)
    CloseHandle(mutex);
#endif
    return result;
}
