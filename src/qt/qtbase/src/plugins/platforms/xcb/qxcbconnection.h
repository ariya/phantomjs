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

#ifndef QXCBCONNECTION_H
#define QXCBCONNECTION_H

#include <xcb/xcb.h>

#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
#include <QVarLengthArray>
#include <qpa/qwindowsysteminterface.h>

// This is needed to make Qt compile together with XKB. xkb.h is using a variable
// which is called 'explicit', this is a reserved keyword in c++
#ifndef QT_NO_XKB
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit
#endif

#ifndef QT_NO_TABLETEVENT
#include <QTabletEvent>
#endif

#ifdef XCB_USE_XINPUT2_MAEMO
struct XInput2MaemoData;
#elif XCB_USE_XINPUT2
#include <X11/extensions/XI2.h>
#ifdef XIScrollClass
#define XCB_USE_XINPUT21    // XI 2.1 adds smooth scrolling support
#ifdef XI_TouchBeginMask
#define XCB_USE_XINPUT22    // XI 2.2 adds multi-point touch support
#endif
#endif
struct XInput2DeviceData;
#endif
struct xcb_randr_get_output_info_reply_t;

//#define Q_XCB_DEBUG

QT_BEGIN_NAMESPACE

class QXcbScreen;
class QXcbWindow;
class QXcbDrag;
class QXcbKeyboard;
class QXcbClipboard;
class QXcbWMSupport;
class QXcbNativeInterface;
class QXcbSystemTrayTracker;

namespace QXcbAtom {
    enum Atom {
        // window-manager <-> client protocols
        WM_PROTOCOLS,
        WM_DELETE_WINDOW,
        WM_TAKE_FOCUS,
        _NET_WM_PING,
        _NET_WM_CONTEXT_HELP,
        _NET_WM_SYNC_REQUEST,
        _NET_WM_SYNC_REQUEST_COUNTER,
        MANAGER, // System tray notification
        _NET_SYSTEM_TRAY_OPCODE, // System tray operation

        // ICCCM window state
        WM_STATE,
        WM_CHANGE_STATE,
        WM_CLASS,

        // Session management
        WM_CLIENT_LEADER,
        WM_WINDOW_ROLE,
        SM_CLIENT_ID,

        // Clipboard
        CLIPBOARD,
        INCR,
        TARGETS,
        MULTIPLE,
        TIMESTAMP,
        SAVE_TARGETS,
        CLIP_TEMPORARY,
        _QT_SELECTION,
        _QT_CLIPBOARD_SENTINEL,
        _QT_SELECTION_SENTINEL,
        CLIPBOARD_MANAGER,

        RESOURCE_MANAGER,

        _XSETROOT_ID,

        _QT_SCROLL_DONE,
        _QT_INPUT_ENCODING,

        // Qt/XCB specific
        _QT_CLOSE_CONNECTION,

        _MOTIF_WM_HINTS,

        DTWM_IS_RUNNING,
        ENLIGHTENMENT_DESKTOP,
        _DT_SAVE_MODE,
        _SGI_DESKS_MANAGER,

        // EWMH (aka NETWM)
        _NET_SUPPORTED,
        _NET_VIRTUAL_ROOTS,
        _NET_WORKAREA,

        _NET_MOVERESIZE_WINDOW,
        _NET_WM_MOVERESIZE,

        _NET_WM_NAME,
        _NET_WM_ICON_NAME,
        _NET_WM_ICON,

        _NET_WM_PID,

        _NET_WM_WINDOW_OPACITY,

        _NET_WM_STATE,
        _NET_WM_STATE_ABOVE,
        _NET_WM_STATE_BELOW,
        _NET_WM_STATE_FULLSCREEN,
        _NET_WM_STATE_MAXIMIZED_HORZ,
        _NET_WM_STATE_MAXIMIZED_VERT,
        _NET_WM_STATE_MODAL,
        _NET_WM_STATE_STAYS_ON_TOP,
        _NET_WM_STATE_DEMANDS_ATTENTION,

