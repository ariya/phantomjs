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

#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/QDebug>

#include "qxcbconnection.h"
#include "qxcbkeyboard.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbclipboard.h"
#include "qxcbdrag.h"
#include "qxcbwmsupport.h"
#include "qxcbnativeinterface.h"
#include "qxcbintegration.h"
#include "qxcbsystemtraytracker.h"

#include <QSocketNotifier>
#include <QAbstractEventDispatcher>
#include <QTimer>
#include <QByteArray>

#include <algorithm>

#include <dlfcn.h>
#include <stdio.h>
#include <errno.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xfixes.h>

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlibint.h>
#endif

#if defined(XCB_USE_XINPUT2) || defined(XCB_USE_XINPUT2_MAEMO)
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#endif

#ifdef XCB_USE_RENDER
#include <xcb/render.h>
#endif

#if defined(XCB_HAS_XCB_GLX)
#include <xcb/glx.h>
#endif

#ifdef XCB_USE_EGL //don't pull in eglext prototypes
#include <EGL/egl.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef XCB_USE_XLIB
static const char * const xcbConnectionErrors[] = {
    "No error", /* Error 0 */
    "I/O error", /* XCB_CONN_ERROR */
    "Unsupported extension used", /* XCB_CONN_CLOSED_EXT_NOTSUPPORTED */
    "Out of memory", /* XCB_CONN_CLOSED_MEM_INSUFFICIENT */
    "Maximum allowed requested length exceeded", /* XCB_CONN_CLOSED_REQ_LEN_EXCEED */
    "Failed to parse display string", /* XCB_CONN_CLOSED_PARSE_ERR */
    "No such screen on display", /* XCB_CONN_CLOSED_INVALID_SCREEN */
    "Error during FD passing" /* XCB_CONN_CLOSED_FDPASSING_FAILED */
};

static int nullErrorHandler(Display *, XErrorEvent *)
{
    return 0;
}

static int ioErrorHandler(Display *dpy)
{
    xcb_connection_t *conn = XGetXCBConnection(dpy);
    if (conn != NULL) {
        /* Print a message with a textual description of the error */
        int code = xcb_connection_has_error(conn);
        const char *str = "Unknown error";
        int arrayLength = sizeof(xcbConnectionErrors) / sizeof(xcbConnectionErrors[0]);
        if (code >= 0 && code < arrayLength)
            str = xcbConnectionErrors[code];

        qWarning("The X11 connection broke: %s (code %d)", str, code);
    }
    return _XDefaultIOError(dpy);
}
#endif

QXcbScreen* QXcbConnection::findOrCreateScreen(QList<QXcbScreen *>& newScreens,
    int screenNumber, xcb_screen_t* xcbScreen, xcb_randr_get_output_info_reply_t *output)
{
    QString name;
    if (output)
        name = QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output),
                xcb_randr_get_output_info_name_length(output));
    else {
        QByteArray displayName = m_displayName;
        int dotPos = displayName.lastIndexOf('.');
        if (dotPos != -1)
            displayName.truncate(dotPos);
        name = displayName + QLatin1Char('.') + QString::number(screenNumber);
    }
    foreach (QXcbScreen* scr, m_screens)
        if (scr->name() == name && scr->root() == xcbScreen->root)
            return scr;
    QXcbScreen *ret = new QXcbScreen(this, xcbScreen, output, name, screenNumber);
    newScreens << ret;
    return ret;
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted ones
*/
void QXcbConnection::updateScreens()
{
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(m_setup);
    int screenNumber = 0;       // index of this QScreen in QGuiApplication::screens()
    int xcbScreenNumber = 0;    // screen number in the xcb sense
    QSet<QXcbScreen *> activeScreens;
    QList<QXcbScreen *> newScreens;
    QXcbScreen* primaryScreen = NULL;
    while (it.rem) {
        // Each "screen" in xcb terminology is a virtual desktop,
        // potentially a collection of separate juxtaposed monitors.
        // But we want a separate QScreen for each output (e.g. DVI-I-1, VGA-1, etc.)
        // which will become virtual siblings.
        xcb_screen_t *xcbScreen = it.data;
        QList<QPlatformScreen *> siblings;
        int outputCount = 0;
        if (has_randr_extension) {
            xcb_generic_error_t *error = NULL;
            xcb_randr_get_output_primary_cookie_t primaryCookie =
                xcb_randr_get_output_primary(xcb_connection(), xcbScreen->root);
            xcb_randr_get_screen_resources_cookie_t resourcesCookie =
                xcb_randr_get_screen_resources(xcb_connection(), xcbScreen->root);
            xcb_randr_get_output_primary_reply_t *primary =
                    xcb_randr_get_output_primary_reply(xcb_connection(), primaryCookie, &error);
            if (!primary || error) {
                qWarning("QXcbConnection: Failed to get the primary output of the screen");
                free(error);
            } else {
                xcb_randr_get_screen_resources_reply_t *resources =
                        xcb_randr_get_screen_resources_reply(xcb_connection(), resourcesCookie, &error);
                if (!resources || error) {
                    qWarning("QXcbConnection: Failed to get the screen resources");
                    free(error);
                } else {
                    xcb_timestamp_t timestamp = resources->config_timestamp;
                    outputCount = xcb_randr_get_screen_resources_outputs_length(resources);
                    xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(resources);

                    for (int i = 0; i < outputCount; i++) {
                        xcb_randr_get_output_info_reply_t *output =
                                xcb_randr_get_output_info_reply(xcb_connection(),
                                    xcb_randr_get_output_info_unchecked(xcb_connection(), outputs[i], timestamp), NULL);
                        if (output == NULL)
                            continue;

#ifdef Q_XCB_DEBUG
                        QString outputName = QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output),
                                                               xcb_randr_get_output_info_name_length(output));
#endif

                        if (output->crtc == XCB_NONE) {
#ifdef Q_XCB_DEBUG
                            qDebug("Screen output %s is not connected", qPrintable(outputName));
#endif
                            continue;
                        }

                        QXcbScreen *screen = findOrCreateScreen(newScreens, xcbScreenNumber, xcbScreen, output);
                        siblings << screen;
                        activeScreens << screen;
                        ++screenNumber;
                        // There can be multiple outputs per screen, use either
                        // the first or an exact match.  An exact match isn't
                        // always available if primary->output is XCB_NONE
                        // or currently disconnected output.
                        if (m_primaryScreen == xcbScreenNumber) {
                            if (!primaryScreen || (primary && outputs[i] == primary->output)) {
                                primaryScreen = screen;
                                siblings.prepend(siblings.takeLast());
#ifdef Q_XCB_DEBUG
                                qDebug("Primary output is %d: %s", primary->output, qPrintable(outputName));
#endif
                            }
                        }
                        free(output);
                    }
                }
                free(resources);
            }
            free(primary);
        }
        // If there's no randr extension, or there was some error above, or the screen
        // doesn't have outputs for some other reason (e.g. on VNC or ssh -X), just assume there is one screen.
        if (outputCount == 0) {
#ifdef Q_XCB_DEBUG
                qDebug("Found a screen with zero outputs");
#endif
            QXcbScreen *screen = findOrCreateScreen(newScreens, xcbScreenNumber, xcbScreen);
            siblings << screen;
            activeScreens << screen;
            if (!primaryScreen)
                primaryScreen = screen;
            ++screenNumber;
        }
        foreach (QPlatformScreen* s, siblings)
            ((QXcbScreen*)s)->setVirtualSiblings(siblings);
        xcb_screen_next(&it);
        ++xcbScreenNumber;
    } // for each xcb screen

    // Now activeScreens is the complete set of screens which are active at this time.
    // Delete any existing screens which are not in activeScreens
    for (int i = m_screens.count() - 1; i >= 0; --i) {
        if (!activeScreens.contains(m_screens[i])) {
            delete m_screens[i];
            m_screens.removeAt(i);
        }
    }

    // Add any new screens, and make sure the primary screen comes first
    // since it is used by QGuiApplication::primaryScreen()
    foreach (QXcbScreen* screen, newScreens) {
        if (screen == primaryScreen)
            m_screens.prepend(screen);
        else
            m_screens.append(screen);
    }

    // Now that they are in the right order, emit the added signals for new screens only
    foreach (QXcbScreen* screen, m_screens)
        if (newScreens.contains(screen))
            ((QXcbIntegration*)QGuiApplicationPrivate::platformIntegration())->screenAdded(screen);
}

