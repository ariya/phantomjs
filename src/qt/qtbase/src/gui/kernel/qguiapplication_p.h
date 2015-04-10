/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QGUIAPPLICATION_P_H
#define QGUIAPPLICATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qguiapplication.h>

#include <QtCore/QPointF>
#include <QtCore/private/qcoreapplication_p.h>

#include <QtCore/private/qthread_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include "private/qshortcutmap_p.h"
#include <qicon.h>

QT_BEGIN_NAMESPACE

class QPlatformIntegration;
class QPlatformTheme;
class QPlatformDragQtResponse;
struct QDrawHelperGammaTables;
#ifndef QT_NO_DRAGANDDROP
class QDrag;
#endif // QT_NO_DRAGANDDROP

class Q_GUI_EXPORT QGuiApplicationPrivate : public QCoreApplicationPrivate
{
    Q_DECLARE_PUBLIC(QGuiApplication)
public:
    QGuiApplicationPrivate(int &argc, char **argv, int flags);
    ~QGuiApplicationPrivate();

    void createPlatformIntegration();
    void createEventDispatcher() Q_DECL_OVERRIDE;
    void eventDispatcherReady() Q_DECL_OVERRIDE;

    virtual void notifyLayoutDirectionChange();
    virtual void notifyActiveWindowChange(QWindow *previous);

    virtual bool shouldQuit();

    bool shouldQuitInternal(const QWindowList &processedWindows);
    virtual bool tryCloseAllWindows();

    static Qt::KeyboardModifiers modifier_buttons;
    static Qt::MouseButtons mouse_buttons;

    static QPlatformIntegration *platform_integration;

    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }

    static QPlatformTheme *platform_theme;

    static QPlatformTheme *platformTheme()
    { return platform_theme; }

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    {
        if (QCoreApplication::instance())
            return QCoreApplication::instance()->d_func()->threadData->eventDispatcher.load();
        else
            return 0;
    }

    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);
    static void processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *e);
    static void processWindowScreenChangedEvent(QWindowSystemInterfacePrivate::WindowScreenChangedEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void updateFilteredScreenOrientation(QScreen *screen);
    static void reportScreenOrientationChange(QScreen *screen);
    static void reportScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e);
    static void reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
    static void reportAvailableGeometryChange(QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e);
    static void reportLogicalDotsPerInchChange(QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e);
    static void reportRefreshRateChange(QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e);
    static void processThemeChanged(QWindowSystemInterfacePrivate::ThemeChangeEvent *tce);

    static void processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e);

    static void processFileOpenEvent(QWindowSystemInterfacePrivate::FileOpenEvent *e);

    static void processTabletEvent(QWindowSystemInterfacePrivate::TabletEvent *e);
    static void processTabletEnterProximityEvent(QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e);
    static void processTabletLeaveProximityEvent(QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e);

#ifndef QT_NO_GESTURES
    static void processGestureEvent(QWindowSystemInterfacePrivate::GestureEvent *e);
#endif

    static void processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e);
#ifndef QT_NO_CONTEXTMENU
    static void processContextMenuEvent(QWindowSystemInterfacePrivate::ContextMenuEvent *e);
#endif

#ifndef QT_NO_DRAGANDDROP
    static QPlatformDragQtResponse processDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);
    static QPlatformDropQtResponse processDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);
#endif

    static bool processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result);

    static void sendQWindowEventToQPlatformWindow(QWindow *window, QEvent *event);

    static inline Qt::Alignment visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
    {
        if (!(alignment & Qt::AlignHorizontal_Mask))
            alignment |= Qt::AlignLeft;
        if (!(alignment & Qt::AlignAbsolute) && (alignment & (Qt::AlignLeft | Qt::AlignRight))) {
            if (direction == Qt::RightToLeft)
                alignment ^= (Qt::AlignLeft | Qt::AlignRight);
            alignment |= Qt::AlignAbsolute;
        }
        return alignment;
    }

    static void emitLastWindowClosed();

    QPixmap getPixmapCursor(Qt::CursorShape cshape);

    void _q_updateFocusObject(QObject *object);

    static QGuiApplicationPrivate *instance() { return self; }

    static QIcon *app_icon;
    static QString *platform_name;
    static QString *displayName;

    QWindowList modalWindowList;
    static void showModalWindow(QWindow *window);
    static void hideModalWindow(QWindow *window);
    static void updateBlockedStatus(QWindow *window);
    virtual bool isWindowBlocked(QWindow *window, QWindow **blockingWindow = 0) const;

    static bool synthesizeMouseFromTouchEventsEnabled();

    static Qt::MouseButtons buttons;
    static ulong mousePressTime;
    static Qt::MouseButton mousePressButton;
    static int mousePressX;
    static int mousePressY;
    static int mouse_double_click_distance;
    static QPointF lastCursorPosition;
    static bool tabletState;
    static QWindow *tabletPressTarget;
    static QWindow *currentMouseWindow;
    static Qt::ApplicationState applicationState;

