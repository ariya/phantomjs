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

#include "qsystemtrayicon_p.h"
#ifndef QT_NO_SYSTEMTRAYICON

#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0600
#  undef _WIN32_WINNT
#endif
#if !defined(_WIN32_WINNT)
#  define _WIN32_WINNT 0x0600
#endif

#if defined(_WIN32_IE) && _WIN32_IE < 0x0600
#  undef _WIN32_IE
#endif
#if !defined(_WIN32_IE)
#  define _WIN32_IE 0x0600 //required for NOTIFYICONDATA_V2_SIZE
#endif

#include <private/qsystemlibrary_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <QSettings>
#include <QDebug>
#include <QHash>

#include <qt_windows.h>
#include <commctrl.h>
#include <windowsx.h>

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

#ifndef NIN_KEYSELECT
#    define NIN_KEYSELECT (WM_USER + 1)
#endif

#ifdef Q_CC_MINGW
#    define NIN_SELECT (WM_USER + 0)
#    define NIN_BALLOONTIMEOUT (WM_USER + 4)
#    define NIN_BALLOONUSERCLICK (WM_USER + 5)
#    define NIF_SHOWTIP 0x00000080
#    define NOTIFYICON_VERSION_4 4
#endif

#define Q_MSGFLT_ALLOW 1

typedef HRESULT (WINAPI *PtrShell_NotifyIconGetRect)(const Q_NOTIFYICONIDENTIFIER* identifier, RECT* iconLocation);
typedef BOOL (WINAPI *PtrChangeWindowMessageFilter)(UINT message, DWORD dwFlag);
typedef BOOL (WINAPI *PtrChangeWindowMessageFilterEx)(HWND hWnd, UINT message, DWORD action, void* pChangeFilterStruct);

// Copy QString data to a limited wchar_t array including \0.
static inline void qStringToLimitedWCharArray(QString in, wchar_t *target, int maxLength)
{
    const int length = qMin(maxLength - 1, in.size());
    if (length < in.size())
        in.truncate(length);
    in.toWCharArray(target);
    target[length] = wchar_t(0);
}

class QSystemTrayIconSys
{
public:
    QSystemTrayIconSys(HWND hwnd, QSystemTrayIcon *object);
    ~QSystemTrayIconSys();
    bool trayMessage(DWORD msg);
    void setIconContents(NOTIFYICONDATA &data);
    bool showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon type, uint uSecs);
    QRect findIconGeometry(UINT iconId);
    HICON createIcon();
    bool winEvent(MSG *m, long *result);

private:
    const HWND m_hwnd;
    HICON hIcon;
    QPoint globalPos;
    QSystemTrayIcon *q;
    uint notifyIconSize;
    int version;
    bool ignoreNextMouseRelease;
};

static bool allowsMessages()
{
#ifndef QT_NO_SETTINGS
    const QString key = QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
    const QSettings settings(key, QSettings::NativeFormat);
    return settings.value(QStringLiteral("EnableBalloonTips"), true).toBool();
#else
    return false;
#endif
}

typedef QHash<HWND, QSystemTrayIconSys *> HandleTrayIconHash;

Q_GLOBAL_STATIC(HandleTrayIconHash, handleTrayIconHash)

