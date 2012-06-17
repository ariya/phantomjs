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

// ### 4.0: examine Q_EXPORT's below. The respective symbols had all
// been in use (e.g. in the KDE wm) before the introduction of a version
// map. One might want to turn some of them into proper public API and
// provide a proper alternative for others. See also the exports in
// qapplication_win.cpp, which suggest a unification.

#include "qplatformdefs.h"

#include "qcolormap.h"
#include "qdesktopwidget.h"
#include "qapplication.h"
#include "qapplication_p.h"
#include "qcursor.h"
#include "qwidget.h"
#include "qbitarray.h"
#include "qpainter.h"
#include "qfile.h"
#include "qpixmapcache.h"
#include "qdatetime.h"
#include "qtextcodec.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qsocketnotifier.h"
#include "qsessionmanager.h"
#include "qclipboard.h"
#include "qwhatsthis.h"
#include "qsettings.h"
#include "qstylefactory.h"
#include "qfileinfo.h"
#include "qdir.h"
#include "qhash.h"
#include "qevent.h"
#include "qevent_p.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include <private/qcrashhandler_p.h>
#include <private/qcolor_p.h>
#include <private/qcursor_p.h>
#include <private/qiconloader_p.h>
#include <qgtkstyle.h>
#include "qstyle.h"
#include "qmetaobject.h"
#include "qtimer.h"
#include "qlibrary.h"
#include <private/qgraphicssystemfactory_p.h>
#include "qguiplatformplugin_p.h"
#include "qkde_p.h"

#if !defined (QT_NO_TABLET)
extern "C" {
#   define class c_class  //XIproto.h has a name member named 'class' which the c++ compiler doesn't like
#   include <wacomcfg.h>
#   undef class
}
#endif

#ifndef QT_GUI_DOUBLE_CLICK_RADIUS
#define QT_GUI_DOUBLE_CLICK_RADIUS 5
#endif


//#define ALIEN_DEBUG

#if !defined(QT_NO_GLIB)
#  include "qguieventdispatcher_glib_p.h"
#endif
#include "qeventdispatcher_x11_p.h"
#include <private/qpaintengine_x11_p.h>

#include <private/qkeymapper_p.h>

// Input method stuff
#ifndef QT_NO_IM
#include "qinputcontext.h"
#include "qinputcontextfactory.h"
#endif // QT_NO_IM

#ifndef QT_NO_XFIXES
#include <X11/extensions/Xfixes.h>
#endif // QT_NO_XFIXES

#include "qt_x11_p.h"
#include "qx11info_x11.h"

#define XK_MISCELLANY
#include <X11/keysymdef.h>
#if !defined(QT_NO_XINPUT)
#include <X11/extensions/XI.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#include "qwidget_p.h"

#include <private/qbackingstore_p.h>

#ifdef QT_RX71_MULTITOUCH
#  include <qsocketnotifier.h>
#  include <linux/input.h>
#  include <errno.h>
#endif

#if _POSIX_VERSION+0 < 200112L && !defined(Q_OS_BSD4)
# define QT_NO_UNSETENV
#endif

QT_BEGIN_NAMESPACE

//#define X_NOT_BROKEN
#ifdef X_NOT_BROKEN
// Some X libraries are built with setlocale #defined to _Xsetlocale,
// even though library users are then built WITHOUT such a definition.
// This creates a problem - Qt might setlocale() one value, but then
// X looks and doesn't see the value Qt set. The solution here is to
// implement _Xsetlocale just in case X calls it - redirecting it to
// the real libC version.
//
# ifndef setlocale
extern "C" char *_Xsetlocale(int category, const char *locale);
char *_Xsetlocale(int category, const char *locale)
{
    //qDebug("_Xsetlocale(%d,%s),category,locale");
    return setlocale(category,locale);
}
# endif // setlocale
#endif // X_NOT_BROKEN

/* Warning: if you modify this string, modify the list of atoms in qt_x11_p.h as well! */
static const char * x11_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"
    "_NET_WM_SYNC_REQUEST\0"
    "_NET_WM_SYNC_REQUEST_COUNTER\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"

    // Session management
    "WM_CLIENT_LEADER\0"
    "WM_WINDOW_ROLE\0"
    "SM_CLIENT_ID\0"

    // Clipboard
    "CLIPBOARD\0"
    "INCR\0"
    "TARGETS\0"
    "MULTIPLE\0"
    "TIMESTAMP\0"
    "SAVE_TARGETS\0"
    "CLIP_TEMPORARY\0"
    "_QT_SELECTION\0"
    "_QT_CLIPBOARD_SENTINEL\0"
    "_QT_SELECTION_SENTINEL\0"
    "CLIPBOARD_MANAGER\0"

    "RESOURCE_MANAGER\0"

    "_XSETROOT_ID\0"

    "_QT_SCROLL_DONE\0"
    "_QT_INPUT_ENCODING\0"

    "_MOTIF_WM_HINTS\0"

    "DTWM_IS_RUNNING\0"
    "ENLIGHTENMENT_DESKTOP\0"
    "_DT_SAVE_MODE\0"
    "_SGI_DESKS_MANAGER\0"

    // EWMH (aka NETWM)
    "_NET_SUPPORTED\0"
    "_NET_VIRTUAL_ROOTS\0"
    "_NET_WORKAREA\0"

    "_NET_MOVERESIZE_WINDOW\0"
    "_NET_WM_MOVERESIZE\0"

    "_NET_WM_NAME\0"
    "_NET_WM_ICON_NAME\0"
    "_NET_WM_ICON\0"

    "_NET_WM_PID\0"

    "_NET_WM_WINDOW_OPACITY\0"

    "_NET_WM_STATE\0"
    "_NET_WM_STATE_ABOVE\0"
    "_NET_WM_STATE_BELOW\0"
    "_NET_WM_STATE_FULLSCREEN\0"
    "_NET_WM_STATE_MAXIMIZED_HORZ\0"
    "_NET_WM_STATE_MAXIMIZED_VERT\0"
    "_NET_WM_STATE_MODAL\0"
    "_NET_WM_STATE_STAYS_ON_TOP\0"
    "_NET_WM_STATE_DEMANDS_ATTENTION\0"

    "_NET_WM_USER_TIME\0"
    "_NET_WM_USER_TIME_WINDOW\0"
    "_NET_WM_FULL_PLACEMENT\0"

    "_NET_WM_WINDOW_TYPE\0"
    "_NET_WM_WINDOW_TYPE_DESKTOP\0"
    "_NET_WM_WINDOW_TYPE_DOCK\0"
    "_NET_WM_WINDOW_TYPE_TOOLBAR\0"
    "_NET_WM_WINDOW_TYPE_MENU\0"
    "_NET_WM_WINDOW_TYPE_UTILITY\0"
    "_NET_WM_WINDOW_TYPE_SPLASH\0"
    "_NET_WM_WINDOW_TYPE_DIALOG\0"
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU\0"
    "_NET_WM_WINDOW_TYPE_POPUP_MENU\0"
    "_NET_WM_WINDOW_TYPE_TOOLTIP\0"
    "_NET_WM_WINDOW_TYPE_NOTIFICATION\0"
    "_NET_WM_WINDOW_TYPE_COMBO\0"
    "_NET_WM_WINDOW_TYPE_DND\0"
    "_NET_WM_WINDOW_TYPE_NORMAL\0"
    "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE\0"

    "_KDE_NET_WM_FRAME_STRUT\0"

    "_NET_STARTUP_INFO\0"
    "_NET_STARTUP_INFO_BEGIN\0"

    "_NET_SUPPORTING_WM_CHECK\0"

    "_NET_WM_CM_S0\0"

    "_NET_SYSTEM_TRAY_VISUAL\0"

    "_NET_ACTIVE_WINDOW\0"

    // Property formats
    "COMPOUND_TEXT\0"
    "TEXT\0"
    "UTF8_STRING\0"

    // xdnd
    "XdndEnter\0"
    "XdndPosition\0"
    "XdndStatus\0"
    "XdndLeave\0"
    "XdndDrop\0"
    "XdndFinished\0"
    "XdndTypeList\0"
    "XdndActionList\0"

    "XdndSelection\0"

    "XdndAware\0"
    "XdndProxy\0"

    "XdndActionCopy\0"
    "XdndActionLink\0"
    "XdndActionMove\0"
    "XdndActionPrivate\0"

    // Motif DND
    "_MOTIF_DRAG_AND_DROP_MESSAGE\0"
    "_MOTIF_DRAG_INITIATOR_INFO\0"
    "_MOTIF_DRAG_RECEIVER_INFO\0"
    "_MOTIF_DRAG_WINDOW\0"
    "_MOTIF_DRAG_TARGETS\0"

    "XmTRANSFER_SUCCESS\0"
    "XmTRANSFER_FAILURE\0"

    // Xkb
    "_XKB_RULES_NAMES\0"

    // XEMBED
    "_XEMBED\0"
    "_XEMBED_INFO\0"

    // Wacom old. (before version 0.10)
    "Wacom Stylus\0"
    "Wacom Cursor\0"
    "Wacom Eraser\0"

    // Tablet
    "STYLUS\0"
    "ERASER\0"
};

Q_GUI_EXPORT QX11Data *qt_x11Data = 0;

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/
static const char *appName = 0;                        // application name
static const char *appClass = 0;                        // application class
static const char *appFont        = 0;                // application font
static const char *appBGCol        = 0;                // application bg color
static const char *appFGCol        = 0;                // application fg color
static const char *appBTNCol        = 0;                // application btn color
static const char *mwGeometry        = 0;                // main widget geometry
static const char *mwTitle        = 0;                // main widget title
char    *qt_ximServer        = 0;                // XIM Server will connect to
static bool        appSync                = false;        // X11 synchronization
#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // X11 grabbing enabled
static bool        appDoGrab        = false;        // X11 grabbing override (gdb)
#endif
static bool        app_save_rootinfo = false;        // save root info
static bool        app_do_modal        = false;        // modal mode
static Window        curWin = 0;                        // current window


// function to update the workarea of the screen - in qdesktopwidget_x11.cpp
extern void qt_desktopwidget_update_workarea();

// Function to change the window manager state (from qwidget_x11.cpp)
extern void qt_change_net_wm_state(const QWidget *w, bool set, Atom one, Atom two = 0);

// modifier masks for alt, meta, super, hyper, and mode_switch - detected when the application starts
// and/or keyboard layout changes
uchar qt_alt_mask = 0;
uchar qt_meta_mask = 0;
uchar qt_super_mask = 0;
uchar qt_hyper_mask = 0;
uchar qt_mode_switch_mask = 0;

// flags for extensions for special Languages, currently only for RTL languages
bool         qt_use_rtl_extensions = false;

static Window        mouseActWindow             = 0;        // window where mouse is
static Qt::MouseButton  mouseButtonPressed   = Qt::NoButton; // last mouse button pressed
static Qt::MouseButtons mouseButtonState     = Qt::NoButton; // mouse button state
static Time        mouseButtonPressTime = 0;        // when was a button pressed
static short        mouseXPos, mouseYPos;                // mouse pres position in act window
static short        mouseGlobalXPos, mouseGlobalYPos; // global mouse press position

extern QWidgetList *qt_modal_stack;                // stack of modal widgets

// window where mouse buttons have been pressed
static Window pressed_window = XNone;

// popup control
static bool replayPopupMouseEvent = false;
static bool popupGrabOk;

bool qt_sm_blockUserInput = false;                // session management

Q_GUI_EXPORT int qt_xfocusout_grab_counter = 0;

#if !defined (QT_NO_TABLET)
Q_GLOBAL_STATIC(QTabletDeviceDataList, tablet_devices)
QTabletDeviceDataList *qt_tablet_devices()
{
    return tablet_devices();
}

extern bool qt_tabletChokeMouse;
#endif

typedef bool(*QX11FilterFunction)(XEvent *event);

Q_GLOBAL_STATIC(QList<QX11FilterFunction>, x11Filters)

Q_GUI_EXPORT void qt_installX11EventFilter(QX11FilterFunction func)
{
    Q_ASSERT(func);

    if (QList<QX11FilterFunction> *list = x11Filters())
        list->append(func);
}

Q_GUI_EXPORT void qt_removeX11EventFilter(QX11FilterFunction func)
{
    Q_ASSERT(func);

    if (QList<QX11FilterFunction> *list = x11Filters())
        list->removeOne(func);
}


static bool qt_x11EventFilter(XEvent* ev)
{
    long unused;
    if (qApp->filterEvent(ev, &unused))
        return true;
    if (const QList<QX11FilterFunction> *list = x11Filters()) {
        for (QList<QX11FilterFunction>::const_iterator it = list->constBegin(); it != list->constEnd(); ++it) {
            if ((*it)(ev))
                return true;
        }
    }

    return qApp->x11EventFilter(ev);
}

#if !defined(QT_NO_XIM)
XIMStyle        qt_xim_preferred_style = 0;
#endif
int qt_ximComposingKeycode=0;
QTextCodec * qt_input_mapper = 0;

extern bool qt_check_clipboard_sentinel(); //def in qclipboard_x11.cpp
extern bool qt_check_selection_sentinel(); //def in qclipboard_x11.cpp
extern bool qt_xfixes_clipboard_changed(Window clipboardOwner, Time timestamp); //def in qclipboard_x11.cpp
extern bool qt_xfixes_selection_changed(Window selectionOwner, Time timestamp); //def in qclipboard_x11.cpp

static void        qt_save_rootinfo();
Q_GUI_EXPORT bool qt_try_modal(QWidget *, XEvent *);

QWidget *qt_button_down = 0; // last widget to be pressed with the mouse
QPointer<QWidget> qt_last_mouse_receiver = 0;
static QWidget *qt_popup_down = 0;  // popup that contains the pressed widget

extern bool qt_xdnd_dragging;

// gui or non-gui from qapplication.cpp
extern bool qt_is_gui_used;

/*!
    \internal
    Try to resolve a \a symbol from \a library with the version specified
    by \a vernum.

    Note that, in the case of the Xfixes library, \a vernum is not the same as
    \c XFIXES_MAJOR - it is a part of soname and may differ from the Xfixes
    version.
*/
static void* qt_load_library_runtime(const char *library, int vernum,
                                     int highestVernum, const char *symbol)
{
    QList<int> versions;
    // we try to load in the following order:
    // explicit version -> the default one -> (from the highest (highestVernum) to the lowest (vernum) )
    if (vernum != -1)
        versions << vernum;
    versions << -1;
    if (vernum != -1) {
        for(int i = highestVernum; i > vernum; --i)
            versions << i;
    }
    Q_FOREACH(int version, versions) {
        QLatin1String libName(library);
        QLibrary xfixesLib(libName, version);
        void *ptr = xfixesLib.resolve(symbol);
        if (ptr)
            return ptr;
    }
    return 0;
}

#ifndef QT_NO_XINPUT
# ifdef QT_RUNTIME_XINPUT
#  define XINPUT_LOAD_RUNTIME(vernum, symbol, symbol_type) \
    (symbol_type)qt_load_library_runtime("libXi", vernum, 6, #symbol);
#  define XINPUT_LOAD(symbol) \
    XINPUT_LOAD_RUNTIME(1, symbol, Ptr##symbol)
# else // not runtime XInput
#  define XINPUT_LOAD(symbol) symbol
# endif // QT_RUNTIME_XINPUT
#else // not using Xinput at all
# define XINPUT_LOAD(symbol) 0
#endif // QT_NO_XINPUT

#ifndef QT_NO_XFIXES
# ifdef QT_RUNTIME_XFIXES
#  define XFIXES_LOAD_RUNTIME(vernum, symbol, symbol_type) \
    (symbol_type)qt_load_library_runtime("libXfixes", vernum, 4, #symbol);
#  define XFIXES_LOAD_V1(symbol) \
    XFIXES_LOAD_RUNTIME(1, symbol, Ptr##symbol)
#  define XFIXES_LOAD_V2(symbol) \
    XFIXES_LOAD_RUNTIME(2, symbol, Ptr##symbol)

# else // not runtime Xfixes

#  if XFIXES_MAJOR >= 2
#   define XFIXES_LOAD_V1(symbol) symbol
#   define XFIXES_LOAD_V2(symbol) symbol
#  elif XFIXES_MAJOR >= 1
#   define XFIXES_LOAD_V1(symbol) symbol
#   define XFIXES_LOAD_V2(symbol) 0
#  else
#   error Unsupported version of Xfixes
#  endif
# endif // QT_RUNTIME_XFIXES
#else // not using Xfixes at all
# define XFIXES_LOAD_V1(symbol) 0
# define XFIXES_LOAD_V2(symbol) 0
#endif // QT_NO_XFIXES

#ifndef QT_NO_XFIXES

struct qt_xfixes_selection_event_data
{
    // which selection to filter out.
    Atom selection;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool qt_xfixes_scanner(Display*, XEvent *event, XPointer arg)
{
    qt_xfixes_selection_event_data *data =
        reinterpret_cast<qt_xfixes_selection_event_data*>(arg);
    if (event->type == X11->xfixes_eventbase + XFixesSelectionNotify) {
        XFixesSelectionNotifyEvent *xfixes_event = reinterpret_cast<XFixesSelectionNotifyEvent*>(event);
        if (xfixes_event->selection == data->selection)
            return true;
    }
    return false;
}

#if defined(Q_C_CALLBACKS)
}
#endif

#endif // QT_NO_XFIXES

class QETWidget : public QWidget                // event translator widget
{
public:
    QWidgetPrivate* d_func() { return QWidget::d_func(); }
    bool translateMouseEvent(const XEvent *);
    void translatePaintEvent(const XEvent *);
    bool translateConfigEvent(const XEvent *);
    bool translateCloseEvent(const XEvent *);
    bool translateScrollDoneEvent(const XEvent *);
    bool translateWheelEvent(int global_x, int global_y, int delta, Qt::MouseButtons buttons,
                             Qt::KeyboardModifiers modifiers, Qt::Orientation orient);
#if !defined (QT_NO_TABLET)
    bool translateXinputEvent(const XEvent*, QTabletDeviceData *tablet);
#endif
    bool translatePropertyEvent(const XEvent *);

    void doDeferredMap()
    {
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        if (!testAttribute(Qt::WA_Resized)) {
            adjustSize();
            setAttribute(Qt::WA_Resized, false);
        }

        /*
          workaround for WM's that throw away ConfigureRequests from the following:

          window->hide();
          window->move(x, y); // could also be resize(), move()+resize(), or setGeometry()
          window->show();
        */
        QRect r = geometry();

        XMoveResizeWindow(X11->display,
                          internalWinId(),
                          r.x(),
                          r.y(),
                          r.width(),
                          r.height());

        // static gravity!
        XSizeHints sh;
        long unused;
        XGetWMNormalHints(X11->display, internalWinId(), &sh, &unused);
        sh.flags |= USPosition | PPosition | USSize | PSize | PWinGravity;
        sh.x = r.x();
        sh.y = r.y();
        sh.width = r.width();
        sh.height = r.height();
        sh.win_gravity = StaticGravity;
        XSetWMNormalHints(X11->display, internalWinId(), &sh);

        setAttribute(Qt::WA_Mapped);
        if (testAttribute(Qt::WA_DontShowOnScreen))
            return;
        d_func()->topData()->waitingForMapNotify = 1;
        XMapWindow(X11->display, internalWinId());
    }
};


void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
#if !defined(QT_NO_GLIB)
    if (qgetenv("QT_NO_GLIB").isEmpty() && QEventDispatcherGlib::versionSupported())
        eventDispatcher = (q->type() != QApplication::Tty
                           ? new QGuiEventDispatcherGlib(q)
                           : new QEventDispatcherGlib(q));
    else
#endif
        eventDispatcher = (q->type() != QApplication::Tty
                           ? new QEventDispatcherX11(q)
                           : new QEventDispatcherUNIX(q));
}

/*****************************************************************************
  Default X error handlers
 *****************************************************************************/

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static int (*original_x_errhandler)(Display *dpy, XErrorEvent *);
static int (*original_xio_errhandler)(Display *dpy);

static int qt_x_errhandler(Display *dpy, XErrorEvent *err)
{
    if (X11->display != dpy) {
        // only handle X errors for our display
        return 0;
    }

    switch (err->error_code) {
    case BadAtom:
        if (err->request_code == 20 /* X_GetProperty */
            && (err->resourceid == XA_RESOURCE_MANAGER
                || err->resourceid == XA_RGB_DEFAULT_MAP
                || err->resourceid == ATOM(_NET_SUPPORTED)
                || err->resourceid == ATOM(_NET_SUPPORTING_WM_CHECK)
                || err->resourceid == ATOM(XdndProxy)
                || err->resourceid == ATOM(XdndAware))) {
            // Perhaps we're running under SECURITY reduction? :/
            return 0;
        }
        break;

    case BadWindow:
        if (err->request_code == 2 /* X_ChangeWindowAttributes */
            || err->request_code == 38 /* X_QueryPointer */) {
            for (int i = 0; i < ScreenCount(dpy); ++i) {
                if (err->resourceid == RootWindow(dpy, i)) {
                    // Perhaps we're running under SECURITY reduction? :/
                    return 0;
                }
            }
        }
        X11->seen_badwindow = true;
        if (err->request_code == 25 /* X_SendEvent */) {
            for (int i = 0; i < ScreenCount(dpy); ++i) {
                if (err->resourceid == RootWindow(dpy, i)) {
                    // Perhaps we're running under SECURITY reduction? :/
                    return 0;
                }
            }
            if (X11->xdndHandleBadwindow()) {
                qDebug("xdndHandleBadwindow returned true");
                return 0;
            }
        }
        if (X11->ignore_badwindow)
            return 0;
        break;

    default:
#if !defined(QT_NO_XINPUT)
        if (err->request_code == X11->xinput_major
            && err->error_code == (X11->xinput_errorbase + XI_BadDevice)
            && err->minor_code == 3 /* X_OpenDevice */) {
            return 0;
        }
#endif
        break;
    }

    char errstr[256];
    XGetErrorText( dpy, err->error_code, errstr, 256 );
    char buffer[256];
    char request_str[256];
    qsnprintf(buffer, 256, "%d", err->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", buffer, "", request_str, 256);
    if (err->request_code < 128) {
        // X error for a normal protocol request
        qWarning( "X Error: %s %d\n"
                  "  Major opcode: %d (%s)\n"
                  "  Resource id:  0x%lx",
                  errstr, err->error_code,
                  err->request_code,
                  request_str,
                  err->resourceid );
    } else {
        // X error for an extension request
        const char *extensionName = 0;
        if (err->request_code == X11->xrender_major)
            extensionName = "RENDER";
        else if (err->request_code == X11->xrandr_major)
            extensionName = "RANDR";
        else if (err->request_code == X11->xinput_major)
            extensionName = "XInputExtension";
        else if (err->request_code == X11->mitshm_major)
            extensionName = "MIT-SHM";
#ifndef QT_NO_XKB
        else if(err->request_code == X11->xkb_major)
            extensionName = "XKEYBOARD";
#endif

        char minor_str[256];
        if (extensionName) {
            qsnprintf(buffer, 256, "%s.%d", extensionName, err->minor_code);
            XGetErrorDatabaseText(dpy, "XRequest", buffer, "", minor_str, 256);
        } else {
            extensionName = "Uknown extension";
            qsnprintf(minor_str, 256, "Unknown request");
        }
        qWarning( "X Error: %s %d\n"
                  "  Extension:    %d (%s)\n"
                  "  Minor opcode: %d (%s)\n"
                  "  Resource id:  0x%lx",
                  errstr, err->error_code,
                  err->request_code,
                  extensionName,
                  err->minor_code,
                  minor_str,
                  err->resourceid );
    }

    // ### we really should distinguish between severe, non-severe and
    // ### application specific errors

    return 0;
}


static int qt_xio_errhandler(Display *)
{
    qWarning("%s: Fatal IO error: client killed", appName);
    QApplicationPrivate::reset_instance_pointer();
    exit(1);
    //### give the application a chance for a proper shutdown instead,
    //### exit(1) doesn't help.
    return 0;
}

#if defined(Q_C_CALLBACKS)
}
#endif

#ifndef QT_NO_XSYNC
struct qt_sync_request_event_data
{
    WId window;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool qt_sync_request_scanner(Display*, XEvent *event, XPointer arg)
{
    qt_sync_request_event_data *data =
        reinterpret_cast<qt_sync_request_event_data*>(arg);
    if (event->type == ClientMessage &&
        event->xany.window == data->window &&
        event->xclient.message_type == ATOM(WM_PROTOCOLS) &&
        (Atom)event->xclient.data.l[0] == ATOM(_NET_WM_SYNC_REQUEST)) {
        QWidget *w = QWidget::find(event->xany.window);
        if (QTLWExtra *tlw = ((QETWidget*)w)->d_func()->maybeTopData()) {
            const ulong timestamp = (const ulong) event->xclient.data.l[1];
            if (timestamp > X11->time)
                X11->time = timestamp;
            if (timestamp == CurrentTime || timestamp > tlw->syncRequestTimestamp) {
                tlw->syncRequestTimestamp = timestamp;
                tlw->newCounterValueLo = event->xclient.data.l[2];
                tlw->newCounterValueHi = event->xclient.data.l[3];
            }
        }
        return true;
    }
    return false;
}

#if defined(Q_C_CALLBACKS)
}
#endif
#endif // QT_NO_XSYNC

static void qt_x11_create_intern_atoms()
{
    const char *names[QX11Data::NAtoms];
    const char *ptr = x11_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == QX11Data::NPredefinedAtoms);

    QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
    settings_atom_name += XDisplayName(X11->displayName);
    names[i++] = settings_atom_name;

    Q_ASSERT(i == QX11Data::NAtoms);
#if defined(XlibSpecificationRelease) && (XlibSpecificationRelease >= 6)
    XInternAtoms(X11->display, (char **)names, i, False, X11->atoms);
#else
    for (i = 0; i < QX11Data::NAtoms; ++i)
        X11->atoms[i] = XInternAtom(X11->display, (char *)names[i], False);
#endif
}

Q_GUI_EXPORT void qt_x11_apply_settings_in_all_apps()
{
    QByteArray stamp;
    QDataStream s(&stamp, QIODevice::WriteOnly);
    s << QDateTime::currentDateTime();

    XChangeProperty(QX11Info::display(), QX11Info::appRootWindow(0),
                    ATOM(_QT_SETTINGS_TIMESTAMP), ATOM(_QT_SETTINGS_TIMESTAMP), 8,
                    PropModeReplace, (unsigned char *)stamp.data(), stamp.size());
}

/*! \internal
    apply the settings to the application
*/
bool QApplicationPrivate::x11_apply_settings()
{
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));

    settings.beginGroup(QLatin1String("Qt"));

    /*
      Qt settings. This is now they are written into the datastream.

      Palette / *                - QPalette
      font                       - QFont
      libraryPath                - QStringList
      style                      - QString
      doubleClickInterval        - int
      keyboardInputInterval  - int
      cursorFlashTime            - int
      wheelScrollLines           - int
      colorSpec                  - QString
      defaultCodec               - QString
      globalStrut/width          - int
      globalStrut/height         - int
      GUIEffects                 - QStringList
      Font Substitutions/ *      - QStringList
      Font Substitutions/...     - QStringList
    */

    QStringList strlist;
    int i;
    QPalette pal(Qt::black);
    int groupCount = 0;
    strlist = settings.value(QLatin1String("Palette/active")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Active, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/inactive")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Inactive, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/disabled")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Disabled, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }

    // ### Fix properly for 4.6
    bool usingGtkSettings = QApplicationPrivate::app_style && QApplicationPrivate::app_style->inherits("QGtkStyle");
    if (!usingGtkSettings) {
        if (groupCount == QPalette::NColorGroups)
            QApplicationPrivate::setSystemPalette(pal);
    }

    if (!appFont) {
        // ### Fix properly for 4.6
        if (!usingGtkSettings) {
            QFont font(QApplication::font());
            QString fontDescription;
            // Override Qt font if KDE4 settings can be used
            if (X11->desktopVersion == 4) {
                QSettings kdeSettings(QKde::kdeHome() + QLatin1String("/share/config/kdeglobals"), QSettings::IniFormat);
                fontDescription = kdeSettings.value(QLatin1String("font")).toString();
                if (fontDescription.isEmpty()) {
                    // KDE stores fonts without quotes
                    fontDescription = kdeSettings.value(QLatin1String("font")).toStringList().join(QLatin1String(","));
                }
            }
            if (fontDescription.isEmpty())
                fontDescription = settings.value(QLatin1String("font")).toString();
            if (!fontDescription .isEmpty()) {
                font.fromString(fontDescription );
                QApplicationPrivate::setSystemFont(font);
            }
        }
    }

    // read library (ie. plugin) path list
    QString libpathkey =
        QString::fromLatin1("%1.%2/libraryPath")
        .arg(QT_VERSION >> 16)
        .arg((QT_VERSION & 0xff00) >> 8);
    QStringList pathlist = settings.value(libpathkey).toString().split(QLatin1Char(':'));
    if (! pathlist.isEmpty()) {
        QStringList::ConstIterator it = pathlist.constBegin();
        while (it != pathlist.constEnd())
            QApplication::addLibraryPath(*it++);
    }

    // read new QStyle
    QString stylename = settings.value(QLatin1String("style")).toString();

    if (stylename.isEmpty() && QApplicationPrivate::styleOverride.isNull() && X11->use_xrender) {
        stylename = qt_guiPlatformPlugin()->styleName();
    }

    static QString currentStyleName = stylename;
    if (QCoreApplication::startingUp()) {
        if (!stylename.isEmpty() && QApplicationPrivate::styleOverride.isNull())
            QApplicationPrivate::styleOverride = stylename;
    } else {
        if (currentStyleName != stylename) {
            currentStyleName = stylename;
            QApplication::setStyle(stylename);
        }
    }

    int num =
        settings.value(QLatin1String("doubleClickInterval"),
                       QApplication::doubleClickInterval()).toInt();
    QApplication::setDoubleClickInterval(num);

    num =
        settings.value(QLatin1String("cursorFlashTime"),
                       QApplication::cursorFlashTime()).toInt();
    QApplication::setCursorFlashTime(num);

#ifndef QT_NO_WHEELEVENT
    num =
        settings.value(QLatin1String("wheelScrollLines"),
                       QApplication::wheelScrollLines()).toInt();
    QApplication::setWheelScrollLines(num);
#endif

    QString colorspec = settings.value(QLatin1String("colorSpec"),
                                       QVariant(QLatin1String("default"))).toString();
    if (colorspec == QLatin1String("normal"))
        QApplication::setColorSpec(QApplication::NormalColor);
    else if (colorspec == QLatin1String("custom"))
        QApplication::setColorSpec(QApplication::CustomColor);
    else if (colorspec == QLatin1String("many"))
        QApplication::setColorSpec(QApplication::ManyColor);
    else if (colorspec != QLatin1String("default"))
        colorspec = QLatin1String("default");

    QString defaultcodec = settings.value(QLatin1String("defaultCodec"),
                                          QVariant(QLatin1String("none"))).toString();
    if (defaultcodec != QLatin1String("none")) {
        QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1());
        if (codec)
            QTextCodec::setCodecForTr(codec);
    }

    int w = settings.value(QLatin1String("globalStrut/width")).toInt();
    int h = settings.value(QLatin1String("globalStrut/height")).toInt();
    QSize strut(w, h);
    if (strut.isValid())
        QApplication::setGlobalStrut(strut);

    QStringList effects = settings.value(QLatin1String("GUIEffects")).toStringList();
    QApplication::setEffectEnabled(Qt::UI_General,
                                   effects.contains(QLatin1String("general")));
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu,
                                   effects.contains(QLatin1String("animatemenu")));
    QApplication::setEffectEnabled(Qt::UI_FadeMenu,
                                   effects.contains(QLatin1String("fademenu")));
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo,
                                   effects.contains(QLatin1String("animatecombo")));
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip,
                                   effects.contains(QLatin1String("animatetooltip")));
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip,
                                   effects.contains(QLatin1String("fadetooltip")));
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox,
                                   effects.contains(QLatin1String("animatetoolbox")));

    if (!X11->has_fontconfig) {
        settings.beginGroup(QLatin1String("Font Substitutions"));
        QStringList fontsubs = settings.childKeys();
        if (!fontsubs.isEmpty()) {
            QStringList::Iterator it = fontsubs.begin();
            for (; it != fontsubs.end(); ++it) {
                QString fam = *it;
                QStringList subs = settings.value(fam).toStringList();
                QFont::insertSubstitutions(fam, subs);
            }
        }
        settings.endGroup();
    }

    qt_use_rtl_extensions =
        settings.value(QLatin1String("useRtlExtensions"), false).toBool();

