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

#include "qevent.h"
#include "qwidget.h"
#include "qdesktopwidget.h"
#include "qapplication.h"
#include "qapplication_p.h"
#include "qabstracteventdispatcher.h"
#include "qnamespace.h"
#include "qpainter.h"
#include "qbitmap.h"
#include "qlayout.h"
#include "qtextcodec.h"
#include "qelapsedtimer.h"
#include "qcursor.h"
#include "qstack.h"
#include "qcolormap.h"
#include "qdebug.h"
#include "qmenu.h"
#include "private/qmenu_p.h"
#include "private/qbackingstore_p.h"
#include "private/qwindowsurface_x11_p.h"

//extern bool qt_sendSpontaneousEvent(QObject *, QEvent *); //qapplication_x11.cpp

#include <private/qpixmap_x11_p.h>
#include <private/qpaintengine_x11_p.h>
#include "qt_x11_p.h"
#include "qx11info_x11.h"

#include <stdlib.h>

//#define ALIEN_DEBUG

// defined in qapplication_x11.cpp
//bool qt_wstate_iconified(WId);
//void qt_updated_rootinfo();


#if !defined(QT_NO_IM)
#include "qinputcontext.h"
#include "qinputcontextfactory.h"
#endif

#include "qwidget_p.h"

#define XCOORD_MAX 16383
#define WRECT_MAX 8191

QT_BEGIN_NAMESPACE

extern bool qt_nograb();

QWidget *QWidgetPrivate::mouseGrabber = 0;
QWidget *QWidgetPrivate::keyboardGrabber = 0;

void qt_net_remove_user_time(QWidget *tlw);
void qt_net_update_user_time(QWidget *tlw, unsigned long timestamp);

int qt_x11_create_desktop_on_screen = -1;

extern void qt_net_update_user_time(QWidget *tlw, unsigned long timestamp);

// MWM support
struct QtMWMHints {
    ulong flags, functions, decorations;
    long input_mode;
    ulong status;
};

enum {
    MWM_HINTS_FUNCTIONS   = (1L << 0),

    MWM_FUNC_ALL      = (1L << 0),
    MWM_FUNC_RESIZE   = (1L << 1),
    MWM_FUNC_MOVE     = (1L << 2),
    MWM_FUNC_MINIMIZE = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE    = (1L << 5),

    MWM_HINTS_DECORATIONS = (1L << 1),

    MWM_DECOR_ALL      = (1L << 0),
    MWM_DECOR_BORDER   = (1L << 1),
    MWM_DECOR_RESIZEH  = (1L << 2),
    MWM_DECOR_TITLE    = (1L << 3),
    MWM_DECOR_MENU     = (1L << 4),
    MWM_DECOR_MINIMIZE = (1L << 5),
    MWM_DECOR_MAXIMIZE = (1L << 6),

    MWM_HINTS_INPUT_MODE = (1L << 2),

    MWM_INPUT_MODELESS                  = 0L,
    MWM_INPUT_PRIMARY_APPLICATION_MODAL = 1L,
    MWM_INPUT_FULL_APPLICATION_MODAL    = 3L
};


static QtMWMHints GetMWMHints(Display *display, Window window)
{
    QtMWMHints mwmhints;

    Atom type;
    int format;
    ulong nitems, bytesLeft;
    uchar *data = 0;
    if ((XGetWindowProperty(display, window, ATOM(_MOTIF_WM_HINTS), 0, 5, false,
                            ATOM(_MOTIF_WM_HINTS), &type, &format, &nitems, &bytesLeft,
                            &data) == Success)
        && (type == ATOM(_MOTIF_WM_HINTS)
            && format == 32
            && nitems >= 5)) {
        mwmhints = *(reinterpret_cast<QtMWMHints *>(data));
    } else {
        mwmhints.flags = 0L;
        mwmhints.functions = MWM_FUNC_ALL;
        mwmhints.decorations = MWM_DECOR_ALL;
        mwmhints.input_mode = 0L;
        mwmhints.status = 0L;
    }

    if (data)
        XFree(data);

    return mwmhints;
}

static void SetMWMHints(Display *display, Window window, const QtMWMHints &mwmhints)
{
    if (mwmhints.flags != 0l) {
        XChangeProperty(display, window, ATOM(_MOTIF_WM_HINTS), ATOM(_MOTIF_WM_HINTS), 32,
                        PropModeReplace, (unsigned char *) &mwmhints, 5);
    } else {
        XDeleteProperty(display, window, ATOM(_MOTIF_WM_HINTS));
    }
}

// Returns true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWidget *w)
{
    return ((w->windowType() == Qt::Dialog
             || w->windowType() == Qt::Sheet
             || w->windowType() == Qt::Tool
             || w->windowType() == Qt::SplashScreen
             || w->windowType() == Qt::ToolTip
             || w->windowType() == Qt::Drawer
             || w->windowType() == Qt::Popup)
            && !w->testAttribute(Qt::WA_X11BypassTransientForHint));
}

static void do_size_hints(QWidget* widget, QWExtra *x);

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

const uint stdWidgetEventMask =                        // X event mask
        (uint)(
            KeyPressMask | KeyReleaseMask |
            ButtonPressMask | ButtonReleaseMask |
            KeymapStateMask |
            ButtonMotionMask | PointerMotionMask |
            EnterWindowMask | LeaveWindowMask |
            FocusChangeMask |
            ExposureMask |
            PropertyChangeMask |
            StructureNotifyMask
       );

const uint stdDesktopEventMask =                        // X event mask
       (uint)(
           KeymapStateMask |
           EnterWindowMask | LeaveWindowMask |
           PropertyChangeMask
      );


/*
  The qt_ functions below are implemented in qwidgetcreate_x11.cpp.
*/

Window qt_XCreateWindow(const QWidget *creator,
                         Display *display, Window parent,
                         int x, int y, uint w, uint h,
                         int borderwidth, int depth,
                         uint windowclass, Visual *visual,
                         ulong valuemask, XSetWindowAttributes *attributes);
Window qt_XCreateSimpleWindow(const QWidget *creator,
                               Display *display, Window parent,
                               int x, int y, uint w, uint h, int borderwidth,
                               ulong border, ulong background);
void qt_XDestroyWindow(const QWidget *destroyer,
                        Display *display, Window window);


static void qt_insert_sip(QWidget* scrolled_widget, int dx, int dy)
{
    if (!scrolled_widget->isWindow() && !scrolled_widget->internalWinId())
        return;
    QX11Data::ScrollInProgress sip = { X11->sip_serial++, scrolled_widget, dx, dy };
    X11->sip_list.append(sip);

    XClientMessageEvent client_message;
    client_message.type = ClientMessage;
    client_message.window = scrolled_widget->internalWinId();
    client_message.format = 32;
    client_message.message_type = ATOM(_QT_SCROLL_DONE);
    client_message.data.l[0] = sip.id;

    XSendEvent(X11->display, scrolled_widget->internalWinId(), False, NoEventMask,
        (XEvent*)&client_message);
}

static int qt_sip_count(QWidget* scrolled_widget)
{
    int sips=0;

    for (int i = 0; i < X11->sip_list.size(); ++i) {
        const QX11Data::ScrollInProgress &sip = X11->sip_list.at(i);
        if (sip.scrolled_widget == scrolled_widget)
            sips++;
    }

    return sips;
}

static void create_wm_client_leader()
{
    if (X11->wm_client_leader) return;

    X11->wm_client_leader =
        XCreateSimpleWindow(X11->display,
                             QX11Info::appRootWindow(),
                             0, 0, 1, 1, 0, 0, 0);

    // set client leader property to itself
    XChangeProperty(X11->display,
                     X11->wm_client_leader, ATOM(WM_CLIENT_LEADER),
                     XA_WINDOW, 32, PropModeReplace,
                     (unsigned char *)&X11->wm_client_leader, 1);

#ifndef QT_NO_SESSIONMANAGER
    // If we are session managed, inform the window manager about it
    QByteArray session = qApp->sessionId().toLatin1();
    if (!session.isEmpty()) {
        XChangeProperty(X11->display,
                         X11->wm_client_leader, ATOM(SM_CLIENT_ID),
                         XA_STRING, 8, PropModeReplace,
                         (unsigned char *)session.data(), session.size());
    }
#endif
}

/*!
   \internal
   Update the X11 cursor of the widget w.
   \a force is true if this function is called from dispatchEnterLeave, it means that the
   mouse is actually directly under this widget.
 */
void qt_x11_enforce_cursor(QWidget * w, bool force)
{
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    static QPointer<QWidget> lastUnderMouse = 0;
    if (force) {
        lastUnderMouse = w;
    } else if (lastUnderMouse && lastUnderMouse->effectiveWinId() == w->effectiveWinId()) {
        w = lastUnderMouse;
    } else if (!w->internalWinId()) {
        return; //the mouse is not under this widget, and it's not native, so don't change it
    }

    while (!w->internalWinId() && w->parentWidget() && !w->isWindow() && !w->testAttribute(Qt::WA_SetCursor))
        w = w->parentWidget();

    QWidget *nativeParent = w;
    if (!w->internalWinId())
        nativeParent = w->nativeParentWidget();
    // This does the same as effectiveWinId(), but since it is possible
    // to not have a native parent widget due to a special hack in
    // qwidget for reparenting widgets to a different X11 screen,
    // added additional check to make sure native parent widget exists.
    if (!nativeParent || !nativeParent->internalWinId())
        return;
    WId winid = nativeParent->internalWinId();

    if (w->isWindow() || w->testAttribute(Qt::WA_SetCursor)) {
#ifndef QT_NO_CURSOR
        QCursor *oc = QApplication::overrideCursor();
        if (oc) {
            XDefineCursor(X11->display, winid, oc->handle());
        } else if (w->isEnabled()) {
            XDefineCursor(X11->display, winid, w->cursor().handle());
        } else {
            // enforce the windows behavior of clearing the cursor on
            // disabled widgets
            XDefineCursor(X11->display, winid, XNone);
        }
#endif
    } else {
        XDefineCursor(X11->display, winid, XNone);
    }
}

Q_GUI_EXPORT void qt_x11_enforce_cursor(QWidget * w)
{
    qt_x11_enforce_cursor(w, false);
}

void qt_x11_wait_for_window_manager(QWidget *w, bool sendPostedEvents)
{
    if (!w || (!w->isWindow() && !w->internalWinId()))
        return;
    QApplication::flush();
    XEvent ev;
    QElapsedTimer t;
    t.start();
    static const int maximumWaitTime = 2000;
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    WId winid = w->internalWinId();

    // first deliver events that are already in the local queue
    if (sendPostedEvents)
        QApplication::sendPostedEvents();

    // the normal sequence is:
    //  ... ConfigureNotify ... ReparentNotify ... MapNotify ... Expose
    // with X11BypassWindowManagerHint:
    //  ConfigureNotify ... MapNotify ... Expose

    enum State {
        Initial, Mapped
    } state = Initial;

    do {
        if (XEventsQueued(X11->display, QueuedAlready)) {
            XNextEvent(X11->display, &ev);
            // Pass the event through the event dispatcher filter so that applications
            // which install an event filter on the dispatcher get to handle it first.
            if (!QAbstractEventDispatcher::instance()->filterEvent(&ev)) {
                qApp->x11ProcessEvent(&ev);

                switch (state) {
                case Initial:
                    if (ev.type == MapNotify && ev.xany.window == winid)
                        state = Mapped;
                    break;
                case Mapped:
                    if (ev.type == Expose && ev.xany.window == winid)
                        return;
                    break;
                }
            }
        } else {
            if (!XEventsQueued(X11->display, QueuedAfterFlush))
                qApp->syncX(); // non-busy wait
        }
        if (t.elapsed() > maximumWaitTime)
            return;
    } while(1);
}

Q_GUI_EXPORT void qt_x11_wait_for_window_manager(QWidget *w)
{
    qt_x11_wait_for_window_manager(w, true);
}

void qt_change_net_wm_state(const QWidget* w, bool set, Atom one, Atom two = 0)
{
    if (!w->isVisible()) // not managed by the window manager
        return;

    XEvent e;
    e.xclient.type = ClientMessage;
    e.xclient.message_type = ATOM(_NET_WM_STATE);
    e.xclient.display = X11->display;
    e.xclient.window = w->internalWinId();
    e.xclient.format = 32;
    e.xclient.data.l[0] = set ? 1 : 0;
    e.xclient.data.l[1] = one;
    e.xclient.data.l[2] = two;
    e.xclient.data.l[3] = 0;
    e.xclient.data.l[4] = 0;
    XSendEvent(X11->display, RootWindow(X11->display, w->x11Info().screen()),
               false, (SubstructureNotifyMask | SubstructureRedirectMask), &e);
}

struct QX11WindowAttributes {
    const XWindowAttributes *att;
};

void qt_x11_getX11InfoForWindow(QX11Info * xinfo, const XWindowAttributes &a)
{
    QX11WindowAttributes att;
    att.att = &a;
    qt_x11_getX11InfoForWindow(xinfo,att);
}


