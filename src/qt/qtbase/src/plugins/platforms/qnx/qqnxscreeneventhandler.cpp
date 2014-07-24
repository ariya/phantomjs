/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglobal.h"

#include "qqnxscreeneventhandler.h"
#if defined(QQNX_SCREENEVENTTHREAD)
#include "qqnxscreeneventthread.h"
#endif
#include "qqnxintegration.h"
#include "qqnxkeytranslator.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventfilter.h"

#include <QDebug>
#include <QGuiApplication>

#include <errno.h>
#include <sys/keycodes.h>

#if defined(QQNXSCREENEVENT_DEBUG)
#define qScreenEventDebug qDebug
#else
#define qScreenEventDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxScreenEventHandler::QQnxScreenEventHandler(QQnxIntegration *integration)
    : m_qnxIntegration(integration)
    , m_lastButtonState(Qt::NoButton)
    , m_lastMouseWindow(0)
    , m_touchDevice(0)
#if defined(QQNX_SCREENEVENTTHREAD)
    , m_eventThread(0)
#endif
    , m_focusLostTimer(-1)
{
    // Create a touch device
    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    // initialize array of touch points
    for (int i = 0; i < MaximumTouchPoints; i++) {

        // map array index to id
        m_touchPoints[i].id = i;

        // pressure is not supported - use default
        m_touchPoints[i].pressure = 1.0;

        // nothing touching
        m_touchPoints[i].state = Qt::TouchPointReleased;
    }
}

void QQnxScreenEventHandler::addScreenEventFilter(QQnxScreenEventFilter *filter)
{
    m_eventFilters.append(filter);
}

void QQnxScreenEventHandler::removeScreenEventFilter(QQnxScreenEventFilter *filter)
{
    m_eventFilters.removeOne(filter);
}

bool QQnxScreenEventHandler::handleEvent(screen_event_t event)
{
    // get the event type
    int qnxType;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &qnxType),
                        "Failed to query event type");

    return handleEvent(event, qnxType);
}

bool QQnxScreenEventHandler::handleEvent(screen_event_t event, int qnxType)
{
    switch (qnxType) {
    case SCREEN_EVENT_MTOUCH_TOUCH:
    case SCREEN_EVENT_MTOUCH_MOVE:
    case SCREEN_EVENT_MTOUCH_RELEASE:
        handleTouchEvent(event, qnxType);
        break;

    case SCREEN_EVENT_KEYBOARD:
        handleKeyboardEvent(event);
        break;

    case SCREEN_EVENT_POINTER:
        handlePointerEvent(event);
        break;

    case SCREEN_EVENT_CREATE:
        handleCreateEvent(event);
        break;

    case SCREEN_EVENT_CLOSE:
        handleCloseEvent(event);
        break;

    case SCREEN_EVENT_DISPLAY:
        handleDisplayEvent(event);
        break;

    case SCREEN_EVENT_PROPERTY:
        handlePropertyEvent(event);
        break;

    default:
        // event ignored
        qScreenEventDebug() << Q_FUNC_INFO << "unknown event" << qnxType;
        return false;
    }

    return true;
}

void QQnxScreenEventHandler::injectKeyboardEvent(int flags, int sym, int modifiers, int scan, int cap)
{
    Q_UNUSED(scan);

    Qt::KeyboardModifiers qtMod = Qt::NoModifier;
    if (modifiers & KEYMOD_SHIFT)
        qtMod |= Qt::ShiftModifier;
    if (modifiers & KEYMOD_CTRL)
        qtMod |= Qt::ControlModifier;
    if (modifiers & KEYMOD_ALT)
        qtMod |= Qt::AltModifier;

    // determine event type
    QEvent::Type type = (flags & KEY_DOWN) ? QEvent::KeyPress : QEvent::KeyRelease;

    // Check if the key cap is valid
    if (flags & KEY_CAP_VALID) {
        Qt::Key key;
        QString keyStr;

        if (cap >= 0x20 && cap <= 0x0ff) {
            key = Qt::Key(std::toupper(cap));   // Qt expects the CAP to be upper case.

            if ( qtMod & Qt::ControlModifier ) {
                keyStr = QChar((int)(key & 0x3f));
            } else {
                if (flags & KEY_SYM_VALID)
                    keyStr = QChar(sym);
            }
        } else if ((cap > 0x0ff && cap < UNICODE_PRIVATE_USE_AREA_FIRST) || cap > UNICODE_PRIVATE_USE_AREA_LAST) {
            key = (Qt::Key)cap;
            keyStr = QChar(sym);
        } else {
            if (isKeypadKey(cap))
                qtMod |= Qt::KeypadModifier; // Is this right?
            key = keyTranslator(cap);
        }

        QWindowSystemInterface::handleExtendedKeyEvent(QGuiApplication::focusWindow(), type, key, qtMod,
                scan, sym, modifiers, keyStr);
        qScreenEventDebug() << Q_FUNC_INFO << "Qt key t=" << type << ", k=" << key << ", s=" << keyStr;
    }
}