#ifndef QT_NO_XIM
    if (qt_xim_preferred_style == 0) {
        QString ximInputStyle = settings.value(QLatin1String("XIMInputStyle"),
                                               QVariant(QLatin1String("on the spot"))).toString().toLower();
        if (ximInputStyle == QLatin1String("on the spot"))
            qt_xim_preferred_style = XIMPreeditCallbacks | XIMStatusNothing;
        else if (ximInputStyle == QLatin1String("over the spot"))
            qt_xim_preferred_style = XIMPreeditPosition | XIMStatusNothing;
        else if (ximInputStyle == QLatin1String("off the spot"))
            qt_xim_preferred_style = XIMPreeditArea | XIMStatusArea;
        else if (ximInputStyle == QLatin1String("root"))
            qt_xim_preferred_style = XIMPreeditNothing | XIMStatusNothing;
    }
#endif
    QStringList inputMethods = QInputContextFactory::keys();
    if (inputMethods.size() > 2 && inputMethods.contains(QLatin1String("imsw-multi"))) {
        X11->default_im = QLatin1String("imsw-multi");
    } else {
        X11->default_im = settings.value(QLatin1String("DefaultInputMethod"),
                                         QLatin1String("xim")).toString();
    }

    settings.endGroup(); // Qt

    return true;
}


/*! \internal
    Resets the QApplication::instance() pointer to zero
*/
void QApplicationPrivate::reset_instance_pointer()
{ QApplication::self = 0; }


// read the _QT_INPUT_ENCODING property and apply the settings to
// the application
static void qt_set_input_encoding()
{
    Atom type;
    int format;
    ulong  nitems, after = 1;
    unsigned char *data = 0;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                                ATOM(_QT_INPUT_ENCODING), 0, 1024,
                                False, XA_STRING, &type, &format, &nitems,
                                &after, &data);
    if (e != Success || !nitems || type == XNone) {
        // Always use the locale codec, since we have no examples of non-local
        // XIMs, and since we cannot get a sensible answer about the encoding
        // from the XIM.
        qt_input_mapper = QTextCodec::codecForLocale();

    } else {
        if (!qstricmp((char *)data, "locale"))
            qt_input_mapper = QTextCodec::codecForLocale();
        else
            qt_input_mapper = QTextCodec::codecForName((char *)data);
        // make sure we have an input codec
        if(!qt_input_mapper)
            qt_input_mapper = QTextCodec::codecForName("ISO 8859-1");
    }
    if (qt_input_mapper && qt_input_mapper->mibEnum() == 11) // 8859-8
        qt_input_mapper = QTextCodec::codecForName("ISO 8859-8-I");
    if(data)
        XFree((char *)data);
}

// set font, foreground and background from x11 resources. The
// arguments may override the resource settings.
static void qt_set_x11_resources(const char* font = 0, const char* fg = 0,
                                 const char* bg = 0, const char* button = 0)
{

    QString resFont, resFG, resBG, resButton, resEF, sysFont, selectBackground, selectForeground;

    QApplication::setEffectEnabled(Qt::UI_General, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);

    bool paletteAlreadySet = false;
    if (QApplication::desktopSettingsAware()) {
        // first, read from settings
        QApplicationPrivate::x11_apply_settings();
        // the call to QApplication::style() below creates the system
        // palette, which breaks the logic after the RESOURCE_MANAGER
        // loop... so I have to save this value to be able to use it later
        paletteAlreadySet = (QApplicationPrivate::sys_pal != 0);

        // second, parse the RESOURCE_MANAGER property
        int format;
        ulong  nitems, after = 1;
        QString res;
        long offset = 0;
        Atom type = XNone;

        while (after > 0) {
            uchar *data = 0;
            if (XGetWindowProperty(X11->display, QX11Info::appRootWindow(0),
                                   ATOM(RESOURCE_MANAGER),
                                   offset, 8192, False, AnyPropertyType,
                                   &type, &format, &nitems, &after,
                                   &data) != Success) {
                res = QString();
                break;
            }
            if (type == XA_STRING)
                res += QString::fromLatin1((char*)data);
            else
                res += QString::fromLocal8Bit((char*)data);
            offset += 2048; // offset is in 32bit quantities... 8192/4 == 2048
            if (data)
                XFree((char *)data);
        }

        QString key, value;
        int l = 0, r;
        QString apn = QString::fromLocal8Bit(appName);
        QString apc = QString::fromLocal8Bit(appClass);
        int apnl = apn.length();
        int apcl = apc.length();
        int resl = res.length();

        while (l < resl) {
            r = res.indexOf(QLatin1Char('\n'), l);
            if (r < 0)
                r = resl;
            while (res.at(l).isSpace())
                l++;
            bool mine = false;
            QChar sc = res.at(l + 1);
            if (res.at(l) == QLatin1Char('*') &&
                (sc == QLatin1Char('f') || sc == QLatin1Char('b') || sc == QLatin1Char('g') ||
                 sc == QLatin1Char('F') || sc == QLatin1Char('B') || sc == QLatin1Char('G') ||
                 sc == QLatin1Char('s') || sc == QLatin1Char('S')
                 // capital T only, since we're looking for "Text.selectSomething"
                 || sc == QLatin1Char('T'))) {
                // OPTIMIZED, since we only want "*[fbgsT].."
                QString item = res.mid(l, r - l).simplified();
                int i = item.indexOf(QLatin1Char(':'));
                key = item.left(i).trimmed().mid(1).toLower();
                value = item.right(item.length() - i - 1).trimmed();
                mine = true;
            } else if ((apnl && res.at(l) == apn.at(0)) || (appClass && apcl && res.at(l) == apc.at(0))) {
                if (res.mid(l,apnl) == apn && (res.at(l+apnl) == QLatin1Char('.')
                                               || res.at(l+apnl) == QLatin1Char('*'))) {
                    QString item = res.mid(l, r - l).simplified();
                    int i = item.indexOf(QLatin1Char(':'));
                    key = item.left(i).trimmed().mid(apnl+1).toLower();
                    value = item.right(item.length() - i - 1).trimmed();
                    mine = true;
                } else if (res.mid(l,apcl) == apc && (res.at(l+apcl) == QLatin1Char('.')
                                                      || res.at(l+apcl) == QLatin1Char('*'))) {
                    QString item = res.mid(l, r - l).simplified();
                    int i = item.indexOf(QLatin1Char(':'));
                    key = item.left(i).trimmed().mid(apcl+1).toLower();
                    value = item.right(item.length() - i - 1).trimmed();
                    mine = true;
                }
            }

            if (mine) {
                if (!font && key == QLatin1String("systemfont"))
                    sysFont = value.left(value.lastIndexOf(QLatin1Char(':')));
                if (!font && key == QLatin1String("font"))
                    resFont = value;
                else if (!fg && !paletteAlreadySet) {
                    if (key == QLatin1String("foreground"))
                        resFG = value;
                    else if (!bg && key == QLatin1String("background"))
                        resBG = value;
                    else if (!bg && !button && key == QLatin1String("button.background"))
                        resButton = value;
                    else if (key == QLatin1String("text.selectbackground")) {
                        selectBackground = value;
                    } else if (key == QLatin1String("text.selectforeground")) {
                        selectForeground = value;
                    }
                } else if (key == QLatin1String("guieffects"))
                    resEF = value;
                // NOTE: if you add more, change the [fbg] stuff above
            }

            l = r + 1;
        }
    }
    if (!sysFont.isEmpty())
        resFont = sysFont;
    if (resFont.isEmpty())
        resFont = QString::fromLocal8Bit(font);
    if (resFG.isEmpty())
        resFG = QString::fromLocal8Bit(fg);
    if (resBG.isEmpty())
        resBG = QString::fromLocal8Bit(bg);
    if (resButton.isEmpty())
        resButton = QString::fromLocal8Bit(button);
    if (!resFont.isEmpty()
        && !X11->has_fontconfig
        && !QApplicationPrivate::sys_font) {
        // set application font
        QFont fnt;
        fnt.setRawName(resFont);

        // the font we get may actually be an alias for another font,
        // so we reset the application font to the real font info.
        if (! fnt.exactMatch()) {
            QFontInfo fontinfo(fnt);
            fnt.setFamily(fontinfo.family());
            fnt.setRawMode(fontinfo.rawMode());

            if (! fnt.rawMode()) {
                fnt.setItalic(fontinfo.italic());
                fnt.setWeight(fontinfo.weight());
                fnt.setUnderline(fontinfo.underline());
                fnt.setStrikeOut(fontinfo.strikeOut());
                fnt.setStyleHint(fontinfo.styleHint());

                if (fnt.pointSize() <= 0 && fnt.pixelSize() <= 0) {
                    // size is all wrong... fix it
                    qreal pointSize = fontinfo.pixelSize() * 72. / (float) QX11Info::appDpiY();
                    if (pointSize <= 0)
                        pointSize = 12;
                    fnt.setPointSize(qRound(pointSize));
                }
            }
        }

        QApplicationPrivate::setSystemFont(fnt);
    }
    // QGtkStyle sets it's own system palette
    bool gtkStyle = QApplicationPrivate::app_style && QApplicationPrivate::app_style->inherits("QGtkStyle");
    bool kdeColors = (QApplication::desktopSettingsAware() && X11->desktopEnvironment == DE_KDE);
    if (!gtkStyle && (kdeColors || (button || !resBG.isEmpty() || !resFG.isEmpty()))) {// set app colors
        bool allowX11ColorNames = QColor::allowX11ColorNames();
        QColor::setAllowX11ColorNames(true);

        (void) QApplication::style();  // trigger creation of application style and system palettes
        QColor btn;
        QColor bg;
        QColor fg;
        QColor bfg;
        QColor wfg;
        if (!resBG.isEmpty())
            bg = QColor(resBG);
        if (!bg.isValid())
            bg = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::Window);

        if (!resFG.isEmpty())
            fg = QColor(resFG);
        if (!fg.isValid())
            fg = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::WindowText);

        if (!resButton.isEmpty())
            btn = QColor(resButton);
        else if (!resBG.isEmpty())
            btn = bg;
        if (!btn.isValid())
            btn = QApplicationPrivate::sys_pal->color(QPalette::Active, QPalette::Button);

        int h,s,v;
        fg.getHsv(&h,&s,&v);
        QColor base = Qt::white;
        bool bright_mode = false;
        if (v >= 255 - 50) {
            base = btn.darker(150);
            bright_mode = true;
        }

        QPalette pal(fg, btn, btn.lighter(125), btn.darker(130), btn.darker(120), wfg.isValid() ? wfg : fg, Qt::white, base, bg);
        QColor disabled((fg.red()   + btn.red())  / 2,
                        (fg.green() + btn.green())/ 2,
                        (fg.blue()  + btn.blue()) / 2);
        pal.setColorGroup(QPalette::Disabled, disabled, btn, btn.lighter(125),
                          btn.darker(130), btn.darker(150), disabled, Qt::white, Qt::white, bg);

        QColor highlight, highlightText;
        if (!selectBackground.isEmpty() && !selectForeground.isEmpty()) {
            highlight = QColor(selectBackground);
            highlightText = QColor(selectForeground);
        }

        if (highlight.isValid() && highlightText.isValid()) {
            pal.setColor(QPalette::Highlight, highlight);
            pal.setColor(QPalette::HighlightedText, highlightText);

            // calculate disabled colors by removing saturation
            highlight.setHsv(highlight.hue(), 0, highlight.value(), highlight.alpha());
            highlightText.setHsv(highlightText.hue(), 0, highlightText.value(), highlightText.alpha());
            pal.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, highlightText);
        } else if (bright_mode) {
            pal.setColor(QPalette::HighlightedText, base);
            pal.setColor(QPalette::Highlight, Qt::white);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, base);
            pal.setColor(QPalette::Disabled, QPalette::Highlight, Qt::white);
        } else {
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::Highlight, Qt::darkBlue);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::Disabled, QPalette::Highlight, Qt::darkBlue);
        }

        pal = qt_guiPlatformPlugin()->palette().resolve(pal);
        QApplicationPrivate::setSystemPalette(pal);
        QColor::setAllowX11ColorNames(allowX11ColorNames);
    }

    if (!resEF.isEmpty()) {
        QStringList effects = resEF.split(QLatin1Char(' '));
        QApplication::setEffectEnabled(Qt::UI_General, effects.contains(QLatin1String("general")));
        QApplication::setEffectEnabled(Qt::UI_AnimateMenu,
                                       effects.contains(QLatin1String("animatemenu")));
        QApplication::setEffectEnabled(Qt::UI_FadeMenu,
                                       effects.contains(QLatin1String("fademenu")));
        QApplication::setEffectEnabled(Qt::UI_AnimateCombo,
                                       effects.contains(QLatin1String("animatecombo")));
        QApplication::setEffectEnabled(Qt::UI_AnimateTooltip,
                                       effects.contains(QLatin1String("animatetooltip")));
        QApplication::setEffectEnabled(Qt::UI_FadeTooltip,
                                       effects.contains(QLatin1String("fadetooltip")));
        QApplication::setEffectEnabled(Qt::UI_AnimateToolBox,
                                       effects.contains(QLatin1String("animatetoolbox")));
    }

    QIconLoader::instance()->updateSystemTheme();
}


// update the supported array
static void qt_get_net_supported()
{
    Atom type;
    int format;
    long offset = 0;
    unsigned long nitems, after;
    unsigned char *data = 0;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_SUPPORTED), 0, 0,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);
    if (data)
        XFree(data);

    if (X11->net_supported_list)
        delete [] X11->net_supported_list;
    X11->net_supported_list = 0;

    if (e == Success && type == XA_ATOM && format == 32) {
        QBuffer ts;
        ts.open(QIODevice::WriteOnly);

        while (after > 0) {
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_SUPPORTED), offset, 1024,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);

            if (type == XA_ATOM && format == 32) {
                ts.write(reinterpret_cast<char *>(data), nitems * sizeof(long));
                offset += nitems;
            } else
                after = 0;
            if (data)
                XFree(data);
        }

        // compute nitems
        QByteArray buffer(ts.buffer());
        nitems = buffer.size() / sizeof(Atom);
        X11->net_supported_list = new Atom[nitems + 1];
        Atom *a = (Atom *) buffer.data();
        uint i;
        for (i = 0; i < nitems; i++)
            X11->net_supported_list[i] = a[i];
        X11->net_supported_list[nitems] = 0;
    }
}


bool QX11Data::isSupportedByWM(Atom atom)
{
    if (!X11->net_supported_list)
        return false;

    bool supported = false;
    int i = 0;
    while (X11->net_supported_list[i] != 0) {
        if (X11->net_supported_list[i++] == atom) {
            supported = true;
            break;
        }
    }

    return supported;
}


// update the virtual roots array
static void qt_get_net_virtual_roots()
{
    if (X11->net_virtual_root_list)
        delete [] X11->net_virtual_root_list;
    X11->net_virtual_root_list = 0;

    if (!X11->isSupportedByWM(ATOM(_NET_VIRTUAL_ROOTS)))
        return;

    Atom type;
    int format;
    long offset = 0;
    unsigned long nitems, after;
    unsigned char *data;

    int e = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_VIRTUAL_ROOTS), 0, 0,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);
    if (data)
        XFree(data);

    if (e == Success && type == XA_ATOM && format == 32) {
        QBuffer ts;
        ts.open(QIODevice::WriteOnly);

        while (after > 0) {
            XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_VIRTUAL_ROOTS), offset, 1024,
                               False, XA_ATOM, &type, &format, &nitems, &after, &data);

            if (type == XA_ATOM && format == 32) {
                ts.write(reinterpret_cast<char *>(data), nitems * 4);
                offset += nitems;
            } else
                after = 0;
            if (data)
                XFree(data);
        }

        // compute nitems
        QByteArray buffer(ts.buffer());
        nitems = buffer.size() / sizeof(Window);
        X11->net_virtual_root_list = new Window[nitems + 1];
        Window *a = (Window *) buffer.data();
        uint i;
        for (i = 0; i < nitems; i++)
            X11->net_virtual_root_list[i] = a[i];
        X11->net_virtual_root_list[nitems] = 0;
    }
}

void qt_net_remove_user_time(QWidget *tlw)
{
    Q_ASSERT(tlw);
    QTLWExtra *extra = tlw->d_func()->maybeTopData();
    if (extra && extra->userTimeWindow) {
        Q_ASSERT(tlw->internalWinId());
        XDeleteProperty(X11->display, tlw->internalWinId(), ATOM(_NET_WM_USER_TIME_WINDOW));
        XDestroyWindow(X11->display, extra->userTimeWindow);
        extra->userTimeWindow = 0;
    }
}

void qt_net_update_user_time(QWidget *tlw, unsigned long timestamp)
{
    Q_ASSERT(tlw);
    Q_ASSERT(tlw->isWindow());
    Q_ASSERT(tlw->testAttribute(Qt::WA_WState_Created));
    QTLWExtra *extra = tlw->d_func()->topData();
    WId wid = tlw->internalWinId();
    const bool isSupportedByWM = X11->isSupportedByWM(ATOM(_NET_WM_USER_TIME_WINDOW));
    if (extra->userTimeWindow || isSupportedByWM) {
        if (!extra->userTimeWindow) {
            extra->userTimeWindow = XCreateSimpleWindow(X11->display,
                                                        tlw->internalWinId(),
                                                        -1, -1, 1, 1, 0, 0, 0);
            wid = extra->userTimeWindow;
            XChangeProperty(X11->display, tlw->internalWinId(), ATOM(_NET_WM_USER_TIME_WINDOW),
                            XA_WINDOW, 32, PropModeReplace,
                            (unsigned char *)&wid, 1);
            XDeleteProperty(X11->display, tlw->internalWinId(), ATOM(_NET_WM_USER_TIME));
        } else if (!isSupportedByWM) {
            // WM no longer supports it, then we should remove the
            // _NET_WM_USER_TIME_WINDOW atom.
            qt_net_remove_user_time(tlw);
        } else {
            wid = extra->userTimeWindow;
        }
    }
    XChangeProperty(X11->display, wid, ATOM(_NET_WM_USER_TIME),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &timestamp, 1);
}

static void qt_check_focus_model()
{
    Window fw = XNone;
    int unused;
    XGetInputFocus(X11->display, &fw, &unused);
    if (fw == PointerRoot)
        X11->focus_model = QX11Data::FM_PointerRoot;
    else
        X11->focus_model = QX11Data::FM_Other;
}

#ifndef QT_NO_TABLET

#if !defined (Q_OS_IRIX)
// from include/Xwacom.h
#  define XWACOM_PARAM_TOOLID 322
#  define XWACOM_PARAM_TOOLSERIAL 323

typedef WACOMCONFIG * (*PtrWacomConfigInit) (Display*, WACOMERRORFUNC);
typedef WACOMDEVICE * (*PtrWacomConfigOpenDevice) (WACOMCONFIG*, const char*);
typedef int *(*PtrWacomConfigGetRawParam) (WACOMDEVICE*, int, int*, int, unsigned*);
typedef int (*PtrWacomConfigCloseDevice) (WACOMDEVICE *);
typedef void (*PtrWacomConfigTerm) (WACOMCONFIG *);

static PtrWacomConfigInit ptrWacomConfigInit = 0;
static PtrWacomConfigOpenDevice ptrWacomConfigOpenDevice = 0;
static PtrWacomConfigGetRawParam ptrWacomConfigGetRawParam = 0;
static PtrWacomConfigCloseDevice ptrWacomConfigCloseDevice = 0;
static PtrWacomConfigTerm ptrWacomConfigTerm = 0;
Q_GLOBAL_STATIC(QByteArray, wacomDeviceName)
#endif

#endif

