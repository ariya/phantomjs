/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef EventSendingController_h
#define EventSendingController_h

#include "JSWrappable.h"
#include <WebKit2/WKEvent.h>
#include <WebKit2/WKGeometry.h>
#include <wtf/PassRefPtr.h>

#if !PLATFORM(MAC) && !PLATFORM(QT) && !PLATFORM(GTK) && !PLATFORM(EFL)
#define USE_WEBPROCESS_EVENT_SIMULATION
#endif

namespace WTR {

class EventSendingController : public JSWrappable {
public:
    static PassRefPtr<EventSendingController> create();
    virtual ~EventSendingController();

    void makeWindowObject(JSContextRef, JSObjectRef windowObject, JSValueRef* exception);

    // JSWrappable
    virtual JSClassRef wrapperClass();

    void mouseDown(int button, JSValueRef modifierArray);
    void mouseUp(int button, JSValueRef modifierArray);
    void mouseMoveTo(int x, int y);
    void mouseScrollBy(int x, int y);
    void continuousMouseScrollBy(int x, int y, bool paged);
    JSValueRef contextClick();
    void leapForward(int milliseconds);
    void scheduleAsynchronousClick();

    void keyDown(JSStringRef key, JSValueRef modifierArray, int location);
    void scheduleAsynchronousKeyDown(JSStringRef key);

    // Zoom functions.
    void textZoomIn();
    void textZoomOut();
    void zoomPageIn();
    void zoomPageOut();
    void scalePageBy(double scale, double x, double y);

#if ENABLE(TOUCH_EVENTS)
    // Touch events.
    void addTouchPoint(int x, int y);
    void updateTouchPoint(int index, int x, int y);
    void setTouchModifier(const JSStringRef &modifier, bool enable);
    void setTouchPointRadius(int radiusX, int radiusY);
    void touchStart();
    void touchMove();
    void touchEnd();
    void touchCancel();
    void clearTouchPoints();
    void releaseTouchPoint(int index);
    void cancelTouchPoint(int index);
#endif

private:
    EventSendingController();

#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    void updateClickCount(WKEventMouseButton);

    double m_time;
    WKPoint m_position;

    int m_clickCount;
    double m_clickTime;
    WKPoint m_clickPosition;
    WKEventMouseButton m_clickButton;
#endif
};

} // namespace WTR

#endif // EventSendingController_h
