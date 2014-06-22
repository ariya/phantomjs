/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbwindow.h"

#include <QtDebug>
#include <QScreen>
#include <QtGui/QIcon>
#include <QtGui/QRegion>

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbdrag.h"
#include "qxcbkeyboard.h"
#include "qxcbwmsupport.h"
#include "qxcbimage.h"
#include "qxcbnativeinterface.h"

#include <qpa/qplatformintegration.h>

#include <algorithm>

// FIXME This workaround can be removed for xcb-icccm > 3.8
#define class class_name
#include <xcb/xcb_icccm.h>
#undef class
#include <xcb/xfixes.h>
#include <xcb/shape.h>

// xcb-icccm 3.8 support
#ifdef XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS
#define xcb_get_wm_hints_reply xcb_icccm_get_wm_hints_reply
#define xcb_get_wm_hints xcb_icccm_get_wm_hints
#define xcb_get_wm_hints_unchecked xcb_icccm_get_wm_hints_unchecked
#define xcb_set_wm_hints xcb_icccm_set_wm_hints
#define xcb_set_wm_normal_hints xcb_icccm_set_wm_normal_hints
#define xcb_size_hints_set_base_size xcb_icccm_size_hints_set_base_size
#define xcb_size_hints_set_max_size xcb_icccm_size_hints_set_max_size
#define xcb_size_hints_set_min_size xcb_icccm_size_hints_set_min_size
#define xcb_size_hints_set_position xcb_icccm_size_hints_set_position
#define xcb_size_hints_set_resize_inc xcb_icccm_size_hints_set_resize_inc
#define xcb_size_hints_set_size xcb_icccm_size_hints_set_size
#define xcb_size_hints_set_win_gravity xcb_icccm_size_hints_set_win_gravity
#define xcb_wm_hints_set_iconic xcb_icccm_wm_hints_set_iconic
#define xcb_wm_hints_set_normal xcb_icccm_wm_hints_set_normal
#define xcb_wm_hints_set_input xcb_icccm_wm_hints_set_input
#define xcb_wm_hints_t xcb_icccm_wm_hints_t
#define XCB_WM_STATE_ICONIC XCB_ICCCM_WM_STATE_ICONIC
#define XCB_WM_STATE_WITHDRAWN XCB_ICCCM_WM_STATE_WITHDRAWN
#endif

#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>

#include <qpa/qplatformbackingstore.h>
#include <qpa/qwindowsysteminterface.h>

#include <stdio.h>

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(XCB_USE_XINPUT2_MAEMO) || defined(XCB_USE_XINPUT2)
#include <X11/extensions/XInput2.h>
#endif

#if defined(XCB_USE_GLX)
#include "qglxintegration.h"
#include <QtPlatformSupport/private/qglxconvenience_p.h>
#elif defined(XCB_USE_EGL)
#include "qxcbeglsurface.h"
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qxlibeglintegration_p.h>
#endif

#define XCOORD_MAX 16383
enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

//#ifdef NET_WM_STATE_DEBUG

QT_BEGIN_NAMESPACE

#undef FocusIn

enum QX11EmbedFocusInDetail {
    XEMBED_FOCUS_CURRENT = 0,
    XEMBED_FOCUS_FIRST = 1,
    XEMBED_FOCUS_LAST = 2
};

enum QX11EmbedInfoFlags {
    XEMBED_MAPPED = (1 << 0),
};

enum QX11EmbedMessageType {
    XEMBED_EMBEDDED_NOTIFY = 0,
    XEMBED_WINDOW_ACTIVATE = 1,
    XEMBED_WINDOW_DEACTIVATE = 2,
    XEMBED_REQUEST_FOCUS = 3,
    XEMBED_FOCUS_IN = 4,
    XEMBED_FOCUS_OUT = 5,
    XEMBED_FOCUS_NEXT = 6,
    XEMBED_FOCUS_PREV = 7,
    XEMBED_MODALITY_ON = 10,
    XEMBED_MODALITY_OFF = 11,
    XEMBED_REGISTER_ACCELERATOR = 12,
    XEMBED_UNREGISTER_ACCELERATOR = 13,
    XEMBED_ACTIVATE_ACCELERATOR = 14
};

const quint32 XEMBED_VERSION = 0;

// Returns \c true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWindow *w)
{
    return w->type() == Qt::Dialog
           || w->type() == Qt::Sheet
           || w->type() == Qt::Tool
           || w->type() == Qt::SplashScreen
           || w->type() == Qt::ToolTip
           || w->type() == Qt::Drawer
           || w->type() == Qt::Popup;
}

static inline QImage::Format imageFormatForDepth(int depth)
{
    switch (depth) {
        case 32: return QImage::Format_ARGB32_Premultiplied;
        case 24: return QImage::Format_RGB32;
        case 16: return QImage::Format_RGB16;
        default:
                 qWarning("Unsupported screen depth: %d", depth);
                 return QImage::Format_Invalid;
    }
}

static inline bool positionIncludesFrame(QWindow *w)
{
    return qt_window_private(w)->positionPolicy == QWindowPrivate::WindowFrameInclusive;
}

QXcbWindow::QXcbWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_window(0)
    , m_syncCounter(0)
    , m_gravity(XCB_GRAVITY_STATIC)
    , m_mapped(false)
    , m_transparent(false)
    , m_usingSyncProtocol(false)
    , m_deferredActivation(false)
    , m_embedded(false)
    , m_alertState(false)
    , m_netWmUserTimeWindow(XCB_NONE)
    , m_dirtyFrameMargins(false)
#if defined(XCB_USE_EGL)
    , m_eglSurface(0)
#endif
    , m_lastWindowStateEvent(-1)
{
    m_screen = static_cast<QXcbScreen *>(window->screen()->handle());

    setConnection(m_screen->connection());

    if (window->type() != Qt::ForeignWindow)
        create();
    else
        m_window = window->winId();
}