QXcbConnection::QXcbConnection(QXcbNativeInterface *nativeInterface, bool canGrabServer, const char *displayName)
    : m_connection(0)
    , m_canGrabServer(canGrabServer)
    , m_primaryScreen(0)
    , m_displayName(displayName ? QByteArray(displayName) : qgetenv("DISPLAY"))
    , m_nativeInterface(nativeInterface)
#ifdef XCB_USE_XINPUT2_MAEMO
    , m_xinputData(0)
#endif
    , xfixes_first_event(0)
    , xrandr_first_event(0)
    , xkb_first_event(0)
    , has_glx_extension(false)
    , has_shape_extension(false)
    , has_randr_extension(false)
    , has_input_shape(false)
    , has_touch_without_mouse_emulation(false)
    , has_xkb(false)
    , debug_xinput_devices(false)
    , debug_xinput(false)
    , m_buttons(0)
    , m_focusWindow(0)
    , m_systemTrayTracker(0)
{
#ifdef XCB_USE_EGL
    EGLNativeDisplayType dpy = EGL_DEFAULT_DISPLAY;
#elif defined(XCB_USE_XLIB)
    Display *dpy;
#endif
#ifdef XCB_USE_XLIB
    dpy = XOpenDisplay(m_displayName.constData());
    if (dpy) {
        m_primaryScreen = DefaultScreen(dpy);
        m_connection = XGetXCBConnection(dpy);
        XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
        XSetErrorHandler(nullErrorHandler);
        XSetIOErrorHandler(ioErrorHandler);
        m_xlib_display = dpy;
    }
#else
    m_connection = xcb_connect(m_displayName.constData(), &m_primaryScreen);
#endif //XCB_USE_XLIB

    if (!m_connection || xcb_connection_has_error(m_connection))
        qFatal("QXcbConnection: Could not connect to display %s", m_displayName.constData());

#ifdef XCB_USE_EGL
    EGLDisplay eglDisplay = eglGetDisplay(dpy);
    m_egl_display = eglDisplay;
    EGLint major, minor;
    m_has_egl = eglInitialize(eglDisplay, &major, &minor);
#endif //XCB_USE_EGL

    m_reader = new QXcbEventReader(this);
    m_reader->start();

    xcb_extension_t *extensions[] = {
        &xcb_shm_id, &xcb_xfixes_id, &xcb_randr_id, &xcb_shape_id, &xcb_sync_id,
#ifndef QT_NO_XKB
        &xcb_xkb_id,
#endif
#ifdef XCB_USE_RENDER
        &xcb_render_id,
#endif
#ifdef XCB_HAS_XCB_GLX
        &xcb_glx_id,
#endif
        0
    };

    for (xcb_extension_t **ext_it = extensions; *ext_it; ++ext_it)
        xcb_prefetch_extension_data (m_connection, *ext_it);

    m_setup = xcb_get_setup(xcb_connection());

    initializeAllAtoms();

    m_time = XCB_CURRENT_TIME;
    m_netWmUserTime = XCB_CURRENT_TIME;

    initializeXRandr();
    updateScreens();

    initializeGLX();
    initializeXFixes();
    initializeXRender();
    m_xi2Enabled = false;
#ifdef XCB_USE_XINPUT2_MAEMO
    initializeXInput2Maemo();
#elif defined(XCB_USE_XINPUT2)
    initializeXInput2();
#endif
    initializeXShape();
    initializeXKB();

    m_wmSupport.reset(new QXcbWMSupport(this));
    m_keyboard = new QXcbKeyboard(this);
#ifndef QT_NO_CLIPBOARD
    m_clipboard = new QXcbClipboard(this);
#endif
#ifndef QT_NO_DRAGANDDROP
    m_drag = new QXcbDrag(this);
#endif

    m_startupId = qgetenv("DESKTOP_STARTUP_ID");
    if (!m_startupId.isNull())
        qunsetenv("DESKTOP_STARTUP_ID");

    sync();
}

QXcbConnection::~QXcbConnection()
{
#ifndef QT_NO_CLIPBOARD
    delete m_clipboard;
#endif
#ifndef QT_NO_DRAGANDDROP
    delete m_drag;
#endif

#ifdef XCB_USE_XINPUT2_MAEMO
    finalizeXInput2Maemo();
#elif defined(XCB_USE_XINPUT2)
    finalizeXInput2();
#endif

    if (m_reader->isRunning()) {
        sendConnectionEvent(QXcbAtom::_QT_CLOSE_CONNECTION);
        m_reader->wait();
    }

    delete m_reader;

    // Delete screens in reverse order to avoid crash in case of multiple screens
    while (!m_screens.isEmpty())
        delete m_screens.takeLast();

#ifdef XCB_USE_EGL
    if (m_has_egl)
        eglTerminate(m_egl_display);
#endif //XCB_USE_EGL

#ifdef XCB_USE_XLIB
    XCloseDisplay((Display *)m_xlib_display);
#else
    xcb_disconnect(xcb_connection());
#endif

    delete m_keyboard;
}

void QXcbConnection::addWindowEventListener(xcb_window_t id, QXcbWindowEventListener *eventListener)
{
    m_mapper.insert(id, eventListener);
}

void QXcbConnection::removeWindowEventListener(xcb_window_t id)
{
    m_mapper.remove(id);
}

QXcbWindowEventListener *QXcbConnection::windowEventListenerFromId(xcb_window_t id)
{
    return m_mapper.value(id, 0);
}

QXcbWindow *QXcbConnection::platformWindowFromId(xcb_window_t id)
{
    QXcbWindowEventListener *listener = m_mapper.value(id, 0);
    if (listener)
        return listener->toWindow();
    return 0;
}

#define HANDLE_PLATFORM_WINDOW_EVENT(event_t, windowMember, handler) \
{ \
    event_t *e = (event_t *)event; \
    if (QXcbWindowEventListener *eventListener = windowEventListenerFromId(e->windowMember))  { \
        handled = eventListener->handleGenericEvent(event, &result); \
        if (!handled) \
            eventListener->handler(e); \
    } \
} \
break;

#define HANDLE_KEYBOARD_EVENT(event_t, handler) \
{ \
    event_t *e = (event_t *)event; \
    if (QXcbWindowEventListener *eventListener = windowEventListenerFromId(e->event)) { \
        handled = eventListener->handleGenericEvent(event, &result); \
        if (!handled) \
            m_keyboard->handler(m_focusWindow ? m_focusWindow : eventListener, e); \
    } \
} \
break;

//#define XCB_EVENT_DEBUG

