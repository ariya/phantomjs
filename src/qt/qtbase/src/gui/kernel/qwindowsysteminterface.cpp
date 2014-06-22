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
#include "qwindowsysteminterface.h"
#include <qpa/qplatformwindow.h>
#include "qwindowsysteminterface_p.h"
#include "private/qguiapplication_p.h"
#include "private/qevent_p.h"
#include "private/qtouchdevice_p.h"
#include <QAbstractEventDispatcher>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE


QElapsedTimer QWindowSystemInterfacePrivate::eventTime;
bool QWindowSystemInterfacePrivate::synchronousWindowsSystemEvents = false;
QWaitCondition QWindowSystemInterfacePrivate::eventsFlushed;
QMutex QWindowSystemInterfacePrivate::flushEventMutex;

//------------------------------------------------------------
//
// Callback functions for plugins:
//

QWindowSystemInterfacePrivate::WindowSystemEventList QWindowSystemInterfacePrivate::windowSystemEventQueue;

extern QPointer<QWindow> qt_last_mouse_receiver;

/*!
    \class QWindowSystemInterface
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QWindowSystemInterface provides an event queue for the QPA platform.

    The platform plugins call the various functions to notify about events. The events are queued
    until sendWindowSystemEvents() is called by the event dispatcher.
*/

void QWindowSystemInterface::handleEnterEvent(QWindow *tlw, const QPointF &local, const QPointF &global)
{
    if (tlw) {
        QWindowSystemInterfacePrivate::EnterEvent *e = new QWindowSystemInterfacePrivate::EnterEvent(tlw, local, global);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
    }
}

