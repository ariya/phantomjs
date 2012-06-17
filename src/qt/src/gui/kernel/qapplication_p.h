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

#ifndef QAPPLICATION_P_H
#define QAPPLICATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp, qcolor_x11.cpp, qfiledialog.cpp
// and many other.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qapplication.h"
#include "QtGui/qevent.h"
#include "QtGui/qfont.h"
#include "QtGui/qcursor.h"
#include "QtGui/qregion.h"
#include "QtCore/qmutex.h"
#include "QtCore/qtranslator.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qhash.h"
#include "QtCore/qpointer.h"
#include "private/qcoreapplication_p.h"
#include "QtGui/private/qshortcutmap_p.h"
#include <private/qthread_p.h>
#include "QtCore/qpoint.h"
#include <QTime>
#ifdef Q_WS_QWS
#include "QtGui/qscreen_qws.h"
#include <private/qgraphicssystem_qws_p.h>
#endif
#ifdef Q_OS_SYMBIAN
#include <w32std.h>
#endif
#ifdef Q_WS_QPA
#include <QWindowSystemInterface>
#include "qwindowsysteminterface_qpa_p.h"
#include "QtGui/qplatformintegration_qpa.h"
#endif

QT_BEGIN_NAMESPACE

class QClipboard;
class QGraphicsScene;
class QGraphicsSystem;
class QInputContext;
class QObject;
class QWidget;
class QSocketNotifier;
#ifndef QT_NO_GESTURES
class QGestureManager;
#endif

extern bool qt_is_gui_used;
#ifndef QT_NO_CLIPBOARD
extern QClipboard *qt_clipboard;
#endif

#if defined (Q_OS_WIN32) || defined (Q_OS_CYGWIN) || defined(Q_OS_WINCE)
extern QSysInfo::WinVersion qt_winver;
enum { QT_TABLET_NPACKETQSIZE = 128 };
# ifdef Q_OS_WINCE
  extern DWORD qt_cever;
# endif
#elif defined (Q_OS_MAC)
extern QSysInfo::MacVersion qt_macver;
#endif
#if defined(Q_WS_QWS)
class QWSManager;
class QDirectPainter;
struct QWSServerCleaner { ~QWSServerCleaner(); };
#endif

#ifndef QT_NO_TABLET
struct QTabletDeviceData
{
#ifndef Q_WS_MAC
    int minPressure;
    int maxPressure;
    int minTanPressure;
    int maxTanPressure;
    int minX, maxX, minY, maxY, minZ, maxZ;
    inline QPointF scaleCoord(int coordX, int coordY, int outOriginX, int outExtentX,
                              int outOriginY, int outExtentY) const;
#endif

#if defined(Q_WS_X11) || (defined(Q_WS_MAC) && !defined(QT_MAC_USE_COCOA))
    QPointer<QWidget> widgetToGetPress;
#endif

#ifdef Q_WS_X11
    int deviceType;
    enum {
        TOTAL_XINPUT_EVENTS = 64
    };
    void *device;
    int eventCount;
    long unsigned int eventList[TOTAL_XINPUT_EVENTS]; // XEventClass is in fact a long unsigned int

    int xinput_motion;
    int xinput_key_press;
    int xinput_key_release;
    int xinput_button_press;
    int xinput_button_release;
    int xinput_proximity_in;
    int xinput_proximity_out;
#elif defined(Q_WS_WIN)
    qint64 llId;
    int currentDevice;
    int currentPointerType;
#elif defined(Q_WS_MAC)
    quint64 tabletUniqueID;
    int tabletDeviceType;
    int tabletPointerType;
    int capabilityMask;
#endif
};

static inline int sign(int x)
{
    return x >= 0 ? 1 : -1;
}

#ifndef Q_WS_MAC
inline QPointF QTabletDeviceData::scaleCoord(int coordX, int coordY,
                                            int outOriginX, int outExtentX,
                                            int outOriginY, int outExtentY) const
{
    QPointF ret;

    if (sign(outExtentX) == sign(maxX))
        ret.setX(((coordX - minX) * qAbs(outExtentX) / qAbs(qreal(maxX - minX))) + outOriginX);
    else
        ret.setX(((qAbs(maxX) - (coordX - minX)) * qAbs(outExtentX) / qAbs(qreal(maxX - minX)))
                 + outOriginX);

    if (sign(outExtentY) == sign(maxY))
        ret.setY(((coordY - minY) * qAbs(outExtentY) / qAbs(qreal(maxY - minY))) + outOriginY);
    else
        ret.setY(((qAbs(maxY) - (coordY - minY)) * qAbs(outExtentY) / qAbs(qreal(maxY - minY)))
                 + outOriginY);

    return ret;
}
#endif

