/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#include "WebEventFactoryQt.h"
#include <QKeyEvent>
#include <QLineF>
#include <QTransform>
#include <WebCore/FloatPoint.h>
#include <WebCore/FloatSize.h>
#include <WebCore/IntPoint.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <wtf/ASCIICType.h>
#include <wtf/CurrentTime.h>

using namespace WebCore;

namespace WebKit {

static inline double currentTimeForEvent(const QInputEvent* event)
{
    ASSERT(event);

    // Use the input event timestamps if they are available.
    // These timestamps are in milliseconds, thus convert them to seconds.
    if (event->timestamp())
        return static_cast<double>(event->timestamp()) / 1000;

    return WTF::currentTime();
}

static WebMouseEvent::Button mouseButtonForEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || (event->buttons() & Qt::LeftButton))
        return WebMouseEvent::LeftButton;
    else if (event->button() == Qt::RightButton || (event->buttons() & Qt::RightButton))
        return WebMouseEvent::RightButton;
    else if (event->button() == Qt::MidButton || (event->buttons() & Qt::MidButton))
        return WebMouseEvent::MiddleButton;
    return WebMouseEvent::NoButton;
}

static WebEvent::Type webEventTypeForEvent(const QEvent* event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return WebEvent::MouseDown;
    case QEvent::MouseButtonRelease:
        return WebEvent::MouseUp;
    case QEvent::MouseMove:
        return WebEvent::MouseMove;
    case QEvent::Wheel:
        return WebEvent::Wheel;
    case QEvent::KeyPress:
        return WebEvent::KeyDown;
    case QEvent::KeyRelease:
        return WebEvent::KeyUp;
#if ENABLE(TOUCH_EVENTS)
    case QEvent::TouchBegin:
        return WebEvent::TouchStart;
    case QEvent::TouchUpdate:
        return WebEvent::TouchMove;
    case QEvent::TouchEnd:
        return WebEvent::TouchEnd;
    case QEvent::TouchCancel:
        return WebEvent::TouchCancel;
#endif
    case QEvent::MouseButtonDblClick:
        ASSERT_NOT_REACHED();
        return WebEvent::NoType;
    default:
        // assert
        return WebEvent::MouseMove;
    }
}

static inline WebEvent::Modifiers modifiersForEvent(Qt::KeyboardModifiers modifiers)
{
    unsigned result = 0;
    if (modifiers & Qt::ShiftModifier)
        result |= WebEvent::ShiftKey;
    if (modifiers & Qt::ControlModifier)
        result |= WebEvent::ControlKey;
    if (modifiers & Qt::AltModifier)
        result |= WebEvent::AltKey;
    if (modifiers & Qt::MetaModifier)
        result |= WebEvent::MetaKey;
    return (WebEvent::Modifiers)result;
}

WebMouseEvent WebEventFactory::createWebMouseEvent(QMouseEvent* event, const QTransform& fromItemTransform, int eventClickCount)
{
    static FloatPoint lastPos = FloatPoint(0, 0);

    WebEvent::Type type             = webEventTypeForEvent(event);
    WebMouseEvent::Button button    = mouseButtonForEvent(event);
    float deltaX                    = event->pos().x() - lastPos.x();
    float deltaY                    = event->pos().y() - lastPos.y();
    int clickCount                  = eventClickCount;
    WebEvent::Modifiers modifiers   = modifiersForEvent(event->modifiers());
    double timestamp                = currentTimeForEvent(event);
    lastPos.set(event->localPos().x(), event->localPos().y());

    return WebMouseEvent(type, button, fromItemTransform.map(event->localPos()).toPoint(), event->screenPos().toPoint(), deltaX, deltaY, 0.0f, clickCount, modifiers, timestamp);
}