#if defined(QQNX_SCREENEVENTTHREAD)
void QQnxScreenEventHandler::setScreenEventThread(QQnxScreenEventThread *eventThread)
{
    m_eventThread = eventThread;
}

void QQnxScreenEventHandler::processEventsFromScreenThread()
{
    if (!m_eventThread)
        return;

    QQnxScreenEventArray *events = m_eventThread->lock();

    for (int i = 0; i < events->size(); ++i) {
        screen_event_t event = events->at(i);
        if (!event)
            continue;
        (*events)[i] = 0;

        m_eventThread->unlock();

        long result = 0;
        QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
        bool handled = dispatcher && dispatcher->filterNativeEvent(QByteArrayLiteral("screen_event_t"), event, &result);
        if (!handled)
            handleEvent(event);
        screen_destroy_event(event);

        m_eventThread->lock();
    }

    events->clear();

    m_eventThread->unlock();
}
#endif

void QQnxScreenEventHandler::handleKeyboardEvent(screen_event_t event)
{
    // get flags of key event
    int flags;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_FLAGS, &flags),
                        "Failed to query event flags");

    // get key code
    int sym;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SYM, &sym),
                        "Failed to query event sym");

    int modifiers;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_MODIFIERS, &modifiers),
                        "Failed to query event modifieres");

    int scan;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SCAN, &scan),
                        "Failed to query event scan");

    int cap;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &cap),
                        "Failed to query event cap");

    int sequenceId = 0;
#if defined(Q_OS_BLACKBERRY)
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SEQUENCE_ID, &sequenceId),
            "Failed to query event seqId");
#endif

    bool inject = true;
    Q_FOREACH (QQnxScreenEventFilter *filter, m_eventFilters) {
        if (filter->handleKeyboardEvent(flags, sym, modifiers, scan, cap, sequenceId)) {
            inject = false;
            break;
        }
    }

    if (inject)
        injectKeyboardEvent(flags, sym, modifiers, scan, cap);
}

void QQnxScreenEventHandler::handlePointerEvent(screen_event_t event)
{
    errno = 0;

    // Query the window that was clicked
    screen_window_t qnxWindow;
    void *handle;
    Q_SCREEN_CHECKERROR(screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle),
                        "Failed to query event window");

    qnxWindow = static_cast<screen_window_t>(handle);

    // Query the button states
    int buttonState = 0;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttonState),
                        "Failed to query event button state");

    // Query the window position
    int windowPos[2];
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos),
            "Failed to query event window position");

    // Query the screen position
    int pos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos),
                        "Failed to query event position");

    // Query the wheel delta
    int wheelDelta = 0;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_WHEEL, &wheelDelta),
            "Failed to query event wheel delta");

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::window(qnxWindow);

    // Generate enter and leave events as needed.
    if (qnxWindow != m_lastMouseWindow) {
        QWindow *wOld = QQnxIntegration::window(m_lastMouseWindow);

        if (wOld) {
            QWindowSystemInterface::handleLeaveEvent(wOld);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt leave, w=" << wOld;
        }

        if (w) {
            QWindowSystemInterface::handleEnterEvent(w);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt enter, w=" << w;
        }
    }

    m_lastMouseWindow = qnxWindow;

    // Apply scaling to wheel delta and invert value for Qt. We'll probably want to scale
    // this via a system preference at some point. But for now this is a sane value and makes
    // the wheel usable.
    wheelDelta *= -10;

    // convert point to local coordinates
    QPoint globalPoint(pos[0], pos[1]);
    QPoint localPoint(windowPos[0], windowPos[1]);

    // Convert buttons.
    // Some QNX header files invert 'Right Button versus "Left Button' ('Right' == 0x01). But they also offer a 'Button Swap' bit,
    // so we may receive events as shown. (If this is wrong, the fix is easy.)
    // QNX Button mask is 8 buttons wide, with a maximum value of x080.
    Qt::MouseButtons buttons = Qt::NoButton;
    if (buttonState & 0x01)
        buttons |= Qt::LeftButton;
    if (buttonState & 0x02)
        buttons |= Qt::MidButton;
    if (buttonState & 0x04)
        buttons |= Qt::RightButton;
    if (buttonState & 0x08)
        buttons |= Qt::ExtraButton1;    // AKA 'Qt::BackButton'
    if (buttonState & 0x10)
        buttons |= Qt::ExtraButton2;    // AKA 'Qt::ForwardButton'
    if (buttonState & 0x20)
        buttons |= Qt::ExtraButton3;
    if (buttonState & 0x40)
        buttons |= Qt::ExtraButton4;
    if (buttonState & 0x80)
        buttons |= Qt::ExtraButton5;

    if (w) {
        // Inject mouse event into Qt only if something has changed.
        if (m_lastGlobalMousePoint != globalPoint ||
            m_lastLocalMousePoint != localPoint ||
            m_lastButtonState != buttons) {
            QWindowSystemInterface::handleMouseEvent(w, localPoint, globalPoint, buttons);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt mouse, w=" << w << ", (" << localPoint.x() << "," << localPoint.y() << "), b=" << static_cast<int>(buttons);
        }

        if (wheelDelta) {
            // Screen only supports a single wheel, so we will assume Vertical orientation for
            // now since that is pretty much standard.
            QWindowSystemInterface::handleWheelEvent(w, localPoint, globalPoint, wheelDelta, Qt::Vertical);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt wheel, w=" << w << ", (" << localPoint.x() << "," << localPoint.y() << "), d=" << static_cast<int>(wheelDelta);
        }
    }

    m_lastGlobalMousePoint = globalPoint;
    m_lastLocalMousePoint = localPoint;
    m_lastButtonState = buttons;
}

