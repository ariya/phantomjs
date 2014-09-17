/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#define _WIN32_IE 0x0600 //required for NOTIFYICONDATA_V2_SIZE

#include <qt_windows.h>
#include <shlwapi.h>
#include <QApplication>

QT_BEGIN_NAMESPACE

static const UINT q_uNOTIFYICONID = 13;     // IDs from 0 to 12 are reserved on WinCE.
#define MYWM_NOTIFYICON (WM_APP+101)

struct Q_NOTIFYICONIDENTIFIER {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    GUID guidItem;
};

class QSystemTrayIconSys : QWidget
{
public:
    QSystemTrayIconSys(QSystemTrayIcon *object);
    ~QSystemTrayIconSys();
    bool winEvent( MSG *m, long *result );
    bool trayMessage(DWORD msg);
    void setIconContents(NOTIFYICONDATA &data);
    void createIcon();
    QRect findTrayGeometry();
    HICON hIcon;
    QPoint globalPos;
    QSystemTrayIcon *q;
private:
    uint notifyIconSize;
    int maxTipLength;
    bool ignoreNextMouseRelease;
};

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *object)
    : hIcon(0), q(object), ignoreNextMouseRelease(false)

{
    notifyIconSize = FIELD_OFFSET(NOTIFYICONDATA, szTip[64]); // NOTIFYICONDATAW_V1_SIZE;
    maxTipLength = 64;
}

QSystemTrayIconSys::~QSystemTrayIconSys()
{
    if (hIcon)
        DestroyIcon(hIcon);
}

QRect QSystemTrayIconSys::findTrayGeometry()
{
    // Use lower right corner as fallback
    QPoint brCorner = qApp->desktop()->screenGeometry().bottomRight();
    QRect ret(brCorner.x() - 10, brCorner.y() - 10, 10, 10);
    return ret;
}

void QSystemTrayIconSys::setIconContents(NOTIFYICONDATA &tnd)
{
    tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = MYWM_NOTIFYICON;
    tnd.hIcon = hIcon;
    QString tip = q->toolTip();

    if (!tip.isNull()) {
        tip = tip.left(maxTipLength - 1) + QChar();
        memcpy(tnd.szTip, tip.utf16(), qMin(tip.length() + 1, maxTipLength) * sizeof(wchar_t));
    }
}

bool QSystemTrayIconSys::trayMessage(DWORD msg)
{
    NOTIFYICONDATA tnd;
    memset(&tnd, 0, notifyIconSize);
    tnd.uID = q_uNOTIFYICONID;
    tnd.cbSize = notifyIconSize;
    tnd.hWnd = winId();

    Q_ASSERT(testAttribute(Qt::WA_WState_Created));

    if (msg != NIM_DELETE) {
        setIconContents(tnd);
    }

    return Shell_NotifyIcon(msg, &tnd);
}

void QSystemTrayIconSys::createIcon()
{
    hIcon = 0;
    QIcon icon = q->icon();
    if (icon.isNull())
        return;

    //const QSize preferredSize(GetSystemMetrics(SM_CXSMICON) * 2, GetSystemMetrics(SM_CYSMICON) * 2);
    const QSize preferredSize(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    QPixmap pm = icon.pixmap(preferredSize);
    if (pm.isNull())
        return;

    hIcon = pm.toWinHICON();
}

bool QSystemTrayIconSys::winEvent( MSG *m, long *result )
{
    switch(m->message) {
    case WM_CREATE:
        SetWindowLong(winId(), GWL_USERDATA, (LONG)((CREATESTRUCTW*)m->lParam)->lpCreateParams);
        break;

    case MYWM_NOTIFYICON:
        {
            QPoint gpos = QCursor::pos();

            switch (m->lParam) {
            case WM_LBUTTONUP:
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

            case WM_RBUTTONUP:
                if (q->contextMenu()) {
                    q->contextMenu()->popup(gpos);

                    // We must ensure that the popup menu doesn't show up behind the task bar.
                    QRect desktopRect = qApp->desktop()->availableGeometry();
                    int maxY = desktopRect.y() + desktopRect.height() - q->contextMenu()->height();
                    if (gpos.y() > maxY) {
                        gpos.ry() = maxY;
                        q->contextMenu()->move(gpos);
                    }
                }
                emit q->activated(QSystemTrayIcon::Context);
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
        return QWidget::winEvent(m, result);
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

void QSystemTrayIconPrivate::showMessage_sys(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon type, int timeOut)
{
    if (!sys)
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

    //show QBalloonTip
    QRect trayRect = sys->findTrayGeometry();
    QBalloonTip::showBalloon(type, title, message, sys->q, QPoint(trayRect.left(),
                             trayRect.center().y()), uSecs, false);
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    return QRect();
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
    // Calling sys->trayMessage(NIM_MODIFY) on an existing icon is broken on Windows CE.
    // So we need to call updateIcon_sys() which creates a new icon handle.
    updateIcon_sys();
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return true;
}

QT_END_NAMESPACE

#endif