void QXcbWindow::create()
{
    destroy();

    m_deferredExpose = false;
    m_configureNotifyPending = true;
    m_windowState = Qt::WindowNoState;

    Qt::WindowType type = window()->type();

    if (type == Qt::Desktop) {
        m_window = m_screen->root();
        m_depth = m_screen->screen()->root_depth;
        m_imageFormat = imageFormatForDepth(m_depth);
        connection()->addWindowEventListener(m_window, this);
        return;
    }

    // Determine gravity from initial position. Do not change
    // later as it will cause the window to move uncontrollably.
    m_gravity = positionIncludesFrame(window()) ?
                XCB_GRAVITY_NORTH_WEST : XCB_GRAVITY_STATIC;

    const quint32 mask = XCB_CW_BACK_PIXMAP | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_SAVE_UNDER | XCB_CW_EVENT_MASK;
    const quint32 values[] = {
        // XCB_CW_BACK_PIXMAP
        XCB_NONE,
        // XCB_CW_OVERRIDE_REDIRECT
        type == Qt::Popup || type == Qt::ToolTip || (window()->flags() & Qt::BypassWindowManagerHint),
        // XCB_CW_SAVE_UNDER
        type == Qt::Popup || type == Qt::Tool || type == Qt::SplashScreen || type == Qt::ToolTip || type == Qt::Drawer,
        // XCB_CW_EVENT_MASK
        XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_BUTTON_MOTION
        | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_FOCUS_CHANGE
    };

    // Parameters to XCreateWindow() are frame corner + inner size.
    // This fits in case position policy is frame inclusive. There is
    // currently no way to implement it for frame-exclusive geometries.
    QRect rect = window()->geometry();
    QPlatformWindow::setGeometry(rect);

    QSize minimumSize = window()->minimumSize();
    if (rect.width() > 0 || rect.height() > 0) {
        rect.setWidth(qBound(1, rect.width(), XCOORD_MAX));
        rect.setHeight(qBound(1, rect.height(), XCOORD_MAX));
    } else if (minimumSize.width() > 0 || minimumSize.height() > 0) {
        rect.setSize(minimumSize);
    } else {
        rect.setWidth(defaultWindowWidth);
        rect.setHeight(defaultWindowHeight);
    }

    xcb_window_t xcb_parent_id = m_screen->root();
    if (parent()) {
        xcb_parent_id = static_cast<QXcbWindow *>(parent())->xcb_window();
        m_embedded = parent()->window()->type() == Qt::ForeignWindow;

        QSurfaceFormat parentFormat = parent()->window()->requestedFormat();
        if (window()->surfaceType() != QSurface::OpenGLSurface && parentFormat.hasAlpha()) {
            window()->setFormat(parentFormat);
        }
    }
    m_format = window()->requestedFormat();

#if (defined(XCB_USE_GLX) || defined(XCB_USE_EGL)) && defined(XCB_USE_XLIB)
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)) {
#if defined(XCB_USE_GLX)
        XVisualInfo *visualInfo = qglx_findVisualInfo(DISPLAY_FROM_XCB(m_screen), m_screen->screenNumber(), &m_format);
#elif defined(XCB_USE_EGL)
        EGLDisplay eglDisplay = connection()->egl_display();
        EGLConfig eglConfig = q_configFromGLFormat(eglDisplay, m_format, true);
        m_format = q_glFormatFromConfig(eglDisplay, eglConfig, m_format);

        VisualID id = QXlibEglIntegration::getCompatibleVisualId(DISPLAY_FROM_XCB(this), eglDisplay, eglConfig);

        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = id;

        XVisualInfo *visualInfo;
        int matchingCount = 0;
        visualInfo = XGetVisualInfo(DISPLAY_FROM_XCB(this), VisualIDMask, &visualInfoTemplate, &matchingCount);
#endif //XCB_USE_GLX
        if (!visualInfo && window()->surfaceType() == QSurface::OpenGLSurface)
            qFatal("Could not initialize OpenGL");

        if (!visualInfo && window()->surfaceType() == QSurface::RasterGLSurface) {
            qWarning("Could not initialize OpenGL for RasterGLSurface, reverting to RasterSurface.");
            window()->setSurfaceType(QSurface::RasterSurface);
        }
        if (visualInfo) {
            m_depth = visualInfo->depth;
            m_imageFormat = imageFormatForDepth(m_depth);
            Colormap cmap = XCreateColormap(DISPLAY_FROM_XCB(this), xcb_parent_id, visualInfo->visual, AllocNone);

            XSetWindowAttributes a;
            a.background_pixel = WhitePixel(DISPLAY_FROM_XCB(this), m_screen->screenNumber());
            a.border_pixel = BlackPixel(DISPLAY_FROM_XCB(this), m_screen->screenNumber());
            a.colormap = cmap;

            m_visualId = visualInfo->visualid;

            m_window = XCreateWindow(DISPLAY_FROM_XCB(this), xcb_parent_id, rect.x(), rect.y(), rect.width(), rect.height(),
                                      0, visualInfo->depth, InputOutput, visualInfo->visual,
                                      CWBackPixel|CWBorderPixel|CWColormap, &a);

            XFree(visualInfo);
        }
    }

    if (!m_window)
#endif //defined(XCB_USE_GLX) || defined(XCB_USE_EGL)
    {
        m_window = xcb_generate_id(xcb_connection());
        m_depth = m_screen->screen()->root_depth;
        m_imageFormat = imageFormatForDepth(m_depth);
        m_visualId = m_screen->screen()->root_visual;

        Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                     XCB_COPY_FROM_PARENT,            // depth -- same as root
                                     m_window,                        // window id
                                     xcb_parent_id,                   // parent window id
                                     rect.x(),
                                     rect.y(),
                                     rect.width(),
                                     rect.height(),
                                     0,                               // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                     m_visualId,                      // visual
                                     0,                               // value mask
                                     0));                             // value list
    }

    connection()->addWindowEventListener(m_window, this);

    Q_XCB_CALL(xcb_change_window_attributes(xcb_connection(), m_window, mask, values));

    propagateSizeHints();

    xcb_atom_t properties[5];
    int propertyCount = 0;
    properties[propertyCount++] = atom(QXcbAtom::WM_DELETE_WINDOW);
    properties[propertyCount++] = atom(QXcbAtom::WM_TAKE_FOCUS);
    properties[propertyCount++] = atom(QXcbAtom::_NET_WM_PING);

    m_usingSyncProtocol = m_screen->syncRequestSupported() && window()->surfaceType() != QSurface::OpenGLSurface;

    if (m_usingSyncProtocol)
        properties[propertyCount++] = atom(QXcbAtom::_NET_WM_SYNC_REQUEST);

    if (window()->flags() & Qt::WindowContextHelpButtonHint)
        properties[propertyCount++] = atom(QXcbAtom::_NET_WM_CONTEXT_HELP);

    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::WM_PROTOCOLS),
                                   XCB_ATOM_ATOM,
                                   32,
                                   propertyCount,
                                   properties));
    m_syncValue.hi = 0;
    m_syncValue.lo = 0;

    const QByteArray wmClass = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration())->wmClass();
    if (!wmClass.isEmpty()) {
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE,
                                       m_window, atom(QXcbAtom::WM_CLASS),
                                       XCB_ATOM_STRING, 8, wmClass.size(), wmClass.constData()));
    }

    if (m_usingSyncProtocol) {
        m_syncCounter = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_sync_create_counter(xcb_connection(), m_syncCounter, m_syncValue));

        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_SYNC_REQUEST_COUNTER),
                                       XCB_ATOM_CARDINAL,
                                       32,
                                       1,
                                       &m_syncCounter));
    }

    // set the PID to let the WM kill the application if unresponsive
    quint32 pid = getpid();
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_NET_WM_PID), XCB_ATOM_CARDINAL, 32,
                                   1, &pid));

    xcb_wm_hints_t hints;
    memset(&hints, 0, sizeof(hints));
    xcb_wm_hints_set_normal(&hints);

    xcb_wm_hints_set_input(&hints, !(window()->flags() & Qt::WindowDoesNotAcceptFocus));

    xcb_set_wm_hints(xcb_connection(), m_window, &hints);

    xcb_window_t leader = m_screen->clientLeader();
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::WM_CLIENT_LEADER), XCB_ATOM_WINDOW, 32,
                                   1, &leader));

    /* Add XEMBED info; this operation doesn't initiate the embedding. */
    quint32 data[] = { XEMBED_VERSION, XEMBED_MAPPED };
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_XEMBED_INFO),
                                   atom(QXcbAtom::_XEMBED_INFO),
                                   32, 2, (void *)data));


#ifdef XCB_USE_XINPUT2_MAEMO
    if (connection()->isUsingXInput2Maemo()) {
        XIEventMask xieventmask;
        uchar bitmask[2] = { 0, 0 };

        xieventmask.deviceid = XIAllMasterDevices;
        xieventmask.mask = bitmask;
        xieventmask.mask_len = sizeof(bitmask);

        XISetMask(bitmask, XI_ButtonPress);
        XISetMask(bitmask, XI_ButtonRelease);
        XISetMask(bitmask, XI_Motion);

        XISelectEvents(DISPLAY_FROM_XCB(this), m_window, &xieventmask, 1);
    }
#elif defined(XCB_USE_XINPUT2)
    connection()->xi2Select(m_window);
#endif

    setWindowState(window()->windowState());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());

    if (window()->flags() & Qt::WindowTransparentForInput)
        setTransparentForMouseEvents(true);

#ifdef XCB_USE_XLIB
    // force sync to read outstanding requests - see QTBUG-29106
    XSync(DISPLAY_FROM_XCB(m_screen), false);
#endif

#ifndef QT_NO_DRAGANDDROP
    connection()->drag()->dndEnable(this, true);
#endif

    const qreal opacity = qt_window_private(window())->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);
    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
}

QXcbWindow::~QXcbWindow()
{
    if (window()->type() != Qt::ForeignWindow)
        destroy();
}

