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

#ifdef Q_WS_WINCE

#include "qguifunctions_wince.h"

QT_BEGIN_NAMESPACE

const QString qt_reg_winclass(QWidget *w);                // defined in qapplication_win.cpp
extern "C" LRESULT QT_WIN_CALLBACK QtWndProc(HWND, UINT, WPARAM, LPARAM);

//#define TABLET_DEBUG
#define PACKETDATA  (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE \
                     | PK_ORIENTATION | PK_CURSOR | PK_Z)
#define PACKETMODE  0

typedef HCTX        (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL        (API *PtrWTClose)(HCTX);
typedef UINT        (API *PtrWTInfo)(UINT, UINT, LPVOID);
typedef BOOL        (API *PtrWTEnable)(HCTX, BOOL);
typedef BOOL        (API *PtrWTOverlap)(HCTX, BOOL);
typedef int        (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
typedef BOOL        (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
typedef int     (API *PtrWTQueueSizeGet)(HCTX);
typedef BOOL    (API *PtrWTQueueSizeSet)(HCTX, int);

#ifndef QT_NO_TABLETEVENT
static void qt_tablet_init_wce();
static void qt_tablet_cleanup_wce();

static void qt_tablet_init_wce() {
    static bool firstTime = true;
    if (!firstTime)
        return;
    firstTime = false;
    qt_tablet_widget = new QWidget(0);
    qt_tablet_widget->createWinId();
    qt_tablet_widget->setObjectName(QLatin1String("Qt internal tablet widget"));
    LOGCONTEXT lcMine;
    qAddPostRoutine(qt_tablet_cleanup_wce);
    struct tagAXIS tpOri[3];
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

static void qt_tablet_cleanup_wce() {
    if (ptrWTClose)
        ptrWTClose(qt_tablet_context);
    delete qt_tablet_widget;
    qt_tablet_widget = 0;
}
#endif // QT_NO_TABLETEVENT


// The internal qWinRequestConfig, defined in qapplication_win.cpp, stores move,
// resize and setGeometry requests for a widget that is already
// processing a config event. The purpose is to avoid recursion.
//
void qWinRequestConfig(WId, int, int, int, int, int);

void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow) {
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
    WId id;

    QString windowClassName = qt_reg_winclass(q);

    if (!window)                                // always initialize
        initializeWindow = true;

    if (popup)
        flags |= Qt::WindowStaysOnTopHint; // a popup stays on top

    if (flags & (Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowContextHelpButtonHint)) {
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowTitleHint;
        flags &= ~Qt::FramelessWindowHint;
    }

    if (sw < 0) {                                // get the (primary) screen size
        sw = GetSystemMetrics(SM_CXSCREEN);
        sh = GetSystemMetrics(SM_CYSCREEN);
    }

    if (desktop) {                                // desktop widget
        popup = false;                                // force this flags off
        data.crect.setRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    parentw = q->parentWidget() ? q->parentWidget()->effectiveWinId() : 0;

    QString title;
    int style = WS_CHILD;
    int exsty = WS_EX_NOPARENTNOTIFY;

    if (topLevel) {
        if (!(flags & Qt::FramelessWindowHint) && !tool && !q->testAttribute(Qt::WA_DontShowOnScreen))
          style = (WS_OVERLAPPED) | WS_SYSMENU;
        else
          style = WS_POPUP;
        if ((type == Qt::ToolTip) || (type == Qt::SplashScreen)) {
            style = WS_POPUP;
            exsty |= WS_EX_NOANIMATION;
        } else {
            if (flags & Qt::WindowTitleHint)
                style |= WS_CAPTION;
            if (flags & Qt::WindowSystemMenuHint)
                style |= WS_SYSMENU;
            if (flags & Qt::WindowContextHelpButtonHint)
                exsty |= WS_EX_CONTEXTHELP;
#ifndef Q_WS_WINCE_WM
            if (flags & Qt::WindowMinimizeButtonHint)
                style |= WS_MINIMIZEBOX;
            if (shouldShowMaximizeButton())
                style |= WS_MAXIMIZEBOX;
#endif
            if (tool)
                exsty |= WS_EX_TOOLWINDOW;
        }
    }
    if (dialog) {
        style = WS_BORDER | WS_CAPTION;
        if (flags & Qt::WindowOkButtonHint)
            exsty |= WS_EX_CAPTIONOKBTN;
        if (flags & Qt::WindowCancelButtonHint || flags & Qt::WA_DeleteOnClose)
            style |= WS_SYSMENU;
        if (flags & Qt::WindowContextHelpButtonHint)
            exsty |= WS_EX_CONTEXTHELP;
    }
    if (popup) {
        style = WS_POPUP;
        exsty |= WS_EX_NOANIMATION;
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

        res = SetWindowLong( window, GWL_WNDPROC, (LONG)QtWndProc );

        if (!res)
            qErrnoWarning("QWidget::create: Failed to set window procedure");
    } else if (desktop) {                        // desktop widget
        id = GetDesktopWindow();
        if (!id) { //Create a dummy desktop
            RECT r;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
            id = CreateWindow(reinterpret_cast<const wchar_t *>(windowClassName.utf16()),
                              reinterpret_cast<const wchar_t *>(title.utf16()), style,
                              r.left, r.top, r.right - r.left, r.bottom - r.top,
                              0, 0, appinst, 0);
        }
        setWinId(id);
    } else if (topLevel) {                       // create top-level widget
        const bool wasMoved = q->testAttribute(Qt::WA_Moved);
        
        int x, y;
        if (qt_wince_is_mobile()) {
            x = wasMoved ? data.crect.left() : CW_USEDEFAULT;
            y = wasMoved ? data.crect.top() : CW_USEDEFAULT;
        } else {
            x = wasMoved ? data.crect.left() : 100;
            y = wasMoved ? data.crect.top() : 100;
        }
        
        int w = CW_USEDEFAULT;
        int h = CW_USEDEFAULT;

        // Adjust for framestrut when needed
        RECT rect = {0,0,0,0};
        if (AdjustWindowRectEx(&rect, style, FALSE, exsty)) {
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

        id = CreateWindowEx(exsty, reinterpret_cast<const wchar_t *>(windowClassName.utf16()),
                            reinterpret_cast<const wchar_t *>(title.utf16()), style,
                            x, y, w, h,
                            0, 0, appinst, 0);

        if (!id)
            qErrnoWarning("QWidget::create: Failed to create window");
        setWinId(id);
        if ((flags & Qt::WindowStaysOnTopHint) || (type == Qt::ToolTip))
            SetWindowPos(id, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    } else if (q->testAttribute(Qt::WA_NativeWindow) || paintOnScreen()) { // create child widget
        id = CreateWindowEx(exsty, (wchar_t*)windowClassName.utf16(), (wchar_t*)title.utf16(), style,
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
        if (!q->testAttribute(Qt::WA_DontShowOnScreen) || q->testAttribute(Qt::WA_Moved))
            ClientToScreen(id, &pt);
        data.crect = QRect(QPoint(pt.x, pt.y),
                            QPoint(pt.x + cr.right - 1, pt.y + cr.bottom - 1));

        if (data.fstrut_dirty) {
            // be nice to activeqt
            updateFrameStrut();
        }
    }

    q->setAttribute(Qt::WA_WState_Created);                // accept move/resize events
    hd = 0;                                        // no display context

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
        qt_tablet_init_wce();
#endif // QT_NO_TABLETEVENT

    if (q->testAttribute(Qt::WA_DropSiteRegistered))
        registerDropSite(true);

    if (maybeTopData() && maybeTopData()->opacity != 255)
        q->setWindowOpacity(maybeTopData()->opacity/255.);

    if (!topLevel && q->testAttribute(Qt::WA_NativeWindow) && q->testAttribute(Qt::WA_Mapped)) {
        Q_ASSERT(q->internalWinId());
        ShowWindow(q->internalWinId(), SW_SHOW);
    }
}

/*
  \internal
  Platform-specific part of QWidget::show().
*/
void QWidgetPrivate::show_sys() {
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


    int sm = SW_SHOW;
    bool fakedMaximize = false;
    if (q->isWindow()) {
#ifndef Q_WS_WINCE_WM
        if (q->isMinimized()) {
            sm = SW_SHOWMINIMIZED;
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
        } else
#else
        // Imitate minimizing on Windows mobile by hiding the widget.
        if (q->isMinimized())
            sm = SW_HIDE;
#endif
        if (q->isHidden()) {
            sm = SW_HIDE;
        }
    }
    if (q->testAttribute(Qt::WA_ShowWithoutActivating)
        || (q->windowType() == Qt::Popup)
        || (q->windowType() == Qt::ToolTip)
        || (q->windowType() == Qt::Tool)) {
        sm = SW_SHOWNOACTIVATE;
    }

    ShowWindow(q->internalWinId(), sm);

    if (q->isMaximized() && q->isWindow())
        qt_wince_maximize(q);

#ifndef Q_WS_WINCE_WM
    if (!qt_wince_is_mobile() && q->isFullScreen()) {
        HWND handle = FindWindow(L"HHTaskBar", L"");
        if (handle) {
            ShowWindow(handle, SW_HIDE);
            EnableWindow(handle, false);
        }
    }

    if (fakedMaximize) {
        int style = GetWindowLong(q->internalWinId(), GWL_STYLE);
        SetWindowLong(q->internalWinId(), GWL_STYLE, style & ~WS_MAXIMIZEBOX);
        SetWindowPos(q->internalWinId(), 0, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED);
    }
#else
    Q_UNUSED(fakedMaximize);
#endif

    if (q->isWindow() && sm == SW_SHOW)
        SetForegroundWindow(q->internalWinId());

    invalidateBuffer(q->rect());
}

void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;

    int max = SW_SHOWNORMAL;
    int normal = SW_SHOWNOACTIVATE;

    if ((oldstate & Qt::WindowMinimized) && !(newstate & Qt::WindowMinimized))
        newstate |= Qt::WindowActive;
    if (newstate & Qt::WindowActive)
        normal = SW_SHOWNORMAL;
    if (isWindow()) {
        createWinId();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if ((!testAttribute(Qt::WA_Resized) && !isVisible()))
            adjustSize();
        if (!d->topData()->normalGeometry.isValid()) {
            if (newstate & Qt::WindowMaximized && !(oldstate & Qt::WindowFullScreen))
                d->topData()->normalGeometry = geometry();
            if (newstate & Qt::WindowMinimized && !(oldstate & Qt::WindowFullScreen))
                d->topData()->normalGeometry = geometry();
        }
        if ((oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized)) {
            if (!(newstate & Qt::WindowMaximized)) {
                int style = GetWindowLong(internalWinId(), GWL_STYLE) | WS_BORDER | WS_POPUP | WS_CAPTION;
                SetWindowLong(internalWinId(), GWL_STYLE, style);
                SetWindowLong(internalWinId(), GWL_EXSTYLE, GetWindowLong (internalWinId(), GWL_EXSTYLE) & ~ WS_EX_NODRAG);
                qt_wince_unmaximize(this);
            }
            if (isVisible() && newstate & Qt::WindowMaximized)
                qt_wince_maximize(this);
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
                d->topData()->savedFlags = (Qt::WindowFlags)GetWindowLong(internalWinId(), GWL_STYLE);
                UINT style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLong(internalWinId(), GWL_STYLE, style);
                QRect r = qApp->desktop()->screenGeometry(this);
                UINT swpf = SWP_FRAMECHANGED;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;
                qt_wince_full_screen(internalWinId(), true, swpf);
                d->updateFrameStrut();
            } else {
                UINT style = d->topData()->savedFlags;
                if (isVisible())
                    style |= WS_VISIBLE;
                SetWindowLong(internalWinId(), GWL_STYLE, style);
                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE;
                if (newstate & Qt::WindowActive)
                    swpf |= SWP_NOACTIVATE;
                qt_wince_full_screen(internalWinId(), false, swpf);
                d->updateFrameStrut();

                // preserve maximized state
                if (isVisible()) {
                    ShowWindow(internalWinId(), (newstate & Qt::WindowMaximized) ? max : normal);
                    if (newstate & Qt::WindowMaximized)
                        qt_wince_maximize(this);
                }
                if (!(newstate & Qt::WindowMaximized)) {
                    QRect r = d->topData()->normalGeometry;
                    d->topData()->normalGeometry = QRect(0,0,-1,-1);
                    if (r.isValid())
                        setGeometry(r);
                }
            }
        }
        if ((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (newstate & Qt::WindowMinimized)
                qt_wince_minimize(internalWinId());
            else if (newstate & Qt::WindowMaximized) {
                ShowWindow(internalWinId(), max);
                qt_wince_maximize(this);
            } else {
                ShowWindow(internalWinId(), normal);
            }
        }
    }
    data->window_state = newstate;
    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
}

void QWidgetPrivate::deleteSysExtra()
{
    Q_Q(QWidget);
    if (!qt_wince_is_mobile() && q->isFullScreen()) {
        HWND handle = FindWindow(L"HHTaskBar", L"");
        if (handle) {
            ShowWindow(handle, SW_SHOWNORMAL);
            EnableWindow(handle, true);
        }
    }
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level) {
    Q_UNUSED(level);
    return;
}

// The procedure does nothing, but is required for mousegrabbing to work
LRESULT QT_WIN_CALLBACK qJournalRecordProc(int nCode, WPARAM wParam, LPARAM lParam) {
    Q_UNUSED(nCode);
    Q_UNUSED(wParam);
    Q_UNUSED(lParam);
    return 0;
}

void QWidget::grabMouse() {
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(internalWinId());
        mouseGrb = this;
    }
}

#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor) {
    if (!qt_nograb()) {
        if (mouseGrb)
            mouseGrb->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        SetCapture(internalWinId());
        mouseGrbCur = new QCursor(cursor);
        SetCursor(mouseGrbCur->handle());
        mouseGrb = this;
    }
}
#endif

void QWidget::releaseMouse() {
    if (!qt_nograb() && mouseGrb == this) {
        ReleaseCapture();
        if (journalRec) {
            journalRec = 0;
        }
        if (mouseGrbCur) {
            delete mouseGrbCur;
            mouseGrbCur = 0;
        }
        mouseGrb = 0;
    }
}

void QWidget::show()
{
    Qt::WindowFlags flags = windowFlags() & 0xff;
    int threshold = qApp->autoMaximizeThreshold();
    if ((threshold < 0) || (windowState() & Qt::WindowFullScreen) || (windowState() & Qt::WindowMaximized)) {
        setVisible(true);
        return;
    }
    int height = sizeHint().height();
    int screenHeight = (qreal(threshold) / 100.0f * qApp->desktop()->screenGeometry(this).height());
    bool maximize  = height > screenHeight;
    if (!maximize) {
        // If we do not maximize yet we check the widget and its child widgets whether they are
        //vertically expanding. If one of the widgets is expanding we maximize.
        QList<QWidget *> list = findChildren<QWidget *>();
        bool expandingChild = sizePolicy().verticalPolicy () == QSizePolicy::Expanding;
        for (int i = 0; (i < list.size()) && !expandingChild; ++i) {
            expandingChild = list.at(i)->sizePolicy().verticalPolicy () == QSizePolicy::Expanding;
        }
        maximize = expandingChild;
    }
    if ((minimumSizeHint().height() > qApp->desktop()->screenGeometry(this).height()) || (minimumSizeHint().width() > qApp->desktop()->screenGeometry(this).width()))
        maximize = false;

    if ((flags == Qt::Window || flags == Qt::Dialog) && maximize) {
        setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
            | Qt::WindowMaximized);
        setVisible(true);
    }
    else {
        setVisible(true);
    }
}

QT_END_NAMESPACE

#endif // Q_WS_WINCE