/*****************************************************************************
  qt_init() - initializes Qt for X11
 *****************************************************************************/

#if !defined(QT_NO_FONTCONFIG)
static void getXDefault(const char *group, const char *key, int *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        char *end = 0;
        int v = strtol(str, &end, 0);
        if (str != end)
            *val = v;
        // otherwise use fontconfig to convert the string to integer
        else
            FcNameConstant((FcChar8 *) str, val);
    }
}

static void getXDefault(const char *group, const char *key, double *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        bool ok;
        double v = QByteArray(str).toDouble(&ok);
        if (ok)
            *val = v;
    }
}

static void getXDefault(const char *group, const char *key, bool *val)
{
    char *str = XGetDefault(X11->display, group, key);
    if (str) {
        char c = str[0];
        if (isupper((int)c))
            c = tolower(c);
        if (c == 't' || c == 'y' || c == '1')
            *val = true;
        else if (c == 'f' || c == 'n' || c == '0')
            *val = false;
        if (c == 'o') {
            c = str[1];
            if (isupper((int)c))
                c = tolower(c);
            if (c == 'n')
                *val = true;
            if (c == 'f')
                *val = false;
        }
    }
}
#endif

// ### This should be static but it isn't because of the friend declaration
// ### in qpaintdevice.h which then should have a static too but can't have
// ### it because "storage class specifiers invalid in friend function
// ### declarations" :-) Ideas anyone?
void qt_init(QApplicationPrivate *priv, int,
	     Display *display, Qt::HANDLE visual, Qt::HANDLE colormap)
{
    X11 = new QX11Data;
    X11->display = display;
    X11->displayName = 0;
    X11->foreignDisplay = (display != 0);
    X11->focus_model = -1;

    // RANDR
    X11->use_xrandr = false;
    X11->xrandr_major = 0;
    X11->xrandr_eventbase = 0;
    X11->xrandr_errorbase = 0;

    // RENDER
    X11->use_xrender = false;
    X11->xrender_major = 0;
    X11->xrender_version = 0;

    // XFIXES
    X11->use_xfixes = false;
    X11->xfixes_major = 0;
    X11->xfixes_eventbase = 0;
    X11->xfixes_errorbase = 0;

    // XInputExtension
    X11->use_xinput = false;
    X11->xinput_major = 0;
    X11->xinput_eventbase = 0;
    X11->xinput_errorbase = 0;

    X11->use_xkb = false;
    X11->xkb_major = 0;
    X11->xkb_eventbase = 0;
    X11->xkb_errorbase = 0;

    // MIT-SHM
    X11->use_mitshm = false;
    X11->use_mitshm_pixmaps = false;
    X11->mitshm_major = 0;

    X11->sip_serial = 0;
    X11->net_supported_list = 0;
    X11->net_virtual_root_list = 0;
    X11->wm_client_leader = 0;
    X11->screens = 0;
    X11->argbVisuals = 0;
    X11->argbColormaps = 0;
    X11->screenCount = 0;
    X11->time = CurrentTime;
    X11->userTime = CurrentTime;
    X11->ignore_badwindow = false;
    X11->seen_badwindow = false;

    X11->motifdnd_active = false;

    X11->default_im = QLatin1String("imsw-multi");
    priv->inputContext = 0;

    // colormap control
    X11->visual_class = -1;
    X11->visual_id = -1;
    X11->color_count = 0;
    X11->custom_cmap = false;

    // outside visual/colormap
    X11->visual = reinterpret_cast<Visual *>(visual);
    X11->colormap = colormap;

    // Fontconfig
    X11->has_fontconfig = false;
#if !defined(QT_NO_FONTCONFIG)
    if (qgetenv("QT_X11_NO_FONTCONFIG").isNull())
        X11->has_fontconfig = FcInit();
    X11->fc_antialias = true;
#endif

#ifndef QT_NO_XRENDER
    memset(X11->solid_fills, 0, sizeof(X11->solid_fills));
    for (int i = 0; i < X11->solid_fill_count; ++i)
        X11->solid_fills[i].screen = -1;
    memset(X11->pattern_fills, 0, sizeof(X11->pattern_fills));
    for (int i = 0; i < X11->pattern_fill_count; ++i)
        X11->pattern_fills[i].screen = -1;
#endif

    X11->startupId = 0;

    int argc = priv->argc;
    char **argv = priv->argv;

    if (X11->display) {
        // Qt part of other application

        // Set application name and class
        appName = qstrdup("Qt-subapplication");
        char *app_class = 0;
        if (argv) {
            const char* p = strrchr(argv[0], '/');
            app_class = qstrdup(p ? p + 1 : argv[0]);
            if (app_class[0])
                app_class[0] = toupper(app_class[0]);
        }
        appClass = app_class;
    } else {
        // Qt controls everything (default)

        if (QApplication::testAttribute(Qt::AA_X11InitThreads))
            XInitThreads();

        // Set application name and class
        char *app_class = 0;
        if (argv && argv[0]) {
            const char *p = strrchr(argv[0], '/');
            appName = p ? p + 1 : argv[0];
            app_class = qstrdup(appName);
            if (app_class[0])
                app_class[0] = toupper(app_class[0]);
        }
        appClass = app_class;
    }

    // Install default error handlers
    original_x_errhandler = XSetErrorHandler(qt_x_errhandler);
    original_xio_errhandler = XSetIOErrorHandler(qt_xio_errhandler);

    // Get command line params
    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg(argv[i]);
        if (arg == "-display") {
            if (++i < argc && !X11->display)
                X11->displayName = argv[i];
        } else if (arg == "-fn" || arg == "-font") {
            if (++i < argc)
                appFont = argv[i];
        } else if (arg == "-bg" || arg == "-background") {
            if (++i < argc)
                appBGCol = argv[i];
        } else if (arg == "-btn" || arg == "-button") {
            if (++i < argc)
                appBTNCol = argv[i];
        } else if (arg == "-fg" || arg == "-foreground") {
            if (++i < argc)
                appFGCol = argv[i];
        } else if (arg == "-name") {
            if (++i < argc)
                appName = argv[i];
        } else if (arg == "-title") {
            if (++i < argc)
                mwTitle = argv[i];
        } else if (arg == "-geometry") {
            if (++i < argc)
                mwGeometry = argv[i];
        } else if (arg == "-im") {
            if (++i < argc)
                qt_ximServer = argv[i];
        } else if (arg == "-ncols") {   // xv and netscape use this name
            if (++i < argc)
                X11->color_count = qMax(0,atoi(argv[i]));
        } else if (arg == "-visual") {  // xv and netscape use this name
            if (++i < argc && !X11->visual) {
                QString s = QString::fromLocal8Bit(argv[i]).toLower();
                if (s == QLatin1String("staticgray"))
                    X11->visual_class = StaticGray;
                else if (s == QLatin1String("grayscale"))
                    X11->visual_class = XGrayScale;
                else if (s == QLatin1String("staticcolor"))
                    X11->visual_class = StaticColor;
                else if (s == QLatin1String("pseudocolor"))
                    X11->visual_class = PseudoColor;
                else if (s == QLatin1String("truecolor"))
                    X11->visual_class = TrueColor;
                else if (s == QLatin1String("directcolor"))
                    X11->visual_class = DirectColor;
                else
                    X11->visual_id = static_cast<int>(strtol(argv[i], 0, 0));
            }
#ifndef QT_NO_XIM
        } else if (arg == "-inputstyle") {
            if (++i < argc) {
                QString s = QString::fromLocal8Bit(argv[i]).toLower();
                if (s == QLatin1String("onthespot"))
                    qt_xim_preferred_style = XIMPreeditCallbacks |
                                             XIMStatusNothing;
                else if (s == QLatin1String("overthespot"))
                    qt_xim_preferred_style = XIMPreeditPosition |
                                             XIMStatusNothing;
                else if (s == QLatin1String("offthespot"))
                    qt_xim_preferred_style = XIMPreeditArea |
                                             XIMStatusArea;
                else if (s == QLatin1String("root"))
                    qt_xim_preferred_style = XIMPreeditNothing |
                                             XIMStatusNothing;
            }
#endif
        } else if (arg == "-cmap") {    // xv uses this name
            if (!X11->colormap)
                X11->custom_cmap = true;
        }
        else if (arg == "-sync")
            appSync = !appSync;
#if defined(QT_DEBUG)
        else if (arg == "-nograb")
            appNoGrab = !appNoGrab;
        else if (arg == "-dograb")
            appDoGrab = !appDoGrab;
#endif
        else
            argv[j++] = argv[i];
    }

    priv->argc = j;

#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
    if (!appNoGrab && !appDoGrab) {
        QString s;
        s.sprintf("/proc/%d/cmdline", getppid());
        QFile f(s);
        if (f.open(QIODevice::ReadOnly)) {
            s.clear();
            char c;
            while (f.getChar(&c) && c) {
                if (c == '/')
                    s.clear();
                else
                    s += QLatin1Char(c);
            }
            if (s == QLatin1String("gdb")) {
                appNoGrab = true;
                qDebug("Qt: gdb: -nograb added to command-line options.\n"
                       "\t Use the -dograb option to enforce grabbing.");
            }
            f.close();
        }
    }
#endif

    // Connect to X server
    if (qt_is_gui_used && !X11->display) {
        if ((X11->display = XOpenDisplay(X11->displayName)) == 0) {
            qWarning("%s: cannot connect to X server %s", appName,
                     XDisplayName(X11->displayName));
            QApplicationPrivate::reset_instance_pointer();
            exit(1);
        }

        if (appSync)                                // if "-sync" argument
            XSynchronize(X11->display, true);
    }

    // Common code, regardless of whether display is foreign.

    // Get X parameters

    if (qt_is_gui_used) {
        X11->defaultScreen = DefaultScreen(X11->display);
        X11->screenCount = ScreenCount(X11->display);

        int formatCount = 0;
        XPixmapFormatValues *values = XListPixmapFormats(X11->display, &formatCount);
        for (int i = 0; i < formatCount; ++i)
            X11->bppForDepth[values[i].depth] = values[i].bits_per_pixel;
        XFree(values);

        X11->screens = new QX11InfoData[X11->screenCount];
        X11->argbVisuals = new Visual *[X11->screenCount];
        X11->argbColormaps = new Colormap[X11->screenCount];

        for (int s = 0; s < X11->screenCount; s++) {
            QX11InfoData *screen = X11->screens + s;
            screen->ref = 1; // ensures it doesn't get deleted
            screen->screen = s;

            int widthMM = DisplayWidthMM(X11->display, s);
            if (widthMM != 0) {
                screen->dpiX = (DisplayWidth(X11->display, s) * 254 + widthMM * 5) / (widthMM * 10);
            } else {
                screen->dpiX = 72;
            }

            int heightMM = DisplayHeightMM(X11->display, s);
            if (heightMM != 0) {
                screen->dpiY = (DisplayHeight(X11->display, s) * 254 + heightMM * 5) / (heightMM * 10);
            } else {
                screen->dpiY = 72;
            }

            X11->argbVisuals[s] = 0;
            X11->argbColormaps[s] = 0;
        }


#ifndef QT_NO_XRENDER
        int xrender_eventbase,  xrender_errorbase;
        // See if XRender is supported on the connected display
        if (XQueryExtension(X11->display, "RENDER", &X11->xrender_major,
                            &xrender_eventbase, &xrender_errorbase)
            && XRenderQueryExtension(X11->display, &xrender_eventbase,
                                     &xrender_errorbase)) {
            // Check the version as well - we need v0.4 or higher
            int major = 0;
            int minor = 0;
            XRenderQueryVersion(X11->display, &major, &minor);
            if (qgetenv("QT_X11_NO_XRENDER").isNull()) {
                X11->use_xrender = (major >= 0 && minor >= 5);
                X11->xrender_version = major*100+minor;
                // workaround for broken XServer on Ubuntu Breezy (6.8 compiled with 7.0
                // protocol headers)
                if (X11->xrender_version == 10
                    && VendorRelease(X11->display) < 60900000
                    && QByteArray(ServerVendor(X11->display)).contains("X.Org"))
                    X11->xrender_version = 9;
            }
        }
#endif // QT_NO_XRENDER

#ifndef QT_NO_MITSHM
        int mitshm_minor;
        int mitshm_major;
        int mitshm_eventbase;
        int mitshm_errorbase;
        int mitshm_pixmaps;
        if (XQueryExtension(X11->display, "MIT-SHM", &X11->mitshm_major,
                            &mitshm_eventbase, &mitshm_errorbase)
            && XShmQueryVersion(X11->display, &mitshm_major, &mitshm_minor,
                                &mitshm_pixmaps))
        {
            QString displayName = QLatin1String(XDisplayName(NULL));

            // MITSHM only works for local displays, so do a quick check here
            // to determine whether the display is local or not (not 100 % accurate).
            // BGR server layouts are not supported either, since it requires the raster
            // engine to work on a QImage with BGR layout.
            bool local = displayName.isEmpty() || displayName.lastIndexOf(QLatin1Char(':')) == 0;
            if (local && (qgetenv("QT_X11_NO_MITSHM").toInt() == 0)) {
                Visual *defaultVisual = DefaultVisual(X11->display, DefaultScreen(X11->display));
                X11->use_mitshm = ((defaultVisual->red_mask == 0xff0000
                                    || defaultVisual->red_mask == 0xf800)
                                   && (defaultVisual->green_mask == 0xff00
                                       || defaultVisual->green_mask == 0x7e0)
                                   && (defaultVisual->blue_mask == 0xff
                                       || defaultVisual->blue_mask == 0x1f));
                X11->use_mitshm_pixmaps = X11->use_mitshm && mitshm_pixmaps;
            }
        }
#endif // QT_NO_MITSHM

        // initialize the graphics system - order is imporant here - it must be done before
        // the QColormap::initialize() call
        QApplicationPrivate::graphics_system = QGraphicsSystemFactory::create(QApplicationPrivate::graphics_system_name);
        QColormap::initialize();

        // Support protocols
        X11->xdndSetup();

        // Finally create all atoms
        qt_x11_create_intern_atoms();

        // initialize NET lists
        qt_get_net_supported();
        qt_get_net_virtual_roots();

#ifndef QT_NO_XRANDR
        // See if XRandR is supported on the connected display
        if (XQueryExtension(X11->display, "RANDR", &X11->xrandr_major,
                            &X11->xrandr_eventbase, &X11->xrandr_errorbase)) {

#  ifdef QT_RUNTIME_XRANDR
            X11->ptrXRRSelectInput = 0;
            X11->ptrXRRUpdateConfiguration = 0;
            X11->ptrXRRRootToScreen = 0;
            X11->ptrXRRQueryExtension = 0;
            QLibrary xrandrLib(QLatin1String("Xrandr"), 2);
            if (!xrandrLib.load()) { // try without the version number
                xrandrLib.setFileName(QLatin1String("Xrandr"));
                xrandrLib.load();
            }
            if (xrandrLib.isLoaded()) {
                X11->ptrXRRSelectInput =
                    (PtrXRRSelectInput) xrandrLib.resolve("XRRSelectInput");
                X11->ptrXRRUpdateConfiguration =
                    (PtrXRRUpdateConfiguration) xrandrLib.resolve("XRRUpdateConfiguration");
                X11->ptrXRRRootToScreen =
                    (PtrXRRRootToScreen) xrandrLib.resolve("XRRRootToScreen");
                X11->ptrXRRQueryExtension =
                    (PtrXRRQueryExtension) xrandrLib.resolve("XRRQueryExtension");
            }
#  else
            X11->ptrXRRSelectInput = XRRSelectInput;
            X11->ptrXRRUpdateConfiguration = XRRUpdateConfiguration;
            X11->ptrXRRRootToScreen = XRRRootToScreen;
            X11->ptrXRRQueryExtension = XRRQueryExtension;
#  endif

            if (X11->ptrXRRQueryExtension
                && X11->ptrXRRQueryExtension(X11->display, &X11->xrandr_eventbase, &X11->xrandr_errorbase)) {
                // XRandR is supported
                X11->use_xrandr = true;
            }
        }
#endif // QT_NO_XRANDR

#ifndef QT_NO_XRENDER
        if (X11->use_xrender) {
            // XRender is supported, let's see if we have a PictFormat for the
            // default visual
            XRenderPictFormat *format =
                XRenderFindVisualFormat(X11->display,
                                        (Visual *) QX11Info::appVisual(X11->defaultScreen));

            if (!format) {
                X11->use_xrender = false;
            }
        }
#endif // QT_NO_XRENDER

#ifndef QT_NO_XFIXES
        // See if Xfixes is supported on the connected display
        if (XQueryExtension(X11->display, "XFIXES", &X11->xfixes_major,
                            &X11->xfixes_eventbase, &X11->xfixes_errorbase)) {
            X11->ptrXFixesQueryExtension  = XFIXES_LOAD_V1(XFixesQueryExtension);
            X11->ptrXFixesQueryVersion    = XFIXES_LOAD_V1(XFixesQueryVersion);
            X11->ptrXFixesSetCursorName   = XFIXES_LOAD_V2(XFixesSetCursorName);
            X11->ptrXFixesSelectSelectionInput = XFIXES_LOAD_V2(XFixesSelectSelectionInput);

            if(X11->ptrXFixesQueryExtension && X11->ptrXFixesQueryVersion
               && X11->ptrXFixesQueryExtension(X11->display, &X11->xfixes_eventbase,
                                               &X11->xfixes_errorbase)) {
                // Xfixes is supported.
                // Note: the XFixes protocol version is negotiated using QueryVersion.
                // We supply the highest version we support, the X server replies with
                // the highest version it supports, but no higher than the version we
                // asked for. The version sent back is the protocol version the X server
                // will use to talk us. If this call is removed, the behavior of the
                // X server when it receives an XFixes request is undefined.
                int major = 3;
                int minor = 0;
                X11->ptrXFixesQueryVersion(X11->display, &major, &minor);
                X11->use_xfixes = (major >= 1);
                X11->xfixes_major = major;
            }
        } else {
            X11->ptrXFixesQueryExtension  = 0;
            X11->ptrXFixesQueryVersion    = 0;
            X11->ptrXFixesSetCursorName   = 0;
            X11->ptrXFixesSelectSelectionInput = 0;
        }
#endif // QT_NO_XFIXES

#ifndef QT_NO_XCURSOR
#ifdef QT_RUNTIME_XCURSOR
        X11->ptrXcursorLibraryLoadCursor = 0;
        QLibrary xcursorLib(QLatin1String("Xcursor"), 1);
        bool xcursorFound = xcursorLib.load();
        if (!xcursorFound) { //try without the version number
            xcursorLib.setFileName(QLatin1String("Xcursor"));
            xcursorFound = xcursorLib.load();
        }
        if (xcursorFound) {
            X11->ptrXcursorLibraryLoadCursor =
                (PtrXcursorLibraryLoadCursor) xcursorLib.resolve("XcursorLibraryLoadCursor");
        }
#else
        X11->ptrXcursorLibraryLoadCursor = XcursorLibraryLoadCursor;
#endif // QT_RUNTIME_XCURSOR
#endif // QT_NO_XCURSOR

#ifndef QT_NO_XSYNC
        int xsync_evbase, xsync_errbase;
        int major, minor;
        if (XSyncQueryExtension(X11->display, &xsync_evbase, &xsync_errbase))
            XSyncInitialize(X11->display, &major, &minor);
#endif // QT_NO_XSYNC

#ifndef QT_NO_XINERAMA
#ifdef QT_RUNTIME_XINERAMA
        X11->ptrXineramaQueryExtension = 0;
        X11->ptrXineramaIsActive = 0;
        X11->ptrXineramaQueryScreens = 0;
        QLibrary xineramaLib(QLatin1String("Xinerama"), 1);
        bool xineramaFound = xineramaLib.load();
        if (!xineramaFound) { //try without the version number
            xineramaLib.setFileName(QLatin1String("Xinerama"));
            xineramaFound = xineramaLib.load();
        }
        if (xineramaFound) {
            X11->ptrXineramaQueryExtension =
                (PtrXineramaQueryExtension) xineramaLib.resolve("XineramaQueryExtension");
            X11->ptrXineramaIsActive =
                (PtrXineramaIsActive) xineramaLib.resolve("XineramaIsActive");
            X11->ptrXineramaQueryScreens =
                (PtrXineramaQueryScreens) xineramaLib.resolve("XineramaQueryScreens");
        }
#else
        X11->ptrXineramaQueryScreens = XineramaQueryScreens;
        X11->ptrXineramaIsActive = XineramaIsActive;
        X11->ptrXineramaQueryExtension = XineramaQueryExtension;
#endif // QT_RUNTIME_XINERAMA
#endif // QT_NO_XINERAMA

#ifndef QT_NO_XINPUT
        // See if Xinput is supported on the connected display
        X11->ptrXCloseDevice = 0;
        X11->ptrXListInputDevices = 0;
        X11->ptrXOpenDevice = 0;
        X11->ptrXFreeDeviceList = 0;
        X11->ptrXSelectExtensionEvent = 0;
        X11->use_xinput = XQueryExtension(X11->display, "XInputExtension", &X11->xinput_major,
                                          &X11->xinput_eventbase, &X11->xinput_errorbase);
        if (X11->use_xinput) {
            X11->ptrXCloseDevice = XINPUT_LOAD(XCloseDevice);
            X11->ptrXListInputDevices = XINPUT_LOAD(XListInputDevices);
            X11->ptrXOpenDevice = XINPUT_LOAD(XOpenDevice);
            X11->ptrXFreeDeviceList = XINPUT_LOAD(XFreeDeviceList);
            X11->ptrXSelectExtensionEvent = XINPUT_LOAD(XSelectExtensionEvent);
        }
#endif // QT_NO_XINPUT

#ifndef QT_NO_XKB
        int xkblibMajor = XkbMajorVersion;
        int xkblibMinor = XkbMinorVersion;
        X11->use_xkb = XkbQueryExtension(X11->display,
                                         &X11->xkb_major,
                                         &X11->xkb_eventbase,
                                         &X11->xkb_errorbase,
                                         &xkblibMajor,
                                         &xkblibMinor);
        if (X11->use_xkb) {
            // If XKB is detected, set the GrabsUseXKBState option so input method
            // compositions continue to work (ie. deadkeys)
            unsigned int state = XkbPCF_GrabsUseXKBStateMask;
            (void) XkbSetPerClientControls(X11->display, state, &state);

            // select for group change events
            XkbSelectEventDetails(X11->display,
                                  XkbUseCoreKbd,
                                  XkbStateNotify,
                                  XkbAllStateComponentsMask,
                                  XkbGroupStateMask);

            // current group state is queried when creating the keymapper, no need to do it here
        }
#endif


#if !defined(QT_NO_FONTCONFIG)
        int dpi = 0;
        getXDefault("Xft", FC_DPI, &dpi);
        if (dpi) {
            for (int s = 0; s < ScreenCount(X11->display); ++s) {
                QX11Info::setAppDpiX(s, dpi);
                QX11Info::setAppDpiY(s, dpi);
            }
        }
        double fc_scale = 1.;
        getXDefault("Xft", FC_SCALE, &fc_scale);
        X11->fc_scale = fc_scale;
        for (int s = 0; s < ScreenCount(X11->display); ++s) {
            int subpixel = FC_RGBA_UNKNOWN;
#if !defined(QT_NO_XRENDER) && (RENDER_MAJOR > 0 || RENDER_MINOR >= 6)
            if (X11->use_xrender) {
                int rsp = XRenderQuerySubpixelOrder(X11->display, s);
                switch (rsp) {
                default:
                case SubPixelUnknown:
                    subpixel = FC_RGBA_UNKNOWN;
                    break;
                case SubPixelHorizontalRGB:
                    subpixel = FC_RGBA_RGB;
                    break;
                case SubPixelHorizontalBGR:
                    subpixel = FC_RGBA_BGR;
                    break;
                case SubPixelVerticalRGB:
                    subpixel = FC_RGBA_VRGB;
                    break;
                case SubPixelVerticalBGR:
                    subpixel = FC_RGBA_VBGR;
                    break;
                case SubPixelNone:
                    subpixel = FC_RGBA_NONE;
                    break;
                }
            }
#endif

            char *rgba = XGetDefault(X11->display, "Xft", FC_RGBA);
            if (rgba) {
                char *end = 0;
                int v = strtol(rgba, &end, 0);
                if (rgba != end) {
                    subpixel = v;
                } else if (qstrncmp(rgba, "unknown", 7) == 0) {
                    subpixel = FC_RGBA_UNKNOWN;
                } else if (qstrncmp(rgba, "rgb", 3) == 0) {
                    subpixel = FC_RGBA_RGB;
                } else if (qstrncmp(rgba, "bgr", 3) == 0) {
                    subpixel = FC_RGBA_BGR;
                } else if (qstrncmp(rgba, "vrgb", 4) == 0) {
                    subpixel = FC_RGBA_VRGB;
                } else if (qstrncmp(rgba, "vbgr", 4) == 0) {
                    subpixel = FC_RGBA_VBGR;
                } else if (qstrncmp(rgba, "none", 4) == 0) {
                    subpixel = FC_RGBA_NONE;
                }
            }
            X11->screens[s].subpixel = subpixel;
        }
        getXDefault("Xft", FC_ANTIALIAS, &X11->fc_antialias);
#ifdef FC_HINT_STYLE
        X11->fc_hint_style = -1;
        getXDefault("Xft", FC_HINT_STYLE, &X11->fc_hint_style);
#endif
#if 0
        // ###### these are implemented by Xft, not sure we need them
        getXDefault("Xft", FC_AUTOHINT, &X11->fc_autohint);
        getXDefault("Xft", FC_HINTING, &X11->fc_autohint);
        getXDefault("Xft", FC_MINSPACE, &X11->fc_autohint);
#endif
#endif // QT_NO_XRENDER

        // initialize key mapper
        QKeyMapper::changeKeyboard();

        // Misc. initialization
#if 0 //disabled for now..
        QSegfaultHandler::initialize(priv->argv, priv->argc);
#endif
        QCursorData::initialize();
    }
    QFont::initialize();

    if(qt_is_gui_used) {
        qApp->setObjectName(QString::fromLocal8Bit(appName));

        int screen;
        for (screen = 0; screen < X11->screenCount; ++screen) {
            XSelectInput(X11->display, QX11Info::appRootWindow(screen),
                         KeymapStateMask | EnterWindowMask | LeaveWindowMask | PropertyChangeMask);

#ifndef QT_NO_XRANDR
            if (X11->use_xrandr)
                X11->ptrXRRSelectInput(X11->display, QX11Info::appRootWindow(screen), True);
#endif // QT_NO_XRANDR
        }
    }

    if (qt_is_gui_used) {
        // Attempt to determine the current running X11 Desktop Enviornment
        // Use dbus if/when we can, but fall back to using windowManagerName() for now

#ifndef QT_NO_XFIXES
        if (X11->ptrXFixesSelectSelectionInput)
            X11->ptrXFixesSelectSelectionInput(X11->display, QX11Info::appRootWindow(), ATOM(_NET_WM_CM_S0),
                                       XFixesSetSelectionOwnerNotifyMask
                                       | XFixesSelectionWindowDestroyNotifyMask
                                       | XFixesSelectionClientCloseNotifyMask);
#endif // QT_NO_XFIXES
        X11->compositingManagerRunning = XGetSelectionOwner(X11->display,
                                                            ATOM(_NET_WM_CM_S0));
        X11->desktopEnvironment = DE_UNKNOWN;
        X11->desktopVersion = 0;

        Atom type;
        int format;
        unsigned long length, after;
        uchar *data = 0;
        int rc;

        do {
            if (!qgetenv("KDE_FULL_SESSION").isEmpty()) {
                X11->desktopEnvironment = DE_KDE;
                X11->desktopVersion = qgetenv("KDE_SESSION_VERSION").toInt();
                break;
            }

            if (qgetenv("DESKTOP_SESSION") == "gnome") {
                X11->desktopEnvironment = DE_GNOME;
                break;
            }

            // GNOME_DESKTOP_SESSION_ID is deprecated for some reason, but still check it
            if (!qgetenv("GNOME_DESKTOP_SESSION_ID").isEmpty()) {
                X11->desktopEnvironment = DE_GNOME;
                break;
            }

            rc = XGetWindowProperty(X11->display, QX11Info::appRootWindow(), ATOM(_DT_SAVE_MODE),
                                    0, 2, False, XA_STRING, &type, &format, &length,
                                    &after, &data);
            if (rc == Success && length) {
                if (!strcmp(reinterpret_cast<char *>(data), "xfce4")) {
                    // Pretend that xfce4 is gnome, as it uses the same libraries.
                    // The detection above is stolen from xdg-open.
                    X11->desktopEnvironment = DE_GNOME;
                    break;
                }

                // We got the property but it wasn't xfce4. Free data before it gets overwritten.
                XFree(data);
                data = 0;
            }

            rc = XGetWindowProperty(X11->display, QX11Info::appRootWindow(), ATOM(DTWM_IS_RUNNING),
                                    0, 1, False, AnyPropertyType, &type, &format, &length,
                                    &after, &data);
            if (rc == Success && length) {
                // DTWM is running, meaning most likely CDE is running...
                X11->desktopEnvironment = DE_CDE;
                break;
            }

            rc = XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                                    ATOM(_SGI_DESKS_MANAGER), 0, 1, False, XA_WINDOW,
                                    &type, &format, &length, &after, &data);
            if (rc == Success && length) {
                X11->desktopEnvironment = DE_4DWM;
                break;
            }

            if (XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                               ATOM(_NET_SUPPORTING_WM_CHECK),
                               0, 1024, False, XA_WINDOW, &type,
                               &format, &length, &after, &data) == Success) {
                if (type == XA_WINDOW && format == 32) {
                    Window windowManagerWindow = *((Window*) data);
                    XFree(data);
                    data = 0;

                    if (windowManagerWindow != XNone) {
                        Atom utf8atom = ATOM(UTF8_STRING);
                        if (XGetWindowProperty(QX11Info::display(), windowManagerWindow, ATOM(_NET_WM_NAME),
                                               0, 1024, False, utf8atom, &type,
                                               &format, &length, &after, &data) == Success) {
                            if (type == utf8atom && format == 8) {
                                if (qstrcmp((const char *)data, "MCompositor") == 0)
                                    X11->desktopEnvironment = DE_MEEGO_COMPOSITOR;
                            }
                        }
                    }
                }
            }

        } while(0);

        if (data)
            XFree((char *)data);