void printXcbEvent(const char *message, xcb_generic_event_t *event)
{
#ifdef XCB_EVENT_DEBUG
#define PRINT_XCB_EVENT(ev) \
    case ev: \
        qDebug("QXcbConnection: %s: %d - %s - sequence: %d", message, int(ev), #ev, event->sequence); \
        break;

    switch (event->response_type & ~0x80) {
    PRINT_XCB_EVENT(XCB_KEY_PRESS);
    PRINT_XCB_EVENT(XCB_KEY_RELEASE);
    PRINT_XCB_EVENT(XCB_BUTTON_PRESS);
    PRINT_XCB_EVENT(XCB_BUTTON_RELEASE);
    PRINT_XCB_EVENT(XCB_MOTION_NOTIFY);
    PRINT_XCB_EVENT(XCB_ENTER_NOTIFY);
    PRINT_XCB_EVENT(XCB_LEAVE_NOTIFY);
    PRINT_XCB_EVENT(XCB_FOCUS_IN);
    PRINT_XCB_EVENT(XCB_FOCUS_OUT);
    PRINT_XCB_EVENT(XCB_KEYMAP_NOTIFY);
    PRINT_XCB_EVENT(XCB_EXPOSE);
    PRINT_XCB_EVENT(XCB_GRAPHICS_EXPOSURE);
    PRINT_XCB_EVENT(XCB_VISIBILITY_NOTIFY);
    PRINT_XCB_EVENT(XCB_CREATE_NOTIFY);
    PRINT_XCB_EVENT(XCB_DESTROY_NOTIFY);
    PRINT_XCB_EVENT(XCB_UNMAP_NOTIFY);
    PRINT_XCB_EVENT(XCB_MAP_NOTIFY);
    PRINT_XCB_EVENT(XCB_MAP_REQUEST);
    PRINT_XCB_EVENT(XCB_REPARENT_NOTIFY);
    PRINT_XCB_EVENT(XCB_CONFIGURE_NOTIFY);
    PRINT_XCB_EVENT(XCB_CONFIGURE_REQUEST);
    PRINT_XCB_EVENT(XCB_GRAVITY_NOTIFY);
    PRINT_XCB_EVENT(XCB_RESIZE_REQUEST);
    PRINT_XCB_EVENT(XCB_CIRCULATE_NOTIFY);
    PRINT_XCB_EVENT(XCB_CIRCULATE_REQUEST);
    PRINT_XCB_EVENT(XCB_PROPERTY_NOTIFY);
    PRINT_XCB_EVENT(XCB_SELECTION_CLEAR);
    PRINT_XCB_EVENT(XCB_SELECTION_REQUEST);
    PRINT_XCB_EVENT(XCB_SELECTION_NOTIFY);
    PRINT_XCB_EVENT(XCB_COLORMAP_NOTIFY);
    PRINT_XCB_EVENT(XCB_CLIENT_MESSAGE);
    default:
        qDebug("QXcbConnection: %s: unknown event - response_type: %d - sequence: %d", message, int(event->response_type & ~0x80), int(event->sequence));
    }
#else
    Q_UNUSED(message);
    Q_UNUSED(event);
#endif
}

const char *xcb_errors[] =
{
    "Success",
    "BadRequest",
    "BadValue",
    "BadWindow",
    "BadPixmap",
    "BadAtom",
    "BadCursor",
    "BadFont",
    "BadMatch",
    "BadDrawable",
    "BadAccess",
    "BadAlloc",
    "BadColor",
    "BadGC",
    "BadIDChoice",
    "BadName",
    "BadLength",
    "BadImplementation",
    "Unknown"
};

const char *xcb_protocol_request_codes[] =
{
    "Null",
    "CreateWindow",
    "ChangeWindowAttributes",
    "GetWindowAttributes",
    "DestroyWindow",
    "DestroySubwindows",
    "ChangeSaveSet",
    "ReparentWindow",
    "MapWindow",
    "MapSubwindows",
    "UnmapWindow",
    "UnmapSubwindows",
    "ConfigureWindow",
    "CirculateWindow",
    "GetGeometry",
    "QueryTree",
    "InternAtom",
    "GetAtomName",
    "ChangeProperty",
    "DeleteProperty",
    "GetProperty",
    "ListProperties",
    "SetSelectionOwner",
    "GetSelectionOwner",
    "ConvertSelection",
    "SendEvent",
    "GrabPointer",
    "UngrabPointer",
    "GrabButton",
    "UngrabButton",
    "ChangeActivePointerGrab",
    "GrabKeyboard",
    "UngrabKeyboard",
    "GrabKey",
    "UngrabKey",
    "AllowEvents",
    "GrabServer",
    "UngrabServer",
    "QueryPointer",
    "GetMotionEvents",
    "TranslateCoords",
    "WarpPointer",
    "SetInputFocus",
    "GetInputFocus",
    "QueryKeymap",
    "OpenFont",
    "CloseFont",
    "QueryFont",
    "QueryTextExtents",
    "ListFonts",
    "ListFontsWithInfo",
    "SetFontPath",
    "GetFontPath",
    "CreatePixmap",
    "FreePixmap",
    "CreateGC",
    "ChangeGC",
    "CopyGC",
    "SetDashes",
    "SetClipRectangles",
    "FreeGC",
    "ClearArea",
    "CopyArea",
    "CopyPlane",
    "PolyPoint",
    "PolyLine",
    "PolySegment",
    "PolyRectangle",
    "PolyArc",
    "FillPoly",
    "PolyFillRectangle",
    "PolyFillArc",
    "PutImage",
    "GetImage",
    "PolyText8",
    "PolyText16",
    "ImageText8",
    "ImageText16",
    "CreateColormap",
    "FreeColormap",
    "CopyColormapAndFree",
    "InstallColormap",
    "UninstallColormap",
    "ListInstalledColormaps",
    "AllocColor",
    "AllocNamedColor",
    "AllocColorCells",
    "AllocColorPlanes",
    "FreeColors",
    "StoreColors",
    "StoreNamedColor",
    "QueryColors",
    "LookupColor",
    "CreateCursor",
    "CreateGlyphCursor",
    "FreeCursor",
    "RecolorCursor",
    "QueryBestSize",
    "QueryExtension",
    "ListExtensions",
    "ChangeKeyboardMapping",
    "GetKeyboardMapping",
    "ChangeKeyboardControl",
    "GetKeyboardControl",
    "Bell",
    "ChangePointerControl",
    "GetPointerControl",
    "SetScreenSaver",
    "GetScreenSaver",
    "ChangeHosts",
    "ListHosts",
    "SetAccessControl",
    "SetCloseDownMode",
    "KillClient",
    "RotateProperties",
    "ForceScreenSaver",
    "SetPointerMapping",
    "GetPointerMapping",
    "SetModifierMapping",
    "GetModifierMapping",
    "Unknown"
};

#ifdef Q_XCB_DEBUG
void QXcbConnection::log(const char *file, int line, int sequence)
{
    QMutexLocker locker(&m_callLogMutex);
    CallInfo info;
    info.sequence = sequence;
    info.file = file;
    info.line = line;
    m_callLog << info;
}
#endif

void QXcbConnection::handleXcbError(xcb_generic_error_t *error)
{
    long result = 0;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    if (dispatcher && dispatcher->filterNativeEvent(m_nativeInterface->genericEventFilterType(), error, &result))
        return;

    uint clamped_error_code = qMin<uint>(error->error_code, (sizeof(xcb_errors) / sizeof(xcb_errors[0])) - 1);
    uint clamped_major_code = qMin<uint>(error->major_code, (sizeof(xcb_protocol_request_codes) / sizeof(xcb_protocol_request_codes[0])) - 1);

    qWarning("QXcbConnection: XCB error: %d (%s), sequence: %d, resource id: %d, major code: %d (%s), minor code: %d",
           int(error->error_code), xcb_errors[clamped_error_code],
           int(error->sequence), int(error->resource_id),
           int(error->major_code), xcb_protocol_request_codes[clamped_major_code],
           int(error->minor_code));
#ifdef Q_XCB_DEBUG
    QMutexLocker locker(&m_callLogMutex);
    int i = 0;
    for (; i < m_callLog.size(); ++i) {
        if (m_callLog.at(i).sequence == error->sequence) {
            qDebug("Caused by: %s:%d", qPrintable(m_callLog.at(i).file), m_callLog.at(i).line);
            break;
        } else if (m_callLog.at(i).sequence > error->sequence) {
            qDebug("Caused some time before: %s:%d", qPrintable(m_callLog.at(i).file), m_callLog.at(i).line);
            if (i > 0)
                qDebug("and after: %s:%d", qPrintable(m_callLog.at(i-1).file), m_callLog.at(i-1).line);
            break;
        }
    }
    if (i == m_callLog.size() && !m_callLog.isEmpty())
        qDebug("Caused some time after: %s:%d", qPrintable(m_callLog.first().file), m_callLog.first().line);
#endif
}

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & XCB_BUTTON_MASK_1)
        ret |= Qt::LeftButton;
    if (s & XCB_BUTTON_MASK_2)
        ret |= Qt::MidButton;
    if (s & XCB_BUTTON_MASK_3)
        ret |= Qt::RightButton;
    return ret;
}

