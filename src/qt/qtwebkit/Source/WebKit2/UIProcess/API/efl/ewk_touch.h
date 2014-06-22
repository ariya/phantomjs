/*
 * Copyright (C) 2012 Samsung Electronics
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

#ifndef ewk_touch_h
#define ewk_touch_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Represents types of touch event.
typedef enum {
    EWK_TOUCH_START,
    EWK_TOUCH_MOVE,
    EWK_TOUCH_END,
    EWK_TOUCH_CANCEL
} Ewk_Touch_Event_Type;

/// Creates a type name for Ewk_Touch_Point.
typedef struct Ewk_Touch_Point Ewk_Touch_Point;

/// Represents a touch point.
struct Ewk_Touch_Point {
    int id; /**< identifier of the touch event */
    int x; /**< the horizontal position of the touch event */
    int y; /**< the vertical position of the touch event */
    Evas_Touch_Point_State state; /**< state of the touch event */
};

#ifdef __cplusplus
}
#endif

#endif // ewk_touch_h