void QQnxScreenEventHandler::handleTouchEvent(screen_event_t event, int qnxType)
{
    // get display coordinates of touch
    int pos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos),
                        "Failed to query event position");

    QCursor::setPos(pos[0], pos[1]);

    // get window coordinates of touch
    int windowPos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos),
                        "Failed to query event window position");

    // determine which finger touched
    int touchId;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, &touchId),
                        "Failed to query event touch id");

    // determine which window was touched
    void *handle;
    Q_SCREEN_CHECKERROR(screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle),
                        "Failed to query event window");

    errno = 0;
    int touchArea[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SIZE, touchArea),
                        "Failed to query event touch area");

    int touchPressure;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_PRESSURE, &touchPressure),
            "Failed to query event touch pressure");

    screen_window_t qnxWindow = static_cast<screen_window_t>(handle);

    // check if finger is valid
    if (touchId < MaximumTouchPoints) {

        // Map window handle to top-level QWindow
        QWindow *w = QQnxIntegration::window(qnxWindow);

        // Generate enter and leave events as needed.
        if (qnxWindow != m_lastMouseWindow) {
            QWindow *wOld = QQnxIntegration::window(m_lastMouseWindow);

            if (wOld) {
                QWindowSystemInterface::handleLeaveEvent(wOld);
                qScreenEventDebug() << Q_FUNC_INFO << "Qt leave, w=" << wOld;
            }

            if (w) {
                QWindowSystemInterface::handleEnterEvent(w);
                qScreenEventDebug() << Q_FUNC_INFO << "Qt enter, w=" << w;
            }
        }
        m_lastMouseWindow = qnxWindow;

        if (w) {
            // get size of screen which contains window
            QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWindow(w);
            QSizeF screenSize = platformScreen->geometry().size();

            // update cached position of current touch point
            m_touchPoints[touchId].normalPosition =
                            QPointF(static_cast<qreal>(pos[0]) / screenSize.width(),
                                    static_cast<qreal>(pos[1]) / screenSize.height());

            m_touchPoints[touchId].area = QRectF(w->geometry().left() + windowPos[0] - (touchArea[0]>>1),
                                                 w->geometry().top()  + windowPos[1] - (touchArea[1]>>1),
                                                 (touchArea[0]>>1), (touchArea[1]>>1));
            QWindow *parent = w->parent();
            while (parent) {
                m_touchPoints[touchId].area.translate(parent->geometry().topLeft());
                parent = parent->parent();
            }

            //Qt expects the pressure between 0 and 1. There is however no definit upper limit for
            //the integer value of touch event pressure. The 200 was determined by experiment, it
            //usually does not get higher than that.
            m_touchPoints[touchId].pressure = static_cast<qreal>(touchPressure)/200.0;
            // Can happen, because there is no upper limit for pressure
            if (m_touchPoints[touchId].pressure > 1)
                m_touchPoints[touchId].pressure = 1;

            // determine event type and update state of current touch point
            QEvent::Type type = QEvent::None;
            switch (qnxType) {
            case SCREEN_EVENT_MTOUCH_TOUCH:
                m_touchPoints[touchId].state = Qt::TouchPointPressed;
                type = QEvent::TouchBegin;
                break;
            case SCREEN_EVENT_MTOUCH_MOVE:
                m_touchPoints[touchId].state = Qt::TouchPointMoved;
                type = QEvent::TouchUpdate;
                break;
            case SCREEN_EVENT_MTOUCH_RELEASE:
                m_touchPoints[touchId].state = Qt::TouchPointReleased;
                type = QEvent::TouchEnd;
                break;
            }

            // build list of active touch points
            QList<QWindowSystemInterface::TouchPoint> pointList;
            for (int i = 0; i < MaximumTouchPoints; i++) {
                if (i == touchId) {
                    // current touch point is always active
                    pointList.append(m_touchPoints[i]);
                } else if (m_touchPoints[i].state != Qt::TouchPointReleased) {
                    // finger is down but did not move
                    m_touchPoints[i].state = Qt::TouchPointStationary;
                    pointList.append(m_touchPoints[i]);
                }
            }

            // inject event into Qt
            QWindowSystemInterface::handleTouchEvent(w, m_touchDevice, pointList);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt touch, w =" << w
                                << ", p=" << m_touchPoints[touchId].area.topLeft()
                                << ", t=" << type;
        }
    }
}