static QVector<Atom> getNetWmState(QWidget *w)
{
    QVector<Atom> returnValue;

    // Don't read anything, just get the size of the property data
    Atom actualType;
    int actualFormat;
    ulong propertyLength;
    ulong bytesLeft;
    uchar *propertyData = 0;
    if (XGetWindowProperty(X11->display, w->internalWinId(), ATOM(_NET_WM_STATE), 0, 0,
                           False, XA_ATOM, &actualType, &actualFormat,
                           &propertyLength, &bytesLeft, &propertyData) == Success
        && actualType == XA_ATOM && actualFormat == 32) {
        returnValue.resize(bytesLeft / 4);
        XFree((char*) propertyData);
        propertyData = 0;

        // fetch all data
        if (XGetWindowProperty(X11->display, w->internalWinId(), ATOM(_NET_WM_STATE), 0,
                               returnValue.size(), False, XA_ATOM, &actualType, &actualFormat,
                               &propertyLength, &bytesLeft, &propertyData) != Success) {
            returnValue.clear();
        } else if (propertyLength != (ulong)returnValue.size()) {
            returnValue.resize(propertyLength);
        }

        // put it into netWmState
        if (!returnValue.isEmpty()) {
            memcpy(returnValue.data(), propertyData, returnValue.size() * sizeof(Atom));
        }
        if (propertyData)
            XFree((char*) propertyData);
    }

    return returnValue;
}

void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);
    Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::SplashScreen
                 || type == Qt::ToolTip || type == Qt::Drawer);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::create_sys START:" << q << "topLevel?" << topLevel << "WId:"
             << window << "initializeWindow:" << initializeWindow << "destroyOldWindow" << destroyOldWindow;
#endif
    if (topLevel) {
        if (parentWidget) { // if our parent stays on top, so must we
            QWidget *ptl = parentWidget->window();
            if(ptl && (ptl->windowFlags() & Qt::WindowStaysOnTopHint))
                flags |= Qt::WindowStaysOnTopHint;
        }

        if (type == Qt::SplashScreen) {
            if (X11->isSupportedByWM(ATOM(_NET_WM_WINDOW_TYPE_SPLASH))) {
                flags &= ~Qt::X11BypassWindowManagerHint;
            } else {
                flags |= Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint;
            }
        }
        // All these buttons depend on the system menu, so we enable it
        if (flags & (Qt::WindowMinimizeButtonHint
                     | Qt::WindowMaximizeButtonHint
                     | Qt::WindowContextHelpButtonHint))
            flags |= Qt::WindowSystemMenuHint;
    }


    Window parentw, destroyw = 0;
    WId id = 0;

    // always initialize
    if (!window)
        initializeWindow = true;

    QX11Info *parentXinfo = parentWidget ? &parentWidget->d_func()->xinfo : 0;

    if (desktop &&
        qt_x11_create_desktop_on_screen >= 0 &&
        qt_x11_create_desktop_on_screen != xinfo.screen()) {
        // desktop on a certain screen other than the default requested
        QX11InfoData *xd = &X11->screens[qt_x11_create_desktop_on_screen];
        xinfo.setX11Data(xd);
    } else if (parentXinfo && (parentXinfo->screen() != xinfo.screen()
                               || (parentXinfo->visual() != xinfo.visual()
                                   && !q->inherits("QGLWidget"))))
    {
        // QGLWidgets have to be excluded here as they have a
        // specially crafted QX11Info structure which can't be swapped
        // out with the parent widgets QX11Info. The parent visual,
        // for instance, might not even be GL capable.
        xinfo = *parentXinfo;
    }

    //get display, screen number, root window and desktop geometry for
    //the current screen
    Display *dpy = X11->display;
    int scr = xinfo.screen();
    Window root_win = RootWindow(dpy, scr);
    int sw = DisplayWidth(dpy,scr);
    int sh = DisplayHeight(dpy,scr);

    if (desktop) {                                // desktop widget
        popup = false;                        // force these flags off
        data.crect.setRect(0, 0, sw, sh);
    } else if (topLevel && !q->testAttribute(Qt::WA_Resized)) {
        QDesktopWidget *desktopWidget = qApp->desktop();
        if (desktopWidget->isVirtualDesktop()) {
            QRect r = desktopWidget->screenGeometry();
            sw = r.width();
            sh = r.height();
        }

        int width = sw / 2;
        int height = 4 * sh / 10;
        if (extra) {
            width = qMax(qMin(width, extra->maxw), extra->minw);
            height = qMax(qMin(height, extra->maxh), extra->minh);
        }
        data.crect.setSize(QSize(width, height));
    }

    parentw = topLevel ? root_win : parentWidget->effectiveWinId();

    XSetWindowAttributes wsa;

    if (window) {                                // override the old window
        if (destroyOldWindow) {
            if (topLevel)
                X11->dndEnable(q, false);
            destroyw = data.winid;
        }
        id = window;
        setWinId(window);
        XWindowAttributes a;
        XGetWindowAttributes(dpy, window, &a);
        data.crect.setRect(a.x, a.y, a.width, a.height);

        if (a.map_state == IsUnmapped)
            q->setAttribute(Qt::WA_WState_Visible, false);
        else
            q->setAttribute(Qt::WA_WState_Visible);

        qt_x11_getX11InfoForWindow(&xinfo,a);

    } else if (desktop) {                        // desktop widget
#ifdef QWIDGET_EXTRA_DEBUG
        qDebug() << "create desktop";
#endif
        id = (WId)parentw;                        // id = root window
//         QWidget *otherDesktop = find(id);        // is there another desktop?
//         if (otherDesktop && otherDesktop->testWFlags(Qt::WPaintDesktop)) {
//             otherDesktop->d->setWinId(0);        // remove id from widget mapper
//             d->setWinId(id);                     // make sure otherDesktop is
//             otherDesktop->d->setWinId(id);       // found first
//         } else {
        setWinId(id);
//         }
    } else if (topLevel || q->testAttribute(Qt::WA_NativeWindow) || paintOnScreen()) {
#ifdef QWIDGET_EXTRA_DEBUG
        static int topLevels = 0;
        static int children = 0;
        if (parentw == root_win)
            qDebug() << "create toplevel" << ++topLevels;
        else
            qDebug() << "create child" << ++children;
#endif
        QRect safeRect = data.crect; //##### must handle huge sizes as well.... i.e. wrect
        if (safeRect.width() < 1|| safeRect.height() < 1) {
            if (topLevel) {
                // top-levels must be at least 1x1
                safeRect.setSize(safeRect.size().expandedTo(QSize(1, 1)));
            } else {
                // create it way off screen, and rely on
                // setWSGeometry() to do the right thing with it later
                safeRect = QRect(-1000,-1000,1,1);
            }
        }
#ifndef QT_NO_XRENDER
        int screen = xinfo.screen();
        if (topLevel && X11->use_xrender
            && xinfo.depth() != 32 && X11->argbVisuals[screen]
            && q->testAttribute(Qt::WA_TranslucentBackground))
        {
            QX11InfoData *xd = xinfo.getX11Data(true);

            xd->screen = screen;
            xd->visual = X11->argbVisuals[screen];
            xd->colormap = X11->argbColormaps[screen];
            xd->depth = 32;
            xd->defaultVisual = false;
            xd->defaultColormap = false;
            xd->cells = xd->visual->map_entries;
            xinfo.setX11Data(xd);
        }
#endif
        if (xinfo.defaultVisual() && xinfo.defaultColormap()) {
            id = (WId)qt_XCreateSimpleWindow(q, dpy, parentw,
                                             safeRect.left(), safeRect.top(),
                                             safeRect.width(), safeRect.height(),
                                             0,
                                             BlackPixel(dpy, xinfo.screen()),
                                             WhitePixel(dpy, xinfo.screen()));
        } else {
            wsa.background_pixel = WhitePixel(dpy, xinfo.screen());
            wsa.border_pixel = BlackPixel(dpy, xinfo.screen());
            wsa.colormap = xinfo.colormap();
            id = (WId)qt_XCreateWindow(q, dpy, parentw,
                                       safeRect.left(), safeRect.top(),
                                       safeRect.width(), safeRect.height(),
                                       0, xinfo.depth(), InputOutput,
                                       (Visual *) xinfo.visual(),
                                       CWBackPixel|CWBorderPixel|CWColormap,
                                       &wsa);
        }

        setWinId(id);                                // set widget id/handle + hd
    }

#ifndef QT_NO_XRENDER
    if (picture) {
        XRenderFreePicture(X11->display, picture);
        picture = 0;
    }

    if (X11->use_xrender && !desktop && q->internalWinId()) {
        XRenderPictFormat *format = XRenderFindVisualFormat(dpy, (Visual *) xinfo.visual());
        if (format)
            picture = XRenderCreatePicture(dpy, id, format, 0, 0);
    }
