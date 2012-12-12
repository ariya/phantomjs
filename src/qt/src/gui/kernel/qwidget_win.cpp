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

#include "qapplication.h"
#include "qapplication_p.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qimage.h"
#include "qlayout.h"
#include "qpainter.h"
#include "qstack.h"
#include "qthread.h"
#include "qt_windows.h"
#include "qwidget.h"
#include "qwidget_p.h"
#include "private/qbackingstore_p.h"
#include "private/qwindowsurface_raster_p.h"

#include "qscrollbar.h"
#include "qabstractscrollarea.h"
#include <private/qabstractscrollarea_p.h>

#include <qdebug.h>

#include <private/qapplication_p.h>
#include <private/qwininputcontext_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qsystemlibrary_p.h>

#if defined(Q_WS_WINCE)
#include "qguifunctions_wince.h"
QT_USE_NAMESPACE
extern void qt_wince_maximize(QWidget *widget);                          //defined in qguifunctions_wince.cpp
extern void qt_wince_unmaximize(QWidget *widget);                        //defined in qguifunctions_wince.cpp
extern void qt_wince_minimize(HWND hwnd);                                //defined in qguifunctions_wince.cpp
extern void qt_wince_full_screen(HWND hwnd, bool fullScreen, UINT swpf); //defined in qguifunctions_wince.cpp
extern bool qt_wince_is_mobile();                                        //defined in qguifunctions_wince.cpp
#endif

typedef BOOL    (WINAPI *PtrSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
static PtrSetLayeredWindowAttributes ptrSetLayeredWindowAttributes = 0;

#ifndef QT_NO_DIRECTDRAW
#include <ddraw.h>
#include <private/qimage_p.h>
static IDirectDraw *qt_ddraw_object;
static IDirectDrawSurface *qt_ddraw_primary;
#endif



#if defined(QT_NON_COMMERCIAL)
#include "qnc_win.h"
#endif

#if !defined(WS_EX_TOOLWINDOW)
#define WS_EX_TOOLWINDOW 0x00000080
#endif

#if !defined(GWLP_WNDPROC)
#define GWLP_WNDPROC GWL_WNDPROC
#endif

//#define TABLET_DEBUG
#define PACKETDATA  (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE \
                     | PK_ORIENTATION | PK_CURSOR | PK_Z)
#define PACKETMODE  0
#include <wintab.h>
#include <pktdef.h>

QT_BEGIN_NAMESPACE

