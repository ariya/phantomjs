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

#ifndef QSYSTEMTRAYICON_P_H
#define QSYSTEMTRAYICON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qsystemtrayicon.h"
#include "private/qobject_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include "QtGui/qmenu.h"
#include "QtGui/qpixmap.h"
#include "QtCore/qstring.h"
#include "QtCore/qpointer.h"

QT_BEGIN_NAMESPACE

class QSystemTrayIconSys;
class QToolButton;
class QLabel;

class QSystemTrayIconPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSystemTrayIcon)

public:
    QSystemTrayIconPrivate() : sys(0), visible(false) { }

    void install_sys();
    void remove_sys();
    void updateIcon_sys();
    void updateToolTip_sys();
    void updateMenu_sys();
    QRect geometry_sys() const;
    void showMessage_sys(const QString &msg, const QString &title, QSystemTrayIcon::MessageIcon icon, int secs);

    static bool isSystemTrayAvailable_sys();
    static bool supportsMessages_sys();

    QPointer<QMenu> menu;
    QIcon icon;
    QString toolTip;
    QSystemTrayIconSys *sys;
    bool visible;
};

class QBalloonTip : public QWidget
{
    Q_OBJECT
public:
    static void showBalloon(QSystemTrayIcon::MessageIcon icon, const QString& title,
                            const QString& msg, QSystemTrayIcon *trayIcon,
                            const QPoint& pos, int timeout, bool showArrow = true);
    static void hideBalloon();
    static bool isBalloonVisible();

private:
    QBalloonTip(QSystemTrayIcon::MessageIcon icon, const QString& title,
                const QString& msg, QSystemTrayIcon *trayIcon);
    ~QBalloonTip();
    void balloon(const QPoint&, int, bool);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void mousePressEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent *e);

private:
    QSystemTrayIcon *trayIcon;
    QPixmap pixmap;
    int timerId;
};

#if defined(Q_WS_X11)
QT_BEGIN_INCLUDE_NAMESPACE
#include <QtCore/qcoreapplication.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
QT_END_INCLUDE_NAMESPACE

class QSystemTrayIconSys : public QWidget
{
    friend class QSystemTrayIconPrivate;

public:
    QSystemTrayIconSys(QSystemTrayIcon *q);
    ~QSystemTrayIconSys();
    enum {
        SYSTEM_TRAY_REQUEST_DOCK = 0,
        SYSTEM_TRAY_BEGIN_MESSAGE = 1,
        SYSTEM_TRAY_CANCEL_MESSAGE =2
    };

    void addToTray();
    void updateIcon();
    XVisualInfo* getSysTrayVisualInfo();

    // QObject::event is public but QWidget's ::event() re-implementation
    // is protected ;(
    inline bool deliverToolTipEvent(QEvent *e)
    { return QWidget::event(e); }

    static Window sysTrayWindow;
    static QList<QSystemTrayIconSys *> trayIcons;
    static QCoreApplication::EventFilter oldEventFilter;
    static bool sysTrayTracker(void *message, long *result);
    static Window locateSystemTray();
    static Atom sysTraySelection;
    static XVisualInfo sysTrayVisual;

protected:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);
    bool x11Event(XEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event);
#endif
    bool event(QEvent *e);

private:
    QPixmap background;
    QSystemTrayIcon *q;
    Colormap colormap;
};
#endif // Q_WS_X11

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON

#endif // QSYSTEMTRAYICON_P_H

