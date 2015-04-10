/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef NativeWebWheelEvent_h
#define NativeWebWheelEvent_h

#include "WebEvent.h"

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
OBJC_CLASS NSView;
#elif PLATFORM(QT)
#include <qevent.h>
#elif PLATFORM(GTK)
#include <GOwnPtrGtk.h>
typedef union _GdkEvent GdkEvent;
#elif PLATFORM(EFL)
#include <Evas.h>
#include <WebCore/AffineTransform.h>
#endif

namespace WebKit {

class NativeWebWheelEvent : public WebWheelEvent {
public:
#if USE(APPKIT)
    NativeWebWheelEvent(NSEvent *, NSView *);
#elif PLATFORM(QT)
    explicit NativeWebWheelEvent(QWheelEvent*, const QTransform& fromItemTransform);
#elif PLATFORM(GTK)
    NativeWebWheelEvent(const NativeWebWheelEvent&);
    NativeWebWheelEvent(GdkEvent*);
#elif PLATFORM(EFL)
    NativeWebWheelEvent(const Evas_Event_Mouse_Wheel*, const WebCore::AffineTransform& toWebContent, const WebCore::AffineTransform& toDeviceScreen);
#endif

#if USE(APPKIT)
    NSEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(QT)
    const QWheelEvent* nativeEvent() const { return m_nativeEvent; }
#elif PLATFORM(GTK)
    const GdkEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(EFL)
    const Evas_Event_Mouse_Wheel* nativeEvent() const { return m_nativeEvent; }
#endif

private:
#if USE(APPKIT)
    RetainPtr<NSEvent> m_nativeEvent;
#elif PLATFORM(QT)
    QWheelEvent* m_nativeEvent;
#elif PLATFORM(GTK)
    GOwnPtr<GdkEvent> m_nativeEvent;
#elif PLATFORM(EFL)
    const Evas_Event_Mouse_Wheel* m_nativeEvent;
#endif
};

} // namespace WebKit

#endif // NativeWebWheelEvent_h