#if !defined(QT_NO_STYLE_GTK)
        if (X11->desktopEnvironment == DE_GNOME) {
            static bool menusHaveIcons = QGtkStyle::getGConfBool(QLatin1String("/desktop/gnome/interface/menus_have_icons"), true);
            QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, !menusHaveIcons);
        }
#endif
        qt_set_input_encoding();

        qt_set_x11_resources(appFont, appFGCol, appBGCol, appBTNCol);

        // be smart about the size of the default font. most X servers have helvetica
        // 12 point available at 2 resolutions:
        //     75dpi (12 pixels) and 100dpi (17 pixels).
        // At 95 DPI, a 12 point font should be 16 pixels tall - in which case a 17
        // pixel font is a closer match than a 12 pixel font
        int ptsz = (X11->use_xrender
                    ? 9
                    : (int) (((QX11Info::appDpiY() >= 95 ? 17. : 12.) *
                              72. / (float) QX11Info::appDpiY()) + 0.5));

        if (!QApplicationPrivate::sys_font) {
            // no font from settings or RESOURCE_MANAGER, provide a fallback
            QFont f(X11->has_fontconfig ? QLatin1String("Sans Serif") : QLatin1String("Helvetica"),
                    ptsz);
            QApplicationPrivate::setSystemFont(f);
        }

#if !defined (QT_NO_TABLET)
        if (X11->use_xinput) {
            int ndev,
                i,
                j;
            bool gotStylus,
                gotEraser;
            XDeviceInfo *devices = 0, *devs;
            XInputClassInfo *ip;
            XAnyClassPtr any;
            XValuatorInfoPtr v;
            XAxisInfoPtr a;
            XDevice *dev = 0;

            if (X11->ptrXListInputDevices) {
                devices = X11->ptrXListInputDevices(X11->display, &ndev);
                if (!devices)
                    qWarning("QApplication: Failed to get list of tablet devices");
            }
            if (!devices)
                ndev = -1;
            QTabletEvent::TabletDevice deviceType;
            for (devs = devices, i = 0; i < ndev && devs; i++, devs++) {
                dev = 0;
                deviceType = QTabletEvent::NoDevice;
                gotStylus = false;
                gotEraser = false;

#if defined(Q_OS_IRIX)
                QString devName = QString::fromLocal8Bit(devs->name).toLower();
                if (devName == QLatin1String(WACOM_NAME)) {
                    deviceType = QTabletEvent::Stylus;
                    gotStylus = true;
                }
#else
                if (devs->type == ATOM(XWacomStylus) || devs->type == ATOM(XTabletStylus)) {
                    deviceType = QTabletEvent::Stylus;
                    if (wacomDeviceName()->isEmpty())
                        wacomDeviceName()->append(devs->name);
                    gotStylus = true;
                } else if (devs->type == ATOM(XWacomEraser) || devs->type == ATOM(XTabletEraser)) {
                    deviceType = QTabletEvent::XFreeEraser;
                    gotEraser = true;
                }
#endif
                if (deviceType == QTabletEvent::NoDevice)
                    continue;

                if (gotStylus || gotEraser) {
                    if (X11->ptrXOpenDevice)
                        dev = X11->ptrXOpenDevice(X11->display, devs->id);

                    if (!dev)
                        continue;

                    QTabletDeviceData device_data;
                    device_data.deviceType = deviceType;
                    device_data.eventCount = 0;
                    device_data.device = dev;
                    device_data.xinput_motion = -1;
                    device_data.xinput_key_press = -1;
                    device_data.xinput_key_release = -1;
                    device_data.xinput_button_press = -1;
                    device_data.xinput_button_release = -1;
                    device_data.xinput_proximity_in = -1;
                    device_data.xinput_proximity_out = -1;
                    device_data.widgetToGetPress = 0;

                    if (dev->num_classes > 0) {
                        for (ip = dev->classes, j = 0; j < dev->num_classes;
                             ip++, j++) {
                            switch (ip->input_class) {
                            case KeyClass:
                                DeviceKeyPress(dev, device_data.xinput_key_press,
                                               device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                DeviceKeyRelease(dev, device_data.xinput_key_release,
                                                 device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                break;
                            case ButtonClass:
                                DeviceButtonPress(dev, device_data.xinput_button_press,
                                                  device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                DeviceButtonRelease(dev, device_data.xinput_button_release,
                                                    device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                break;
                            case ValuatorClass:
                                // I'm only going to be interested in motion when the
                                // stylus is already down anyway!
                                DeviceMotionNotify(dev, device_data.xinput_motion,
                                                   device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                ProximityIn(dev, device_data.xinput_proximity_in, device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                                ProximityOut(dev, device_data.xinput_proximity_out, device_data.eventList[device_data.eventCount]);
                                if (device_data.eventList[device_data.eventCount])
                                    ++device_data.eventCount;
                            default:
                                break;
                            }
                        }
                    }

                    // get the min/max value for pressure!
                    any = (XAnyClassPtr) (devs->inputclassinfo);
                    for (j = 0; j < devs->num_classes; j++) {
                        if (any->c_class == ValuatorClass) {
                            v = (XValuatorInfoPtr) any;
                            a = (XAxisInfoPtr) ((char *) v +
                                                sizeof (XValuatorInfo));
#if defined (Q_OS_IRIX)
                            // I'm not exaclty wild about this, but the
                            // dimensions of the tablet are more relevant here
                            // than the min and max values from the axis
                            // (actually it seems to be 2/3 or what is in the
                            // axis.  So we'll try to parse it from this
                            // string. --tws
                            char returnString[SGIDeviceRtrnLen];
                            int tmp;
                            if (XSGIMiscQueryExtension(X11->display, &tmp, &tmp)
                                && XSGIDeviceQuery(X11->display, devs->id,
                                                   "dimensions", returnString)) {
                                QString str = QLatin1String(returnString);
                                int comma = str.indexOf(',');
                                device_data.minX = 0;
                                device_data.minY = 0;
                                device_data.maxX = str.left(comma).toInt();
                                device_data.maxY = str.mid(comma + 1).toInt();
                            } else {
                                device_data.minX = a[WAC_XCOORD_I].min_value;
                                device_data.maxX = a[WAC_XCOORD_I].max_value;
                                device_data.minY = a[WAC_YCOORD_I].min_value;
                                device_data.maxY = a[WAC_YCOORD_I].max_value;
                            }
                            device_data.minPressure = a[WAC_PRESSURE_I].min_value;
                            device_data.maxPressure = a[WAC_PRESSURE_I].max_value;
                            device_data.minTanPressure = a[WAC_TAN_PRESSURE_I].min_value;
                            device_data.maxTanPressure = a[WAC_TAN_PRESSURE_I].max_value;
                            device_data.minZ = a[WAC_ZCOORD_I].min_value;
                            device_data.maxZ = a[WAC_ZCOORD_I].max_value;
#else
                            device_data.minX = a[0].min_value;
                            device_data.maxX = a[0].max_value;
                            device_data.minY = a[1].min_value;
                            device_data.maxY = a[1].max_value;
                            device_data.minPressure = a[2].min_value;
                            device_data.maxPressure = a[2].max_value;
                            device_data.minTanPressure = 0;
                            device_data.maxTanPressure = 0;
                            device_data.minZ = 0;
                            device_data.maxZ = 0;
#endif

                            // got the max pressure no need to go further...
                            break;
                        }
                        any = (XAnyClassPtr) ((char *) any + any->length);
                    } // end of for loop

                    tablet_devices()->append(device_data);
                } // if (gotStylus || gotEraser)
            }
            if (X11->ptrXFreeDeviceList)
                X11->ptrXFreeDeviceList(devices);
        }
#endif // QT_NO_TABLET

        X11->startupId = getenv("DESKTOP_STARTUP_ID");
        if (X11->startupId) {
#ifndef QT_NO_UNSETENV
            unsetenv("DESKTOP_STARTUP_ID");
#else
            // it's a small memory leak, however we won't crash if Qt is
            // unloaded and someones tries to use the envoriment.
            putenv(strdup("DESKTOP_STARTUP_ID="));
#endif
        }
   } else {
        // read some non-GUI settings when not using the X server...

        if (QApplication::desktopSettingsAware()) {
            QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
            settings.beginGroup(QLatin1String("Qt"));

            // read library (ie. plugin) path list
            QString libpathkey = QString::fromLatin1("%1.%2/libraryPath")
                                 .arg(QT_VERSION >> 16)
                                 .arg((QT_VERSION & 0xff00) >> 8);
            QStringList pathlist =
                settings.value(libpathkey).toString().split(QLatin1Char(':'));
            if (! pathlist.isEmpty()) {
                QStringList::ConstIterator it = pathlist.constBegin();
                while (it != pathlist.constEnd())
                    QApplication::addLibraryPath(*it++);
            }

            QString defaultcodec = settings.value(QLatin1String("defaultCodec"),
                                                  QVariant(QLatin1String("none"))).toString();
            if (defaultcodec != QLatin1String("none")) {
                QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1());
                if (codec)
                    QTextCodec::setCodecForTr(codec);
            }

            settings.endGroup(); // Qt
        }
    }

#if !defined (Q_OS_IRIX) && !defined (QT_NO_TABLET)
    QLibrary wacom(QString::fromLatin1("wacomcfg"), 0); // version 0 is the latest release at time of writing this.
    // NOTE: C casts instead of reinterpret_cast for GCC 3.3.x
    ptrWacomConfigInit = (PtrWacomConfigInit)wacom.resolve("WacomConfigInit");
    ptrWacomConfigOpenDevice = (PtrWacomConfigOpenDevice)wacom.resolve("WacomConfigOpenDevice");
    ptrWacomConfigGetRawParam  = (PtrWacomConfigGetRawParam)wacom.resolve("WacomConfigGetRawParam");
    ptrWacomConfigCloseDevice = (PtrWacomConfigCloseDevice)wacom.resolve("WacomConfigCloseDevice");
    ptrWacomConfigTerm = (PtrWacomConfigTerm)wacom.resolve("WacomConfigTerm");

    if (ptrWacomConfigInit == 0 || ptrWacomConfigOpenDevice == 0 || ptrWacomConfigGetRawParam == 0
        || ptrWacomConfigCloseDevice == 0 || ptrWacomConfigTerm == 0) { // either we have all, or we have none.
            ptrWacomConfigInit = 0;
            ptrWacomConfigOpenDevice = 0;
            ptrWacomConfigGetRawParam  = 0;
            ptrWacomConfigCloseDevice = 0;
            ptrWacomConfigTerm = 0;
    }
#endif
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
}

/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/

void qt_cleanup()
{
    if (app_save_rootinfo)                        // root window must keep state
        qt_save_rootinfo();

    if (qt_is_gui_used) {
        QPixmapCache::clear();
        QCursorData::cleanup();
        QFont::cleanup();
        QColormap::cleanup();

#if !defined (QT_NO_TABLET)
        QTabletDeviceDataList *devices = qt_tablet_devices();
        if (X11->ptrXCloseDevice)
            for (int i = 0; i < devices->size(); ++i)
                X11->ptrXCloseDevice(X11->display, (XDevice*)devices->at(i).device);
        devices->clear();
#endif
    }

#ifndef QT_NO_XRENDER
    for (int i = 0; i < X11->solid_fill_count; ++i) {
        if (X11->solid_fills[i].picture)
            XRenderFreePicture(X11->display, X11->solid_fills[i].picture);
    }
    for (int i = 0; i < X11->pattern_fill_count; ++i) {
        if (X11->pattern_fills[i].picture)
            XRenderFreePicture(X11->display, X11->pattern_fills[i].picture);
    }
#endif

#if !defined(QT_NO_IM)
    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;
#endif

    // Reset the error handlers
    if (qt_is_gui_used)
        XSync(X11->display, False); // sync first to process all possible errors
    XSetErrorHandler(original_x_errhandler);
    XSetIOErrorHandler(original_xio_errhandler);

    if (X11->argbColormaps) {
        for (int s = 0; s < X11->screenCount; s++) {
            if (X11->argbColormaps[s])
                XFreeColormap(X11->display, X11->argbColormaps[s]);
        }
    }

    if (qt_is_gui_used && !X11->foreignDisplay)
        XCloseDisplay(X11->display);                // close X display
    X11->display = 0;

    delete [] X11->screens;
    delete [] X11->argbVisuals;
    delete [] X11->argbColormaps;

    if (X11->foreignDisplay) {
        delete [] (char *)appName;
        appName = 0;
    }

    delete [] (char *)appClass;
    appClass = 0;

    if (X11->net_supported_list)
        delete [] X11->net_supported_list;
    X11->net_supported_list = 0;

    if (X11->net_virtual_root_list)
        delete [] X11->net_virtual_root_list;
    X11->net_virtual_root_list = 0;

    delete X11;
    X11 = 0;
}


/*****************************************************************************
  Platform specific global and internal functions
 *****************************************************************************/

void qt_save_rootinfo()                                // save new root info
{
    Atom type;
    int format;
    unsigned long length, after;
    uchar *data = 0;

    if (ATOM(_XSETROOT_ID)) {                        // kill old pixmap
        if (XGetWindowProperty(X11->display, QX11Info::appRootWindow(),
                                 ATOM(_XSETROOT_ID), 0, 1,
                                 True, AnyPropertyType, &type, &format,
                                 &length, &after, &data) == Success) {
            if (type == XA_PIXMAP && format == 32 && length == 1 &&
                 after == 0 && data) {
                XKillClient(X11->display, *((Pixmap*)data));
            }
            Pixmap dummy = XCreatePixmap(X11->display, QX11Info::appRootWindow(),
                                          1, 1, 1);
            XChangeProperty(X11->display, QX11Info::appRootWindow(),
                             ATOM(_XSETROOT_ID), XA_PIXMAP, 32,
                             PropModeReplace, (uchar *)&dummy, 1);
            XSetCloseDownMode(X11->display, RetainPermanent);
        }
    }
    if (data)
        XFree((char *)data);
}

void qt_updated_rootinfo()
{
    app_save_rootinfo = true;
}

// ### Cleanup, this function is not in use!
bool qt_wstate_iconified(WId winid)
{
    Atom type;
    int format;
    unsigned long length, after;
    uchar *data = 0;
    int r = XGetWindowProperty(X11->display, winid, ATOM(WM_STATE), 0, 2,
                                 False, AnyPropertyType, &type, &format,
                                 &length, &after, &data);
    bool iconic = false;
    if (r == Success && data && format == 32) {
        // quint32 *wstate = (quint32*)data;
        unsigned long *wstate = (unsigned long *) data;
        iconic = (*wstate == IconicState);
        XFree((char *)data);
    }
    return iconic;
}

QString QApplicationPrivate::appName() const
{
    return QString::fromLocal8Bit(QT_PREPEND_NAMESPACE(appName));
}

const char *QX11Info::appClass()                                // get application class
{
    return QT_PREPEND_NAMESPACE(appClass);
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}


/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/

#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
#ifndef QT_NO_DEBUG
    if (mainWidget && mainWidget->parentWidget() && mainWidget->isWindow())
        qWarning("QApplication::setMainWidget: New main widget (%s/%s) "
                  "has a parent",
                  mainWidget->metaObject()->className(), mainWidget->objectName().toLocal8Bit().constData());
#endif
    if (mainWidget)
        mainWidget->d_func()->createWinId();
    QApplicationPrivate::main_widget = mainWidget;
    if (QApplicationPrivate::main_widget) // give WM command line
        QApplicationPrivate::applyX11SpecificCommandLineArguments(QApplicationPrivate::main_widget);
}
#endif

void QApplicationPrivate::applyX11SpecificCommandLineArguments(QWidget *main_widget)
{
    static bool beenHereDoneThat = false;
    if (beenHereDoneThat)
        return;
    beenHereDoneThat = true;
    Q_ASSERT(main_widget->testAttribute(Qt::WA_WState_Created));
    if (mwTitle) {
        XStoreName(X11->display, main_widget->effectiveWinId(), (char*)mwTitle);
        QByteArray net_wm_name = QString::fromLocal8Bit(mwTitle).toUtf8();
        XChangeProperty(X11->display, main_widget->effectiveWinId(), ATOM(_NET_WM_NAME), ATOM(UTF8_STRING), 8,
                        PropModeReplace, (unsigned char *)net_wm_name.data(), net_wm_name.size());
    }
    if (mwGeometry) { // parse geometry
        int x, y;
        int w, h;
        int m = XParseGeometry((char*)mwGeometry, &x, &y, (uint*)&w, (uint*)&h);
        QSize minSize = main_widget->minimumSize();
        QSize maxSize = main_widget->maximumSize();
        if ((m & XValue) == 0)
            x = main_widget->geometry().x();
        if ((m & YValue) == 0)
            y = main_widget->geometry().y();
        if ((m & WidthValue) == 0)
            w = main_widget->width();
        if ((m & HeightValue) == 0)
            h = main_widget->height();
        w = qMin(w,maxSize.width());
        h = qMin(h,maxSize.height());
        w = qMax(w,minSize.width());
        h = qMax(h,minSize.height());
        if ((m & XNegative)) {
            x = QApplication::desktop()->width()  + x - w;
        }
        if ((m & YNegative)) {
            y = QApplication::desktop()->height() + y - h;
        }
        main_widget->setGeometry(x, y, w, h);
    }
}

#ifndef QT_NO_CURSOR

/*****************************************************************************
  QApplication cursor stack
 *****************************************************************************/

void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);

    QWidgetList all = allWidgets();
    for (QWidgetList::ConstIterator it = all.constBegin(); it != all.constEnd(); ++it) {
        register QWidget *w = *it;
        if ((w->testAttribute(Qt::WA_SetCursor) || w->isWindow()) && (w->windowType() != Qt::Desktop))
            qt_x11_enforce_cursor(w);
    }
    XFlush(X11->display);                                // make X execute it NOW
}

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();

    if (QWidgetPrivate::mapper != 0 && !closingDown()) {
        QWidgetList all = allWidgets();
        for (QWidgetList::ConstIterator it = all.constBegin(); it != all.constEnd(); ++it) {
            register QWidget *w = *it;
            if ((w->testAttribute(Qt::WA_SetCursor) || w->isWindow()) && (w->windowType() != Qt::Desktop))
                qt_x11_enforce_cursor(w);
        }
        XFlush(X11->display);
    }
}

#endif


/*****************************************************************************
  Routines to find a Qt widget from a screen position
 *****************************************************************************/

Window QX11Data::findClientWindow(Window win, Atom property, bool leaf)
{
    Atom   type = XNone;
    int           format, i;
    ulong  nitems, after;
    uchar *data = 0;
    Window root, parent, target=0, *children=0;
    uint   nchildren;
    if (XGetWindowProperty(X11->display, win, property, 0, 0, false, AnyPropertyType,
                             &type, &format, &nitems, &after, &data) == Success) {
        if (data)
            XFree((char *)data);
        if (type)
            return win;
    }
    if (!XQueryTree(X11->display,win,&root,&parent,&children,&nchildren)) {
        if (children)
            XFree((char *)children);
        return 0;
    }
    for (i=nchildren-1; !target && i >= 0; i--)
        target = X11->findClientWindow(children[i], property, leaf);
    if (children)
        XFree((char *)children);
    return target;
}

QWidget *QApplication::topLevelAt(const QPoint &p)
{
#ifdef QT_NO_CURSOR
    Q_UNUSED(p);
    return 0;
#else
    int screen = QCursor::x11Screen();
    int unused;

    int x = p.x();
    int y = p.y();
    Window target;
    if (!XTranslateCoordinates(X11->display,
                               QX11Info::appRootWindow(screen),
                               QX11Info::appRootWindow(screen),
                               x, y, &unused, &unused, &target)) {
        return 0;
    }
    if (!target || target == QX11Info::appRootWindow(screen))
        return 0;
    QWidget *w;
    w = QWidget::find((WId)target);

    if (!w) {
        X11->ignoreBadwindow();
        target = X11->findClientWindow(target, ATOM(WM_STATE), true);
        if (X11->badwindow())
            return 0;
        w = QWidget::find((WId)target);
        if (!w) {
            // Perhaps the widget at (x,y) is inside a foreign application?
            // Search all toplevel widgets to see if one is within target
            QWidgetList list = QApplication::topLevelWidgets();
            for (int i = 0; i < list.count(); ++i) {
                QWidget *widget = list.at(i);
                Window ctarget = target;
                if (widget->isVisible() && !(widget->windowType() == Qt::Desktop)) {
                    Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
                    Window wid = widget->internalWinId();
                    while (ctarget && !w) {
                        X11->ignoreBadwindow();
                        if (!XTranslateCoordinates(X11->display,
                                                   QX11Info::appRootWindow(screen),
                                                   ctarget, x, y, &unused, &unused, &ctarget)
                                || X11->badwindow())
                            break;
                        if (ctarget == wid) {
                            // Found!
                            w = widget;
                            break;
                        }
                    }
                }
                if (w)
                    break;
            }
        }
    }
    return w ? w->window() : 0;
#endif
}

void QApplication::syncX()
{
    if (X11->display)
        XSync(X11->display, False);  // don't discard events
}


void QApplication::beep()
{
    if (X11->display)
        XBell(X11->display, 0);
    else
        printf("\7");
}

void QApplication::alert(QWidget *widget, int msec)
{
    if (!QApplicationPrivate::checkInstance("alert"))
        return;

    QWidgetList windowsToMark;
    if (!widget) {
        windowsToMark += topLevelWidgets();
    } else {
        windowsToMark.append(widget->window());
    }

    for (int i = 0; i < windowsToMark.size(); ++i) {
        QWidget *window = windowsToMark.at(i);
        if (!window->isActiveWindow()) {
            qt_change_net_wm_state(window, true, ATOM(_NET_WM_STATE_DEMANDS_ATTENTION));
            if (msec != 0) {
                QTimer *timer = new QTimer(qApp);
                timer->setSingleShot(true);
                connect(timer, SIGNAL(timeout()), qApp, SLOT(_q_alertTimeOut()));
                if (QTimer *oldTimer = qApp->d_func()->alertTimerHash.value(window)) {
                    qApp->d_func()->alertTimerHash.remove(window);
                    delete oldTimer;
                }
                qApp->d_func()->alertTimerHash.insert(window, timer);
                timer->start(msec);
            }
        }
    }
}