WebWheelEvent WebEventFactory::createWebWheelEvent(QWheelEvent* e, const QTransform& fromItemTransform)
{
    float deltaX                            = 0;
    float deltaY                            = 0;
    float wheelTicksX                       = 0;
    float wheelTicksY                       = 0;
    WebWheelEvent::Granularity granularity  = WebWheelEvent::ScrollByPixelWheelEvent;
    WebEvent::Modifiers modifiers           = modifiersForEvent(e->modifiers());
    double timestamp                        = currentTimeForEvent(e);

    if (e->orientation() == Qt::Horizontal) {
        deltaX = e->delta();
        wheelTicksX = deltaX / 120.0f;
    } else {
        deltaY = e->delta();
        wheelTicksY = deltaY / 120.0f;
    }

    // Since we report the scroll by the pixel, convert the delta to pixel distance using standard scroll step.
    // Use the same single scroll step as QTextEdit (in QTextEditPrivate::init [h,v]bar->setSingleStep)
    static const float cDefaultQtScrollStep = 20.f;
    // ### FIXME: Default from QtGui. Should use Qt platform theme API once configurable.
    const int wheelScrollLines = 3;
    deltaX = wheelTicksX * wheelScrollLines * cDefaultQtScrollStep;
    deltaY = wheelTicksY * wheelScrollLines * cDefaultQtScrollStep;

    // Transform the position and the pixel scrolling distance.
    QLineF transformedScroll = fromItemTransform.map(QLineF(e->posF(), e->posF() + QPointF(deltaX, deltaY)));
    IntPoint transformedPoint = transformedScroll.p1().toPoint();
    IntPoint globalPoint = e->globalPosF().toPoint();
    FloatSize transformedDelta(transformedScroll.dx(), transformedScroll.dy());
    FloatSize wheelTicks(wheelTicksX, wheelTicksY);
    return WebWheelEvent(WebEvent::Wheel, transformedPoint, globalPoint, transformedDelta, wheelTicks, granularity, modifiers, timestamp);
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(QKeyEvent* event)
{
    const int state                 = event->modifiers();
    WebEvent::Type type             = webEventTypeForEvent(event);
    const String text               = event->text();
    const String unmodifiedText     = event->text();
    bool isAutoRepeat               = event->isAutoRepeat();
    bool isSystemKey                = false; // FIXME: No idea what that is.
    bool isKeypad                   = (state & Qt::KeypadModifier);
    const String keyIdentifier      = keyIdentifierForQtKeyCode(event->key());
    int windowsVirtualKeyCode       = windowsKeyCodeForKeyEvent(event->key(), isKeypad);
    int nativeVirtualKeyCode        = event->nativeVirtualKey();
    int macCharCode                 = 0;
    WebEvent::Modifiers modifiers   = modifiersForEvent(event->modifiers());
    double timestamp                = currentTimeForEvent(event);

    return WebKeyboardEvent(type, text, unmodifiedText, keyIdentifier, windowsVirtualKeyCode, nativeVirtualKeyCode, macCharCode, isAutoRepeat, isKeypad, isSystemKey, modifiers, timestamp);
}

#if ENABLE(TOUCH_EVENTS)
WebTouchEvent WebEventFactory::createWebTouchEvent(const QTouchEvent* event, const QTransform& fromItemTransform)
{
    WebEvent::Type type  = webEventTypeForEvent(event);
    WebPlatformTouchPoint::TouchPointState state = static_cast<WebPlatformTouchPoint::TouchPointState>(0);
    unsigned int id;
    WebEvent::Modifiers modifiers   = modifiersForEvent(event->modifiers());
    double timestamp                = currentTimeForEvent(event);

    const QList<QTouchEvent::TouchPoint>& points = event->touchPoints();
    
    Vector<WebPlatformTouchPoint, 6> m_touchPoints;
    for (int i = 0; i < points.count(); ++i) {
        const QTouchEvent::TouchPoint& touchPoint = points.at(i);
        id = static_cast<unsigned>(touchPoint.id());
        switch (touchPoint.state()) {
        case Qt::TouchPointReleased: 
            state = WebPlatformTouchPoint::TouchReleased; 
            break;
        case Qt::TouchPointMoved: 
            state = WebPlatformTouchPoint::TouchMoved; 
            break;
        case Qt::TouchPointPressed: 
            state = WebPlatformTouchPoint::TouchPressed; 
            break;
        case Qt::TouchPointStationary: 
            state = WebPlatformTouchPoint::TouchStationary; 
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        // Qt does not have a Qt::TouchPointCancelled point state, so if we receive a touch cancel event,
        // simply cancel all touch points here.
        if (type == WebEvent::TouchCancel)
            state = WebPlatformTouchPoint::TouchCancelled;

        IntSize radius(touchPoint.rect().width()/ 2, touchPoint.rect().height() / 2);
        m_touchPoints.append(WebPlatformTouchPoint(id, state, touchPoint.screenPos().toPoint(), fromItemTransform.map(touchPoint.pos()).toPoint(), radius, 0.0, touchPoint.pressure()));
    }

    return WebTouchEvent(type, m_touchPoints, modifiers, timestamp);
}
#endif

} // namespace WebKit
