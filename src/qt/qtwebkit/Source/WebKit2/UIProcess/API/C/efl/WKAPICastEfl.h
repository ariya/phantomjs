/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKAPICastEfl_h
#define WKAPICastEfl_h

#ifndef WKAPICast_h
#error "Please #include \"WKAPICast.h\" instead of this file directly."
#endif

#include <WebCore/TextDirection.h>
#include <WebKit2/WKPopupItem.h>

#if ENABLE(TOUCH_EVENTS)
#include "WebEvent.h"
#include <WebKit2/WKEventEfl.h>
#endif

namespace WebKit {

class WebView;
class WebPopupItemEfl;
class WebPopupMenuListenerEfl;

WK_ADD_API_MAPPING(WKViewRef, WebView)
WK_ADD_API_MAPPING(WKPopupItemRef, WebPopupItemEfl)
WK_ADD_API_MAPPING(WKPopupMenuListenerRef, WebPopupMenuListenerEfl)

#if ENABLE(TOUCH_EVENTS)
class EwkTouchEvent;
class EwkTouchPoint;

WK_ADD_API_MAPPING(WKTouchEventRef, EwkTouchEvent)
WK_ADD_API_MAPPING(WKTouchPointRef, EwkTouchPoint)
#endif

// Enum conversions.
inline WKPopupItemTextDirection toAPI(WebCore::TextDirection direction)
{
    WKPopupItemTextDirection wkDirection = kWKPopupItemTextDirectionLTR;

    switch (direction) {
    case WebCore::RTL:
        wkDirection = kWKPopupItemTextDirectionRTL;
        break;
    case WebCore::LTR:
        wkDirection = kWKPopupItemTextDirectionLTR;
        break;
    }

    return wkDirection;
}

#if ENABLE(TOUCH_EVENTS)
inline WKEventType toAPI(WebEvent::Type type)
{
    switch (type) {
    case WebEvent::TouchStart:
        return kWKEventTypeTouchStart;
    case WebEvent::TouchMove:
        return kWKEventTypeTouchMove;
    case WebEvent::TouchEnd:
        return kWKEventTypeTouchEnd;
    case WebEvent::TouchCancel:
        return kWKEventTypeTouchCancel;
    default:
        return kWKEventTypeNoType;
    }
}

inline WKTouchPointState toAPI(WebPlatformTouchPoint::TouchPointState state)
{
    switch (state) {
    case WebPlatformTouchPoint::TouchReleased:
        return kWKTouchPointStateTouchReleased;
    case WebPlatformTouchPoint::TouchPressed:
        return kWKTouchPointStateTouchPressed;
    case WebPlatformTouchPoint::TouchMoved:
        return kWKTouchPointStateTouchMoved;
    case WebPlatformTouchPoint::TouchStationary:
        return kWKTouchPointStateTouchStationary;
    case WebPlatformTouchPoint::TouchCancelled:
        return kWKTouchPointStateTouchCancelled;
    }

    ASSERT_NOT_REACHED();
    return kWKTouchPointStateTouchCancelled;
}
#endif

}

#endif // WKAPICastEfl_h