static Qt::MouseButton translateMouseButton(xcb_button_t s)
{
    switch (s) {
    case 1: return Qt::LeftButton;
    case 2: return Qt::MidButton;
    case 3: return Qt::RightButton;
    // Button values 4-7 were already handled as Wheel events, and won't occur here.
    case 8: return Qt::BackButton;      // Also known as Qt::ExtraButton1
    case 9: return Qt::ForwardButton;   // Also known as Qt::ExtraButton2
    case 10: return Qt::ExtraButton3;
    case 11: return Qt::ExtraButton4;
    case 12: return Qt::ExtraButton5;
    case 13: return Qt::ExtraButton6;
    case 14: return Qt::ExtraButton7;
    case 15: return Qt::ExtraButton8;
    case 16: return Qt::ExtraButton9;
    case 17: return Qt::ExtraButton10;
    case 18: return Qt::ExtraButton11;
    case 19: return Qt::ExtraButton12;
    case 20: return Qt::ExtraButton13;
    case 21: return Qt::ExtraButton14;
    case 22: return Qt::ExtraButton15;
    case 23: return Qt::ExtraButton16;
    case 24: return Qt::ExtraButton17;
    case 25: return Qt::ExtraButton18;
    case 26: return Qt::ExtraButton19;
    case 27: return Qt::ExtraButton20;
    case 28: return Qt::ExtraButton21;
    case 29: return Qt::ExtraButton22;
    case 30: return Qt::ExtraButton23;
    case 31: return Qt::ExtraButton24;
    default: return Qt::NoButton;
    }
}

void QXcbConnection::handleButtonPress(xcb_generic_event_t *ev)
{
    xcb_button_press_event_t *event = (xcb_button_press_event_t *)ev;

    // the event explicitly contains the state of the three first buttons,
    // the rest we need to manage ourselves
    m_buttons = (m_buttons & ~0x7) | translateMouseButtons(event->state);
    m_buttons |= translateMouseButton(event->detail);
    if (Q_UNLIKELY(debug_xinput))
        qDebug("xcb: pressed mouse button %d, button state %X", event->detail, static_cast<unsigned int>(m_buttons));
}

void QXcbConnection::handleButtonRelease(xcb_generic_event_t *ev)
{
    xcb_button_release_event_t *event = (xcb_button_release_event_t *)ev;

    // the event explicitly contains the state of the three first buttons,
    // the rest we need to manage ourselves
    m_buttons = (m_buttons & ~0x7) | translateMouseButtons(event->state);
    m_buttons &= ~translateMouseButton(event->detail);
    if (Q_UNLIKELY(debug_xinput))
        qDebug("xcb: released mouse button %d, button state %X", event->detail, static_cast<unsigned int>(m_buttons));
}

#ifndef QT_NO_XKB
namespace {
    typedef union {
        /* All XKB events share these fields. */
        struct {
            uint8_t response_type;
            uint8_t xkbType;
            uint16_t sequence;
            xcb_timestamp_t time;
            uint8_t deviceID;
        } any;
        xcb_xkb_new_keyboard_notify_event_t new_keyboard_notify;
        xcb_xkb_map_notify_event_t map_notify;
        xcb_xkb_state_notify_event_t state_notify;
    } _xkb_event;
}
#endif

void QXcbConnection::handleXcbEvent(xcb_generic_event_t *event)
{
#ifdef Q_XCB_DEBUG
    {
        QMutexLocker locker(&m_callLogMutex);
        int i = 0;
        for (; i < m_callLog.size(); ++i)
            if (m_callLog.at(i).sequence >= event->sequence)
                break;
        m_callLog.remove(0, i);
    }
#endif

    long result = 0;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    bool handled = dispatcher && dispatcher->filterNativeEvent(m_nativeInterface->genericEventFilterType(), event, &result);

    uint response_type = event->response_type & ~0x80;

    if (!handled) {
        switch (response_type) {
        case XCB_EXPOSE:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_expose_event_t, window, handleExposeEvent);
        case XCB_BUTTON_PRESS:
            m_keyboard->updateXKBStateFromCore(((xcb_button_press_event_t *)event)->state);
            handleButtonPress(event);
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_button_press_event_t, event, handleButtonPressEvent);
        case XCB_BUTTON_RELEASE:
            m_keyboard->updateXKBStateFromCore(((xcb_button_release_event_t *)event)->state);
            handleButtonRelease(event);
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_button_release_event_t, event, handleButtonReleaseEvent);
        case XCB_MOTION_NOTIFY:
            if (Q_UNLIKELY(debug_xinput)) {
                xcb_motion_notify_event_t *mev = (xcb_motion_notify_event_t *)event;
                qDebug("xcb: moved mouse to %4d, %4d; button state %X", mev->event_x, mev->event_y, static_cast<unsigned int>(m_buttons));
            }
            m_keyboard->updateXKBStateFromCore(((xcb_motion_notify_event_t *)event)->state);
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_motion_notify_event_t, event, handleMotionNotifyEvent);
        case XCB_CONFIGURE_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_configure_notify_event_t, event, handleConfigureNotifyEvent);
        case XCB_MAP_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_map_notify_event_t, event, handleMapNotifyEvent);
        case XCB_UNMAP_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_unmap_notify_event_t, event, handleUnmapNotifyEvent);
        case XCB_DESTROY_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_destroy_notify_event_t, event, handleDestroyNotifyEvent);
        case XCB_CLIENT_MESSAGE:
            handleClientMessageEvent((xcb_client_message_event_t *)event);
            break;
        case XCB_ENTER_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_enter_notify_event_t, event, handleEnterNotifyEvent);
        case XCB_LEAVE_NOTIFY:
            m_keyboard->updateXKBStateFromCore(((xcb_leave_notify_event_t *)event)->state);
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_leave_notify_event_t, event, handleLeaveNotifyEvent);
        case XCB_FOCUS_IN:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_focus_in_event_t, event, handleFocusInEvent);
        case XCB_FOCUS_OUT:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_focus_out_event_t, event, handleFocusOutEvent);
        case XCB_KEY_PRESS:
            m_keyboard->updateXKBStateFromCore(((xcb_key_press_event_t *)event)->state);
            HANDLE_KEYBOARD_EVENT(xcb_key_press_event_t, handleKeyPressEvent);
        case XCB_KEY_RELEASE:
            m_keyboard->updateXKBStateFromCore(((xcb_key_release_event_t *)event)->state);
            HANDLE_KEYBOARD_EVENT(xcb_key_release_event_t, handleKeyReleaseEvent);
        case XCB_MAPPING_NOTIFY:
            m_keyboard->handleMappingNotifyEvent((xcb_mapping_notify_event_t *)event);
            break;
        case XCB_SELECTION_REQUEST:
        {
            xcb_selection_request_event_t *sr = (xcb_selection_request_event_t *)event;
#ifndef QT_NO_DRAGANDDROP
            if (sr->selection == atom(QXcbAtom::XdndSelection))
                m_drag->handleSelectionRequest(sr);
            else
#endif
            {
#ifndef QT_NO_CLIPBOARD
                m_clipboard->handleSelectionRequest(sr);
#endif
            }
            break;
        }
        case XCB_SELECTION_CLEAR:
            setTime(((xcb_selection_clear_event_t *)event)->time);
