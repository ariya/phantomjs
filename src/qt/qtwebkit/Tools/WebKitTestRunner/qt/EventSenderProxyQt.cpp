/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventSenderProxy.h"

#include "PlatformWebView.h"
#include "TestController.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QtTest/QtTest>
#include <WebKit2/WKPagePrivate.h>
#include <WebKit2/WKStringQt.h>
#include <qpa/qwindowsysteminterface.h>

namespace WTR {

#define KEYCODE_DEL         127
#define KEYCODE_BACKSPACE   8
#define KEYCODE_LEFTARROW   0xf702
#define KEYCODE_RIGHTARROW  0xf703
#define KEYCODE_UPARROW     0xf700
#define KEYCODE_DOWNARROW   0xf701

struct WTREventQueue {
    QEvent* m_event;
    int m_delay;
};

static WTREventQueue eventQueue[1024];
static unsigned endOfQueue;
static bool isReplayingEvents;

EventSenderProxy::EventSenderProxy(TestController* testController)
    : m_testController(testController)
    , m_time(0)
    , m_position()
    , m_leftMouseButtonDown(false)
    , m_clickCount(0)
    , m_clickTime(0)
    , m_clickPosition()
    , m_clickButton(kWKEventMouseButtonNoButton)
    , m_mouseButtons(0)
#if ENABLE(TOUCH_EVENTS)
    , m_touchActive(false)
#endif
{
    memset(eventQueue, 0, sizeof(eventQueue));
    endOfQueue = 0;
    isReplayingEvents = false;
}

EventSenderProxy::~EventSenderProxy()
{
}

static Qt::MouseButton getMouseButton(unsigned button)
{
    Qt::MouseButton mouseButton;
    switch (button) {
    case 0:
        mouseButton = Qt::LeftButton;
        break;
    case 1:
        mouseButton = Qt::MidButton;
        break;
    case 2:
        mouseButton = Qt::RightButton;
        break;
    case 3:
        // fast/events/mouse-click-events expects the 4th button to be treated as the middle button
        mouseButton = Qt::MidButton;
        break;
    default:
        mouseButton = Qt::LeftButton;
        break;
    }
    return mouseButton;
}

static Qt::KeyboardModifiers getModifiers(WKEventModifiers modifiersRef)
{
    Qt::KeyboardModifiers modifiers = 0;

    if (modifiersRef & kWKEventModifiersControlKey)
        modifiers |= Qt::ControlModifier;
    if (modifiersRef & kWKEventModifiersShiftKey)
        modifiers |= Qt::ShiftModifier;
    if (modifiersRef & kWKEventModifiersAltKey)
        modifiers |= Qt::AltModifier;
    if (modifiersRef & kWKEventModifiersMetaKey)
        modifiers |= Qt::MetaModifier;

    return modifiers;
}

void EventSenderProxy::keyDown(WKStringRef keyRef, WKEventModifiers modifiersRef, unsigned location)
{
    const QString key = WKStringCopyQString(keyRef);
    QString keyText = key;

    Qt::KeyboardModifiers modifiers = getModifiers(modifiersRef);

    if (location == 3)
        modifiers |= Qt::KeypadModifier;
    int code = 0;
    if (key.length() == 1) {
        code = key.unicode()->unicode();
        // map special keycodes used by the tests to something that works for Qt/X11
        if (code == '\r') {
            code = Qt::Key_Return;
        } else if (code == '\t') {
            code = Qt::Key_Tab;
            if (modifiers == Qt::ShiftModifier)
                code = Qt::Key_Backtab;
            keyText = QStringLiteral("\t");
        } else if (code == KEYCODE_DEL || code == KEYCODE_BACKSPACE) {
            code = Qt::Key_Backspace;
            if (modifiers == Qt::AltModifier)
                modifiers = Qt::ControlModifier;
            keyText = QString();
        } else if (code == 'o' && modifiers == Qt::ControlModifier) {
            // Mimic the emacs ctrl-o binding on Mac by inserting a paragraph
            // separator and then putting the cursor back to its original
            // position. Allows us to pass emacs-ctrl-o.html
            keyText = QLatin1String("\n");
            code = '\n';
            modifiers = 0;
            QKeyEvent event(QEvent::KeyPress, code, modifiers, keyText);
            m_testController->mainWebView()->sendEvent(&event);
            QKeyEvent event2(QEvent::KeyRelease, code, modifiers, keyText);
            m_testController->mainWebView()->sendEvent(&event2);
            keyText = QString();
            code = Qt::Key_Left;
        } else if (code == 'y' && modifiers == Qt::ControlModifier) {
            keyText = QLatin1String("c");
            code = 'c';
        } else if (code == 'k' && modifiers == Qt::ControlModifier) {
            keyText = QLatin1String("x");
            code = 'x';
        } else if (code == 'a' && modifiers == Qt::ControlModifier) {
            keyText = QString();
            code = Qt::Key_Home;
            modifiers = 0;
        } else if (code == KEYCODE_LEFTARROW) {
            keyText = QString();
            code = Qt::Key_Left;
            if (modifiers & Qt::MetaModifier) {
                code = Qt::Key_Home;
                modifiers &= ~Qt::MetaModifier;
            }
        } else if (code == KEYCODE_RIGHTARROW) {
            keyText = QString();
            code = Qt::Key_Right;
            if (modifiers & Qt::MetaModifier) {
                code = Qt::Key_End;
                modifiers &= ~Qt::MetaModifier;
            }
        } else if (code == KEYCODE_UPARROW) {
            keyText = QString();
            code = Qt::Key_Up;
            if (modifiers & Qt::MetaModifier) {
                code = Qt::Key_PageUp;
                modifiers &= ~Qt::MetaModifier;
            }
        } else if (code == KEYCODE_DOWNARROW) {
            keyText = QString();
            code = Qt::Key_Down;
            if (modifiers & Qt::MetaModifier) {
                code = Qt::Key_PageDown;
                modifiers &= ~Qt::MetaModifier;
            }
        } else
            code = key.unicode()->toUpper().unicode();
    } else {
        if (key.startsWith(QLatin1Char('F')) && key.count() <= 3) {
            keyText = keyText.mid(1);
            int functionKey = keyText.toInt();
            Q_ASSERT(functionKey >= 1 && functionKey <= 35);
            code = Qt::Key_F1 + (functionKey - 1);
        // map special keycode strings used by the tests to something that works for Qt/X11
        } else if (key == QLatin1String("leftArrow")) {
            keyText = QString();
            code = Qt::Key_Left;
        } else if (key == QLatin1String("rightArrow")) {
            keyText = QString();
            code = Qt::Key_Right;
        } else if (key == QLatin1String("upArrow")) {
            keyText = QString();
            code = Qt::Key_Up;
        } else if (key == QLatin1String("downArrow")) {
            keyText = QString();
            code = Qt::Key_Down;
        } else if (key == QLatin1String("pageUp")) {
            keyText = QString();
            code = Qt::Key_PageUp;
        } else if (key == QLatin1String("pageDown")) {
            keyText = QString();
            code = Qt::Key_PageDown;
        } else if (key == QLatin1String("home")) {
            keyText = QString();
            code = Qt::Key_Home;
        } else if (key == QLatin1String("end")) {
            keyText = QString();
            code = Qt::Key_End;
        } else if (key == QLatin1String("insert")) {
            keyText = QString();
            code = Qt::Key_Insert;
        } else if (key == QLatin1String("delete")) {
            keyText = QString();
            code = Qt::Key_Delete;
        } else if (key == QLatin1String("printScreen")) {
            keyText = QString();
            code = Qt::Key_Print;
        } else if (key == QLatin1String("menu")) {
            keyText = QString();
            code = Qt::Key_Menu;
        }
    }

    QKeyEvent* pressEvent = new QKeyEvent(QEvent::KeyPress, code, modifiers, keyText);
    sendOrQueueEvent(pressEvent);
    QKeyEvent* releaseEvent = new QKeyEvent(QEvent::KeyRelease, code, modifiers, keyText);
    sendOrQueueEvent(releaseEvent);

}

void EventSenderProxy::updateClickCountForButton(int button)
{
    if (m_time - m_clickTime < QApplication::doubleClickInterval() && m_position == m_clickPosition && button == m_clickButton) {
        m_clickTime = m_time;
        return;
    }

    m_clickTime = m_time;
    m_clickPosition = m_position;
    m_clickButton = button;
}

void EventSenderProxy::mouseDown(unsigned button, WKEventModifiers wkModifiers)
{
    Qt::KeyboardModifiers modifiers = getModifiers(wkModifiers);
    Qt::MouseButton mouseButton = getMouseButton(button);

    updateClickCountForButton(button);

    m_mouseButtons |= mouseButton;

    QPoint mousePos(m_position.x, m_position.y);
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonPress,
        mousePos, mousePos, mouseButton, m_mouseButtons, modifiers);