extern "C" LRESULT QT_WIN_CALLBACK qWindowsTrayconWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == MYWM_TASKBARCREATED || message == MYWM_NOTIFYICON) {
        if (QSystemTrayIconSys *trayIcon = handleTrayIconHash()->value(hwnd)) {
            MSG msg;
            msg.hwnd = hwnd;         // re-create MSG structure
            msg.message = message;   // time and pt fields ignored
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.pt.x = GET_X_LPARAM(lParam);
            msg.pt.y = GET_Y_LPARAM(lParam);
            long result = 0;
            if (trayIcon->winEvent(&msg, &result))
                return result;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Invoke a service of the native Windows interface to create
// a non-visible toplevel window to receive tray messages.
// Note: Message windows (HWND_MESSAGE) are not sufficient, they
// will not receive the "TaskbarCreated" message.
static inline HWND createTrayIconMessageWindow()
{
    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
    if (!ni)
        return 0;
    // Register window class in the platform plugin.
    QString className;
    void *wndProc = reinterpret_cast<void *>(qWindowsTrayconWndProc);
    if (!QMetaObject::invokeMethod(ni, "registerWindowClass", Qt::DirectConnection,
                                   Q_RETURN_ARG(QString, className),
                                   Q_ARG(QString, QStringLiteral("QTrayIconMessageWindowClass")),
                                   Q_ARG(void *, wndProc))) {
        return 0;
    }
    const wchar_t windowName[] = L"QTrayIconMessageWindow";
    return CreateWindowEx(0, (wchar_t*)className.utf16(),
                          windowName, WS_OVERLAPPED,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, NULL, (HINSTANCE)GetModuleHandle(0), NULL);
}

QSystemTrayIconSys::QSystemTrayIconSys(HWND hwnd, QSystemTrayIcon *object)
    : m_hwnd(hwnd), hIcon(0), q(object)
    , notifyIconSize(NOTIFYICONDATA_V2_SIZE), version(NOTIFYICON_VERSION)
    , ignoreNextMouseRelease(false)

{
    handleTrayIconHash()->insert(m_hwnd, this);

    if (QSysInfo::windowsVersion() < QSysInfo::WV_VISTA) {
        notifyIconSize = NOTIFYICONDATA_V2_SIZE;
        version = NOTIFYICON_VERSION;
    }

    // For restoring the tray icon after explorer crashes
    if (!MYWM_TASKBARCREATED) {
        MYWM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
    }

    // Allow the WM_TASKBARCREATED message through the UIPI filter on Windows Vista and higher
    static PtrChangeWindowMessageFilterEx pChangeWindowMessageFilterEx =
        (PtrChangeWindowMessageFilterEx)QSystemLibrary::resolve(QLatin1String("user32"), "ChangeWindowMessageFilterEx");

    if (pChangeWindowMessageFilterEx) {
        // Call the safer ChangeWindowMessageFilterEx API if available (Windows 7 onwards)
        pChangeWindowMessageFilterEx(m_hwnd, MYWM_TASKBARCREATED, Q_MSGFLT_ALLOW, 0);
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
    handleTrayIconHash()->remove(m_hwnd);
    if (hIcon)
        DestroyIcon(hIcon);
    DestroyWindow(m_hwnd);
}

void QSystemTrayIconSys::setIconContents(NOTIFYICONDATA &tnd)
{
    tnd.uFlags |= NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = MYWM_NOTIFYICON;
    tnd.hIcon = hIcon;
    const QString tip = q->toolTip();
    if (!tip.isNull())
        qStringToLimitedWCharArray(tip, tnd.szTip, sizeof(tnd.szTip)/sizeof(wchar_t));
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
    qStringToLimitedWCharArray(message, tnd.szInfo, 256);
    qStringToLimitedWCharArray(title, tnd.szInfoTitle, 64);

    tnd.uID = q_uNOTIFYICONID;
    tnd.dwInfoFlags = iconFlag(type);
    tnd.cbSize = notifyIconSize;
    tnd.hWnd = m_hwnd;
    tnd.uTimeout = uSecs;
    tnd.uFlags = NIF_INFO | NIF_SHOWTIP;

    return Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

bool QSystemTrayIconSys::trayMessage(DWORD msg)
{
    NOTIFYICONDATA tnd;
    memset(&tnd, 0, notifyIconSize);

    tnd.uID = q_uNOTIFYICONID;
    tnd.cbSize = notifyIconSize;
    tnd.hWnd = m_hwnd;
    tnd.uFlags = NIF_SHOWTIP;
    tnd.uVersion = version;

    if (msg == NIM_ADD || msg == NIM_MODIFY) {
        setIconContents(tnd);
    }

    bool success = Shell_NotifyIcon(msg, &tnd);

    if (msg == NIM_ADD)
        return success && Shell_NotifyIcon(NIM_SETVERSION, &tnd);
    else
        return success;
}

Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &);

HICON QSystemTrayIconSys::createIcon()
{
    const HICON oldIcon = hIcon;
    hIcon = 0;
    const QIcon icon = q->icon();
    if (icon.isNull())
        return oldIcon;
    const int iconSizeX = GetSystemMetrics(SM_CXSMICON);
    const int iconSizeY = GetSystemMetrics(SM_CYSMICON);
    const QSize size = icon.actualSize(QSize(iconSizeX, iconSizeY));
    const QPixmap pm = icon.pixmap(size);
    if (pm.isNull())
        return oldIcon;
    hIcon = qt_pixmapToWinHICON(pm);
    return oldIcon;
}

bool QSystemTrayIconSys::winEvent( MSG *m, long *result )
{
    *result = 0;
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
        if (m->message == MYWM_TASKBARCREATED) // self-registered message id.
            trayMessage(NIM_ADD);
        break;
    }
    return false;
}

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(0),
      visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
}

void QSystemTrayIconPrivate::install_sys()
{
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        if (const HWND hwnd = createTrayIconMessageWindow()) {
            sys = new QSystemTrayIconSys(hwnd, q);
            sys->createIcon();
            sys->trayMessage(NIM_ADD);
        } else {
            qWarning("%s: The platform plugin failed to create a message window.", Q_FUNC_INFO);
        }
    }
}

/*
* This function tries to determine the icon geometry from the tray
*
* If it fails an invalid rect is returned.
*/

QRect QSystemTrayIconSys::findIconGeometry(UINT iconId)
{
    struct AppData
    {
        HWND hwnd;
        UINT uID;
    };

    static PtrShell_NotifyIconGetRect Shell_NotifyIconGetRect =
        (PtrShell_NotifyIconGetRect)QSystemLibrary::resolve(QLatin1String("shell32"),
                                                            "Shell_NotifyIconGetRect");

    if (Shell_NotifyIconGetRect) {
        Q_NOTIFYICONIDENTIFIER nid;
        memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = m_hwnd;
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
        AppData appData = { 0, 0 };
        SendMessage(trayHandle, TB_GETBUTTON, toolbarButton , (LPARAM)data);

        if (!ReadProcessMemory(trayProcess, data, &buttonData, sizeof(TBBUTTON), &numBytes))
            continue;

        if (!ReadProcessMemory(trayProcess, (LPVOID) buttonData.dwData, &appData, sizeof(AppData), &numBytes))
            continue;

        bool isHidden = buttonData.fsState & TBSTATE_HIDDEN;

        if (m_hwnd == appData.hwnd && appData.uID == iconId && !isHidden) {
            SendMessage(trayHandle, TB_GETITEMRECT, toolbarButton , (LPARAM)data);
            RECT iconRect = {0, 0, 0, 0};
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

void QSystemTrayIconPrivate::showMessage_sys(const QString &title,
                                             const QString &messageIn,
                                             QSystemTrayIcon::MessageIcon type,
                                             int timeOut)
{
    if (!sys || !allowsMessages())
        return;

    // 10 sec default
    const uint uSecs = timeOut < 0 ? uint(10000) : uint(timeOut);
    // For empty messages, ensures that they show when only title is set
    QString message = messageIn;
    if (message.isEmpty() && !title.isEmpty())
        message.append(QLatin1Char(' '));

    sys->showMessage(title, message, type, uSecs);
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

    const HICON hIconToDestroy = sys->createIcon();
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