void QXcbWindow::destroy()
{
    if (connection()->focusWindow() == this)
        connection()->setFocusWindow(0);

    if (m_syncCounter && m_usingSyncProtocol)
        Q_XCB_CALL(xcb_sync_destroy_counter(xcb_connection(), m_syncCounter));
    if (m_window) {
        if (m_netWmUserTimeWindow) {
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
            // Some window managers, like metacity, do XSelectInput on the _NET_WM_USER_TIME_WINDOW window,
            // without trapping BadWindow (which crashes when the user time window is destroyed).
            connection()->sync();
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        }
        connection()->removeWindowEventListener(m_window);
        Q_XCB_CALL(xcb_destroy_window(xcb_connection(), m_window));
        m_window = 0;
    }
    m_mapped = false;

#if defined(XCB_USE_EGL)
    delete m_eglSurface;
    m_eglSurface = 0;
#endif
}

void QXcbWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);

    propagateSizeHints();
    const QRect wmGeometry = windowToWmGeometry(rect);

    if (qt_window_private(window())->positionAutomatic) {
        const quint32 mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(1,           wmGeometry.width(),  XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.height(), XCOORD_MAX),
        };
        Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values)));
    } else {
        const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(-XCOORD_MAX, wmGeometry.x(),      XCOORD_MAX),
            qBound<qint32>(-XCOORD_MAX, wmGeometry.y(),      XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.width(),  XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.height(), XCOORD_MAX),
        };
        Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values)));
    }

    xcb_flush(xcb_connection());
}

QMargins QXcbWindow::frameMargins() const
{
    if (m_dirtyFrameMargins) {
        xcb_window_t window = m_window;
        xcb_window_t parent = m_window;

        bool foundRoot = false;

        const QVector<xcb_window_t> &virtualRoots =
            connection()->wmSupport()->virtualRoots();

        while (!foundRoot) {
            xcb_query_tree_cookie_t cookie = xcb_query_tree_unchecked(xcb_connection(), parent);

            xcb_query_tree_reply_t *reply = xcb_query_tree_reply(xcb_connection(), cookie, NULL);
            if (reply) {
                if (reply->root == reply->parent || virtualRoots.indexOf(reply->parent) != -1) {
                    foundRoot = true;
                } else {
                    window = parent;
                    parent = reply->parent;
                }

                free(reply);
            } else {
                m_dirtyFrameMargins = false;
                m_frameMargins = QMargins();
                return m_frameMargins;
            }
        }

        QPoint offset;

        xcb_translate_coordinates_reply_t *reply =
            xcb_translate_coordinates_reply(
                xcb_connection(),
                xcb_translate_coordinates(xcb_connection(), window, parent, 0, 0),
                NULL);

        if (reply) {
            offset = QPoint(reply->dst_x, reply->dst_y);
            free(reply);
        }

        xcb_get_geometry_reply_t *geom =
            xcb_get_geometry_reply(
                xcb_connection(),
                xcb_get_geometry(xcb_connection(), parent),
                NULL);

        if (geom) {
            // --
            // add the border_width for the window managers frame... some window managers
            // do not use a border_width of zero for their frames, and if we the left and
            // top strut, we ensure that pos() is absolutely correct.  frameGeometry()
            // will still be incorrect though... perhaps i should have foffset as well, to
            // indicate the frame offset (equal to the border_width on X).
            // - Brad
            // -- copied from qwidget_x11.cpp

            int left = offset.x() + geom->border_width;
            int top = offset.y() + geom->border_width;
            int right = geom->width + geom->border_width - geometry().width() - offset.x();
            int bottom = geom->height + geom->border_width - geometry().height() - offset.y();

            m_frameMargins = QMargins(left, top, right, bottom);

            free(geom);
        }

        m_dirtyFrameMargins = false;
    }

    return m_frameMargins;
}

void QXcbWindow::setVisible(bool visible)
{
    if (visible)
        show();
    else
        hide();
}

void QXcbWindow::show()
{
    if (window()->isTopLevel()) {
        xcb_get_property_cookie_t cookie = xcb_get_wm_hints_unchecked(xcb_connection(), m_window);

        xcb_wm_hints_t hints;
        xcb_get_wm_hints_reply(xcb_connection(), cookie, &hints, NULL);

        if (window()->windowState() & Qt::WindowMinimized)
            xcb_wm_hints_set_iconic(&hints);
        else
            xcb_wm_hints_set_normal(&hints);

        xcb_wm_hints_set_input(&hints, !(window()->flags() & Qt::WindowDoesNotAcceptFocus));

        xcb_set_wm_hints(xcb_connection(), m_window, &hints);

        // update WM_NORMAL_HINTS
        propagateSizeHints();

        // update WM_TRANSIENT_FOR
        const QWindow *tp = window()->transientParent();
        if (isTransient(window()) || tp != 0) {
            xcb_window_t transientXcbParent = 0;
            if (tp && tp->handle())
                transientXcbParent = static_cast<const QXcbWindow *>(tp->handle())->winId();
            // Default to client leader if there is no transient parent, else modal dialogs can
            // be hidden by their parents.
            if (!transientXcbParent)
                transientXcbParent = static_cast<QXcbScreen *>(screen())->clientLeader();
            if (transientXcbParent) { // ICCCM 4.1.2.6
                Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                               XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,
                                               1, &transientXcbParent));
            }
        }

        // update _MOTIF_WM_HINTS
        updateMotifWmHintsBeforeMap();

        // update _NET_WM_STATE
        updateNetWmStateBeforeMap();
    }

    if (connection()->time() != XCB_TIME_CURRENT_TIME)
        updateNetWmUserTime(connection()->time());

    Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));

    m_screen->windowShown(this);

    connection()->sync();
}

void QXcbWindow::hide()
{
    Q_XCB_CALL(xcb_unmap_window(xcb_connection(), m_window));

    // send synthetic UnmapNotify event according to icccm 4.1.4
    xcb_unmap_notify_event_t event;
    event.response_type = XCB_UNMAP_NOTIFY;
    event.event = m_screen->root();
    event.window = m_window;
    event.from_configure = false;
    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, m_screen->root(),
                              XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));

    xcb_flush(xcb_connection());

    m_mapped = false;
}

struct QtMotifWmHints {
    quint32 flags, functions, decorations;
    qint32 input_mode;
    quint32 status;
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

static QtMotifWmHints getMotifWmHints(QXcbConnection *c, xcb_window_t window)
{
    QtMotifWmHints hints;

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(c->xcb_connection(), 0, window, c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                         c->atom(QXcbAtom::_MOTIF_WM_HINTS), 0, 20);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(c->xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == c->atom(QXcbAtom::_MOTIF_WM_HINTS)) {
        hints = *((QtMotifWmHints *)xcb_get_property_value(reply));
    } else {
        hints.flags = 0L;
        hints.functions = MWM_FUNC_ALL;
        hints.decorations = MWM_DECOR_ALL;
        hints.input_mode = 0L;
        hints.status = 0L;
    }

    free(reply);

    return hints;
}

static void setMotifWmHints(QXcbConnection *c, xcb_window_t window, const QtMotifWmHints &hints)
{
    if (hints.flags != 0l) {
        Q_XCB_CALL2(xcb_change_property(c->xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       window,
                                       c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       32,
                                       5,
                                       &hints), c);
    } else {
        Q_XCB_CALL2(xcb_delete_property(c->xcb_connection(), window, c->atom(QXcbAtom::_MOTIF_WM_HINTS)), c);
    }
}

QXcbWindow::NetWmStates QXcbWindow::netWmStates()
{
    NetWmStates result(0);

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(xcb_connection(), 0, m_window, atom(QXcbAtom::_NET_WM_STATE),
                         XCB_ATOM_ATOM, 0, 1024);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const xcb_atom_t *states = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        const xcb_atom_t *statesEnd = states + reply->length;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_ABOVE)))
            result |= NetWmStateAbove;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_BELOW)))
            result |= NetWmStateBelow;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN)))
            result |= NetWmStateFullScreen;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ)))
            result |= NetWmStateMaximizedHorz;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT)))
            result |= NetWmStateMaximizedVert;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MODAL)))
            result |= NetWmStateModal;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP)))
            result |= NetWmStateStaysOnTop;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION)))
            result |= NetWmStateDemandsAttention;
        free(reply);
    } else {
#ifdef NET_WM_STATE_DEBUG
        printf("getting net wm state (%x), empty\n", m_window);
#endif
    }

    return result;
}