void QApplicationPrivate::_q_alertTimeOut()
{
    if (QTimer *timer = qobject_cast<QTimer *>(q_func()->sender())) {
        QHash<QWidget *, QTimer *>::iterator it = alertTimerHash.begin();
        while (it != alertTimerHash.end()) {
            if (it.value() == timer) {
                QWidget *window = it.key();
                qt_change_net_wm_state(window, false, ATOM(_NET_WM_STATE_DEMANDS_ATTENTION));
                alertTimerHash.erase(it);
                timer->deleteLater();
                break;
            }
            ++it;
        }
    }
}

Qt::KeyboardModifiers QApplication::queryKeyboardModifiers()
{
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint keybstate;
    for (int i = 0; i < ScreenCount(X11->display); ++i) {
        if (XQueryPointer(X11->display, QX11Info::appRootWindow(i), &root, &child,
                          &root_x, &root_y, &win_x, &win_y, &keybstate))
            return X11->translateModifiers(keybstate & 0x00ff);
    }
    return 0;

}

/*****************************************************************************
  Special lookup functions for windows that have been reparented recently
 *****************************************************************************/

static QWidgetMapper *wPRmapper = 0;                // alternative widget mapper

void qPRCreate(const QWidget *widget, Window oldwin)
{                                                // QWidget::reparent mechanism
    if (!wPRmapper)
        wPRmapper = new QWidgetMapper;

    QETWidget *w = static_cast<QETWidget *>(const_cast<QWidget *>(widget));
    wPRmapper->insert((int)oldwin, w);        // add old window to mapper
    w->setAttribute(Qt::WA_WState_Reparented);        // set reparented flag
}

void qPRCleanup(QWidget *widget)
{
    QETWidget *etw = static_cast<QETWidget *>(const_cast<QWidget *>(widget));
    if (!(wPRmapper && widget->testAttribute(Qt::WA_WState_Reparented)))
        return;                                        // not a reparented widget
    QWidgetMapper::Iterator it = wPRmapper->begin();
    while (it != wPRmapper->constEnd()) {
        QWidget *w = *it;
        if (w == etw) {                       // found widget
            etw->setAttribute(Qt::WA_WState_Reparented, false); // clear flag
            it = wPRmapper->erase(it);// old window no longer needed
        } else {
            ++it;
        }
    }
    if (wPRmapper->size() == 0) {        // became empty
        delete wPRmapper;                // then reset alt mapper
        wPRmapper = 0;
    }
}

static QETWidget *qPRFindWidget(Window oldwin)
{
    return wPRmapper ? (QETWidget*)wPRmapper->value((int)oldwin, 0) : 0;
}

int QApplication::x11ClientMessage(QWidget* w, XEvent* event, bool passive_only)
{
    if (w && !w->internalWinId())
        return 0;
    QETWidget *widget = (QETWidget*)w;
    if (event->xclient.format == 32 && event->xclient.message_type) {
        if (event->xclient.message_type == ATOM(WM_PROTOCOLS)) {
            Atom a = event->xclient.data.l[0];
            if (a == ATOM(WM_DELETE_WINDOW)) {
                if (passive_only) return 0;
                widget->translateCloseEvent(event);
            }
            else if (a == ATOM(WM_TAKE_FOCUS)) {
                if ((ulong) event->xclient.data.l[1] > X11->time)
                    X11->time = event->xclient.data.l[1];
                QWidget *amw = activeModalWidget();
                if (amw && amw->testAttribute(Qt::WA_X11DoNotAcceptFocus))
                    amw = 0;
                if (amw && !QApplicationPrivate::tryModalHelper(widget, 0)) {
                    QWidget *p = amw->parentWidget();
                    while (p && p != widget)
                        p = p->parentWidget();
                    if (!p || !X11->net_supported_list)
                        amw->raise(); // help broken window managers
                    amw->activateWindow();
                }
#ifndef QT_NO_WHATSTHIS
            } else if (a == ATOM(_NET_WM_CONTEXT_HELP)) {
                QWhatsThis::enterWhatsThisMode();
#endif // QT_NO_WHATSTHIS
            } else if (a == ATOM(_NET_WM_PING)) {
                // avoid send/reply loops
                Window root = RootWindow(X11->display, w->x11Info().screen());
                if (event->xclient.window != root) {
                    event->xclient.window = root;
                    XSendEvent(event->xclient.display, event->xclient.window,
                                False, SubstructureNotifyMask|SubstructureRedirectMask, event);
                }
#ifndef QT_NO_XSYNC
            } else if (a == ATOM(_NET_WM_SYNC_REQUEST)) {
                const ulong timestamp = (const ulong) event->xclient.data.l[1];
                if (timestamp > X11->time)
                    X11->time = timestamp;
                if (QTLWExtra *tlw = w->d_func()->maybeTopData()) {
                    if (timestamp == CurrentTime || timestamp > tlw->syncRequestTimestamp) {
                        tlw->syncRequestTimestamp = timestamp;
                        tlw->newCounterValueLo = event->xclient.data.l[2];
                        tlw->newCounterValueHi = event->xclient.data.l[3];
                    }
                }
#endif
            }
        } else if (event->xclient.message_type == ATOM(_QT_SCROLL_DONE)) {
            widget->translateScrollDoneEvent(event);
        } else if (event->xclient.message_type == ATOM(XdndPosition)) {
            X11->xdndHandlePosition(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndEnter)) {
            X11->xdndHandleEnter(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndStatus)) {
            X11->xdndHandleStatus(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndLeave)) {
            X11->xdndHandleLeave(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndDrop)) {
            X11->xdndHandleDrop(widget, event, passive_only);
        } else if (event->xclient.message_type == ATOM(XdndFinished)) {
            X11->xdndHandleFinished(widget, event, passive_only);
        } else {
            if (passive_only) return 0;
            // All other are interactions
        }
    } else {
        X11->motifdndHandle(widget, event, passive_only);
    }

    return 0;
}

int QApplication::x11ProcessEvent(XEvent* event)
{
    Q_D(QApplication);
    QScopedLoopLevelCounter loopLevelCounter(d->threadData);

#ifdef ALIEN_DEBUG
    //qDebug() << "QApplication::x11ProcessEvent:" << event->type;
#endif
    switch (event->type) {
    case ButtonPress:
        pressed_window = event->xbutton.window;
        X11->userTime = event->xbutton.time;
        // fallthrough intended
    case ButtonRelease:
        X11->time = event->xbutton.time;
        break;
    case MotionNotify:
        X11->time = event->xmotion.time;
        break;
    case XKeyPress:
        X11->userTime = event->xkey.time;
        // fallthrough intended
    case XKeyRelease:
        X11->time = event->xkey.time;
        break;
    case PropertyNotify:
        X11->time = event->xproperty.time;
        break;
    case EnterNotify:
    case LeaveNotify:
        X11->time = event->xcrossing.time;
        break;
    case SelectionClear:
        X11->time = event->xselectionclear.time;
        break;
    default:
        break;
    }
#ifndef QT_NO_XFIXES
    if (X11->use_xfixes && event->type == (X11->xfixes_eventbase + XFixesSelectionNotify)) {
        XFixesSelectionNotifyEvent *req =
            reinterpret_cast<XFixesSelectionNotifyEvent *>(event);
        X11->time = req->selection_timestamp;
        if (req->selection == ATOM(_NET_WM_CM_S0))
            X11->compositingManagerRunning = req->owner;
    }
#endif

    QETWidget *widget = (QETWidget*)QWidget::find((WId)event->xany.window);

    if (wPRmapper) {                                // just did a widget reparent?
        if (widget == 0) {                        // not in std widget mapper
            switch (event->type) {                // only for mouse/key events
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
            case XKeyPress:
            case XKeyRelease:
                widget = qPRFindWidget(event->xany.window);
                break;
            }
        }
        else if (widget->testAttribute(Qt::WA_WState_Reparented))
            qPRCleanup(widget);                // remove from alt mapper
    }

    QETWidget *keywidget=0;
    bool grabbed=false;
    if (event->type==XKeyPress || event->type==XKeyRelease) {
        keywidget = (QETWidget*)QWidget::keyboardGrabber();
        if (keywidget) {
            grabbed = true;
        } else if (!keywidget) {
            if (d->inPopupMode()) // no focus widget, see if we have a popup
                keywidget = (QETWidget*) (activePopupWidget()->focusWidget() ? activePopupWidget()->focusWidget() : activePopupWidget());
            else if (QApplicationPrivate::focus_widget)
                keywidget = (QETWidget*)QApplicationPrivate::focus_widget;
            else if (widget)
                keywidget = (QETWidget*)widget->window();
        }
    }

#ifndef QT_NO_IM
    // Filtering input events by the input context. It has to be taken
    // place before any other key event consumers such as eventfilters
    // and accelerators because some input methods require quite
    // various key combination and sequences. It often conflicts with
    // accelerators and so on, so we must give the input context the
    // filtering opportunity first to ensure all input methods work
    // properly regardless of application design.

    if(keywidget && keywidget->isEnabled() && keywidget->testAttribute(Qt::WA_InputMethodEnabled)) {
        // block user interaction during session management
	if((event->type==XKeyPress || event->type==XKeyRelease) && qt_sm_blockUserInput)
	    return true;

        // for XIM handling
	QInputContext *qic = keywidget->inputContext();
	if(qic && qic->x11FilterEvent(keywidget, event))
	    return true;

	// filterEvent() accepts QEvent *event rather than preexpanded
	// key event attribute values. This is intended to pass other
	// QInputEvent in future. Other non IM-related events should
	// not be forwarded to input contexts to prevent weird event
	// handling.
	if ((event->type == XKeyPress || event->type == XKeyRelease)) {
	    int code = -1;
	    int count = 0;
	    Qt::KeyboardModifiers modifiers;
	    QEvent::Type type;
	    QString text;
            KeySym keySym;

            qt_keymapper_private()->translateKeyEventInternal(keywidget, event, keySym, count,
                                                              text, modifiers, code, type, false);

	    // both key press/release is required for some complex
	    // input methods. don't eliminate anything.
	    QKeyEventEx keyevent(type, code, modifiers, text, false, qMax(qMax(count, 1), text.length()),
                                 event->xkey.keycode, keySym, event->xkey.state);
	    if(qic && qic->filterEvent(&keyevent))
		return true;
	}
    } else
#endif // QT_NO_IM
        {
            if (XFilterEvent(event, XNone))
                return true;
        }

    if (qt_x11EventFilter(event))                // send through app filter
        return 1;

    if (event->type == MappingNotify) {
        // keyboard mapping changed
        XRefreshKeyboardMapping(&event->xmapping);

        QKeyMapper::changeKeyboard();
        return 0;
    }
#ifndef QT_NO_XKB
    else if (X11->use_xkb && event->type == X11->xkb_eventbase) {
        XkbAnyEvent *xkbevent = (XkbAnyEvent *) event;
        switch (xkbevent->xkb_type) {
        case XkbStateNotify:
            {
                XkbStateNotifyEvent *xkbstateevent = (XkbStateNotifyEvent *) xkbevent;
                if ((xkbstateevent->changed & XkbGroupStateMask) != 0) {
                    qt_keymapper_private()->xkb_currentGroup = xkbstateevent->group;
                    QKeyMapper::changeKeyboard();
                }
                break;
            }
        default:
            break;
        }
    }
#endif

    if (!widget) {                                // don't know this windows
        QWidget* popup = QApplication::activePopupWidget();
        if (popup) {

            /*
              That is more than suboptimal. The real solution should
              do some keyevent and buttonevent translation, so that
              the popup still continues to work as the user expects.
              Unfortunately this translation is currently only
              possible with a known widget. I'll change that soon
              (Matthias).
            */

            // Danger - make sure we don't lock the server
            switch (event->type) {
            case ButtonPress:
            case ButtonRelease:
            case XKeyPress:
            case XKeyRelease:
                do {
                    popup->close();
                } while ((popup = qApp->activePopupWidget()));
                return 1;
            }
        }
        return -1;
    }

    if (event->type == XKeyPress || event->type == XKeyRelease)
        widget = keywidget; // send XKeyEvents through keywidget->x11Event()

    if (app_do_modal)                                // modal event handling
        if (!qt_try_modal(widget, event)) {
            if (event->type == ClientMessage && !widget->x11Event(event))
                x11ClientMessage(widget, event, true);
            return 1;
        }


    if (widget->x11Event(event))                // send through widget filter
        return 1;
#if !defined (QT_NO_TABLET)
    if (!qt_xdnd_dragging) {
        QTabletDeviceDataList *tablets = qt_tablet_devices();
        for (int i = 0; i < tablets->size(); ++i) {
            QTabletDeviceData &tab = tablets->operator [](i);
            if (event->type == tab.xinput_motion
            || event->type == tab.xinput_button_release
            || event->type == tab.xinput_button_press
            || event->type == tab.xinput_proximity_in
            || event->type == tab.xinput_proximity_out) {
                widget->translateXinputEvent(event, &tab);
                return 0;
            }
        }
    }
#endif

#ifndef QT_NO_XRANDR
    if (X11->use_xrandr && event->type == (X11->xrandr_eventbase + RRScreenChangeNotify)) {
        // update Xlib internals with the latest screen configuration
        X11->ptrXRRUpdateConfiguration(event);

        // update the size for desktop widget
        int scr = X11->ptrXRRRootToScreen(X11->display, event->xany.window);
        QDesktopWidget *desktop = QApplication::desktop();
        QWidget *w = desktop->screen(scr);
        QSize oldSize(w->size());
        w->data->crect.setWidth(DisplayWidth(X11->display, scr));
        w->data->crect.setHeight(DisplayHeight(X11->display, scr));
        QResizeEvent e(w->size(), oldSize);
        QApplication::sendEvent(w, &e);
        if (w != desktop)
            QApplication::sendEvent(desktop, &e);
    }
#endif // QT_NO_XRANDR

#ifndef QT_NO_XFIXES
    if (X11->use_xfixes && event->type == (X11->xfixes_eventbase + XFixesSelectionNotify)) {
        XFixesSelectionNotifyEvent *req = reinterpret_cast<XFixesSelectionNotifyEvent *>(event);

        // compress all XFixes events related to this selection
        // we don't want to handle old SelectionNotify events.
        qt_xfixes_selection_event_data xfixes_event;
        xfixes_event.selection = req->selection;
        for (XEvent ev;;) {
            if (!XCheckIfEvent(X11->display, &ev, &qt_xfixes_scanner, (XPointer)&xfixes_event))
                break;
        }

        if (req->selection == ATOM(CLIPBOARD)) {
            if (qt_xfixes_clipboard_changed(req->owner, req->selection_timestamp)) {
                emit clipboard()->changed(QClipboard::Clipboard);
                emit clipboard()->dataChanged();
            }
        } else if (req->selection == XA_PRIMARY) {
            if (qt_xfixes_selection_changed(req->owner, req->selection_timestamp)) {
                emit clipboard()->changed(QClipboard::Selection);
                emit clipboard()->selectionChanged();
            }
        }
    }
#endif // QT_NO_XFIXES

    switch (event->type) {

    case ButtonRelease:                        // mouse event
        if (!d->inPopupMode() && !QWidget::mouseGrabber() && pressed_window != widget->internalWinId()
            && (widget = (QETWidget*) QWidget::find((WId)pressed_window)) == 0)
            break;
        // fall through intended
    case ButtonPress:
        if (event->xbutton.root != RootWindow(X11->display, widget->x11Info().screen())
            && ! qt_xdnd_dragging) {
            while (activePopupWidget())
                activePopupWidget()->close();
            return 1;
        }
        if (event->type == ButtonPress)
            qt_net_update_user_time(widget->window(), X11->userTime);
        // fall through intended
    case MotionNotify:
#if !defined(QT_NO_TABLET)
        if (!qt_tabletChokeMouse) {
#endif
            if (widget->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                QPoint pos(event->xbutton.x, event->xbutton.y);
                pos = widget->d_func()->mapFromWS(pos);
                QWidget *window = widget->window();
                pos = widget->mapTo(window, pos);
                if (QWidget *child = window->childAt(pos)) {
                    widget = static_cast<QETWidget *>(child);
                    pos = child->mapFrom(window, pos);
                    event->xbutton.x = pos.x();
                    event->xbutton.y = pos.y();
                }
            }
            widget->translateMouseEvent(event);
#if !defined(QT_NO_TABLET)
        } else {
            qt_tabletChokeMouse = false;
        }
#endif
        break;

    case XKeyPress:                                // keyboard event
        qt_net_update_user_time(widget->window(), X11->userTime);
        // fallthrough intended
    case XKeyRelease:
        {
            if (keywidget && keywidget->isEnabled()) { // should always exist
                // qDebug("sending key event");
                qt_keymapper_private()->translateKeyEvent(keywidget, event, grabbed);
            }
            break;
        }

    case GraphicsExpose:
    case Expose:                                // paint event
        widget->translatePaintEvent(event);
        break;

    case ConfigureNotify:                        // window move/resize event
        if (event->xconfigure.event == event->xconfigure.window)
            widget->translateConfigEvent(event);
        break;

    case XFocusIn: {                                // got focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (d->inPopupMode()) // some delayed focus event to ignore
            break;
        if (!widget->isWindow())
            break;
        if (event->xfocus.detail != NotifyAncestor &&
            event->xfocus.detail != NotifyInferior &&
            event->xfocus.detail != NotifyNonlinear)
            break;
        setActiveWindow(widget);
        if (X11->focus_model == QX11Data::FM_PointerRoot) {
            // We got real input focus from somewhere, but we were in PointerRoot
            // mode, so we don't trust this event.  Check the focus model to make
            // sure we know what focus mode we are using...
            qt_check_focus_model();
        }
    }
        break;

    case XFocusOut:                                // lost focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (!widget->isWindow())
            break;
        if (event->xfocus.mode == NotifyGrab) {
            qt_xfocusout_grab_counter++;
            break;
        }
        if (event->xfocus.detail != NotifyAncestor &&
            event->xfocus.detail != NotifyNonlinearVirtual &&
            event->xfocus.detail != NotifyNonlinear)
            break;
        if (!d->inPopupMode() && widget == QApplicationPrivate::active_window) {
            XEvent ev;
            bool focus_will_change = false;
            if (XCheckTypedEvent(X11->display, XFocusIn, &ev)) {
                // we're about to get an XFocusIn, if we know we will
                // get a new active window, we don't want to set the
                // active window to 0 now
                QWidget *w2 = QWidget::find(ev.xany.window);
                if (w2
                    && w2->windowType() != Qt::Desktop
                    && !d->inPopupMode() // some delayed focus event to ignore
                    && w2->isWindow()
                    && (ev.xfocus.detail == NotifyAncestor
                        || ev.xfocus.detail == NotifyInferior
                        || ev.xfocus.detail == NotifyNonlinear))
                    focus_will_change = true;

                XPutBackEvent(X11->display, &ev);
            }
            if (!focus_will_change)
                setActiveWindow(0);
        }
        break;

    case EnterNotify: {                        // enter window
        if (QWidget::mouseGrabber() && (!d->inPopupMode() || widget->window() != activePopupWidget()))
            break;
        if ((event->xcrossing.mode != NotifyNormal
             && event->xcrossing.mode != NotifyUngrab)
            || event->xcrossing.detail == NotifyVirtual
            || event->xcrossing.detail == NotifyNonlinearVirtual)
            break;
        if (event->xcrossing.focus &&
            !(widget->windowType() == Qt::Desktop) && !widget->isActiveWindow()) {
            if (X11->focus_model == QX11Data::FM_Unknown) // check focus model
                qt_check_focus_model();
            if (X11->focus_model == QX11Data::FM_PointerRoot) // PointerRoot mode
                setActiveWindow(widget);
        }

        if (qt_button_down && !d->inPopupMode())
            break;

        QWidget *alien = widget->childAt(widget->d_func()->mapFromWS(QPoint(event->xcrossing.x,
                                                                            event->xcrossing.y)));
        QWidget *enter = alien ? alien : widget;
        QWidget *leave = 0;
        if (qt_last_mouse_receiver && !qt_last_mouse_receiver->internalWinId())
            leave = qt_last_mouse_receiver;
        else
            leave = QWidget::find(curWin);

        // ### Alien: enter/leave might be wrong here with overlapping siblings
        // if the enter widget is native and stacked under a non-native widget.
        QApplicationPrivate::dispatchEnterLeave(enter, leave);
        curWin = widget->internalWinId();
        qt_last_mouse_receiver = enter;
        if (!d->inPopupMode() || widget->window() == activePopupWidget())
            widget->translateMouseEvent(event); //we don't get MotionNotify, emulate it
    }
        break;
    case LeaveNotify: {                        // leave window
        QWidget *mouseGrabber = QWidget::mouseGrabber();
        if (mouseGrabber && !d->inPopupMode())
            break;
        if (curWin && widget->internalWinId() != curWin)
            break;
        if ((event->xcrossing.mode != NotifyNormal
            && event->xcrossing.mode != NotifyUngrab)
            || event->xcrossing.detail == NotifyInferior)
            break;
        if (!(widget->windowType() == Qt::Desktop))
            widget->translateMouseEvent(event); //we don't get MotionNotify, emulate it

        QWidget* enter = 0;
        QPoint enterPoint;
        XEvent ev;
        while (XCheckMaskEvent(X11->display, EnterWindowMask | LeaveWindowMask , &ev)
               && !qt_x11EventFilter(&ev)) {
            QWidget* event_widget = QWidget::find(ev.xcrossing.window);
            if(event_widget && event_widget->x11Event(&ev))
                break;
            if (ev.type == LeaveNotify
                || (ev.xcrossing.mode != NotifyNormal
                    && ev.xcrossing.mode != NotifyUngrab)
                || ev.xcrossing.detail == NotifyVirtual
                || ev.xcrossing.detail == NotifyNonlinearVirtual)
                continue;
            enter = event_widget;
            if (enter)
                enterPoint = enter->d_func()->mapFromWS(QPoint(ev.xcrossing.x, ev.xcrossing.y));
            if (ev.xcrossing.focus &&
                enter && !(enter->windowType() == Qt::Desktop) && !enter->isActiveWindow()) {
                if (X11->focus_model == QX11Data::FM_Unknown) // check focus model
                    qt_check_focus_model();
                if (X11->focus_model == QX11Data::FM_PointerRoot) // PointerRoot mode
                    setActiveWindow(enter);
            }
            break;
        }

        if ((! enter || (enter->windowType() == Qt::Desktop)) &&
            event->xcrossing.focus && widget == QApplicationPrivate::active_window &&
            X11->focus_model == QX11Data::FM_PointerRoot // PointerRoot mode
            ) {
            setActiveWindow(0);
        }

        if (qt_button_down && !d->inPopupMode())
            break;

        if (!curWin)
            QApplicationPrivate::dispatchEnterLeave(widget, 0);

        if (enter) {
            QWidget *alienEnter = enter->childAt(enterPoint);
            if (alienEnter)
                enter = alienEnter;
        }

        QWidget *leave = qt_last_mouse_receiver ? qt_last_mouse_receiver : widget;
        QWidget *activePopupWidget = qApp->activePopupWidget();

        if (mouseGrabber && activePopupWidget && leave == activePopupWidget)
            enter = mouseGrabber;
        else if (enter != widget && mouseGrabber) {
            if (!widget->rect().contains(widget->d_func()->mapFromWS(QPoint(event->xcrossing.x,
                                                                            event->xcrossing.y))))
                break;
        }

        QApplicationPrivate::dispatchEnterLeave(enter, leave);
        qt_last_mouse_receiver = enter;

        if (enter && QApplicationPrivate::tryModalHelper(enter, 0)) {
            QWidget *nativeEnter = enter->internalWinId() ? enter : enter->nativeParentWidget();
            curWin = nativeEnter->internalWinId();
            static_cast<QETWidget *>(nativeEnter)->translateMouseEvent(&ev); //we don't get MotionNotify, emulate it
        } else {
            curWin = 0;
            qt_last_mouse_receiver = 0;
        }
    }
        break;

    case UnmapNotify:                                // window hidden
        if (widget->isWindow()) {
            Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
            widget->d_func()->topData()->waitingForMapNotify = 0;

            if (widget->windowType() != Qt::Popup && !widget->testAttribute(Qt::WA_DontShowOnScreen)) {
                widget->setAttribute(Qt::WA_Mapped, false);
                if (widget->isVisible()) {
                    widget->d_func()->topData()->spont_unmapped = 1;
                    QHideEvent e;
                    QApplication::sendSpontaneousEvent(widget, &e);
                    widget->d_func()->hideChildren(true);
                }
            }

            if (!widget->d_func()->topData()->validWMState && X11->deferred_map.removeAll(widget))
                widget->doDeferredMap();
        }
        break;

    case MapNotify:                                // window shown
        if (widget->isWindow()) {
            // if we got a MapNotify when we were not waiting for it, it most
            // likely means the user has already asked to hide the window before
            // it ever being shown, so we try to withdraw a window after sending
            // the QShowEvent.
            bool pendingHide = widget->testAttribute(Qt::WA_WState_ExplicitShowHide) && widget->testAttribute(Qt::WA_WState_Hidden);
            widget->d_func()->topData()->waitingForMapNotify = 0;

            if (widget->windowType() != Qt::Popup) {
                widget->setAttribute(Qt::WA_Mapped);
                if (widget->d_func()->topData()->spont_unmapped) {
                    widget->d_func()->topData()->spont_unmapped = 0;
                    widget->d_func()->showChildren(true);
                    QShowEvent e;
                    QApplication::sendSpontaneousEvent(widget, &e);

                    // show() must have been called on this widget in
                    // order to reach this point, but we could have
                    // cleared these 2 attributes in case something
                    // previously forced us into WithdrawnState
                    // (e.g. kdocker)
                    widget->setAttribute(Qt::WA_WState_ExplicitShowHide, true);
                    widget->setAttribute(Qt::WA_WState_Visible, true);
                }
            }
            if (pendingHide) // hide the window
                XWithdrawWindow(X11->display, widget->internalWinId(), widget->x11Info().screen());
        }
        break;

    case ClientMessage:                        // client message
        return x11ClientMessage(widget,event,False);

    case ReparentNotify: {                      // window manager reparents
        // compress old reparent events to self
        XEvent ev;
        while (XCheckTypedWindowEvent(X11->display,
                                      widget->effectiveWinId(),
                                      ReparentNotify,
                                      &ev)) {
            if (ev.xreparent.window != ev.xreparent.event) {
                XPutBackEvent(X11->display, &ev);
                break;
            }
        }
        if (widget->isWindow()) {
            QTLWExtra *topData = widget->d_func()->topData();

            // store the parent. Useful for many things, embedding for instance.
            topData->parentWinId = event->xreparent.parent;

            // the widget frame strut should also be invalidated
            widget->data->fstrut_dirty = 1;

            // work around broken window managers... if we get a
            // ReparentNotify before the MapNotify, we assume that
            // we're being managed by a reparenting window
            // manager.
            //
            // however, the WM_STATE property may not have been set
            // yet, but we are going to assume that it will
            // be... otherwise we could try to map again after getting
            // an UnmapNotify... which could then, in turn, trigger a
            // race in the window manager which causes the window to
            // disappear when it really should be hidden.
            if (topData->waitingForMapNotify && !topData->validWMState) {
                topData->waitingForMapNotify = 0;
                topData->validWMState = 1;
            }

            if (X11->focus_model != QX11Data::FM_Unknown) {
                // toplevel reparented...
                QWidget *newparent = QWidget::find(event->xreparent.parent);
                if (! newparent || (newparent->windowType() == Qt::Desktop)) {
                    // we don't know about the new parent (or we've been
                    // reparented to root), perhaps a window manager
                    // has been (re)started?  reset the focus model to unknown
                    X11->focus_model = QX11Data::FM_Unknown;
                }
            }
        }
        break;
    }
    case SelectionRequest: {
        XSelectionRequestEvent *req = &event->xselectionrequest;
        if (! req)
            break;

        if (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)) {
            X11->xdndHandleSelectionRequest(req);

        } else if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }
    case SelectionClear: {
        XSelectionClearEvent *req = &event->xselectionclear;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)))
            break;

        if (qt_clipboard && !X11->use_xfixes) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }

    case SelectionNotify: {
        XSelectionEvent *req = &event->xselection;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)))
            break;

        if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }
    case PropertyNotify:
        // some properties changed
        if (event->xproperty.window == QX11Info::appRootWindow(0)) {
            // root properties for the first screen
            if (!X11->use_xfixes && event->xproperty.atom == ATOM(_QT_CLIPBOARD_SENTINEL)) {
                if (qt_check_clipboard_sentinel()) {
                    emit clipboard()->changed(QClipboard::Clipboard);
                    emit clipboard()->dataChanged();
                }
            } else if (!X11->use_xfixes && event->xproperty.atom == ATOM(_QT_SELECTION_SENTINEL)) {
                if (qt_check_selection_sentinel()) {
                    emit clipboard()->changed(QClipboard::Selection);
                    emit clipboard()->selectionChanged();
                }
            } else if (QApplicationPrivate::obey_desktop_settings) {
                if (event->xproperty.atom == ATOM(RESOURCE_MANAGER))
                    qt_set_x11_resources();
                else if (event->xproperty.atom == ATOM(_QT_SETTINGS_TIMESTAMP))
                    qt_set_x11_resources();
            }
        }
        if (event->xproperty.window == QX11Info::appRootWindow()) {
            // root properties for the default screen
            if (event->xproperty.atom == ATOM(_QT_INPUT_ENCODING)) {
                qt_set_input_encoding();
            } else if (event->xproperty.atom == ATOM(_NET_SUPPORTED)) {
                qt_get_net_supported();
            } else if (event->xproperty.atom == ATOM(_NET_VIRTUAL_ROOTS)) {
                qt_get_net_virtual_roots();
            } else if (event->xproperty.atom == ATOM(_NET_WORKAREA)) {
                qt_desktopwidget_update_workarea();

                // emit the workAreaResized() signal
                QDesktopWidget *desktop = QApplication::desktop();
                int numScreens = desktop->numScreens();
                for (int i = 0; i < numScreens; ++i)
                    emit desktop->workAreaResized(i);
            }
        } else if (widget) {
            widget->translatePropertyEvent(event);
        }  else {
            return -1; // don't know this window
        }
        break;

    default:
        break;
    }

    return 0;
}