#ifndef QT_NO_CLIPBOARD
    static QClipboard *qt_clipboard;
#endif

    static QPalette *app_pal;

    static QWindowList window_list;
    static QWindow *focus_window;

#ifndef QT_NO_CURSOR
    QList<QCursor> cursor_list;
#endif
    static QList<QScreen *> screen_list;

    static QFont *app_font;

    QStyleHints *styleHints;
    static bool obey_desktop_settings;
    static bool noGrab;
    QInputMethod *inputMethod;

    QString firstWindowTitle;

    static QList<QObject *> generic_plugin_list;
#ifndef QT_NO_SHORTCUT
    QShortcutMap shortcutMap;
#endif

#ifndef QT_NO_SESSIONMANAGER
    QSessionManager *session_manager;
    bool is_session_restored;
    bool is_saving_session;
    void commitData();
    void saveState();
#endif

    struct ActiveTouchPointsKey {
        ActiveTouchPointsKey(QTouchDevice *dev, int id) : device(dev), touchPointId(id) { }
        QTouchDevice *device;
        int touchPointId;
    };
    struct ActiveTouchPointsValue {
        QPointer<QWindow> window;
        QPointer<QObject> target;
        QTouchEvent::TouchPoint touchPoint;
    };
    QHash<ActiveTouchPointsKey, ActiveTouchPointsValue> activeTouchPoints;
    QEvent::Type lastTouchType;
    struct SynthesizedMouseData {
        SynthesizedMouseData(const QPointF &p, const QPointF &sp, QWindow *w)
            : pos(p), screenPos(sp), window(w) { }
        QPointF pos;
        QPointF screenPos;
        QPointer<QWindow> window;
    };
    QHash<QWindow *, SynthesizedMouseData> synthesizedMousePoints;

    static int mouseEventCaps(QMouseEvent *event);
    static QVector2D mouseEventVelocity(QMouseEvent *event);
    static void setMouseEventCapsAndVelocity(QMouseEvent *event, int caps, const QVector2D &velocity);

    static Qt::MouseEventSource mouseEventSource(const QMouseEvent *event);
    static void setMouseEventSource(QMouseEvent *event, Qt::MouseEventSource source);

    static Qt::MouseEventFlags mouseEventFlags(const QMouseEvent *event);
    static void setMouseEventFlags(QMouseEvent *event, Qt::MouseEventFlags flags);

    const QDrawHelperGammaTables *gammaTables();

    // hook reimplemented in QApplication to apply the QStyle function on the QIcon
    virtual QPixmap applyQIconStyleHelper(QIcon::Mode, const QPixmap &basePixmap) const { return basePixmap; }

    virtual void notifyWindowIconChanged();

    static QRect applyWindowGeometrySpecification(const QRect &windowGeometry, const QWindow *window);

    static void setApplicationState(Qt::ApplicationState state);

protected:
    virtual void notifyThemeChanged();
    bool tryCloseRemainingWindows(QWindowList processedWindows);
#ifndef QT_NO_DRAGANDDROP
    virtual void notifyDragStarted(const QDrag *);
#endif // QT_NO_DRAGANDDROP

private:
    friend class QDragManager;

    void init();

    static QGuiApplicationPrivate *self;
    static QTouchDevice *m_fakeTouchDevice;
    static int m_fakeMouseSourcePointId;
    QAtomicPointer<QDrawHelperGammaTables> m_gammaTables;
};

Q_GUI_EXPORT uint qHash(const QGuiApplicationPrivate::ActiveTouchPointsKey &k);

Q_GUI_EXPORT bool operator==(const QGuiApplicationPrivate::ActiveTouchPointsKey &a,
                             const QGuiApplicationPrivate::ActiveTouchPointsKey &b);

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_P_H
