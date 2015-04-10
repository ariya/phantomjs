/*
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
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

#ifndef NativeWebTouchEvent_h
#define NativeWebTouchEvent_h

#include "WebEvent.h"

#if PLATFORM(QT)
#include <QTouchEvent>
#elif PLATFORM(EFL)
#include "EwkTouchEvent.h"
#include <WebCore/AffineTransform.h>
#include <wtf/RefPtr.h>
#endif

namespace WebKit {

class NativeWebTouchEvent : public WebTouchEvent {
public:
#if PLATFORM(QT)
    explicit NativeWebTouchEvent(const QTouchEvent*, const QTransform& fromItemTransform);
#elif PLATFORM(EFL)
    NativeWebTouchEvent(EwkTouchEvent*, const WebCore::AffineTransform&);
#endif

#if PLATFORM(QT)
    const QTouchEvent* nativeEvent() const { return &m_nativeEvent; }
#elif PLATFORM(EFL)
    const EwkTouchEvent* nativeEvent() const { return m_nativeEvent.get(); }
#endif

private:
#if PLATFORM(QT)
    const QTouchEvent m_nativeEvent;
#elif PLATFORM(EFL)
    RefPtr<EwkTouchEvent> m_nativeEvent;
#endif
};

} // namespace WebKit

#endif // NativeWebTouchEvent_h