bool QApplication::x11EventFilter(XEvent *)
{
    return false;
}



/*****************************************************************************
  Modal widgets; Since Xlib has little support for this we roll our own
  modal widget mechanism.
  A modal widget without a parent becomes application-modal.
  A modal widget with a parent becomes modal to its parent and grandparents..

  QApplicationPrivate::enterModal()
        Enters modal state
        Arguments:
            QWidget *widget        A modal widget

  QApplicationPrivate::leaveModal()
        Leaves modal state for a widget
        Arguments:
            QWidget *widget        A modal widget
 *****************************************************************************/

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;

    QWidget *leave = qt_last_mouse_receiver;
    if (!leave)
        leave = QWidget::find((WId)curWin);
    QApplicationPrivate::dispatchEnterLeave(0, leave);
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
    curWin = 0;
    qt_last_mouse_receiver = 0;
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
            QPoint p(QCursor::pos());
            QWidget* w = QApplication::widgetAt(p.x(), p.y());
            QWidget *leave = qt_last_mouse_receiver;
            if (!leave)
                leave = QWidget::find((WId)curWin);
            if (QWidget *grabber = QWidget::mouseGrabber()) {
                w = grabber;
                if (leave == w)
                    leave = 0;
            }
            QApplicationPrivate::dispatchEnterLeave(w, leave); // send synthetic enter event
            curWin = w ? w->effectiveWinId() : 0;
            qt_last_mouse_receiver = w;
        }
    }
    app_do_modal = qt_modal_stack != 0;
}

bool qt_try_modal(QWidget *widget, XEvent *event)
{
    if (qt_xdnd_dragging) {
        // allow mouse events while DnD is active
        switch (event->type) {
        case ButtonPress:
        case ButtonRelease:
        case MotionNotify:
            return true;
        default:
            break;
        }
    }

    // allow mouse release events to be sent to widgets that have been pressed
    if (event->type == ButtonRelease) {
        QWidget *alienWidget = widget->childAt(widget->mapFromGlobal(QPoint(event->xbutton.x_root,
                                                                            event->xbutton.y_root)));
        if (widget == qt_button_down || (alienWidget && alienWidget == qt_button_down))
            return true;
    }

    if (QApplicationPrivate::tryModalHelper(widget))
        return true;

    // disallow mouse/key events
    switch (event->type) {
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    case XKeyPress:
    case XKeyRelease:
    case EnterNotify:
    case LeaveNotify:
    case ClientMessage:
        return false;
    default:
        break;
    }

    return true;
}


/*****************************************************************************
  Popup widget mechanism

  openPopup()
        Adds a widget to the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be added

  closePopup()
        Removes a widget from the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be removed
 *****************************************************************************/


static int openPopupCount = 0;
void QApplicationPrivate::openPopup(QWidget *popup)
{
    Q_Q(QApplication);
    openPopupCount++;
    if (!QApplicationPrivate::popupWidgets) {                        // create list
        QApplicationPrivate::popupWidgets = new QWidgetList;
    }
    QApplicationPrivate::popupWidgets->append(popup);                // add to end of list
    Display *dpy = X11->display;
    if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard
        Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
        int r = XGrabKeyboard(dpy, popup->effectiveWinId(), false,
                              GrabModeAsync, GrabModeAsync, X11->time);
        if ((popupGrabOk = (r == GrabSuccess))) {
            r = XGrabPointer(dpy, popup->effectiveWinId(), true,
                             (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                              | EnterWindowMask | LeaveWindowMask | PointerMotionMask),
                             GrabModeAsync, GrabModeAsync, XNone, XNone, X11->time);
            if (!(popupGrabOk = (r == GrabSuccess))) {
                // transfer grab back to the keyboard grabber if any
                if (QWidgetPrivate::keyboardGrabber != 0)
                    QWidgetPrivate::keyboardGrabber->grabKeyboard();
                else
                    XUngrabKeyboard(dpy, X11->time);
            }
        }
    }

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q->sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!QApplicationPrivate::popupWidgets)
        return;
    QApplicationPrivate::popupWidgets->removeAll(popup);
    if (popup == qt_popup_down) {
        qt_button_down = 0;
        qt_popup_down = 0;
    }
    if (QApplicationPrivate::popupWidgets->count() == 0) {                // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;
        if (!qt_nograb() && popupGrabOk) {        // grabbing not disabled
            Display *dpy = X11->display;
            if (popup->geometry().contains(QPoint(mouseGlobalXPos, mouseGlobalYPos))
                || popup->testAttribute(Qt::WA_NoMouseReplay)) {
                // mouse release event or inside
                replayPopupMouseEvent = false;
            } else {                                // mouse press event
                mouseButtonPressTime -= 10000;        // avoid double click
                replayPopupMouseEvent = true;
            }
            // transfer grab back to mouse grabber if any, otherwise release the grab
            if (QWidgetPrivate::mouseGrabber != 0)
                QWidgetPrivate::mouseGrabber->grabMouse();
            else
                XUngrabPointer(dpy, X11->time);

            // transfer grab back to keyboard grabber if any, otherwise release the grab
            if (QWidgetPrivate::keyboardGrabber != 0)
                QWidgetPrivate::keyboardGrabber->grabKeyboard();
            else
                XUngrabKeyboard(dpy, X11->time);

            XFlush(dpy);
        }
        if (QApplicationPrivate::active_window) {
            if (QWidget *fw = QApplicationPrivate::active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }
    } else {
        // popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        // regrab the keyboard and mouse in case 'popup' lost the grab
        if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard
            Display *dpy = X11->display;
            Q_ASSERT(aw->testAttribute(Qt::WA_WState_Created));
            int r = XGrabKeyboard(dpy, aw->effectiveWinId(), false,
                                  GrabModeAsync, GrabModeAsync, X11->time);
            if ((popupGrabOk = (r == GrabSuccess))) {
                r = XGrabPointer(dpy, aw->effectiveWinId(), true,
                                 (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                                  | EnterWindowMask | LeaveWindowMask | PointerMotionMask),
                                 GrabModeAsync, GrabModeAsync, XNone, XNone, X11->time);
                if (!(popupGrabOk = (r == GrabSuccess))) {
                    // transfer grab back to keyboard grabber
                    if (QWidgetPrivate::keyboardGrabber != 0)
                        QWidgetPrivate::keyboardGrabber->grabKeyboard();
                    else
                        XUngrabKeyboard(dpy, X11->time);
                }
            }
        }
    }
}

/*****************************************************************************
  Event translation; translates X11 events to Qt events
 *****************************************************************************/

//
// Mouse event translation
//
// Xlib doesn't give mouse double click events, so we generate them by
// comparing window, time and position between two mouse press events.
//

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & Button1Mask)
        ret |= Qt::LeftButton;
    if (s & Button2Mask)
        ret |= Qt::MidButton;
    if (s & Button3Mask)
        ret |= Qt::RightButton;
    return ret;
}

Qt::KeyboardModifiers QX11Data::translateModifiers(int s)
{
    Qt::KeyboardModifiers ret = 0;
    if (s & ShiftMask)
        ret |= Qt::ShiftModifier;
    if (s & ControlMask)
        ret |= Qt::ControlModifier;
    if (s & qt_alt_mask)
        ret |= Qt::AltModifier;
    if (s & qt_meta_mask)
        ret |= Qt::MetaModifier;
    if (s & qt_mode_switch_mask)
        ret |= Qt::GroupSwitchModifier;
    return ret;
}

bool QETWidget::translateMouseEvent(const XEvent *event)
{
    if (!isWindow() && testAttribute(Qt::WA_NativeWindow))
        Q_ASSERT(internalWinId());

    Q_D(QWidget);
    QEvent::Type type;                                // event parameters
    QPoint pos;
    QPoint globalPos;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    XEvent nextEvent;

    if (qt_sm_blockUserInput) // block user interaction during session management
        return true;

    if (event->type == MotionNotify) { // mouse move
        if (event->xmotion.root != RootWindow(X11->display, x11Info().screen()) &&
            ! qt_xdnd_dragging)
            return false;

        XMotionEvent lastMotion = event->xmotion;
        while(XPending(X11->display))  { // compress mouse moves
            XNextEvent(X11->display, &nextEvent);
            if (nextEvent.type == ConfigureNotify
                || nextEvent.type == PropertyNotify
                || nextEvent.type == Expose
                || nextEvent.type == GraphicsExpose
                || nextEvent.type == NoExpose
                || nextEvent.type == KeymapNotify
                || ((nextEvent.type == EnterNotify || nextEvent.type == LeaveNotify)
                    && qt_button_down == this)
                || (nextEvent.type == ClientMessage
                    && (nextEvent.xclient.message_type == ATOM(_QT_SCROLL_DONE) ||
                    (nextEvent.xclient.message_type == ATOM(WM_PROTOCOLS) &&
                     (Atom)nextEvent.xclient.data.l[0] == ATOM(_NET_WM_SYNC_REQUEST))))) {
                // Pass the event through the event dispatcher filter so that applications
                // which install an event filter on the dispatcher get to handle it first.
                if (!QAbstractEventDispatcher::instance()->filterEvent(&nextEvent))
                    qApp->x11ProcessEvent(&nextEvent);
                continue;
            } else if (nextEvent.type != MotionNotify ||
                       nextEvent.xmotion.window != event->xmotion.window ||
                       nextEvent.xmotion.state != event->xmotion.state) {
                XPutBackEvent(X11->display, &nextEvent);
                break;
            }
            if (!qt_x11EventFilter(&nextEvent)
                && !x11Event(&nextEvent)) // send event through filter
                lastMotion = nextEvent.xmotion;
            else
                break;
        }
        type = QEvent::MouseMove;
        pos.rx() = lastMotion.x;
        pos.ry() = lastMotion.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = lastMotion.x_root;
        globalPos.ry() = lastMotion.y_root;
        buttons = translateMouseButtons(lastMotion.state);
        modifiers = X11->translateModifiers(lastMotion.state);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
    } else if (event->type == EnterNotify || event->type == LeaveNotify) {
        XEvent *xevent = (XEvent *)event;
        //unsigned int xstate = event->xcrossing.state;
        type = QEvent::MouseMove;
        pos.rx() = xevent->xcrossing.x;
        pos.ry() = xevent->xcrossing.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = xevent->xcrossing.x_root;
        globalPos.ry() = xevent->xcrossing.y_root;
        buttons = translateMouseButtons(xevent->xcrossing.state);
        modifiers = X11->translateModifiers(xevent->xcrossing.state);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
        if (qt_button_down)
            return true;
    } else {                                        // button press or release
        pos.rx() = event->xbutton.x;
        pos.ry() = event->xbutton.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = event->xbutton.x_root;
        globalPos.ry() = event->xbutton.y_root;
        buttons = translateMouseButtons(event->xbutton.state);
        modifiers = X11->translateModifiers(event->xbutton.state);
        switch (event->xbutton.button) {
        case Button1: button = Qt::LeftButton; break;
        case Button2: button = Qt::MidButton; break;
        case Button3: button = Qt::RightButton; break;
        case Button4:
        case Button5:
        case 6:
        case 7:
            // the fancy mouse wheel.

            // We are only interested in ButtonPress.
            if (event->type == ButtonPress){
                // compress wheel events (the X Server will simply
                // send a button press for each single notch,
                // regardless whether the application can catch up
                // or not)
                int delta = 1;
                XEvent xevent;
                while (XCheckTypedWindowEvent(X11->display, effectiveWinId(), ButtonPress, &xevent)){
                    if (xevent.xbutton.button != event->xbutton.button){
                        XPutBackEvent(X11->display, &xevent);
                        break;
                    }
                    delta++;
                }

                // the delta is defined as multiples of
                // WHEEL_DELTA, which is set to 120. Future wheels
                // may offer a finer-resolution. A positive delta
                // indicates forward rotation, a negative one
                // backward rotation respectively.
                int btn = event->xbutton.button;
                delta *= 120 * ((btn == Button4 || btn == 6) ? 1 : -1);
                bool hor = (((btn == Button4 || btn == Button5) && (modifiers & Qt::AltModifier)) ||
                            (btn == 6 || btn == 7));
                translateWheelEvent(globalPos.x(), globalPos.y(), delta, buttons,
                                    modifiers, (hor) ? Qt::Horizontal: Qt::Vertical);
            }
            return true;
        case 8: button = Qt::XButton1; break;
        case 9: button = Qt::XButton2; break;
        }
        if (event->type == ButtonPress) {        // mouse button pressed
            buttons |= button;
#if defined(Q_OS_IRIX) && !defined(QT_NO_TABLET)
            QTabletDeviceDataList *tablets = qt_tablet_devices();
            for (int i = 0; i < tablets->size(); ++i) {
                QTabletDeviceData &tab = tablets->operator[](i);
                XEvent myEv;
                if (XCheckTypedEvent(X11->display, tab.xinput_button_press, &myEv)) {
                        if (translateXinputEvent(&myEv, &tab)) {
                            //Spontaneous event sent.  Check if we need to continue.
                            if (qt_tabletChokeMouse) {
                                qt_tabletChokeMouse = false;
                                return false;
                            }
                        }
                }
            }
#endif
            if (!qt_button_down) {
                qt_button_down = childAt(pos);        //magic for masked widgets
                if (!qt_button_down)
                    qt_button_down = this;
            }
            if (mouseActWindow == event->xbutton.window &&
                mouseButtonPressed == button &&
                (long)event->xbutton.time -(long)mouseButtonPressTime
                < QApplication::doubleClickInterval() &&
                qAbs(event->xbutton.x - mouseXPos) < QT_GUI_DOUBLE_CLICK_RADIUS &&
                qAbs(event->xbutton.y - mouseYPos) < QT_GUI_DOUBLE_CLICK_RADIUS) {
                type = QEvent::MouseButtonDblClick;
                mouseButtonPressTime -= 2000;        // no double-click next time
            } else {
                type = QEvent::MouseButtonPress;
                mouseButtonPressTime = event->xbutton.time;
            }
            mouseButtonPressed = button;        // save event params for
            mouseXPos = event->xbutton.x;                // future double click tests
            mouseYPos = event->xbutton.y;
            mouseGlobalXPos = globalPos.x();
            mouseGlobalYPos = globalPos.y();
        } else {                                // mouse button released
            buttons &= ~button;
#if defined(Q_OS_IRIX) && !defined(QT_NO_TABLET)
            QTabletDeviceDataList *tablets = qt_tablet_devices();
            for (int i = 0; i < tablets->size(); ++i) {
                QTabletDeviceData &tab = tablets->operator[](i);
                XEvent myEv;
                if (XCheckTypedEvent(X11->display, tab.xinput_button_press, &myEv)) {
                        if (translateXinputEvent(&myEv, &tab)) {
                            //Spontaneous event sent.  Check if we need to continue.
                            if (qt_tabletChokeMouse) {
                                qt_tabletChokeMouse = false;
                                return false;
                            }
                        }
                }
            }
#endif
            type = QEvent::MouseButtonRelease;
        }
    }
    mouseActWindow = effectiveWinId();                        // save some event params
    mouseButtonState = buttons;
    if (type == 0)                                // don't send event
        return false;

    if (qApp->d_func()->inPopupMode()) {                        // in popup mode
        QWidget *activePopupWidget = qApp->activePopupWidget();
        QWidget *popup = qApp->activePopupWidget();
        if (popup != this) {
            if (event->type == LeaveNotify)
                return false;
            if ((windowType() == Qt::Popup) && rect().contains(pos) && 0)
                popup = this;
            else                                // send to last popup
                pos = popup->mapFromGlobal(globalPos);
        }
        bool releaseAfter = false;
        QWidget *popupChild  = popup->childAt(pos);

        if (popup != qt_popup_down){
            qt_button_down = 0;
            qt_popup_down = 0;
        }

        switch (type) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            qt_button_down = popupChild;
            qt_popup_down = popup;
            break;
        case QEvent::MouseButtonRelease:
            releaseAfter = true;
            break;
        default:
            break;                                // nothing for mouse move
        }

        int oldOpenPopupCount = openPopupCount;

        if (popup->isEnabled()) {
            // deliver event
            replayPopupMouseEvent = false;
            QWidget *receiver = popup;
            QPoint widgetPos = pos;
            if (qt_button_down)
                receiver = qt_button_down;
            else if (popupChild)
                receiver = popupChild;
            if (receiver != popup)
                widgetPos = receiver->mapFromGlobal(globalPos);
            QWidget *alien = childAt(mapFromGlobal(globalPos));
            QMouseEvent e(type, widgetPos, globalPos, button, buttons, modifiers);
            QApplicationPrivate::sendMouseEvent(receiver, &e, alien, this, &qt_button_down, qt_last_mouse_receiver);
        } else {
            // close disabled popups when a mouse button is pressed or released
            switch (type) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
                popup->close();
                break;
            default:
                break;
            }
        }

        if (qApp->activePopupWidget() != activePopupWidget
            && replayPopupMouseEvent) {
            // the active popup was closed, replay the mouse event
            if (!(windowType() == Qt::Popup)) {
#if 1
                qt_button_down = 0;
#else
                if (buttons == button)
                    qt_button_down = this;
                QMouseEvent e(type, mapFromGlobal(globalPos), globalPos, button,
                              buttons, modifiers);
                QApplication::sendSpontaneousEvent(this, &e);

                if (type == QEvent::MouseButtonPress
                    && button == Qt::RightButton
                    && (openPopupCount == oldOpenPopupCount)) {
                    QContextMenuEvent e(QContextMenuEvent::Mouse, mapFromGlobal(globalPos),
                                        globalPos, modifiers);
                    QApplication::sendSpontaneousEvent(this, &e);
                }
#endif
            }
            replayPopupMouseEvent = false;
        } else if (type == QEvent::MouseButtonPress
                   && button == Qt::RightButton
                   && (openPopupCount == oldOpenPopupCount)) {
            QWidget *popupEvent = popup;
            if (qt_button_down)
                popupEvent = qt_button_down;
            else if(popupChild)
                popupEvent = popupChild;
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos, modifiers);
            QApplication::sendSpontaneousEvent(popupEvent, &e);
        }

        if (releaseAfter) {
            qt_button_down = 0;
            qt_popup_down = 0;
        }
    } else {
        QWidget *alienWidget = childAt(pos);
        QWidget *widget = QApplicationPrivate::pickMouseReceiver(this, globalPos, pos, type, buttons,
                                                                 qt_button_down, alienWidget);
        if (!widget) {
            if (type == QEvent::MouseButtonRelease)
                QApplicationPrivate::mouse_buttons &= ~button;
            return false; // don't send event
        }

        int oldOpenPopupCount = openPopupCount;
        QMouseEvent e(type, pos, globalPos, button, buttons, modifiers);
        QApplicationPrivate::sendMouseEvent(widget, &e, alienWidget, this, &qt_button_down,
                                            qt_last_mouse_receiver);
        if (type == QEvent::MouseButtonPress
            && button == Qt::RightButton
            && (openPopupCount == oldOpenPopupCount)) {
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos, modifiers);
            QApplication::sendSpontaneousEvent(widget, &e);
        }
    }
    return true;
}


//
// Wheel event translation
//
bool QETWidget::translateWheelEvent(int global_x, int global_y, int delta,
                                    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                                    Qt::Orientation orient)
{
    const QPoint globalPos = QPoint(global_x, global_y);
    QPoint pos = mapFromGlobal(globalPos);
    QWidget *widget = childAt(pos);
    if (!widget)
        widget = this;
    else if (!widget->internalWinId())
        pos = widget->mapFromGlobal(globalPos);

#ifdef ALIEN_DEBUG
        qDebug() << "QETWidget::translateWheelEvent: receiver:" << widget << "pos:" << pos;
#endif

    // send the event to the widget or its ancestors
    {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && window() != popup)
            popup->close();
#ifndef QT_NO_WHEELEVENT
        QWheelEvent e(pos, globalPos, delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(widget, &e))
#endif
            return true;
    }

    // send the event to the widget that has the focus or its ancestors, if different
    if (widget != qApp->focusWidget() && (widget = qApp->focusWidget())) {
        if (widget && !widget->internalWinId())
            pos = widget->mapFromGlobal(globalPos);
        QWidget* popup = qApp->activePopupWidget();
        if (popup && widget != popup)
            popup->hide();
#ifndef QT_NO_WHEELEVENT
        QWheelEvent e(pos, globalPos, delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(widget, &e))
#endif
            return true;
    }
    return false;
}


//
// XInput Translation Event
//
#if !defined (QT_NO_TABLET)

#if !defined (Q_OS_IRIX)
void fetchWacomToolId(int &deviceType, qint64 &serialId)
{
    if (ptrWacomConfigInit == 0) // we actually have the lib
        return;
    WACOMCONFIG *config = ptrWacomConfigInit(X11->display, 0);
    if (config == 0)
        return;
    WACOMDEVICE *device = ptrWacomConfigOpenDevice (config, wacomDeviceName()->constData());
    if (device == 0)
        return;
    unsigned keys[1];
    int serialInt;
    ptrWacomConfigGetRawParam (device, XWACOM_PARAM_TOOLSERIAL, &serialInt, 1, keys);
    serialId = serialInt;
    int toolId;
    ptrWacomConfigGetRawParam (device, XWACOM_PARAM_TOOLID, &toolId, 1, keys);
    switch(toolId) {
    case 0x007: /* Mouse 4D and 2D */
    case 0x017: /* Intuos3 2D Mouse */
    case 0x094:
    case 0x09c:
        deviceType = QTabletEvent::FourDMouse;
        break;
    case 0x096: /* Lens cursor */
    case 0x097: /* Intuos3 Lens cursor */
        deviceType = QTabletEvent::Puck;
        break;
    case 0x0fa:
    case 0x81b: /* Intuos3 Classic Pen Eraser */
    case 0x82a: /* Eraser */
    case 0x82b: /* Intuos3 Grip Pen Eraser */
    case 0x85a:
    case 0x91a:
    case 0x91b: /* Intuos3 Airbrush Eraser */
    case 0xd1a:
        deviceType = QTabletEvent::XFreeEraser;
        break;
    case 0x112:
    case 0x912:
    case 0x913: /* Intuos3 Airbrush */
    case 0xd12:
        deviceType = QTabletEvent::Airbrush;
        break;
    case 0x012:
    case 0x022:
    case 0x032:
    case 0x801: /* Intuos3 Inking pen */
    case 0x812: /* Inking pen */
    case 0x813: /* Intuos3 Classic Pen */
    case 0x822: /* Pen */
    case 0x823: /* Intuos3 Grip Pen */
    case 0x832: /* Stroke pen */
    case 0x842:
    case 0x852:
    case 0x885: /* Intuos3 Marker Pen */
    default: /* Unknown tool */
        deviceType = QTabletEvent::Stylus;
    }

    /* Close device and return */
    ptrWacomConfigCloseDevice (device);
    ptrWacomConfigTerm(config);
}
#endif