#ifndef QT_NO_CLIPBOARD
            m_clipboard->handleSelectionClearRequest((xcb_selection_clear_event_t *)event);
#endif
            handled = true;
            break;
        case XCB_SELECTION_NOTIFY:
            setTime(((xcb_selection_notify_event_t *)event)->time);
            handled = false;
            break;
        case XCB_PROPERTY_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_property_notify_event_t, window, handlePropertyNotifyEvent);
            break;
#ifdef XCB_USE_XINPUT2_MAEMO
        case GenericEvent:
            handleGenericEventMaemo((xcb_ge_event_t*)event);
            break;
#elif defined(XCB_USE_XINPUT2)
        case GenericEvent:
            if (m_xi2Enabled)
                xi2HandleEvent(reinterpret_cast<xcb_ge_event_t *>(event));
            break;
#endif
        default:
            handled = false;
            break;
        }
    }

    if (!handled) {
        if (response_type == xfixes_first_event + XCB_XFIXES_SELECTION_NOTIFY) {
            setTime(((xcb_xfixes_selection_notify_event_t *)event)->timestamp);
#ifndef QT_NO_CLIPBOARD
            m_clipboard->handleXFixesSelectionRequest((xcb_xfixes_selection_notify_event_t *)event);
#endif
            handled = true;
        } else if (has_randr_extension && response_type == xrandr_first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
            updateScreens();
            xcb_randr_screen_change_notify_event_t *change_event = (xcb_randr_screen_change_notify_event_t *)event;
            foreach (QXcbScreen *s, m_screens) {
                if (s->root() == change_event->root ) {
                    s->handleScreenChange(change_event);
                    s->updateRefreshRate();
                }
            }
            handled = true;
#ifndef QT_NO_XKB
        } else if (response_type == xkb_first_event) { // https://bugs.freedesktop.org/show_bug.cgi?id=51295
            _xkb_event *xkb_event = reinterpret_cast<_xkb_event *>(event);
            if (xkb_event->any.deviceID == m_keyboard->coreDeviceId()) {
                switch (xkb_event->any.xkbType) {
                    // XkbNewKkdNotify and XkbMapNotify together capture all sorts of keymap
                    // updates (e.g. xmodmap, xkbcomp, setxkbmap), with minimal redundent recompilations.
                    case XCB_XKB_STATE_NOTIFY:
                        m_keyboard->updateXKBState(&xkb_event->state_notify);
                        handled = true;
                        break;
                    case XCB_XKB_MAP_NOTIFY:
                        m_keyboard->handleMappingNotifyEvent(&xkb_event->map_notify);
                        handled = true;
                        break;
                    case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                        xcb_xkb_new_keyboard_notify_event_t *ev = &xkb_event->new_keyboard_notify;
                        if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES)
                            m_keyboard->updateKeymap();
                        break;
                    }
                    default:
                        break;
                }
            }
#endif
        }
    }

#ifdef XCB_USE_XLIB
    if (!handled) {
        // Check if a custom XEvent constructor was registered in xlib for this event type, and call it discarding the constructed XEvent if any.
        // XESetWireToEvent might be used by libraries to intercept messages from the X server e.g. the OpenGL lib waiting for DRI2 events.
        Display *xdisplay = (Display *)m_xlib_display;
        XLockDisplay(xdisplay);
        Bool (*proc)(Display*, XEvent*, xEvent*) = XESetWireToEvent(xdisplay, response_type, 0);
        if (proc) {
            XESetWireToEvent(xdisplay, response_type, proc);
            XEvent dummy;
            event->sequence = LastKnownRequestProcessed(m_xlib_display);
            proc(xdisplay, &dummy, (xEvent*)event);
        }
        XUnlockDisplay(xdisplay);
    }
#endif

    if (handled)
        printXcbEvent("Handled XCB event", event);
    else
        printXcbEvent("Unhandled XCB event", event);
}

void QXcbConnection::addPeekFunc(PeekFunc f)
{
    m_peekFuncs.append(f);
}

QXcbEventReader::QXcbEventReader(QXcbConnection *connection)
    : m_connection(connection)
    , m_xcb_poll_for_queued_event(0)
{
#ifdef RTLD_DEFAULT
    m_xcb_poll_for_queued_event = (XcbPollForQueuedEventFunctionPointer)dlsym(RTLD_DEFAULT, "xcb_poll_for_queued_event");
#endif

#ifdef Q_XCB_DEBUG
    if (m_xcb_poll_for_queued_event)
        qDebug("Using threaded event reader with xcb_poll_for_queued_event");
#endif
}

void QXcbEventReader::start()
{
    if (m_xcb_poll_for_queued_event) {
        connect(this, SIGNAL(eventPending()), m_connection, SLOT(processXcbEvents()), Qt::QueuedConnection);
        connect(this, SIGNAL(finished()), m_connection, SLOT(processXcbEvents()));
        QThread::start();
    } else {
        // Must be done after we have an event-dispatcher. By posting a method invocation
        // we are sure that by the time the method is called we have an event-dispatcher.
        QMetaObject::invokeMethod(this, "registerForEvents", Qt::QueuedConnection);
    }
}

void QXcbEventReader::registerForEvents()
{
    QSocketNotifier *notifier = new QSocketNotifier(xcb_get_file_descriptor(m_connection->xcb_connection()), QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), m_connection, SLOT(processXcbEvents()));

    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::eventDispatcher;
    connect(dispatcher, SIGNAL(aboutToBlock()), m_connection, SLOT(processXcbEvents()));
    connect(dispatcher, SIGNAL(awake()), m_connection, SLOT(processXcbEvents()));
}

void QXcbEventReader::run()
{
    xcb_generic_event_t *event;
    while (m_connection && (event = xcb_wait_for_event(m_connection->xcb_connection()))) {
        m_mutex.lock();
        addEvent(event);
        while (m_connection && (event = m_xcb_poll_for_queued_event(m_connection->xcb_connection())))
            addEvent(event);
        m_mutex.unlock();
        emit eventPending();
    }

    m_mutex.lock();
    for (int i = 0; i < m_events.size(); ++i)
        free(m_events.at(i));
    m_events.clear();
    m_mutex.unlock();
}