#endif // QT_NO_XRENDER

    QtMWMHints mwmhints;
    mwmhints.flags = 0L;
    mwmhints.functions = 0L;
    mwmhints.decorations = 0;
    mwmhints.input_mode = 0L;
    mwmhints.status = 0L;

    if (topLevel) {
        ulong wsa_mask = 0;
        if (type != Qt::SplashScreen) { // && customize) {
            mwmhints.flags |= MWM_HINTS_DECORATIONS;

            bool customize = flags & Qt::CustomizeWindowHint;
            if (!(flags & Qt::FramelessWindowHint) && !(customize && !(flags & Qt::WindowTitleHint))) {
                mwmhints.decorations |= MWM_DECOR_BORDER;
                mwmhints.decorations |= MWM_DECOR_RESIZEH;

                if (flags & Qt::WindowTitleHint)
                    mwmhints.decorations |= MWM_DECOR_TITLE;

                if (flags & Qt::WindowSystemMenuHint)
                    mwmhints.decorations |= MWM_DECOR_MENU;

                if (flags & Qt::WindowMinimizeButtonHint) {
                    mwmhints.decorations |= MWM_DECOR_MINIMIZE;
                    mwmhints.functions |= MWM_FUNC_MINIMIZE;
                }

                if (flags & Qt::WindowMaximizeButtonHint) {
                    mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
                    mwmhints.functions |= MWM_FUNC_MAXIMIZE;
                }

                if (flags & Qt::WindowCloseButtonHint)
                     mwmhints.functions |= MWM_FUNC_CLOSE;
            }
        } else {
            // if type == Qt::SplashScreen
            mwmhints.decorations = MWM_DECOR_ALL;
        }

        if (tool) {
            wsa.save_under = True;
            wsa_mask |= CWSaveUnder;
        }

        if (flags & Qt::X11BypassWindowManagerHint) {
            wsa.override_redirect = True;
            wsa_mask |= CWOverrideRedirect;
        }

        if (wsa_mask && initializeWindow) {
            Q_ASSERT(id);
            XChangeWindowAttributes(dpy, id, wsa_mask, &wsa);
        }

        if (mwmhints.functions != 0) {
            mwmhints.flags |= MWM_HINTS_FUNCTIONS;
            mwmhints.functions |= MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
        } else {
            mwmhints.functions = MWM_FUNC_ALL;
        }

        if (!(flags & Qt::FramelessWindowHint)
            && flags & Qt::CustomizeWindowHint
            && flags & Qt::WindowTitleHint
            && !(flags &
                 (Qt::WindowMinimizeButtonHint
                  | Qt::WindowMaximizeButtonHint
                  | Qt::WindowCloseButtonHint))) {
            // a special case - only the titlebar without any button
            mwmhints.flags = MWM_HINTS_FUNCTIONS;
            mwmhints.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
            mwmhints.decorations = 0;
        }
    }

    if (!initializeWindow) {
        // do no initialization
    } else if (popup) {                        // popup widget
        // set EWMH window types
        setNetWmWindowTypes();

        wsa.override_redirect = True;
        wsa.save_under = True;
        Q_ASSERT(id);
        XChangeWindowAttributes(dpy, id, CWOverrideRedirect | CWSaveUnder,
                                &wsa);
    } else if (topLevel && !desktop) {        // top-level widget
        if (!X11->wm_client_leader)
            create_wm_client_leader();

        // note: WM_TRANSIENT_FOR is set in QWidgetPrivate::show_sys()

        XSizeHints size_hints;
        memset(&size_hints, 0, sizeof(size_hints));
        size_hints.flags = USSize | PSize | PWinGravity;
        size_hints.x = data.crect.left();
        size_hints.y = data.crect.top();
        size_hints.width = data.crect.width();
        size_hints.height = data.crect.height();
        size_hints.win_gravity =
            QApplication::isRightToLeft() ? NorthEastGravity : NorthWestGravity;

        XWMHints wm_hints;                        // window manager hints
        memset(&wm_hints, 0, sizeof(wm_hints)); // make valgrind happy
        wm_hints.flags = InputHint | StateHint | WindowGroupHint;
        wm_hints.input = q->testAttribute(Qt::WA_X11DoNotAcceptFocus) ? False : True;
        wm_hints.initial_state = NormalState;
        wm_hints.window_group = X11->wm_client_leader;

        XClassHint class_hint;
        QByteArray appName = qAppName().toLatin1();
        class_hint.res_name = appName.data(); // application name
        class_hint.res_class = const_cast<char *>(QX11Info::appClass());   // application class

        XSetWMProperties(dpy, id, 0, 0,
                         qApp->d_func()->argv, qApp->d_func()->argc,
                         &size_hints, &wm_hints, &class_hint);

        XResizeWindow(dpy, id,
                      qBound(1, data.crect.width(), XCOORD_MAX),
                      qBound(1, data.crect.height(), XCOORD_MAX));
        XStoreName(dpy, id, appName.data());
        Atom protocols[5];
        int n = 0;
        protocols[n++] = ATOM(WM_DELETE_WINDOW);        // support del window protocol
        protocols[n++] = ATOM(WM_TAKE_FOCUS);                // support take focus window protocol
        protocols[n++] = ATOM(_NET_WM_PING);                // support _NET_WM_PING protocol
#ifndef QT_NO_XSYNC
        protocols[n++] = ATOM(_NET_WM_SYNC_REQUEST);        // support _NET_WM_SYNC_REQUEST protocol
#endif // QT_NO_XSYNC
        if (flags & Qt::WindowContextHelpButtonHint)
            protocols[n++] = ATOM(_NET_WM_CONTEXT_HELP);
        XSetWMProtocols(dpy, id, protocols, n);

        // set mwm hints
        SetMWMHints(dpy, id, mwmhints);

        // set EWMH window types
        setNetWmWindowTypes();

        // set _NET_WM_PID
        long curr_pid = getpid();
        XChangeProperty(dpy, id, ATOM(_NET_WM_PID), XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) &curr_pid, 1);

        // when we create a toplevel widget, the frame strut should be dirty
        data.fstrut_dirty = 1;

        // declare the widget's window role
        if (QTLWExtra *topData = maybeTopData()) {
            if (!topData->role.isEmpty()) {
                QByteArray windowRole = topData->role.toUtf8();
                XChangeProperty(dpy, id,
                                ATOM(WM_WINDOW_ROLE), XA_STRING, 8, PropModeReplace,
                                (unsigned char *)windowRole.constData(), windowRole.length());
            }
        }

        // set client leader property
        XChangeProperty(dpy, id, ATOM(WM_CLIENT_LEADER),
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *)&X11->wm_client_leader, 1);
    } else {
        // non-toplevel widgets don't have a frame, so no need to
        // update the strut
        data.fstrut_dirty = 0;
    }

    if (initializeWindow && q->internalWinId()) {
        // don't erase when resizing
        wsa.bit_gravity = QApplication::isRightToLeft() ? NorthEastGravity : NorthWestGravity;
        Q_ASSERT(id);
        XChangeWindowAttributes(dpy, id, CWBitGravity, &wsa);
    }

    // set X11 event mask
    if (desktop) {
//         QWidget* main_desktop = find(id);
//         if (main_desktop->testWFlags(Qt::WPaintDesktop))
//             XSelectInput(dpy, id, stdDesktopEventMask | ExposureMask);
//         else
        XSelectInput(dpy, id, stdDesktopEventMask);
    } else if (q->internalWinId()) {
        XSelectInput(dpy, id, stdWidgetEventMask);
#if !defined (QT_NO_TABLET)
        QTabletDeviceDataList *tablet_list = qt_tablet_devices();
        if (X11->ptrXSelectExtensionEvent) {
            for (int i = 0; i < tablet_list->size(); ++i) {
                QTabletDeviceData tablet = tablet_list->at(i);
                X11->ptrXSelectExtensionEvent(dpy, id, reinterpret_cast<XEventClass*>(tablet.eventList),
                                              tablet.eventCount);
            }
        }
#endif
    }

    if (desktop) {
        q->setAttribute(Qt::WA_WState_Visible);
    } else if (topLevel) {                        // set X cursor
        if (initializeWindow) {
            qt_x11_enforce_cursor(q);

            if (QTLWExtra *topData = maybeTopData())
                if (!topData->caption.isEmpty())
                    setWindowTitle_helper(topData->caption);

            //always enable dnd: it's not worth the effort to maintain the state
            // NOTE: this always creates topData()
            X11->dndEnable(q, true);

            if (maybeTopData() && maybeTopData()->opacity != 255)
                q->setWindowOpacity(maybeTopData()->opacity/255.);

        }
    } else if (q->internalWinId()) {
        qt_x11_enforce_cursor(q);
        if (QWidget *p = q->parentWidget()) // reset the cursor on the native parent
            qt_x11_enforce_cursor(p);
    }

    if (extra && !extra->mask.isEmpty() && q->internalWinId())
        XShapeCombineRegion(X11->display, q->internalWinId(), ShapeBounding, 0, 0,
                            extra->mask.handle(), ShapeSet);

    if (q->hasFocus() && q->testAttribute(Qt::WA_InputMethodEnabled)) {
        QInputContext *inputContext = q->inputContext();
        if (inputContext)
            inputContext->setFocusWidget(q);
    }

    if (destroyw) {
        qt_XDestroyWindow(q, dpy, destroyw);
        if (QTLWExtra *topData = maybeTopData()) {
#ifndef QT_NO_XSYNC
            if (topData->syncUpdateCounter)
                XSyncDestroyCounter(dpy, topData->syncUpdateCounter);
#endif
            // we destroyed our old window - reset the top-level state
            createTLSysExtra();
        }
    }

    // newly created windows are positioned at the window system's
    // (0,0) position. If the parent uses wrect mapping to expand the
    // coordinate system, we must also adjust this widget's window
    // system position
    if (!topLevel && !parentWidget->data->wrect.topLeft().isNull())
        setWSGeometry();
    else if (topLevel && (data.crect.width() == 0 || data.crect.height() == 0))
        q->setAttribute(Qt::WA_OutsideWSRange, true);

    if (!topLevel && q->testAttribute(Qt::WA_NativeWindow) && q->testAttribute(Qt::WA_Mapped)) {
        Q_ASSERT(q->internalWinId());
        XMapWindow(X11->display, q->internalWinId());
        // Ensure that mapped alien widgets are flushed immediately when re-created as native widgets.
        if (QWindowSurface *surface = q->windowSurface())
            surface->flush(q, q->rect(), q->mapTo(surface->window(), QPoint()));
    }

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::create_sys END:" << q;
#endif
}

static void qt_x11_recreateWidget(QWidget *widget)
{
    if (widget->inherits("QGLWidget")) {
        // We send QGLWidgets a ParentChange event which causes them to
        // recreate their GL context, which in turn causes them to choose
        // their visual again. Now that WA_TranslucentBackground is set,
        // QGLContext::chooseVisual will select an ARGB visual.

        // QGLWidget expects a ParentAboutToChange to be sent first
        QEvent aboutToChangeEvent(QEvent::ParentAboutToChange);
        QApplication::sendEvent(widget, &aboutToChangeEvent);

        QEvent parentChangeEvent(QEvent::ParentChange);
        QApplication::sendEvent(widget, &parentChangeEvent);
    } else {
        // For regular widgets, reparent them with their parent which
        // also triggers a recreation of the native window
        QPoint pos = widget->pos();
        bool visible = widget->isVisible();
        if (visible)
            widget->hide();

        widget->setParent(widget->parentWidget(), widget->windowFlags());
        widget->move(pos);
        if (visible)
            widget->show();
    }
}

static void qt_x11_recreateNativeWidgetsRecursive(QWidget *widget)
{
    if (widget->internalWinId())
        qt_x11_recreateWidget(widget);

    const QObjectList &children = widget->children();
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget*>(children.at(i));
        if (child)
            qt_x11_recreateNativeWidgetsRecursive(child);
    }
}

void QWidgetPrivate::x11UpdateIsOpaque()
{
#ifndef QT_NO_XRENDER
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->testAttribute(Qt::WA_TranslucentBackground))
        return;

    bool topLevel = (data.window_flags & Qt::Window);
    int screen = xinfo.screen();
    if (topLevel && X11->use_xrender
        && X11->argbVisuals[screen] && xinfo.depth() != 32)
    {
        qt_x11_recreateNativeWidgetsRecursive(q);
    }
#endif
}

/*
  Returns true if the background is inherited; otherwise returns
  false.

  Mainly used in the paintOnScreen case.
*/
bool QWidgetPrivate::isBackgroundInherited() const
{
    Q_Q(const QWidget);

    // windows do not inherit their background
    if (q->isWindow() || q->windowType() == Qt::SubWindow)
        return false;

    if (q->testAttribute(Qt::WA_NoSystemBackground) || q->testAttribute(Qt::WA_OpaquePaintEvent))
        return false;

    const QPalette &pal = q->palette();
    QPalette::ColorRole bg = q->backgroundRole();
    QBrush brush = pal.brush(bg);

    // non opaque brushes leaves us no choice, we must inherit
    if (!q->autoFillBackground() || !brush.isOpaque())
        return true;

    if (brush.style() == Qt::SolidPattern) {
        // the background is just a solid color. If there is no
        // propagated contents, then we claim as performance
        // optimization that it was not inheritet. This is the normal
        // case in standard Windows or Motif style.
        const QWidget *w = q->parentWidget();
        if (!w->d_func()->isBackgroundInherited())
            return false;
    }

    return true;
}

void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);
    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();
    if (testAttribute(Qt::WA_WState_Created)) {
        setAttribute(Qt::WA_WState_Created, false);
        QObjectList childList = children();
        for (int i = 0; i < childList.size(); ++i) { // destroy all widget children
            register QObject *obj = childList.at(i);
            if (obj->isWidgetType())
                static_cast<QWidget*>(obj)->destroy(destroySubWindows,
                                                    destroySubWindows);
        }
        if (QWidgetPrivate::mouseGrabber == this)
            releaseMouse();
        if (QWidgetPrivate::keyboardGrabber == this)
            releaseKeyboard();
        if (isWindow())
            X11->deferred_map.removeAll(this);
        if (isModal()) {
            // just be sure we leave modal
            QApplicationPrivate::leaveModal(this);
        }
        else if ((windowType() == Qt::Popup))
            qApp->d_func()->closePopup(this);

#ifndef QT_NO_XRENDER
        if (d->picture) {
            if (destroyWindow)
                XRenderFreePicture(X11->display, d->picture);
            d->picture = 0;
        }
#endif // QT_NO_XRENDER

        // delete the _NET_WM_USER_TIME_WINDOW
        qt_net_remove_user_time(this);

        if ((windowType() == Qt::Desktop)) {
            if (acceptDrops())
                X11->dndEnable(this, false);
        } else {
            if (isWindow())
                X11->dndEnable(this, false);
            if (destroyWindow)
                qt_XDestroyWindow(this, X11->display, data->winid);
        }
        QT_TRY {
            d->setWinId(0);
        } QT_CATCH (const std::bad_alloc &) {
            // swallow - destructors must not throw
        }

        extern void qPRCleanup(QWidget *widget); // from qapplication_x11.cpp
        if (testAttribute(Qt::WA_WState_Reparented))
            qPRCleanup(this);

        if(d->ic) {
            delete d->ic;
        } else {
            // release previous focus information participating with
            // preedit preservation of qic
            QInputContext *qic = QApplicationPrivate::inputContext;
            if (qic)
                qic->widgetDestroyed(this);
        }
    }
}

void QWidgetPrivate::setParent_sys(QWidget *parent, Qt::WindowFlags f)
{
    Q_Q(QWidget);
#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::setParent_sys START" << q << "parent:" << parent;
#endif
    QX11Info old_xinfo = xinfo;
    if (parent && parent->windowType() == Qt::Desktop) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget
        xinfo = parent->d_func()->xinfo;
        parent = 0;
    }

    QTLWExtra *topData = maybeTopData();
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));
    extern void qPRCreate(const QWidget *, Window);
#ifndef QT_NO_CURSOR
    QCursor oldcurs;
#endif

    // dnd unregister (we will register again below)
    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        q->setAttribute(Qt::WA_DropSiteRegistered, false);

    // if we are a top then remove our dnd prop for now
    // it will get rest later
    if (q->isWindow() && wasCreated)
        X11->dndEnable(q, false);

    if (topData)
        qt_net_remove_user_time(q);

//     QWidget *oldparent = q->parentWidget();
    WId old_winid = wasCreated ? data.winid : 0;
    if ((q->windowType() == Qt::Desktop))
        old_winid = 0;
    setWinId(0);

#ifndef QT_NO_XRENDER
    if (picture) {
        XRenderFreePicture(X11->display, picture);
        picture = 0;
    }
