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

#include "qsystemtrayicon_p.h"
#ifndef QT_NO_SYSTEMTRAYICON

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x600
#endif

#include <qt_windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <private/qsystemlibrary_p.h>
#include <QApplication>
#include <QSettings>

QT_BEGIN_NAMESPACE

static const UINT q_uNOTIFYICONID = 0;

static uint MYWM_TASKBARCREATED = 0;
#define MYWM_NOTIFYICON (WM_APP+101)

struct Q_NOTIFYICONIDENTIFIER {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    GUID guidItem;
};

#ifndef NOTIFYICON_VERSION_4
#define NOTIFYICON_VERSION_4 4
#endif

#ifndef NIN_SELECT
#define NIN_SELECT (WM_USER + 0)
#endif

#ifndef NIN_KEYSELECT
#define NIN_KEYSELECT (WM_USER + 1)
#endif

#ifndef NIN_BALLOONTIMEOUT
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#endif

#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000080
#endif

#define Q_MSGFLT_ALLOW 1

typedef HRESULT (WINAPI *PtrShell_NotifyIconGetRect)(const Q_NOTIFYICONIDENTIFIER* identifier, RECT* iconLocation);
typedef BOOL (WINAPI *PtrChangeWindowMessageFilter)(UINT message, DWORD dwFlag);
typedef BOOL (WINAPI *PtrChangeWindowMessageFilterEx)(HWND hWnd, UINT message, DWORD action, void* pChangeFilterStruct);

class QSystemTrayIconSys : QWidget
{
public:
    QSystemTrayIconSys(QSystemTrayIcon *object);
    ~QSystemTrayIconSys();
    bool winEvent( MSG *m, long *result );
    bool trayMessage(DWORD msg);
    void setIconContents(NOTIFYICONDATA &data);
    bool showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon type, uint uSecs);
    QRect findIconGeometry(const int a_iButtonID);
    void createIcon();
    HICON hIcon;
    QPoint globalPos;
    QSystemTrayIcon *q;
private:
    uint notifyIconSize;
    int maxTipLength;
    int version;
    bool ignoreNextMouseRelease;
};

static bool allowsMessages()
{
#ifndef QT_NO_SETTINGS
    QSettings settings(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft"
                                      "\\Windows\\CurrentVersion\\Explorer\\Advanced"), QSettings::NativeFormat);
    return settings.value(QLatin1String("EnableBalloonTips"), true).toBool();
#else
    return false;
#endif
}

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *object)
    : hIcon(0), q(object), ignoreNextMouseRelease(false)

{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA) {
        notifyIconSize = sizeof(NOTIFYICONDATA);
        version = NOTIFYICON_VERSION_4;
    } else {
        notifyIconSize = NOTIFYICONDATA_V2_SIZE;
        version = NOTIFYICON_VERSION;
    }

    maxTipLength = 128;

    // For restoring the tray icon after explorer crashes
    if (!MYWM_TASKBARCREATED) {
        MYWM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
    }

    // Allow the WM_TASKBARCREATED message through the UIPI filter on Windows Vista and higher
    static PtrChangeWindowMessageFilterEx pChangeWindowMessageFilterEx =
        (PtrChangeWindowMessageFilterEx)QSystemLibrary::resolve(QLatin1String("user32"), "ChangeWindowMessageFilterEx");

    if (pChangeWindowMessageFilterEx) {
        // Call the safer ChangeWindowMessageFilterEx API if available
        pChangeWindowMessageFilterEx(winId(), MYWM_TASKBARCREATED, Q_MSGFLT_ALLOW, 0);
    } else {
        static PtrChangeWindowMessageFilter pChangeWindowMessageFilter =
            (PtrChangeWindowMessageFilter)QSystemLibrary::resolve(QLatin1String("user32"), "ChangeWindowMessageFilter");

        if (pChangeWindowMessageFilter) {
            // Call the deprecated ChangeWindowMessageFilter API otherwise
            pChangeWindowMessageFilter(MYWM_TASKBARCREATED, Q_MSGFLT_ALLOW);
        }
    }
}

QSystemTrayIconSys::~QSystemTrayIconSys()
{
    if (hIcon)
        DestroyIcon(hIcon);
}

void QSystemTrayIconSys::setIconContents(NOTIFYICONDATA &tnd)
{
    tnd.uFlags |= NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = MYWM_NOTIFYICON;
    tnd.hIcon = hIcon;
    QString tip = q->toolTip();

    if (!tip.isNull()) {
        tip = tip.left(maxTipLength - 1) + QChar();
        memcpy(tnd.szTip, tip.utf16(), qMin(tip.length() + 1, maxTipLength) * sizeof(wchar_t));
    }
}