void QXcbEventReader::addEvent(xcb_generic_event_t *event)
{
    if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE
        && ((xcb_client_message_event_t *)event)->type == m_connection->atom(QXcbAtom::_QT_CLOSE_CONNECTION))
        m_connection = 0;
    m_events << event;
}

QXcbEventArray *QXcbEventReader::lock()
{
    m_mutex.lock();
    if (!m_xcb_poll_for_queued_event) {
        while (xcb_generic_event_t *event = xcb_poll_for_event(m_connection->xcb_connection()))
            m_events << event;
    }
    return &m_events;
}

void QXcbEventReader::unlock()
{
    m_mutex.unlock();
}

void QXcbConnection::setFocusWindow(QXcbWindow *w)
{
    m_focusWindow = w;
}

void QXcbConnection::grabServer()
{
    if (m_canGrabServer)
        xcb_grab_server(m_connection);
}

void QXcbConnection::ungrabServer()
{
    if (m_canGrabServer)
        xcb_ungrab_server(m_connection);
}

void QXcbConnection::sendConnectionEvent(QXcbAtom::Atom a, uint id)
{
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    const xcb_window_t eventListener = xcb_generate_id(m_connection);
    Q_XCB_CALL(xcb_create_window(m_connection, XCB_COPY_FROM_PARENT,
                                 eventListener, m_screens.at(0)->root(),
                                 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                                 m_screens.at(0)->screen()->root_visual, 0, 0));

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = eventListener;
    event.type = atom(a);
    event.data.data32[0] = id;

    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, eventListener, XCB_EVENT_MASK_NO_EVENT, (const char *)&event));
    Q_XCB_CALL(xcb_destroy_window(m_connection, eventListener));
    xcb_flush(xcb_connection());
}

namespace
{
    class PropertyNotifyEvent {
    public:
        PropertyNotifyEvent(xcb_window_t win, xcb_atom_t property)
            : window(win), type(XCB_PROPERTY_NOTIFY), atom(property) {}
        xcb_window_t window;
        int type;
        xcb_atom_t atom;
        bool checkEvent(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            if ((event->response_type & ~0x80) != type) {
                return false;
            } else {
                xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;
                if ((pn->window == window) && (pn->atom == atom))
                    return true;
            }
            return false;
        }
    };
}

xcb_timestamp_t QXcbConnection::getTimestamp()
{
    // send a dummy event to myself to get the timestamp from X server.
    xcb_window_t root_win = rootWindow();
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_APPEND, root_win, atom(QXcbAtom::CLIP_TEMPORARY),
                        XCB_ATOM_INTEGER, 32, 0, NULL);

    connection()->flush();
    PropertyNotifyEvent checker(root_win, atom(QXcbAtom::CLIP_TEMPORARY));

    xcb_generic_event_t *event = 0;
    // lets keep this inside a loop to avoid a possible race condition, where
    // reader thread has not yet had the time to acquire the mutex in order
    // to add the new set of events to its event queue
    while (!event) {
        connection()->sync();
        event = checkEvent(checker);
    }

    xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;
    xcb_timestamp_t timestamp = pn->time;
    free(event);

    xcb_delete_property(xcb_connection(), root_win, atom(QXcbAtom::CLIP_TEMPORARY));

    return timestamp;
}

xcb_window_t QXcbConnection::rootWindow()
{
    return screens().at(primaryScreen())->root();
}

void QXcbConnection::processXcbEvents()
{
    int connection_error = xcb_connection_has_error(xcb_connection());
    if (connection_error) {
        qWarning("The X11 connection broke (error %d). Did the X11 server die?", connection_error);
        exit(1);
    }

    QXcbEventArray *eventqueue = m_reader->lock();

    for(int i = 0; i < eventqueue->size(); ++i) {
        xcb_generic_event_t *event = eventqueue->at(i);
        if (!event)
            continue;
        QScopedPointer<xcb_generic_event_t, QScopedPointerPodDeleter> eventGuard(event);
        (*eventqueue)[i] = 0;

        uint response_type = event->response_type & ~0x80;

        if (!response_type) {
            handleXcbError((xcb_generic_error_t *)event);
        } else {
            if (response_type == XCB_MOTION_NOTIFY) {
                // compress multiple motion notify events in a row
                // to avoid swamping the event queue
                xcb_generic_event_t *next = eventqueue->value(i+1, 0);
                if (next && (next->response_type & ~0x80) == XCB_MOTION_NOTIFY)
                    continue;
            }

            if (response_type == XCB_CONFIGURE_NOTIFY) {
                // compress multiple configure notify events for the same window
                bool found = false;
                for (int j = i; j < eventqueue->size(); ++j) {
                    xcb_generic_event_t *other = eventqueue->at(j);
                    if (other && (other->response_type & ~0x80) == XCB_CONFIGURE_NOTIFY
                        && ((xcb_configure_notify_event_t *)other)->event == ((xcb_configure_notify_event_t *)event)->event)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    continue;
            }

            bool accepted = false;
            if (clipboard()->processIncr())
                clipboard()->incrTransactionPeeker(event, accepted);
            if (accepted)
                continue;

            QVector<PeekFunc>::iterator it = m_peekFuncs.begin();
            while (it != m_peekFuncs.end()) {
                // These callbacks return true if the event is what they were
                // waiting for, remove them from the list in that case.
                if ((*it)(this, event))
                    it = m_peekFuncs.erase(it);
                else
                    ++it;
            }
            m_reader->unlock();
            handleXcbEvent(event);
            m_reader->lock();
        }
    }

    eventqueue->clear();

    m_reader->unlock();

    // Indicate with a null event that the event the callbacks are waiting for
    // is not in the queue currently.
    Q_FOREACH (PeekFunc f, m_peekFuncs)
        f(this, 0);
    m_peekFuncs.clear();

    xcb_flush(xcb_connection());
}

void QXcbConnection::handleClientMessageEvent(const xcb_client_message_event_t *event)
{
    if (event->format != 32)
        return;

#ifndef QT_NO_DRAGANDDROP
    if (event->type == atom(QXcbAtom::XdndStatus)) {
        drag()->handleStatus(event);
    } else if (event->type == atom(QXcbAtom::XdndFinished)) {
        drag()->handleFinished(event);
    }
#endif
    if (m_systemTrayTracker && event->type == atom(QXcbAtom::MANAGER))
        m_systemTrayTracker->notifyManagerClientMessageEvent(event);

    QXcbWindow *window = platformWindowFromId(event->window);
    if (!window)
        return;

    window->handleClientMessageEvent(event);
}

xcb_generic_event_t *QXcbConnection::checkEvent(int type)
{
    QXcbEventArray *eventqueue = m_reader->lock();

    for (int i = 0; i < eventqueue->size(); ++i) {
        xcb_generic_event_t *event = eventqueue->at(i);
        if (event && event->response_type == type) {
            (*eventqueue)[i] = 0;
            m_reader->unlock();
            return event;
        }
    }

    m_reader->unlock();

    return 0;
}

static const char * xcb_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"
    "_NET_WM_SYNC_REQUEST\0"
    "_NET_WM_SYNC_REQUEST_COUNTER\0"
    "MANAGER\0"
    "_NET_SYSTEM_TRAY_OPCODE\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"
    "WM_CLASS\0"

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

    "_QT_CLOSE_CONNECTION\0"

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
    "TEXT\0"
    "UTF8_STRING\0"
    "CARDINAL\0"

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

    // XInput2
    "Button Left\0"
    "Button Middle\0"
    "Button Right\0"
    "Button Wheel Up\0"
    "Button Wheel Down\0"
    "Button Horiz Wheel Left\0"
    "Button Horiz Wheel Right\0"
    "Abs MT Position X\0"
    "Abs MT Position Y\0"
    "Abs MT Touch Major\0"
    "Abs MT Touch Minor\0"
    "Abs MT Pressure\0"
    "Abs MT Tracking ID\0"
    "Max Contacts\0"
    "Rel X\0"
    "Rel Y\0"
    // XInput2 tablet
    "Abs X\0"
    "Abs Y\0"
    "Abs Pressure\0"
    "Abs Tilt X\0"
    "Abs Tilt Y\0"
    "Abs Wheel\0"
    "Abs Distance\0"
    "Wacom Serial IDs\0"
    "INTEGER\0"
    "Rel Horiz Wheel\0"
    "Rel Vert Wheel\0"
    "Rel Horiz Scroll\0"
    "Rel Vert Scroll\0"
#if XCB_USE_MAEMO_WINDOW_PROPERTIES
    "_MEEGOTOUCH_ORIENTATION_ANGLE\0"
#endif
    "_XSETTINGS_SETTINGS\0"
    "_COMPIZ_DECOR_PENDING\0"
    "_COMPIZ_DECOR_REQUEST\0"
    "_COMPIZ_DECOR_DELETE_PIXMAP\0" // \0\0 terminates loop.
};

