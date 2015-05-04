/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QWINDOWSYSTEMINTERFACE_H
#define QWINDOWSYSTEMINTERFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/QTime>
#include <QtGui/qwindowdefs.h>
#include <QtCore/QEvent>
#include <QtCore/QAbstractEventDispatcher>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtCore/QWeakPointer>
#include <QtCore/QMutex>
#include <QtGui/QTouchEvent>
#include <QtCore/QEventLoop>
#include <QtGui/QVector2D>

QT_BEGIN_NAMESPACE

class QMimeData;
class QTouchDevice;
class QPlatformDragQtResponse;
class QPlatformDropQtResponse;


class Q_GUI_EXPORT QWindowSystemInterface
{
public:
    static void handleMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    static void handleMouseEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                 Qt::KeyboardModifiers mods = Qt::NoModifier,
                                 Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    static void handleFrameStrutMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                           Qt::KeyboardModifiers mods = Qt::NoModifier,
                                           Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);
    static void handleFrameStrutMouseEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                           Qt::KeyboardModifiers mods = Qt::NoModifier,
                                           Qt::MouseEventSource source = Qt::MouseEventNotSynthesized);

    static bool tryHandleShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                  const QString & text = QString(), bool autorep = false, ushort count = 1);
    static bool tryHandleShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                  const QString & text = QString(), bool autorep = false, ushort count = 1);

    static bool tryHandleExtendedShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                          quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                          const QString & text = QString(), bool autorep = false, ushort count = 1);
    static bool tryHandleExtendedShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                          quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                          const QString & text = QString(), bool autorep = false, ushort count = 1);

    static void handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);
    static void handleKeyEvent(QWindow *w, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);

    static void handleExtendedKeyEvent(QWindow *w, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static void handleExtendedKeyEvent(QWindow *w, ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static void handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods = Qt::NoModifier, Qt::ScrollPhase phase = Qt::ScrollUpdate);
    static void handleWheelEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods = Qt::NoModifier, Qt::ScrollPhase phase = Qt::ScrollUpdate);

    // Wheel event compatibility functions. Will be removed: do not use.
    static void handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods = Qt::NoModifier);
    static void handleWheelEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods = Qt::NoModifier);

    struct TouchPoint {
        TouchPoint() : id(0), pressure(0), state(Qt::TouchPointStationary), flags(0) { }
        int id;                 // for application use
        QPointF normalPosition; // touch device coordinates, (0 to 1, 0 to 1)
        QRectF area;            // the touched area, centered at position in screen coordinates
        qreal pressure;         // 0 to 1
        Qt::TouchPointState state; //Qt::TouchPoint{Pressed|Moved|Stationary|Released}
        QVector2D velocity;     // in screen coordinate system, pixels / seconds
        QTouchEvent::TouchPoint::InfoFlags flags;
        QVector<QPointF> rawPositions; // in screen coordinates
    };

    static void registerTouchDevice(QTouchDevice *device);
    static void handleTouchEvent(QWindow *w, QTouchDevice *device,
                                 const QList<struct TouchPoint> &points, Qt::KeyboardModifiers mods = Qt::NoModifier);
    static void handleTouchEvent(QWindow *w, ulong timestamp, QTouchDevice *device,
                                 const QList<struct TouchPoint> &points, Qt::KeyboardModifiers mods = Qt::NoModifier);
    static void handleTouchCancelEvent(QWindow *w, QTouchDevice *device, Qt::KeyboardModifiers mods = Qt::NoModifier);
    static void handleTouchCancelEvent(QWindow *w, ulong timestamp, QTouchDevice *device, Qt::KeyboardModifiers mods = Qt::NoModifier);

    // rect is relative to parent
    static void handleGeometryChange(QWindow *w, const QRect &newRect, const QRect &oldRect = QRect());
    static void handleCloseEvent(QWindow *w, bool *accepted = 0);
    static void handleEnterEvent(QWindow *w, const QPointF &local = QPointF(), const QPointF& global = QPointF());
    static void handleLeaveEvent(QWindow *w);
    static void handleEnterLeaveEvent(QWindow *enter, QWindow *leave, const QPointF &local = QPointF(), const QPointF& global = QPointF());
    static void handleWindowActivated(QWindow *w, Qt::FocusReason r = Qt::OtherFocusReason);

    static void handleWindowStateChanged(QWindow *w, Qt::WindowState newState);
    static void handleWindowScreenChanged(QWindow *w, QScreen *newScreen);

    static void handleApplicationStateChanged(Qt::ApplicationState newState, bool forcePropagate = false);

    // region is in local coordinates, do not confuse with geometry which is parent-relative
    static void handleExposeEvent(QWindow *tlw, const QRegion &region);