static int iconFlag( QSystemTrayIcon::MessageIcon icon )
{
    switch (icon) {
        case QSystemTrayIcon::Information:
            return NIIF_INFO;
        case QSystemTrayIcon::Warning:
            return NIIF_WARNING;
        case QSystemTrayIcon::Critical:
            return NIIF_ERROR;
        case QSystemTrayIcon::NoIcon:
            return NIIF_NONE;
        default:
            Q_ASSERT_X(false, "QSystemTrayIconSys::showMessage", "Invalid QSystemTrayIcon::MessageIcon value");
            return NIIF_NONE;
    }
}

bool QSystemTrayIconSys::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon type, uint uSecs)
{
    NOTIFYICONDATA tnd;
    memset(&tnd, 0, notifyIconSize);

    memcpy(tnd.szInfo, message.utf16(), qMin(message.length() + 1, 256) * sizeof(wchar_t));
    memcpy(tnd.szInfoTitle, title.utf16(), qMin(title.length() + 1, 64) * sizeof(wchar_t));

    tnd.uID = q_uNOTIFYICONID;
    tnd.dwInfoFlags = iconFlag(type);
    tnd.cbSize = notifyIconSize;
    tnd.hWnd = winId();
    tnd.uTimeout = uSecs;
    tnd.uFlags = NIF_INFO | NIF_SHOWTIP;

    Q_ASSERT(testAttribute(Qt::WA_WState_Created));

    return Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

bool QSystemTrayIconSys::trayMessage(DWORD msg)
{
    NOTIFYICONDATA tnd;
    memset(&tnd, 0, notifyIconSize);

    tnd.uID = q_uNOTIFYICONID;
    tnd.cbSize = notifyIconSize;
    tnd.hWnd = winId();
    tnd.uFlags = NIF_SHOWTIP;
    tnd.uVersion = version;

    Q_ASSERT(testAttribute(Qt::WA_WState_Created));

    if (msg == NIM_ADD || msg == NIM_MODIFY) {
        setIconContents(tnd);
    }

    bool success = Shell_NotifyIcon(msg, &tnd);

    if (msg == NIM_ADD)
        return success && Shell_NotifyIcon(NIM_SETVERSION, &tnd);
    else
        return success;
}

void QSystemTrayIconSys::createIcon()
{
    hIcon = 0;
    QIcon icon = q->icon();
    if (icon.isNull())
        return;

    const int iconSizeX = GetSystemMetrics(SM_CXSMICON);
    const int iconSizeY = GetSystemMetrics(SM_CYSMICON);
    QSize size = icon.actualSize(QSize(iconSizeX, iconSizeY));
    QPixmap pm = icon.pixmap(size);
    if (pm.isNull())
        return;

    hIcon = pm.toWinHICON();
}

bool QSystemTrayIconSys::winEvent( MSG *m, long *result )
{
    switch(m->message) {
    case MYWM_NOTIFYICON:
        {
            int message = 0;
            QPoint gpos;

            if (version == NOTIFYICON_VERSION_4) {
                Q_ASSERT(q_uNOTIFYICONID == HIWORD(m->lParam));
                message = LOWORD(m->lParam);
                gpos = QPoint(GET_X_LPARAM(m->wParam), GET_Y_LPARAM(m->wParam));
            } else {
                Q_ASSERT(q_uNOTIFYICONID == m->wParam);
                message = m->lParam;
                gpos = QCursor::pos();
            }

            switch (message) {
            case NIN_SELECT:
            case NIN_KEYSELECT:
                if (ignoreNextMouseRelease)
                    ignoreNextMouseRelease = false;
                else 
                    emit q->activated(QSystemTrayIcon::Trigger);
                break;

            case WM_LBUTTONDBLCLK:
                ignoreNextMouseRelease = true; // Since DBLCLICK Generates a second mouse 
                                               // release we must ignore it
                emit q->activated(QSystemTrayIcon::DoubleClick);
                break;

            case WM_CONTEXTMENU:
                if (q->contextMenu()) {
                    q->contextMenu()->popup(gpos);
                    q->contextMenu()->activateWindow();
                }
                emit q->activated(QSystemTrayIcon::Context);
                break;

            case NIN_BALLOONUSERCLICK:
                emit q->messageClicked();
                break;

            case WM_MBUTTONUP:
                emit q->activated(QSystemTrayIcon::MiddleClick);
                break;

            default:
                break;
            }
            break;
        }
    default:
        if (m->message == MYWM_TASKBARCREATED)
            trayMessage(NIM_ADD);
        else
            return QWidget::winEvent(m, result);
        break;
    }
    return 0;
}

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        sys = new QSystemTrayIconSys(q);
        sys->createIcon();
        sys->trayMessage(NIM_ADD);
    }
}