typedef QList<QTabletDeviceData> QTabletDeviceDataList;
QTabletDeviceDataList *qt_tablet_devices();
# if defined(Q_WS_MAC)
typedef QHash<int, QTabletDeviceData> QMacTabletHash;
QMacTabletHash *qt_mac_tablet_hash();
# endif
#endif

#ifdef QT3_SUPPORT
extern "C" {
    typedef bool (*Ptrqt_tryAccelEvent)(QWidget *w, QKeyEvent *e);
    typedef bool (*Ptrqt_tryComposeUnicode)(QWidget *w, QKeyEvent *e);
    typedef bool (*Ptrqt_dispatchAccelEvent)(QWidget *w, QKeyEvent *e);
}
#endif

#if defined(Q_WS_WIN)
typedef BOOL (WINAPI *PtrRegisterTouchWindow)(HWND, ULONG);
typedef BOOL (WINAPI *PtrGetTouchInputInfo)(HANDLE, UINT, PVOID, int);
typedef BOOL (WINAPI *PtrCloseTouchInputHandle)(HANDLE);

#ifndef QT_NO_GESTURES
typedef BOOL (WINAPI *PtrGetGestureInfo)(HANDLE, PVOID);
typedef BOOL (WINAPI *PtrGetGestureExtraArgs)(HANDLE, UINT, PBYTE);
typedef BOOL (WINAPI *PtrCloseGestureInfoHandle)(HANDLE);
typedef BOOL (WINAPI *PtrSetGestureConfig)(HWND, DWORD, UINT, PVOID, UINT);
typedef BOOL (WINAPI *PtrGetGestureConfig)(HWND, DWORD, DWORD, PUINT, PVOID, UINT);

typedef BOOL (WINAPI *PtrBeginPanningFeedback)(HWND);
typedef BOOL (WINAPI *PtrUpdatePanningFeedback)(HWND, LONG, LONG, BOOL);
typedef BOOL (WINAPI *PtrEndPanningFeedback)(HWND, BOOL);

#ifndef WM_GESTURE
#  define WM_GESTURE 0x0119

#  define GID_BEGIN                       1
#  define GID_END                         2
#  define GID_ZOOM                        3
#  define GID_PAN                         4
#  define GID_ROTATE                      5
#  define GID_TWOFINGERTAP                6
#  define GID_ROLLOVER                    7

typedef struct tagGESTUREINFO
{
    UINT cbSize;
    DWORD dwFlags;
    DWORD dwID;
    HWND hwndTarget;
    POINTS ptsLocation;
    DWORD dwInstanceID;
    DWORD dwSequenceID;
    ULONGLONG ullArguments;
    UINT cbExtraArgs;
} GESTUREINFO;

#  define GC_PAN                                      0x00000001
#  define GC_PAN_WITH_SINGLE_FINGER_VERTICALLY        0x00000002
#  define GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY      0x00000004

#  define GC_ZOOM                                     0x00000001
#  define GC_ROTATE                                   0x00000001

typedef struct tagGESTURECONFIG
{
    DWORD dwID;
    DWORD dwWant;
    DWORD dwBlock;
} GESTURECONFIG;

#  define GID_ROTATE_ANGLE_FROM_ARGUMENT(arg) ((((double)(arg) / 65535.0) * 4.0 * 3.14159265) - 2.0*3.14159265)

#endif // WM_GESTURE

#if defined(Q_WS_WINCE_WM) && defined(QT_WINCE_GESTURES)
#undef GID_ZOOM
#define GID_ZOOM 0xf000
#undef GID_ROTATE
#define GID_ROTATE 0xf001
#undef GID_TWOFINGERTAP
#define GID_TWOFINGERTAP 0xf002
#undef GID_ROLLOVER
#define GID_ROLLOVER 0xf003
#endif

#endif // QT_NO_GESTURES

#endif // Q_WS_WIN

typedef QHash<QByteArray, QFont> FontHash;
FontHash *qt_app_fonts_hash();

typedef QHash<QByteArray, QPalette> PaletteHash;
PaletteHash *qt_app_palettes_hash();