        _NET_WM_USER_TIME,
        _NET_WM_USER_TIME_WINDOW,
        _NET_WM_FULL_PLACEMENT,

        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_DESKTOP,
        _NET_WM_WINDOW_TYPE_DOCK,
        _NET_WM_WINDOW_TYPE_TOOLBAR,
        _NET_WM_WINDOW_TYPE_MENU,
        _NET_WM_WINDOW_TYPE_UTILITY,
        _NET_WM_WINDOW_TYPE_SPLASH,
        _NET_WM_WINDOW_TYPE_DIALOG,
        _NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        _NET_WM_WINDOW_TYPE_POPUP_MENU,
        _NET_WM_WINDOW_TYPE_TOOLTIP,
        _NET_WM_WINDOW_TYPE_NOTIFICATION,
        _NET_WM_WINDOW_TYPE_COMBO,
        _NET_WM_WINDOW_TYPE_DND,
        _NET_WM_WINDOW_TYPE_NORMAL,
        _KDE_NET_WM_WINDOW_TYPE_OVERRIDE,

        _KDE_NET_WM_FRAME_STRUT,

        _NET_STARTUP_INFO,
        _NET_STARTUP_INFO_BEGIN,

        _NET_SUPPORTING_WM_CHECK,

        _NET_WM_CM_S0,

        _NET_SYSTEM_TRAY_VISUAL,

        _NET_ACTIVE_WINDOW,

        // Property formats
        TEXT,
        UTF8_STRING,
        CARDINAL,

        // Xdnd
        XdndEnter,
        XdndPosition,
        XdndStatus,
        XdndLeave,
        XdndDrop,
        XdndFinished,
        XdndTypelist,
        XdndActionList,

        XdndSelection,

        XdndAware,
        XdndProxy,

        XdndActionCopy,
        XdndActionLink,
        XdndActionMove,
        XdndActionPrivate,

        // Motif DND
        _MOTIF_DRAG_AND_DROP_MESSAGE,
        _MOTIF_DRAG_INITIATOR_INFO,
        _MOTIF_DRAG_RECEIVER_INFO,
        _MOTIF_DRAG_WINDOW,
        _MOTIF_DRAG_TARGETS,

        XmTRANSFER_SUCCESS,
        XmTRANSFER_FAILURE,

        // Xkb
        _XKB_RULES_NAMES,

        // XEMBED
        _XEMBED,
        _XEMBED_INFO,

        // XInput2
        ButtonLeft,
        ButtonMiddle,
        ButtonRight,
        ButtonWheelUp,
        ButtonWheelDown,
        ButtonHorizWheelLeft,
        ButtonHorizWheelRight,
        AbsMTPositionX,
        AbsMTPositionY,
        AbsMTTouchMajor,
        AbsMTTouchMinor,
        AbsMTPressure,
        AbsMTTrackingID,
        MaxContacts,
        RelX,
        RelY,
        // XInput2 tablet
        AbsX,
        AbsY,
        AbsPressure,
        AbsTiltX,
        AbsTiltY,
        AbsWheel,
        AbsDistance,
        WacomSerialIDs,
        INTEGER,
        RelHorizWheel,
        RelVertWheel,
        RelHorizScroll,
        RelVertScroll,

#if XCB_USE_MAEMO_WINDOW_PROPERTIES
        MeegoTouchOrientationAngle,
#endif
        _XSETTINGS_SETTINGS,

        _COMPIZ_DECOR_PENDING,
        _COMPIZ_DECOR_REQUEST,
        _COMPIZ_DECOR_DELETE_PIXMAP,

        NPredefinedAtoms,

        _QT_SETTINGS_TIMESTAMP = NPredefinedAtoms,
        NAtoms
    };
}

typedef QVarLengthArray<xcb_generic_event_t *, 64> QXcbEventArray;

class QXcbConnection;
class QXcbEventReader : public QThread
{
    Q_OBJECT
public:
    QXcbEventReader(QXcbConnection *connection);

    void run();

    QXcbEventArray *lock();
    void unlock();

    void start();

signals:
    void eventPending();

private slots:
    void registerForEvents();

private:
    void addEvent(xcb_generic_event_t *event);

    QMutex m_mutex;
    QXcbEventArray m_events;
    QXcbConnection *m_connection;

    typedef xcb_generic_event_t * (*XcbPollForQueuedEventFunctionPointer)(xcb_connection_t *c);
    XcbPollForQueuedEventFunctionPointer m_xcb_poll_for_queued_event;
};

class QXcbWindowEventListener
{
public:
    virtual bool handleGenericEvent(xcb_generic_event_t *, long *) { return false; }