#endif

    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if (wasCreated && old_winid) {
        XUnmapWindow(X11->display, old_winid);
        if (!old_xinfo.screen() != xinfo.screen())
            XReparentWindow(X11->display, old_winid, RootWindow(X11->display, xinfo.screen()), 0, 0);
    }
    if (topData) {
        topData->parentWinId = 0;
        // zero the frame strut and mark it dirty
        topData->frameStrut.setCoords(0, 0, 0, 0);

        // reparenting from top-level, make sure show() works again
        topData->waitingForMapNotify = 0;
        topData->validWMState = 0;
    }
    data.fstrut_dirty = (!parent || (f & Qt::Window)); // toplevels get a dirty framestrut

    QObjectPrivate::setParent_helper(parent);
    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
    // keep compatibility with previous versions, we need to preserve the created state
    // (but we recreate the winId for the widget being reparented, again for compatibility)
    if (wasCreated)
        createWinId();
    if (q->isWindow() || (!parent || parent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    if (wasCreated) {
        QObjectList chlist = q->children();
        for (int i = 0; i < chlist.size(); ++i) { // reparent children
            QObject *obj = chlist.at(i);
            if (obj->isWidgetType()) {
                QWidget *w = (QWidget *)obj;
                if (!w->testAttribute(Qt::WA_WState_Created))
                    continue;
                if (xinfo.screen() != w->d_func()->xinfo.screen()) {
                    // ### force setParent() to not shortcut out (because
                    // ### we're setting the parent to the current parent)
                    // ### setParent will add child back to the list
                    // ### of children so we need to make sure the
                    // ### widget won't be added twice.
                    w->d_func()->parent = 0;
                    this->children.removeOne(w);
                    w->setParent(q);
                } else if (!w->isWindow()) {
                    w->d_func()->invalidateBuffer(w->rect());
                    if (w->internalWinId()) {
                        if (w->testAttribute(Qt::WA_NativeWindow)) {
                            QWidget *nativeParentWidget = w->nativeParentWidget();
                            // Qt::WA_NativeWindow ensures that we always have a nativeParentWidget
                            Q_ASSERT(nativeParentWidget != 0);
                            QPoint p = w->mapTo(nativeParentWidget, QPoint());
                            XReparentWindow(X11->display,
                                            w->internalWinId(),
                                            nativeParentWidget->internalWinId(),
                                            p.x(), p.y());
                        } else {
                            w->d_func()->setParent_sys(q, w->data->window_flags);
                        }
                    }
                } else if (isTransient(w)) {
                    /*
                      when reparenting toplevel windows with toplevel-transient children,
                      we need to make sure that the window manager gets the updated
                      WM_TRANSIENT_FOR information... unfortunately, some window managers
                      don't handle changing WM_TRANSIENT_FOR before the toplevel window is
                      visible, so we unmap and remap all toplevel-transient children *after*
                      the toplevel parent has been mapped.  thankfully, this is easy in Qt :)

                      note that the WM_TRANSIENT_FOR hint is actually updated in
                      QWidgetPrivate::show_sys()
                    */
                    if (w->internalWinId())
                        XUnmapWindow(X11->display, w->internalWinId());
                    QApplication::postEvent(w, new QEvent(QEvent::ShowWindowRequest));
                }
            }
        }
        qPRCreate(q, old_winid);
        updateSystemBackground();

        if (old_winid) {
            Window *cmwret;
            int count;
            if (XGetWMColormapWindows(X11->display, old_winid, &cmwret, &count)) {
                Window *cmw;
                int cmw_size = sizeof(Window)*count;
                cmw = new Window[count];
                memcpy((char *)cmw, (char *)cmwret, cmw_size);
                XFree((char *)cmwret);
                int i;
                for (i=0; i<count; i++) {
                    if (cmw[i] == old_winid) {
                        cmw[i] = q->internalWinId();
                        break;
                    }
                }
                int top_count;
                if (XGetWMColormapWindows(X11->display, q->window()->internalWinId(),
                                          &cmwret, &top_count))
                {
                    Window *merged_cmw = new Window[count + top_count];
                    memcpy((char *)merged_cmw, (char *)cmw, cmw_size);
                    memcpy((char *)merged_cmw + cmw_size, (char *)cmwret, sizeof(Window)*top_count);
                    delete [] cmw;
                    XFree((char *)cmwret);
                    cmw = merged_cmw;
                    count += top_count;
                }

                XSetWMColormapWindows(X11->display, q->window()->internalWinId(), cmw, count);
                delete [] cmw;
            }

            qt_XDestroyWindow(q, X11->display, old_winid);
        }
    }

    // check if we need to register our dropsite
    if (q->testAttribute(Qt::WA_AcceptDrops)
        || (!q->isWindow() && q->parentWidget() && q->parentWidget()->testAttribute(Qt::WA_DropSiteRegistered))) {
        q->setAttribute(Qt::WA_DropSiteRegistered, true);
    }
#if !defined(QT_NO_IM)
    ic = 0;
#endif
    invalidateBuffer(q->rect());
#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::setParent_sys END" << q;
#endif
}

QPoint QWidgetPrivate::mapToGlobal(const QPoint &pos) const
{
    Q_Q(const QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->internalWinId()) {
        QPoint p = pos + q->data->crect.topLeft();
        //cannot trust that !isWindow() implies parentWidget() before create
        return (q->isWindow() || !q->parentWidget()) ?  p : q->parentWidget()->d_func()->mapToGlobal(p);
    }
    int x, y;
    Window child;
    QPoint p = mapToWS(pos);
    XTranslateCoordinates(X11->display, q->internalWinId(),
                          QApplication::desktop()->screen(xinfo.screen())->internalWinId(),
                          p.x(), p.y(), &x, &y, &child);
    return QPoint(x, y);
}

QPoint QWidgetPrivate::mapFromGlobal(const QPoint &pos) const
{
    Q_Q(const QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->internalWinId()) {
        //cannot trust that !isWindow() implies parentWidget() before create
        QPoint p = (q->isWindow() || !q->parentWidget()) ?  pos : q->parentWidget()->d_func()->mapFromGlobal(pos);
        return p - q->data->crect.topLeft();
    }
    int x, y;
    Window child;
    XTranslateCoordinates(X11->display,
                          QApplication::desktop()->screen(xinfo.screen())->internalWinId(),
                          q->internalWinId(), pos.x(), pos.y(), &x, &y, &child);
    return mapFromWS(QPoint(x, y));
}

QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    return d->mapToGlobal(pos);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    return d->mapFromGlobal(pos);
}

void QWidgetPrivate::updateSystemBackground()
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->internalWinId())
        return;
    QBrush brush = q->palette().brush(QPalette::Active, q->backgroundRole());
    Qt::WindowType type = q->windowType();
    if (brush.style() == Qt::NoBrush
        || q->testAttribute(Qt::WA_NoSystemBackground)
        || q->testAttribute(Qt::WA_UpdatesDisabled)
        || type == Qt::Popup || type == Qt::ToolTip) {
            if (QX11Info::isCompositingManagerRunning()
                && q->testAttribute(Qt::WA_TranslucentBackground)
                && !(q->parent()))
                XSetWindowBackground(X11->display, q->internalWinId(),
                                     QColormap::instance(xinfo.screen()).pixel(Qt::transparent));
            else
                XSetWindowBackgroundPixmap(X11->display, q->internalWinId(), XNone);
        }
    else if (brush.style() == Qt::SolidPattern && brush.isOpaque())
        XSetWindowBackground(X11->display, q->internalWinId(),
                             QColormap::instance(xinfo.screen()).pixel(brush.color()));
    else if (isBackgroundInherited())
        XSetWindowBackgroundPixmap(X11->display, q->internalWinId(), ParentRelative);
    else if (brush.style() == Qt::TexturePattern) {
        extern QPixmap qt_toX11Pixmap(const QPixmap &pixmap); // qpixmap_x11.cpp
        XSetWindowBackgroundPixmap(X11->display, q->internalWinId(),
                                   static_cast<QX11PixmapData*>(qt_toX11Pixmap(brush.texture()).data.data())->x11ConvertToDefaultDepth());
    } else
        XSetWindowBackground(X11->display, q->internalWinId(),
                             QColormap::instance(xinfo.screen()).pixel(brush.color()));
}

#ifndef QT_NO_CURSOR
void QWidgetPrivate::setCursor_sys(const QCursor &)
{
    Q_Q(QWidget);
    qt_x11_enforce_cursor(q);
    XFlush(X11->display);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_x11_enforce_cursor(q);
    XFlush(X11->display);
}
#endif

static XTextProperty*
qstring_to_xtp(const QString& s)
{
    static XTextProperty tp = { 0, 0, 0, 0 };
    static bool free_prop = true; // we can't free tp.value in case it references
    // the data of the static QCString below.
    if (tp.value) {
        if (free_prop)
            XFree(tp.value);
        tp.value = 0;
        free_prop = true;
    }

    static const QTextCodec* mapper = QTextCodec::codecForLocale();
    int errCode = 0;
    if (mapper) {
        QByteArray mapped = mapper->fromUnicode(s);
        char* tl[2];
        tl[0] = mapped.data();
        tl[1] = 0;
        errCode = XmbTextListToTextProperty(X11->display, tl, 1, XStdICCTextStyle, &tp);
#if defined(QT_DEBUG)
        if (errCode < 0)
            qDebug("qstring_to_xtp result code %d", errCode);
#endif
    }
    if (!mapper || errCode < 0) {
        static QByteArray qcs;
        qcs = s.toAscii();
        tp.value = (uchar*)qcs.data();
        tp.encoding = XA_STRING;
        tp.format = 8;
        tp.nitems = qcs.length();
        free_prop = false;
    }

    // ### If we knew WM could understand unicode, we could use
    // ### a much simpler, cheaper encoding...
    /*
        tp.value = (XChar2b*)s.unicode();
        tp.encoding = XA_UNICODE; // wish
        tp.format = 16;
        tp.nitems = s.length();
    */

    return &tp;
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (!q->internalWinId())
        return;
    XSetWMName(X11->display, q->internalWinId(), qstring_to_xtp(caption));

    QByteArray net_wm_name = caption.toUtf8();
    XChangeProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_NAME), ATOM(UTF8_STRING), 8,
                    PropModeReplace, (unsigned char *)net_wm_name.data(), net_wm_name.size());
}

void QWidgetPrivate::setWindowIcon_sys(bool forceReset)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
    QTLWExtra *topData = this->topData();
    if (topData->iconPixmap && !forceReset)
        // already been set
        return;

    // preparing images to set the _NET_WM_ICON property
    QIcon icon = q->windowIcon();
    QVector<long> icon_data;
    Qt::HANDLE pixmap_handle = 0;
    if (!icon.isNull()) {
        QList<QSize> availableSizes = icon.availableSizes();
        if(availableSizes.isEmpty()) {
            // try to use default sizes since the icon can be a scalable image like svg.
            availableSizes.push_back(QSize(16,16));
            availableSizes.push_back(QSize(32,32));
            availableSizes.push_back(QSize(64,64));
            availableSizes.push_back(QSize(128,128));
        }
        for(int i = 0; i < availableSizes.size(); ++i) {
            QSize size = availableSizes.at(i);
            QPixmap pixmap = icon.pixmap(size);
            if (!pixmap.isNull()) {
                QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
                int pos = icon_data.size();
                icon_data.resize(pos + 2 + image.width()*image.height());
                icon_data[pos++] = image.width();
                icon_data[pos++] = image.height();
                if (sizeof(long) == sizeof(quint32)) {
                    memcpy(icon_data.data() + pos, image.scanLine(0), image.byteCount());
                } else {
                    for (int y = 0; y < image.height(); ++y) {
                        uint *scanLine = reinterpret_cast<uint *>(image.scanLine(y));
                        for (int x = 0; x < image.width(); ++x)
                            icon_data[pos + y*image.width() + x] = scanLine[x];
                    }
                }
            }
        }
        if (!icon_data.isEmpty()) {
            extern QPixmap qt_toX11Pixmap(const QPixmap &pixmap);
            /*
              if the app is running on an unknown desktop, or it is not
              using the default visual, convert the icon to 1bpp as stated
              in the ICCCM section 4.1.2.4; otherwise, create the icon pixmap
              in the default depth (even though this violates the ICCCM)
            */
            if (X11->desktopEnvironment == DE_UNKNOWN
                || !QX11Info::appDefaultVisual(xinfo.screen())
                || !QX11Info::appDefaultColormap(xinfo.screen())) {
                // unknown DE or non-default visual/colormap, use 1bpp bitmap
                if (!forceReset || !topData->iconPixmap)
                    topData->iconPixmap = new QPixmap(qt_toX11Pixmap(QBitmap(icon.pixmap(QSize(64,64)))));
                pixmap_handle = topData->iconPixmap->handle();
            } else {
                // default depth, use a normal pixmap (even though this
                // violates the ICCCM), since this works on all DEs known to Qt
                if (!forceReset || !topData->iconPixmap)
                    topData->iconPixmap = new QPixmap(qt_toX11Pixmap(icon.pixmap(QSize(64,64))));
                pixmap_handle = static_cast<QX11PixmapData*>(topData->iconPixmap->data.data())->x11ConvertToDefaultDepth();
            }
        }
    }

    if (!q->internalWinId())
        return;

    if (!icon_data.isEmpty()) {
        XChangeProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_ICON), XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) icon_data.data(),
                        icon_data.size());
    } else {
        XDeleteProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_ICON));
    }

    XWMHints *h = XGetWMHints(X11->display, q->internalWinId());
    XWMHints wm_hints;
    if (!h) {
        memset(&wm_hints, 0, sizeof(wm_hints)); // make valgrind happy
        h = &wm_hints;
    }

    if (pixmap_handle) {
        h->icon_pixmap = pixmap_handle;
        h->flags |= IconPixmapHint;
    } else {
        h->icon_pixmap = 0;
        h->flags &= ~(IconPixmapHint | IconMaskHint);
    }

    XSetWMHints(X11->display, q->internalWinId(), h);
    if (h != &wm_hints)
        XFree((char *)h);
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    XSetWMIconName(X11->display, q->internalWinId(), qstring_to_xtp(iconText));

    QByteArray icon_name = iconText.toUtf8();
    XChangeProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_ICON_NAME), ATOM(UTF8_STRING), 8,
                    PropModeReplace, (unsigned char *) icon_name.constData(), icon_name.size());
}