QXcbAtom::Atom QXcbConnection::qatom(xcb_atom_t xatom) const
{
    return static_cast<QXcbAtom::Atom>(std::find(m_allAtoms, m_allAtoms + QXcbAtom::NAtoms, xatom) - m_allAtoms);
}

void QXcbConnection::initializeAllAtoms() {
    const char *names[QXcbAtom::NAtoms];
    const char *ptr = xcb_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == QXcbAtom::NPredefinedAtoms);

    QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
    settings_atom_name += m_displayName;
    names[i++] = settings_atom_name;

    xcb_intern_atom_cookie_t cookies[QXcbAtom::NAtoms];

    Q_ASSERT(i == QXcbAtom::NAtoms);
    for (i = 0; i < QXcbAtom::NAtoms; ++i)
        cookies[i] = xcb_intern_atom(xcb_connection(), false, strlen(names[i]), names[i]);

    for (i = 0; i < QXcbAtom::NAtoms; ++i) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_connection(), cookies[i], 0);
        m_allAtoms[i] = reply->atom;
        free(reply);
    }
}

xcb_atom_t QXcbConnection::internAtom(const char *name)
{
    if (!name || *name == 0)
        return XCB_NONE;

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_connection(), false, strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_connection(), cookie, 0);
    int atom = reply->atom;
    free(reply);
    return atom;
}

QByteArray QXcbConnection::atomName(xcb_atom_t atom)
{
    if (!atom)
        return QByteArray();

    xcb_generic_error_t *error = 0;
    xcb_get_atom_name_cookie_t cookie = Q_XCB_CALL(xcb_get_atom_name(xcb_connection(), atom));
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(xcb_connection(), cookie, &error);
    if (error) {
        qWarning() << "QXcbConnection::atomName: bad Atom" << atom;
        free(error);
    }
    if (reply) {
        QByteArray result(xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
        free(reply);
        return result;
    }
    return QByteArray();
}

const xcb_format_t *QXcbConnection::formatForDepth(uint8_t depth) const
{
    xcb_format_iterator_t iterator =
        xcb_setup_pixmap_formats_iterator(m_setup);

    while (iterator.rem) {
        xcb_format_t *format = iterator.data;
        if (format->depth == depth)
            return format;
        xcb_format_next(&iterator);
    }

    return 0;
}

void QXcbConnection::sync()
{
    // from xcb_aux_sync
    xcb_get_input_focus_cookie_t cookie = Q_XCB_CALL(xcb_get_input_focus(xcb_connection()));
    free(xcb_get_input_focus_reply(xcb_connection(), cookie, 0));
}

void QXcbConnection::initializeXFixes()
{
    xcb_generic_error_t *error = 0;
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_xfixes_id);
    if (!reply || !reply->present)
        return;

    xfixes_first_event = reply->first_event;
    xcb_xfixes_query_version_cookie_t xfixes_query_cookie = xcb_xfixes_query_version(m_connection,
                                                                                     XCB_XFIXES_MAJOR_VERSION,
                                                                                     XCB_XFIXES_MINOR_VERSION);
    xcb_xfixes_query_version_reply_t *xfixes_query = xcb_xfixes_query_version_reply (m_connection,
                                                                                     xfixes_query_cookie, &error);
    if (!xfixes_query || error || xfixes_query->major_version < 2) {
        qWarning("QXcbConnection: Failed to initialize XFixes");
        free(error);
        xfixes_first_event = 0;
    }
    free(xfixes_query);
}

void QXcbConnection::initializeXRender()
{
#ifdef XCB_USE_RENDER
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_render_id);
    if (!reply || !reply->present)
        return;

    xcb_generic_error_t *error = 0;
    xcb_render_query_version_cookie_t xrender_query_cookie = xcb_render_query_version(m_connection,
                                                                                      XCB_RENDER_MAJOR_VERSION,
                                                                                      XCB_RENDER_MINOR_VERSION);
    xcb_render_query_version_reply_t *xrender_query = xcb_render_query_version_reply(m_connection,
                                                                                     xrender_query_cookie, &error);
    if (!xrender_query || error || (xrender_query->major_version == 0 && xrender_query->minor_version < 5)) {
        qWarning("QXcbConnection: Failed to initialize XRender");
        free(error);
    }
    free(xrender_query);
#endif
}

void QXcbConnection::initializeGLX()
{
#ifdef XCB_HAS_XCB_GLX
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_glx_id);
    if (!reply || !reply->present)
        return;

    has_glx_extension = true;

    xcb_generic_error_t *error = 0;
    xcb_glx_query_version_cookie_t xglx_query_cookie = xcb_glx_query_version(m_connection,
                                                                             XCB_GLX_MAJOR_VERSION,
                                                                             XCB_GLX_MINOR_VERSION);
    xcb_glx_query_version_reply_t *xglx_query = xcb_glx_query_version_reply(m_connection,
                                                                            xglx_query_cookie, &error);
    if (!xglx_query || error) {
        qWarning("QXcbConnection: Failed to initialize GLX");
        free(error);
        has_glx_extension = false;
    }
    free(xglx_query);
#else
    // no way to check, assume GLX is present
    has_glx_extension = true;
#endif
}

void QXcbConnection::initializeXRandr()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_randr_id);
    if (!reply || !reply->present)
        return;

    xrandr_first_event = reply->first_event;

    xcb_generic_error_t *error = 0;
    xcb_randr_query_version_cookie_t xrandr_query_cookie = xcb_randr_query_version(m_connection,
                                                                                   XCB_RANDR_MAJOR_VERSION,
                                                                                   XCB_RANDR_MINOR_VERSION);

    has_randr_extension = true;

    xcb_randr_query_version_reply_t *xrandr_query = xcb_randr_query_version_reply(m_connection,
                                                                                  xrandr_query_cookie, &error);
    if (!xrandr_query || error || (xrandr_query->major_version < 1 || (xrandr_query->major_version == 1 && xrandr_query->minor_version < 2))) {
        qWarning("QXcbConnection: Failed to initialize XRandr");
        free(error);
        has_randr_extension = false;
    }
    free(xrandr_query);
}