    virtual void handleExposeEvent(const xcb_expose_event_t *) {}
    virtual void handleClientMessageEvent(const xcb_client_message_event_t *) {}
    virtual void handleConfigureNotifyEvent(const xcb_configure_notify_event_t *) {}
    virtual void handleMapNotifyEvent(const xcb_map_notify_event_t *) {}
    virtual void handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *) {}
    virtual void handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *) {}
    virtual void handleButtonPressEvent(const xcb_button_press_event_t *) {}
    virtual void handleButtonReleaseEvent(const xcb_button_release_event_t *) {}
    virtual void handleMotionNotifyEvent(const xcb_motion_notify_event_t *) {}
    virtual void handleEnterNotifyEvent(const xcb_enter_notify_event_t *) {}
    virtual void handleLeaveNotifyEvent(const xcb_leave_notify_event_t *) {}
    virtual void handleFocusInEvent(const xcb_focus_in_event_t *) {}
    virtual void handleFocusOutEvent(const xcb_focus_out_event_t *) {}
    virtual void handlePropertyNotifyEvent(const xcb_property_notify_event_t *) {}

    virtual QXcbWindow *toWindow() { return 0; }
};

typedef QHash<xcb_window_t, QXcbWindowEventListener *> WindowMapper;

class QAbstractEventDispatcher;
class QXcbConnection : public QObject
{
    Q_OBJECT
public:
    QXcbConnection(QXcbNativeInterface *nativeInterface, bool canGrabServer, const char *displayName = 0);
    ~QXcbConnection();

    QXcbConnection *connection() const { return const_cast<QXcbConnection *>(this); }

    const QList<QXcbScreen *> &screens() const { return m_screens; }
    int primaryScreen() const { return m_primaryScreen; }

    inline xcb_atom_t atom(QXcbAtom::Atom atom) const { return m_allAtoms[atom]; }
    QXcbAtom::Atom qatom(xcb_atom_t atom) const;
    xcb_atom_t internAtom(const char *name);
    QByteArray atomName(xcb_atom_t atom);

    const char *displayName() const { return m_displayName.constData(); }
    xcb_connection_t *xcb_connection() const { return m_connection; }
    const xcb_setup_t *setup() const { return m_setup; }
    const xcb_format_t *formatForDepth(uint8_t depth) const;

    QXcbKeyboard *keyboard() const { return m_keyboard; }

#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *clipboard() const { return m_clipboard; }
#endif
#ifndef QT_NO_DRAGANDDROP
    QXcbDrag *drag() const { return m_drag; }
#endif

    QXcbWMSupport *wmSupport() const { return m_wmSupport.data(); }
    xcb_window_t rootWindow();
#ifdef XCB_USE_XLIB
    void *xlib_display() const { return m_xlib_display; }
#endif

#ifdef XCB_USE_EGL
    bool hasEgl() const;
#endif
#if defined(XCB_USE_EGL)
    void *egl_display() const { return m_egl_display; }
#endif
#ifdef XCB_USE_XINPUT2_MAEMO
    bool isUsingXInput2Maemo();
#elif defined(XCB_USE_XINPUT2)
    void xi2Select(xcb_window_t window);
#endif
#ifdef XCB_USE_XINPUT21
    bool isUsingXInput21() { return m_xi2Enabled && m_xi2Minor >= 1; }
#else
    bool isUsingXInput21() { return false; }
#endif


    void sync();
    void flush() { xcb_flush(m_connection); }

    void handleXcbError(xcb_generic_error_t *error);
    void handleXcbEvent(xcb_generic_event_t *event);

    void addWindowEventListener(xcb_window_t id, QXcbWindowEventListener *eventListener);
    void removeWindowEventListener(xcb_window_t id);
    QXcbWindowEventListener *windowEventListenerFromId(xcb_window_t id);
    QXcbWindow *platformWindowFromId(xcb_window_t id);

    xcb_generic_event_t *checkEvent(int type);
    template<typename T>
    inline xcb_generic_event_t *checkEvent(T &checker);

    typedef bool (*PeekFunc)(QXcbConnection *, xcb_generic_event_t *);
    void addPeekFunc(PeekFunc f);