void QWidget::grabMouse()
{
    if (isVisible() && !qt_nograb()) {
        if (QWidgetPrivate::mouseGrabber && QWidgetPrivate::mouseGrabber != this)
            QWidgetPrivate::mouseGrabber->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
#ifndef QT_NO_DEBUG
        int status =
#endif
            XGrabPointer(X11->display, effectiveWinId(), False,
                          (uint)(ButtonPressMask | ButtonReleaseMask |
                                  PointerMotionMask | EnterWindowMask |
                                  LeaveWindowMask),
                          GrabModeAsync, GrabModeAsync,
                          XNone, XNone, X11->time);
#ifndef QT_NO_DEBUG
        if (status) {
            const char *s =
                status == GrabNotViewable ? "\"GrabNotViewable\"" :
                status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
                status == GrabFrozen      ? "\"GrabFrozen\"" :
                status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
                "<?>";
            qWarning("QWidget::grabMouse: Failed with %s", s);
        }
#endif
        QWidgetPrivate::mouseGrabber = this;
    }
}


#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    if (!qt_nograb()) {
        if (QWidgetPrivate::mouseGrabber && QWidgetPrivate::mouseGrabber != this)
            QWidgetPrivate::mouseGrabber->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
#ifndef QT_NO_DEBUG
        int status =
#endif
        XGrabPointer(X11->display, effectiveWinId(), False,
                      (uint)(ButtonPressMask | ButtonReleaseMask |
                             PointerMotionMask | EnterWindowMask | LeaveWindowMask),
                      GrabModeAsync, GrabModeAsync,
                      XNone, cursor.handle(), X11->time);
#ifndef QT_NO_DEBUG
        if (status) {
            const char *s =
                status == GrabNotViewable ? "\"GrabNotViewable\"" :
                status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
                status == GrabFrozen      ? "\"GrabFrozen\"" :
                status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
                                            "<?>";
            qWarning("QWidget::grabMouse: Failed with %s", s);
        }
#endif
        QWidgetPrivate::mouseGrabber = this;
    }
}
#endif


void QWidget::releaseMouse()
{
    if (!qt_nograb() && QWidgetPrivate::mouseGrabber == this) {
        XUngrabPointer(X11->display, X11->time);
        XFlush(X11->display);
        QWidgetPrivate::mouseGrabber = 0;
    }
}


void QWidget::grabKeyboard()
{
    if (!qt_nograb()) {
        if (QWidgetPrivate::keyboardGrabber && QWidgetPrivate::keyboardGrabber != this)
            QWidgetPrivate::keyboardGrabber->releaseKeyboard();
        XGrabKeyboard(X11->display, effectiveWinId(), False, GrabModeAsync, GrabModeAsync,
                      X11->time);
        QWidgetPrivate::keyboardGrabber = this;
    }
}


void QWidget::releaseKeyboard()
{
    if (!qt_nograb() && QWidgetPrivate::keyboardGrabber == this) {
        XUngrabKeyboard(X11->display, X11->time);
        QWidgetPrivate::keyboardGrabber = 0;
    }
}


QWidget *QWidget::mouseGrabber()
{
    return QWidgetPrivate::mouseGrabber;
}


QWidget *QWidget::keyboardGrabber()
{
    return QWidgetPrivate::keyboardGrabber;
}

void QWidget::activateWindow()
{
    QWidget *tlw = window();
    if (tlw->isVisible() && !tlw->d_func()->topData()->embedded && !X11->deferred_map.contains(tlw)) {
        if (X11->userTime == 0)
            X11->userTime = X11->time;
        qt_net_update_user_time(tlw, X11->userTime);

        if (X11->isSupportedByWM(ATOM(_NET_ACTIVE_WINDOW))
            && !(tlw->windowFlags() & Qt::X11BypassWindowManagerHint)) {
            XEvent e;
            e.xclient.type = ClientMessage;
            e.xclient.message_type = ATOM(_NET_ACTIVE_WINDOW);
            e.xclient.display = X11->display;
            e.xclient.window = tlw->internalWinId();
            e.xclient.format = 32;
            e.xclient.data.l[0] = 1;     // 1 == application
            e.xclient.data.l[1] = X11->userTime;
            if (QWidget *aw = QApplication::activeWindow())
                e.xclient.data.l[2] = aw->internalWinId();
            else
                e.xclient.data.l[2] = XNone;
            e.xclient.data.l[3] = 0;
            e.xclient.data.l[4] = 0;
            XSendEvent(X11->display, RootWindow(X11->display, tlw->x11Info().screen()),
                       false, SubstructureNotifyMask | SubstructureRedirectMask, &e);
        } else {
            if (!qt_widget_private(tlw)->topData()->waitingForMapNotify)
                XSetInputFocus(X11->display, tlw->internalWinId(), XRevertToParent, X11->time);
        }
    }
}

void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    bool needShow = false;
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;
    if (isWindow()) {
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        QTLWExtra *top = d->topData();

        if ((oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized)) {
            if (X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))
                && X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_VERT))) {
                if ((newstate & Qt::WindowMaximized) && !(oldstate & Qt::WindowFullScreen))
                    top->normalGeometry = geometry();
                qt_change_net_wm_state(this, (newstate & Qt::WindowMaximized),
                                       ATOM(_NET_WM_STATE_MAXIMIZED_HORZ),
                                       ATOM(_NET_WM_STATE_MAXIMIZED_VERT));
            } else if (! (newstate & Qt::WindowFullScreen)) {
                if (newstate & Qt::WindowMaximized) {
                    // save original geometry
                    const QRect normalGeometry = geometry();

                    if (isVisible()) {
                        data->fstrut_dirty = true;
                        const QRect maxRect = QApplication::desktop()->availableGeometry(this);
                        const QRect r = top->normalGeometry;
                        const QRect fs = d->frameStrut();
                        setGeometry(maxRect.x() + fs.left(),
                                    maxRect.y() + fs.top(),
                                    maxRect.width() - fs.left() - fs.right(),
                                    maxRect.height() - fs.top() - fs.bottom());
                        top->normalGeometry = r;
                    }

                    if (top->normalGeometry.width() < 0)
                        top->normalGeometry = normalGeometry;
                } else {
                    // restore original geometry
                    setGeometry(top->normalGeometry);
                }
            }
        }

        if ((oldstate & Qt::WindowFullScreen) != (newstate & Qt::WindowFullScreen)) {
            if (X11->isSupportedByWM(ATOM(_NET_WM_STATE_FULLSCREEN))) {
                if (newstate & Qt::WindowFullScreen) {
                    top->normalGeometry = geometry();
                    top->fullScreenOffset = d->frameStrut().topLeft();
                }
                qt_change_net_wm_state(this, (newstate & Qt::WindowFullScreen),
                                       ATOM(_NET_WM_STATE_FULLSCREEN));
            } else {
                needShow = isVisible();

                if (newstate & Qt::WindowFullScreen) {
                    data->fstrut_dirty = true;
                    const QRect normalGeometry = geometry();
                    const QPoint fullScreenOffset = d->frameStrut().topLeft();

                    top->savedFlags = windowFlags();
                    setParent(0, Qt::Window | Qt::FramelessWindowHint);
                    const QRect r = top->normalGeometry;
                    setGeometry(qApp->desktop()->screenGeometry(this));
                    top->normalGeometry = r;

                    if (top->normalGeometry.width() < 0) {
                        top->normalGeometry = normalGeometry;
                        top->fullScreenOffset = fullScreenOffset;
                    }
                } else {
                    setParent(0, top->savedFlags);

                    if (newstate & Qt::WindowMaximized) {
                        // from fullscreen to maximized
                        data->fstrut_dirty = true;
                        const QRect maxRect = QApplication::desktop()->availableGeometry(this);
                        const QRect r = top->normalGeometry;
                        const QRect fs = d->frameStrut();
                        setGeometry(maxRect.x() + fs.left(),
                                    maxRect.y() + fs.top(),
                                    maxRect.width() - fs.left() - fs.right(),
                                    maxRect.height() - fs.top() - fs.bottom());
                        top->normalGeometry = r;
                    } else {
                        // restore original geometry
                        setGeometry(top->normalGeometry.adjusted(-top->fullScreenOffset.x(),
                                                                 -top->fullScreenOffset.y(),
                                                                 -top->fullScreenOffset.x(),
                                                                 -top->fullScreenOffset.y()));
                    }
                }
            }
        }

        createWinId();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        if ((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (isVisible()) {
                if (newstate & Qt::WindowMinimized) {
                    XEvent e;
                    e.xclient.type = ClientMessage;
                    e.xclient.message_type = ATOM(WM_CHANGE_STATE);
                    e.xclient.display = X11->display;
                    e.xclient.window = data->winid;
                    e.xclient.format = 32;
                    e.xclient.data.l[0] = IconicState;
                    e.xclient.data.l[1] = 0;
                    e.xclient.data.l[2] = 0;
                    e.xclient.data.l[3] = 0;
                    e.xclient.data.l[4] = 0;
                    XSendEvent(X11->display,
                               RootWindow(X11->display,d->xinfo.screen()),
                               False, (SubstructureNotifyMask|SubstructureRedirectMask), &e);
                } else {
                    setAttribute(Qt::WA_Mapped);
                    XMapWindow(X11->display, effectiveWinId());
                }
            }

            needShow = false;
        }
    }

    data->window_state = newstate;

    if (needShow)
        show();

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}

/*!
  \internal
  Platform-specific part of QWidget::show().
*/