#ifndef QT_NO_DRAGANDDROP
    // Drag and drop. These events are sent immediately.
    static QPlatformDragQtResponse handleDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);
    static QPlatformDropQtResponse handleDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions);
#endif

    static bool handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result);

    // Changes to the screen
    static void handleScreenOrientationChange(QScreen *screen, Qt::ScreenOrientation newOrientation);
    static void handleScreenGeometryChange(QScreen *screen, const QRect &newGeometry, const QRect &newAvailableGeometry);
    static void handleScreenLogicalDotsPerInchChange(QScreen *screen, qreal newDpiX, qreal newDpiY);
    static void handleScreenRefreshRateChange(QScreen *screen, qreal newRefreshRate);

    static void handleThemeChange(QWindow *tlw);

    static void handleFileOpenEvent(const QString& fileName);
    static void handleFileOpenEvent(const QUrl &url);

    static void handleTabletEvent(QWindow *w, ulong timestamp, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, int xTilt, int yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static void handleTabletEvent(QWindow *w, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, int xTilt, int yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    static void handleTabletEvent(QWindow *w, ulong timestamp, bool down, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier); // ### remove in Qt 6
    static void handleTabletEvent(QWindow *w, bool down, const QPointF &local, const QPointF &global,
                                  int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                  qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                  Qt::KeyboardModifiers modifiers = Qt::NoModifier); // ### remove in Qt 6
    static void handleTabletEnterProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid);
    static void handleTabletEnterProximityEvent(int device, int pointerType, qint64 uid);
    static void handleTabletLeaveProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid);
    static void handleTabletLeaveProximityEvent(int device, int pointerType, qint64 uid);

#ifndef QT_NO_GESTURES
    static void handleGestureEvent(QWindow *window,  ulong timestamp, Qt::NativeGestureType type,
                                   QPointF &local, QPointF &global);
    static void handleGestureEventWithRealValue(QWindow *window,  ulong timestamp, Qt::NativeGestureType type,
                                                qreal value, QPointF &local, QPointF &global);
    static void handleGestureEventWithSequenceIdAndValue(QWindow *window, ulong timestamp,Qt::NativeGestureType type,
                                                         ulong sequenceId, quint64 value, QPointF &local, QPointF &global);
#endif // QT_NO_GESTURES

    static void handlePlatformPanelEvent(QWindow *w);
#ifndef QT_NO_CONTEXTMENU
    static void handleContextMenuEvent(QWindow *w, bool mouseTriggered,
                                       const QPoint &pos, const QPoint &globalPos,
                                       Qt::KeyboardModifiers modifiers);
#endif
#ifndef QT_NO_WHATSTHIS
    static void handleEnterWhatsThisEvent();
#endif

    // For event dispatcher implementations
    static bool sendWindowSystemEvents(QEventLoop::ProcessEventsFlags flags);
    static void setSynchronousWindowsSystemEvents(bool enable);
    static void flushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void deferredFlushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags);
    static int windowSystemEventsQueued();
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QWindowSystemInterface::TouchPoint &p);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSYSTEMINTERFACE_H