void QQnxScreenEventHandler::handleCloseEvent(screen_event_t event)
{
    screen_window_t window = 0;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window),
            "Failed to query window property");

    Q_EMIT windowClosed(window);

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::window(window);
    if (w != 0)
        QWindowSystemInterface::handleCloseEvent(w);
}

void QQnxScreenEventHandler::handleCreateEvent(screen_event_t event)
{
    screen_window_t window = 0;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window),
            "Failed to query window property");

    Q_EMIT newWindowCreated(window);
}

void QQnxScreenEventHandler::handleDisplayEvent(screen_event_t event)
{
    screen_display_t nativeDisplay = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_DISPLAY, (void **)&nativeDisplay) != 0) {
        qWarning("QQnx: failed to query display property, errno=%d", errno);
        return;
    }

    int isAttached = 0;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_ATTACHED, &isAttached) != 0) {
        qWarning("QQnx: failed to query display attached property, errno=%d", errno);
        return;
    }

    qScreenEventDebug() << Q_FUNC_INFO << "display attachment is now:" << isAttached;
    QQnxScreen *screen = m_qnxIntegration->screenForNative(nativeDisplay);

    if (!screen) {
        if (isAttached) {
            int val[2];
            screen_get_display_property_iv(nativeDisplay, SCREEN_PROPERTY_SIZE, val);
            if (val[0] == 0 && val[1] == 0) //If screen size is invalid, wait for the next event
                return;

            qScreenEventDebug() << "creating new QQnxScreen for newly attached display";
            m_qnxIntegration->createDisplay(nativeDisplay, false /* not primary, we assume */);
        }
    } else if (!isAttached) {
        // We never remove the primary display, the qpa plugin doesn't support that and it crashes.
        // To support it, this would be needed:
        // - Adjust all qnx qpa code which uses screens
        // - Make QWidgetBackingStore not dereference a null paint device
        // - Create platform resources ( QQnxWindow ) for all QWindow because they would be deleted
        //   when you delete the screen

        if (!screen->isPrimaryScreen()) {
            // libscreen display is deactivated, let's remove the QQnxScreen / QScreen
            qScreenEventDebug() << "removing display";
            m_qnxIntegration->removeDisplay(screen);
        }
    }
}

void QQnxScreenEventHandler::handlePropertyEvent(screen_event_t event)
{
    errno = 0;
    int objectType;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE, &objectType),
            "Failed to query object type property");

    if (objectType != SCREEN_OBJECT_TYPE_WINDOW)
        return;

    errno = 0;
    screen_window_t window = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0)
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    errno = 0;
    int property;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_NAME, &property) != 0)
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    switch (property) {
    case SCREEN_PROPERTY_KEYBOARD_FOCUS:
        handleKeyboardFocusPropertyEvent(window);
        break;
    default:
        // event ignored
        qScreenEventDebug() << Q_FUNC_INFO << "Ignore property event for property: " << property;
    }
}

void QQnxScreenEventHandler::handleKeyboardFocusPropertyEvent(screen_window_t window)
{
    errno = 0;
    int focus = 0;
    if (window && screen_get_window_property_iv(window, SCREEN_PROPERTY_KEYBOARD_FOCUS, &focus) != 0)
        qFatal("QQnx: failed to query keyboard focus property, errno=%d", errno);

    QWindow *focusWindow = QQnxIntegration::window(window);

    if (m_focusLostTimer != -1) {
        killTimer(m_focusLostTimer);
        m_focusLostTimer = -1;
    }

    if (focus && focusWindow != QGuiApplication::focusWindow())
        QWindowSystemInterface::handleWindowActivated(focusWindow);
    else if (!focus && focusWindow == QGuiApplication::focusWindow())
        m_focusLostTimer = startTimer(50);
}

void QQnxScreenEventHandler::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_focusLostTimer) {
        killTimer(m_focusLostTimer);
        m_focusLostTimer = -1;
        event->accept();
    } else {
        QObject::timerEvent(event);
    }
}

#include "moc_qqnxscreeneventhandler.cpp"

QT_END_NAMESPACE