class Q_GUI_EXPORT QApplicationPrivate : public QCoreApplicationPrivate
{
    Q_DECLARE_PUBLIC(QApplication)
public:
    QApplicationPrivate(int &argc, char **argv, QApplication::Type type, int flags);
    ~QApplicationPrivate();

#if defined(Q_WS_X11)
#ifndef QT_NO_SETTINGS
    static bool x11_apply_settings();
#endif
    static void reset_instance_pointer();
#elif defined(Q_WS_QWS)
    static bool qws_apply_settings();
    static QWidget *findWidget(const QObjectList&, const QPoint &, bool rec);
#endif
    static bool quitOnLastWindowClosed;
    static void emitLastWindowClosed();
#ifdef Q_WS_WINCE
    static int autoMaximizeThreshold;
#endif
    static bool autoSipEnabled;
    static QString desktopStyleKey();

    static QGraphicsSystem *graphicsSystem()
#if defined(Q_WS_QWS)
    { return QScreen::instance()->graphicsSystem(); }
#else
    { return graphics_system; }
#endif

#if defined(Q_WS_QPA)
    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    { return QCoreApplication::instance()->d_func()->threadData->eventDispatcher; }
#endif

    void createEventDispatcher();
    QString appName() const;
    static void dispatchEnterLeave(QWidget *enter, QWidget *leave);

    //modality
    static void enterModal(QWidget*);
    static void leaveModal(QWidget*);
    static void enterModal_sys(QWidget*);
    static void leaveModal_sys(QWidget*);
    static bool isBlockedByModal(QWidget *widget);
    static bool modalState();
    static bool tryModalHelper(QWidget *widget, QWidget **rettop = 0);
#ifdef Q_WS_MAC
    static QWidget *tryModalHelper_sys(QWidget *top);
	bool canQuit();
#endif

    bool notify_helper(QObject *receiver, QEvent * e);

    void construct(
#ifdef Q_WS_X11
                   Display *dpy = 0, Qt::HANDLE visual = 0, Qt::HANDLE cmap = 0
#endif
                   );
    void initialize();
    void process_cmdline();

#if defined(Q_WS_X11)
    static void x11_initialize_style();
#endif

    enum KeyPlatform {
        KB_Win = 1,
        KB_Mac = 2,
        KB_X11 = 4,
        KB_KDE = 8,
        KB_Gnome = 16,
        KB_CDE = 32,
        KB_S60 = 64,
        KB_All = 0xffff
    };

    static uint currentPlatform();
    bool inPopupMode() const;
    void closePopup(QWidget *popup);
    void openPopup(QWidget *popup);
    static void setFocusWidget(QWidget *focus, Qt::FocusReason reason);
    static QWidget *focusNextPrevChild_helper(QWidget *toplevel, bool next);

#ifndef QT_NO_SESSIONMANAGER
    QSessionManager *session_manager;
    QString session_id;
    QString session_key;
    bool is_session_restored;
#endif

#ifndef QT_NO_CURSOR
    QList<QCursor> cursor_list;
#endif
#ifndef QT_NO_GRAPHICSVIEW
    // Maintain a list of all scenes to ensure font and palette propagation to
    // all scenes.
    QList<QGraphicsScene *> scene_list;
#endif

    QBasicTimer toolTipWakeUp, toolTipFallAsleep;
    QPoint toolTipPos, toolTipGlobalPos, hoverGlobalPos;
    QPointer<QWidget> toolTipWidget;
#ifndef QT_NO_SHORTCUT
    QShortcutMap shortcutMap;
#endif

#ifdef QT3_SUPPORT
    bool qt_compat_used;
    bool qt_compat_resolved;
    Ptrqt_tryAccelEvent qt_tryAccelEvent;
    Ptrqt_tryComposeUnicode qt_tryComposeUnicode;
    Ptrqt_dispatchAccelEvent qt_dispatchAccelEvent;

    bool use_compat() {
        return qt_tryAccelEvent
               && qt_tryComposeUnicode
               && qt_dispatchAccelEvent;
    }
#endif
    static QInputContext *inputContext;
#ifdef Q_OS_SYMBIAN
    static bool inputContextBeingCreated;
#endif
    static Qt::MouseButtons mouse_buttons;
    static Qt::KeyboardModifiers modifier_buttons;

    static QSize app_strut;
    static QWidgetList *popupWidgets;
    static QStyle *app_style;
    static int app_cspec;
    static QPalette *app_pal;
    static QPalette *sys_pal;
    static QPalette *set_pal;
    static QGraphicsSystem *graphics_system;
    static QString graphics_system_name;
    static bool runtime_graphics_system;
#ifdef Q_WS_QPA
    static QPlatformIntegration *platform_integration;
#endif

private:
    static QFont *app_font; // private for a reason! Always use QApplication::font() instead!
public:
    static QFont *sys_font;
    static QFont *set_font;
    static QWidget *main_widget;
    static QWidget *focus_widget;
    static QWidget *hidden_focus_widget;
    static QWidget *active_window;
    static QIcon *app_icon;
    static bool obey_desktop_settings;
    static int  cursor_flash_time;
    static int  mouse_double_click_time;
    static int  keyboard_input_time;
#ifndef QT_NO_WHEELEVENT
    static int  wheel_scroll_lines;
#endif