struct qt_tablet_motion_data
{
    bool filterByWidget;
    const QWidget *widget;
    const QWidget *etWidget;
    int tabletMotionType;
    bool error; // found a reason to stop searching
};

static Bool qt_mouseMotion_scanner(Display *, XEvent *event, XPointer arg)
{
    qt_tablet_motion_data *data = (qt_tablet_motion_data *) arg;
    if (data->error)
        return false;

    if (event->type == MotionNotify)
        return true;

    data->error = event->type != data->tabletMotionType; // we stop compression when another event gets in between.
    return false;
}

static Bool qt_tabletMotion_scanner(Display *, XEvent *event, XPointer arg)
{
    qt_tablet_motion_data *data = (qt_tablet_motion_data *) arg;
    if (data->error)
        return false;
    if (event->type == data->tabletMotionType) {
        const XDeviceMotionEvent *const motion = reinterpret_cast<const XDeviceMotionEvent*>(event);
        if (data->filterByWidget) {
            const QPoint curr(motion->x, motion->y);
            const QWidget *w = data->etWidget;
            const QWidget *const child = w->childAt(curr);
            if (child) {
                w = child;
            }
            if (w == data->widget)
                return true;
        } else {
            return true;
        }
    }

    data->error = event->type != MotionNotify; // we stop compression when another event gets in between.
    return false;
}

bool QETWidget::translateXinputEvent(const XEvent *ev, QTabletDeviceData *tablet)
{
#if defined (Q_OS_IRIX)
    // Wacom has put defines in their wacom.h file so it would be quite wise
    // to use them, need to think of a decent way of not using
    // it when it doesn't exist...
    XDeviceState *s;
    XInputClass *iClass;
    XValuatorState *vs;
    int j;
#endif

    Q_ASSERT(tablet != 0);

    QWidget *w = this;
    QPoint global,
        curr;
    QPointF hiRes;
    qreal pressure = 0;
    int xTilt = 0,
        yTilt = 0,
        z = 0;
    qreal tangentialPressure = 0;
    qreal rotation = 0;
    int deviceType = QTabletEvent::NoDevice;
    int pointerType = QTabletEvent::UnknownPointer;
    const XDeviceMotionEvent *motion = 0;
    XDeviceButtonEvent *button = 0;
    const XProximityNotifyEvent *proximity = 0;
    QEvent::Type t;
    Qt::KeyboardModifiers modifiers = 0;
#if !defined (Q_OS_IRIX)
    XID device_id;
#endif

    if (ev->type == tablet->xinput_motion) {
        motion = reinterpret_cast<const XDeviceMotionEvent*>(ev);
        t = QEvent::TabletMove;
        global = QPoint(motion->x_root, motion->y_root);
        curr = QPoint(motion->x, motion->y);
#if !defined (Q_OS_IRIX)
        device_id = motion->deviceid;
#endif
    } else if (ev->type == tablet->xinput_button_press || ev->type == tablet->xinput_button_release) {
        if (ev->type == tablet->xinput_button_press) {
            t = QEvent::TabletPress;
        } else {
            t = QEvent::TabletRelease;
        }
        button = (XDeviceButtonEvent*)ev;

        global = QPoint(button->x_root, button->y_root);
        curr = QPoint(button->x, button->y);
#if !defined (Q_OS_IRIX)
        device_id = button->deviceid;
#endif
    } else { // Proximity
        if (ev->type == tablet->xinput_proximity_in)
            t = QEvent::TabletEnterProximity;
        else
            t = QEvent::TabletLeaveProximity;
        proximity = (const XProximityNotifyEvent*)ev;
#if !defined (Q_OS_IRIX)
        device_id = proximity->deviceid;
#endif
    }

    qint64 uid = 0;
#if defined (Q_OS_IRIX)
    QRect screenArea = qApp->desktop()->screenGeometry(this);
    s = XQueryDeviceState(X11->display, static_cast<XDevice *>(tablet->device));
    if (!s)
        return false;
    iClass = s->data;
    for (j = 0; j < s->num_classes; j++) {
        if (iClass->c_class == ValuatorClass) {
            vs = reinterpret_cast<XValuatorState *>(iClass);
            // figure out what device we have, based on bitmasking...
            if (vs->valuators[WAC_TRANSDUCER_I]
                 & WAC_TRANSDUCER_PROX_MSK) {
                switch (vs->valuators[WAC_TRANSDUCER_I]
                         & WAC_TRANSDUCER_MSK) {
                case WAC_PUCK_ID:
                    pointerType = QTabletEvent::Puck;
                    break;
                case WAC_STYLUS_ID:
                    pointerType = QTabletEvent::Pen;
                    break;
                case WAC_ERASER_ID:
                    pointerType = QTabletEvent::Eraser;
                    break;
                }
                // Get a Unique Id for the device, Wacom gives us this ability
                uid = vs->valuators[WAC_TRANSDUCER_I] & WAC_TRANSDUCER_ID_MSK;
                uid = (uid << 24) | vs->valuators[WAC_SERIAL_NUM_I];
                switch (WAC_TRANSDUCER_I & 0x0F0600) {
                case 0x080200:
                    deviceType = QTabletEvent::Stylus;
                    break;
                case 0x090200:
                    deviceType = QTabletEvent::Airbrush;
                    break;
                case 0x000400:
                    deviceType = QTabletEvent::FourDMouse;
                    break;
                case 0x000600:
                    deviceType = QTabletEvent::Puck;
                    break;
                case 0x080400:
                    deviceType = QTabletEvent::RotationStylus;
                    break;
                }
            } else {
                pointerType = QTabletEvent::UnknownPointer;
                deviceType = QTabletEvent::NoDevice;
                uid = 0;
            }

            if (!proximity) {
                // apparently Wacom needs a cast for the +/- values to make sense
                xTilt = short(vs->valuators[WAC_XTILT_I]);
                yTilt = short(vs->valuators[WAC_YTILT_I]);
                pressure = vs->valuators[WAC_PRESSURE_I];
                if (deviceType == QTabletEvent::FourDMouse
                        || deviceType == QTabletEvent::RotationStylus) {
                    rotation = vs->valuators[WAC_ROTATION_I] / 64.0;
                    if (deviceType == QTabletEvent::FourDMouse)
                        z = vs->valuators[WAC_ZCOORD_I];
                } else if (deviceType == QTabletEvent::Airbrush) {
                    tangentialPressure = vs->valuators[WAC_TAN_PRESSURE_I]
                                            / qreal(tablet->maxTanPressure - tablet->minTanPressure);
                }

                hiRes = tablet->scaleCoord(vs->valuators[WAC_XCOORD_I], vs->valuators[WAC_YCOORD_I],
                                           screenArea.x(), screenArea.width(),
                                           screenArea.y(), screenArea.height());
            }
            break;
        }
        iClass = reinterpret_cast<XInputClass*>(reinterpret_cast<char*>(iClass) + iClass->length);
    }
    XFreeDeviceState(s);
#else
    QTabletDeviceDataList *tablet_list = qt_tablet_devices();
    for (int i = 0; i < tablet_list->size(); ++i) {
        const QTabletDeviceData &t = tablet_list->at(i);
        if (device_id == static_cast<XDevice *>(t.device)->device_id) {
            deviceType = t.deviceType;
            if (t.deviceType == QTabletEvent::XFreeEraser) {
                deviceType = QTabletEvent::Stylus;
                pointerType = QTabletEvent::Eraser;
            } else if (t.deviceType == QTabletEvent::Stylus) {
                pointerType = QTabletEvent::Pen;
            }
            break;
        }
    }

    fetchWacomToolId(deviceType, uid);

    QRect screenArea = qApp->desktop()->rect();
    if (motion) {
        xTilt = (short) motion->axis_data[3];
        yTilt = (short) motion->axis_data[4];
        rotation = ((short) motion->axis_data[5]) / 64.0;
        pressure = (short) motion->axis_data[2];
        modifiers = X11->translateModifiers(motion->state);
        hiRes = tablet->scaleCoord(motion->axis_data[0], motion->axis_data[1],
                                    screenArea.x(), screenArea.width(),
                                    screenArea.y(), screenArea.height());
    } else if (button) {
        xTilt = (short) button->axis_data[3];
        yTilt = (short) button->axis_data[4];
        rotation = ((short) button->axis_data[5]) / 64.0;
        pressure = (short) button->axis_data[2];
        modifiers = X11->translateModifiers(button->state);
        hiRes = tablet->scaleCoord(button->axis_data[0], button->axis_data[1],
                                    screenArea.x(), screenArea.width(),
                                    screenArea.y(), screenArea.height());
    } else if (proximity) {
        pressure = 0;
        modifiers = 0;
    }
    if (deviceType == QTabletEvent::Airbrush) {
        tangentialPressure = rotation;
        rotation = 0.;
    }
#endif

    if (tablet->widgetToGetPress) {
        w = tablet->widgetToGetPress;
    } else {
        QWidget *child = w->childAt(curr);
        if (child)
            w = child;
    }
    curr = w->mapFromGlobal(global);

    if (t == QEvent::TabletPress) {
        tablet->widgetToGetPress = w;
    } else if (t == QEvent::TabletRelease && tablet->widgetToGetPress) {
        w = tablet->widgetToGetPress;
        curr = w->mapFromGlobal(global);
        tablet->widgetToGetPress = 0;
    }

    QTabletEvent e(t, curr, global, hiRes,
                   deviceType, pointerType,
                   qreal(pressure / qreal(tablet->maxPressure - tablet->minPressure)),
                   xTilt, yTilt, tangentialPressure, rotation, z, modifiers, uid);
    if (proximity) {
        QApplication::sendSpontaneousEvent(qApp, &e);
    } else {
        QApplication::sendSpontaneousEvent(w, &e);
        const bool accepted = e.isAccepted();
        if (!accepted && ev->type == tablet->xinput_motion) {
            // If the widget does not accept tablet events, we drop the next ones from the event queue
            // for this widget so it is not overloaded with the numerous tablet events.
            qt_tablet_motion_data tabletMotionData;
            tabletMotionData.tabletMotionType = tablet->xinput_motion;
            tabletMotionData.widget = w;
            tabletMotionData.etWidget = this;
            // if nothing is pressed, the events are filtered by position
            tabletMotionData.filterByWidget = (tablet->widgetToGetPress == 0);

            bool reinsertMouseEvent = false;
            XEvent mouseMotionEvent;
            while (true) {
                // Find first mouse event since we expect them in pairs inside Qt
                tabletMotionData.error =false;
                if (XCheckIfEvent(X11->display, &mouseMotionEvent, &qt_mouseMotion_scanner, (XPointer) &tabletMotionData)) {
                    reinsertMouseEvent = true;
                } else {
                    break;
                }

                // Now discard any duplicate tablet events.
                tabletMotionData.error = false;
                XEvent dummy;
                while (XCheckIfEvent(X11->display, &dummy, &qt_tabletMotion_scanner, (XPointer) &tabletMotionData)) {
                    // just discard the event
                }
            }

            if (reinsertMouseEvent) {
                XPutBackEvent(X11->display, &mouseMotionEvent);
            }
        }
    }
    return true;
}
#endif

bool QETWidget::translatePropertyEvent(const XEvent *event)
{
    Q_D(QWidget);
    if (!isWindow()) return true;

    Atom ret;
    int format, e;
    unsigned char *data = 0;
    unsigned long nitems, after;

    if (event->xproperty.atom == ATOM(_KDE_NET_WM_FRAME_STRUT)) {
        this->data->fstrut_dirty = 1;

        if (event->xproperty.state == PropertyNewValue) {
            e = XGetWindowProperty(X11->display, event->xproperty.window, ATOM(_KDE_NET_WM_FRAME_STRUT),
                                   0, 4, // struts are 4 longs
                                   False, XA_CARDINAL, &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == XA_CARDINAL &&
                format == 32 && nitems == 4) {
                long *strut = (long *) data;
                d->topData()->frameStrut.setCoords(strut[0], strut[2], strut[1], strut[3]);
                this->data->fstrut_dirty = 0;
            }
        }
    } else if (event->xproperty.atom == ATOM(_NET_WM_STATE)) {
        bool max = false;
        bool full = false;
        Qt::WindowStates oldState = Qt::WindowStates(this->data->window_state);

        if (event->xproperty.state == PropertyNewValue) {
            // using length of 1024 should be safe for all current and
            // possible NET states...
            e = XGetWindowProperty(X11->display, event->xproperty.window, ATOM(_NET_WM_STATE), 0, 1024,
                                   False, XA_ATOM, &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == XA_ATOM && format == 32 && nitems > 0) {
                Atom *states = (Atom *) data;

                unsigned long i;
                uint maximized = 0;
                for (i = 0; i < nitems; i++) {
                    if (states[i] == ATOM(_NET_WM_STATE_MAXIMIZED_VERT))
                        maximized |= 1;
                    else if (states[i] == ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))
                        maximized |= 2;
                    else if (states[i] == ATOM(_NET_WM_STATE_FULLSCREEN))
                        full = true;
                }
                if (maximized == 3) {
                    // only set maximized if both horizontal and vertical properties are set
                    max = true;
                }
            }
        }

        bool send_event = false;

        if (X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_VERT))
            && X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ))) {
            if (max && !isMaximized()) {
                this->data->window_state = this->data->window_state | Qt::WindowMaximized;
                send_event = true;
            } else if (!max && isMaximized()) {
                this->data->window_state &= ~Qt::WindowMaximized;
                send_event = true;
            }
        }

        if (X11->isSupportedByWM(ATOM(_NET_WM_STATE_FULLSCREEN))) {
            if (full && !isFullScreen()) {
                this->data->window_state = this->data->window_state | Qt::WindowFullScreen;
                send_event = true;
            } else if (!full && isFullScreen()) {
                this->data->window_state &= ~Qt::WindowFullScreen;
                send_event = true;
            }
        }

        if (send_event) {
            QWindowStateChangeEvent e(oldState);
            QApplication::sendSpontaneousEvent(this, &e);
        }
    } else if (event->xproperty.atom == ATOM(WM_STATE)) {
        // the widget frame strut should also be invalidated
        this->data->fstrut_dirty = 1;

        if (event->xproperty.state == PropertyDelete) {
            // the window manager has removed the WM State property,
            // so it is now in the withdrawn state (ICCCM 4.1.3.1) and
            // we are free to reuse this window
            d->topData()->parentWinId = 0;
            d->topData()->validWMState = 0;
            // map the window if we were waiting for a transition to
            // withdrawn
            if (X11->deferred_map.removeAll(this)) {
                doDeferredMap();
            } else if (isVisible()
                       && !testAttribute(Qt::WA_Mapped)
                       && !testAttribute(Qt::WA_OutsideWSRange)) {
                // so that show() will work again. As stated in the
                // ICCCM section 4.1.4: "Only the client can effect a
                // transition into or out of the Withdrawn state.",
                // but apparently this particular window manager
                // doesn't seem to care
                setAttribute(Qt::WA_WState_ExplicitShowHide, false);
                setAttribute(Qt::WA_WState_Visible, false);
            }
        } else {
            // the window manager has changed the WM State property...
            // we are wanting to see if we are withdrawn so that we
            // can reuse this window...
            e = XGetWindowProperty(X11->display, internalWinId(), ATOM(WM_STATE), 0, 2, False,
                                   ATOM(WM_STATE), &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == ATOM(WM_STATE) && format == 32 && nitems > 0) {
                long *state = (long *) data;
                switch (state[0]) {
                case WithdrawnState:
                    // if we are in the withdrawn state, we are free
                    // to reuse this window provided we remove the
                    // WM_STATE property (ICCCM 4.1.3.1)
                    XDeleteProperty(X11->display, internalWinId(), ATOM(WM_STATE));

                    // set the parent id to zero, so that show() will
                    // work again
                    d->topData()->parentWinId = 0;
                    d->topData()->validWMState = 0;
                    // map the window if we were waiting for a
                    // transition to withdrawn
                    if (X11->deferred_map.removeAll(this)) {
                        doDeferredMap();
                    } else if (isVisible()
                               && !testAttribute(Qt::WA_Mapped)
                               && !testAttribute(Qt::WA_OutsideWSRange)) {
                        // so that show() will work again. As stated
                        // in the ICCCM section 4.1.4: "Only the
                        // client can effect a transition into or out
                        // of the Withdrawn state.", but apparently
                        // this particular window manager doesn't seem
                        // to care
                        setAttribute(Qt::WA_WState_ExplicitShowHide, false);
                        setAttribute(Qt::WA_WState_Visible, false);
                    }
                    break;

                case IconicState:
                    d->topData()->validWMState = 1;
                    if (!isMinimized()) {
                        // window was minimized
                        this->data->window_state = this->data->window_state | Qt::WindowMinimized;
                        QWindowStateChangeEvent e(Qt::WindowStates(this->data->window_state & ~Qt::WindowMinimized));
                        QApplication::sendSpontaneousEvent(this, &e);
                    }
                    break;

                default:
                    d->topData()->validWMState = 1;
                    if (isMinimized()) {
                        // window was un-minimized
                        this->data->window_state &= ~Qt::WindowMinimized;
                        QWindowStateChangeEvent e(Qt::WindowStates(this->data->window_state | Qt::WindowMinimized));
                        QApplication::sendSpontaneousEvent(this, &e);
                    }
                    break;
                }
            }
        }
    } else if (event->xproperty.atom == ATOM(_NET_WM_WINDOW_OPACITY)) {
        // the window opacity was changed
        if (event->xproperty.state == PropertyNewValue) {
            e = XGetWindowProperty(event->xclient.display,
                                   event->xclient.window,
                                   ATOM(_NET_WM_WINDOW_OPACITY),
                                   0, 1, False, XA_CARDINAL,
                                   &ret, &format, &nitems, &after, &data);

            if (e == Success && ret == XA_CARDINAL && format == 32 && nitems == 1
                && after == 0 && data) {
                ulong value = *(ulong*)(data);
                d->topData()->opacity = uint(value >> 24);
            }
        } else
            d->topData()->opacity = 255;
    }

    if (data)
        XFree(data);

    return true;
}


//
// Paint event translation
//
// When receiving many expose events, we compress them (union of all expose
// rectangles) into one event which is sent to the widget.

struct PaintEventInfo {
    Window window;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool isPaintOrScrollDoneEvent(Display *, XEvent *ev, XPointer a)
{
    PaintEventInfo *info = (PaintEventInfo *)a;
    if (ev->type == Expose || ev->type == GraphicsExpose
        || (ev->type == ClientMessage && ev->xclient.message_type == ATOM(_QT_SCROLL_DONE)))
    {
        if (ev->xexpose.window == info->window)
            return True;
    }
    return False;
}

#if defined(Q_C_CALLBACKS)
}
#endif



static
bool translateBySips(QWidget* that, QRect& paintRect)
{
    int dx=0, dy=0;
    int sips=0;
    for (int i = 0; i < X11->sip_list.size(); ++i) {
        const QX11Data::ScrollInProgress &sip = X11->sip_list.at(i);
        if (sip.scrolled_widget == that) {
            if (sips) {
                dx += sip.dx;
                dy += sip.dy;
            }
            sips++;
        }
    }
    if (sips > 1) {
        paintRect.translate(dx, dy);
        return true;
    }
    return false;
}

void QETWidget::translatePaintEvent(const XEvent *event)
{
    if (!isWindow() && testAttribute(Qt::WA_NativeWindow))
        Q_ASSERT(internalWinId());

    Q_D(QWidget);
    QRect  paintRect(event->xexpose.x, event->xexpose.y,
                     event->xexpose.width, event->xexpose.height);
    XEvent xevent;
    PaintEventInfo info;
    info.window = internalWinId();
    translateBySips(this, paintRect);
    paintRect = d->mapFromWS(paintRect);

    QRegion paintRegion = paintRect;

    // WARNING: this is O(number_of_events * number_of_matching_events)
    while (XCheckIfEvent(X11->display,&xevent,isPaintOrScrollDoneEvent,
                         (XPointer)&info) &&
           !qt_x11EventFilter(&xevent)  &&
           !x11Event(&xevent)) // send event through filter
    {
        if (xevent.type == Expose || xevent.type == GraphicsExpose) {
            QRect exposure(xevent.xexpose.x,
                           xevent.xexpose.y,
                           xevent.xexpose.width,
                           xevent.xexpose.height);
            translateBySips(this, exposure);
            exposure = d->mapFromWS(exposure);
            paintRegion |= exposure;
        } else {
            translateScrollDoneEvent(&xevent);
        }
    }

    if (!paintRegion.isEmpty() && !testAttribute(Qt::WA_WState_ConfigPending))
        d->syncBackingStore(paintRegion);
}

//
// Scroll-done event translation.
//

bool QETWidget::translateScrollDoneEvent(const XEvent *event)
{
    long id = event->xclient.data.l[0];

    // Remove any scroll-in-progress record for the given id.
    for (int i = 0; i < X11->sip_list.size(); ++i) {
        const QX11Data::ScrollInProgress &sip = X11->sip_list.at(i);
        if (sip.id == id) {
            X11->sip_list.removeAt(i);
            return true;
        }
    }

    return false;
}

//
// ConfigureNotify (window move and resize) event translation

bool QETWidget::translateConfigEvent(const XEvent *event)
{
    Q_ASSERT((!isWindow() && !testAttribute(Qt::WA_NativeWindow)) ? internalWinId() : true);

    Q_D(QWidget);
    bool wasResize = testAttribute(Qt::WA_WState_ConfigPending); // set in QWidget::setGeometry_sys()
    setAttribute(Qt::WA_WState_ConfigPending, false);

    if (testAttribute(Qt::WA_OutsideWSRange)) {
        // discard events for windows that have a geometry X can't handle
        XEvent xevent;
        while (XCheckTypedWindowEvent(X11->display,internalWinId(), ConfigureNotify,&xevent) &&
               !qt_x11EventFilter(&xevent)  &&
               !x11Event(&xevent)) // send event through filter
            ;
        return true;
    }

    const QSize oldSize = size();

    if (isWindow()) {
        QPoint newCPos(geometry().topLeft());
        QSize  newSize(event->xconfigure.width, event->xconfigure.height);

        bool trust = isVisible()
                     && (d->topData()->parentWinId == XNone ||
                         d->topData()->parentWinId == QX11Info::appRootWindow());
        bool isCPos = false;

        if (event->xconfigure.send_event || trust) {
            // if a ConfigureNotify comes from a real sendevent request, we can
            // trust its values.
            newCPos.rx() = event->xconfigure.x + event->xconfigure.border_width;
            newCPos.ry() = event->xconfigure.y + event->xconfigure.border_width;
            isCPos = true;
        }
        if (isVisible())
            QApplication::syncX();

        if (d->extra->compress_events) {
            // ConfigureNotify compression for faster opaque resizing
            XEvent otherEvent;
            while (XCheckTypedWindowEvent(X11->display, internalWinId(), ConfigureNotify,
                                          &otherEvent)) {
                if (qt_x11EventFilter(&otherEvent))
                    continue;

                if (x11Event(&otherEvent))
                    continue;

                if (otherEvent.xconfigure.event != otherEvent.xconfigure.window)
                    continue;

                newSize.setWidth(otherEvent.xconfigure.width);
                newSize.setHeight(otherEvent.xconfigure.height);

                if (otherEvent.xconfigure.send_event || trust) {
                    newCPos.rx() = otherEvent.xconfigure.x +
                                   otherEvent.xconfigure.border_width;
                    newCPos.ry() = otherEvent.xconfigure.y +
                                   otherEvent.xconfigure.border_width;
                    isCPos = true;
                }
            }
#ifndef QT_NO_XSYNC
            qt_sync_request_event_data sync_event;
            sync_event.window = internalWinId();
            for (XEvent ev;;) {
                if (!XCheckIfEvent(X11->display, &ev, &qt_sync_request_scanner, (XPointer)&sync_event))
                    break;
            }
#endif // QT_NO_XSYNC
        }

        if (!isCPos) {
            // we didn't get an updated position of the toplevel.
            // either we haven't moved or there is a bug in the window manager.
            // anyway, let's query the position to be certain.
            int x, y;
            Window child;
            XTranslateCoordinates(X11->display, internalWinId(),
                                  QApplication::desktop()->screen(d->xinfo.screen())->internalWinId(),
                                  0, 0, &x, &y, &child);
            newCPos.rx() = x;
            newCPos.ry() = y;
        }

        QRect cr (geometry());
        if (newCPos != cr.topLeft()) { // compare with cpos (exluding frame)
            QPoint oldPos = geometry().topLeft();
            cr.moveTopLeft(newCPos);
            data->crect = cr;
            if (isVisible()) {
                QMoveEvent e(newCPos, oldPos); // pos (including frame), not cpos
                QApplication::sendSpontaneousEvent(this, &e);
            } else {
                setAttribute(Qt::WA_PendingMoveEvent, true);
            }
        }
        if (newSize != cr.size()) { // size changed
            cr.setSize(newSize);
            data->crect = cr;

            uint old_state = data->window_state;
            if (!X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_VERT))
                && !X11->isSupportedByWM(ATOM(_NET_WM_STATE_MAXIMIZED_HORZ)))
                data->window_state &= ~Qt::WindowMaximized;
            if (!X11->isSupportedByWM(ATOM(_NET_WM_STATE_FULLSCREEN)))
                data->window_state &= ~Qt::WindowFullScreen;

            if (old_state != data->window_state) {
                QWindowStateChangeEvent e((Qt::WindowStates) old_state);
                QApplication::sendEvent(this, &e);
            }

            if (!isVisible())
                setAttribute(Qt::WA_PendingResizeEvent, true);
            wasResize = true;
        }

    } else {
        XEvent xevent;
        while (XCheckTypedWindowEvent(X11->display,internalWinId(), ConfigureNotify,&xevent) &&
               !qt_x11EventFilter(&xevent)  &&
               !x11Event(&xevent)) // send event through filter
            ;
    }

    if (wasResize) {
        if (isVisible() && data->crect.size() != oldSize) {
            Q_ASSERT(d->extra->topextra);
            QWidgetBackingStore *bs = d->extra->topextra->backingStore.data();
            const bool hasStaticContents = bs && bs->hasStaticContents();
            // If we have a backing store with static contents, we have to disable the top-level
            // resize optimization in order to get invalidated regions for resized widgets.
            // The optimization discards all invalidateBuffer() calls since we're going to
            // repaint everything anyways, but that's not the case with static contents.
            if (!hasStaticContents)
                d->extra->topextra->inTopLevelResize = true;
            QResizeEvent e(data->crect.size(), oldSize);
            QApplication::sendSpontaneousEvent(this, &e);
        }

        const bool waitingForMapNotify = d->extra->topextra && d->extra->topextra->waitingForMapNotify;
        if (!waitingForMapNotify) {
            if (d->paintOnScreen()) {
                QRegion updateRegion(rect());
                if (testAttribute(Qt::WA_StaticContents))
                    updateRegion -= QRect(0, 0, oldSize.width(), oldSize.height());
                d->syncBackingStore(updateRegion);
            } else {
                d->syncBackingStore();
            }
        }

        if (d->extra && d->extra->topextra)
            d->extra->topextra->inTopLevelResize = false;
    }