    // We aren't generating MouseButtonDblClick events as they aren't used.
    sendOrQueueEvent(event);
}

void EventSenderProxy::mouseUp(unsigned button, WKEventModifiers)
{
    Qt::MouseButton mouseButton = getMouseButton(button);
    m_mouseButtons &= ~mouseButton;

    QPoint mousePos(m_position.x, m_position.y);
    QMouseEvent* event = new QMouseEvent(QEvent::MouseButtonRelease,
        mousePos, mousePos, mouseButton, m_mouseButtons, Qt::NoModifier);

    sendOrQueueEvent(event);
}

void EventSenderProxy::mouseMoveTo(double x, double y)
{
    m_position.x = x;
    m_position.y = y;

    QPoint mousePos(m_position.x, m_position.y);
    QMouseEvent* event = new QMouseEvent(QEvent::MouseMove,
        mousePos, mousePos, Qt::NoButton, m_mouseButtons, Qt::NoModifier);

    sendOrQueueEvent(event);
}

void EventSenderProxy::mouseScrollBy(int, int)
{
    // FIXME: Implement this.
}

void EventSenderProxy::continuousMouseScrollBy(int, int, bool)
{
    // FIXME: Implement this.
}

void EventSenderProxy::leapForward(int ms)
{
    eventQueue[endOfQueue].m_delay = ms;
    m_time += ms;
}