void QWindowSystemInterface::handleLeaveEvent(QWindow *tlw)
{
    QWindowSystemInterfacePrivate::LeaveEvent *e = new QWindowSystemInterfacePrivate::LeaveEvent(tlw);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

/*!
    This method can be used to ensure leave and enter events are both in queue when moving from
    one QWindow to another. This allows QWindow subclasses to check for a queued enter event
    when handling the leave event (\c QWindowSystemInterfacePrivate::peekWindowSystemEvent) to
    determine where mouse went and act accordingly. E.g. QWidgetWindow needs to know if mouse
    cursor moves between windows in same window hierarchy.
*/
void QWindowSystemInterface::handleEnterLeaveEvent(QWindow *enter, QWindow *leave, const QPointF &local, const QPointF& global)
{
    bool wasSynchronous = QWindowSystemInterfacePrivate::synchronousWindowsSystemEvents;
    if (wasSynchronous)
        setSynchronousWindowsSystemEvents(false);
    handleLeaveEvent(leave);
    handleEnterEvent(enter, local, global);
    if (wasSynchronous) {
        flushWindowSystemEvents();
        setSynchronousWindowsSystemEvents(true);
    }
}

void QWindowSystemInterface::handleWindowActivated(QWindow *tlw, Qt::FocusReason r)
{
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *e =
        new QWindowSystemInterfacePrivate::ActivatedWindowEvent(tlw, r);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWindowStateChanged(QWindow *tlw, Qt::WindowState newState)
{
    QWindowSystemInterfacePrivate::WindowStateChangedEvent *e =
        new QWindowSystemInterfacePrivate::WindowStateChangedEvent(tlw, newState);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWindowScreenChanged(QWindow *tlw, QScreen *screen)
{
    QWindowSystemInterfacePrivate::WindowScreenChangedEvent *e =
        new QWindowSystemInterfacePrivate::WindowScreenChangedEvent(tlw, screen);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState newState)
{
    Q_ASSERT(QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState));
    QWindowSystemInterfacePrivate::ApplicationStateChangedEvent *e =
        new QWindowSystemInterfacePrivate::ApplicationStateChangedEvent(newState);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

/*!
  If \a oldRect is null, Qt will use the previously reported geometry instead.
 */
void QWindowSystemInterface::handleGeometryChange(QWindow *tlw, const QRect &newRect, const QRect &oldRect)
{
    QWindowSystemInterfacePrivate::GeometryChangeEvent *e = new QWindowSystemInterfacePrivate::GeometryChangeEvent(tlw,newRect, oldRect);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleCloseEvent(QWindow *tlw, bool *accepted)
{
    if (tlw) {
        QWindowSystemInterfacePrivate::CloseEvent *e =
                new QWindowSystemInterfacePrivate::CloseEvent(tlw, accepted);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
    }
}

/*!

\a w == 0 means that the event is in global coords only, \a local will be ignored in this case

*/
void QWindowSystemInterface::handleMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                              Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleMouseEvent(w, time, local, global, b, mods, source);
}

void QWindowSystemInterface::handleMouseEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                              Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    QWindowSystemInterfacePrivate::MouseEvent * e =
        new QWindowSystemInterfacePrivate::MouseEvent(w, timestamp, local, global, b, mods, source);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                                        Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    const unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleFrameStrutMouseEvent(w, time, local, global, b, mods, source);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *w, ulong timestamp, const QPointF & local, const QPointF & global, Qt::MouseButtons b,
                                                        Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    QWindowSystemInterfacePrivate::MouseEvent * e =
            new QWindowSystemInterfacePrivate::MouseEvent(w, timestamp,
                                                          QWindowSystemInterfacePrivate::FrameStrutMouse,
                                                          local, global, b, mods, source);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

bool QWindowSystemInterface::tryHandleShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                               const QString & text, bool autorep, ushort count)
{
    unsigned long timestamp = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return tryHandleShortcutEvent(w, timestamp, k, mods, text, autorep, count);
}

bool QWindowSystemInterface::tryHandleShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                               const QString & text, bool autorep, ushort count)
{
#ifndef QT_NO_SHORTCUT
    QGuiApplicationPrivate::modifier_buttons = mods;

    QKeyEvent qevent(QEvent::ShortcutOverride, k, mods, text, autorep, count);
    qevent.setTimestamp(timestamp);
    return QGuiApplicationPrivate::instance()->shortcutMap.tryShortcutEvent(w, &qevent);
#else
    Q_UNUSED(w)
    Q_UNUSED(timestamp)
    Q_UNUSED(k)
    Q_UNUSED(mods)
    Q_UNUSED(text)
    Q_UNUSED(autorep)
    Q_UNUSED(count)
    return false;
#endif
}

bool QWindowSystemInterface::tryHandleExtendedShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                                       quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                                       const QString &text, bool autorep, ushort count)
{
    unsigned long timestamp = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return tryHandleExtendedShortcutEvent(w, timestamp, k, mods, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
}

bool QWindowSystemInterface::tryHandleExtendedShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                                       quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                                       const QString &text, bool autorep, ushort count)
{
#ifndef QT_NO_SHORTCUT
    QGuiApplicationPrivate::modifier_buttons = mods;

    QKeyEvent qevent(QEvent::ShortcutOverride, k, mods, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
    qevent.setTimestamp(timestamp);
    return QGuiApplicationPrivate::instance()->shortcutMap.tryShortcutEvent(w, &qevent);
#else
    Q_UNUSED(w)
    Q_UNUSED(timestamp)
    Q_UNUSED(k)
    Q_UNUSED(mods)
    Q_UNUSED(nativeScanCode)
    Q_UNUSED(nativeVirtualKey)
    Q_UNUSED(nativeModifiers)
    Q_UNUSED(text)
    Q_UNUSED(autorep)
    Q_UNUSED(count)
    return false;
#endif
}


void QWindowSystemInterface::handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleKeyEvent(w, time, t, k, mods, text, autorep, count);
}