    static bool animate_ui;
    static bool animate_menu;
    static bool animate_tooltip;
    static bool animate_combo;
    static bool fade_menu;
    static bool fade_tooltip;
    static bool animate_toolbox;
    static bool widgetCount; // Coupled with -widgetcount switch
    static bool load_testability; // Coupled with -testability switch

#ifdef Q_WS_MAC
    static bool native_modal_dialog_active;
#endif

    static void setSystemPalette(const QPalette &pal);
    static void setPalette_helper(const QPalette &palette, const char* className, bool clearWidgetPaletteHash);
    static void initializeWidgetPaletteHash();
    static void setSystemFont(const QFont &font);

#if defined(Q_WS_X11)
    static void applyX11SpecificCommandLineArguments(QWidget *main_widget);
#elif defined(Q_WS_QWS)
    static void applyQWSSpecificCommandLineArguments(QWidget *main_widget);
#endif

#ifdef Q_WS_MAC
    static OSStatus globalEventProcessor(EventHandlerCallRef, EventRef, void *);
    static OSStatus globalAppleEventProcessor(const AppleEvent *, AppleEvent *, long);
    static OSStatus tabletProximityCallback(EventHandlerCallRef, EventRef, void *);
#ifdef QT_MAC_USE_COCOA
    static void qt_initAfterNSAppStarted();
    static void setupAppleEvents();
#endif
    static bool qt_mac_apply_settings();
#endif

#ifdef Q_WS_QPA
    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

//    static void reportScreenCount(int count);
    static void reportScreenCount(QWindowSystemInterfacePrivate::ScreenCountEvent *e);
//    static void reportGeometryChange(int screenIndex);
    static void reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
//    static void reportAvailableGeometryChange(int screenIndex);
    static void reportAvailableGeometryChange(QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e);
    static void reportLocaleChange();
#endif

#ifdef Q_WS_QWS
    QPointer<QWSManager> last_manager;
    QWSServerCleaner qwsServerCleaner;
# ifndef QT_NO_DIRECTPAINTER
    QMap<WId, QDirectPainter *> *directPainters;
# endif
    QRect maxWindowRect(const QScreen *screen) const { return maxWindowRects[screen]; }
    void setMaxWindowRect(const QScreen *screen, int screenNo, const QRect &rect);
    void setScreenTransformation(QScreen *screen, int screenNo, int transformation);
#endif

    static QApplicationPrivate *instance() { return self; }

    static QString styleOverride;

#ifdef QT_KEYPAD_NAVIGATION
    static QWidget *oldEditFocus;
    static Qt::NavigationMode navigationMode;
#endif

#if defined(Q_WS_MAC) || defined(Q_WS_X11)
    void _q_alertTimeOut();
    QHash<QWidget *, QTimer *> alertTimerHash;
#endif
#ifndef QT_NO_STYLE_STYLESHEET
    static QString styleSheet;
#endif
    static QPointer<QWidget> leaveAfterRelease;
    static QWidget *pickMouseReceiver(QWidget *candidate, const QPoint &globalPos, QPoint &pos,
                                      QEvent::Type type, Qt::MouseButtons buttons,
                                      QWidget *buttonDown, QWidget *alienWidget);
    static bool sendMouseEvent(QWidget *receiver, QMouseEvent *event, QWidget *alienWidget,
                               QWidget *native, QWidget **buttonDown, QPointer<QWidget> &lastMouseReceiver,
                               bool spontaneous = true);
#ifdef Q_OS_SYMBIAN
    static void setNavigationMode(Qt::NavigationMode mode);
    static TUint resolveS60ScanCode(TInt scanCode, TUint keysym);
    QSet<WId> nativeWindows;

    int symbianProcessWsEvent(const QSymbianEvent *symbianEvent);
    int symbianHandleCommand(const QSymbianEvent *symbianEvent);
    int symbianResourceChange(const QSymbianEvent *symbianEvent);
    void symbianHandleLiteModeStartup();

    void _q_aboutToQuit();