void QXcbWindow::setNetWmStates(NetWmStates states)
{
    QVector<xcb_atom_t> atoms;
    if (states & NetWmStateAbove)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_ABOVE));
    if (states & NetWmStateBelow)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_BELOW));
    if (states & NetWmStateFullScreen)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
    if (states & NetWmStateMaximizedHorz)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ));
    if (states & NetWmStateMaximizedVert)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
    if (states & NetWmStateModal)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MODAL));
    if (states & NetWmStateStaysOnTop)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP));
    if (states & NetWmStateDemandsAttention)
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION));

    if (atoms.isEmpty()) {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_STATE)));
    } else {
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                       atom(QXcbAtom::_NET_WM_STATE), XCB_ATOM_ATOM, 32,
                                       atoms.count(), atoms.constData()));
    }
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    if (flags & Qt::WindowTransparentForInput) {
        uint32_t mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_VISIBILITY_CHANGE
                 | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_RESIZE_REDIRECT
                | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                | XCB_EVENT_MASK_FOCUS_CHANGE  | XCB_EVENT_MASK_PROPERTY_CHANGE
                | XCB_EVENT_MASK_COLOR_MAP_CHANGE | XCB_EVENT_MASK_OWNER_GRAB_BUTTON;
        xcb_change_window_attributes(xcb_connection(), xcb_window(), XCB_CW_EVENT_MASK, &mask);
    }

    setNetWmWindowFlags(flags);
    setMotifWindowFlags(flags);

    setTransparentForMouseEvents(flags & Qt::WindowTransparentForInput);
    updateDoesNotAcceptFocus(flags & Qt::WindowDoesNotAcceptFocus);
}

void QXcbWindow::setMotifWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    QtMotifWmHints mwmhints;
    mwmhints.flags = 0L;
    mwmhints.functions = 0L;
    mwmhints.decorations = 0;
    mwmhints.input_mode = 0L;
    mwmhints.status = 0L;

    if (type != Qt::SplashScreen) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;

        bool customize = flags & Qt::CustomizeWindowHint;
        if (!(flags & Qt::FramelessWindowHint) && !(customize && !(flags & Qt::WindowTitleHint))) {
            mwmhints.decorations |= MWM_DECOR_BORDER;
            mwmhints.decorations |= MWM_DECOR_RESIZEH;
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
              | Qt::WindowCloseButtonHint)))
    {
        // a special case - only the titlebar without any button
        mwmhints.flags = MWM_HINTS_FUNCTIONS;
        mwmhints.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
        mwmhints.decorations = 0;
    }

    setMotifWmHints(connection(), m_window, mwmhints);
}

void QXcbWindow::changeNetWmState(bool set, xcb_atom_t one, xcb_atom_t two)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = m_window;
    event.type = atom(QXcbAtom::_NET_WM_STATE);
    event.data.data32[0] = set ? 1 : 0;
    event.data.data32[1] = one;
    event.data.data32[2] = two;
    event.data.data32[3] = 0;
    event.data.data32[4] = 0;

    Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
}

void QXcbWindow::setWindowState(Qt::WindowState state)
{
    if (state == m_windowState)
        return;

    // unset old state
    switch (m_windowState) {
    case Qt::WindowMinimized:
        Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));
        break;
    case Qt::WindowMaximized:
        changeNetWmState(false,
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ),
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
        break;
    case Qt::WindowFullScreen:
        changeNetWmState(false, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
        break;
    default:
        break;
    }

    // set new state
    switch (state) {
    case Qt::WindowMinimized:
        {
            xcb_client_message_event_t event;

            event.response_type = XCB_CLIENT_MESSAGE;
            event.format = 32;
            event.window = m_window;
            event.type = atom(QXcbAtom::WM_CHANGE_STATE);
            event.data.data32[0] = XCB_WM_STATE_ICONIC;
            event.data.data32[1] = 0;
            event.data.data32[2] = 0;
            event.data.data32[3] = 0;
            event.data.data32[4] = 0;

            Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
        }
        break;
    case Qt::WindowMaximized:
        changeNetWmState(true,
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ),
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
        break;
    case Qt::WindowFullScreen:
        changeNetWmState(true, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
        break;
    case Qt::WindowNoState:
        break;
    default:
        break;
    }

    connection()->sync();

    m_windowState = state;
}

void QXcbWindow::setNetWmWindowFlags(Qt::WindowFlags flags)
{
    // in order of decreasing priority
    QVector<uint> windowTypes;

    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DIALOG));
        break;
    case Qt::Tool:
    case Qt::Drawer:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_UTILITY));
        break;
    case Qt::ToolTip:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLTIP));
        break;
    case Qt::SplashScreen:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_SPLASH));
        break;
    default:
        break;
    }

    if (flags & Qt::FramelessWindowHint)
        windowTypes.append(atom(QXcbAtom::_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));

    windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL));

    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_NET_WM_WINDOW_TYPE), XCB_ATOM_ATOM, 32,
                                   windowTypes.count(), windowTypes.constData()));
}

void QXcbWindow::updateMotifWmHintsBeforeMap()
{
    QtMotifWmHints mwmhints = getMotifWmHints(connection(), m_window);

    if (window()->modality() != Qt::NonModal) {
        switch (window()->modality()) {
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

    if (window()->minimumSize() == window()->maximumSize()) {
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
    }

    if (window()->flags() & Qt::WindowMinimizeButtonHint) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;
        mwmhints.decorations |= MWM_DECOR_MINIMIZE;
        mwmhints.functions |= MWM_FUNC_MINIMIZE;
    }
    if (window()->flags() & Qt::WindowMaximizeButtonHint) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;
        mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
        mwmhints.functions |= MWM_FUNC_MAXIMIZE;
    }
    if (window()->flags() & Qt::WindowCloseButtonHint)
        mwmhints.functions |= MWM_FUNC_CLOSE;

    setMotifWmHints(connection(), m_window, mwmhints);
}

void QXcbWindow::updateNetWmStateBeforeMap()
{
    NetWmStates states(0);

    const Qt::WindowFlags flags = window()->flags();
    if (flags & Qt::WindowStaysOnTopHint) {
        states |= NetWmStateAbove;
        states |= NetWmStateStaysOnTop;
    } else if (flags & Qt::WindowStaysOnBottomHint) {
        states |= NetWmStateBelow;
    }

    if (window()->windowState() & Qt::WindowFullScreen)
        states |= NetWmStateFullScreen;

    if (window()->windowState() & Qt::WindowMaximized) {
        states |= NetWmStateMaximizedHorz;
        states |= NetWmStateMaximizedVert;
    }

    if (window()->modality() != Qt::NonModal)
        states |= NetWmStateModal;

    setNetWmStates(states);
}

void QXcbWindow::updateNetWmUserTime(xcb_timestamp_t timestamp)
{
    xcb_window_t wid = m_window;
    connection()->setNetWmUserTime(timestamp);

    const bool isSupportedByWM = connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
    if (m_netWmUserTimeWindow || isSupportedByWM) {
        if (!m_netWmUserTimeWindow) {
            m_netWmUserTimeWindow = xcb_generate_id(xcb_connection());
            Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                         XCB_COPY_FROM_PARENT,            // depth -- same as root
                                         m_netWmUserTimeWindow,                        // window id
                                         m_window,                   // parent window id
                                         -1, -1, 1, 1,
                                         0,                               // border width
                                         XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                         m_visualId,                      // visual
                                         0,                               // value mask
                                         0));                             // value list
            wid = m_netWmUserTimeWindow;
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW),
                                XCB_ATOM_WINDOW, 32, 1, &m_netWmUserTimeWindow);
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME));
#ifndef QT_NO_DEBUG
            QByteArray ba("Qt NET_WM user time window");
            Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                           XCB_PROP_MODE_REPLACE,
                                           m_netWmUserTimeWindow,
                                           atom(QXcbAtom::_NET_WM_NAME),
                                           atom(QXcbAtom::UTF8_STRING),
                                           8,
                                           ba.length(),
                                           ba.constData()));