void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        q->setAttribute(Qt::WA_Mapped);
        if (QTLWExtra *tlwExtra = maybeTopData())
            tlwExtra->waitingForMapNotify = 0;
        return;
    }

    if (q->isWindow()) {
        XWMHints *h = XGetWMHints(X11->display, q->internalWinId());
        XWMHints  wm_hints;
        bool got_hints = h != 0;
        if (!got_hints) {
            memset(&wm_hints, 0, sizeof(wm_hints)); // make valgrind happy
            h = &wm_hints;
        }
        h->initial_state = q->isMinimized() ? IconicState : NormalState;
        h->flags |= StateHint;
        XSetWMHints(X11->display, q->internalWinId(), h);
        if (got_hints)
            XFree((char *)h);

        // update WM_NORMAL_HINTS
        do_size_hints(q, extra);

        // udpate WM_TRANSIENT_FOR
        if (isTransient(q)) {
            QWidget *p = q->parentWidget();

#ifndef QT_NO_MENU
            // hackish ... try to find the main window related to this QMenu
            if (qobject_cast<QMenu *>(q)) {
                p = static_cast<QMenuPrivate*>(this)->causedPopup.widget;
                if (!p)
                    p = q->parentWidget();
                if (!p)
                    p = QApplication::widgetAt(q->pos());
                if (!p)
                    p = qApp->activeWindow();
            }
#endif
            if (p)
                p = p->window();
            if (p) {
                // transient for window
                XSetTransientForHint(X11->display, q->internalWinId(), p->internalWinId());
            } else {
                // transient for group
                XSetTransientForHint(X11->display, q->internalWinId(), X11->wm_client_leader);
            }
        }

        // update _MOTIF_WM_HINTS
        QtMWMHints mwmhints = GetMWMHints(X11->display, q->internalWinId());

        if (data.window_modality != Qt::NonModal) {
            switch (data.window_modality) {
            case Qt::WindowModal:
                mwmhints.input_mode = MWM_INPUT_PRIMARY_APPLICATION_MODAL;
                break;
            case Qt::ApplicationModal:
            default:
                mwmhints.input_mode = MWM_INPUT_FULL_APPLICATION_MODAL;
                break;
            }
            mwmhints.flags |= MWM_HINTS_INPUT_MODE;
        } else {
            mwmhints.input_mode = MWM_INPUT_MODELESS;
            mwmhints.flags &= ~MWM_HINTS_INPUT_MODE;
        }

        if (q->minimumSize() == q->maximumSize()) {
            // fixed size, remove the resize handle (since mwm/dtwm
            // isn't smart enough to do it itself)
            mwmhints.flags |= MWM_HINTS_FUNCTIONS;
            if (mwmhints.functions == MWM_FUNC_ALL) {
                mwmhints.functions = MWM_FUNC_MOVE;
            } else {
                mwmhints.functions &= ~MWM_FUNC_RESIZE;
            }

            if (mwmhints.decorations == MWM_DECOR_ALL) {
                mwmhints.flags |= MWM_HINTS_DECORATIONS;
                mwmhints.decorations = (MWM_DECOR_BORDER
                                        | MWM_DECOR_TITLE
                                        | MWM_DECOR_MENU);
            } else {
                mwmhints.decorations &= ~MWM_DECOR_RESIZEH;
            }

            if (q->windowFlags() & Qt::WindowMinimizeButtonHint) {
                mwmhints.flags |= MWM_HINTS_DECORATIONS;
                mwmhints.decorations |= MWM_DECOR_MINIMIZE;
                mwmhints.functions |= MWM_FUNC_MINIMIZE;
            }
            if (q->windowFlags() & Qt::WindowMaximizeButtonHint) {
                mwmhints.flags |= MWM_HINTS_DECORATIONS;
                mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
                mwmhints.functions |= MWM_FUNC_MAXIMIZE;
            }
            if (q->windowFlags() & Qt::WindowCloseButtonHint)
                mwmhints.functions |= MWM_FUNC_CLOSE;
        }

        SetMWMHints(X11->display, q->internalWinId(), mwmhints);

        // update _NET_WM_STATE
        QVector<Atom> netWmState = getNetWmState(q);

        Qt::WindowFlags flags = q->windowFlags();
        if (flags & Qt::WindowStaysOnTopHint) {
            if (flags & Qt::WindowStaysOnBottomHint)
                qWarning() << "QWidget: Incompatible window flags: the window can't be on top and on bottom at the same time";
            if (!netWmState.contains(ATOM(_NET_WM_STATE_ABOVE)))
                netWmState.append(ATOM(_NET_WM_STATE_ABOVE));
            if (!netWmState.contains(ATOM(_NET_WM_STATE_STAYS_ON_TOP)))
                netWmState.append(ATOM(_NET_WM_STATE_STAYS_ON_TOP));
        } else if (flags & Qt::WindowStaysOnBottomHint) {
            if (!netWmState.contains(ATOM(_NET_WM_STATE_BELOW)))
                netWmState.append(ATOM(_NET_WM_STATE_BELOW));
        }
        if (q->isFullScreen()) {
            if (!netWmState.contains(ATOM(_NET_WM_STATE_FULLSCREEN)))
                netWmState.append(ATOM(_NET_WM_STATE_FULLSCREEN));
        }
        if (q->isMaximized()) {
            if (!netWmState.contains(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ)))
                netWmState.append(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ));
            if (!netWmState.contains(ATOM(_NET_WM_STATE_MAXIMIZED_VERT)))
                netWmState.append(ATOM(_NET_WM_STATE_MAXIMIZED_VERT));
        }
        if (data.window_modality != Qt::NonModal) {
            if (!netWmState.contains(ATOM(_NET_WM_STATE_MODAL)))
                netWmState.append(ATOM(_NET_WM_STATE_MODAL));
        }

        if (!netWmState.isEmpty()) {
            XChangeProperty(X11->display, q->internalWinId(),
                            ATOM(_NET_WM_STATE), XA_ATOM, 32, PropModeReplace,
                            (unsigned char *) netWmState.data(), netWmState.size());
        } else {
            XDeleteProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_STATE));
        }

        // set _NET_WM_USER_TIME
        Time userTime = X11->userTime;
        bool setUserTime = false;
        if (q->testAttribute(Qt::WA_ShowWithoutActivating)) {
            userTime = 0;
            setUserTime = true;
        } else if (userTime != CurrentTime) {
            setUserTime = true;
        }
        if (setUserTime)
            qt_net_update_user_time(q, userTime);

#ifndef QT_NO_XSYNC
        if (!topData()->syncUpdateCounter) {
            XSyncValue value;
            XSyncIntToValue(&value, 0);
            topData()->syncUpdateCounter = XSyncCreateCounter(X11->display, value);

            XChangeProperty(X11->display, q->internalWinId(),
                            ATOM(_NET_WM_SYNC_REQUEST_COUNTER),
                            XA_CARDINAL,
                            32, PropModeReplace,
                            (uchar *) &topData()->syncUpdateCounter, 1);

            topData()->newCounterValueHi = 0;
            topData()->newCounterValueLo = 0;
        }
#endif

        if (!topData()->embedded
            && (topData()->validWMState || topData()->waitingForMapNotify)
            && !q->isMinimized()) {
            X11->deferred_map.append(q);
            return;
        }

        if (q->isMaximized() && !q->isFullScreen()
            && !(X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))
                 && X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_VERT)))) {
            XMapWindow(X11->display, q->internalWinId());
            data.fstrut_dirty = true;
            qt_x11_wait_for_window_manager(q);

            // if the wm was not smart enough to adjust our size, do that manually
            QRect maxRect = QApplication::desktop()->availableGeometry(q);

            QTLWExtra *top = topData();
            QRect normalRect = top->normalGeometry;
            const QRect fs = frameStrut();

            q->setGeometry(maxRect.x() + fs.left(),
                           maxRect.y() + fs.top(),
                           maxRect.width() - fs.left() - fs.right(),
                           maxRect.height() - fs.top() - fs.bottom());

            // restore the original normalGeometry
            top->normalGeometry = normalRect;
            // internalSetGeometry() clears the maximized flag... make sure we set it back
            data.window_state = data.window_state | Qt::WindowMaximized;
            q->setAttribute(Qt::WA_Mapped);
            return;
        }

        if (q->isFullScreen() && !X11->isSupportedByWM(ATOM(_NET_WM_STATE_FULLSCREEN))) {
            XMapWindow(X11->display, q->internalWinId());
            qt_x11_wait_for_window_manager(q);
            q->setAttribute(Qt::WA_Mapped);
            return;
        }
    }

    invalidateBuffer(q->rect());

    if (q->testAttribute(Qt::WA_OutsideWSRange))
        return;
    q->setAttribute(Qt::WA_Mapped);
    if (q->isWindow())
        topData()->waitingForMapNotify = 1;

    if (!q->isWindow()
        && (!q->autoFillBackground()
            || q->palette().brush(q->backgroundRole()).style() == Qt::LinearGradientPattern)) {
        if (q->internalWinId()) {
            XSetWindowBackgroundPixmap(X11->display, q->internalWinId(), XNone);
            XMapWindow(X11->display, q->internalWinId());
            updateSystemBackground();
        }
        return;
    }

    if (q->internalWinId())
        XMapWindow(X11->display, q->internalWinId());

    // Freedesktop.org Startup Notification
    if (X11->startupId && q->isWindow()) {
        QByteArray message("remove: ID=");
        message.append(X11->startupId);
        sendStartupMessage(message.constData());
        X11->startupId = 0;
    }
}

/*!
  \internal
  Platform-specific part of QWidget::show().
*/

void QWidgetPrivate::sendStartupMessage(const char *message) const
{
    Q_Q(const QWidget);

    if (!message)
        return;

    XEvent xevent;
    xevent.xclient.type = ClientMessage;
    xevent.xclient.message_type = ATOM(_NET_STARTUP_INFO_BEGIN);
    xevent.xclient.display = X11->display;
    xevent.xclient.window = q->internalWinId();
    xevent.xclient.format = 8;

    Window rootWindow = RootWindow(X11->display, DefaultScreen(X11->display));
    uint sent = 0;
    uint length = strlen(message) + 1;
    do {
        if (sent == 20)
            xevent.xclient.message_type = ATOM(_NET_STARTUP_INFO);

        for (uint i = 0; i < 20 && i + sent <= length; i++)
            xevent.xclient.data.b[i] = message[i + sent++];

        XSendEvent(X11->display, rootWindow, false, PropertyChangeMask, &xevent);
    } while (sent <= length);
}

void QWidgetPrivate::setNetWmWindowTypes()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (!q->isWindow()) {
        if (q->internalWinId())
            XDeleteProperty(X11->display, q->internalWinId(), ATOM(_NET_WM_WINDOW_TYPE));
        return;
    }

    QVector<long> windowTypes;

    // manual selection 1 (these are never set by Qt and take precedence)
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDesktop))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DESKTOP));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDock))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DOCK));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeNotification))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_NOTIFICATION));

    // manual selection 2 (Qt uses these during auto selection);
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeUtility))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_UTILITY));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeSplash))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_SPLASH));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDialog))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DIALOG));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeToolTip))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_TOOLTIP));

    // manual selection 3 (these can be set by Qt, but don't have a
    // corresponding Qt::WindowType). note that order of the *MENU
    // atoms is important
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeMenu))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_MENU));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDropDownMenu))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypePopupMenu))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_POPUP_MENU));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeToolBar))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_TOOLBAR));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeCombo))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_COMBO));
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDND))
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DND));

    // automatic selection
    switch (q->windowType()) {
    case Qt::Dialog:
    case Qt::Sheet:
        // dialog netwm type
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_DIALOG));
        break;

    case Qt::Tool:
    case Qt::Drawer:
        // utility netwm type
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_UTILITY));
        break;

    case Qt::ToolTip:
        // tooltip netwm type
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_TOOLTIP));
        break;

    case Qt::SplashScreen:
        // splash netwm type
        windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_SPLASH));
        break;

    default:
        break;
    }

    if (q->windowFlags() & Qt::FramelessWindowHint) {
        // override netwm type - quick and easy for KDE noborder
        windowTypes.append(ATOM(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));
    }

    // normal netwm type - default
    windowTypes.append(ATOM(_NET_WM_WINDOW_TYPE_NORMAL));

    if (!windowTypes.isEmpty()) {
        XChangeProperty(X11->display, q->winId(), ATOM(_NET_WM_WINDOW_TYPE), XA_ATOM, 32,
                        PropModeReplace, (unsigned char *) windowTypes.constData(),
                        windowTypes.count());
    } else {
        XDeleteProperty(X11->display, q->winId(), ATOM(_NET_WM_WINDOW_TYPE));
    }
}

/*!
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    deactivateWidgetCleanup();
    if (q->isWindow()) {
        X11->deferred_map.removeAll(q);
        if (q->internalWinId()) // in nsplugin, may be 0
            XWithdrawWindow(X11->display, q->internalWinId(), xinfo.screen());
        XFlush(X11->display);
    } else {
        invalidateBuffer(q->rect());
        if (q->internalWinId()) // in nsplugin, may be 0
            XUnmapWindow(X11->display, q->internalWinId());
    }
    q->setAttribute(Qt::WA_Mapped, false);
}

void QWidgetPrivate::setFocus_sys()
{

}


void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        XRaiseWindow(X11->display, q->internalWinId());
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        XLowerWindow(X11->display, q->internalWinId());
    if(!q->isWindow())
        invalidateBuffer(q->rect());
}

void QWidgetPrivate::stackUnder_sys(QWidget* w)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId() && w->internalWinId()) {
        Window stack[2];
        stack[0] = w->internalWinId();;
        stack[1] = q->internalWinId();
        XRestackWindows(X11->display, stack, 2);
    }
    if(!q->isWindow() || !w->internalWinId())
        invalidateBuffer(q->rect());
}


static void do_size_hints(QWidget* widget, QWExtra *x)
{
    Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
    XSizeHints s;
    memset(&s, 0, sizeof(s));
    if (x) {
        QRect g = widget->geometry();
        s.x = g.x();
        s.y = g.y();
        s.width = g.width();
        s.height = g.height();
        if (x->minw > 0 || x->minh > 0) {
            // add minimum size hints
            s.flags |= PMinSize;
            s.min_width  = qMin(XCOORD_MAX, x->minw);
            s.min_height = qMin(XCOORD_MAX, x->minh);
        }
        if (x->maxw < QWIDGETSIZE_MAX || x->maxh < QWIDGETSIZE_MAX) {
            // add maximum size hints
            s.flags |= PMaxSize;
            s.max_width  = qMin(XCOORD_MAX, x->maxw);
            s.max_height = qMin(XCOORD_MAX, x->maxh);
        }
        if (x->topextra &&
            (x->topextra->incw > 0 || x->topextra->inch > 0)) {
            // add resize increment hints
            s.flags |= PResizeInc | PBaseSize;
            s.width_inc = x->topextra->incw;
            s.height_inc = x->topextra->inch;
            s.base_width = x->topextra->basew;
            s.base_height = x->topextra->baseh;
        }
    }
    if (widget->testAttribute(Qt::WA_Moved)) {
        // user (i.e. command-line) specified position
        s.flags |= USPosition;
        s.flags |= PPosition;
    }
    if (widget->testAttribute(Qt::WA_Resized)) {
        // user (i.e. command-line) specified size
        s.flags |= USSize;
        s.flags |= PSize;
    }
    s.flags |= PWinGravity;
    if (widget->testAttribute(Qt::WA_Moved) && x && x->topextra && !x->topextra->posFromMove) {
        // position came from setGeometry(), tell the WM that we don't
        // want our window gravity-shifted
        s.win_gravity = StaticGravity;
    } else {
        // position came from move()
        s.x = widget->x();
        s.y = widget->y();
        s.win_gravity = QApplication::isRightToLeft() ? NorthEastGravity : NorthWestGravity;
    }
    if (widget->internalWinId())
        XSetWMNormalHints(X11->display, widget->internalWinId(), &s);
}


/*
  Helper function for non-toplevel widgets. Helps to map Qt's 32bit
  coordinate system to X11's 16bit coordinate system.

  Sets the geometry of the widget to data.crect, but clipped to sizes
  that X can handle. Unmaps widgets that are completely outside the
  valid range.

  Maintains data.wrect, which is the geometry of the X widget,
  measured in this widget's coordinate system.

  if the parent is not clipped, parentWRect is empty, otherwise
  parentWRect is the geometry of the parent's X rect, measured in
  parent's coord sys
 */
