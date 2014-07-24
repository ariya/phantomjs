/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012-2013 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef WKEventEfl_h
#define WKEventEfl_h

#include <WebKit2/WKEvent.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

enum WKEventType {
    kWKEventTypeNoType = -1,
    kWKEventTypeTouchStart,
    kWKEventTypeTouchMove,
    kWKEventTypeTouchEnd,
    kWKEventTypeTouchCancel
};
typedef enum WKEventType WKEventType;

enum WKTouchPointState {
    kWKTouchPointStateTouchReleased,
    kWKTouchPointStateTouchPressed,
    kWKTouchPointStateTouchMoved,
    kWKTouchPointStateTouchStationary,
    kWKTouchPointStateTouchCancelled
};
typedef enum WKTouchPointState WKTouchPointState;

WK_EXPORT WKTouchPointRef WKTouchPointCreate(int id, WKPoint position, WKPoint screenPosition, WKTouchPointState, WKSize radius, float rotationAngle, float forceFactor);
WK_EXPORT WKTouchEventRef WKTouchEventCreate(WKEventType, WKArrayRef, WKEventModifiers, double timestamp);

WK_EXPORT WKEventType WKTouchEventGetType(WKTouchEventRef);
WK_EXPORT WKArrayRef WKTouchEventGetTouchPoints(WKTouchEventRef);
WK_EXPORT WKEventModifiers WKTouchEventGetModifiers(WKTouchEventRef);
WK_EXPORT double WKTouchEventGetTimestamp(WKTouchEventRef);

WK_EXPORT uint32_t WKTouchPointGetID(WKTouchPointRef);
WK_EXPORT WKTouchPointState WKTouchPointGetState(WKTouchPointRef);
WK_EXPORT WKPoint WKTouchPointGetScreenPosition(WKTouchPointRef);
WK_EXPORT WKPoint WKTouchPointGetPosition(WKTouchPointRef);
WK_EXPORT WKSize WKTouchPointGetRadius(WKTouchPointRef);
WK_EXPORT float WKTouchPointGetRotationAngle(WKTouchPointRef);
WK_EXPORT float WKTouchPointGetForceFactor(WKTouchPointRef);

#ifdef __cplusplus
}
#endif

#endif /* WKEventEfl_h */