#endif
        } else if (!isSupportedByWM) {
            // WM no longer supports it, then we should remove the
            // _NET_WM_USER_TIME_WINDOW atom.
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        } else {
            wid = m_netWmUserTimeWindow;
        }
    }
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, wid, atom(QXcbAtom::_NET_WM_USER_TIME),
                        XCB_ATOM_CARDINAL, 32, 1, &timestamp);
}

void QXcbWindow::setTransparentForMouseEvents(bool transparent)
{
    if (!connection()->hasXFixes() || transparent == m_transparent)
        return;

    xcb_rectangle_t rectangle;

    xcb_rectangle_t *rect = 0;
    int nrect = 0;

    if (!transparent) {
        rectangle.x = 0;
        rectangle.y = 0;
        rectangle.width = geometry().width();
        rectangle.height = geometry().height();
        rect = &rectangle;
        nrect = 1;
    }

    xcb_xfixes_region_t region = xcb_generate_id(xcb_connection());
    xcb_xfixes_create_region(xcb_connection(), region, nrect, rect);
    xcb_xfixes_set_window_shape_region_checked(xcb_connection(), m_window, XCB_SHAPE_SK_INPUT, 0, 0, region);
    xcb_xfixes_destroy_region(xcb_connection(), region);

    m_transparent = transparent;
}

void QXcbWindow::updateDoesNotAcceptFocus(bool doesNotAcceptFocus)
{
    xcb_get_property_cookie_t cookie = xcb_get_wm_hints_unchecked(xcb_connection(), m_window);

    xcb_wm_hints_t hints;
    if (!xcb_get_wm_hints_reply(xcb_connection(), cookie, &hints, NULL)) {
        return;
    }

    xcb_wm_hints_set_input(&hints, !doesNotAcceptFocus);
    xcb_set_wm_hints(xcb_connection(), m_window, &hints);
}

WId QXcbWindow::winId() const
{
    return m_window;
}

void QXcbWindow::setParent(const QPlatformWindow *parent)
{
    QPoint topLeft = geometry().topLeft();

    xcb_window_t xcb_parent_id;
    if (parent) {
        const QXcbWindow *qXcbParent = static_cast<const QXcbWindow *>(parent);
        xcb_parent_id = qXcbParent->xcb_window();
        m_embedded = qXcbParent->window()->type() == Qt::ForeignWindow;
    } else {
        xcb_parent_id = m_screen->root();
        m_embedded = false;
    }
    Q_XCB_CALL(xcb_reparent_window(xcb_connection(), xcb_window(), xcb_parent_id, topLeft.x(), topLeft.y()));
}

void QXcbWindow::setWindowTitle(const QString &title)
{
    const QString fullTitle = formatWindowTitle(title, QString::fromUtf8(" \xe2\x80\x94 ")); // unicode character U+2014, EM DASH
    const QByteArray ba = fullTitle.toUtf8();
    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_NAME),
                                   atom(QXcbAtom::UTF8_STRING),
                                   8,
                                   ba.length(),
                                   ba.constData()));
}

void QXcbWindow::setWindowIcon(const QIcon &icon)
{
    QVector<quint32> icon_data;

    if (!icon.isNull()) {
        QList<QSize> availableSizes = icon.availableSizes();
        if (availableSizes.isEmpty()) {
            // try to use default sizes since the icon can be a scalable image like svg.
            availableSizes.push_back(QSize(16,16));
            availableSizes.push_back(QSize(32,32));
            availableSizes.push_back(QSize(64,64));
            availableSizes.push_back(QSize(128,128));
        }
        for (int i = 0; i < availableSizes.size(); ++i) {
            QSize size = availableSizes.at(i);
            QPixmap pixmap = icon.pixmap(size);
            if (!pixmap.isNull()) {
                QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
                int pos = icon_data.size();
                icon_data.resize(pos + 2 + image.width()*image.height());
                icon_data[pos++] = image.width();
                icon_data[pos++] = image.height();
                memcpy(icon_data.data() + pos, image.bits(), image.width()*image.height()*4);
            }
        }
    }

    if (!icon_data.isEmpty()) {
        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_ICON),
                                       atom(QXcbAtom::CARDINAL),
                                       32,
                                       icon_data.size(),
                                       (unsigned char *) icon_data.data()));
    } else {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(),
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_ICON)));
    }
}

void QXcbWindow::raise()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_ABOVE };
    Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, values));
}

void QXcbWindow::lower()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_BELOW };
    Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, values));
}

// Adapt the geometry to match the WM expection with regards
// to gravity.
QRect QXcbWindow::windowToWmGeometry(QRect r) const
{
    if (m_dirtyFrameMargins || m_frameMargins.isNull())
        return r;
    const bool frameInclusive = positionIncludesFrame(window());
    // XCB_GRAVITY_STATIC requires the inner geometry, whereas
    // XCB_GRAVITY_NORTH_WEST requires the frame geometry
    if (frameInclusive && m_gravity == XCB_GRAVITY_STATIC) {
        r.translate(m_frameMargins.left(), m_frameMargins.top());
    } else if (!frameInclusive && m_gravity == XCB_GRAVITY_NORTH_WEST) {
        r.translate(-m_frameMargins.left(), -m_frameMargins.top());
    }
    return r;
}

void QXcbWindow::propagateSizeHints()
{
    // update WM_NORMAL_HINTS
    xcb_size_hints_t hints;
    memset(&hints, 0, sizeof(hints));

    const QRect rect = windowToWmGeometry(geometry());

    QWindow *win = window();

    if (!qt_window_private(win)->positionAutomatic)
        xcb_size_hints_set_position(&hints, true, rect.x(), rect.y());
    if (rect.width() < QWINDOWSIZE_MAX || rect.height() < QWINDOWSIZE_MAX)
        xcb_size_hints_set_size(&hints, true, rect.width(), rect.height());
    xcb_size_hints_set_win_gravity(&hints, m_gravity);

    QSize minimumSize = win->minimumSize();
    QSize maximumSize = win->maximumSize();
    QSize baseSize = win->baseSize();
    QSize sizeIncrement = win->sizeIncrement();

    if (minimumSize.width() > 0 || minimumSize.height() > 0)
        xcb_size_hints_set_min_size(&hints, minimumSize.width(), minimumSize.height());

    if (maximumSize.width() < QWINDOWSIZE_MAX || maximumSize.height() < QWINDOWSIZE_MAX)
        xcb_size_hints_set_max_size(&hints,
                                    qMin(XCOORD_MAX, maximumSize.width()),
                                    qMin(XCOORD_MAX, maximumSize.height()));

    if (sizeIncrement.width() > 0 || sizeIncrement.height() > 0) {
        xcb_size_hints_set_base_size(&hints, baseSize.width(), baseSize.height());
        xcb_size_hints_set_resize_inc(&hints, sizeIncrement.width(), sizeIncrement.height());
    }

    xcb_set_wm_normal_hints(xcb_connection(), m_window, &hints);
}

void QXcbWindow::requestActivateWindow()
{
    /* Never activate embedded windows; doing that would prevent the container
     * to re-gain the keyboard focus later. */
    if (m_embedded) {
        QPlatformWindow::requestActivateWindow();
        return;
    }

    if (!m_mapped) {
        m_deferredActivation = true;
        return;
    }
    m_deferredActivation = false;

    updateNetWmUserTime(connection()->time());

    if (window()->isTopLevel()
        && !(window()->flags() & Qt::X11BypassWindowManagerHint)
        && connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::_NET_ACTIVE_WINDOW))) {
        xcb_client_message_event_t event;

        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.window = m_window;
        event.type = atom(QXcbAtom::_NET_ACTIVE_WINDOW);
        event.data.data32[0] = 1;
        event.data.data32[1] = connection()->time();
        QWindow *focusWindow = QGuiApplication::focusWindow();
        event.data.data32[2] = focusWindow ? focusWindow->winId() : XCB_NONE;
        event.data.data32[3] = 0;
        event.data.data32[4] = 0;

        Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
    } else {
        Q_XCB_CALL(xcb_set_input_focus(xcb_connection(), XCB_INPUT_FOCUS_PARENT, m_window, connection()->time()));
    }

    connection()->sync();
}

