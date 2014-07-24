/*
 * Copyright (C) 2011 Samsung Electronics
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

#ifndef WebEventFactory_h
#define WebEventFactory_h

#include "WebEvent.h"
#include <Evas.h>
#include <WebCore/AffineTransform.h>

namespace WebKit {

#if ENABLE(TOUCH_EVENTS)
class EwkTouchEvent;
#endif

class WebEventFactory {
public:
    static WebMouseEvent createWebMouseEvent(const Evas_Event_Mouse_Down*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    static WebMouseEvent createWebMouseEvent(const Evas_Event_Mouse_Up*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    static WebMouseEvent createWebMouseEvent(const Evas_Event_Mouse_Move*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    static WebWheelEvent createWebWheelEvent(const Evas_Event_Mouse_Wheel*, const WebCore::AffineTransform&, const WebCore::AffineTransform&);
    static WebKeyboardEvent createWebKeyboardEvent(const Evas_Event_Key_Down*);
    static WebKeyboardEvent createWebKeyboardEvent(const Evas_Event_Key_Up*);
#if ENABLE(TOUCH_EVENTS)
    static WebTouchEvent createWebTouchEvent(const EwkTouchEvent*, const WebCore::AffineTransform&);
#endif
};

} // namespace WebKit

#endif // WebEventFactory_h