void QWindowSystemInterface::handleKeyEvent(QWindow *tlw, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count)
{
    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(tlw, timestamp, t, k, mods, text, autorep, count);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleExtendedKeyEvent(QWindow *w, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleExtendedKeyEvent(w, time, type, key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers,
                           text, autorep, count);
}

void QWindowSystemInterface::handleExtendedKeyEvent(QWindow *tlw, ulong timestamp, QEvent::Type type, int key,
                                                    Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count)
{
    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(tlw, timestamp, type, key, modifiers,
                nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleWheelEvent(w, time, local, global, d, o, mods);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *tlw, ulong timestamp, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods)
{
    QPoint point = (o == Qt::Vertical) ? QPoint(0, d) : QPoint(d, 0);
    handleWheelEvent(tlw, timestamp, local, global, QPoint(), point, mods);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleWheelEvent(w, time, local, global, pixelDelta, angleDelta, mods, phase);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *tlw, ulong timestamp, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase)
{
    // Qt 4 sends two separate wheel events for horizontal and vertical
    // deltas. For Qt 5 we want to send the deltas in one event, but at the
    // same time preserve source and behavior compatibility with Qt 4.
    //
    // In addition high-resolution pixel-based deltas are also supported.
    // Platforms that does not support these may pass a null point here.
    // Angle deltas must always be sent in addition to pixel deltas.
    QWindowSystemInterfacePrivate::WheelEvent *e;

    // Pass Qt::ScrollBegin and Qt::ScrollEnd through
    // even if the wheel delta is null.
    if (angleDelta.isNull() && phase == Qt::ScrollUpdate)
        return;

    // Simple case: vertical deltas only:
    if (angleDelta.y() != 0 && angleDelta.x() == 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical, mods, phase);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
        return;
    }

    // Simple case: horizontal deltas only:
    if (angleDelta.y() == 0 && angleDelta.x() != 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.x(), Qt::Horizontal, mods, phase);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
        return;
    }

    // Both horizontal and vertical deltas: Send two wheel events.
    // The first event contains the Qt 5 pixel and angle delta as points,
    // and in addition the Qt 4 compatibility vertical angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical, mods, phase);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);

    // The second event contains null pixel and angle points and the
    // Qt 4 compatibility horizontal angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, QPoint(), QPoint(), angleDelta.x(), Qt::Horizontal, mods, phase);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}


QWindowSystemInterfacePrivate::ExposeEvent::ExposeEvent(QWindow *exposed, const QRegion &region)
    : WindowSystemEvent(Expose)
    , exposed(exposed)
    , isExposed(exposed && exposed->handle() ? exposed->handle()->isExposed() : false)
    , region(region)
{
}

int QWindowSystemInterfacePrivate::windowSystemEventsQueued()
{
    return windowSystemEventQueue.count();
}

QWindowSystemInterfacePrivate::WindowSystemEvent * QWindowSystemInterfacePrivate::getWindowSystemEvent()
{
    return windowSystemEventQueue.takeFirstOrReturnNull();
}

QWindowSystemInterfacePrivate::WindowSystemEvent *QWindowSystemInterfacePrivate::getNonUserInputWindowSystemEvent()
{
    return windowSystemEventQueue.takeFirstNonUserInputOrReturnNull();
}

QWindowSystemInterfacePrivate::WindowSystemEvent *QWindowSystemInterfacePrivate::peekWindowSystemEvent(EventType t)
{
    return windowSystemEventQueue.peekAtFirstOfType(t);
}

void QWindowSystemInterfacePrivate::removeWindowSystemEvent(WindowSystemEvent *event)
{
    windowSystemEventQueue.remove(event);
}

void QWindowSystemInterfacePrivate::handleWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *ev)
{
    if (synchronousWindowsSystemEvents) {
        QGuiApplicationPrivate::processWindowSystemEvent(ev);
    } else {
        windowSystemEventQueue.append(ev);
        QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::qt_qpa_core_dispatcher();
        if (dispatcher)
            dispatcher->wakeUp();
    }
}

void QWindowSystemInterface::registerTouchDevice(QTouchDevice *device)
{
    QTouchDevicePrivate::registerDevice(device);
}