typedef HCTX        (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL        (API *PtrWTClose)(HCTX);
typedef UINT        (API *PtrWTInfo)(UINT, UINT, LPVOID);
typedef BOOL        (API *PtrWTEnable)(HCTX, BOOL);
typedef BOOL        (API *PtrWTOverlap)(HCTX, BOOL);
typedef int        (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
typedef BOOL        (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
typedef int     (API *PtrWTQueueSizeGet)(HCTX);
typedef BOOL    (API *PtrWTQueueSizeSet)(HCTX, int);

static PtrWTOpen ptrWTOpen = 0;
static PtrWTClose ptrWTClose = 0;
static PtrWTInfo ptrWTInfo = 0;
static PtrWTQueueSizeGet ptrWTQueueSizeGet = 0;
static PtrWTQueueSizeSet ptrWTQueueSizeSet = 0;
#ifndef QT_NO_TABLETEVENT
static void init_wintab_functions();
static void qt_tablet_init();
static void qt_tablet_cleanup();
#endif // QT_NO_TABLETEVENT
extern HCTX qt_tablet_context;
extern bool qt_tablet_tilt_support;

static QWidget *qt_tablet_widget = 0;
QWidget* qt_get_tablet_widget()
{
    return qt_tablet_widget;
}

extern bool qt_is_gui_used;

#ifndef QT_NO_TABLETEVENT
static void init_wintab_functions()
{
#if defined(Q_OS_WINCE)
    return;
#else
    if (!qt_is_gui_used)
        return;
    QSystemLibrary library(QLatin1String("wintab32"));
    ptrWTOpen = (PtrWTOpen)library.resolve("WTOpenW");
    ptrWTInfo = (PtrWTInfo)library.resolve("WTInfoW");
    ptrWTClose = (PtrWTClose)library.resolve("WTClose");
    ptrWTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve("WTQueueSizeGet");
    ptrWTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve("WTQueueSizeSet");
#endif // Q_OS_WINCE
}

static void qt_tablet_init()
{
    static bool firstTime = true;
    if (!firstTime)
        return;
    firstTime = false;
    qt_tablet_widget = new QWidget(0);
    qt_tablet_widget->createWinId();
    qt_tablet_widget->setObjectName(QLatin1String("Qt internal tablet widget"));
    // We don't need this internal widget to appear in QApplication::topLevelWidgets()
    if (QWidgetPrivate::allWidgets)
        QWidgetPrivate::allWidgets->remove(qt_tablet_widget);
    LOGCONTEXT lcMine;
    qAddPostRoutine(qt_tablet_cleanup);
    struct tagAXIS tpOri[3];
    init_wintab_functions();
    if (ptrWTInfo && ptrWTOpen && ptrWTQueueSizeGet && ptrWTQueueSizeSet) {
        // make sure we have WinTab
        if (!ptrWTInfo(0, 0, NULL)) {
#ifdef TABLET_DEBUG
            qWarning("QWidget: Wintab services not available");
#endif
            return;
        }

        // some tablets don't support tilt, check if it is possible,
        qt_tablet_tilt_support = ptrWTInfo(WTI_DEVICES, DVC_ORIENTATION, &tpOri);
        if (qt_tablet_tilt_support) {
            // check for azimuth and altitude
            qt_tablet_tilt_support = tpOri[0].axResolution && tpOri[1].axResolution;
        }
        // build our context from the default context
        ptrWTInfo(WTI_DEFSYSCTX, 0, &lcMine);
        // Go for the raw coordinates, the tablet event will return good stuff
        lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
        lcMine.lcPktData = PACKETDATA;
        lcMine.lcPktMode = PACKETMODE;
        lcMine.lcMoveMask = PACKETDATA;
        lcMine.lcOutOrgX = 0;
        lcMine.lcOutExtX = lcMine.lcInExtX;
        lcMine.lcOutOrgY = 0;
        lcMine.lcOutExtY = -lcMine.lcInExtY;
        qt_tablet_context = ptrWTOpen(qt_tablet_widget->winId(), &lcMine, true);
#ifdef TABLET_DEBUG
        qDebug("Tablet is %p", qt_tablet_context);
#endif
        if (!qt_tablet_context) {
#ifdef TABLET_DEBUG
            qWarning("QWidget: Failed to open the tablet");
#endif
            return;
        }
        // Set the size of the Packet Queue to the correct size...
        int currSize = ptrWTQueueSizeGet(qt_tablet_context);
        if (!ptrWTQueueSizeSet(qt_tablet_context, QT_TABLET_NPACKETQSIZE)) {
            // Ideally one might want to use a smaller
            // multiple, but for now, since we managed to destroy
            // the existing Q with the previous call, set it back
            // to the other size, which should work.  If not,
            // there will be trouble.
            if (!ptrWTQueueSizeSet(qt_tablet_context, currSize)) {
                Q_ASSERT_X(0, "Qt::Internal", "There is no packet queue for"
                         " the tablet. The tablet will not work");
            }
        }
    }
}

static void qt_tablet_cleanup()
{
    if (ptrWTClose)
        ptrWTClose(qt_tablet_context);
    delete qt_tablet_widget;
    qt_tablet_widget = 0;
}
#endif // QT_NO_TABLETEVENT

const QString qt_reg_winclass(QWidget *w);                // defined in qapplication_win.cpp

#ifndef QT_NO_DRAGANDDROP
void            qt_olednd_unregister(QWidget* widget, QOleDropTarget *dst); // dnd_win
QOleDropTarget* qt_olednd_register(QWidget* widget);
#endif

extern bool qt_nograb();
extern HRGN qt_win_bitmapToRegion(const QBitmap& bitmap);

static QWidget *mouseGrb    = 0;
static QCursor *mouseGrbCur = 0;
static QWidget *keyboardGrb = 0;
static HHOOK   journalRec  = 0;

extern "C" LRESULT QT_WIN_CALLBACK QtWndProc(HWND, UINT, WPARAM, LPARAM);

#define XCOORD_MAX 16383
#define WRECT_MAX 16383

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

#ifndef Q_WS_WINCE
void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);
    static int sw = -1, sh = -1;

    Qt::WindowType type = q->windowType();
    Qt::WindowFlags flags = data.window_flags;

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet
                   || (flags & Qt::MSWindowsFixedSizeDialogHint));
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::Drawer);

    HINSTANCE appinst  = qWinAppInst();
    HWND parentw, destroyw = 0;
    WId id = 0;

    QString windowClassName = qt_reg_winclass(q);

    if (!window)                                // always initialize
        initializeWindow = true;

    if (popup)
        flags |= Qt::WindowStaysOnTopHint; // a popup stays on top

    if (sw < 0) {                                // get the (primary) screen size
        sw = GetSystemMetrics(SM_CXSCREEN);
        sh = GetSystemMetrics(SM_CYSCREEN);
    }

    if (desktop && !q->testAttribute(Qt::WA_DontShowOnScreen)) {                                // desktop widget
        popup = false;                                // force this flags off
        data.crect.setRect(GetSystemMetrics(76 /* SM_XVIRTUALSCREEN  */), GetSystemMetrics(77 /* SM_YVIRTUALSCREEN  */),
                           GetSystemMetrics(78 /* SM_CXVIRTUALSCREEN */), GetSystemMetrics(79 /* SM_CYVIRTUALSCREEN */));
    }

    parentw = q->parentWidget() ? q->parentWidget()->effectiveWinId() : 0;

    QString title;
    int style = WS_CHILD;
    int exsty = 0;

    if (window) {
        style = GetWindowLong(window, GWL_STYLE);
        if (!style)
            qErrnoWarning("QWidget::create: GetWindowLong failed");
        topLevel = false; // #### needed for some IE plugins??
    } else if (popup || (type == Qt::ToolTip) || (type == Qt::SplashScreen)) {
        style = WS_POPUP;
    } else if (topLevel && !desktop) {
        if (flags & Qt::FramelessWindowHint)
            style = WS_POPUP;                // no border
        else if (flags & Qt::WindowTitleHint)
            style = WS_OVERLAPPED;
        else
            style = 0;
    }
    if (!desktop) {
        // if (!testAttribute(Qt::WA_PaintUnclipped))
        // ### Commented out for now as it causes some problems, but
        // this should be correct anyway, so dig some more into this
#ifndef Q_FLATTEN_EXPOSE
        style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
#endif
        if (topLevel) {
            if ((type == Qt::Window || dialog || tool)) {
                if (!(flags & Qt::FramelessWindowHint)) {
                    style |= WS_POPUP;
                    if (!(flags & Qt::MSWindowsFixedSizeDialogHint))
                        style |= WS_THICKFRAME;
                    else
                        style |= WS_DLGFRAME;
                }
                if (flags & Qt::WindowTitleHint)
                    style |= WS_CAPTION;
                if (flags & Qt::WindowSystemMenuHint)
                    style |= WS_SYSMENU;
                if (flags & Qt::WindowMinimizeButtonHint)
                    style |= WS_MINIMIZEBOX;
                if (shouldShowMaximizeButton())
                    style |= WS_MAXIMIZEBOX;
                if (tool)
                    exsty |= WS_EX_TOOLWINDOW;
                if (flags & Qt::WindowContextHelpButtonHint)
                    exsty |= WS_EX_CONTEXTHELP;
            } else {
                 exsty |= WS_EX_TOOLWINDOW;
            }
        }
    }

    if (flags & Qt::WindowTitleHint) {
        title = q->isWindow() ? qAppName() : q->objectName();
    }

    // The Qt::WA_WState_Created flag is checked by translateConfigEvent() in
    // qapplication_win.cpp. We switch it off temporarily to avoid move
    // and resize events during creationt
    q->setAttribute(Qt::WA_WState_Created, false);

    if (window) {                                // override the old window
        if (destroyOldWindow)
            destroyw = data.winid;
        id = window;
        setWinId(window);
        LONG res = SetWindowLong(window, GWL_STYLE, style);
        if (!res)
            qErrnoWarning("QWidget::create: Failed to set window style");
#ifdef _WIN64
        res = SetWindowLongPtr( window, GWLP_WNDPROC, (LONG_PTR)QtWndProc );
#else
        res = SetWindowLong( window, GWL_WNDPROC, (LONG)QtWndProc );
#endif
        if (!res)
            qErrnoWarning("QWidget::create: Failed to set window procedure");
    } else if (desktop) {                        // desktop widget
        id = GetDesktopWindow();
//         QWidget *otherDesktop = QWidget::find(id);        // is there another desktop?
//         if (otherDesktop && otherDesktop->testWFlags(Qt::WPaintDesktop)) {
//             otherDesktop->d_func()->setWinId(0);        // remove id from widget mapper
//             d->setWinId(id);                     // make sure otherDesktop is
//             otherDesktop->d_func()->setWinId(id);       //   found first
//         } else {
            setWinId(id);
//         }
    } else if (topLevel) {                       // create top-level widget
        if (popup)
            parentw = 0;

        const bool wasMoved = q->testAttribute(Qt::WA_Moved);
        int x = wasMoved ? data.crect.left() : CW_USEDEFAULT;
        int y = wasMoved ? data.crect.top() : CW_USEDEFAULT;
        int w = CW_USEDEFAULT;
        int h = CW_USEDEFAULT;

        // Adjust for framestrut when needed
        RECT rect = {0,0,0,0};
        bool isVisibleOnScreen = !q->testAttribute(Qt::WA_DontShowOnScreen);
        if (isVisibleOnScreen && AdjustWindowRectEx(&rect, style & ~WS_OVERLAPPED, FALSE, exsty)) {
            QTLWExtra *td = maybeTopData();
            if (wasMoved && (td && !td->posFromMove)) {
                x = data.crect.x() + rect.left;
                y = data.crect.y() + rect.top;
            }

            if (q->testAttribute(Qt::WA_Resized)) {
                w = data.crect.width() + (rect.right - rect.left);
                h = data.crect.height() + (rect.bottom - rect.top);
            }
        }
        //update position & initial size of POPUP window
        if (isVisibleOnScreen && topLevel && initializeWindow && (style & WS_POPUP)) {
            if (!q->testAttribute(Qt::WA_Resized)) {
                w = sw/2;
                h = 4*sh/10;
                if (extra) {
                    int dx = rect.right - rect.left;
                    int dy = rect.bottom - rect.top;
                    w = qMin(w, extra->maxw + dx);
                    h = qMin(h, extra->maxh + dy);
                    w = qMax(w, extra->minw + dx);
                    h = qMax(h, extra->minh + dy);
                }
            }
            if (!wasMoved) {
                x = qMax(sw/2 - w/2, 0);
                y = qMax(sh/2 - h/2, 0);
            }
        }

        id = CreateWindowEx(exsty, reinterpret_cast<const wchar_t *>(windowClassName.utf16()),
                            reinterpret_cast<const wchar_t *>(title.utf16()), style,
                            x, y, w, h,
                            parentw, NULL, appinst, NULL);
        if (!id)
            qErrnoWarning("QWidget::create: Failed to create window");
        setWinId(id);
        if ((flags & Qt::WindowStaysOnTopHint) || (type == Qt::ToolTip)) {
            SetWindowPos(id, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            if (flags & Qt::WindowStaysOnBottomHint)
                qWarning() << "QWidget: Incompatible window flags: the window can't be on top and on bottom at the same time";
        } else if (flags & Qt::WindowStaysOnBottomHint)
            SetWindowPos(id, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        winUpdateIsOpaque();
    } else if (q->testAttribute(Qt::WA_NativeWindow) || paintOnScreen()) { // create child widget
        id = CreateWindowEx(exsty, reinterpret_cast<const wchar_t *>(windowClassName.utf16()),
                            reinterpret_cast<const wchar_t *>(title.utf16()), style,
                            data.crect.left(), data.crect.top(), data.crect.width(), data.crect.height(),
                            parentw, NULL, appinst, NULL);
        if (!id)
            qErrnoWarning("QWidget::create: Failed to create window");
        SetWindowPos(id, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        setWinId(id);
    }

    if (desktop) {
        q->setAttribute(Qt::WA_WState_Visible);
    } else if (topLevel && !q->testAttribute(Qt::WA_DontShowOnScreen)) {
        RECT  cr;
        GetClientRect(id, &cr);
        // one cannot trust cr.left and cr.top, use a correction POINT instead
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(id, &pt);

        if (data.crect.width() == 0 || data.crect.height() == 0) {
            data.crect = QRect(pt.x, pt.y, data.crect.width(), data.crect.height());
        } else {
            data.crect = QRect(QPoint(pt.x, pt.y),
                               QPoint(pt.x + cr.right - 1, pt.y + cr.bottom - 1));
        }

        if (data.fstrut_dirty) {
            // be nice to activeqt
            updateFrameStrut();
        }
    }

    if (topLevel) {
        if (data.window_flags & Qt::CustomizeWindowHint
            && data.window_flags & Qt::WindowTitleHint) {
            HMENU systemMenu = GetSystemMenu((HWND)q->internalWinId(), FALSE);
            if (data.window_flags & Qt::WindowCloseButtonHint)
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_ENABLED);
            else
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_GRAYED);
        }
    }

    q->setAttribute(Qt::WA_WState_Created);                // accept move/resize events
    hd = 0;                                        // no display context

    if (q->testAttribute(Qt::WA_AcceptTouchEvents))
        registerTouchWindow();

    if (window) {                                // got window from outside
        if (IsWindowVisible(window))
            q->setAttribute(Qt::WA_WState_Visible);
        else
            q->setAttribute(Qt::WA_WState_Visible, false);
    }

    if (extra && !extra->mask.isEmpty())
        setMask_sys(extra->mask);

#if defined(QT_NON_COMMERCIAL)
    QT_NC_WIDGET_CREATE
#endif

    if (q->hasFocus() && q->testAttribute(Qt::WA_InputMethodEnabled))
        q->inputContext()->setFocusWidget(q);

    if (destroyw) {
        DestroyWindow(destroyw);
    }

#ifndef QT_NO_TABLETEVENT
    if (q != qt_tablet_widget && QWidgetPrivate::mapper)
        qt_tablet_init();
#endif // QT_NO_TABLETEVENT

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        registerDropSite(true);

    if (maybeTopData() && maybeTopData()->opacity != 255)
        q->setWindowOpacity(maybeTopData()->opacity/255.);

    if (topLevel && (data.crect.width() == 0 || data.crect.height() == 0)) {
        q->setAttribute(Qt::WA_OutsideWSRange, true);
    }

    if (!topLevel && q->testAttribute(Qt::WA_NativeWindow) && q->testAttribute(Qt::WA_Mapped)) {
        Q_ASSERT(q->internalWinId());
        ShowWindow(q->internalWinId(), SW_SHOW);
    }
}