#ifndef QT_NO_XSYNC
    if (QTLWExtra *tlwExtra = d->maybeTopData()) {
        if (tlwExtra->newCounterValueLo != 0 || tlwExtra->newCounterValueHi != 0) {
            XSyncValue value;
            XSyncIntsToValue(&value,
                             tlwExtra->newCounterValueLo,
                             tlwExtra->newCounterValueHi);

            XSyncSetCounter(X11->display, tlwExtra->syncUpdateCounter, value);
            tlwExtra->newCounterValueHi = 0;
            tlwExtra->newCounterValueLo = 0;
        }
    }
#endif
    return true;
}

//
// Close window event translation.
//
bool QETWidget::translateCloseEvent(const XEvent *)
{
    Q_D(QWidget);
    return d->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}


void QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

void QApplication::setDoubleClickInterval(int ms)
{
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    return QApplicationPrivate::mouse_double_click_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    return QApplicationPrivate::keyboard_input_time;
}

#ifndef QT_NO_WHEELEVENT
void QApplication::setWheelScrollLines(int n)
{
    QApplicationPrivate::wheel_scroll_lines = n;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}
#endif

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    switch (effect) {
    case Qt::UI_AnimateMenu:
        if (enable) QApplicationPrivate::fade_menu = false;
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeMenu:
        if (enable)
            QApplicationPrivate::animate_menu = true;
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateTooltip:
        if (enable) QApplicationPrivate::fade_tooltip = false;
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_FadeTooltip:
        if (enable)
            QApplicationPrivate::animate_tooltip = true;
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16 || !QApplicationPrivate::animate_ui)
        return false;

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        return QApplicationPrivate::animate_ui;
    }
}

/*****************************************************************************
  Session management support
 *****************************************************************************/

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_INCLUDE_NAMESPACE
#include <X11/SM/SMlib.h>
QT_END_INCLUDE_NAMESPACE

class QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(QSessionManager* mgr, QString& id, QString& key)
        : QObjectPrivate(), sm(mgr), sessionId(id), sessionKey(key),
            restartHint(QSessionManager::RestartIfRunning), eventLoop(0) {}
    QSessionManager* sm;
    QStringList restartCommand;
    QStringList discardCommand;
    QString& sessionId;
    QString& sessionKey;
    QSessionManager::RestartHint restartHint;
    QEventLoop *eventLoop;
};

class QSmSocketReceiver : public QObject
{
    Q_OBJECT
public:
    QSmSocketReceiver(int socket)
        {
            QSocketNotifier* sn = new QSocketNotifier(socket, QSocketNotifier::Read, this);
            connect(sn, SIGNAL(activated(int)), this, SLOT(socketActivated(int)));
        }

public slots:
     void socketActivated(int);
};


static SmcConn smcConnection = 0;
static bool sm_interactionActive;
static bool sm_smActive;
static int sm_interactStyle;
static int sm_saveType;
static bool sm_cancel;
// static bool sm_waitingForPhase2;  ### never used?!?
static bool sm_waitingForInteraction;
static bool sm_isshutdown;
// static bool sm_shouldbefast;  ### never used?!?
static bool sm_phase2;
static bool sm_in_phase2;

static QSmSocketReceiver* sm_receiver = 0;

static void resetSmState();
static void sm_setProperty(const char* name, const char* type,
                            int num_vals, SmPropValue* vals);
static void sm_saveYourselfCallback(SmcConn smcConn, SmPointer clientData,
                                  int saveType, Bool shutdown , int interactStyle, Bool fast);
static void sm_saveYourselfPhase2Callback(SmcConn smcConn, SmPointer clientData) ;
static void sm_dieCallback(SmcConn smcConn, SmPointer clientData) ;
static void sm_shutdownCancelledCallback(SmcConn smcConn, SmPointer clientData);
static void sm_saveCompleteCallback(SmcConn smcConn, SmPointer clientData);
static void sm_interactCallback(SmcConn smcConn, SmPointer clientData);
static void sm_performSaveYourself(QSessionManagerPrivate*);

static void resetSmState()
{
//    sm_waitingForPhase2 = false; ### never used?!?
    sm_waitingForInteraction = false;
    sm_interactionActive = false;
    sm_interactStyle = SmInteractStyleNone;
    sm_smActive = false;
    qt_sm_blockUserInput = false;
    sm_isshutdown = false;
//    sm_shouldbefast = false; ### never used?!?
    sm_phase2 = false;
    sm_in_phase2 = false;
}


// theoretically it's possible to set several properties at once. For
// simplicity, however, we do just one property at a time
static void sm_setProperty(const char* name, const char* type,
                            int num_vals, SmPropValue* vals)
{
    if (num_vals) {
      SmProp prop;
      prop.name = (char*)name;
      prop.type = (char*)type;
      prop.num_vals = num_vals;
      prop.vals = vals;

      SmProp* props[1];
      props[0] = &prop;
      SmcSetProperties(smcConnection, 1, props);
    }
    else {
      char* names[1];
      names[0] = (char*) name;
      SmcDeleteProperties(smcConnection, 1, names);
    }
}

static void sm_setProperty(const QString& name, const QString& value)
{
    QByteArray v = value.toUtf8();
    SmPropValue prop;
    prop.length = v.length();
    prop.value = (SmPointer) v.constData();
    sm_setProperty(name.toLatin1().data(), SmARRAY8, 1, &prop);
}

static void sm_setProperty(const QString& name, const QStringList& value)
{
    SmPropValue *prop = new SmPropValue[value.count()];
    int count = 0;
    QList<QByteArray> vl;
    for (QStringList::ConstIterator it = value.begin(); it != value.end(); ++it) {
      prop[count].length = (*it).length();
      vl.append((*it).toUtf8());
      prop[count].value = (char*)vl.last().data();
      ++count;
    }
    sm_setProperty(name.toLatin1().data(), SmLISTofARRAY8, count, prop);
    delete [] prop;
}


// workaround for broken libsm, see below
struct QT_smcConn {
    unsigned int save_yourself_in_progress : 1;
    unsigned int shutdown_in_progress : 1;
};

static void sm_saveYourselfCallback(SmcConn smcConn, SmPointer clientData,
                                  int saveType, Bool shutdown , int interactStyle, Bool /*fast*/)
{
    if (smcConn != smcConnection)
        return;
    sm_cancel = false;
    sm_smActive = true;
    sm_isshutdown = shutdown;
    sm_saveType = saveType;
    sm_interactStyle = interactStyle;
//    sm_shouldbefast = fast; ### never used?!?

    // ugly workaround for broken libSM. libSM should do that _before_
    // actually invoking the callback in sm_process.c
    ((QT_smcConn*)smcConn)->save_yourself_in_progress = true;
    if (sm_isshutdown)
        ((QT_smcConn*)smcConn)->shutdown_in_progress = true;

    sm_performSaveYourself((QSessionManagerPrivate*) clientData);
    if (!sm_isshutdown) // we cannot expect a confirmation message in that case
        resetSmState();
}

static void sm_performSaveYourself(QSessionManagerPrivate* smd)
{
    if (sm_isshutdown)
        qt_sm_blockUserInput = true;

    QSessionManager* sm = smd->sm;

    // generate a new session key
    timeval tv;
    gettimeofday(&tv, 0);
    smd->sessionKey  = QString::number(qulonglong(tv.tv_sec)) + QLatin1Char('_') + QString::number(qulonglong(tv.tv_usec));

    QStringList arguments = qApp->arguments();
    QString argument0 = arguments.isEmpty() ? qApp->applicationFilePath() : arguments.at(0);

    // tell the session manager about our program in best POSIX style
    sm_setProperty(QString::fromLatin1(SmProgram), argument0);
    // tell the session manager about our user as well.
    struct passwd *entryPtr = 0;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    QVarLengthArray<char, 1024> buf(qMax<long>(sysconf(_SC_GETPW_R_SIZE_MAX), 1024L));
    struct passwd entry;
    while (getpwuid_r(geteuid(), &entry, buf.data(), buf.size(), &entryPtr) == ERANGE) {
        if (buf.size() >= 32768) {
            // too big already, fail
            static char badusername[] = "";
            entryPtr = &entry;
            entry.pw_name = badusername;
            break;
        }

        // retry with a bigger buffer
        buf.resize(buf.size() * 2);
    }
#else
    entryPtr = getpwuid(geteuid());
#endif
    if (entryPtr)
        sm_setProperty(QString::fromLatin1(SmUserID), QString::fromLatin1(entryPtr->pw_name));

    // generate a restart and discard command that makes sense
    QStringList restart;
    restart  << argument0 << QLatin1String("-session")
             << smd->sessionId + QLatin1Char('_') + smd->sessionKey;
    if (qstricmp(appName, QX11Info::appClass()) != 0)
        restart << QLatin1String("-name") << qAppName();
    sm->setRestartCommand(restart);
    QStringList discard;
    sm->setDiscardCommand(discard);

    switch (sm_saveType) {
    case SmSaveBoth:
        qApp->commitData(*sm);
        if (sm_isshutdown && sm_cancel)
            break; // we cancelled the shutdown, no need to save state
    // fall through
    case SmSaveLocal:
        qApp->saveState(*sm);
        break;
    case SmSaveGlobal:
        qApp->commitData(*sm);
        break;
    default:
        break;
    }

    if (sm_phase2 && !sm_in_phase2) {
        SmcRequestSaveYourselfPhase2(smcConnection, sm_saveYourselfPhase2Callback, (SmPointer*) smd);
        qt_sm_blockUserInput = false;
    }
    else {
        // close eventual interaction monitors and cancel the
        // shutdown, if required. Note that we can only cancel when
        // performing a shutdown, it does not work for checkpoints
        if (sm_interactionActive) {
            SmcInteractDone(smcConnection, sm_isshutdown && sm_cancel);
            sm_interactionActive = false;
        }
        else if (sm_cancel && sm_isshutdown) {
            if (sm->allowsErrorInteraction()) {
                SmcInteractDone(smcConnection, True);
                sm_interactionActive = false;
            }
        }

        // set restart and discard command in session manager
        sm_setProperty(QString::fromLatin1(SmRestartCommand), sm->restartCommand());
        sm_setProperty(QString::fromLatin1(SmDiscardCommand), sm->discardCommand());

        // set the restart hint
        SmPropValue prop;
        prop.length = sizeof(int);
        int value = sm->restartHint();
        prop.value = (SmPointer) &value;
        sm_setProperty(SmRestartStyleHint, SmCARD8, 1, &prop);

        // we are done
        SmcSaveYourselfDone(smcConnection, !sm_cancel);
    }
}

static void sm_dieCallback(SmcConn smcConn, SmPointer /* clientData */)
{
    if (smcConn != smcConnection)
        return;
    resetSmState();
    QEvent quitEvent(QEvent::Quit);
    QApplication::sendEvent(qApp, &quitEvent);
}

static void sm_shutdownCancelledCallback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    if (sm_waitingForInteraction)
        ((QSessionManagerPrivate *) clientData)->eventLoop->exit();
    resetSmState();
}

static void sm_saveCompleteCallback(SmcConn smcConn, SmPointer /*clientData */)
{
    if (smcConn != smcConnection)
        return;
    resetSmState();
}

static void sm_interactCallback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    if (sm_waitingForInteraction)
        ((QSessionManagerPrivate *) clientData)->eventLoop->exit();
}

static void sm_saveYourselfPhase2Callback(SmcConn smcConn, SmPointer clientData)
{
    if (smcConn != smcConnection)
        return;
    sm_in_phase2 = true;
    sm_performSaveYourself((QSessionManagerPrivate*) clientData);
}


void QSmSocketReceiver::socketActivated(int)
{
    IceProcessMessages(SmcGetIceConnection(smcConnection), 0, 0);
}


#undef Bool
QT_BEGIN_INCLUDE_NAMESPACE
#include "qapplication_x11.moc"
QT_END_INCLUDE_NAMESPACE

QSessionManager::QSessionManager(QApplication * app, QString &id, QString& key)
    : QObject(*new QSessionManagerPrivate(this, id, key), app)
{
    Q_D(QSessionManager);
    d->restartHint = RestartIfRunning;

    resetSmState();
    char cerror[256];
    char* myId = 0;
    QByteArray b_id = id.toLatin1();
    char* prevId = b_id.data();

    SmcCallbacks cb;
    cb.save_yourself.callback = sm_saveYourselfCallback;
    cb.save_yourself.client_data = (SmPointer) d;
    cb.die.callback = sm_dieCallback;
    cb.die.client_data = (SmPointer) d;
    cb.save_complete.callback = sm_saveCompleteCallback;
    cb.save_complete.client_data = (SmPointer) d;
    cb.shutdown_cancelled.callback = sm_shutdownCancelledCallback;
    cb.shutdown_cancelled.client_data = (SmPointer) d;

    // avoid showing a warning message below
    if (qgetenv("SESSION_MANAGER").isEmpty())
        return;

    smcConnection = SmcOpenConnection(0, 0, 1, 0,
                                       SmcSaveYourselfProcMask |
                                       SmcDieProcMask |
                                       SmcSaveCompleteProcMask |
                                       SmcShutdownCancelledProcMask,
                                       &cb,
                                       prevId,
                                       &myId,
                                       256, cerror);

    id = QString::fromLatin1(myId);
    ::free(myId); // it was allocated by C

    QString error = QString::fromLocal8Bit(cerror);
    if (!smcConnection) {
        qWarning("Qt: Session management error: %s", qPrintable(error));
    }
    else {
        sm_receiver = new QSmSocketReceiver(IceConnectionNumber(SmcGetIceConnection(smcConnection)));
    }
}

QSessionManager::~QSessionManager()
{
    if (smcConnection)
        SmcCloseConnection(smcConnection, 0, 0);
    smcConnection = 0;
    delete sm_receiver;
}

QString QSessionManager::sessionId() const
{
    Q_D(const QSessionManager);
    return d->sessionId;
}

QString QSessionManager::sessionKey() const
{
    Q_D(const QSessionManager);
    return d->sessionKey;
}


void* QSessionManager::handle() const
{
    return (void*) smcConnection;
}


bool QSessionManager::allowsInteraction()
{
    Q_D(QSessionManager);
    if (sm_interactionActive)
        return true;

    if (sm_waitingForInteraction)
        return false;

    if (sm_interactStyle == SmInteractStyleAny) {
        sm_waitingForInteraction =  SmcInteractRequest(smcConnection, SmDialogNormal,
                                                        sm_interactCallback, (SmPointer*) d);
    }
    if (sm_waitingForInteraction) {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec();
        d->eventLoop = 0;

        sm_waitingForInteraction = false;
        if (sm_smActive) { // not cancelled
            sm_interactionActive = true;
            qt_sm_blockUserInput = false;
            return true;
        }
    }
    return false;
}

bool QSessionManager::allowsErrorInteraction()
{
    Q_D(QSessionManager);
    if (sm_interactionActive)
        return true;

    if (sm_waitingForInteraction)
        return false;

    if (sm_interactStyle == SmInteractStyleAny || sm_interactStyle == SmInteractStyleErrors) {
        sm_waitingForInteraction =  SmcInteractRequest(smcConnection, SmDialogError,
                                                        sm_interactCallback, (SmPointer*) d);
    }
    if (sm_waitingForInteraction) {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec();
        d->eventLoop = 0;

        sm_waitingForInteraction = false;
        if (sm_smActive) { // not cancelled
            sm_interactionActive = true;
            qt_sm_blockUserInput = false;
            return true;
        }
    }
    return false;
}

void QSessionManager::release()
{
    if (sm_interactionActive) {
        SmcInteractDone(smcConnection, False);
        sm_interactionActive = false;
        if (sm_smActive && sm_isshutdown)
            qt_sm_blockUserInput = true;
    }
}

void QSessionManager::cancel()
{
    sm_cancel = true;
}

void QSessionManager::setRestartHint(QSessionManager::RestartHint hint)
{
    Q_D(QSessionManager);
    d->restartHint = hint;
}

QSessionManager::RestartHint QSessionManager::restartHint() const
{
    Q_D(const QSessionManager);
    return d->restartHint;
}

void QSessionManager::setRestartCommand(const QStringList& command)
{
    Q_D(QSessionManager);
    d->restartCommand = command;
}

QStringList QSessionManager::restartCommand() const
{
    Q_D(const QSessionManager);
    return d->restartCommand;
}

void QSessionManager::setDiscardCommand(const QStringList& command)
{
    Q_D(QSessionManager);
    d->discardCommand = command;
}

QStringList QSessionManager::discardCommand() const
{
    Q_D(const QSessionManager);
    return d->discardCommand;
}

void QSessionManager::setManagerProperty(const QString& name, const QString& value)
{
    sm_setProperty(name, value);
}

void QSessionManager::setManagerProperty(const QString& name, const QStringList& value)
{
    sm_setProperty(name, value);
}

bool QSessionManager::isPhase2() const
{
    return sm_in_phase2;
}

void QSessionManager::requestPhase2()
{
    sm_phase2 = true;
}

#endif // QT_NO_SESSIONMANAGER

#if defined(QT_RX71_MULTITOUCH)

static inline int testBit(const char *array, int bit)
{
    return (array[bit/8] & (1<<(bit%8)));
}

static int openRX71Device(const QByteArray &deviceName)
{
    int fd = open(deviceName, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        fd = -errno;
        return fd;
    }

    // fetch the event type mask and check that the device reports absolute coordinates
    char eventTypeMask[(EV_MAX + sizeof(char) - 1) * sizeof(char) + 1];
    memset(eventTypeMask, 0, sizeof(eventTypeMask));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(eventTypeMask)), eventTypeMask) < 0) {
        close(fd);
        return -1;
    }
    if (!testBit(eventTypeMask, EV_ABS)) {
        close(fd);
        return -1;
    }

    // make sure that we can get the absolute X and Y positions from the device
    char absMask[(ABS_MAX + sizeof(char) - 1) * sizeof(char) + 1];
    memset(absMask, 0, sizeof(absMask));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absMask)), absMask) < 0) {
        close(fd);
        return -1;
    }
    if (!testBit(absMask, ABS_X) || !testBit(absMask, ABS_Y)) {
        close(fd);
        return -1;
    }

    return fd;
}

void QApplicationPrivate::initializeMultitouch_sys()
{
    Q_Q(QApplication);

    QByteArray deviceName = QByteArray("/dev/input/event");
    int currentDeviceNumber = 0;
    for (;;) {
        int fd = openRX71Device(QByteArray(deviceName + QByteArray::number(currentDeviceNumber++)));
        if (fd == -ENOENT) {
            // no more devices
            break;
        }
        if (fd < 0) {
            // not a touch device
            continue;
        }

        struct input_absinfo abs_x, abs_y, abs_z;
        ioctl(fd, EVIOCGABS(ABS_X), &abs_x);
        ioctl(fd, EVIOCGABS(ABS_Y), &abs_y);
        ioctl(fd, EVIOCGABS(ABS_Z), &abs_z);

        int deviceNumber = allRX71TouchPoints.count();

        QSocketNotifier *socketNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, q);
        QObject::connect(socketNotifier, SIGNAL(activated(int)), q, SLOT(_q_readRX71MultiTouchEvents()));

        RX71TouchPointState touchPointState = {
            socketNotifier,
            QTouchEvent::TouchPoint(deviceNumber),

            abs_x.minimum, abs_x.maximum, q->desktop()->screenGeometry().width(),
            abs_y.minimum, abs_y.maximum, q->desktop()->screenGeometry().height(),
            abs_z.minimum, abs_z.maximum
        };
        allRX71TouchPoints.append(touchPointState);
    }

    hasRX71MultiTouch = allRX71TouchPoints.count() > 1;
    if (!hasRX71MultiTouch) {
        for (int i = 0; i < allRX71TouchPoints.count(); ++i) {
            QSocketNotifier *socketNotifier = allRX71TouchPoints.at(i).socketNotifier;
            close(socketNotifier->socket());
            delete socketNotifier;
        }
        allRX71TouchPoints.clear();
    }
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
    hasRX71MultiTouch = false;
    for (int i = 0; i < allRX71TouchPoints.count(); ++i) {
        QSocketNotifier *socketNotifier = allRX71TouchPoints.at(i).socketNotifier;
        close(socketNotifier->socket());
        delete socketNotifier;
    }
    allRX71TouchPoints.clear();
}

bool QApplicationPrivate::readRX71MultiTouchEvents(int deviceNumber)
{
    RX71TouchPointState &touchPointState = allRX71TouchPoints[deviceNumber];
    QSocketNotifier *socketNotifier = touchPointState.socketNotifier;
    int fd = socketNotifier->socket();

    QTouchEvent::TouchPoint &touchPoint = touchPointState.touchPoint;

    bool down = touchPoint.state() != Qt::TouchPointReleased;
    if (down)
        touchPoint.setState(Qt::TouchPointStationary);

    bool changed = false;
    for (;;) {
        struct input_event inputEvent;
        int bytesRead = read(fd, &inputEvent, sizeof(inputEvent));
        if (bytesRead <= 0)
            break;
        if (bytesRead != sizeof(inputEvent)) {
            qWarning("Qt: INTERNAL ERROR: short read in readRX71MultiTouchEvents()");
            return false;
        }

        switch (inputEvent.type) {
        case EV_SYN:
            changed = true;
            switch (touchPoint.state()) {
            case Qt::TouchPointPressed:
            case Qt::TouchPointReleased:
                // make sure we don't compress pressed and releases with any other events
                return changed;
            default:
                break;
            }
            continue;
        case EV_KEY:
        case EV_ABS:
            break;
        default:
            qWarning("Qt: WARNING: unknown event type %d on multitouch device", inputEvent.type);
            continue;
        }

        QPointF screenPos = touchPoint.screenPos();
        switch (inputEvent.code) {
        case BTN_TOUCH:
            if (!down && inputEvent.value != 0)
                touchPoint.setState(Qt::TouchPointPressed);
            else if (down && inputEvent.value == 0)
                touchPoint.setState(Qt::TouchPointReleased);
            break;
        case ABS_TOOL_WIDTH:
        case ABS_VOLUME:
        case ABS_PRESSURE:
            // ignore for now
            break;
        case ABS_X:
        {
            qreal newValue = ((qreal(inputEvent.value - touchPointState.minX)
                              / qreal(touchPointState.maxX - touchPointState.minX))
                              * touchPointState.scaleX);
            screenPos.rx() = newValue;
            touchPoint.setScreenPos(screenPos);
            break;
        }
        case ABS_Y:
        {
            qreal newValue = ((qreal(inputEvent.value - touchPointState.minY)
                              / qreal(touchPointState.maxY - touchPointState.minY))
                              * touchPointState.scaleY);
            screenPos.ry() = newValue;
            touchPoint.setScreenPos(screenPos);
            break;
        }
        case ABS_Z:
        {
            // map Z (signal strength) to pressure for now
            qreal newValue = (qreal(inputEvent.value - touchPointState.minZ)
                              / qreal(touchPointState.maxZ - touchPointState.minZ));
            touchPoint.setPressure(newValue);
            break;
        }
        default:
            qWarning("Qt: WARNING: unknown event code %d on multitouch device", inputEvent.code);
            continue;
        }
    }

    if (down && touchPoint.state() != Qt::TouchPointReleased)
        touchPoint.setState(changed ? Qt::TouchPointMoved : Qt::TouchPointStationary);

    return changed;
}

void QApplicationPrivate::_q_readRX71MultiTouchEvents()
{
    // read touch events from all devices
    bool changed = false;
    for (int i = 0; i < allRX71TouchPoints.count(); ++i)
        changed = readRX71MultiTouchEvents(i) || changed;
    if (!changed)
        return;

    QList<QTouchEvent::TouchPoint> touchPoints;
    for (int i = 0; i < allRX71TouchPoints.count(); ++i)
        touchPoints.append(allRX71TouchPoints.at(i).touchPoint);

    translateRawTouchEvent(0, QTouchEvent::TouchScreen, touchPoints);
}

#else // !QT_RX71_MULTITOUCH

void QApplicationPrivate::initializeMultitouch_sys()
{ }
void QApplicationPrivate::cleanupMultitouch_sys()
{ }

#endif // QT_RX71_MULTITOUCH

QT_END_NAMESPACE