#if XCB_USE_MAEMO_WINDOW_PROPERTIES
void QXcbWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    int angle = 0;
    switch (orientation) {
        case Qt::PortraitOrientation: angle = 270; break;
        case Qt::LandscapeOrientation: angle = 0; break;
        case Qt::InvertedPortraitOrientation: angle = 90; break;
        case Qt::InvertedLandscapeOrientation: angle = 180; break;
        case Qt::PrimaryOrientation: break;
    }
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::MeegoTouchOrientationAngle), XCB_ATOM_CARDINAL, 32,
                                   1, &angle));
}
#endif

QSurfaceFormat QXcbWindow::format() const
{
    // ### return actual format
    return m_format;
}

#if defined(XCB_USE_EGL)
QXcbEGLSurface *QXcbWindow::eglSurface() const
{
    if (!m_eglSurface) {
        EGLDisplay display = connection()->egl_display();
        EGLConfig config = q_configFromGLFormat(display, window()->requestedFormat(), true);
        EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)m_window, 0);

        m_eglSurface = new QXcbEGLSurface(display, surface);
    }

    return m_eglSurface;
}
#endif

class ExposeCompressor
{
public:
    ExposeCompressor(xcb_window_t window, QRegion *region)
        : m_window(window)
        , m_region(region)
        , m_pending(true)
    {
    }

    bool checkEvent(xcb_generic_event_t *event)
    {
        if (!event)
            return false;
        if ((event->response_type & ~0x80) != XCB_EXPOSE)
            return false;
        xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
        if (expose->window != m_window)
            return false;
        if (expose->count == 0)
            m_pending = false;
        *m_region |= QRect(expose->x, expose->y, expose->width, expose->height);
        return true;
    }

    bool pending() const
    {
        return m_pending;
    }

private:
    xcb_window_t m_window;
    QRegion *m_region;
    bool m_pending;
};

bool QXcbWindow::handleGenericEvent(xcb_generic_event_t *event, long *result)
{
    return QWindowSystemInterface::handleNativeEvent(window(),
                                                     connection()->nativeInterface()->genericEventFilterType(),
                                                     event,
                                                     result);
}

void QXcbWindow::handleExposeEvent(const xcb_expose_event_t *event)
{
    QRect rect(event->x, event->y, event->width, event->height);

    if (m_exposeRegion.isEmpty())
        m_exposeRegion = rect;
    else
        m_exposeRegion |= rect;

    ExposeCompressor compressor(m_window, &m_exposeRegion);
    xcb_generic_event_t *filter = 0;
    do {
        filter = connection()->checkEvent(compressor);
        free(filter);
    } while (filter);

    // if count is non-zero there are more expose events pending
    if (event->count == 0 || !compressor.pending()) {
        QWindowSystemInterface::handleExposeEvent(window(), m_exposeRegion);
        m_exposeRegion = QRegion();
    }
}

void QXcbWindow::handleClientMessageEvent(const xcb_client_message_event_t *event)
{
    if (event->format != 32)
        return;

    if (event->type == atom(QXcbAtom::WM_PROTOCOLS)) {
        if (event->data.data32[0] == atom(QXcbAtom::WM_DELETE_WINDOW)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (event->data.data32[0] == atom(QXcbAtom::WM_TAKE_FOCUS)) {
            connection()->setTime(event->data.data32[1]);
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_PING)) {
            if (event->window == m_screen->root())
                return;

            xcb_client_message_event_t reply = *event;

            reply.response_type = XCB_CLIENT_MESSAGE;
            reply.window = m_screen->root();

            xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&reply);
            xcb_flush(xcb_connection());
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_SYNC_REQUEST)) {
            connection()->setTime(event->data.data32[1]);
            m_syncValue.lo = event->data.data32[2];
            m_syncValue.hi = event->data.data32[3];
#ifndef QT_NO_WHATSTHIS
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_CONTEXT_HELP)) {
            QWindowSystemInterface::handleEnterWhatsThisEvent();
#endif
        } else {
            qWarning() << "QXcbWindow: Unhandled WM_PROTOCOLS message:" << connection()->atomName(event->data.data32[0]);
        }
#ifndef QT_NO_DRAGANDDROP
    } else if (event->type == atom(QXcbAtom::XdndEnter)) {
        connection()->drag()->handleEnter(window(), event);
    } else if (event->type == atom(QXcbAtom::XdndPosition)) {
        connection()->drag()->handlePosition(window(), event);
    } else if (event->type == atom(QXcbAtom::XdndLeave)) {
        connection()->drag()->handleLeave(window(), event);
    } else if (event->type == atom(QXcbAtom::XdndDrop)) {
        connection()->drag()->handleDrop(window(), event);
#endif
    } else if (event->type == atom(QXcbAtom::_XEMBED)) {
        handleXEmbedMessage(event);
    } else if (event->type == atom(QXcbAtom::_NET_ACTIVE_WINDOW)) {
        connection()->setFocusWindow(this);
        QWindowSystemInterface::handleWindowActivated(window(), Qt::ActiveWindowFocusReason);
    } else if (event->type == atom(QXcbAtom::MANAGER)
               || event->type == atom(QXcbAtom::_NET_WM_STATE)
               || event->type == atom(QXcbAtom::WM_CHANGE_STATE)) {
        // Ignore _NET_WM_STATE, MANAGER which are relate to tray icons
        // and other messages.
    } else if (event->type == atom(QXcbAtom::_COMPIZ_DECOR_PENDING)
            || event->type == atom(QXcbAtom::_COMPIZ_DECOR_REQUEST)
            || event->type == atom(QXcbAtom::_COMPIZ_DECOR_DELETE_PIXMAP)) {
        //silence the _COMPIZ messages for now
    } else {
        qWarning() << "QXcbWindow: Unhandled client message:" << connection()->atomName(event->type);
    }
}

void QXcbWindow::handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event)
{
    bool fromSendEvent = (event->response_type & 0x80);
    QPoint pos(event->x, event->y);
    if (!parent() && !fromSendEvent) {
        // Do not trust the position, query it instead.
        xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(xcb_connection(), xcb_window(),
                                                                              m_screen->root(), 0, 0);
        xcb_translate_coordinates_reply_t *reply = xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
        if (reply) {
            pos.setX(reply->dst_x);
            pos.setY(reply->dst_y);
            free(reply);
        }
    }

    QRect rect(pos, QSize(event->width, event->height));

    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);

    if (!m_screen->availableGeometry().intersects(rect)) {
        Q_FOREACH (QPlatformScreen* screen, m_screen->virtualSiblings()) {
            if (screen->availableGeometry().intersects(rect)) {
                m_screen = static_cast<QXcbScreen*>(screen);
                QWindowSystemInterface::handleWindowScreenChanged(window(), m_screen->QPlatformScreen::screen());
                break;
            }
        }
    }

    m_configureNotifyPending = false;

    if (m_deferredExpose) {
        m_deferredExpose = false;
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }

    m_dirtyFrameMargins = true;
}

bool QXcbWindow::isExposed() const
{
    return m_mapped;
}

bool QXcbWindow::isEmbedded(const QPlatformWindow *parentWindow) const
{
    if (!m_embedded)
        return false;

    return parentWindow ? (parentWindow == parent()) : true;
}

QPoint QXcbWindow::mapToGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return pos;

    QPoint ret;
    xcb_translate_coordinates_cookie_t cookie =
        xcb_translate_coordinates(xcb_connection(), xcb_window(), m_screen->root(),
                                  pos.x(), pos.y());
    xcb_translate_coordinates_reply_t *reply =
        xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
        free(reply);
    }

    return ret;
}

QPoint QXcbWindow::mapFromGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return pos;
    QPoint ret;
    xcb_translate_coordinates_cookie_t cookie =
        xcb_translate_coordinates(xcb_connection(), m_screen->root(), xcb_window(),
                                  pos.x(), pos.y());
    xcb_translate_coordinates_reply_t *reply =
        xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
        free(reply);
    }

    return ret;
}

void QXcbWindow::handleMapNotifyEvent(const xcb_map_notify_event_t *event)
{
    if (event->window == m_window) {
        m_mapped = true;
        if (m_deferredActivation)
            requestActivateWindow();
        if (m_configureNotifyPending)
            m_deferredExpose = true;
        else
            QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }
}