#endif //Q_WS_WINCE


void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);
    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();
    if (testAttribute(Qt::WA_WState_Created)) {
        setAttribute(Qt::WA_WState_Created, false);
        for(int i = 0; i < d->children.size(); ++i) { // destroy all widget children
            register QObject *obj = d->children.at(i);
            if (obj->isWidgetType())
                ((QWidget*)obj)->destroy(destroySubWindows,
                                         destroySubWindows);
        }
        if (mouseGrb == this)
            releaseMouse();
        if (keyboardGrb == this)
            releaseKeyboard();
        if (testAttribute(Qt::WA_ShowModal))                // just be sure we leave modal
            QApplicationPrivate::leaveModal(this);
        else if ((windowType() == Qt::Popup))
            qApp->d_func()->closePopup(this);
        if (destroyWindow && !(windowType() == Qt::Desktop) && internalWinId()) {
            DestroyWindow(internalWinId());
        }
#ifdef Q_WS_WINCE
        if (destroyWindow && (windowType() == Qt::Desktop) && !GetDesktopWindow()) {
            DestroyWindow(internalWinId());
        }

#endif
        QT_TRY {
            d->setWinId(0);
        } QT_CATCH (const std::bad_alloc &) {
            // swallow - destructors must not throw
        }
    }
}

void QWidgetPrivate::reparentChildren()
{
    Q_Q(QWidget);
    QObjectList chlist = q->children();
    for(int i = 0; i < chlist.size(); ++i) { // reparent children
        QObject *obj = chlist.at(i);
        if (obj->isWidgetType()) {
            QWidget *w = (QWidget *)obj;
            if ((w->windowType() == Qt::Popup)) {
                ;
            } else if (w->isWindow()) {
                bool showIt = w->isVisible();
                QPoint old_pos = w->pos();
                w->setParent(q, w->windowFlags());
                w->move(old_pos);
                if (showIt)
                    w->show();
            } else {
                w->d_func()->invalidateBuffer(w->rect());
                SetParent(w->effectiveWinId(), q->effectiveWinId());
                w->d_func()->reparentChildren();
            }
        }
    }
}

void QWidgetPrivate::setParent_sys(QWidget *parent, Qt::WindowFlags f)
{
    Q_Q(QWidget);
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));

    WId old_winid = data.winid;
    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if (q->isVisible() && data.winid) {
        ShowWindow(data.winid, SW_HIDE);
        SetParent(data.winid, 0);
    }
    bool dropSiteWasRegistered = false;
    if (q->testAttribute(Qt::WA_DropSiteRegistered)) {
        dropSiteWasRegistered = true;
        q->setAttribute(Qt::WA_DropSiteRegistered, false); // ole dnd unregister (we will register again below)
    }

    if ((q->windowType() == Qt::Desktop))
        old_winid = 0;
    setWinId(0);

    QObjectPrivate::setParent_helper(parent);
    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    data.window_flags = f;
    data.fstrut_dirty = true;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
    // keep compatibility with previous versions, we need to preserve the created state
    // (but we recreate the winId for the widget being reparented, again for compatibility)
    if (wasCreated || (!q->isWindow() && parent->testAttribute(Qt::WA_WState_Created)))
        createWinId();
    if (q->isWindow() || (!parent || parent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    if (wasCreated) {
        reparentChildren();
    }

    if (extra && !extra->mask.isEmpty()) {
        QRegion r = extra->mask;
        extra->mask = QRegion();
        q->setMask(r);
    }
    if (extra && extra->topextra && !extra->topextra->caption.isEmpty()) {
        setWindowIcon_sys(true);
        setWindowTitle_helper(extra->topextra->caption);
    }
    if (old_winid)
        DestroyWindow(old_winid);

    if (q->testAttribute(Qt::WA_AcceptDrops) || dropSiteWasRegistered
        || (!q->isWindow() && q->parentWidget() && q->parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
        q->setAttribute(Qt::WA_DropSiteRegistered, true);

#ifdef Q_WS_WINCE
    // Show borderless toplevel windows in tasklist & NavBar
    if (!parent) {
        QString txt = q->windowTitle().isEmpty()?qAppName():q->windowTitle();
        SetWindowText(q->internalWinId(), (wchar_t*)txt.utf16());
    }
#endif
    invalidateBuffer(q->rect());
}


QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    QWidget *parentWindow = window();
    QWExtra *extra = parentWindow->d_func()->extra;
    if (!isVisible() || parentWindow->isMinimized() || !testAttribute(Qt::WA_WState_Created) || !internalWinId()
        || (extra
#ifndef QT_NO_GRAPHICSVIEW
            && extra->proxyWidget
#endif //QT_NO_GRAPHICSVIEW
            )) {
        if (extra && extra->topextra && extra->topextra->embedded) {
            QPoint pt = mapTo(parentWindow, pos);
            POINT p = {pt.x(), pt.y()};
            ClientToScreen(parentWindow->effectiveWinId(), &p);
            return QPoint(p.x, p.y);
        } else {
            QPoint toGlobal = mapTo(parentWindow, pos) + parentWindow->pos();
            // Adjust for window decorations
            toGlobal += parentWindow->geometry().topLeft() - parentWindow->frameGeometry().topLeft();
            return toGlobal;
        }
    }
    POINT p;
    QPoint tmp = d->mapToWS(pos);
    p.x = tmp.x();
    p.y = tmp.y();
    ClientToScreen(internalWinId(), &p);
    return QPoint(p.x, p.y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    QWidget *parentWindow = window();
    QWExtra *extra = parentWindow->d_func()->extra;
    if (!isVisible() || parentWindow->isMinimized() || !testAttribute(Qt::WA_WState_Created) || !internalWinId()
        || (extra
#ifndef QT_NO_GRAPHICSVIEW
            && extra->proxyWidget
#endif //QT_NO_GRAPHICSVIEW
            )) {
        if (extra && extra->topextra && extra->topextra->embedded) {
            POINT p = {pos.x(), pos.y()};
            ScreenToClient(parentWindow->effectiveWinId(), &p);
            return mapFrom(parentWindow, QPoint(p.x, p.y));
        } else {
            QPoint fromGlobal = mapFrom(parentWindow, pos - parentWindow->pos());
            // Adjust for window decorations
            fromGlobal -= parentWindow->geometry().topLeft() - parentWindow->frameGeometry().topLeft();
            return fromGlobal;
        }
    }
    POINT p;
    p.x = pos.x();
    p.y = pos.y();
    ScreenToClient(internalWinId(), &p);
    return d->mapFromWS(QPoint(p.x, p.y));
}

void QWidgetPrivate::updateSystemBackground() {}

#ifndef QT_NO_CURSOR
void QWidgetPrivate::setCursor_sys(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    Q_Q(QWidget);
    qt_win_set_cursor(q, false);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_win_set_cursor(q, false);
}
#endif

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    SetWindowText(q->internalWinId(), (wchar_t*)caption.utf16());
}

HICON qt_createIcon(QIcon icon, int xSize, int ySize, QPixmap **cache)
{
    HICON result = 0;
    if (!icon.isNull()) { // valid icon
        QSize size = icon.actualSize(QSize(xSize, ySize));
        QPixmap pm = icon.pixmap(size);
        if (pm.isNull())
            return 0;

        result = pm.toWinHICON();

        if (cache) {
            delete *cache;
            *cache = new QPixmap(pm);;
        }
    }
    return result;
}

void QWidgetPrivate::setWindowIcon_sys(bool forceReset)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->isWindow())
        return;
    QTLWExtra* x = topData();
    if (x->iconPixmap && !forceReset)
        // already been set
        return;

    if (x->winIconBig) {
        DestroyIcon(x->winIconBig);
        x->winIconBig = 0;
    }
    if (x->winIconSmall) {
        DestroyIcon(x->winIconSmall);
        x->winIconSmall = 0;
    }

    x->winIconSmall = qt_createIcon(q->windowIcon(),
                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                    &(x->iconPixmap));
    x->winIconBig = qt_createIcon(q->windowIcon(),
                                  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                                  &(x->iconPixmap));
    if (x->winIconBig) {
        SendMessage(q->internalWinId(), WM_SETICON, 0 /* ICON_SMALL */, (LPARAM)x->winIconSmall);
        SendMessage(q->internalWinId(), WM_SETICON, 1 /* ICON_BIG */, (LPARAM)x->winIconBig);
    } else {
        SendMessage(q->internalWinId(), WM_SETICON, 0 /* ICON_SMALL */, (LPARAM)x->winIconSmall);
        SendMessage(q->internalWinId(), WM_SETICON, 1 /* ICON_BIG */, (LPARAM)x->winIconSmall);
    }
}