void QWindowSystemInterface::handleTouchEvent(QWindow *w, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchEvent(w, time, device, points, mods);
}

QList<QTouchEvent::TouchPoint> QWindowSystemInterfacePrivate::convertTouchPoints(const QList<QWindowSystemInterface::TouchPoint> &points, QEvent::Type *type)
{
    QList<QTouchEvent::TouchPoint> touchPoints;
    Qt::TouchPointStates states;
    QTouchEvent::TouchPoint p;

    QList<QWindowSystemInterface::TouchPoint>::const_iterator point = points.constBegin();
    QList<QWindowSystemInterface::TouchPoint>::const_iterator end = points.constEnd();
    while (point != end) {
        p.setId(point->id);
        p.setPressure(point->pressure);
        states |= point->state;
        p.setState(point->state);

        const QPointF screenPos = point->area.center();
        p.setScreenPos(screenPos);
        p.setScreenRect(point->area);

        // The local pos and rect are not set, they will be calculated
        // when the event gets processed by QGuiApplication.

        p.setNormalizedPos(point->normalPosition);
        p.setVelocity(point->velocity);
        p.setFlags(point->flags);
        p.setRawScreenPositions(point->rawPositions);

        touchPoints.append(p);
        ++point;
    }

    // Determine the event type based on the combined point states.
    if (type) {
        *type = QEvent::TouchUpdate;
        if (states == Qt::TouchPointPressed)
            *type = QEvent::TouchBegin;
        else if (states == Qt::TouchPointReleased)
            *type = QEvent::TouchEnd;
    }

    return touchPoints;
}