void QXcbWindow::handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *event)
{
    if (event->window == m_window) {
        m_mapped = false;
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
{
    const bool isWheel = event->detail >= 4 && event->detail <= 7;
    if (!isWheel && window() != QGuiApplication::focusWindow()) {
        QWindow *w = static_cast<QWindowPrivate *>(QObjectPrivate::get(window()))->eventReceiver();
        if (!(w->flags() & Qt::WindowDoesNotAcceptFocus))
            w->requestActivate();
    }

    updateNetWmUserTime(event->time);

    if (m_embedded) {
        if (window() != QGuiApplication::focusWindow()) {
            const QXcbWindow *container = static_cast<const QXcbWindow *>(parent());
            Q_ASSERT(container != 0);

            sendXEmbedMessage(container->xcb_window(), XEMBED_REQUEST_FOCUS);
        }
    }

    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);

    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);

    if (isWheel) {
        if (!connection()->isUsingXInput21()) {
            // Logic borrowed from qapplication_x11.cpp
            int delta = 120 * ((event->detail == 4 || event->detail == 6) ? 1 : -1);
            bool hor = (((event->detail == 4 || event->detail == 5)
                         && (modifiers & Qt::AltModifier))
                        || (event->detail == 6 || event->detail == 7));

            QWindowSystemInterface::handleWheelEvent(window(), event->time,
                                                     local, global, delta, hor ? Qt::Horizontal : Qt::Vertical, modifiers);
        }
        return;
    }

    handleMouseEvent(event->time, local, global, modifiers);
}

void QXcbWindow::handleButtonReleaseEvent(const xcb_button_release_event_t *event)
{
    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);

    if (event->detail >= 4 && event->detail <= 7) {
        // mouse wheel, handled in handleButtonPressEvent()
        return;
    }

    handleMouseEvent(event->time, local, global, modifiers);
}

void QXcbWindow::handleMotionNotifyEvent(const xcb_motion_notify_event_t *event)
{
    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);

    handleMouseEvent(event->time, local, global, modifiers);
}

QXcbWindow *QXcbWindow::toWindow() { return this; }

void QXcbWindow::handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global, Qt::KeyboardModifiers modifiers)
{
    connection()->setTime(time);
    QWindowSystemInterface::handleMouseEvent(window(), time, local, global, connection()->buttons(), modifiers);
}

class EnterEventChecker
{
public:
    bool checkEvent(xcb_generic_event_t *event)
    {
        if (!event)
            return false;
        if ((event->response_type & ~0x80) != XCB_ENTER_NOTIFY)
            return false;

        xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)event;

        if ((enter->mode != XCB_NOTIFY_MODE_NORMAL && enter->mode != XCB_NOTIFY_MODE_UNGRAB)
            || enter->detail == XCB_NOTIFY_DETAIL_VIRTUAL
            || enter->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL)
        {
            return false;
        }

        return true;
    }
};

void QXcbWindow::handleEnterNotifyEvent(const xcb_enter_notify_event_t *event)
{
    connection()->setTime(event->time);
#ifdef XCB_USE_XINPUT2
    connection()->handleEnterEvent(event);
#endif

    if ((event->mode != XCB_NOTIFY_MODE_NORMAL && event->mode != XCB_NOTIFY_MODE_UNGRAB)
        || event->detail == XCB_NOTIFY_DETAIL_VIRTUAL
        || event->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL)
    {
        return;
    }

    const QPoint local(event->event_x, event->event_y);
    const QPoint global(event->root_x, event->root_y);
    QWindowSystemInterface::handleEnterEvent(window(), local, global);
}

void QXcbWindow::handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event)
{
    connection()->setTime(event->time);

    if ((event->mode != XCB_NOTIFY_MODE_NORMAL && event->mode != XCB_NOTIFY_MODE_UNGRAB)
        || event->detail == XCB_NOTIFY_DETAIL_VIRTUAL
        || event->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL)
    {
        return;
    }

    EnterEventChecker checker;
    xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)connection()->checkEvent(checker);
    QXcbWindow *enterWindow = enter ? connection()->platformWindowFromId(enter->event) : 0;

    if (enterWindow) {
        QPoint local(enter->event_x, enter->event_y);
        QPoint global(enter->root_x, enter->root_y);

        QWindowSystemInterface::handleEnterLeaveEvent(enterWindow->window(), window(), local, global);
    } else {
        QWindowSystemInterface::handleLeaveEvent(window());
    }

    free(enter);
}

void QXcbWindow::handlePropertyNotifyEvent(const xcb_property_notify_event_t *event)
{
    connection()->setTime(event->time);

    const bool propertyDeleted = event->state == XCB_PROPERTY_DELETE;

    if (event->atom == atom(QXcbAtom::_NET_WM_STATE) || event->atom == atom(QXcbAtom::WM_STATE)) {
        if (propertyDeleted)
            return;

        Qt::WindowState newState = Qt::WindowNoState;
        if (event->atom == atom(QXcbAtom::WM_STATE)) { // WM_STATE: Quick check for 'Minimize'.
            const xcb_get_property_cookie_t get_cookie =
            xcb_get_property(xcb_connection(), 0, m_window, atom(QXcbAtom::WM_STATE),
                             XCB_ATOM_ANY, 0, 1024);

            xcb_get_property_reply_t *reply =
                xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

            if (reply && reply->format == 32 && reply->type == atom(QXcbAtom::WM_STATE)) {
                const quint32 *data = (const quint32 *)xcb_get_property_value(reply);
                if (reply->length != 0 && XCB_WM_STATE_ICONIC == data[0])
                    newState = Qt::WindowMinimized;
            }
            free(reply);
        }
        if (newState != Qt::WindowMinimized) { // Something else changed, get _NET_WM_STATE.
            const NetWmStates states = netWmStates();
            if ((states & NetWmStateMaximizedHorz) && (states & NetWmStateMaximizedVert))
                newState = Qt::WindowMaximized;
            else if (states & NetWmStateFullScreen)
                newState = Qt::WindowFullScreen;
        }
        // Send Window state, compress events in case other flags (modality, etc) are changed.
        if (m_lastWindowStateEvent != newState) {
            QWindowSystemInterface::handleWindowStateChanged(window(), newState);
            m_lastWindowStateEvent = newState;
            m_windowState = newState;
        }
        return;
    } else if (event->atom == atom(QXcbAtom::_NET_WORKAREA) && event->window == m_screen->root()) {
        m_screen->updateGeometry(event->time);
    }
}

void QXcbWindow::handleFocusInEvent(const xcb_focus_in_event_t *)
{
    QWindow *w = window();
    w = static_cast<QWindowPrivate *>(QObjectPrivate::get(w))->eventReceiver();
    connection()->setFocusWindow(static_cast<QXcbWindow *>(w->handle()));
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
}

static bool focusInPeeker(QXcbConnection *connection, xcb_generic_event_t *event)
{
    if (!event) {
        // FocusIn event is not in the queue, proceed with FocusOut normally.
        QWindowSystemInterface::handleWindowActivated(0, Qt::ActiveWindowFocusReason);
        return true;
    }
    uint response_type = event->response_type & ~0x80;
    if (response_type == XCB_FOCUS_IN)
        return true;

    /* We are also interested in XEMBED_FOCUS_IN events */
    if (response_type == XCB_CLIENT_MESSAGE) {
        xcb_client_message_event_t *cme = (xcb_client_message_event_t *)event;
        if (cme->type == connection->atom(QXcbAtom::_XEMBED)
            && cme->data.data32[1] == XEMBED_FOCUS_IN)
            return true;
    }

    return false;
}

void QXcbWindow::handleFocusOutEvent(const xcb_focus_out_event_t *)
{
    connection()->setFocusWindow(0);
    // Do not set the active window to 0 if there is a FocusIn coming.
    // There is however no equivalent for XPutBackEvent so register a
    // callback for QXcbConnection instead.
    connection()->addPeekFunc(focusInPeeker);
}