void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_UNUSED(iconText);
}


QCursor *qt_grab_cursor()
{
    return mouseGrbCur;
}

// The procedure does nothing, but is required for mousegrabbing to work
#ifndef Q_WS_WINCE
LRESULT QT_WIN_CALLBACK qJournalRecordProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(journalRec, nCode, wParam, lParam);
}
#endif //Q_WS_WINCE

/* Works only as long as pointer is inside the application's window,
   which is good enough for QDockWidget.

   Doesn't call SetWindowsHookEx() - this function causes a system-wide
   freeze if any other app on the system installs a hook and fails to
   process events. */
void QWidgetPrivate::grabMouseWhileInWindow()
{
    Q_Q(QWidget);
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        SetCapture(q->effectiveWinId());
        mouseGrb = q;
#ifndef QT_NO_CURSOR
        mouseGrbCur = new QCursor(mouseGrb->cursor());
#endif
    }
}

#ifndef Q_WS_WINCE
void QWidget::grabMouse()
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        journalRec = SetWindowsHookEx(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandle(0), 0);
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(effectiveWinId());
        mouseGrb = this;
#ifndef QT_NO_CURSOR
        mouseGrbCur = new QCursor(mouseGrb->cursor());
#endif
    }
}

#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        journalRec = SetWindowsHookEx(WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandle(0), 0);
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(effectiveWinId());
        mouseGrbCur = new QCursor(cursor);
        SetCursor(mouseGrbCur->handle());
        mouseGrb = this;
    }
}
#endif

void QWidget::releaseMouse()
{
    if (!qt_nograb() && mouseGrb == this) {
        ReleaseCapture();
        if (journalRec) {
            UnhookWindowsHookEx(journalRec);
            journalRec = 0;
        }
        if (mouseGrbCur) {
            delete mouseGrbCur;
            mouseGrbCur = 0;
        }
        mouseGrb = 0;
    }
}
#endif

void QWidget::grabKeyboard()
{
    if (!qt_nograb()) {
        if (keyboardGrb)
            keyboardGrb->releaseKeyboard();
        keyboardGrb = this;
    }
}

void QWidget::releaseKeyboard()
{
    if (!qt_nograb() && keyboardGrb == this)
        keyboardGrb = 0;
}


QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

void QWidget::activateWindow()
{
    window()->createWinId();
    SetForegroundWindow(window()->internalWinId());
}

#ifndef Q_WS_WINCE
void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;

    int max = SW_MAXIMIZE;
    int min = SW_MINIMIZE;

    int normal = SW_SHOWNOACTIVATE;
    if (newstate & Qt::WindowActive) {
        max = SW_SHOWMAXIMIZED;
        min = SW_SHOWMINIMIZED;
        normal = SW_SHOWNORMAL;
    }

    if (isWindow()) {
        createWinId();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));

        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        if ((oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized)) {
            if (newstate & Qt::WindowMaximized && !(oldstate & Qt::WindowFullScreen))
                d->topData()->normalGeometry = geometry();
            if (isVisible() && !(newstate & Qt::WindowMinimized)) {
                ShowWindow(internalWinId(), (newstate & Qt::WindowMaximized) ? max : normal);
                if (!(newstate & Qt::WindowFullScreen)) {
                    QRect r = d->topData()->normalGeometry;
                    if (!(newstate & Qt::WindowMaximized) && r.width() >= 0) {
                        if (pos() != r.topLeft() || size() !=r.size()) {
                            d->topData()->normalGeometry = QRect(0,0,-1,-1);
                            setGeometry(r);
                        }
                    }
                } else {
                    d->updateFrameStrut();
                }
            }
        }

        if ((oldstate & Qt::WindowFullScreen) != (newstate & Qt::WindowFullScreen)) {
            if (newstate & Qt::WindowFullScreen) {
                if (d->topData()->normalGeometry.width() < 0 && !(oldstate & Qt::WindowMaximized))
                    d->topData()->normalGeometry = geometry();
                d->topData()->savedFlags = Qt::WindowFlags(GetWindowLong(internalWinId(), GWL_STYLE));
#ifndef Q_FLATTEN_EXPOSE
                UINT style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
#else
                UINT style = WS_POPUP;
#endif
		if (ulong(d->topData()->savedFlags) & WS_SYSMENU)
		    style |= WS_SYSMENU;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLong(internalWinId(), GWL_STYLE, style);
                QRect r = QApplication::desktop()->screenGeometry(this);
                UINT swpf = SWP_FRAMECHANGED;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;

                SetWindowPos(internalWinId(), HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
                d->updateFrameStrut();
            } else {
                UINT style = d->topData()->savedFlags;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLong(internalWinId(), GWL_STYLE, style);

                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;
                SetWindowPos(internalWinId(), 0, 0, 0, 0, 0, swpf);
                d->updateFrameStrut();

                // preserve maximized state
                if (isVisible())
                    ShowWindow(internalWinId(), (newstate & Qt::WindowMaximized) ? max : normal);

                if (!(newstate & Qt::WindowMaximized)) {
                    QRect r = d->topData()->normalGeometry;
                    d->topData()->normalGeometry = QRect(0,0,-1,-1);
                    if (r.isValid())
                        setGeometry(r);
                }
            }
        }

        if ((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (isVisible())
                ShowWindow(internalWinId(), (newstate & Qt::WindowMinimized) ? min :
                                    (newstate & Qt::WindowMaximized) ? max : normal);
        }
    }
    data->window_state = newstate;
    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}
#endif //Q_WS_WINCE