/*
* This function tries to determine the icon geometry from the tray
*
* If it fails an invalid rect is returned.
*/
QRect QSystemTrayIconSys::findIconGeometry(const int iconId)
{
    static PtrShell_NotifyIconGetRect Shell_NotifyIconGetRect =
        (PtrShell_NotifyIconGetRect)QSystemLibrary::resolve(QLatin1String("shell32"), "Shell_NotifyIconGetRect");

    if (Shell_NotifyIconGetRect) {
        Q_NOTIFYICONIDENTIFIER nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = winId();
        nid.uID = iconId;

        RECT rect;
        HRESULT hr = Shell_NotifyIconGetRect(&nid, &rect);
        if (SUCCEEDED(hr)) {
            return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        }
    }

    QRect ret;

    TBBUTTON buttonData;
    DWORD processID = 0;
    HWND trayHandle = FindWindow(L"Shell_TrayWnd", NULL);

    //find the toolbar used in the notification area
    if (trayHandle) {
        trayHandle = FindWindowEx(trayHandle, NULL, L"TrayNotifyWnd", NULL);
        if (trayHandle) {
            HWND hwnd = FindWindowEx(trayHandle, NULL, L"SysPager", NULL);
            if (hwnd) {
                hwnd = FindWindowEx(hwnd, NULL, L"ToolbarWindow32", NULL);
                if (hwnd)
                    trayHandle = hwnd;
            }
        }
    }

    if (!trayHandle)
        return ret;

    GetWindowThreadProcessId(trayHandle, &processID);
    if (processID <= 0)
        return ret;

    HANDLE trayProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ, 0, processID);
    if (!trayProcess)
        return ret;

    int buttonCount = SendMessage(trayHandle, TB_BUTTONCOUNT, 0, 0);
    LPVOID data = VirtualAllocEx(trayProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);

    if ( buttonCount < 1 || !data ) {
        CloseHandle(trayProcess);
        return ret;
    }

    //search for our icon among all toolbar buttons
    for (int toolbarButton = 0; toolbarButton  < buttonCount; ++toolbarButton ) {
        SIZE_T numBytes = 0;
        DWORD appData[2] = { 0, 0 };
        SendMessage(trayHandle, TB_GETBUTTON, toolbarButton , (LPARAM)data);

        if (!ReadProcessMemory(trayProcess, data, &buttonData, sizeof(TBBUTTON), &numBytes))
            continue;

        if (!ReadProcessMemory(trayProcess, (LPVOID) buttonData.dwData, appData, sizeof(appData), &numBytes))
            continue;

        int currentIconId = appData[1];
        HWND currentIconHandle = (HWND) appData[0];
        bool isHidden = buttonData.fsState & TBSTATE_HIDDEN;

        if (currentIconHandle == winId() &&
            currentIconId == iconId && !isHidden) {
            SendMessage(trayHandle, TB_GETITEMRECT, toolbarButton , (LPARAM)data);
            RECT iconRect = {0, 0};
            if(ReadProcessMemory(trayProcess, data, &iconRect, sizeof(RECT), &numBytes)) {
                MapWindowPoints(trayHandle, NULL, (LPPOINT)&iconRect, 2);
                QRect geometry(iconRect.left + 1, iconRect.top + 1,
                                iconRect.right - iconRect.left - 2,
                                iconRect.bottom - iconRect.top - 2);
                if (geometry.isValid())
                    ret = geometry;
                break;
            }
        }
    }
    VirtualFreeEx(trayProcess, data, 0, MEM_RELEASE);
    CloseHandle(trayProcess);
    return ret;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon type, int timeOut)
{
    if (!sys || !allowsMessages())
        return;

    uint uSecs = 0;
    if ( timeOut < 0)
        uSecs = 10000; //10 sec default
    else uSecs = (int)timeOut;

    //message is limited to 255 chars + NULL
    QString messageString;
    if (message.isEmpty() && !title.isEmpty())
        messageString = QLatin1Char(' '); //ensures that the message shows when only title is set
    else
        messageString = message.left(255) + QChar();

    //title is limited to 63 chars + NULL
    QString titleString = title.left(63) + QChar();

    sys->showMessage(titleString, messageString, type, uSecs);
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (!sys)
        return QRect();

    return sys->findIconGeometry(q_uNOTIFYICONID);
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (!sys)
        return;

    sys->trayMessage(NIM_DELETE);
    delete sys;
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (!sys)
        return;

    HICON hIconToDestroy = sys->hIcon;

    sys->createIcon();
    sys->trayMessage(NIM_MODIFY);

    if (hIconToDestroy)
        DestroyIcon(hIconToDestroy);
}

void QSystemTrayIconPrivate::updateMenu_sys()
{

}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (!sys)
        return;

    sys->trayMessage(NIM_MODIFY);
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return allowsMessages();
}

QT_END_NAMESPACE

#endif