void QXcbWindow::updateSyncRequestCounter()
{
    if (m_usingSyncProtocol && (m_syncValue.lo != 0 || m_syncValue.hi != 0)) {
        Q_XCB_CALL(xcb_sync_set_counter(xcb_connection(), m_syncCounter, m_syncValue));
        connection()->sync();

        m_syncValue.lo = 0;
        m_syncValue.hi = 0;
    }
}

bool QXcbWindow::setKeyboardGrabEnabled(bool grab)
{
    if (!grab) {
        xcb_ungrab_keyboard(xcb_connection(), XCB_TIME_CURRENT_TIME);
        return true;
    }
    xcb_grab_keyboard_cookie_t cookie = xcb_grab_keyboard(xcb_connection(), false,
                                                          m_window, XCB_TIME_CURRENT_TIME,
                                                          XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_keyboard_reply_t *reply = xcb_grab_keyboard_reply(xcb_connection(), cookie, NULL);
    bool result = !(!reply || reply->status != XCB_GRAB_STATUS_SUCCESS);
    free(reply);
    return result;
}

bool QXcbWindow::setMouseGrabEnabled(bool grab)
{
    if (!grab) {
        xcb_ungrab_pointer(xcb_connection(), XCB_TIME_CURRENT_TIME);
        return true;
    }
    xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(xcb_connection(), false, m_window,
                                                        (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
                                                         | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_ENTER_WINDOW
                                                         | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION),
                                                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                                        XCB_WINDOW_NONE, XCB_CURSOR_NONE,
                                                        XCB_TIME_CURRENT_TIME);
    xcb_grab_pointer_reply_t *reply = xcb_grab_pointer_reply(xcb_connection(), cookie, NULL);
    bool result = !(!reply || reply->status != XCB_GRAB_STATUS_SUCCESS);
    free(reply);
    return result;
}

void QXcbWindow::setCursor(xcb_cursor_t cursor)
{
    xcb_change_window_attributes(xcb_connection(), m_window, XCB_CW_CURSOR, &cursor);
    xcb_flush(xcb_connection());
}

void QXcbWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_embedded && !event->spontaneous()) {
            QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);
            switch (focusEvent->reason()) {
            case Qt::TabFocusReason:
            case Qt::BacktabFocusReason:
                {
                const QXcbWindow *container =
                    static_cast<const QXcbWindow *>(parent());
                sendXEmbedMessage(container->xcb_window(),
                                  focusEvent->reason() == Qt::TabFocusReason ?
                                  XEMBED_FOCUS_NEXT : XEMBED_FOCUS_PREV);
                event->accept();
                }
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    QPlatformWindow::windowEvent(event);
}

bool QXcbWindow::startSystemResize(const QPoint &pos, Qt::Corner corner)
{
    const xcb_atom_t moveResize = connection()->atom(QXcbAtom::_NET_WM_MOVERESIZE);
    if (!connection()->wmSupport()->isSupportedByWM(moveResize))
        return false;
    xcb_client_message_event_t xev;
    xev.response_type = XCB_CLIENT_MESSAGE;
    xev.type = moveResize;
    xev.window = xcb_window();
    xev.format = 32;
    const QPoint globalPos = window()->mapToGlobal(pos);
    xev.data.data32[0] = globalPos.x();
    xev.data.data32[1] = globalPos.y();
    const bool bottom = corner == Qt::BottomRightCorner || corner == Qt::BottomLeftCorner;
    const bool left = corner == Qt::BottomLeftCorner || corner == Qt::TopLeftCorner;
    if (bottom)
        xev.data.data32[2] = left ? 6 : 4; // bottomleft/bottomright
    else
        xev.data.data32[2] = left ? 0 : 2; // topleft/topright
    xev.data.data32[3] = XCB_BUTTON_INDEX_1;
    xev.data.data32[4] = 0;
    xcb_ungrab_pointer(connection()->xcb_connection(), XCB_CURRENT_TIME);
    xcb_send_event(connection()->xcb_connection(), false, m_screen->root(),
                   XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                   (const char *)&xev);
    return true;
}

// Sends an XEmbed message.
void QXcbWindow::sendXEmbedMessage(xcb_window_t window, quint32 message,
                                   quint32 detail, quint32 data1, quint32 data2)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = window;
    event.type = atom(QXcbAtom::_XEMBED);
    event.data.data32[0] = connection()->time();
    event.data.data32[1] = message;
    event.data.data32[2] = detail;
    event.data.data32[3] = data1;
    event.data.data32[4] = data2;
    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, window,
                              XCB_EVENT_MASK_NO_EVENT, (const char *)&event));
}

static bool activeWindowChangeQueued(const QWindow *window)
{
    /* Check from window system event queue if the next queued activation
     * targets a window other than @window.
     */
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *systemEvent =
        static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>
        (QWindowSystemInterfacePrivate::peekWindowSystemEvent(QWindowSystemInterfacePrivate::ActivatedWindow));
    return systemEvent && systemEvent->activated != window;
}

void QXcbWindow::handleXEmbedMessage(const xcb_client_message_event_t *event)
{
    connection()->setTime(event->data.data32[0]);
    switch (event->data.data32[1]) {
    case XEMBED_WINDOW_ACTIVATE:
    case XEMBED_WINDOW_DEACTIVATE:
    case XEMBED_EMBEDDED_NOTIFY:
        break;
    case XEMBED_FOCUS_IN:
        Qt::FocusReason reason;
        switch (event->data.data32[2]) {
        case XEMBED_FOCUS_FIRST:
            reason = Qt::TabFocusReason;
            break;
        case XEMBED_FOCUS_LAST:
            reason = Qt::BacktabFocusReason;
            break;
        case XEMBED_FOCUS_CURRENT:
        default:
            reason = Qt::OtherFocusReason;
            break;
        }
        connection()->setFocusWindow(static_cast<QXcbWindow*>(window()->handle()));
        QWindowSystemInterface::handleWindowActivated(window(), reason);
        break;
    case XEMBED_FOCUS_OUT:
        if (window() == QGuiApplication::focusWindow()
            && !activeWindowChangeQueued(window())) {
            connection()->setFocusWindow(0);
            QWindowSystemInterface::handleWindowActivated(0);
        }
        break;
    }
}

static inline xcb_rectangle_t qRectToXCBRectangle(const QRect &r)
{
    xcb_rectangle_t result;
    result.x = qMax(SHRT_MIN, r.x());
    result.y = qMax(SHRT_MIN, r.y());
    result.width = qMin((int)USHRT_MAX, r.width());
    result.height = qMin((int)USHRT_MAX, r.height());
    return result;
}

void QXcbWindow::setOpacity(qreal level)
{
    if (!m_window)
        return;

    quint32 value = qRound64(qBound(qreal(0), level, qreal(1)) * 0xffffffff);

    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_WINDOW_OPACITY),
                                   XCB_ATOM_CARDINAL,
                                   32,
                                   1,
                                   (uchar *)&value));
}

void QXcbWindow::setMask(const QRegion &region)
{
    if (!connection()->hasXShape())
        return;
    if (region.isEmpty()) {
        xcb_shape_mask(connection()->xcb_connection(), XCB_SHAPE_SO_SET,
                       XCB_SHAPE_SK_BOUNDING, xcb_window(), 0, 0, XCB_NONE);
    } else {
        QVector<xcb_rectangle_t> rects;
        foreach (const QRect &r, region.rects())
            rects.push_back(qRectToXCBRectangle(r));
        xcb_shape_rectangles(connection()->xcb_connection(), XCB_SHAPE_SO_SET,
                             XCB_SHAPE_SK_BOUNDING, XCB_CLIP_ORDERING_UNSORTED,
                             xcb_window(), 0, 0, rects.size(), &rects[0]);
    }
}

void QXcbWindow::setAlertState(bool enabled)
{
    if (m_alertState == enabled)
        return;
    const NetWmStates oldState = netWmStates();
    m_alertState = enabled;
    if (enabled) {
        setNetWmStates(oldState | NetWmStateDemandsAttention);
    } else {
        setNetWmStates(oldState & ~NetWmStateDemandsAttention);
    }
}

QT_END_NAMESPACE