/*
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);
    deactivateWidgetCleanup();
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
#ifdef Q_WS_WINCE
    if (!qt_wince_is_mobile() && q->isFullScreen()) {
        HWND handle = FindWindow(L"HHTaskBar", L"");
        if (handle) {
            ShowWindow(handle, 1);
            EnableWindow(handle, true);
        }
    }
#endif
    if (q->windowFlags() != Qt::Desktop) {
        if ((q->windowFlags() & Qt::Popup) && q->internalWinId())
            ShowWindow(q->internalWinId(), SW_HIDE);
        else if (q->internalWinId())
            SetWindowPos(q->internalWinId(),0, 0,0,0,0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
    }
    if (q->isWindow()) {
        if (QWidgetBackingStore *bs = maybeBackingStore())
            bs->releaseBuffer();
    } else {
        invalidateBuffer(q->rect());
    }
    q->setAttribute(Qt::WA_Mapped, false);
}


/*
  \internal
  Platform-specific part of QWidget::show().
*/
#ifndef Q_WS_WINCE
void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);
#if defined(QT_NON_COMMERCIAL)
    QT_NC_SHOW_WINDOW
#endif
    if (q->testAttribute(Qt::WA_OutsideWSRange))
        return;
    q->setAttribute(Qt::WA_Mapped);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        return;
    }

    if (data.window_flags & Qt::Window) {
        QTLWExtra *extra = topData();
        if (!extra->hotkeyRegistered) {
            // Try to set the hotkey using information from STARTUPINFO
            STARTUPINFO startupInfo;
            GetStartupInfo(&startupInfo);
            // If STARTF_USEHOTKEY is set, hStdInput is the virtual keycode
            if (startupInfo.dwFlags & 0x00000200) {
                WPARAM hotKey = (WPARAM)startupInfo.hStdInput;
                SendMessage(data.winid, WM_SETHOTKEY, hotKey, 0);
            }
            extra->hotkeyRegistered = 1;
        }
    }

    int sm = SW_SHOWNORMAL;
    bool fakedMaximize = false;
    if (q->isWindow()) {
        if (q->isMinimized()) {
            sm = SW_SHOWMINIMIZED;
            if (!IsWindowVisible(q->internalWinId()))
                sm = SW_SHOWMINNOACTIVE;
        } else if (q->isMaximized()) {
            sm = SW_SHOWMAXIMIZED;
            // Windows will not behave correctly when we try to maximize a window which does not
            // have minimize nor maximize buttons in the window frame. Windows would then ignore
            // non-available geometry, and rather maximize the widget to the full screen, minus the
            // window frame (caption). So, we do a trick here, by adding a maximize button before
            // maximizing the widget, and then remove the maximize button afterwards.
            Qt::WindowFlags &flags = data.window_flags;
            if (flags & Qt::WindowTitleHint &&
                !(flags & (Qt::WindowMinMaxButtonsHint | Qt::FramelessWindowHint))) {
                fakedMaximize = TRUE;
                int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
                SetWindowLong(q->internalWinId(), GWL_STYLE, style | WS_MAXIMIZEBOX);
            }
        }
    }
    if (q->testAttribute(Qt::WA_ShowWithoutActivating)
        || (q->windowType() == Qt::Popup)
        || (q->windowType() == Qt::ToolTip)
        || (q->windowType() == Qt::Tool)) {
        sm = SW_SHOWNOACTIVATE;
    }


    if (q->internalWinId())
        ShowWindow(q->internalWinId(), sm);

    if (fakedMaximize) {
        int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
        SetWindowLong(q->internalWinId(), GWL_STYLE, style & ~WS_MAXIMIZEBOX);
        SetWindowPos(q->internalWinId(), 0, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED);
    }

    if (q->internalWinId()) {
        if (IsIconic(q->internalWinId()))
            data.window_state |= Qt::WindowMinimized;
        if (IsZoomed(q->internalWinId()))
            data.window_state |= Qt::WindowMaximized;
        // This is to resolve the problem where popups are opened from the
        // system tray and not being implicitly activated
        if (q->windowType() == Qt::Popup &&
            !q->parentWidget() && !qApp->activeWindow()) 
            q->activateWindow();
    }

    winSetupGestures();

    invalidateBuffer(q->rect());
}
#endif //Q_WS_WINCE

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created) && q->window()->windowType() != Qt::Popup)
        SetFocus(q->effectiveWinId());
}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        SetWindowPos(q->internalWinId(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        SetWindowPos(q->internalWinId(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    invalidateBuffer(q->rect());
}

void QWidgetPrivate::stackUnder_sys(QWidget* w)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId() && w->internalWinId())
        SetWindowPos(q->internalWinId(), w->internalWinId() , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    invalidateBuffer(q->rect());
}


/*
  Helper function for non-toplevel widgets. Helps to map Qt's 32bit
  coordinate system to Windpws's 16bit coordinate system.

  This code is duplicated from the X11 code, so any changes there
  should also (most likely) be reflected here.

  (In all comments below: s/X/Windows/g)
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
                if (q->internalWinId())
                    MoveWindow(q->internalWinId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), true);
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
            if (q->internalWinId())
                ShowWindow(q->internalWinId(), SW_HIDE);
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
    for (int i = 0; i < children.size(); ++i) {
        QObject *object = children.at(i);
        if (object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            if (!w->isWindow() && w->testAttribute(Qt::WA_WState_Created))
                w->d_func()->setWSGeometry();
        }
    }

    // move ourselves to the new position and map (if necessary) after
    // the movement. Rationale: moving unmapped windows is much faster
    // than moving mapped windows
    if (q->internalWinId()) {
        if (!parent->internalWinId())
            xrect.translate(parent->mapTo(q->nativeParentWidget(), QPoint(0, 0)));
        MoveWindow(q->internalWinId(), xrect.x(), xrect.y(), xrect.width(), xrect.height(), !jump);
    }
    if (mapWindow && !dontShow) {
        q->setAttribute(Qt::WA_Mapped);
        if (q->internalWinId())
            ShowWindow(q->internalWinId(), SW_SHOWNOACTIVATE);
    }

    if (jump && q->internalWinId())
        InvalidateRect(q->internalWinId(), 0, false);

}

//
// The internal qWinRequestConfig, defined in qapplication_win.cpp, stores move,
// resize and setGeometry requests for a widget that is already
// processing a config event. The purpose is to avoid recursion.
//
void qWinRequestConfig(WId, int, int, int, int, int);

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }
    if (q->isWindow())
        topData()->normalGeometry = QRect(0, 0, -1, -1);

    QSize  oldSize(q->size());
    QPoint oldPos(q->pos());

    if (!q->isWindow())
        isMove = (data.crect.topLeft() != QPoint(x, y));
    bool isResize = w != oldSize.width() || h != oldSize.height();

    if (!isMove && !isResize)
        return;

    if (isResize && !q->testAttribute(Qt::WA_StaticContents) && q->internalWinId())
        ValidateRgn(q->internalWinId(), 0);

#ifdef Q_WS_WINCE
    // On Windows CE we can't just fiddle around with the window state.
    // Too much magic in setWindowState.
    if (isResize && q->isMaximized())
        q->setWindowState(q->windowState() & ~Qt::WindowMaximized);
#else
    if (isResize)
        data.window_state &= ~Qt::WindowMaximized;
#endif

    if (data.window_state & Qt::WindowFullScreen) {
        QTLWExtra *top = topData();

        if (q->isWindow()) {
            // We need to update these flags when we remove the full screen state
            // or the frame will not be updated
            UINT style = top->savedFlags;
            if (q->isVisible())
                style |= WS_VISIBLE;
            SetWindowLong(q->internalWinId(), GWL_STYLE, style);

            UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
            if (data.window_state & Qt::WindowActive)
                swpf |= SWP_NOACTIVATE;
            SetWindowPos(q->internalWinId(), 0, 0, 0, 0, 0, swpf);
            updateFrameStrut();
        }
        data.window_state &= ~Qt::WindowFullScreen;
        topData()->savedFlags = 0;
    }

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    const bool inTopLevelResize = tlwExtra ? tlwExtra->inTopLevelResize : false;
    const bool isTranslucentWindow = !isOpaque && ptrUpdateLayeredWindowIndirect && (data.window_flags & Qt::FramelessWindowHint)
                                     && GetWindowLong(q->internalWinId(), GWL_EXSTYLE) & Q_WS_EX_LAYERED;

    if (q->testAttribute(Qt::WA_WState_ConfigPending)) {        // processing config event
        if (q->internalWinId())
            qWinRequestConfig(q->internalWinId(), isMove ? 2 : 1, x, y, w, h);
    } else {
        if (!q->testAttribute(Qt::WA_DontShowOnScreen))
            q->setAttribute(Qt::WA_WState_ConfigPending);
        if (q->windowType() == Qt::Desktop) {
            data.crect.setRect(x, y, w, h);
        } else if (q->isWindow()) {
            QRect fs(frameStrut());
            if (extra) {
                fs.setLeft(x - fs.left());
                fs.setTop(y - fs.top());
                fs.setRight((x + w - 1) + fs.right());
                fs.setBottom((y + h - 1) + fs.bottom());
            }
            if (w == 0 || h == 0) {
                q->setAttribute(Qt::WA_OutsideWSRange, true);
                if (q->isVisible() && q->testAttribute(Qt::WA_Mapped))
                    hide_sys();
                data.crect = QRect(x, y, w, h);
            } else if (q->isVisible() && q->testAttribute(Qt::WA_OutsideWSRange)) {
                q->setAttribute(Qt::WA_OutsideWSRange, false);

                // put the window in its place and show it
                MoveWindow(q->internalWinId(), fs.x(), fs.y(), fs.width(), fs.height(), true);
                RECT rect;
                if (!q->testAttribute(Qt::WA_DontShowOnScreen)) {
                    GetClientRect(q->internalWinId(), &rect);
                    data.crect.setRect(x, y, rect.right - rect.left, rect.bottom - rect.top);
                } else {
                    data.crect.setRect(x, y, w, h);
                }

                show_sys();
            } else if (!q->testAttribute(Qt::WA_DontShowOnScreen)) {
                q->setAttribute(Qt::WA_OutsideWSRange, false);
#ifndef Q_WS_WINCE
                // If the window is hidden and in maximized state or minimized, instead of moving the
                // window, set the normal position of the window.
                WINDOWPLACEMENT wndpl;
                GetWindowPlacement(q->internalWinId(), &wndpl);
                if ((wndpl.showCmd == SW_MAXIMIZE && !IsWindowVisible(q->internalWinId())) || wndpl.showCmd == SW_SHOWMINIMIZED) {
                    RECT normal = {fs.x(), fs.y(), fs.x()+fs.width(), fs.y()+fs.height()};
                    wndpl.rcNormalPosition = normal;
                    wndpl.showCmd = wndpl.showCmd == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_HIDE;
                    SetWindowPlacement(q->internalWinId(), &wndpl);
                } else {
#else
                if (data.window_state & Qt::WindowMaximized) {
                    qt_wince_maximize(q);
                } else {
#endif
                    MoveWindow(q->internalWinId(), fs.x(), fs.y(), fs.width(), fs.height(), true);
                }
                if (!q->isVisible())
                    InvalidateRect(q->internalWinId(), 0, FALSE);
                RECT rect;
                // If the layout has heightForWidth, the MoveWindow() above can
                // change the size/position, so refresh them.

                if (isTranslucentWindow) {
                    data.crect.setRect(x, y, w, h);
                } else {
                    GetClientRect(q->internalWinId(), &rect);
                    RECT rcNormalPosition ={0, 0, 0, 0};
                    // Use (0,0) as window position for embedded ActiveQt controls.
                    if (!tlwExtra || !tlwExtra->embedded)
                        GetWindowRect(q->internalWinId(), &rcNormalPosition);
                    QRect fStrut(frameStrut());
                    data.crect.setRect(rcNormalPosition.left + fStrut.left(),
                                       rcNormalPosition.top + fStrut.top(),
                                       rect.right - rect.left,
                                       rect.bottom - rect.top);
                    isResize = data.crect.size() != oldSize;
                }
            } else {
                q->setAttribute(Qt::WA_OutsideWSRange, false);
                data.crect.setRect(x, y, w, h);
            }
        } else {
            QRect oldGeom(data.crect);
            data.crect.setRect(x, y, w, h);
            if (q->isVisible() && (!inTopLevelResize || q->internalWinId())) {
                // Top-level resize optimization does not work for native child widgets;
                // disable it for this particular widget.
                if (inTopLevelResize)
                    tlwExtra->inTopLevelResize = false;

                if (!isResize)
                    moveRect(QRect(oldPos, oldSize), x - oldPos.x(), y - oldPos.y());
                else
                    invalidateBuffer_resizeHelper(oldPos, oldSize);

                if (inTopLevelResize)
                    tlwExtra->inTopLevelResize = true;
            }
            if (q->testAttribute(Qt::WA_WState_Created))
                setWSGeometry();
        }
        q->setAttribute(Qt::WA_WState_ConfigPending, false);
    }

    if (q->isWindow() && q->isVisible() && isResize && !inTopLevelResize) {
        invalidateBuffer(q->rect()); //after the resize
    }

    // Process events immediately rather than in translateConfigEvent to
    // avoid windows message process delay.
    if (q->isVisible()) {
        if (isMove && q->pos() != oldPos) {
            QMoveEvent e(q->pos(), oldPos);
            QApplication::sendEvent(q, &e);
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

bool QWidgetPrivate::shouldShowMaximizeButton()
{
    if (data.window_flags & Qt::MSWindowsFixedSizeDialogHint)
        return false;
    // if the user explicitly asked for the maximize button, we try to add
    // it even if the window has fixed size.
    if (data.window_flags & Qt::CustomizeWindowHint &&
        data.window_flags & Qt::WindowMaximizeButtonHint)
        return true;
    if (extra) {
        if ((extra->maxw && extra->maxw != QWIDGETSIZE_MAX && extra->maxw != QLAYOUTSIZE_MAX)
            || (extra->maxh && extra->maxh != QWIDGETSIZE_MAX && extra->maxh != QLAYOUTSIZE_MAX))
            return false;
    }
    return data.window_flags & Qt::WindowMaximizeButtonHint;
}

void QWidgetPrivate::winUpdateIsOpaque()
{
#ifndef Q_WS_WINCE
    Q_Q(QWidget);

    if (!q->isWindow() || !q->testAttribute(Qt::WA_TranslucentBackground))
        return;

    if ((data.window_flags & Qt::FramelessWindowHint) == 0)
        return;

    if (!isOpaque && ptrUpdateLayeredWindowIndirect) {
        SetWindowLong(q->internalWinId(), GWL_EXSTYLE,
            GetWindowLong(q->internalWinId(), GWL_EXSTYLE) | Q_WS_EX_LAYERED);
    } else {
        SetWindowLong(q->internalWinId(), GWL_EXSTYLE,
            GetWindowLong(q->internalWinId(), GWL_EXSTYLE) & ~Q_WS_EX_LAYERED);
    }
#endif
}

void QWidgetPrivate::setConstraints_sys()
{
#ifndef Q_WS_WINCE_WM
    Q_Q(QWidget);
    if (q->isWindow() && q->testAttribute(Qt::WA_WState_Created)) {
        int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
        if (shouldShowMaximizeButton())
            style |= WS_MAXIMIZEBOX;
        else
            style &= ~WS_MAXIMIZEBOX;
        SetWindowLong(q->internalWinId(), GWL_STYLE, style);
    }
#endif
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    Q_Q(QWidget);
    scrollChildren(dx, dy);

    if (!paintOnScreen()) {
        scrollRect(q->rect(), dx, dy);
    } else {
        UINT flags = SW_INVALIDATE;
        if (!q->testAttribute(Qt::WA_OpaquePaintEvent))
            flags |= SW_ERASE;
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        ScrollWindowEx(q->internalWinId(), dx, dy, 0, 0, 0, 0, flags);
        UpdateWindow(q->internalWinId());
    }
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    Q_Q(QWidget);

    if (!paintOnScreen()) {
        scrollRect(r, dx, dy);
    } else {
        RECT wr;
        wr.top = r.top();
        wr.left = r.left();
        wr.bottom = r.bottom()+1;
        wr.right = r.right()+1;

        UINT flags = SW_INVALIDATE;
        if (!q->testAttribute(Qt::WA_OpaquePaintEvent))
            flags |= SW_ERASE;
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        ScrollWindowEx(q->internalWinId(), dx, dy, &wr, &wr, 0, 0, flags);
        UpdateWindow(q->internalWinId());
    }
}

extern Q_GUI_EXPORT HDC qt_win_display_dc();

int QWidget::metric(PaintDeviceMetric m) const
{
    Q_D(const QWidget);
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else {
        bool ownDC = QThread::currentThread() != qApp->thread();
        HDC gdc = ownDC ? GetDC(0) : qt_win_display_dc();
        switch (m) {
        case PdmDpiX:
        case PdmPhysicalDpiX:
                if (d->extra && d->extra->customDpiX)
                    val = d->extra->customDpiX;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else
                    val = GetDeviceCaps(gdc, LOGPIXELSX);
            break;
        case PdmDpiY:
        case PdmPhysicalDpiY:
                if (d->extra && d->extra->customDpiY)
                    val = d->extra->customDpiY;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else
                    val = GetDeviceCaps(gdc, LOGPIXELSY);
            break;
        case PdmWidthMM:
            val = data->crect.width()
                    * GetDeviceCaps(gdc, HORZSIZE)
                    / GetDeviceCaps(gdc, HORZRES);
            break;
        case PdmHeightMM:
            val = data->crect.height()
                    * GetDeviceCaps(gdc, VERTSIZE)
                    / GetDeviceCaps(gdc, VERTRES);
            break;
        case PdmNumColors:
            if (GetDeviceCaps(gdc, RASTERCAPS) & RC_PALETTE)
                val = GetDeviceCaps(gdc, SIZEPALETTE);
            else {
                HDC hd = d->hd ? HDC(d->hd) : gdc;
                int bpp = GetDeviceCaps(hd, BITSPIXEL);
                if (bpp == 32)
                    val = INT_MAX; // ### this is bogus, it should be 2^24 colors for 32 bit as well
                else if(bpp<=8)
                    val = GetDeviceCaps(hd, NUMCOLORS);
                else
                    val = 1 << (bpp * GetDeviceCaps(hd, PLANES));
            }
            break;
        case PdmDepth:
            val = GetDeviceCaps(gdc, BITSPIXEL);
            break;
        default:
            val = 0;
            qWarning("QWidget::metric: Invalid metric command");
        }
        if (ownDC)
            ReleaseDC(0, gdc);
    }
    return val;
}

void QWidgetPrivate::createSysExtra()
{
#ifndef QT_NO_DRAGANDDROP
    extra->dropTarget = 0;
#endif
}

#ifndef Q_WS_WINCE
void QWidgetPrivate::deleteSysExtra()
{
}
#endif //Q_WS_WINCE

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->hotkeyRegistered = 0;
    extra->topextra->savedFlags = 0;
    extra->topextra->winIconBig = 0;
    extra->topextra->winIconSmall = 0;
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra->topextra->winIconSmall)
        DestroyIcon(extra->topextra->winIconSmall);
    if (extra->topextra->winIconBig)
        DestroyIcon(extra->topextra->winIconBig);
}

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
    // Enablement is defined by d->extra->dropTarget != 0.
    if (on) {
        // Turn on.
        createExtra();
#ifndef QT_NO_DRAGANDDROP
        if (!q->internalWinId())
            q->nativeParentWidget()->d_func()->createExtra();
        QWExtra *extra = extraData();
        if (!extra->dropTarget)
            extra->dropTarget = registerOleDnd(q);
#endif
    } else {
        // Turn off.
        QWExtra *extra = extraData();
#ifndef QT_NO_DRAGANDDROP
        if (extra && extra->dropTarget) {
            unregisterOleDnd(q, extra->dropTarget);
            extra->dropTarget = 0;
        }
#endif
    }
}

#ifndef QT_NO_DRAGANDDROP
QOleDropTarget* QWidgetPrivate::registerOleDnd(QWidget *widget)
{
    QOleDropTarget *dropTarget = new QOleDropTarget(widget);
    Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
    if (!widget->internalWinId()) {
        QWidget *nativeParent = widget->nativeParentWidget();
        Q_ASSERT(nativeParent);
        QWExtra *nativeExtra = nativeParent->d_func()->extra;
        Q_ASSERT(nativeExtra);
        if (!nativeExtra->oleDropWidgets.contains(widget))
            nativeExtra->oleDropWidgets.append(widget);
        if (!nativeExtra->dropTarget) {
            nativeExtra->dropTarget = registerOleDnd(nativeParent);
            Q_ASSERT(nativeExtra->dropTarget);
#ifndef Q_OS_WINCE
            CoLockObjectExternal(nativeExtra->dropTarget, false, true);
#endif
            RegisterDragDrop(nativeParent->internalWinId(), nativeExtra->dropTarget);
        }
    } else {
        RegisterDragDrop(widget->internalWinId(), dropTarget);
#ifndef Q_OS_WINCE
        CoLockObjectExternal(dropTarget, true, true);
#endif
    }
    return dropTarget;
}

void QWidgetPrivate::unregisterOleDnd(QWidget *widget, QOleDropTarget *dropTarget)
{
    dropTarget->releaseQt();
    dropTarget->Release();
    Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
    if (!widget->internalWinId()) {
        QWidget *nativeParent = widget->nativeParentWidget();
        while (nativeParent) {
            QWExtra *nativeExtra = nativeParent->d_func()->extra;
            if (!nativeExtra) {
                nativeParent = nativeParent->nativeParentWidget();
                continue;
            }

            const int removeCounter = nativeExtra->oleDropWidgets.removeAll(widget);
            nativeExtra->oleDropWidgets.removeAll(static_cast<QWidget *>(0));
            if (nativeExtra->oleDropWidgets.isEmpty() && nativeExtra->dropTarget
                && !nativeParent->testAttribute(Qt::WA_DropSiteRegistered)) {
#ifndef Q_OS_WINCE
                    CoLockObjectExternal(nativeExtra->dropTarget, false, true);
#endif
                    RevokeDragDrop(nativeParent->internalWinId());
                    nativeExtra->dropTarget = 0;
            }

            if (removeCounter)
                break;

            nativeParent = nativeParent->nativeParentWidget();
        }
    } else {
#ifndef Q_OS_WINCE
        CoLockObjectExternal(dropTarget, false, true);
#endif
        RevokeDragDrop(widget->internalWinId());
    }
}

#endif //QT_NO_DRAGANDDROP

// from qregion_win.cpp
extern HRGN qt_tryCreateRegion(QRegion::RegionType type, int left, int top, int right, int bottom);
void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;

    if (region.isEmpty()) {
        SetWindowRgn(q->internalWinId(), 0, true);
        return;
    }

    // Since SetWindowRegion takes ownership, and we need to translate,
    // we take a copy.
    HRGN wr = qt_tryCreateRegion(QRegion::Rectangle, 0,0,0,0);
    CombineRgn(wr, region.handle(), 0, RGN_COPY);

    QPoint offset = (q->isWindow()
                     ? frameStrut().topLeft()
                     : QPoint(0, 0));
    OffsetRgn(wr, offset.x(), offset.y());

    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (!SetWindowRgn(data.winid, wr, true))
        DeleteObject(wr);
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

    if (!q->internalWinId()) {
        data.fstrut_dirty = false;
        return;
    }

    RECT rect = {0,0,0,0};

    QTLWExtra *top = topData();
    uint exstyle = GetWindowLong(q->internalWinId(), GWL_EXSTYLE);
    uint style = GetWindowLong(q->internalWinId(), GWL_STYLE);
#ifndef Q_WS_WINCE
    if (AdjustWindowRectEx(&rect, style & ~(WS_OVERLAPPED), FALSE, exstyle)) {
#else
    if (AdjustWindowRectEx(&rect, style, FALSE, exstyle)) {
#endif
        top->frameStrut.setCoords(-rect.left, -rect.top, rect.right, rect.bottom);
        data.fstrut_dirty = false;
    }
}

#ifndef Q_WS_WINCE
void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    Q_Q(QWidget);

    if (!isOpaque && ptrUpdateLayeredWindow && (data.window_flags & Qt::FramelessWindowHint)) {
        if (GetWindowLong(q->internalWinId(), GWL_EXSTYLE) & Q_WS_EX_LAYERED) {
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, (int)(255.0 * level), AC_SRC_ALPHA};
            ptrUpdateLayeredWindow(q->internalWinId(), NULL, NULL, NULL, NULL, NULL, 0, &blend, Q_ULW_ALPHA);
        }
        return;
    }

    static bool function_resolved = false;
    if (!function_resolved) {
        ptrSetLayeredWindowAttributes =
            (PtrSetLayeredWindowAttributes) QSystemLibrary::resolve(QLatin1String("user32"),
                                                              "SetLayeredWindowAttributes");
        function_resolved = true;
    }

    if (!ptrSetLayeredWindowAttributes)
        return;

    int wl = GetWindowLong(q->internalWinId(), GWL_EXSTYLE);

    if (level != 1.0) {
        if ((wl&Q_WS_EX_LAYERED) == 0)
            SetWindowLong(q->internalWinId(), GWL_EXSTYLE, wl | Q_WS_EX_LAYERED);
    } else if (wl&Q_WS_EX_LAYERED) {
        SetWindowLong(q->internalWinId(), GWL_EXSTYLE, wl & ~Q_WS_EX_LAYERED);
    }
    ptrSetLayeredWindowAttributes(q->internalWinId(), 0, (int)(level * 255), Q_LWA_ALPHA);
}
#endif //Q_WS_WINCE

// class QGlobalRasterPaintEngine: public QRasterPaintEngine
// {
// public:
//     inline QGlobalRasterPaintEngine() : QRasterPaintEngine() { setFlushOnEnd(false); }
// };
// Q_GLOBAL_STATIC(QGlobalRasterPaintEngine, globalRasterPaintEngine)


#ifndef QT_NO_DIRECTDRAW
static uchar *qt_primary_surface_bits;
static int qt_primary_surface_stride;
static QImage::Format qt_primary_surface_format;

void qt_win_initialize_directdraw()
{
    HRESULT res;

    // Some initialization...
    if (!qt_ddraw_object) {
        res = DirectDrawCreate(0, &qt_ddraw_object, 0);

        if (res != DD_OK)
            qWarning("DirectDrawCreate failed: %d", res);

        qt_ddraw_object->SetCooperativeLevel(0, DDSCL_NORMAL);

        DDSURFACEDESC surfaceDesc;
        memset(&surfaceDesc, 0, sizeof(DDSURFACEDESC));

        surfaceDesc.dwSize = sizeof(DDSURFACEDESC);
        surfaceDesc.dwFlags = DDSD_CAPS;
        surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        res = qt_ddraw_object->CreateSurface(&surfaceDesc, &qt_ddraw_primary, 0);
        if (res != DD_OK)
            qWarning("CreateSurface failed: %d", res);

        memset(&surfaceDesc, 0, sizeof(DDSURFACEDESC));
        surfaceDesc.dwSize = sizeof(DDSURFACEDESC);
        res = qt_ddraw_primary->Lock(0, &surfaceDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, 0);
        if (res != DD_OK)
            qWarning("Locking surface failed: %d", res);

        if (surfaceDesc.ddpfPixelFormat.dwFlags == DDPF_RGB) {
            qt_primary_surface_bits = (uchar *) surfaceDesc.lpSurface;
            qt_primary_surface_stride = surfaceDesc.lPitch;
            qt_primary_surface_format = QImage::Format_RGB32;
        } else {
            qWarning("QWidget painting: unsupported device depth for onscreen painting...\n");
        }

        qt_ddraw_primary->Unlock(0);
    }
}

class QOnScreenRasterPaintEngine : public QRasterPaintEngine
{
public:
    // The image allocated here leaks... Fix if this code is ifdef'ed
    // in
    QOnScreenRasterPaintEngine()
        : QRasterPaintEngine(new QImage(qt_primary_surface_bits,
                                        QApplication::desktop()->width(),
                                        QApplication::desktop()->height(),
                                        qt_primary_surface_stride,
                                        qt_primary_surface_format))
    {
        device = static_cast<QImage *>(d_func()->device);
    }

    bool begin(QPaintDevice *)
    {
        QRegion clip = systemClip();
        originalSystemClip = clip;
        clip.translate(widget->mapToGlobal(QPoint(0, 0)));
        setSystemClip(clip);

        QRect bounds = clip.boundingRect();
        DDSURFACEDESC surface;
        surface.dwSize = sizeof(DDSURFACEDESC);
        HRESULT res = qt_ddraw_primary->Lock((RECT *) &bounds, &surface, DDLOCK_WAIT, 0);
        if (res != DD_OK) {
            qWarning("QWidget painting: locking onscreen bits failed: %d\n", res);
            return false;
        }

        if (surface.lpSurface == qt_primary_surface_bits) {
            qt_primary_surface_bits = (uchar *) surface.lpSurface;
            device->data_ptr()->data = qt_primary_surface_bits;
        }

        return QRasterPaintEngine::begin(device);
    }

    bool end()
    {
        HRESULT res = qt_ddraw_primary->Unlock(0);
        if (res != DD_OK)
            qWarning("QWidget::paint, failed to unlock DirectDraw surface: %d", res);
        bool ok = QRasterPaintEngine::end();
        setSystemClip(originalSystemClip);
        return ok;
    }

    QPoint coordinateOffset() const {
        return -widget->mapToGlobal(QPoint(0, 0));
    }

    const QWidget *widget;
    QImage *device;
    QRegion originalSystemClip;
};
Q_GLOBAL_STATIC(QOnScreenRasterPaintEngine, onScreenPaintEngine)
#else
void qt_win_initialize_directdraw() { }
#endif

QPaintEngine *QWidget::paintEngine() const
{
#ifndef QT_NO_DIRECTDRAW
    QOnScreenRasterPaintEngine *pe = onScreenPaintEngine();
    pe->widget = this;
    return pe;
#endif

    // We set this bit which is checked in setAttribute for
    // Qt::WA_PaintOnScreen. We do this to allow these two scenarios:
    //
    // 1. Users accidentally set Qt::WA_PaintOnScreen on X and port to
    // windows which would mean suddenly their widgets stop working.
    //
    // 2. Users set paint on screen and subclass paintEngine() to
    // return 0, in which case we have a "hole" in the backingstore
    // allowing use of GDI or DirectX directly.
    //
    // 1 is WRONG, but to minimize silent failures, we have set this
    // bit to ignore the setAttribute call. 2. needs to be
    // supported because its our only means of embeddeding native
    // graphics stuff.
    const_cast<QWidgetPrivate *>(d_func())->noPaintOnScreen = 1;

    return 0;
}

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface_sys()
{
    Q_Q(QWidget);
    return new QRasterWindowSurface(q);
}

void QWidgetPrivate::setModal_sys()
{
}

void QWidgetPrivate::registerTouchWindow()
{
    Q_Q(QWidget);

    // enable WM_TOUCH* messages on our window
    if (q->testAttribute(Qt::WA_WState_Created)
        && QApplicationPrivate::RegisterTouchWindow
        && q->windowType() != Qt::Desktop)
        QApplicationPrivate::RegisterTouchWindow(q->effectiveWinId(), 0);
}

void QWidgetPrivate::winSetupGestures()
{
#if !defined(QT_NO_GESTURES) && !defined(QT_NO_NATIVE_GESTURES)
    Q_Q(QWidget);
    if (!q || !q->isVisible() || !nativeGesturePanEnabled)
        return;

    if (!QApplicationPrivate::HasTouchSupport)
        return;
    QApplicationPrivate *qAppPriv = QApplicationPrivate::instance();
    if (!qAppPriv->SetGestureConfig)
        return;
    WId winid = q->internalWinId();

    bool needh = false;
    bool needv = false;
    bool singleFingerPanEnabled = false;

#ifndef QT_NO_SCROLLAREA
    if (QAbstractScrollArea *asa = qobject_cast<QAbstractScrollArea*>(q->parent())) {
        QScrollBar *hbar = asa->horizontalScrollBar();
        QScrollBar *vbar = asa->verticalScrollBar();
        Qt::ScrollBarPolicy hbarpolicy = asa->horizontalScrollBarPolicy();
        Qt::ScrollBarPolicy vbarpolicy = asa->verticalScrollBarPolicy();
        needh = (hbarpolicy == Qt::ScrollBarAlwaysOn ||
                 (hbarpolicy == Qt::ScrollBarAsNeeded && hbar->minimum() < hbar->maximum()));
        needv = (vbarpolicy == Qt::ScrollBarAlwaysOn ||
                 (vbarpolicy == Qt::ScrollBarAsNeeded && vbar->minimum() < vbar->maximum()));
        singleFingerPanEnabled = asa->d_func()->singleFingerPanEnabled;
        if (!winid) {
            winid = q->winId(); // enforces the native winid on the viewport
        }
    }
#endif //QT_NO_SCROLLAREA
    if (winid) {
        GESTURECONFIG gc[1];
        memset(gc, 0, sizeof(gc));
        gc[0].dwID = GID_PAN;
        if (nativeGesturePanEnabled) {
            gc[0].dwWant = GC_PAN;
            if (needv && singleFingerPanEnabled)
                gc[0].dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
            else
                gc[0].dwBlock |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
            if (needh && singleFingerPanEnabled)
                gc[0].dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
            else
                gc[0].dwBlock |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
        } else {
            gc[0].dwBlock = GC_PAN;
        }

        qAppPriv->SetGestureConfig(winid, 0, sizeof(gc)/sizeof(gc[0]), gc, sizeof(gc[0]));
    }
#endif
}

QT_END_NAMESPACE

#ifdef Q_WS_WINCE
#       include "qwidget_wince.cpp"
#endif