void QWidgetPrivate::setWSGeometry(bool dontShow, const QRect &)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    /*
      There are up to four different coordinate systems here:
      Qt coordinate system for this widget.
      X coordinate system for this widget (relative to wrect).
      Qt coordinate system for parent
      X coordinate system for parent (relative to parent's wrect).
     */
    Display *dpy = xinfo.display();
    QRect validRange(-XCOORD_MAX,-XCOORD_MAX, 2*XCOORD_MAX, 2*XCOORD_MAX);
    QRect wrectRange(-WRECT_MAX,-WRECT_MAX, 2*WRECT_MAX, 2*WRECT_MAX);
    QRect wrect;
    //xrect is the X geometry of my X widget. (starts out in  parent's Qt coord sys, and ends up in parent's X coord sys)
    QRect xrect = data.crect;

    const QWidget *const parent = q->parentWidget();
    QRect parentWRect = parent->data->wrect;

    if (parentWRect.isValid()) {
        // parent is clipped, and we have to clip to the same limit as parent
        if (!parentWRect.contains(xrect)) {
            xrect &= parentWRect;
            wrect = xrect;
            //translate from parent's to my Qt coord sys
            wrect.translate(-data.crect.topLeft());
        }
        //translate from parent's Qt coords to parent's X coords
        xrect.translate(-parentWRect.topLeft());

    } else {
        // parent is not clipped, we may or may not have to clip

        if (data.wrect.isValid() && QRect(QPoint(),data.crect.size()).contains(data.wrect)) {
            // This is where the main optimization is: we are already
            // clipped, and if our clip is still valid, we can just
            // move our window, and do not need to move or clip
            // children

            QRect vrect = xrect & parent->rect();
            vrect.translate(-data.crect.topLeft()); //the part of me that's visible through parent, in my Qt coords
            if (data.wrect.contains(vrect)) {
                xrect = data.wrect;
                xrect.translate(data.crect.topLeft());
                if (data.winid)
                    XMoveWindow(dpy, data.winid, xrect.x(), xrect.y());
                return;
            }
        }

        if (!validRange.contains(xrect)) {
            // we are too big, and must clip
            xrect &=wrectRange;
            wrect = xrect;
            wrect.translate(-data.crect.topLeft());
            //parent's X coord system is equal to parent's Qt coord
            //sys, so we don't need to map xrect.
        }

    }

    // unmap if we are outside the valid window system coord system
    bool outsideRange = !xrect.isValid();
    bool mapWindow = false;
    if (q->testAttribute(Qt::WA_OutsideWSRange) != outsideRange) {
        q->setAttribute(Qt::WA_OutsideWSRange, outsideRange);
        if (outsideRange) {
            if (data.winid)
                XUnmapWindow(dpy, data.winid);
            q->setAttribute(Qt::WA_Mapped, false);
        } else if (!q->isHidden()) {
            mapWindow = true;
        }
    }

    if (outsideRange)
        return;

    bool jump = (data.wrect != wrect);
    data.wrect = wrect;


    // and now recursively for all children...
    // ### can be optimized
    for (int i = 0; i < children.size(); ++i) {
        QObject *object = children.at(i);
        if (object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            if (!w->isWindow() && w->testAttribute(Qt::WA_WState_Created))
                w->d_func()->setWSGeometry(jump);
        }
    }

    if (data.winid) {
        // move ourselves to the new position and map (if necessary) after
        // the movement. Rationale: moving unmapped windows is much faster
        // than moving mapped windows
        if (jump) //avoid flicker when jumping
            XSetWindowBackgroundPixmap(dpy, data.winid, XNone);
        if (!parent->internalWinId())
            xrect.translate(parent->mapTo(q->nativeParentWidget(), QPoint(0, 0)));
        XMoveResizeWindow(dpy, data.winid, xrect.x(), xrect.y(), xrect.width(), xrect.height());
    }

    //to avoid flicker, we have to show children after the helper widget has moved
    if (jump) {
        for (int i = 0; i < children.size(); ++i) {
            QObject *object = children.at(i);
            if (object->isWidgetType()) {
                QWidget *w = static_cast<QWidget *>(object);
                if (!w->testAttribute(Qt::WA_OutsideWSRange) && !w->testAttribute(Qt::WA_Mapped) && !w->isHidden()) {
                    w->setAttribute(Qt::WA_Mapped);
                    if (w->internalWinId())
                        XMapWindow(dpy, w->data->winid);
                }
            }
        }
    }


    if  (jump && data.winid)
        XClearArea(dpy, data.winid, 0, 0, wrect.width(), wrect.height(), True);

    if (mapWindow && !dontShow) {
        q->setAttribute(Qt::WA_Mapped);
        if (data.winid)
            XMapWindow(dpy, data.winid);
    }
}

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    Display *dpy = X11->display;

    if ((q->windowType() == Qt::Desktop))
        return;
    if (q->isWindow()) {
        if (!X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_VERT))
            && !X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ)))
            data.window_state &= ~Qt::WindowMaximized;
        if (!X11->isSupportedByWM(ATOM(_NET_WM_STATE_FULLSCREEN)))
            data.window_state &= ~Qt::WindowFullScreen;
        if (QTLWExtra *topData = maybeTopData())
            topData->normalGeometry = QRect(0,0,-1,-1);
    } else {
        uint s = data.window_state;
        s &= ~(Qt::WindowMaximized | Qt::WindowFullScreen);
        data.window_state = s;
    }
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }
    QPoint oldPos(q->pos());
    QSize oldSize(q->size());
    QRect oldGeom(data.crect);
    QRect  r(x, y, w, h);

    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if (!q->isWindow() && oldGeom == r)
        return;

    data.crect = r;
    bool isResize = q->size() != oldSize;

    if (q->isWindow()) {
        if (w == 0 || h == 0) {
            q->setAttribute(Qt::WA_OutsideWSRange, true);
            if (q->isVisible() && q->testAttribute(Qt::WA_Mapped))
                hide_sys();
        } else if (q->isVisible() && q->testAttribute(Qt::WA_OutsideWSRange)) {
            q->setAttribute(Qt::WA_OutsideWSRange, false);

            // put the window in its place and show it
            if (data.winid)
                XMoveResizeWindow(dpy, data.winid, x, y, w, h);
            topData()->posFromMove = false; // force StaticGravity
            do_size_hints(q, extra);
            show_sys();
        } else {
            q->setAttribute(Qt::WA_OutsideWSRange, false);
            if (!q->isVisible())
                do_size_hints(q, extra);
            if (isMove) {
                if ((data.window_flags & Qt::X11BypassWindowManagerHint) == Qt::X11BypassWindowManagerHint
                    // work around 4Dwm's incompliance with ICCCM 4.1.5
                    || X11->desktopEnvironment == DE_4DWM) {
                    if (data.winid)
                        XMoveResizeWindow(dpy, data.winid, x, y, w, h);
                } else if (q->isVisible()
                           && topData()->validWMState
                           && X11->isSupportedByWM(ATOM(_NET_MOVERESIZE_WINDOW))) {
                    XEvent e;
                    e.xclient.type = ClientMessage;
                    e.xclient.message_type = ATOM(_NET_MOVERESIZE_WINDOW);
                    e.xclient.display = X11->display;
                    e.xclient.window = q->internalWinId();
                    e.xclient.format = 32;
                    e.xclient.data.l[0] = StaticGravity | 1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12;
                    e.xclient.data.l[1] = x;
                    e.xclient.data.l[2] = y;
                    e.xclient.data.l[3] = w;
                    e.xclient.data.l[4] = h;
                    XSendEvent(X11->display, RootWindow(X11->display, q->x11Info().screen()),
                               false, (SubstructureNotifyMask | SubstructureRedirectMask), &e);
                } else if (data.winid) {
                    // pos() is right according to ICCCM 4.1.5
                    XMoveResizeWindow(dpy, data.winid, q->pos().x(), q->pos().y(), w, h);
                }
            } else if (isResize && data.winid) {
                if (!q->isVisible()
                    && topData()->validWMState
                    && !q->testAttribute(Qt::WA_PendingMoveEvent)) {
                    /*
                       even though we've not visible, we could be in a
                       race w/ the window manager, and it may ignore
                       our ConfigureRequest. setting posFromMove to
                       false makes sure that doDeferredMap() in
                       qapplication_x11.cpp keeps the window in the
                       right place
                    */
                    topData()->posFromMove = false;
                }
                XResizeWindow(dpy, data.winid, w, h);
            }
        }
        if (isResize && !q->testAttribute(Qt::WA_DontShowOnScreen)) // set config pending only on resize, see qapplication_x11.cpp, translateConfigEvent()
            q->setAttribute(Qt::WA_WState_ConfigPending);

    } else {
        QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
        const bool inTopLevelResize = tlwExtra ? tlwExtra->inTopLevelResize : false;
        const bool disableInTopLevelResize = inTopLevelResize && q->internalWinId();
        if (disableInTopLevelResize) {
            // Top-level resize optimization does not work for native child widgets;
            // disable it for this particular widget.
            tlwExtra->inTopLevelResize = false;
        }

        if (!isResize && (!inTopLevelResize || disableInTopLevelResize) && q->isVisible()) {
            moveRect(QRect(oldPos, oldSize), x - oldPos.x(), y - oldPos.y());
        }
        if (q->testAttribute(Qt::WA_WState_Created))
            setWSGeometry();

        if (isResize && (!inTopLevelResize || disableInTopLevelResize) && q->isVisible())
            invalidateBuffer_resizeHelper(oldPos, oldSize);

        if (disableInTopLevelResize)
            tlwExtra->inTopLevelResize = true;
    }

    if (q->isVisible()) {
        if (isMove && q->pos() != oldPos) {
            if (X11->desktopEnvironment != DE_4DWM) {
                // pos() is right according to ICCCM 4.1.5
                QMoveEvent e(q->pos(), oldPos);
                QApplication::sendEvent(q, &e);
            } else {
                // work around 4Dwm's incompliance with ICCCM 4.1.5
                QMoveEvent e(data.crect.topLeft(), oldGeom.topLeft());
                QApplication::sendEvent(q, &e);
            }
        }
        if (isResize) {
            static bool slowResize = qgetenv("QT_SLOW_TOPLEVEL_RESIZE").toInt();
            // If we have a backing store with static contents, we have to disable the top-level
            // resize optimization in order to get invalidated regions for resized widgets.
            // The optimization discards all invalidateBuffer() calls since we're going to
            // repaint everything anyways, but that's not the case with static contents.
            const bool setTopLevelResize = !slowResize && q->isWindow() && extra && extra->topextra
                                           && !extra->topextra->inTopLevelResize
                                           && (!extra->topextra->backingStore
                                               || !extra->topextra->backingStore->hasStaticContents());
            if (setTopLevelResize)
                extra->topextra->inTopLevelResize = true;
            QResizeEvent e(q->size(), oldSize);
            QApplication::sendEvent(q, &e);
            if (setTopLevelResize)
                extra->topextra->inTopLevelResize = false;
        }
    } else {
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }
}