void QXcbConnection::initializeXShape()
{
    const xcb_query_extension_reply_t *xshape_reply = xcb_get_extension_data(m_connection, &xcb_shape_id);
    if (!xshape_reply || !xshape_reply->present)
        return;

    has_shape_extension = true;
    xcb_shape_query_version_cookie_t cookie = xcb_shape_query_version(m_connection);
    xcb_shape_query_version_reply_t *shape_query = xcb_shape_query_version_reply(m_connection,
                                                                                 cookie, NULL);
    if (!shape_query) {
        qWarning("QXcbConnection: Failed to initialize SHAPE extension");
    } else if (shape_query->major_version > 1 || (shape_query->major_version == 1 && shape_query->minor_version >= 1)) {
        // The input shape is the only thing added in SHAPE 1.1
        has_input_shape = true;
    }
    free(shape_query);
}

void QXcbConnection::initializeXKB()
{
#ifndef QT_NO_XKB
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_xkb_id);
    if (!reply || !reply->present) {
        qWarning() << "Qt: XKEYBOARD extension not present on the X server.";
        xkb_first_event = 0;
        return;
    }
    xkb_first_event = reply->first_event;

    xcb_connection_t *c = connection()->xcb_connection();
    xcb_xkb_use_extension_cookie_t xkb_query_cookie;
    xcb_xkb_use_extension_reply_t *xkb_query;

    xkb_query_cookie = xcb_xkb_use_extension(c, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION);
    xkb_query = xcb_xkb_use_extension_reply(c, xkb_query_cookie, 0);

    if (!xkb_query) {
        qWarning("Qt: Failed to initialize XKB extension");
        return;
    } else if (!xkb_query->supported) {
        qWarning("Qt: Unsupported XKB version (We want %d %d, but X server has %d %d)",
                 XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION,
                 xkb_query->serverMajor, xkb_query->serverMinor);
        free(xkb_query);
        return;
    }

    has_xkb = true;
    free(xkb_query);

    const uint16_t required_map_parts = (XCB_XKB_MAP_PART_KEY_TYPES |
        XCB_XKB_MAP_PART_KEY_SYMS |
        XCB_XKB_MAP_PART_MODIFIER_MAP |
        XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
        XCB_XKB_MAP_PART_KEY_ACTIONS |
        XCB_XKB_MAP_PART_KEY_BEHAVIORS |
        XCB_XKB_MAP_PART_VIRTUAL_MODS |
        XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

    const uint16_t required_events = (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
        XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
        XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

    // XKB events are reported to all interested clients without regard
    // to the current keyboard input focus or grab state
    xcb_void_cookie_t select = xcb_xkb_select_events_checked(c,
                       XCB_XKB_ID_USE_CORE_KBD,
                       required_events,
                       0,
                       required_events,
                       required_map_parts,
                       required_map_parts,
                       0);

    xcb_generic_error_t *error = xcb_request_check(c, select);
    if (error) {
        free(error);
        qWarning() << "Qt: failed to select notify events from xcb-xkb";
        return;
    }
#endif
}

#if defined(XCB_USE_EGL)
bool QXcbConnection::hasEgl() const
{
    return m_has_egl;
}
#endif // defined(XCB_USE_EGL)

#if defined(XCB_USE_XINPUT2) || defined(XCB_USE_XINPUT2_MAEMO)
static int xi2ValuatorOffset(unsigned char *maskPtr, int maskLen, int number)
{
    int offset = 0;
    for (int i = 0; i < maskLen; i++) {
        if (number < 8) {
            if ((maskPtr[i] & (1 << number)) == 0)
                return -1;
        }
        for (int j = 0; j < 8; j++) {
            if (j == number)
                return offset;
            if (maskPtr[i] & (1 << j))
                offset++;
        }
        number -= 8;
    }
    return -1;
}

bool QXcbConnection::xi2GetValuatorValueIfSet(void *event, int valuatorNum, double *value)
{
    xXIDeviceEvent *xideviceevent = static_cast<xXIDeviceEvent *>(event);
    unsigned char *buttonsMaskAddr = (unsigned char*)&xideviceevent[1];
    unsigned char *valuatorsMaskAddr = buttonsMaskAddr + xideviceevent->buttons_len * 4;
    FP3232 *valuatorsValuesAddr = (FP3232*)(valuatorsMaskAddr + xideviceevent->valuators_len * 4);

    int valuatorOffset = xi2ValuatorOffset(valuatorsMaskAddr, xideviceevent->valuators_len, valuatorNum);
    if (valuatorOffset < 0)
        return false;

    *value = valuatorsValuesAddr[valuatorOffset].integral;
    *value += ((double)valuatorsValuesAddr[valuatorOffset].frac / (1 << 16) / (1 << 16));
    return true;
}

// Starting from the xcb version 1.9.3 struct xcb_ge_event_t has changed:
// - "pad0" became "extension"
// - "pad1" and "pad" became "pad0"
// New and old version of this struct share the following fields:
// NOTE: API might change again in the next release of xcb in which case this comment will
// need to be updated to reflect the reality.
typedef struct qt_xcb_ge_event_t {
    uint8_t  response_type;
    uint8_t  extension;
    uint16_t sequence;
    uint32_t length;
    uint16_t event_type;
} qt_xcb_ge_event_t;

bool QXcbConnection::xi2PrepareXIGenericDeviceEvent(xcb_ge_event_t *ev, int opCode)
{
    qt_xcb_ge_event_t *event = (qt_xcb_ge_event_t *)ev;
    // xGenericEvent has "extension" on the second byte, the same is true for xcb_ge_event_t starting from
    // the xcb version 1.9.3, prior to that it was called "pad0".
    if (event->extension == opCode) {
        // xcb event structs contain stuff that wasn't on the wire, the full_sequence field
        // adds an extra 4 bytes and generic events cookie data is on the wire right after the standard 32 bytes.
        // Move this data back to have the same layout in memory as it was on the wire
        // and allow casting, overwriting the full_sequence field.
        memmove((char*) event + 32, (char*) event + 36, event->length * 4);
        return true;
    }
    return false;
}
#endif // defined(XCB_USE_XINPUT2) || defined(XCB_USE_XINPUT2_MAEMO)

QXcbSystemTrayTracker *QXcbConnection::systemTrayTracker()
{
    if (!m_systemTrayTracker) {
        if ( (m_systemTrayTracker = QXcbSystemTrayTracker::create(this)) ) {
            connect(m_systemTrayTracker, SIGNAL(systemTrayWindowChanged(QScreen*)),
                    QGuiApplication::platformNativeInterface(), SIGNAL(systemTrayWindowChanged(QScreen*)));
        }
    }
    return m_systemTrayTracker;
}

QXcbConnectionGrabber::QXcbConnectionGrabber(QXcbConnection *connection)
    :m_connection(connection)
{
    connection->grabServer();
}

QXcbConnectionGrabber::~QXcbConnectionGrabber()
{
    if (m_connection)
        m_connection->ungrabServer();
}

void QXcbConnectionGrabber::release()
{
    if (m_connection) {
        m_connection->ungrabServer();
        m_connection = 0;
    }
}

QT_END_NAMESPACE