#if ENABLE(TOUCH_EVENTS)
void EventSenderProxy::addTouchPoint(int x, int y)
{
    const int id = m_touchPoints.isEmpty() ? 0 : m_touchPoints.last().id() + 1;
    const QPointF pos(x, y);
    QTouchEvent::TouchPoint point(id);
    point.setPos(pos);
    point.setStartPos(pos);
    point.setState(Qt::TouchPointPressed);
    if (!m_touchPointRadius.isNull())
        point.setRect(QRectF(pos - m_touchPointRadius, pos + m_touchPointRadius));
    m_touchPoints.append(point);
}

void EventSenderProxy::updateTouchPoint(int index, int x, int y)
{
    ASSERT(index >= 0 && index < m_touchPoints.count());
    QPointF pos(x, y);
    QTouchEvent::TouchPoint &p = m_touchPoints[index];
    p.setPos(pos);
    p.setState(Qt::TouchPointMoved);
    if (!m_touchPointRadius.isNull())
        p.setRect(QRectF(pos - m_touchPointRadius, pos + m_touchPointRadius));
}

void EventSenderProxy::setTouchModifier(WKEventModifiers modifier, bool enable)
{
    Qt::KeyboardModifiers mod = getModifiers(modifier);

    if (enable)
        m_touchModifiers |= mod;
    else
        m_touchModifiers &= ~mod;
}

void EventSenderProxy::setTouchPointRadius(int radiusX, int radiusY)
{
    m_touchPointRadius = QPoint(radiusX, radiusY);
}

void EventSenderProxy::touchStart()
{
    if (!m_touchActive) {
        sendTouchEvent(QEvent::TouchBegin);
        m_touchActive = true;
    } else
        sendTouchEvent(QEvent::TouchUpdate);
}

void EventSenderProxy::touchMove()
{
    sendTouchEvent(QEvent::TouchUpdate);
}

void EventSenderProxy::touchEnd()
{
    for (int i = 0; i < m_touchPoints.count(); ++i) {
        if (m_touchPoints[i].state() != Qt::TouchPointReleased) {
            sendTouchEvent(QEvent::TouchUpdate);
            return;
        }
    }
    sendTouchEvent(QEvent::TouchEnd);
    m_touchActive = false;
}

void EventSenderProxy::touchCancel()
{
    sendTouchEvent(QEvent::TouchCancel);
    m_touchActive = false;
}

void EventSenderProxy::clearTouchPoints()
{
    m_touchPoints.clear();
    m_touchModifiers = Qt::KeyboardModifiers();
    m_touchActive = false;
    m_touchPointRadius = QPoint();
}

void EventSenderProxy::releaseTouchPoint(int index)
{
    if (index < 0 || index >= m_touchPoints.count())
        return;

    m_touchPoints[index].setState(Qt::TouchPointReleased);
}

void EventSenderProxy::cancelTouchPoint(int index)
{
    // FIXME: No cancellation state in Qt 5, mapped to release instead.
    // PlatformTouchEvent conversion later will map all touch points to
    // cancelled.
    releaseTouchPoint(index);
}

void EventSenderProxy::sendTouchEvent(QEvent::Type type)
{
    static QTouchDevice* device = 0;
    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }

    Qt::TouchPointStates eventStates;
    for (int i = 0; i < m_touchPoints.count(); i++)
        eventStates |= m_touchPoints[i].state();
    QTouchEvent event(type, device, m_touchModifiers, eventStates);
    event.setTouchPoints(m_touchPoints);
    m_testController->mainWebView()->sendEvent(&event);
    QList<QTouchEvent::TouchPoint>::Iterator it = m_touchPoints.begin();
    while (it != m_touchPoints.end()) {
        if (it->state() == Qt::TouchPointReleased)
            it = m_touchPoints.erase(it);
        else {
            it->setState(Qt::TouchPointStationary);
            ++it;
        }
    }
}
#endif

void EventSenderProxy::sendOrQueueEvent(QEvent* event)
{
    if (!endOfQueue && !eventQueue[endOfQueue].m_delay) {
        m_testController->mainWebView()->sendEvent(event);
        delete event;
        return;
    }

    eventQueue[endOfQueue++].m_event = event;
    replaySavedEvents();
}

void EventSenderProxy::replaySavedEvents()
{
    if (isReplayingEvents)
        return;

    isReplayingEvents = true;
    int i = 0;

    while (i < endOfQueue) {
        WTREventQueue& ev = eventQueue[i];
        if (ev.m_delay)
            QTest::qWait(ev.m_delay);
        i++;
        m_testController->mainWebView()->sendEvent(ev.m_event);
        delete ev.m_event;
        ev.m_delay = 0;
    }

    endOfQueue = 0;
    isReplayingEvents = false;
}

} // namespace WTR