void QWidgetPrivate::setConstraints_sys()
{
    Q_Q(QWidget);
#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::setConstraints_sys START" << q;
#endif
    if (q->testAttribute(Qt::WA_WState_Created)) {
        do_size_hints(q, extra);
        QtMWMHints mwmHints = GetMWMHints(X11->display, q->internalWinId());
        const bool wasFuncResize = mwmHints.functions & MWM_FUNC_RESIZE;
        if (q->minimumSize() == q->maximumSize())
            mwmHints.functions &= ~MWM_FUNC_RESIZE;
        else
            mwmHints.functions |= MWM_FUNC_RESIZE;
        if (wasFuncResize != (mwmHints.functions & MWM_FUNC_RESIZE))
            SetMWMHints(X11->display, q->internalWinId(), mwmHints);
    }
#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::setConstraints_sys END" << q;
#endif
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    Q_Q(QWidget);

    scrollChildren(dx, dy);
    if (!paintOnScreen()) {
        scrollRect(q->rect(), dx, dy);
    } else {
        scroll_sys(dx, dy, QRect());
    }
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    Q_Q(QWidget);

    if (!paintOnScreen()) {
        scrollRect(r, dx, dy);
        return;
    }
    bool valid_rect = r.isValid();
    bool just_update = qAbs(dx) > q->width() || qAbs(dy) > q->height();
    QRect sr = valid_rect ? r : clipRect();
    if (just_update)
        q->update();
    else if (!valid_rect)
        dirty.translate(dx, dy);

    int x1, y1, x2, y2, w = sr.width(), h = sr.height();
    if (dx > 0) {
        x1 = sr.x();
        x2 = x1+dx;
        w -= dx;
    } else {
        x2 = sr.x();
        x1 = x2-dx;
        w += dx;
    }
    if (dy > 0) {
        y1 = sr.y();
        y2 = y1+dy;
        h -= dy;
    } else {
        y2 = sr.y();
        y1 = y2-dy;
        h += dy;
    }

    if (dx == 0 && dy == 0)
        return;

    Display *dpy = X11->display;
    // Want expose events
    if (w > 0 && h > 0 && !just_update && q->internalWinId()) {
        GC gc = XCreateGC(dpy, q->internalWinId(), 0, 0);
        XSetGraphicsExposures(dpy, gc, True);
        XCopyArea(dpy, q->internalWinId(), q->internalWinId(), gc, x1, y1, w, h, x2, y2);
        XFreeGC(dpy, gc);
    }

    if (!valid_rect && !children.isEmpty()) {        // scroll children
        QPoint pd(dx, dy);
        for (int i = 0; i < children.size(); ++i) { // move all children
            register QObject *object = children.at(i);
            if (object->isWidgetType()) {
                QWidget *w = static_cast<QWidget *>(object);
                if (!w->isWindow())
                    w->move(w->pos() + pd);
            }
        }
    }

    if (just_update)
        return;

    // Don't let the server be bogged-down with repaint events
    bool repaint_immediately = (qt_sip_count(q) < 3 && !q->testAttribute(Qt::WA_WState_InPaintEvent));

    if (dx) {
        int x = x2 == sr.x() ? sr.x()+w : sr.x();
        if (repaint_immediately)
            q->repaint(x, sr.y(), qAbs(dx), sr.height());
        else if (q->internalWinId())
            XClearArea(dpy, data.winid, x, sr.y(), qAbs(dx), sr.height(), True);
    }
    if (dy) {
        int y = y2 == sr.y() ? sr.y()+h : sr.y();
        if (repaint_immediately)
            q->repaint(sr.x(), y, sr.width(), qAbs(dy));
        else if (q->internalWinId())
            XClearArea(dpy, data.winid, sr.x(), y, sr.width(), qAbs(dy), True);
    }

    qt_insert_sip(q, dx, dy); // #### ignores r
}

int QWidget::metric(PaintDeviceMetric m) const
{
    Q_D(const QWidget);
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else {
        Display *dpy = X11->display;
        int scr = d->xinfo.screen();
        switch (m) {
            case PdmDpiX:
            case PdmPhysicalDpiX:
                if (d->extra && d->extra->customDpiX)
                    val = d->extra->customDpiX;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else
                    val = QX11Info::appDpiX(scr);
                break;
            case PdmDpiY:
            case PdmPhysicalDpiY:
                if (d->extra && d->extra->customDpiY)
                    val = d->extra->customDpiY;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else
                    val = QX11Info::appDpiY(scr);
                break;
            case PdmWidthMM:
                val = (DisplayWidthMM(dpy,scr)*data->crect.width())/
                      DisplayWidth(dpy,scr);
                break;
            case PdmHeightMM:
                val = (DisplayHeightMM(dpy,scr)*data->crect.height())/
                      DisplayHeight(dpy,scr);
                break;
            case PdmNumColors:
                val = d->xinfo.cells();
                break;
            case PdmDepth:
                val = d->xinfo.depth();
                break;
            default:
                val = 0;
                qWarning("QWidget::metric: Invalid metric command");
        }
    }
    return val;
}

void QWidgetPrivate::createSysExtra()
{
    extra->compress_events = true;
    extra->xDndProxy = 0;
}

void QWidgetPrivate::deleteSysExtra()
{
}

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->spont_unmapped = 0;
    extra->topextra->dnd = 0;
    extra->topextra->validWMState = 0;
    extra->topextra->waitingForMapNotify = 0;
    extra->topextra->parentWinId = 0;
    extra->topextra->userTimeWindow = 0;
#ifndef QT_NO_XSYNC
    extra->topextra->syncUpdateCounter = 0;
    extra->topextra->syncRequestTimestamp = 0;
    extra->topextra->newCounterValueHi = 0;
    extra->topextra->newCounterValueLo = 0;
#endif
}

void QWidgetPrivate::deleteTLSysExtra()
{
    // don't destroy input context here. it will be destroyed in
    // QWidget::destroy() destroyInputContext();
#ifndef QT_NO_XSYNC
    if (extra && extra->topextra && extra->topextra->syncUpdateCounter) {
        XSyncDestroyCounter(X11->display, extra->topextra->syncUpdateCounter);
        extra->topextra->syncUpdateCounter = 0;
    }
#endif
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_UNUSED(on);
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;

    if (region.isEmpty()) {
        XShapeCombineMask(X11->display, q->internalWinId(), ShapeBounding, 0, 0,
                          XNone, ShapeSet);
    } else {
        XShapeCombineRegion(X11->display, q->internalWinId(), ShapeBounding, 0, 0,
                            region.handle(), ShapeSet);
    }
}

/*!
  \internal

  Computes the frame rectangle when needed.  This is an internal function, you
  should never call this.
*/

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);

    QTLWExtra *top = topData();
    if (!top->validWMState) {
        return;
    }
    if (!q->isWindow() && !q->internalWinId()) {
        data.fstrut_dirty = false;
        return;
    }

    Atom type_ret;
    Window l = q->effectiveWinId(), w = l, p, r; // target window, its parent, root
    Window *c;
    int i_unused;
    unsigned int nc;
    unsigned char *data_ret;
    unsigned long l_unused;

    while (XQueryTree(X11->display, w, &r, &p, &c, &nc)) {
        if (c && nc > 0)
            XFree(c);

        if (! p) {
            qWarning("QWidget::updateFrameStrut: No parent");
            return;
        }

        // if the parent window is the root window, an Enlightenment virtual root or
        // a NET WM virtual root window, stop here
        data_ret = 0;
        if (p == r ||
            (XGetWindowProperty(X11->display, p,
                                ATOM(ENLIGHTENMENT_DESKTOP), 0, 1, False, XA_CARDINAL,
                                &type_ret, &i_unused, &l_unused, &l_unused,
                                &data_ret) == Success &&
             type_ret == XA_CARDINAL)) {
            if (data_ret)
                XFree(data_ret);

            break;
        } else if (X11->isSupportedByWM(ATOM(_NET_VIRTUAL_ROOTS)) && X11->net_virtual_root_list) {
            int i = 0;
            while (X11->net_virtual_root_list[i] != 0) {
                if (X11->net_virtual_root_list[i++] == p)
                    break;
            }
        }

        l = w;
        w = p;
    }

    // we have our window
    int transx, transy;
    XWindowAttributes wattr;
    if (XTranslateCoordinates(X11->display, l, w,
                              0, 0, &transx, &transy, &p) &&
        XGetWindowAttributes(X11->display, w, &wattr)) {
        top->frameStrut.setCoords(transx,
                                  transy,
                                  wattr.width - data.crect.width() - transx,
                                  wattr.height - data.crect.height() - transy);

        // add the border_width for the window managers frame... some window managers
        // do not use a border_width of zero for their frames, and if we the left and
        // top strut, we ensure that pos() is absolutely correct.  frameGeometry()
        // will still be incorrect though... perhaps i should have foffset as well, to
        // indicate the frame offset (equal to the border_width on X).
        // - Brad
        top->frameStrut.adjust(wattr.border_width,
                               wattr.border_width,
                               wattr.border_width,
                               wattr.border_width);
    }

   data.fstrut_dirty = false;
}

void QWidgetPrivate::setWindowOpacity_sys(qreal opacity)
{
    Q_Q(QWidget);
    ulong value = ulong(opacity * 0xffffffff);
    XChangeProperty(QX11Info::display(), q->internalWinId(), ATOM(_NET_WM_WINDOW_OPACITY), XA_CARDINAL,
                    32, PropModeReplace, (uchar*)&value, 1);
}

const QX11Info &QWidget::x11Info() const
{
    Q_D(const QWidget);
    return d->xinfo;
}

void QWidgetPrivate::setWindowRole()
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;
    QByteArray windowRole = topData()->role.toUtf8();
    XChangeProperty(X11->display, q->internalWinId(),
                    ATOM(WM_WINDOW_ROLE), XA_STRING, 8, PropModeReplace,
                    (unsigned char *)windowRole.constData(), windowRole.length());
}

Q_GLOBAL_STATIC(QX11PaintEngine, qt_widget_paintengine)
QPaintEngine *QWidget::paintEngine() const
{
    Q_D(const QWidget);
    if (qt_widget_paintengine()->isActive()) {
        if (d->extraPaintEngine)
            return d->extraPaintEngine;
        QWidget *self = const_cast<QWidget *>(this);
        self->d_func()->extraPaintEngine = new QX11PaintEngine();
        return d->extraPaintEngine;
    }
    return qt_widget_paintengine();
}

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface_sys()
{
    return new QX11WindowSurface(q_func());
}

Qt::HANDLE QWidget::x11PictureHandle() const
{
#ifndef QT_NO_XRENDER
    Q_D(const QWidget);
    if (!internalWinId() && testAttribute(Qt::WA_WState_Created))
        (void)winId(); // enforce native window
    return d->picture;
#else
    return 0;
#endif // QT_NO_XRENDER
}

#ifndef QT_NO_XRENDER
XRenderColor QX11Data::preMultiply(const QColor &c)
{
    XRenderColor color;
    const uint A = c.alpha(),
               R = c.red(),
               G = c.green(),
               B = c.blue();
    color.alpha = (A | A << 8);
    color.red   = (R | R << 8) * color.alpha / 0x10000;
    color.green = (G | G << 8) * color.alpha / 0x10000;
    color.blue  = (B | B << 8) * color.alpha / 0x10000;
    return color;
}
Picture QX11Data::getSolidFill(int screen, const QColor &c)
{
    if (!X11->use_xrender)
        return XNone;

    XRenderColor color = preMultiply(c);
    for (int i = 0; i < X11->solid_fill_count; ++i) {
        if (X11->solid_fills[i].screen == screen
            && X11->solid_fills[i].color.alpha == color.alpha
            && X11->solid_fills[i].color.red == color.red
            && X11->solid_fills[i].color.green == color.green
            && X11->solid_fills[i].color.blue == color.blue)
            return X11->solid_fills[i].picture;
    }
    // none found, replace one
    int i = qrand() % 16;

    if (X11->solid_fills[i].screen != screen && X11->solid_fills[i].picture) {
        XRenderFreePicture (X11->display, X11->solid_fills[i].picture);
        X11->solid_fills[i].picture = 0;
    }

    if (!X11->solid_fills[i].picture) {
        Pixmap pixmap = XCreatePixmap (X11->display, RootWindow (X11->display, screen), 1, 1, 32);
        XRenderPictureAttributes attrs;
        attrs.repeat = True;
        X11->solid_fills[i].picture = XRenderCreatePicture (X11->display, pixmap,
                                                            XRenderFindStandardFormat(X11->display, PictStandardARGB32),
                                                            CPRepeat, &attrs);
        XFreePixmap (X11->display, pixmap);
    }

    X11->solid_fills[i].color = color;
    X11->solid_fills[i].screen = screen;
    XRenderFillRectangle (X11->display, PictOpSrc, X11->solid_fills[i].picture, &color, 0, 0, 1, 1);
    return X11->solid_fills[i].picture;
}
#endif

void QWidgetPrivate::setModal_sys()
{
}

void qt_x11_getX11InfoForWindow(QX11Info * xinfo, const QX11WindowAttributes &att)
{
    QX11InfoData* xd = xinfo->getX11Data(true);
    const XWindowAttributes &a = *(att.att);
    // find which screen the window is on...
    xd->screen = QX11Info::appScreen(); // by default, use the default :)
    int i;
    for (i = 0; i < ScreenCount(X11->display); i++) {
        if (RootWindow(X11->display, i) == a.root) {
            xd->screen = i;
            break;
        }
    }

    xd->depth = a.depth;
    xd->cells = DisplayCells(X11->display, xd->screen);
    xd->visual = a.visual;
    xd->defaultVisual = (XVisualIDFromVisual((Visual *) a.visual) ==
                         XVisualIDFromVisual((Visual *) QX11Info::appVisual(xinfo->screen())));
    xd->colormap = a.colormap;
    xd->defaultColormap = (a.colormap == QX11Info::appColormap(xinfo->screen()));
    xinfo->setX11Data(xd);
}

void QWidgetPrivate::updateX11AcceptFocus()
{
    Q_Q(QWidget);
    if (!q->isWindow() || !q->internalWinId())
        return;

    XWMHints *h = XGetWMHints(X11->display, q->internalWinId());
    XWMHints wm_hints;
    if (!h) {
        memset(&wm_hints, 0, sizeof(wm_hints)); // make valgrind happy
        h = &wm_hints;
    }
    h->flags |= InputHint;
    h->input = q->testAttribute(Qt::WA_X11DoNotAcceptFocus) ? False : True;

    XSetWMHints(X11->display, q->internalWinId(), h);
    if (h != &wm_hints)
        XFree((char *)h);
}

QT_END_NAMESPACE