    void emitAboutToReleaseGpuResources();
    void emitAboutToUseGpuResources();
#endif
#if defined(Q_WS_WIN) || defined(Q_WS_X11) || defined (Q_WS_QWS) || defined(Q_WS_MAC) || defined(Q_WS_QPA)
    void sendSyntheticEnterLeave(QWidget *widget);
#endif

#ifndef QT_NO_GESTURES
    QGestureManager *gestureManager;
    QWidget *gestureWidget;
#endif
#if defined(Q_WS_X11) || defined(Q_WS_WIN)
    QPixmap *move_cursor;
    QPixmap *copy_cursor;
    QPixmap *link_cursor;
#endif
#if defined(Q_WS_WIN)
    QPixmap *ignore_cursor;
#endif
    QPixmap getPixmapCursor(Qt::CursorShape cshape);

    QMap<int, QWeakPointer<QWidget> > widgetForTouchPointId;
    QMap<int, QTouchEvent::TouchPoint> appCurrentTouchPoints;
    static void updateTouchPointsForWidget(QWidget *widget, QTouchEvent *touchEvent);
    void initializeMultitouch();
    void initializeMultitouch_sys();
    void cleanupMultitouch();
    void cleanupMultitouch_sys();
    int findClosestTouchPointId(const QPointF &screenPos);
    void appendTouchPoint(const QTouchEvent::TouchPoint &touchPoint);
    void removeTouchPoint(int touchPointId);
    static void translateRawTouchEvent(QWidget *widget,
                                       QTouchEvent::DeviceType deviceType,
                                       const QList<QTouchEvent::TouchPoint> &touchPoints);

#if defined(Q_WS_WIN)
    static bool HasTouchSupport;
    static PtrRegisterTouchWindow RegisterTouchWindow;
    static PtrGetTouchInputInfo GetTouchInputInfo;
    static PtrCloseTouchInputHandle CloseTouchInputHandle;

    QHash<DWORD, int> touchInputIDToTouchPointID;
    bool translateTouchEvent(const MSG &msg);

#ifndef QT_NO_GESTURES
    PtrGetGestureInfo GetGestureInfo;
    PtrGetGestureExtraArgs GetGestureExtraArgs;
    PtrCloseGestureInfoHandle CloseGestureInfoHandle;
    PtrSetGestureConfig SetGestureConfig;
    PtrGetGestureConfig GetGestureConfig;
    PtrBeginPanningFeedback BeginPanningFeedback;
    PtrUpdatePanningFeedback UpdatePanningFeedback;
    PtrEndPanningFeedback EndPanningFeedback;
#endif // QT_NO_GESTURES
#endif

#ifdef QT_RX71_MULTITOUCH
    bool hasRX71MultiTouch;

    struct RX71TouchPointState {
        QSocketNotifier *socketNotifier;
        QTouchEvent::TouchPoint touchPoint;

        int minX, maxX, scaleX;
        int minY, maxY, scaleY;
        int minZ, maxZ;
    };
    QList<RX71TouchPointState> allRX71TouchPoints;

    bool readRX71MultiTouchEvents(int deviceNumber);
    void fakeMouseEventFromRX71TouchEvent();
    void _q_readRX71MultiTouchEvents();
#endif

#if defined(Q_OS_SYMBIAN)
    int pressureSupported;
    int maxTouchPressure;
    QList<QTouchEvent::TouchPoint> appAllTouchPoints;

    bool useTranslucentEGLSurfaces;
#endif

private:
#ifdef Q_WS_QWS
    QMap<const QScreen*, QRect> maxWindowRects;
#endif

#ifdef Q_OS_SYMBIAN
    QHash<TInt, TUint> scanCodeCache;
#endif

    static QApplicationPrivate *self;

    static void giveFocusAccordingToFocusPolicy(QWidget *w,
                                                Qt::FocusPolicy focusPolicy,
                                                Qt::FocusReason focusReason);
    static bool shouldSetFocus(QWidget *w, Qt::FocusPolicy policy);


    static bool isAlien(QWidget *);
};

Q_GUI_EXPORT void qt_translateRawTouchEvent(QWidget *window,
                                            QTouchEvent::DeviceType deviceType,
                                            const QList<QTouchEvent::TouchPoint> &touchPoints);

#if defined(Q_WS_WIN)
  extern void qt_win_set_cursor(QWidget *, bool);
#elif defined(Q_WS_X11)
  extern void qt_x11_enforce_cursor(QWidget *, bool);
  extern void qt_x11_enforce_cursor(QWidget *);
#elif defined(Q_OS_SYMBIAN)
  extern void qt_symbian_set_cursor(QWidget *, bool);
#elif defined (Q_WS_QPA)
  extern void qt_qpa_set_cursor(QWidget *, bool);
#endif

QT_END_NAMESPACE

#endif // QAPPLICATION_P_H