    inline xcb_timestamp_t time() const { return m_time; }
    inline void setTime(xcb_timestamp_t t) { if (t > m_time) m_time = t; }

    inline xcb_timestamp_t netWmUserTime() const { return m_netWmUserTime; }
    inline void setNetWmUserTime(xcb_timestamp_t t) { if (t > m_netWmUserTime) m_netWmUserTime = t; }

    bool hasGLX() const { return has_glx_extension; }
    bool hasXFixes() const { return xfixes_first_event > 0; }
    bool hasXShape() const { return has_shape_extension; }
    bool hasXRandr() const { return has_randr_extension; }
    bool hasInputShape() const { return has_input_shape; }
    bool hasTouchWithoutMouseEmulation() const { return has_touch_without_mouse_emulation; }
    bool hasXKB() const { return has_xkb; }

    bool supportsThreadedRendering() const { return m_reader->isRunning(); }

    xcb_timestamp_t getTimestamp();

    Qt::MouseButtons buttons() const { return m_buttons; }

    QXcbWindow *focusWindow() const { return m_focusWindow; }
    void setFocusWindow(QXcbWindow *);

    QByteArray startupId() const { return m_startupId; }
    void clearStartupId() { m_startupId.clear(); }

    void grabServer();
    void ungrabServer();

    QXcbNativeInterface *nativeInterface() const { return m_nativeInterface; }

    QXcbSystemTrayTracker *systemTrayTracker();

#ifdef XCB_USE_XINPUT2
    void handleEnterEvent(const xcb_enter_notify_event_t *);
#endif

private slots:
    void processXcbEvents();

private:
    void initializeAllAtoms();
    void sendConnectionEvent(QXcbAtom::Atom atom, uint id = 0);
    void initializeGLX();
    void initializeXFixes();
    void initializeXRender();
    void initializeXRandr();
    void initializeXShape();
    void initializeXKB();
#ifdef XCB_USE_XINPUT2_MAEMO
    void initializeXInput2Maemo();
    void finalizeXInput2Maemo();
    void handleGenericEventMaemo(xcb_ge_event_t *event);
#endif
    void handleClientMessageEvent(const xcb_client_message_event_t *event);
    QXcbScreen* findOrCreateScreen(QList<QXcbScreen *>& newScreens, int screenNumber,
        xcb_screen_t* xcbScreen, xcb_randr_get_output_info_reply_t *output = NULL);
    void updateScreens();
    void handleButtonPress(xcb_generic_event_t *event);
    void handleButtonRelease(xcb_generic_event_t *event);

    bool m_xi2Enabled;
    int m_xi2Minor;
#ifdef XCB_USE_XINPUT2
    void initializeXInput2();
    void finalizeXInput2();
    XInput2DeviceData *deviceForId(int id);
    void xi2HandleEvent(xcb_ge_event_t *event);
    int m_xiOpCode, m_xiEventBase, m_xiErrorBase;
#ifndef QT_NO_TABLETEVENT
    struct TabletData {
        TabletData() : deviceId(0), down(false), serialId(0), inProximity(false) { }
        int deviceId;
        QTabletEvent::PointerType pointerType;
        bool down;
        qint64 serialId;
        bool inProximity;
        struct ValuatorClassInfo {
            ValuatorClassInfo() : minVal(0), maxVal(0) { }
            double minVal;
            double maxVal;
            int number;
        };
        QHash<int, ValuatorClassInfo> valuatorInfo;
    };
    bool xi2HandleTabletEvent(void *event, TabletData *tabletData);
    void xi2ReportTabletEvent(const TabletData &tabletData, void *event);
    QVector<TabletData> m_tabletData;
#endif
    struct ScrollingDevice {
        ScrollingDevice() : deviceId(0), verticalIndex(0), horizontalIndex(0), orientations(0), legacyOrientations(0) { }
        int deviceId;
        int verticalIndex, horizontalIndex;
        double verticalIncrement, horizontalIncrement;
        Qt::Orientations orientations;
        Qt::Orientations legacyOrientations;
        QPointF lastScrollPosition;
    };
    void xi2HandleScrollEvent(void *event, ScrollingDevice &scrollingDevice);
    QHash<int, ScrollingDevice> m_scrollingDevices;
#endif // XCB_USE_XINPUT2

#if defined(XCB_USE_XINPUT2) || defined(XCB_USE_XINPUT2_MAEMO)
    static bool xi2GetValuatorValueIfSet(void *event, int valuatorNum, double *value);
    static bool xi2PrepareXIGenericDeviceEvent(xcb_ge_event_t *event, int opCode);
#endif

