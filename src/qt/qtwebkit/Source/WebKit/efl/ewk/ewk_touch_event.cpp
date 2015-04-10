/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TOUCH_EVENTS)

#include "PlatformTouchPoint.h"
#include "ewk_touch_event_private.h"
#include <Ecore_Input.h>
#include <wtf/CurrentTime.h>

class WebKitPlatformTouchPoint : public WebCore::PlatformTouchPoint {
public:
    WebKitPlatformTouchPoint(unsigned id, const WebCore::IntPoint& windowPos, WebCore::PlatformTouchPoint::State state)
    {
        m_id = id;
        m_state = state;
        m_screenPos = windowPos;
        m_pos = windowPos;
    }
};

class WebKitPlatformTouchEvent : public WebCore::PlatformTouchEvent {
public:
    WebKitPlatformTouchEvent(const Eina_List* points, const WebCore::IntPoint& pos, Ewk_Touch_Event_Type action, unsigned modifiers)
    {
        switch (action) {
        case EWK_TOUCH_START:
            m_type = WebCore::PlatformEvent::TouchStart;
            break;
        case EWK_TOUCH_MOVE:
            m_type = WebCore::PlatformEvent::TouchMove;
            break;
        case EWK_TOUCH_END:
            m_type = WebCore::PlatformEvent::TouchEnd;
            break;
        case EWK_TOUCH_CANCEL:
            m_type = WebCore::PlatformEvent::TouchCancel;
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        if (modifiers & ECORE_EVENT_MODIFIER_ALT)
            m_modifiers |= WebCore::PlatformEvent::AltKey;
        if (modifiers & ECORE_EVENT_MODIFIER_CTRL)
            m_modifiers |= WebCore::PlatformEvent::CtrlKey;
        if (modifiers & ECORE_EVENT_MODIFIER_SHIFT)
            m_modifiers |= WebCore::PlatformEvent::ShiftKey;
        if (modifiers & ECORE_EVENT_MODIFIER_WIN)
            m_modifiers |= WebCore::PlatformEvent::MetaKey;

        m_timestamp = currentTime();

        const Eina_List* list;
        void* item;
        EINA_LIST_FOREACH(points, list, item) {
            Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(item);
            WebCore::IntPoint pnt = WebCore::IntPoint(point->x - pos.x(), point->y - pos.y());
            m_touchPoints.append(WebKitPlatformTouchPoint(point->id, pnt, static_cast<WebCore::PlatformTouchPoint::State>(point->state)));
        }
    }
};

namespace EWKPrivate {

WebCore::PlatformTouchEvent platformTouchEvent(Evas_Coord x, Evas_Coord y, Eina_List* points, Ewk_Touch_Event_Type action, unsigned modifiers)
{
    return WebKitPlatformTouchEvent(points, WebCore::IntPoint(x, y), action, modifiers);
}

}

#endif // ENABLE(TOUCH_EVENTS)