void QWindowSystemInterface::handleTouchEvent(QWindow *tlw, ulong timestamp, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    if (!points.size()) // Touch events must have at least one point
        return;

    if (!QTouchDevicePrivate::isRegistered(device)) // Disallow passing bogus, non-registered devices.
        return;

    QEvent::Type type;
    QList<QTouchEvent::TouchPoint> touchPoints = QWindowSystemInterfacePrivate::convertTouchPoints(points, &type);

    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(tlw, timestamp, type, device, touchPoints, mods);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTouchCancelEvent(QWindow *w, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchCancelEvent(w, time, device, mods);
}

void QWindowSystemInterface::handleTouchCancelEvent(QWindow *w, ulong timestamp, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(w, timestamp, QEvent::TouchCancel, device,
                                                         QList<QTouchEvent::TouchPoint>(), mods);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenOrientationChange(QScreen *screen, Qt::ScreenOrientation orientation)
{
    QWindowSystemInterfacePrivate::ScreenOrientationEvent *e =
            new QWindowSystemInterfacePrivate::ScreenOrientationEvent(screen, orientation);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenGeometryChange(QScreen *screen, const QRect &geometry)
{
    QWindowSystemInterfacePrivate::ScreenGeometryEvent *e =
            new QWindowSystemInterfacePrivate::ScreenGeometryEvent(screen, geometry);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenAvailableGeometryChange(QScreen *screen, const QRect &availableGeometry)
{
    QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e =
            new QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent(screen, availableGeometry);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(QScreen *screen, qreal dpiX, qreal dpiY)
{
    QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e =
            new QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent(screen, dpiX, dpiY);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenRefreshRateChange(QScreen *screen, qreal newRefreshRate)
{
    QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e =
            new QWindowSystemInterfacePrivate::ScreenRefreshRateEvent(screen, newRefreshRate);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleThemeChange(QWindow *tlw)
{
    QWindowSystemInterfacePrivate::ThemeChangeEvent *e = new QWindowSystemInterfacePrivate::ThemeChangeEvent(tlw);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleExposeEvent(QWindow *tlw, const QRegion &region)
{
    QWindowSystemInterfacePrivate::ExposeEvent *e = new QWindowSystemInterfacePrivate::ExposeEvent(tlw, region);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::deferredFlushWindowSystemEvents()
{
    Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    QMutexLocker locker(&QWindowSystemInterfacePrivate::flushEventMutex);
    flushWindowSystemEvents();
    QWindowSystemInterfacePrivate::eventsFlushed.wakeOne();
}

void QWindowSystemInterface::flushWindowSystemEvents()
{
    const int count = QWindowSystemInterfacePrivate::windowSystemEventQueue.count();
    if (!count)
        return;
    if (!QGuiApplication::instance()) {
        qWarning().nospace()
            << "QWindowSystemInterface::flushWindowSystemEvents() invoked after "
               "QGuiApplication destruction, discarding " << count << " events.";
        QWindowSystemInterfacePrivate::windowSystemEventQueue.clear();
        return;
    }
    if (QThread::currentThread() != QGuiApplication::instance()->thread()) {
        QMutexLocker locker(&QWindowSystemInterfacePrivate::flushEventMutex);
        QWindowSystemInterfacePrivate::FlushEventsEvent *e = new QWindowSystemInterfacePrivate::FlushEventsEvent();
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
        QWindowSystemInterfacePrivate::eventsFlushed.wait(&QWindowSystemInterfacePrivate::flushEventMutex);
    } else {
        sendWindowSystemEvents(QEventLoop::AllEvents);
    }
}

bool QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::ProcessEventsFlags flags)
{
    int nevents = 0;

    while (QWindowSystemInterfacePrivate::windowSystemEventsQueued()) {
        QWindowSystemInterfacePrivate::WindowSystemEvent *event =
            (flags & QEventLoop::ExcludeUserInputEvents) ?
                QWindowSystemInterfacePrivate::getNonUserInputWindowSystemEvent() :
                QWindowSystemInterfacePrivate::getWindowSystemEvent();
        if (!event)
            break;
        nevents++;
        QGuiApplicationPrivate::processWindowSystemEvent(event);
        delete event;
    }

    return (nevents > 0);
}

void QWindowSystemInterface::setSynchronousWindowsSystemEvents(bool enable)
{
    QWindowSystemInterfacePrivate::synchronousWindowsSystemEvents = enable;
}

int QWindowSystemInterface::windowSystemEventsQueued()
{
    return QWindowSystemInterfacePrivate::windowSystemEventsQueued();
}

#ifndef QT_NO_DRAGANDDROP
QPlatformDragQtResponse QWindowSystemInterface::handleDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrag(w, dropData, p,supportedActions);
}

QPlatformDropQtResponse QWindowSystemInterface::handleDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrop(w, dropData, p,supportedActions);
}
#endif // QT_NO_DRAGANDDROP

/*!
    \fn static QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
    \brief Passes a native event identified by \a eventType to the \a window.

    \note This function can only be called from the GUI thread.
*/

bool QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
{
    return QGuiApplicationPrivate::processNativeEvent(window, eventType, message, result);
}

void QWindowSystemInterface::handleFileOpenEvent(const QString& fileName)
{
    QWindowSystemInterfacePrivate::FileOpenEvent e(fileName);
    QGuiApplicationPrivate::processWindowSystemEvent(&e);
}

void QWindowSystemInterface::handleFileOpenEvent(const QUrl &url)
{
    QWindowSystemInterfacePrivate::FileOpenEvent e(url);
    QGuiApplicationPrivate::processWindowSystemEvent(&e);
}

void QWindowSystemInterface::handleTabletEvent(QWindow *w, ulong timestamp, bool down, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    QWindowSystemInterfacePrivate::TabletEvent *e =
            new QWindowSystemInterfacePrivate::TabletEvent(w, timestamp, down, local, global, device, pointerType, pressure,
                                                           xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletEvent(QWindow *w, bool down, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletEvent(w, time, down, local, global, device, pointerType, pressure,
                      xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
}

void QWindowSystemInterface::handleTabletEnterProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid)
{
    QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e =
            new QWindowSystemInterfacePrivate::TabletEnterProximityEvent(timestamp, device, pointerType, uid);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletEnterProximityEvent(int device, int pointerType, qint64 uid)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletEnterProximityEvent(time, device, pointerType, uid);
}

void QWindowSystemInterface::handleTabletLeaveProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid)
{
    QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e =
            new QWindowSystemInterfacePrivate::TabletLeaveProximityEvent(timestamp, device, pointerType, uid);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletLeaveProximityEvent(int device, int pointerType, qint64 uid)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletLeaveProximityEvent(time, device, pointerType, uid);
}

#ifndef QT_NO_GESTURES
void QWindowSystemInterface::handleGestureEvent(QWindow *window, ulong timestamp, Qt::NativeGestureType type,
                                                QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, local, global);
       QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleGestureEventWithRealValue(QWindow *window, ulong timestamp, Qt::NativeGestureType type,
                                                                qreal value, QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, local, global);
    e->realValue = value;
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleGestureEventWithSequenceIdAndValue(QWindow *window, ulong timestamp, Qt::NativeGestureType type,
                                                                         ulong sequenceId, quint64 value, QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, local, global);
    e->sequenceId = sequenceId;
    e->intValue = value;
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif // QT_NO_GESTURES

void QWindowSystemInterface::handlePlatformPanelEvent(QWindow *w)
{
    QWindowSystemInterfacePrivate::PlatformPanelEvent *e =
            new QWindowSystemInterfacePrivate::PlatformPanelEvent(w);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

#ifndef QT_NO_CONTEXTMENU
void QWindowSystemInterface::handleContextMenuEvent(QWindow *w, bool mouseTriggered,
                                                    const QPoint &pos, const QPoint &globalPos,
                                                    Qt::KeyboardModifiers modifiers)
{
    QWindowSystemInterfacePrivate::ContextMenuEvent *e =
            new QWindowSystemInterfacePrivate::ContextMenuEvent(w, mouseTriggered, pos,
                                                                globalPos, modifiers);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif

#ifndef QT_NO_WHATSTHIS
void QWindowSystemInterface::handleEnterWhatsThisEvent()
{
    QWindowSystemInterfacePrivate::WindowSystemEvent *e =
            new QWindowSystemInterfacePrivate::WindowSystemEvent(QWindowSystemInterfacePrivate::EnterWhatsThisMode);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QWindowSystemInterface::TouchPoint &p) {
    dbg.nospace() << "TouchPoint(" << p.id << " @" << p.normalPosition << " press " << p.pressure << " vel " << p.velocity << " state " << (int)p.state;
    return dbg.space();
}
#endif

Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b, Qt::KeyboardModifiers mods = Qt::NoModifier) {
    QWindowSystemInterface::handleMouseEvent(w, local, global,  b,  mods);
}

Q_GUI_EXPORT void qt_handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1)
{
    QWindowSystemInterface::handleKeyEvent(w, t, k, mods, text, autorep, count);
}

static QWindowSystemInterface::TouchPoint touchPoint(const QTouchEvent::TouchPoint& pt)
{
    QWindowSystemInterface::TouchPoint p;
    p.id = pt.id();
    p.flags = pt.flags();
    p.normalPosition = pt.normalizedPos();
    p.area = pt.screenRect();
    p.pressure = pt.pressure();
    p.state = pt.state();
    p.velocity = pt.velocity();
    p.rawPositions = pt.rawScreenPositions();
    return p;
}
static QList<struct QWindowSystemInterface::TouchPoint> touchPointList(const QList<QTouchEvent::TouchPoint>& pointList)
{
    QList<struct QWindowSystemInterface::TouchPoint> newList;

    Q_FOREACH (QTouchEvent::TouchPoint p, pointList)
    {
        newList.append(touchPoint(p));
    }
    return newList;
}

Q_GUI_EXPORT  void qt_handleTouchEvent(QWindow *w, QTouchDevice *device,
                                const QList<QTouchEvent::TouchPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    QWindowSystemInterface::handleTouchEvent(w, device, touchPointList(points), mods);
}

QT_END_NAMESPACE