    xcb_connection_t *m_connection;
    const xcb_setup_t *m_setup;
    bool m_canGrabServer;

    QList<QXcbScreen *> m_screens;
    int m_primaryScreen;

    xcb_atom_t m_allAtoms[QXcbAtom::NAtoms];

    xcb_timestamp_t m_time;
    xcb_timestamp_t m_netWmUserTime;

    QByteArray m_displayName;

    QXcbKeyboard *m_keyboard;
#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *m_clipboard;
#endif
#ifndef QT_NO_DRAGANDDROP
    QXcbDrag *m_drag;
#endif
    QScopedPointer<QXcbWMSupport> m_wmSupport;
    QXcbNativeInterface *m_nativeInterface;

#if defined(XCB_USE_XLIB)
    void *m_xlib_display;
#endif
    QXcbEventReader *m_reader;
#ifdef XCB_USE_XINPUT2_MAEMO
    XInput2MaemoData *m_xinputData;
#elif defined(XCB_USE_XINPUT2)
    QHash<int, QWindowSystemInterface::TouchPoint> m_touchPoints;
    QHash<int, XInput2DeviceData*> m_touchDevices;
#endif
#if defined(XCB_USE_EGL)
    void *m_egl_display;
    bool m_has_egl;
#endif
#ifdef Q_XCB_DEBUG
    struct CallInfo {
        int sequence;
        QByteArray file;
        int line;
    };
    QVector<CallInfo> m_callLog;
    QMutex m_callLogMutex;
    void log(const char *file, int line, int sequence);
    template <typename cookie_t>
    friend cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection, const char *file, int line);
#endif

    WindowMapper m_mapper;

    QVector<PeekFunc> m_peekFuncs;

    uint32_t xfixes_first_event;
    uint32_t xrandr_first_event;
    uint32_t xkb_first_event;

    bool has_glx_extension;
    bool has_shape_extension;
    bool has_randr_extension;
    bool has_input_shape;
    bool has_touch_without_mouse_emulation;
    bool has_xkb;
    bool debug_xinput_devices;
    bool debug_xinput;

    Qt::MouseButtons m_buttons;

    QXcbWindow *m_focusWindow;

    QByteArray m_startupId;
    QXcbSystemTrayTracker *m_systemTrayTracker;

    friend class QXcbEventReader;
};

#define DISPLAY_FROM_XCB(object) ((Display *)(object->connection()->xlib_display()))

template<typename T>
xcb_generic_event_t *QXcbConnection::checkEvent(T &checker)
{
    QXcbEventArray *eventqueue = m_reader->lock();

    for (int i = 0; i < eventqueue->size(); ++i) {
        xcb_generic_event_t *event = eventqueue->at(i);
        if (checker.checkEvent(event)) {
            (*eventqueue)[i] = 0;
            m_reader->unlock();
            return event;
        }
    }
    m_reader->unlock();
    return 0;
}

class QXcbConnectionGrabber
{
public:
    QXcbConnectionGrabber(QXcbConnection *connection);
    ~QXcbConnectionGrabber();
    void release();
private:
    QXcbConnection *m_connection;
};

#ifdef Q_XCB_DEBUG
template <typename cookie_t>
cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection, const char *file, int line)
{
    connection->log(file, line, cookie.sequence);
    return cookie;
}
#define Q_XCB_CALL(x) q_xcb_call_template(x, connection(), __FILE__, __LINE__)
#define Q_XCB_CALL2(x, connection) q_xcb_call_template(x, connection, __FILE__, __LINE__)
#define Q_XCB_NOOP(c) q_xcb_call_template(xcb_no_operation(c->xcb_connection()), c, __FILE__, __LINE__);
#else
#define Q_XCB_CALL(x) x
#define Q_XCB_CALL2(x, connection) x
#define Q_XCB_NOOP(c)
#endif


#if defined(XCB_USE_EGL)
#define EGL_DISPLAY_FROM_XCB(object) ((EGLDisplay)(object->connection()->egl_display()))
#endif

QT_END_NAMESPACE

#endif
