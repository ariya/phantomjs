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

#ifndef NativeWebMouseEvent_h
#define NativeWebMouseEvent_h

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

class NativeWebMouseEvent : public WebMouseEvent {
public:
#if USE(APPKIT)
    NativeWebMouseEvent(NSEvent *, NSView *);
#elif PLATFORM(QT)
    explicit NativeWebMouseEvent(QMouseEvent*, const QTransform& fromItemTransform, int eventClickCount);
#elif PLATFORM(GTK)
    NativeWebMouseEvent(const NativeWebMouseEvent&);
    NativeWebMouseEvent(GdkEvent*, int);
#elif PLATFORM(EFL)
    NativeWebMouseEvent(const Evas_Event_Mouse_Down*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    NativeWebMouseEvent(const Evas_Event_Mouse_Up*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    NativeWebMouseEvent(const Evas_Event_Mouse_Move*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
#endif

#if USE(APPKIT)
    NSEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(QT)
    const QMouseEvent* nativeEvent() const { return m_nativeEvent; }
#elif PLATFORM(GTK)
    const GdkEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(EFL)
    const void* nativeEvent() const { return m_nativeEvent; }
#endif

private:
#if USE(APPKIT)
    RetainPtr<NSEvent> m_nativeEvent;
#elif PLATFORM(QT)
    QMouseEvent* m_nativeEvent;
#elif PLATFORM(GTK)
    GOwnPtr<GdkEvent> m_nativeEvent;
#elif PLATFORM(EFL)
    const void* m_nativeEvent;
#endif
};

} // namespace WebKit

#endif // NativeWebMouseEvent_h
